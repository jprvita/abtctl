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

char *uuid2str(bt_uuid_t *uuid, char *str) {

    /* format: 11223344-5566-7788-9900-112233445566 */
    sprintf(str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
            "%02x%02x%02x%02x%02x%02x", uuid->uu[15], uuid->uu[14],
            uuid->uu[13], uuid->uu[12], uuid->uu[11], uuid->uu[10], uuid->uu[9],
            uuid->uu[8], uuid->uu[7], uuid->uu[6], uuid->uu[5], uuid->uu[4],
            uuid->uu[3], uuid->uu[2], uuid->uu[1], uuid->uu[0]);
    return str;
}

bool str2uuid(const char *str, bt_uuid_t *uuid) {
    /* base UUID used to convert small ones */
    bt_uuid_t _uuid = {.uu = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
                              0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    int ret;

    switch (strlen(str)) {
        case 6: /* 16-bits */
            ret = sscanf(str, "0x%02hhx%02hhx", &_uuid.uu[13], &_uuid.uu[12]);
            if (ret != 2)
                return false;
            break;
        case 36: /* 128-bits */
            ret = sscanf(str, "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-"
                         "%02hhx%02hhx-%02hhx%02hhx-"
                         "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &_uuid.uu[15],
                         &_uuid.uu[14], &_uuid.uu[13], &_uuid.uu[12],
                         &_uuid.uu[11], &_uuid.uu[10], &_uuid.uu[9],
                         &_uuid.uu[8], &_uuid.uu[7], &_uuid.uu[6], &_uuid.uu[5],
                         &_uuid.uu[4], &_uuid.uu[3], &_uuid.uu[2], &_uuid.uu[1],
                         &_uuid.uu[0]);
            if (ret != 16)
                return false;
            break;
        default:
            return false;
    }

    memcpy(uuid, &_uuid, sizeof(_uuid));
    return true;
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
