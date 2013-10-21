#ifndef __RL_HELPER_H__
#define __RL_HELPER_H__

typedef void (*line_process_callback)(char *line);

/* initializes buffers and set line process callback */
void rl_init(line_process_callback cb);
/* close resources */
void rl_quit();
/* add char to line buffer */
void rl_feed(int c);

#endif // __RL_HELPER_H__
