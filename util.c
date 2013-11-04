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

const char *atterror2str(int err) {

    switch (err) {
        case 0x00:
            return "Success";
        case 0x01:
            return "Invalid Handle";
        case 0x02:
            return "Read Not Permitted";
        case 0x03:
            return "Write Not Permitted";
        case 0x04:
            return "Invalid PDU";
        case 0x05:
            return "Insufficient Authentication";
        case 0x06:
            return "Request Not Supported";
        case 0x07:
            return "Invalid Offset";
        case 0x08:
            return "Insufficient Authorization";
        case 0x09:
            return "Prepare Queue Full";
        case 0x0a:
            return "Attribute Not Found";
        case 0x0b:
            return "Attribute Not Long";
        case 0x0c:
            return "Insufficient Encryption Key Size";
        case 0x0d:
            return "Invalid Attribute Value Length";
        case 0x0e:
            return "Unlikely Error";
        case 0x0f:
            return "Insufficient Encryption";
        case 0x10:
            return "Unsupported Group Type";
        case 0x11:
            return "Insufficient Resources";

        /* Bluedroid defined errors */
        /* They are defined in bluedroid source at stack/include/gatt_api.h */
        case 0x80:
            return "No Resources";
        case 0x81:
            return "Internal Error";
        case 0x82:
            return "Wrong State";
        case 0x83:
            return "DB Full";
        case 0x84:
            return "Busy";
        case 0x85:
            return "Error";
        case 0x86:
            return "Command Started";
        case 0x87:
            return "Illegal Parameter";
        case 0x88:
            return "Pending";
        case 0x89:
            return "Auth Fail";
        case 0x8a:
            return "More";
        case 0x8b:
            return "Invalid Config";
        case 0x8c:
            return "Service Started";
        case 0x8d:
            return "Encrypted No MITM";
        case 0x8e:
            return "Not Encrypted";

        default:
            if (err & 0x80)
                return "Application Error";
            else
                return "Reserved";
    }
}
