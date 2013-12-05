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
#include <hardware/bt_gatt.h>
#include <hardware/bt_gatt_client.h>
#include <hardware/hardware.h>

#include "ble.h"

/* Data that have to be acessable by the callbacks */
static struct libdata {
    ble_cbs_t cbs;

    uint8_t btiface_initialized;
    const bt_interface_t *btiface;
    const btgatt_interface_t *gattiface;
    int client;

    uint8_t adapter_state;
} data;

/* Called when the client registration is finished */
static void register_client_cb(int status, int client_if, bt_uuid_t *app_uuid) {
    if (status == BT_STATUS_SUCCESS) {
        data.client = client_if;
        if (data.cbs.enable_cb)
            data.cbs.enable_cb();
    } else
        data.client = 0;
}

/* GATT client interface callbacks */
static const btgatt_client_callbacks_t gattccbs = {
    register_client_cb,
    NULL, /* scan_result_cb */
    NULL, /* connect_cb */
    NULL, /* disconnect_cb */
    NULL, /* search_complete_cb */
    NULL, /* search_result_cb */
    NULL, /* get_characteristic_cb */
    NULL, /* get_descriptor_cb */
    NULL, /* get_included_service_cb */
    NULL, /* register_for_notification_cb */
    NULL, /* notify_cb */
    NULL, /* read_characteristic_cb */
    NULL, /* write_characteristic_cb */
    NULL, /* read_descriptor_cb */
    NULL, /* write_descriptor_cb */
    NULL, /* execute_write_cb */
    NULL, /* read_remote_rssi_cb */
};

/* GATT interface callbacks */
static const btgatt_callbacks_t gattcbs = {
    sizeof(btgatt_callbacks_t),
    &gattccbs,
    NULL  /* btgatt_server_callbacks_t */
};

/* Called every time the adapter state changes */
static void adapter_state_changed_cb(bt_state_t state) {
    /* Arbitrary UUID used to identify this application with the GATT profile */
    bt_uuid_t app_uuid = { .uu = { 0x1b, 0x1c, 0xb9, 0x2e, 0x0d, 0x2e, 0x4c,
                                   0x45, 0xbb, 0xb8, 0xf4, 0x1b, 0x46, 0x39,
                                   0x23, 0x36 } };

    data.adapter_state = state == BT_STATE_ON ? 1 : 0;
    if (data.cbs.adapter_state_cb)
        data.cbs.adapter_state_cb(data.adapter_state);

    if (data.adapter_state) {
        bt_status_t s = data.gattiface->client->register_client(&app_uuid);
        if (s != BT_STATUS_SUCCESS)
            data.btiface->disable();
    } else
        /* Cleanup the Bluetooth interface */
        data.btiface->cleanup();
}

/* This callback is called when the stack finishes initialization / shutdown */
static void thread_event_cb(bt_cb_thread_evt event) {
    bt_status_t s;

    if (event == DISASSOCIATE_JVM) {
        data.btiface_initialized = 0;
        return;
    }

    data.btiface_initialized = 1;

    data.gattiface = data.btiface->get_profile_interface(BT_PROFILE_GATT_ID);
    if (data.gattiface != NULL) {
        s = data.gattiface->init(&gattcbs);
        if (s != BT_STATUS_SUCCESS)
            data.gattiface = NULL;
    }

    /* Enable the adapter */
    s = data.btiface->enable();
    if (s != BT_STATUS_SUCCESS)
        data.btiface->cleanup();
}

/* Bluetooth interface callbacks */
static bt_callbacks_t btcbs = {
    sizeof(bt_callbacks_t),
    adapter_state_changed_cb,
    NULL, /* adapter_properties_cb */
    NULL, /* remote_device_properties_callback */
    NULL, /* device_found_cb */
    NULL, /* discovery_state_changed_cb */
    NULL, /* pin_request_cb */
    NULL, /* ssp_request_cb */
    NULL, /* bond_state_changed_cb */
    NULL, /* acl_state_changed_callback */
    thread_event_cb,
    NULL, /* dut_mode_recv_callback */
    NULL, /* le_test_mode_callback */
};

int ble_enable(ble_cbs_t cbs) {
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

int ble_disable() {
    bt_status_t s;

    if (!data.adapter_state)
        return -1;

    if (!data.btiface)
        return -1;

    if (!data.client)
        return -1;

    s = data.gattiface->client->unregister_client(data.client);
    if (s != BT_STATUS_SUCCESS)
        return -s;

    s = data.btiface->disable();
    if (s != BT_STATUS_SUCCESS)
        return -s;

    return 0;
}
