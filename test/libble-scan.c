/*
 *  libble-scan -- Enables BLE and scans for devices for 30s
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>

#include <libble/ble.h>

static uint8_t enabled;

static void enable_cb(void) {
    printf("BLE enabled.\n");
    enabled = 1;
}

static void adapter_state_cb(uint8_t state) {
    printf("Adapter state changed: %u\n", state);
}

static void scan_cb(const uint8_t *address, int rssi, const uint8_t *adv_data) {
    printf("Device found: %X:%X:%X:%X:%X:%X, RSSI %d\n", address[0], address[1],
            address[2], address[3], address[4], address[5], rssi);
}

static ble_cbs_t ble_cbs = {
    enable_cb,
    adapter_state_cb,
    scan_cb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int main (int argc, char *argv[]) {
    int status;

    if (getuid() != 0) {
        printf("Permission denied\n");
        return 1;
    }

    enabled = 0;
    printf("Initializing libble... ");
    status = ble_enable(ble_cbs);
    if (status != 0) {
        printf("failed (%d).\n", status);
        return -1;
    }
    printf("\n");
    while (!enabled) sleep(1);

    printf("Starting BLE scanning for 30s: %d\n", ble_start_scan());
    sleep(30);
    printf("Stopping BLE scanning: %d\n", ble_stop_scan());

    printf("Disabling the adapter: %d\n", ble_disable());
    sleep(2);

    return 0;
}
