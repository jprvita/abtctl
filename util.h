#ifndef __UTIL_H__
#define __UTIL_H__

#include <hardware/bluetooth.h>

#define BT_ADDRESS_STR_LEN 18

int str2ba(const char *str, bt_bdaddr_t *ba);
/* Needs a buffer of at least BT_ADDRESS_STR_LEN bytes */
char *ba2str(const uint8_t *ba, char *str);

int str_in_list(const char* list[], const char *str);

#endif /* __UTIL_H__ */
