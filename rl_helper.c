/*
 * Line editing helper
 *
 * Copyright (C) 2013 Jefferson Delfes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include "rl_helper.h"

#define MAX_LINE_BUFFER 512
#define MAX_SEQ 5

#define MIN(a, b) \
    ({ \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b; \
    })

typedef enum {
    K_ESC       = 0x1b,
    K_BACKSPACE = 0x7f,
    /* user defined codes (used for escape sequences */
    K_UP    = 0x100,
    K_DOWN  = 0x101,
    K_RIGHT = 0x102,
    K_LEFT  = 0x103,
    K_END   = 0x104,
    K_HOME  = 0x105,
    K_DELETE = 0x106,
} keys_t;

line_process_callback line_cb;
char lnbuf[MAX_LINE_BUFFER]; /* buffer for our line editing */
size_t pos = 0;
char seq[MAX_SEQ]; /* sequence buffer (escape codes) */
size_t seq_pos = 0;
const char *prompt = "> ";

typedef struct {
    char sequence[MAX_SEQ];
    int code;
} char_sequence;

/* code sequence definitions */
const char_sequence seqs[] = {
    {
        .sequence = {K_ESC, '[', 'A'},
        .code = K_UP
    },
    {
        .sequence = {K_ESC, '[', 'B'},
        .code = K_DOWN
    },
    {
        .sequence = {K_ESC, '[', 'C'},
        .code = K_RIGHT
    },
    {
        .sequence = {K_ESC, '[', 'D'},
        .code = K_LEFT
    },
    {
        .sequence = {K_ESC, 'O', 'F'},
        .code = K_END
    },
    {
        .sequence = {K_ESC, 'O', 'H'},
        .code = K_HOME
    },
    {
        .sequence = {K_ESC, '[', '3', '~'},
        .code = K_DELETE
    },
};

void rl_clear_seq() {

    memset(seq, 0, sizeof(seq));
    seq_pos = 0;
}

void rl_clear() {

    rl_clear_seq();
    memset(lnbuf, 0, sizeof(lnbuf));
    pos = 0;
}

/* clear current line and returns to line begin */
void rl_clear_line() {

    printf("\x1b[2K\r");
}

void rl_reprint_prompt() {
    static size_t viewport_pos = 0;
    size_t terminal_cols = 80;
    size_t len = strlen(lnbuf);
    size_t viewport_size = terminal_cols - strlen(prompt) - 1;
    size_t viewport_end;

    rl_clear_line();

    if (pos < viewport_pos) /* cursor before viewport */
        viewport_pos = pos;
    if (pos > viewport_pos + viewport_size) /* cursor after viewport */
        viewport_pos = pos - viewport_size;

    printf("%s%.*s", prompt, MIN(viewport_size, len), lnbuf + viewport_pos);

    viewport_end = MIN(viewport_pos + viewport_size, len);
    while (viewport_end-- != pos)
        putchar('\b'); /* backspace */
    fflush(stdout);
}

void rl_init(line_process_callback cb) {
    struct termios settings;

    /* disable echo */
    tcgetattr(0, &settings); /* read settings from stdin (0) */
    settings.c_lflag &= ~(ICANON | ECHO); /* disable canonical and echo flags */
    tcsetattr(0, TCSANOW, &settings); /* store new settings */

    rl_clear();
    line_cb = cb;
}

void rl_set_prompt(const char *str) {

    prompt = str;
    rl_reprint_prompt();
}

void rl_quit() {

    rl_clear();
    rl_clear_line();
}

/* returns 1 if char was consumed, 0 otherwise */
int rl_parse_seq(int *c) {

    if (seq_pos == 0) {
        /* starts a sequence of chars */
        if (*c == K_ESC) {
            seq[seq_pos++] = *c;
            return 1;
        } else
            return 0;
    } else {
        size_t i;
        if (seq_pos >= sizeof(seq)) {
            /* we lost the sequence */
            rl_clear_seq();
            return 0;
        }

        seq[seq_pos++] = *c;
        for (i = 0; i < sizeof(seqs) / sizeof(char_sequence); i++) {
            if (memcmp(seq, seqs[i].sequence, sizeof(seq)) == 0) {
                /* found sequence, return special code */
                *c = seqs[i].code;
                rl_clear_seq();
                return 0;
            }
        }
        return 1;
    }
}

void rl_feed(int c) {

    if (rl_parse_seq(&c))
        return;

    switch (c) {
        case '\r':
        case '\n':
            putchar('\n');
            line_cb(lnbuf);
            rl_clear();
            rl_reprint_prompt();
            break;
        case K_ESC:
            break;
        case K_BACKSPACE:
            if (pos > 0) {
                memmove(lnbuf + pos - 1, lnbuf + pos, sizeof(lnbuf) - pos);
                pos--;
                rl_reprint_prompt();
            }
            break;
        case K_UP:
        case K_DOWN:
            /* history handle */
            break;
        case K_RIGHT:
            if (pos < strlen(lnbuf)) {
                pos++;
                rl_reprint_prompt();
            }
            break;
        case K_LEFT:
            if (pos > 0) {
                pos--;
                rl_reprint_prompt();
            }
            break;
        case K_END:
            pos = strlen(lnbuf);
            rl_reprint_prompt();
            break;
        case K_HOME:
            pos = 0;
            rl_reprint_prompt();
            break;
        case K_DELETE:
            memmove(lnbuf + pos, lnbuf + pos + 1, sizeof(lnbuf) - pos - 1);
            rl_reprint_prompt();
            break;
        default:
            if (isprint(c)) {
                if (pos < (sizeof(lnbuf) - 1)) {
                    /* shift everything to right and insert char at pos */
                    memmove(lnbuf + pos + 1, lnbuf + pos,
                            sizeof(lnbuf) - pos - 1);
                    lnbuf[pos++] = c;
                    rl_reprint_prompt();
                }
            } else
                printf(" %x ", c);
            break;
    }
}

void rl_printf(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    rl_clear_line();
    vprintf(fmt, ap);
    va_end(ap);

    rl_reprint_prompt();
}
