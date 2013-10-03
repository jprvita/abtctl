#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <hardware/bluetooth.h>

#include "util.h"

static int bachk(const char *str) {
    if (!str)
        return -1;

    if (strlen(str) != 17)
        return -1;

    while (*str) {
        if (!isxdigit(*str++))
            return -1;

            if (!isxdigit(*str++))
                return -1;

            if (*str == 0)
                break;

            if (*str++ != ':')
                return -1;
    }

    return 0;
}

int str2ba(const char *str, bt_bdaddr_t *ba) {
    int i;

    if (bachk(str) < 0) {
        memset(ba, 0, sizeof(*ba));
            return -1;
    }

    for (i = 5; i >= 0; i--, str += 3)
        ba->address[5-i] = strtol(str, NULL, 16);

    return 0;
}

char *ba2str(const uint8_t *ba, char *str) {

    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X", ba[0], ba[1], ba[2], ba[3],
            ba[4], ba[5]);
    return str;
}

int str_in_list(const char* list[], const char *str) {

    unsigned i = 0;

    if (list == NULL || str == NULL)
        return -1;

    while (list[i] != NULL) {
        if (strcmp(list[i], str) == 0)
            return i;

        i++;
    }

    return -1;
}
