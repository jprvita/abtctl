/*
 *  Android Bluetooth Control tool
 *
 *  Copyright (C) 2013 João Paulo Rechi Vita
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hardware/bluetooth.h>
#include <hardware/bt_gatt.h>
#include <hardware/bt_gatt_client.h>
#include <hardware/hardware.h>

#include "util.h"
#include "rl_helper.h"

#define VERSION "0.3"

#define MAX_LINE_SIZE 64
#define MAX_SVCS_SIZE 128
#define MAX_CHARS_SIZE 8
#define MAX_CONNECTIONS 10
#define PENDING_CONN_ID  0
#define INVALID_CONN_ID -1

/* AD types */
#define AD_FLAGS              0x01
#define AD_UUID16_SOME        0x02
#define AD_UUID16_ALL         0x03
#define AD_UUID128_SOME       0x06
#define AD_UUID128_ALL        0x07
#define AD_NAME_SHORT         0x08
#define AD_NAME_COMPLETE      0x09
#define AD_TX_POWER           0x0a
#define AD_SLAVE_CONN_INT     0x12
#define AD_SOLICIT_UUID16     0x14
#define AD_SOLICIT_UUID128    0x15
#define AD_SERVICE_DATA       0x16
#define AD_PUBLIC_ADDRESS     0x17
#define AD_RANDOM_ADDRESS     0x18
#define AD_GAP_APPEARANCE     0x19
#define AD_ADV_INTERVAL       0x1a
#define AD_MANUFACTURER_DATA  0xff

typedef enum {
    NORMAL_PSTATE,
    SSP_CONSENT_PSTATE,
    SSP_ENTRY_PSTATE
} prompt_state_t;

typedef struct char_info {
    btgatt_char_id_t char_id;
    bt_uuid_t *descrs;
    uint8_t descr_count;
} char_info_t;

typedef struct service_info {
    btgatt_srvc_id_t svc_id;
    char_info_t *chars_buf;
    uint8_t chars_buf_size;
    uint8_t char_count;
} service_info_t;

typedef struct connection {
    bt_bdaddr_t remote_addr;
    int conn_id;

    /* When searching for services, we receive at search_result_cb a pointer
     * for btgatt_srvc_id_t. But its value is replaced each time. So one option
     * is to store these values and show a simpler ID to user.
     *
     * This static list limits the number of services that we can store, but it
     * is simpler than using linked list.
     */
    service_info_t svcs[MAX_SVCS_SIZE];
    int svcs_size;
} connection_t;

/* Data that have to be acessable by the callbacks */
struct userdata {
    const bt_interface_t *btiface;
    uint8_t btiface_initialized;
    const btgatt_interface_t *gattiface;
    uint8_t gattiface_initialized;
    uint8_t quit;
    bt_state_t adapter_state; /* The adapter is always OFF in the beginning */
    bt_discovery_state_t discovery_state;
    uint8_t scan_state;
    bool client_registered;
    int client_if;
    bt_bdaddr_t remote_addr;
    int conn_id;

    prompt_state_t prompt_state;
    bt_bdaddr_t r_bd_addr; /* remote address when pairing */

    /* When searching for services, we receive at search_result_cb a pointer
     * for btgatt_srvc_id_t. But its value is replaced each time. So one option
     * is to store these values and show a simpler ID to user.
     *
     * This static list limits the number of services that we can store, but it
     * is simpler than using linked list.
     */
    service_info_t svcs[MAX_SVCS_SIZE];
    int svcs_size;

    connection_t conns[MAX_CONNECTIONS];
} u;

/* Arbitrary UUID used to identify this application with the GATT library. The
 * Android JAVA framework
 * (frameworks/base/core/java/android/bluetooth/BluetoothAdapter.java,
 * frameworks/base/core/java/android/bluetooth/BluetoothGatt.java and
 * frameworks/base/core/java/android/bluetooth/BluetoothGattServer.java) uses
 * the method randomUUID()
 */
static bt_uuid_t app_uuid = {
    .uu = { 0x1b, 0x1c, 0xb9, 0x2e, 0x0d, 0x2e, 0x4c, 0x45, \
            0xbb, 0xb9, 0xf4, 0x1b, 0x46, 0x39, 0x23, 0x36 }
};

/* Struct that defines a user command */
typedef struct cmd {
    const char *name;
    const char *description;
    void (*handler)(char *args);
} cmd_t;


void change_prompt_state(prompt_state_t new_state) {
    static char prompt_line[64] = {0};
    char addr_str[BT_ADDRESS_STR_LEN];

    switch (new_state) {
        case NORMAL_PSTATE:
            strcpy(prompt_line, "> ");
            break;
        case SSP_CONSENT_PSTATE:
            sprintf(prompt_line, "Pair with %s (Y/N)? ",
                    ba2str(u.r_bd_addr.address, addr_str));
            break;
        case SSP_ENTRY_PSTATE:
            sprintf(prompt_line, "Entry PIN code of dev %s: ",
                    ba2str(u.r_bd_addr.address, addr_str));
            break;
    }
    rl_set_prompt(prompt_line);
    u.prompt_state = new_state;
}

static connection_t *get_connection(int conn_id)
{
    int i;

    if (conn_id <= INVALID_CONN_ID)
        return NULL;

    for (i = 0; i < MAX_CONNECTIONS; i++)
        if (u.conns[i].conn_id == conn_id)
            return &u.conns[i];

    return NULL;
}

/* clear any cache list of connected device */
static void clear_list_cache(int conn_id) {
    connection_t *conn;
    uint8_t i;

    conn = get_connection(conn_id);
    if (conn == NULL)
        return;

    for (i = 0; i < conn->svcs_size; i++)
        conn->svcs[i].char_count = 0;
    conn->svcs_size = 0;
}

static int find_svc(btgatt_srvc_id_t *svc) {
    uint8_t i;

    for (i = 0; i < u.svcs_size; i++)
        if (u.svcs[i].svc_id.is_primary == svc->is_primary &&
            u.svcs[i].svc_id.id.inst_id == svc->id.inst_id &&
            !memcmp(&u.svcs[i].svc_id.id.uuid, &svc->id.uuid,
                    sizeof(bt_uuid_t)))
            return i;
    return -1;
}

static int find_char(service_info_t *svc_info, btgatt_char_id_t *ch) {
    uint8_t i;

    for (i = 0; i < svc_info->char_count; i++) {
        btgatt_char_id_t *char_id = &svc_info->chars_buf[i].char_id;

        if (char_id->inst_id == ch->inst_id &&
            !memcmp(&char_id->uuid, &ch->uuid, sizeof(bt_uuid_t)))
            return i;
    }

    return -1;
}

/* Clean blanks until a non-blank is found */
static void line_skip_blanks(char **line) {
    while (**line == ' ')
        (*line)++;
}

/* Parses a sub-string out of a string */
static void line_get_str(char **line, char *str) {
  line_skip_blanks(line);

  while (**line != 0 && **line != ' ') {
        *str = **line;
        (*line)++;
        str++;
  }

  *str = 0;
}

static void cmd_quit(char *args) {
    u.quit = 1;
}

/* Called every time the adapter state changes */
static void adapter_state_change_cb(bt_state_t state) {

    u.adapter_state = state;
    rl_printf("\nAdapter state changed: %i\n", state);

    if (state == BT_STATE_ON) {
       /* Register as a GATT client with the stack
        *
        * This has to be done here because it is the first available point we're
        * sure the GATT interface is initialized and ready to be used, since
        * there is callback for gattiface->init().
        */
        bt_status_t status = u.gattiface->client->register_client(&app_uuid);
        if (status != BT_STATUS_SUCCESS)
            rl_printf("Failed to register as a GATT client, status: %d\n",
                      status);
    }
}

/* Enables the Bluetooth adapter */
static void cmd_enable(char *args) {
    int status;

    if (u.adapter_state == BT_STATE_ON) {
        rl_printf("Bluetooth is already enabled\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to enable Bluetooth Adapter: GATT interface not "
                  "available\n");
        return;
    }

    status = u.btiface->enable();
    if (status != BT_STATUS_SUCCESS)
        rl_printf("Failed to enable Bluetooth\n");
}

/* Disables the Bluetooth adapter */
static void cmd_disable(char *args) {
    bt_status_t result;
    int status;

    if (u.adapter_state == BT_STATE_OFF) {
        rl_printf("Bluetooth is already disabled\n");
        return;
    }

    result = u.gattiface->client->unregister_client(u.client_if);
    if (result != BT_STATUS_SUCCESS)
        rl_printf("Failed to unregister client, error: %u\n", result);

    status = u.btiface->disable();
    if (status != BT_STATUS_SUCCESS)
        rl_printf("Failed to disable Bluetooth\n");
}

static void adapter_properties_cb(bt_status_t status, int num_properties,
                                  bt_property_t *properties) {
    char addr_str[BT_ADDRESS_STR_LEN];
    int i;

    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to get adapter properties, error: %i\n", status);
        return;
    }

    rl_printf("\nAdapter properties\n");

    while (num_properties--) {
        bt_property_t prop = properties[num_properties];

        switch (prop.type) {
            case BT_PROPERTY_BDNAME:
                rl_printf("  Name: %s\n", (const char *) prop.val);
                break;

            case BT_PROPERTY_BDADDR:
                rl_printf("  Address: %s\n", ba2str((uint8_t *) prop.val,
                          addr_str));
                break;

            case BT_PROPERTY_CLASS_OF_DEVICE:
                rl_printf("  Class of Device: 0x%x\n",
                          ((uint32_t *) prop.val)[0]);
                break;

            case BT_PROPERTY_TYPE_OF_DEVICE:
                switch (((bt_device_type_t *) prop.val)[0]) {
                    case BT_DEVICE_DEVTYPE_BREDR:
                        rl_printf("  Device Type: BR/EDR only\n");
                        break;
                    case BT_DEVICE_DEVTYPE_BLE:
                        rl_printf("  Device Type: LE only\n");
                        break;
                    case BT_DEVICE_DEVTYPE_DUAL:
                        rl_printf("  Device Type: DUAL MODE\n");
                        break;
                }
                break;

            case BT_PROPERTY_ADAPTER_BONDED_DEVICES:
                i = prop.len / sizeof(bt_bdaddr_t);
                rl_printf("  Bonded devices: %u\n", i);
                while (i-- > 0) {
                    uint8_t *addr = ((bt_bdaddr_t *) prop.val)[i].address;
                    rl_printf("    Address: %s\n", ba2str(addr, addr_str));
                }
                break;

            default:
                /* Other properties not handled */
                break;
        }
    }
}

static void device_found_cb(int num_properties, bt_property_t *properties) {
    char addr_str[BT_ADDRESS_STR_LEN];

    rl_printf("\nDevice found\n");

    while (num_properties--) {
        bt_property_t prop = properties[num_properties];

        switch (prop.type) {
            case BT_PROPERTY_BDNAME:
                rl_printf("  name: %s\n", (const char *) prop.val);
                break;

            case BT_PROPERTY_BDADDR:
                rl_printf("  addr: %s\n", ba2str((uint8_t *) prop.val,
                          addr_str));
                break;

            case BT_PROPERTY_CLASS_OF_DEVICE:
                rl_printf("  class: 0x%x\n", ((uint32_t *) prop.val)[0]);
                break;

            case BT_PROPERTY_TYPE_OF_DEVICE:
                switch ( ((bt_device_type_t *) prop.val)[0] ) {
                    case BT_DEVICE_DEVTYPE_BREDR:
                        rl_printf("  type: BR/EDR only\n");
                        break;
                    case BT_DEVICE_DEVTYPE_BLE:
                        rl_printf("  type: LE only\n");
                        break;
                    case BT_DEVICE_DEVTYPE_DUAL:
                        rl_printf("  type: DUAL MODE\n");
                        break;
                }
                break;

            case BT_PROPERTY_REMOTE_FRIENDLY_NAME:
                rl_printf("  alias: %s\n", (const char *) prop.val);
                break;

            case BT_PROPERTY_REMOTE_RSSI:
                rl_printf("  rssi: %i\n", ((uint8_t *) prop.val)[0]);
                break;

            case BT_PROPERTY_REMOTE_VERSION_INFO:
                rl_printf("  version info:\n");
                rl_printf("    version: %d\n",
                          ((bt_remote_version_t *) prop.val)->version);
                rl_printf("    subversion: %d\n",
                          ((bt_remote_version_t *) prop.val)->sub_ver);
                rl_printf("    manufacturer: %d\n",
                          ((bt_remote_version_t *) prop.val)->manufacturer);
                break;

            default:
                rl_printf("  Unknown property type:%i len:%i val:%p\n",
                          prop.type, prop.len, prop.val);
                break;
        }
    }
}

static void discovery_state_changed_cb(bt_discovery_state_t state) {
    u.discovery_state = state;
    rl_printf("\nDiscovery state changed: %i\n", state);
}

static void cmd_discovery(char *args) {
    bt_status_t status;
    char arg[MAX_LINE_SIZE];

    line_get_str(&args, arg);

    if (arg[0] == 0 || strcmp(arg, "help") == 0) {
        rl_printf("discovery -- Controls discovery of nearby devices\n");
        rl_printf("Arguments:\n");
        rl_printf("start   starts a new discovery session\n");
        rl_printf("stop    interrupts an ongoing discovery session\n");

    } else if (strcmp(arg, "start") == 0) {

        if (u.adapter_state != BT_STATE_ON) {
            rl_printf("Unable to start discovery: Adapter is down\n");
            return;
        }

        if (u.discovery_state == BT_DISCOVERY_STARTED) {
            rl_printf("Discovery is already running\n");
            return;
        }

        status = u.btiface->start_discovery();
        if (status != BT_STATUS_SUCCESS)
            rl_printf("Failed to start discovery\n");

    } else if (strcmp(arg, "stop") == 0) {

        if (u.discovery_state == BT_DISCOVERY_STOPPED) {
            rl_printf("Unable to stop discovery: Discovery is not running\n");
            return;
        }

        status = u.btiface->cancel_discovery();
        if (status != BT_STATUS_SUCCESS)
            rl_printf("Failed to stop discovery\n");

    } else
        rl_printf("Invalid argument \"%s\"\n", arg);
}

static void parse_ad_data(uint8_t *data, uint8_t length) {
    uint8_t i = 0;
    uint8_t ad_type = data[i++];

    switch (ad_type) {
        uint8_t j;

        case AD_FLAGS: {
            uint8_t mask = data[i];
            static const struct {
                uint8_t bit;
                const char *str;
            } eir_flags_table[] = {
                {0, "LE Limited Discoverable Mode"},
                {1, "LE General Discoverable Mode"},
                {2, "BR/EDR Not Supported"},
                {3, "Simultaneous LE and BR/EDR (Controller)"},
                {4, "Simultaneous LE and BR/EDR (Host)"},
                {0xFF, NULL}
            };

            rl_printf("    Flags\n");

            for (j = 0; eir_flags_table[j].str; j++) {
                if (data[i] & (1 << eir_flags_table[j].bit)) {
                    rl_printf("      %s\n", eir_flags_table[j].str);
                    mask &= ~(1 << eir_flags_table[j].bit);
                }
            }

            if (mask)
                rl_printf("      Unknown flags (0x%02X)\n", mask);

            break;
        }
        case AD_UUID16_ALL:
        case AD_UUID16_SOME:
        case AD_SOLICIT_UUID16: {
            uint8_t count = (length - 1) / sizeof(uint16_t);
            const char *msg = NULL;

            switch (ad_type) {
                case AD_UUID16_ALL:
                    msg = "    Complete list of 16-bit Service UUIDs: ";
                    break;
                case AD_UUID16_SOME:
                    msg = "    Incomplete list of 16-bit Service UUIDs: ";
                    break;
                case AD_SOLICIT_UUID16:
                    msg = "    List of 16-bit Service Solicitation UUIDs: ";
                    break;
            }

            rl_printf("%s%u entr%s\n", msg, count, count == 1 ? "y" : "ies");

            for (j = 0; j < count; j++)
                rl_printf("      0x%02X%02X\n", data[i+j*sizeof(uint16_t)+1],
                          data[i+j*sizeof(uint16_t)]);

            break;
        }
        case AD_UUID128_ALL:
        case AD_UUID128_SOME:
        case AD_SOLICIT_UUID128: {
            uint8_t count = (length - 1) / 16;
            const char *msg = NULL;

            switch (ad_type) {
                case AD_UUID128_ALL:
                    msg = "    Complete list of 128-bit Service UUIDs: ";
                    break;
                case AD_UUID128_SOME:
                    msg = "    Incomplete list of 128-bit Service UUIDs: ";
                    break;
                case AD_SOLICIT_UUID128:
                    msg = "    List of 128-bit Service Solicitation UUIDs: ";
                    break;
            }

            rl_printf("%s%u entr%s\n", msg, count, count == 1 ? "y" : "ies");

            for (j = 0; j < count; j++)
                rl_printf("      %02X %02X %02X %02X %02X %02X %02X %02X %02X "
                          "%02X %02X %02X %02X %02X %02X %02X\n",
                          data[i+j*16+15], data[i+j*16+14], data[i+j*16+13],
                          data[i+j*16+12], data[i+j*16+11], data[i+j*16+10],
                          data[i+j*16+9], data[i+j*16+8], data[i+j*16+7],
                          data[i+j*16+6], data[i+j*16+5], data[i+j*16+4],
                          data[i+j*16+3], data[i+j*16+2], data[i+j*16+1],
                          data[i+j*16]);

            break;
        }
        case AD_NAME_SHORT:
        case AD_NAME_COMPLETE: {
            char name[length];

            memset(name, 0, sizeof(name));
            memcpy(name, &data[i], length-1);

            if (ad_type == AD_NAME_SHORT)
                rl_printf("    Shortened Local Name\n");
            else
                rl_printf("    Complete Local Name\n");

            rl_printf("      %s\n", name);

            break;
        }
        case AD_TX_POWER:
            rl_printf("    TX Power Level\n");
            rl_printf("      %d\n", (int8_t) data[i]);
            break;
        case AD_SLAVE_CONN_INT: {
            uint16_t min, max;

            rl_printf("    Slave Connection Interval\n");

            min = data[i] + (data[i+1] << 4);
            if (min >= 0x0006 && min <= 0x0c80)
                rl_printf("      Minimum = %.2f\n", (float) min * 1.25);

            max = data[i+2] + (data[i+3] << 4);
            if (max >= 0x0006 && max <= 0x0c80)
                rl_printf("      Maximum = %.2f\n", (float) max * 1.25);

            break;
        }
        case AD_SERVICE_DATA:
            rl_printf("    Service Data\n");
            break;
        case AD_PUBLIC_ADDRESS:
        case AD_RANDOM_ADDRESS:
            if (ad_type == AD_PUBLIC_ADDRESS)
                rl_printf("    Public Target Address\n");
            else
                rl_printf("    Random Target Address\n");

            rl_printf("      %02X:%02X:%02X:%02X:%02X:%02X\n", data[i+5],
                      data[i+4], data[i+3], data[i+2], data[i+1], data[i]);
            break;
        case AD_GAP_APPEARANCE:
            rl_printf("    Appearance\n");
            rl_printf("      0x%02X%02X\n", data[i+1], data[i]);
            break;
        case AD_ADV_INTERVAL: {
            uint16_t adv_interval;

            rl_printf("    Advertising Interval\n");

            adv_interval = data[i] + (data[i+1] << 4);
            rl_printf("      %.2f\n", (float) adv_interval * 0.625);

            break;
        }
        case AD_MANUFACTURER_DATA:
            rl_printf("    Manufacturer-specific data\n");
            rl_printf("      Company ID: 0x%02X%02X\n", data[i+1], data[i]);
            rl_printf("      Data:");
            for (j = i+2; j < i+length; j++)
                rl_printf(" %02X", data[j]);
            rl_printf("\n");
            break;
        default:
            rl_printf("    Invalid data type 0x%02X\n", ad_type);
            break;
    }
}

static void scan_result_cb(bt_bdaddr_t *bda, int rssi, uint8_t *adv_data) {
    char addr_str[BT_ADDRESS_STR_LEN];
    uint8_t i = 0;

    rl_printf("\nBLE device found\n");
    rl_printf("  Address: %s\n", ba2str(bda->address, addr_str));
    rl_printf("  RSSI: %d\n", rssi);

    rl_printf("  Advertising Data:\n");
    while (i < 31 && adv_data[i] != 0) {
        uint8_t length, ad_type, j;

        length = adv_data[i++];
        parse_ad_data(&adv_data[i], length);

        i += length;
    }
}

static void cmd_scan(char *args) {
    bt_status_t status;
    char arg[MAX_LINE_SIZE];

    if (u.gattiface == NULL) {
        rl_printf("Unable to start/stop BLE scan: GATT interface not "
                  "available\n");
        return;
    }

    line_get_str(&args, arg);

    if (arg[0] == 0 || strcmp(arg, "help") == 0) {
        rl_printf("scan -- Controls BLE scan of nearby devices\n");
        rl_printf("Arguments:\n");
        rl_printf("start   starts a new scan session\n");
        rl_printf("stop    interrupts an ongoing scan session\n");

    } else if (strcmp(arg, "start") == 0) {

        if (u.adapter_state != BT_STATE_ON) {
            rl_printf("Unable to start discovery: Adapter is down\n");
            return;
        }

        if (u.scan_state == 1) {
            rl_printf("Scan is already running\n");
            return;
        }

        status = u.gattiface->client->scan(u.client_if, 1);
        if (status != BT_STATUS_SUCCESS) {
            rl_printf("Failed to start discovery\n");
            return;
        }

        u.scan_state = 1;

    } else if (strcmp(arg, "stop") == 0) {

        if (u.scan_state == 0) {
            rl_printf("Unable to stop scan: Scan is not running\n");
            return;
        }

        status = u.gattiface->client->scan(u.client_if, 0);
        if (status != BT_STATUS_SUCCESS) {
            rl_printf("Failed to stop scan\n");
            return;
        }

        u.scan_state = 0;

    } else
        rl_printf("Invalid argument \"%s\"\n", arg);
}

static void connect_cb(int conn_id, int status, int client_if,
                       bt_bdaddr_t *bda) {
    char addr_str[BT_ADDRESS_STR_LEN];
    connection_t *conn;

    /* Get the space reserved on buffer (conn_id is zero) */
    conn = get_connection(PENDING_CONN_ID);
    if (conn == NULL) {
        rl_printf("No space reserved on buffer\n");
        return;
    }

    if (status != 0) {
        rl_printf("Failed to connect to device %s, status: %i\n",
                  ba2str(bda->address, addr_str), status);
        conn->conn_id = INVALID_CONN_ID;
        return;
    }

    rl_printf("Connected to device %s, conn_id: %d, client_if: %d\n",
              ba2str(bda->address, addr_str), conn_id, client_if);

    conn->conn_id = conn_id;
}

static void disconnect_cb(int conn_id, int status, int client_if,
                          bt_bdaddr_t *bda) {
    char addr_str[BT_ADDRESS_STR_LEN];
    connection_t *conn;

    rl_printf("Disconnected from device %s, conn_id: %d, client_if: %d, "
              "status: %d\n", ba2str(bda->address, addr_str), conn_id,
              client_if, status);

    conn = get_connection(conn_id);
    if (conn != NULL) {
        conn->conn_id = INVALID_CONN_ID;
        clear_list_cache(conn_id);
    }
}

static void cmd_disconnect(char *args) {
    bt_status_t status;
    connection_t *conn;
    int id;

    if (sscanf(args, " %i ", &id) != 1) {
        rl_printf("Usage: disconnect <connection ID>\n");
        return;
    }

    conn = get_connection(id);
    if (conn == NULL) {
        rl_printf("Invalid connection ID\n");
        return;
    }

    status = u.gattiface->client->disconnect(u.client_if, &conn->remote_addr,
                                             conn->conn_id);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to disconnect, status: %d\n", status);
        return;
    }

    if (id == PENDING_CONN_ID) {
        char addr_str[BT_ADDRESS_STR_LEN];

        conn->conn_id = INVALID_CONN_ID;
        rl_printf("Cancel pending connection: %s\n",
                  ba2str(conn->remote_addr.address, addr_str));
        clear_list_cache(id);
    }
}

void do_ssp_reply(const bt_bdaddr_t *bd_addr, bt_ssp_variant_t variant,
                  uint8_t accept, uint32_t passkey) {
    bt_status_t status = u.btiface->ssp_reply(bd_addr, variant, accept,
                                              passkey);

    if (status != BT_STATUS_SUCCESS) {
        rl_printf("SSP Reply error: %u\n", status);
        return;
    }
}

void pin_request_cb(bt_bdaddr_t *remote_bd_addr, bt_bdname_t *bd_name,
                    uint32_t cod) {

    /* ask user which PIN code is showed at remote device */
    memcpy(&u.r_bd_addr, remote_bd_addr, sizeof(u.r_bd_addr));
    change_prompt_state(SSP_ENTRY_PSTATE);
}

void ssp_request_cb(bt_bdaddr_t *remote_bd_addr, bt_bdname_t *bd_name,
                    uint32_t cod, bt_ssp_variant_t pairing_variant,
                    uint32_t pass_key) {

    if (pairing_variant == BT_SSP_VARIANT_CONSENT) {
        /* we need to ask to user if he wants to bond */
        memcpy(&u.r_bd_addr, remote_bd_addr, sizeof(u.r_bd_addr));
        change_prompt_state(SSP_CONSENT_PSTATE);
    } else {
        char addr_str[BT_ADDRESS_STR_LEN];
        const char *action = "Enter";

        if (pairing_variant == BT_SSP_VARIANT_PASSKEY_CONFIRMATION) {
            action = "Confirm";
            do_ssp_reply(remote_bd_addr, pairing_variant, true, pass_key);
        }

        rl_printf("Remote addr: %s\n",
                  ba2str(remote_bd_addr->address, addr_str));
        rl_printf("%s passkey on peer device: %d\n", action, pass_key);
    }
}

static void cmd_connect(char *args) {
    bt_status_t status;
    connection_t *conn = NULL;
    char arg[MAX_LINE_SIZE];
    int ret, i;

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE connect: GATT interface not available\n");
        return;
    }

    if (u.adapter_state != BT_STATE_ON) {
        rl_printf("Unable to connect: Adapter is down\n");
        return;
    }

    if (u.client_registered == false) {
        rl_printf("Unable to connect: We're not registered as GATT client\n");
        return;
    }

    /* Check if there is a pending connect (conn_id is zero) */
    conn = get_connection(PENDING_CONN_ID);
    if (conn != NULL) {
        rl_printf("Unable to connect: previous connecting on going\n");
        return;
    }

    /* Check if there is space available and save the index */
    for (i = 0; i < MAX_CONNECTIONS; i++)
        if (u.conns[i].conn_id == INVALID_CONN_ID) {
            conn = &u.conns[i];
            break;
        }

    /* No space available on buffer */
    if (conn == NULL) {
        rl_printf("Unable to connect: maximum number of connections "
                  "exceeded\n");
        return;
    }

    line_get_str(&args, arg);

    ret = str2ba(arg, &conn->remote_addr);
    if (ret != 0) {
        rl_printf("Unable to connect: Invalid bluetooth address: %s\n", arg);
        return;
    }

    rl_printf("Connecting to: %s\n", arg);

    status = u.gattiface->client->connect(u.client_if, &conn->remote_addr,
                                          true);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to connect, status: %d\n", status);
        return;
    }

    /* Lock the space on buffer */
    conn->conn_id = PENDING_CONN_ID;
}

static void bond_state_changed_cb(bt_status_t status, bt_bdaddr_t *bda,
                                  bt_bond_state_t state) {
    char addr_str[BT_ADDRESS_STR_LEN];
    char state_str[32] = {0};

    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to change bond state, status: %d\n", status);
        return;
    }

    switch (state) {
        case BT_BOND_STATE_NONE:
            strcpy(state_str, "BT_BOND_STATE_NONE");
            change_prompt_state(NORMAL_PSTATE); /* no bonding process running */
            break;

        case BT_BOND_STATE_BONDING:
            strcpy(state_str, "BT_BOND_STATE_BONDING");
            break;

        case BT_BOND_STATE_BONDED:
            strcpy(state_str, "BT_BOND_STATE_BONDED");
            break;

        default:
            sprintf(state_str, "Unknown (%d)", state);
            break;
    }

    rl_printf("Bond state changed for device %s: %s\n",
              ba2str(bda->address, addr_str), state_str);
}

static void cmd_pair(char *args) {
    char arg[MAX_LINE_SIZE];
    unsigned arg_pos;
    bt_bdaddr_t addr;
    bt_status_t status;
    int ret;
    const char* valid_arguments[] = {
        "create",
        "cancel",
        "remove",
        NULL,
    };

    if (u.btiface == NULL) {
        rl_printf("Unable to BLE pair: Bluetooth interface not available\n");
        return;
    }

    if (u.adapter_state != BT_STATE_ON) {
        rl_printf("Unable to pair: Adapter is down\n");
        return;
    }

    line_get_str(&args, arg);

    if (arg[0] == 0 || strcmp(arg, "help") == 0) {
        rl_printf("bond -- Controls BLE bond process\n");
        rl_printf("Arguments:\n");
        rl_printf("create <address>   start bond process to address\n");
        rl_printf("cancel <address>   cancel bond process to address\n");
        rl_printf("remove <address>   remove bond to address\n");
        return;
    }

    arg_pos = str_in_list(valid_arguments, arg);
    if (str_in_list(valid_arguments, arg) < 0) {
        rl_printf("Invalid argument \"%s\"\n", arg);
        return;
    }

    line_get_str(&args, arg);
    ret = str2ba(arg, &addr);
    if (ret != 0) {
        rl_printf("Invalid bluetooth address: %s\n", arg);
        return;
    }

    switch (arg_pos) {
        case 0:
            status = u.btiface->create_bond(&addr);
            if (status != BT_STATUS_SUCCESS) {
                rl_printf("Failed to create bond, status: %d\n", status);
                return;
            }
            break;
        case 1:
            status = u.btiface->cancel_bond(&addr);
            if (status != BT_STATUS_SUCCESS) {
                rl_printf("Failed to cancel bond, status: %d\n", status);
                return;
            }
            break;
        case 2:
            status = u.btiface->remove_bond(&addr);
            if (status != BT_STATUS_SUCCESS) {
                rl_printf("Failed to remove bond, status: %d\n", status);
                return;
            }
            break;
    }
}

/* called when search has finished */
void search_complete_cb(int conn_id, int status) {

    rl_printf("Search complete, status: %u\n", status);
}

/* called for each search result */
void search_result_cb(int conn_id, btgatt_srvc_id_t *srvc_id) {
    char uuid_str[UUID128_STR_LEN] = {0};
    connection_t *conn = get_connection(conn_id);

    if (conn->svcs_size < MAX_SVCS_SIZE) {
        /* srvc_id value is replaced each time, so we need to copy it */
        memcpy(&conn->svcs[conn->svcs_size].svc_id, srvc_id,
               sizeof(btgatt_srvc_id_t));
        conn->svcs_size++;
    }

    rl_printf("ID:%i %s UUID: %s instance:%i\n", conn->svcs_size - 1,
              srvc_id->is_primary ? "Primary" : "Secondary",
              uuid2str(&srvc_id->id.uuid, uuid_str), srvc_id->id.inst_id);
}

static void cmd_search_svc(char *args) {
    char arg[MAX_LINE_SIZE];
    bt_status_t status;
    connection_t *conn;
    int conn_id;

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE search-svc: GATT interface not avaiable\n");
        return;
    }

    /* Get connection ID argument */
    line_get_str(&args, arg);
    if (sscanf(arg, " %i ", &conn_id) != 1) {
        rl_printf("Usage: search-svc <connection ID> [UUID]\n");
        return;
    }
    conn = get_connection(conn_id);
    if (conn == NULL) {
        rl_printf("Invalid connection ID\n");
        return;
    } else if (conn_id == PENDING_CONN_ID) {
        rl_printf("Connection is not active\n");
        return;
    }

    clear_list_cache(conn_id);

    /* Get UUID argument (if exists) */
    line_get_str(&args, arg);
    if (strlen(arg) > 0) {
            bt_uuid_t uuid;
            if (!str2uuid(arg, &uuid)) {
                rl_printf("Invalid format of UUID: %s\n", arg);
                return;
            }

            status = u.gattiface->client->search_service(conn_id, &uuid);
    } else
            status = u.gattiface->client->search_service(conn_id, NULL);

    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to search services\n");
        return;
    }
}

void get_included_service_cb(int conn_id, int status, btgatt_srvc_id_t *srvc_id,
                             btgatt_srvc_id_t *incl_srvc_id) {

    if (status == 0) {
        bt_status_t ret;
        char uuid_str[UUID128_STR_LEN] = {0};

        rl_printf("Included UUID: %s\n",
                  uuid2str(&incl_srvc_id->id.uuid, uuid_str));

        /* this callback is called only one time, so to have next included
         * service we need to call get_included_service again using incl_srvc_id
         * as parameter
         */
        ret = u.gattiface->client->get_included_service(conn_id, srvc_id,
                                                        incl_srvc_id);
        if (ret != BT_STATUS_SUCCESS) {
            rl_printf("Failed to list included services\n");
            return;
        }
    } else
        rl_printf("Included finished, status: %i\n", status);
}

static void cmd_included(char *args) {
    char arg[MAX_LINE_SIZE];
    bt_status_t status;
    connection_t *conn;
    int conn_id, id;

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE included: GATT interface not avaiable\n");
        return;
    }

    if (sscanf(args, " %i %i", &conn_id, &id) != 2) {
        rl_printf("Usage: included <connection ID> <service ID>\n");
        return;
    }
    conn = get_connection(conn_id);
    if (conn == NULL) {
        rl_printf("Invalid connection ID\n");
        return;
    }

    if (conn->svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    if (id < 0 || id >= conn->svcs_size) {
        rl_printf("Invalid ID: %s need to be between 0 and %i\n", arg,
                  conn->svcs_size - 1);
        return;
    }

    /* get first included service */
    status = u.gattiface->client->get_included_service(conn->conn_id,
                                                       &conn->svcs[id].svc_id,
                                                       NULL);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to list included services\n");
        return;
    }
}

void get_characteristic_cb(int conn_id, int status, btgatt_srvc_id_t *srvc_id,
                           btgatt_char_id_t *char_id, int char_prop) {
    bt_status_t ret;
    char uuid_str[UUID128_STR_LEN] = {0};
    int svc_id;
    service_info_t *svc_info;

    if (status != 0) {
        if (status == 0x85) { /* it's not really an error, just finished */
            rl_printf("List characteristics finished\n");
            return;
        }

        rl_printf("List characteristics finished, status: %i %s\n", status,
                  atterror2str(status));
        return;
    }

    svc_id = find_svc(srvc_id);

    if (svc_id < 0) {
        rl_printf("Received invalid characteristic (service inexistent)\n");
        return;
    }
    svc_info = &u.svcs[svc_id];

    rl_printf("ID:%i UUID: %s instance:%i properties:0x%x\n",
              svc_info->char_count, uuid2str(&char_id->uuid, uuid_str),
              char_id->inst_id, char_prop);

    if (svc_info->char_count == svc_info->chars_buf_size) {
        int i;

        svc_info->chars_buf_size += MAX_CHARS_SIZE;
        svc_info->chars_buf = realloc(svc_info->chars_buf, sizeof(char_info_t) *
                                      svc_info->chars_buf_size);

        for (i = svc_info->char_count; i < svc_info->chars_buf_size; i++) {
            svc_info->chars_buf[i].descrs = NULL;
            svc_info->chars_buf[i].descr_count = 0;
        }
    }

    /* copy characteristic data */
    memcpy(&svc_info->chars_buf[svc_info->char_count].char_id, char_id,
           sizeof(btgatt_char_id_t));
    svc_info->chars_buf[svc_info->char_count].descr_count = 0;

    svc_info->char_count++;

    /* get next characteristic */
    ret = u.gattiface->client->get_characteristic(u.conn_id, srvc_id, char_id);
    if (ret != BT_STATUS_SUCCESS) {
        rl_printf("Failed to list characteristics\n");
        return;
    }
}

/* search all characteristics of specific service */
static void cmd_chars(char *args) {
    bt_status_t status;
    int id;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE characteristics: GATT interface not "
                  "avaiable\n");
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    if (sscanf(args, " %i ", &id) != 1) {
        rl_printf("Usage: characteristics serviceID\n");
        return;
    }

    if (id < 0 || id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", id,
                  u.svcs_size - 1);
        return;
    }

    if (u.svcs[id].chars_buf == NULL) {
        int i;

        u.svcs[id].chars_buf_size = MAX_CHARS_SIZE;
        u.svcs[id].chars_buf = malloc(sizeof(char_info_t) *
                                      u.svcs[id].chars_buf_size);

        for (i = u.svcs[id].char_count; i < u.svcs[id].chars_buf_size; i++) {
            u.svcs[id].chars_buf[i].descrs = NULL;
            u.svcs[id].chars_buf[i].descr_count = 0;
        }
    } else if (u.svcs[id].char_count > 0)
        u.svcs[id].char_count = 0;

    /* get first characteristic of service */
    status = u.gattiface->client->get_characteristic(u.conn_id,
                                                     &u.svcs[id].svc_id, NULL);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to list characteristics\n");
        return;
    }
}

void read_characteristic_cb(int conn_id, int status,
                            btgatt_read_params_t *p_data) {
    char uuid_str[UUID128_STR_LEN] = {0};
    char value_hexstr[BTGATT_MAX_ATTR_LEN * 3 + 1] = {0};
    int i;

    if (status != 0) {
        rl_printf("Read characteristic error, status:%i %s\n", status,
                  atterror2str(status));
        return;
    }

    for (i = 0; i < p_data->value.len; i++)
        sprintf(&value_hexstr[i * 3], "%02hhx ", p_data->value.value[i]);

    rl_printf("Read Characteristic\n");
    rl_printf("  Service UUID:        %s\n", uuid2str(&p_data->srvc_id.id.uuid,
              uuid_str));
    rl_printf("  Characteristic UUID: %s\n", uuid2str(&p_data->char_id.uuid,
              uuid_str));
    rl_printf("  value_type:%i status:%i value(hex): %s\n", p_data->value_type,
              p_data->status, value_hexstr);
}

static void cmd_read_char(char *args) {
    bt_status_t status;
    service_info_t *svc_info;
    char_info_t *char_info;
    int svc_id, char_id, auth;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE read-char: GATT interface not avaiable\n");
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    if (sscanf(args, " %i %i %i ", &svc_id, &char_id, &auth) != 3) {
        rl_printf("Usage: read-char serviceID characteristicID auth\n");
        rl_printf("  auth - enable authentication (1) or not (0)\n");
        return;
    }

    if (svc_id < 0 || svc_id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", svc_id,
                  u.svcs_size - 1);
        return;
    }

    svc_info = &u.svcs[svc_id];
    if (char_id < 0 || char_id >= svc_info->char_count) {
        rl_printf("Invalid characteristicID, try to run characteristics "
                  "command.\n");
        return;
    }

    char_info = &svc_info->chars_buf[char_id];
    status = u.gattiface->client->read_characteristic(u.conn_id,
                                                      &svc_info->svc_id,
                                                      &char_info->char_id,
                                                      auth);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to read characteristic\n");
        return;
    }
}

void write_characteristic_cb(int conn_id, int status,
                             btgatt_write_params_t *p_data) {
    char uuid_str[UUID128_STR_LEN] = {0};

    if (status != 0) {
        rl_printf("Write characteristic error, status:%i %s\n", status,
                  atterror2str(status));
        return;
    }

    rl_printf("Write characteristic success\n");
    rl_printf("  Service UUID:        %s\n", uuid2str(&p_data->srvc_id.id.uuid,
              uuid_str));
    rl_printf("  Characteristic UUID: %s\n", uuid2str(&p_data->char_id.uuid,
              uuid_str));
}

/*
 * @param write_type 1 -> Write Command
 *                   2 -> Write Request
 *                   3 -> Prepare Write
 */
void write_char(int write_type, const char *cmd, char *args) {
    bt_status_t status;
    service_info_t *svc_info;
    char_info_t *char_info;
    char *saveptr = NULL, *tok;
    int params = 0;
    int svc_id, char_id, auth;
    char new_value[BTGATT_MAX_ATTR_LEN];
    int new_value_len = 0;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE %s: GATT interface not avaiable\n", cmd);
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    tok = strtok_r(args, " ", &saveptr);
    while (tok != NULL) {
        switch (params) {
            case 0:
                if (sscanf(tok, " %i ", &svc_id) != 1) {
                    rl_printf("Invalid serviceID: %s\n", tok);
                    return;
                }
                break;
            case 1:
                if (sscanf(tok, " %i ", &char_id) != 1) {
                    rl_printf("Invalid characteristicID: %s\n", tok);
                    return;
                }
                break;
            case 2:
                if (sscanf(tok, " %i ", &auth) != 1) {
                    rl_printf("Invalid auth: %s\n", tok);
                    return;
                }
                break;
            default: {
                char *endptr = NULL;
                unsigned long int v = strtoul(tok, &endptr, 16);

                if (endptr[0] != '\0' || v > 0xff) {
                    rl_printf("Invalid hex value: %s\n", tok);
                    return;
                }

                if (new_value_len == sizeof(new_value)) {
                    rl_printf("Too many bytes to write in value!\n");
                    return;
                }

                new_value[new_value_len] = v;
                new_value_len++;
                break;
            }
        }
        params++;
        tok = strtok_r(NULL, " ", &saveptr);
    }

    if (params < 4) {
        rl_printf("Usage: %s serviceID characteristicID auth value\n", cmd);
        rl_printf("  auth  - enable authentication (1) or not (0)\n");
        rl_printf("  value - a sequence of hex values (eg: DE AD BE EF)\n");
        return;
    }

    if (svc_id < 0 || svc_id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", svc_id,
                  u.svcs_size - 1);
        return;
    }

    svc_info = &u.svcs[svc_id];
    if (char_id < 0 || char_id >= svc_info->char_count) {
        rl_printf("Invalid characteristicID, try to run characteristics "
                  "command.\n");
        return;
    }

    rl_printf("Writing %i bytes\n", new_value_len);
    char_info = &svc_info->chars_buf[char_id];
    status = u.gattiface->client->write_characteristic(u.conn_id,
                                                       &svc_info->svc_id,
                                                       &char_info->char_id,
                                                       write_type,
                                                       new_value_len,
                                                       auth, new_value);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to write characteristic\n");
        return;
    }
}

static void cmd_write_req_char(char *args) {

    write_char(2, "write-req-char", args);
}

static void cmd_write_cmd_char(char *args) {

    write_char(1, "write-cmd-char", args);
}

void get_descriptor_cb(int conn_id, int status, btgatt_srvc_id_t *srvc_id,
                       btgatt_char_id_t *char_id, bt_uuid_t *descr_id) {
    bt_status_t ret;
    char uuid_str[UUID128_STR_LEN] = {0};
    int svc_id, ch_id;
    service_info_t *svc_info = NULL;
    char_info_t *char_info = NULL;

    if (status != 0) {
        if (status == 0x85) { /* it's not really an error, just finished */
            rl_printf("List characteristics descriptors finished\n");
            return;
        }

        rl_printf("List characteristic descriptors finished, status: %i %s\n",
                  status, atterror2str(status));
        return;
    }

    svc_id = find_svc(srvc_id);
    if (svc_id < 0) {
        rl_printf("Received invalid descriptor (service inexistent)\n");
        return;
    }
    svc_info = &u.svcs[svc_id];

    ch_id = find_char(svc_info, char_id);
    if (ch_id < 0) {
        rl_printf("Received invalid descriptor (characteristic inexistent)\n");
        return;
    }
    char_info = &svc_info->chars_buf[ch_id];

    rl_printf("ID:%i UUID: %s\n", char_info->descr_count,
              uuid2str(descr_id, uuid_str));

    if (char_info->descr_count == 255) {
        rl_printf("Max descriptors overflow error\n");
        return;
    }

    char_info->descr_count++;
    char_info->descrs = realloc(char_info->descrs, char_info->descr_count *
                                sizeof(char_info->descrs[0]));

    /* copy descriptor data */
    memcpy(&char_info->descrs[char_info->descr_count - 1], descr_id,
           sizeof(*descr_id));

    /* get next descriptor */
    ret = u.gattiface->client->get_descriptor(u.conn_id, srvc_id, char_id,
                                              descr_id);
    if (ret != BT_STATUS_SUCCESS) {
        rl_printf("Failed to list descriptors\n");
        return;
    }
}

static void cmd_char_desc(char *args) {
    bt_status_t status;
    service_info_t *svc_info;
    char_info_t *char_info;
    int svc_id, char_id;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE char-desc: GATT interface not avaiable\n");
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    if (sscanf(args, " %i %i ", &svc_id, &char_id) != 2) {
        rl_printf("Usage: char-desc serviceID characteristicID\n");
        return;
    }

    if (svc_id < 0 || svc_id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", svc_id,
                  u.svcs_size - 1);
        return;
    }

    svc_info = &u.svcs[svc_id];
    if (char_id < 0 || char_id >= svc_info->char_count) {
        rl_printf("Invalid characteristicID, try to run characteristics "
                  "command.\n");
        return;
    }

    char_info = &svc_info->chars_buf[char_id];
    char_info->descr_count = 0;
    /* get first descriptor */
    status = u.gattiface->client->get_descriptor(u.conn_id, &svc_info->svc_id,
                                                 &char_info->char_id, NULL);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to list characteristic descriptors\n");
        return;
    }
}

void write_descriptor_cb(int conn_id, int status,
                         btgatt_write_params_t *p_data) {
    char uuid_str[UUID128_STR_LEN] = {0};

    if (status != 0) {
        rl_printf("Write descriptor error, status:%i %s\n", status,
                  atterror2str(status));
        return;
    }

    rl_printf("Write descriptor success\n");
    rl_printf("  Service UUID:        %s\n", uuid2str(&p_data->srvc_id.id.uuid,
              uuid_str));
    rl_printf("  Characteristic UUID: %s\n", uuid2str(&p_data->char_id.uuid,
              uuid_str));
    rl_printf("  Descriptor UUID:     %s\n", uuid2str(&p_data->descr_id,
              uuid_str));
}

static void cmd_write_desc(char *args) {
    bt_status_t status;
    service_info_t *svc_info;
    char_info_t *char_info;
    bt_uuid_t *descr_uuid;
    char *saveptr = NULL, *tok;
    int params = 0;
    int svc_id, char_id, desc_id, auth;
    char new_value[BTGATT_MAX_ATTR_LEN];
    int new_value_len = 0;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE write-desc: GATT interface not avaiable\n");
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    tok = strtok_r(args, " ", &saveptr);
    while (tok != NULL) {
        switch (params) {
            case 0:
                if (sscanf(tok, " %i ", &svc_id) != 1) {
                    rl_printf("Invalid serviceID: %s\n", tok);
                    return;
                }
                break;
            case 1:
                if (sscanf(tok, " %i ", &char_id) != 1) {
                    rl_printf("Invalid characteristicID: %s\n", tok);
                    return;
                }
                break;
            case 2:
                if (sscanf(tok, " %i ", &desc_id) != 1) {
                    rl_printf("Invalid descriptorID: %s\n", tok);
                    return;
                }
                break;
            case 3:
                if (sscanf(tok, " %i ", &auth) != 1) {
                    rl_printf("Invalid auth: %s\n", tok);
                    return;
                }
                break;
            default: {
                char *endptr = NULL;
                unsigned long int v = strtoul(tok, &endptr, 16);

                if (endptr[0] != '\0' || v > 0xff) {
                    rl_printf("Invalid hex value: %s\n", tok);
                    return;
                }

                if (new_value_len == sizeof(new_value)) {
                    rl_printf("Too many bytes to write in value!\n");
                    return;
                }

                new_value[new_value_len] = v;
                new_value_len++;
                break;
            }
        }
        params++;
        tok = strtok_r(NULL, " ", &saveptr);
    }

    if (params < 5) {
        rl_printf("Usage: write-desc serviceID characteristicID descriptorID "
                  "auth value\n");
        rl_printf("  auth  - enable authentication (1) or not (0)\n");
        rl_printf("  value - a sequence of hex values (eg: DE AD BE EF)\n");
        return;
    }

    if (svc_id < 0 || svc_id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", svc_id,
                  u.svcs_size - 1);
        return;
    }

    svc_info = &u.svcs[svc_id];
    if (char_id < 0 || char_id >= svc_info->char_count) {
        rl_printf("Invalid characteristicID, try to run characteristics "
                  "command.\n");
        return;
    }

    char_info = &svc_info->chars_buf[char_id];
    if (desc_id < 0 || desc_id >= char_info->descr_count) {
        rl_printf("Invalid descriptorID, try to run char-desc command.\n");
        return;
    }
    descr_uuid = &char_info->descrs[desc_id];

    rl_printf("Writing %i bytes\n", new_value_len);
    status = u.gattiface->client->write_descriptor(u.conn_id, &svc_info->svc_id,
                                                   &char_info->char_id,
                                                   descr_uuid,
                                                   2 /* Write Request */,
                                                   new_value_len, auth,
                                                   new_value);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to write descriptor\n");
        return;
    }
}

void read_descriptor_cb(int conn_id, int status, btgatt_read_params_t *p_data) {
    char uuid_str[UUID128_STR_LEN] = {0};
    char value_hexstr[BTGATT_MAX_ATTR_LEN * 3 + 1] = {0};
    int i;

    if (status != 0) {
        rl_printf("Read descriptor error, status:%i %s\n", status,
                  atterror2str(status));
        return;
    }

    for (i = 0; i < p_data->value.len; i++)
        sprintf(&value_hexstr[i * 3], "%02hhx ", p_data->value.value[i]);

    rl_printf("Read Descriptor\n");
    rl_printf("  Service UUID:        %s\n", uuid2str(&p_data->srvc_id.id.uuid,
              uuid_str));
    rl_printf("  Characteristic UUID: %s\n", uuid2str(&p_data->char_id.uuid,
              uuid_str));
    rl_printf("  Descriptor UUID:     %s\n", uuid2str(&p_data->descr_id,
              uuid_str));
    rl_printf("  value_type:%i status:%i value(hex): %s\n", p_data->value_type,
              p_data->status, value_hexstr);
}

static void cmd_read_desc(char *args) {
    bt_status_t status;
    service_info_t *svc_info;
    char_info_t *char_info;
    bt_uuid_t *descr_uuid;
    int svc_id, char_id, desc_id, auth;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE read-desc: GATT interface not avaiable\n");
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    if (sscanf(args, " %i %i %i %i ", &svc_id, &char_id, &desc_id,
               &auth) != 4) {
        rl_printf("Usage: read-desc serviceID characteristicID descriptorID "
                  "auth\n");
        rl_printf("  auth - enable authentication (1) or not (0)\n");
        return;
    }

    if (svc_id < 0 || svc_id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", svc_id,
                  u.svcs_size - 1);
        return;
    }

    svc_info = &u.svcs[svc_id];
    if (char_id < 0 || char_id >= svc_info->char_count) {
        rl_printf("Invalid characteristicID, try to run characteristics "
                  "command.\n");
        return;
    }

    char_info = &svc_info->chars_buf[char_id];
    if (desc_id < 0 || desc_id >= char_info->descr_count) {
        rl_printf("Invalid descriptorID, try to run char-desc command.\n");
        return;
    }
    descr_uuid = &char_info->descrs[desc_id];

    status = u.gattiface->client->read_descriptor(u.conn_id, &svc_info->svc_id,
                                                  &char_info->char_id,
                                                  descr_uuid, auth);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to read descriptor\n");
        return;
    }
}

void register_for_notification_cb(int conn_id, int registered, int status,
                                  btgatt_srvc_id_t *srvc_id,
                                  btgatt_char_id_t *char_id) {
    char uuid_str[UUID128_STR_LEN] = {0};

    if (status != 0) {
        rl_printf("Un/register for characteristic notification status: %i %s\n",
                  status, atterror2str(status));
        return;
    }

    rl_printf("Register for notification/indication: %s\n", registered ?
               "registered" : "unregistered");
    rl_printf("  Service UUID:        %s\n", uuid2str(&srvc_id->id.uuid,
              uuid_str));
    rl_printf("  Characteristic UUID: %s\n", uuid2str(&char_id->uuid,
              uuid_str));
}

void notify_cb(int conn_id, btgatt_notify_params_t *p_data) {
    char uuid_str[UUID128_STR_LEN] = {0};
    char value_hexstr[BTGATT_MAX_ATTR_LEN * 3 + 1] = {0};
    int i;

    for (i = 0; i < p_data->len; i++)
        sprintf(&value_hexstr[i * 3], "%02hhx ", p_data->value[i]);

    rl_printf("Notify Characteristic\n");
    rl_printf("  Service UUID:        %s\n", uuid2str(&p_data->srvc_id.id.uuid,
              uuid_str));
    rl_printf("  Characteristic UUID: %s\n", uuid2str(&p_data->char_id.uuid,
              uuid_str));
    rl_printf("  is_notify:%i value(hex): %s\n", p_data->is_notify,
              value_hexstr);
}

static void cmd_reg_notification(char *args) {
    bt_status_t status;
    service_info_t *svc_info;
    char_info_t *char_info;
    int svc_id, char_id;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to register notification/indication: GATT interface "
                  "not available\n");
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    if (sscanf(args, " %i %i ", &svc_id, &char_id) != 2) {
        rl_printf("Usage: reg-notif serviceID characteristicID\n");
        return;
    }

    if (svc_id < 0 || svc_id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", svc_id,
                  u.svcs_size - 1);
        return;
    }

    svc_info = &u.svcs[svc_id];
    if (char_id < 0 || char_id >= svc_info->char_count) {
        rl_printf("Invalid characteristicID, try to run characteristics "
                  "command\n");
        return;
    }

    char_info = &svc_info->chars_buf[char_id];
    status = u.gattiface->client->register_for_notification(u.client_if,
                                                           &u.remote_addr,
                                                           &svc_info->svc_id,
                                                           &char_info->char_id);
    if (status != BT_STATUS_SUCCESS)
        rl_printf("Failed to register for characteristic "
                  "notification/indication\n");
}

static void cmd_unreg_notification(char *args) {
    bt_status_t status;
    service_info_t *svc_info;
    char_info_t *char_info;
    int svc_id, char_id;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to unregister notification/indication: GATT interface"
                  " not available\n");
        return;
    }

    if (u.svcs_size <= 0) {
        rl_printf("Run search-svc first to get all services list\n");
        return;
    }

    if (sscanf(args, " %i %i ", &svc_id, &char_id) != 2) {
        rl_printf("Usage: unreg-notif serviceID characteristicID\n");
        return;
    }

    if (svc_id < 0 || svc_id >= u.svcs_size) {
        rl_printf("Invalid serviceID: %i need to be between 0 and %i\n", svc_id,
                  u.svcs_size - 1);
        return;
    }

    svc_info = &u.svcs[svc_id];
    if (char_id < 0 || char_id >= svc_info->char_count) {
        rl_printf("Invalid characteristicID, try to run characteristics "
                  "command\n");
        return;
    }

    char_info = &svc_info->chars_buf[char_id];
    status = u.gattiface->client->deregister_for_notification(u.client_if,
                                                           &u.remote_addr,
                                                           &svc_info->svc_id,
                                                           &char_info->char_id);
    if (status != BT_STATUS_SUCCESS)
        rl_printf("Failed to unregister for characteristic "
                  "notification/indication\n");
}

void read_remote_rssi_cb(int client_if, bt_bdaddr_t *bda, int rssi,
                         int status) {
    char addr_str[BT_ADDRESS_STR_LEN];

    if (status != 0) {
        rl_printf("Read RSSI error, status:%i %s\n", status,
                  atterror2str(status));
        return;
    }

    rl_printf("Address: %s RSSI: %i\n", ba2str(bda->address, addr_str), rssi);
}

static void cmd_rssi(char *args) {
    bt_status_t status;

    if (u.conn_id <= 0) {
        rl_printf("Not connected\n");
        return;
    }

    if (u.gattiface == NULL) {
        rl_printf("Unable to BLE RSSI: GATT interface not avaiable\n");
        return;
    }

    status = u.gattiface->client->read_remote_rssi(u.client_if, &u.remote_addr);
    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to request RSSI, status: %d\n", status);
        return;
    }
}

static void cmd_conns(char *args) {
    int i, c = 0;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        connection_t *conn = &u.conns[i];
        char addr_str[BT_ADDRESS_STR_LEN];

        if (conn->conn_id <= INVALID_CONN_ID)
            continue;

        rl_printf("Connection ID: %i  Address: %s%s\n", conn->conn_id,
                  ba2str(conn->remote_addr.address, addr_str),
                  conn->conn_id == PENDING_CONN_ID ? " (pending)" : "");
        c++;
    }

    if (c == 0)
        rl_printf("No connections active\n");
}

/* List of available user commands */
static const cmd_t cmd_list[] = {
    { "quit", "        Exits", cmd_quit },
    { "enable", "      Enables the Bluetooth adapter", cmd_enable },
    { "disable", "     Disables the Bluetooth adapter", cmd_disable },
    { "discovery", "   Controls discovery of nearby devices", cmd_discovery },
    { "scan", "        Controls BLE scan of nearby devices", cmd_scan },
    { "connect", "     Create a connection to a remote device", cmd_connect },
    { "pair", "        Pair with remote device", cmd_pair },
    { "disconnect", "  Disconnect from remote device", cmd_disconnect },
    { "search-svc", "  Search services on remote device", cmd_search_svc },
    { "included", "    List included services of a service", cmd_included },
    { "characteristics", "List characteristics of a service", cmd_chars },
    { "read-char", "   Read a characteristic of a service", cmd_read_char },
    { "write-req-char", "Write a characteristic (Write Request)",
                                                           cmd_write_req_char },
    { "write-cmd-char", "Write a characteristic (No response)",
                                                           cmd_write_cmd_char },
    { "char-desc", "   List descriptors from a characteristic", cmd_char_desc },
    { "write-desc", "  Write on characteristic descriptor", cmd_write_desc },
    { "read-desc", "   Read a characteristic descriptor", cmd_read_desc },
    { "reg-notif", "   Register to receive characteristic "
                   "notification/indicaton", cmd_reg_notification },
    { "unreg-notif", " Unregister a previous request to receive "
                     "notification/indicaton", cmd_unreg_notification },
    { "rssi", "        Request RSSI for connected device", cmd_rssi },
    { "connections", " Display active connections", cmd_conns },
    { NULL, NULL, NULL }
};

/* Parses a command and calls the respective handler */
static void cmd_process(char *line) {
    char cmd[MAX_LINE_SIZE];
    int i;

    if (line[0] == 0)
        return;

    if (u.prompt_state == SSP_ENTRY_PSTATE) {
        bt_status_t status = u.btiface->pin_reply(&u.r_bd_addr, true,
                                                  strlen(line),
                                                  (bt_pin_code_t *) line);
        change_prompt_state(NORMAL_PSTATE);
        return;
    }

    line_get_str(&line, cmd);

    if (strcmp(cmd, "help") == 0) {
        for (i = 0; cmd_list[i].name != NULL; i++)
            rl_printf("%s %s\n", cmd_list[i].name, cmd_list[i].description);
        return;
    }

    for (i = 0; cmd_list[i].name != NULL; i++)
        if (strcmp(cmd, cmd_list[i].name) == 0) {
            cmd_list[i].handler(line);
            return;
        }

    rl_printf("%s: unknown command, use 'help' for a list of available "
              "commands\n", cmd);
}

static void register_client_cb(int status, int client_if,
                               bt_uuid_t *app_uuid) {

    if (status != BT_STATUS_SUCCESS) {
        rl_printf("Failed to register client, status: %d\n", status);
        return;
    }

    rl_printf("Registered!, client_if: %d\n", client_if);

    u.client_if = client_if;
    u.client_registered = true;
}

/* GATT client callbacks */
static const btgatt_client_callbacks_t gattccbs = {
    register_client_cb, /* Called after client is registered */
    scan_result_cb, /* called every time an advertising report is seen */
    connect_cb, /* called every time a connection attempt finishes */
    disconnect_cb, /* called every time a connection attempt finishes */
    search_complete_cb, /* search_complete_callback */
    search_result_cb, /* search_result_callback */
    get_characteristic_cb, /* get_characteristic_callback */
    get_descriptor_cb, /* get_descriptor_callback */
    get_included_service_cb, /* get_included_service_callback */
    register_for_notification_cb, /* register_for_notification_callback */
    notify_cb, /* notify_callback */
    read_characteristic_cb, /* read_characteristic_callback */
    write_characteristic_cb, /* write_characteristic_callback */
    read_descriptor_cb, /* read_descriptor_callback */
    write_descriptor_cb, /* write_descriptor_callback */
    NULL, /* execute_write_callback */
    read_remote_rssi_cb, /* read_remote_rssi_callback */
};

/* GATT interface callbacks */
static const btgatt_callbacks_t gattcbs = {
    sizeof(btgatt_callbacks_t),
    &gattccbs,
    NULL  /* btgatt_server_callbacks_t */
};

/* This callback is used by the thread that handles Bluetooth interface (btif)
 * to send events for its users. At the moment there are two events defined:
 *
 *     ASSOCIATE_JVM: sinalizes the btif handler thread is ready;
 *     DISASSOCIATE_JVM: sinalizes the btif handler thread is about to exit.
 *
 * This events are used by the JNI to know when the btif handler thread should
 * be associated or dessociated with the JVM
 */
static void thread_event_cb(bt_cb_thread_evt event) {
    rl_printf("\nBluetooth interface %s\n",
              event == ASSOCIATE_JVM ? "ready" : "finished");
    if (event == ASSOCIATE_JVM) {
        u.btiface_initialized = 1;

        u.gattiface = u.btiface->get_profile_interface(BT_PROFILE_GATT_ID);
        if (u.gattiface != NULL) {
            bt_status_t status = u.gattiface->init(&gattcbs);
            if (status != BT_STATUS_SUCCESS) {
                rl_printf("Failed to initialize Bluetooth GATT interface, "
                          "status: %d\n", status);
                u.gattiface = NULL;
            } else
                u.gattiface_initialized = 1;
        } else
            rl_printf("Failed to get Bluetooth GATT Interface\n");
    } else
        u.btiface_initialized = 0;
}

/* Bluetooth interface callbacks */
static bt_callbacks_t btcbs = {
    sizeof(bt_callbacks_t),
    adapter_state_change_cb, /* Called every time the adapter state changes */
    adapter_properties_cb, /* adapter_properties_callback */
    NULL, /* remote_device_properties_callback */
    device_found_cb, /* Called for every device found */
    discovery_state_changed_cb, /* Called every time the discovery state changes */
    pin_request_cb, /* pin_request_callback */
    ssp_request_cb, /* ssp_request_callback */
    bond_state_changed_cb, /* bond_state_changed_callback */
    NULL, /* acl_state_changed_callback */
    thread_event_cb, /* Called when the JVM is associated / dissociated */
    NULL, /* dut_mode_recv_callback */
    NULL, /* le_test_mode_callback */
};

/* Initialize the Bluetooth stack */
static void bt_init() {
    int status, i;
    hw_module_t *module;
    hw_device_t *hwdev;
    bluetooth_device_t *btdev;

    u.btiface_initialized = 0;
    u.quit = 0;
    u.adapter_state = BT_STATE_OFF; /* The adapter is OFF in the beginning */
    u.conn_id = 0;

    for (i = 0; i < MAX_CONNECTIONS; i++)
        u.conns[i].conn_id = INVALID_CONN_ID;

    /* Get the Bluetooth module from libhardware */
    status = hw_get_module(BT_STACK_MODULE_ID, (hw_module_t const**) &module);
    if (status < 0) {
        errno = status;
        err(1, "Failed to get the Bluetooth module");
    }
    rl_printf("Bluetooth stack infomation:\n");
    rl_printf("    id = %s\n", module->id);
    rl_printf("    name = %s\n", module->name);
    rl_printf("    author = %s\n", module->author);
    rl_printf("    HAL API version = %d\n", module->hal_api_version);

    /* Get the Bluetooth hardware device */
    status = module->methods->open(module, BT_STACK_MODULE_ID, &hwdev);
    if (status < 0) {
        errno = status;
        err(2, "Failed to get the Bluetooth hardware device");
    }
    rl_printf("Bluetooth device infomation:\n");
    rl_printf("    API version = %d\n", hwdev->version);

    /* Get the Bluetooth interface */
    btdev = (bluetooth_device_t *) hwdev;
    u.btiface = btdev->get_bluetooth_interface();
    if (u.btiface == NULL)
        err(3, "Failed to get the Bluetooth interface");

    /* Init the Bluetooth interface, setting a callback for each operation */
    status = u.btiface->init(&btcbs);
    if (status != BT_STATUS_SUCCESS && status != BT_STATUS_DONE)
        err(4, "Failed to initialize the Bluetooth interface");
}

/* simple tab completer */
const char *tab_completer_cb(char *line, int pos) {
    int i = 0;
    const cmd_t *p = &(cmd_list[0]);

    while (p->name) {
        /* test only commands starting with it */
        if (strncmp(p->name, line, pos) == 0)
            return p->name + pos;

        p = &cmd_list[++i];
    }

    return NULL;
}

int main(int argc, char *argv[]) {

    /* check if I am root */
    if (getuid() != 0) {
        printf("This software requires root access\n");
        return 1;
    }

    rl_init(cmd_process);
    change_prompt_state(NORMAL_PSTATE);
    rl_set_tab_completer(tab_completer_cb);

    rl_printf("Android Bluetooth control tool version " VERSION "\n");

    bt_init();

    while (!u.quit) {
        int c = getchar();

        if (c == EOF) {
            rl_printf("error reading input, exiting...\n");
            u.quit = true;
            break;
        }

        /* if we are in consent bonding process, we need only a char */
        if (u.prompt_state == SSP_CONSENT_PSTATE) {
            c = toupper(c);
            if (c == 'Y' || c == 'N') {
                bt_status_t status;
                printf("%c\n", c); /* user feedback */
                do_ssp_reply(&u.r_bd_addr, BT_SSP_VARIANT_CONSENT,
                             c == 'Y' ? true : false, 0);
            }
            change_prompt_state(NORMAL_PSTATE);
        } else if (!rl_feed(c))
            break; /* user pressed ctrl-d */
    }

    /* Disable adapter on exit */
    if (u.adapter_state == BT_STATE_ON)
        cmd_disable(NULL);

    /* Cleanup the Bluetooth interface */
    rl_printf("Processing Bluetooth interface cleanup\n");
    u.btiface->cleanup();
    while (u.btiface_initialized)
        usleep(10000);

    rl_quit();
    return 0;
}
