#ifndef __UTIL_H__
#define __UTIL_H__

#include <hardware/bluetooth.h>

int str2ba(const char *str, bt_bdaddr_t *ba);

#endif /* __UTIL_H__ */
