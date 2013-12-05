/*
 *  Android BLE Library -- Provides utility functions to control BLE features
 *
 *  Copyright (C) 2013 João Paulo Rechi Vita
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  the Free Software Foundation; either version 2 of the License, or
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <hardware/bluetooth.h>
#include <hardware/hardware.h>

#include "ble.h"

/* Data that have to be acessable by the callbacks */
static struct libdata {
    ble_cbs_t cbs;

    uint8_t btiface_initialized;
    const bt_interface_t *btiface;
} data;

/* Bluetooth interface callbacks */
static bt_callbacks_t btcbs = {
    sizeof(bt_callbacks_t),
    NULL, /* adapter_state_change_cb */
    NULL, /* adapter_properties_cb */
    NULL, /* remote_device_properties_callback */
    NULL, /* device_found_cb */
    NULL, /* discovery_state_changed_cb */
    NULL, /* pin_request_cb */
    NULL, /* ssp_request_cb */
    NULL, /* bond_state_changed_cb */
    NULL, /* acl_state_changed_callback */
    NULL, /* thread_event_cb */
    NULL, /* dut_mode_recv_callback */
    NULL, /* le_test_mode_callback */
};

int ble_init(ble_cbs_t cbs) {
    int status;
    bt_status_t s;
    hw_module_t *module;
    hw_device_t *hwdev;
    bluetooth_device_t *btdev;

    memset(&data, 0, sizeof(data));

    /* Get the Bluetooth module from libhardware */
    status = hw_get_module(BT_STACK_MODULE_ID, (hw_module_t const**) &module);
    if (status < 0)
        return status;

    /* Get the Bluetooth hardware device */
    status = module->methods->open(module, BT_STACK_MODULE_ID, &hwdev);
    if (status < 0)
        return status;

    /* Get the Bluetooth interface */
    btdev = (bluetooth_device_t *) hwdev;
    data.btiface = btdev->get_bluetooth_interface();
    if (data.btiface == NULL)
        return -1;

    /* Init the Bluetooth interface, setting a callback for each operation */
    s = data.btiface->init(&btcbs);
    if (s != BT_STATUS_SUCCESS && s != BT_STATUS_DONE)
        return -s;

    /* Store the user callbacks */
    data.cbs = cbs;

    return 0;
}

void ble_shutdown() {
    /* Cleanup the Bluetooth interface */
    data.btiface->cleanup();
    while (data.btiface_initialized)
        usleep(10000);
}
