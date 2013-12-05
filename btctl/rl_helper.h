#ifndef __RL_HELPER_H__
#define __RL_HELPER_H__

typedef void (*line_process_callback)(char *line);

typedef const char *(*tab_completer_callback)(char *line, int tab_pos);

/* initializes buffers and set line process callback */
void rl_init(line_process_callback cb);
/* configure prompt string, (eg "> ") */
void rl_set_prompt(const char *str);
/* tab completer */
void rl_set_tab_completer(tab_completer_callback cb);
/* close resources */
void rl_quit();
/* add char to line buffer, returns false on ctrl-d */
bool rl_feed(int c);
/* printf version */
void rl_printf(const char *fmt, ...);

#endif // __RL_HELPER_H__
