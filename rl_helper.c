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

typedef enum {
    K_ESC       = 0x1b,
    K_BACKSPACE = 0x7f,
    /* user defined codes (used for escape sequences */
    K_UP    = 0x100,
    K_DOWN  = 0x101,
    K_RIGHT = 0x102,
    K_LEFT  = 0x103,
} keys_t;

line_process_callback line_cb;
char lnbuf[MAX_LINE_BUFFER]; /* buffer for our line editing */
size_t pos = 0;

void rl_clear() {

    memset(lnbuf, 0, sizeof(lnbuf));
    pos = 0;
}

/* clear current line and returns to line begin */
void rl_clear_line() {

    printf("\x1b[2K\r");
}

void rl_reprint_prompt() {

    rl_clear_line();
    printf("> %s", lnbuf);
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

void rl_quit() {

    rl_clear();
    rl_clear_line();
}

void rl_feed(int c) {

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
                rl_reprint_prompt();
                pos--;
            }
            break;
        case K_UP:
        case K_DOWN:
            /* history handle */
            break;
        case K_RIGHT:
        case K_LEFT:
            break;
        default:
            if (isprint(c)) {
                if (pos < MAX_LINE_BUFFER) {
                    lnbuf[pos++] = c;
                    rl_reprint_prompt();
                }
            } else
                printf(" %x ", c);
            break;
    }
}
