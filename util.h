#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>
#include <hardware/bluetooth.h>

#define BT_ADDRESS_STR_LEN 18
#define UUID128_STR_LEN 16*2+5

int str2ba(const char *str, bt_bdaddr_t *ba);
/* Needs a buffer of at least BT_ADDRESS_STR_LEN bytes */
char *ba2str(const uint8_t *ba, char *str);

/* Needs a buffer of a least UUID128_STR_LEN bytes */
char *uuid2str(bt_uuid_t *uuid, char *str);
/* Accepts 16 or 128 bits. Return true on success */
bool str2uuid(const char *str, bt_uuid_t *uuid);

int str_in_list(const char* list[], const char *str);

#endif /* __UTIL_H__ */
