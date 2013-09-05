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

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hardware/bluetooth.h>
#include <hardware/hardware.h>

#define MAX_LINE_SIZE 64

/* Data that have to be acessable by the callbacks */
struct userdata {
    const bt_interface_t *btiface;
    uint8_t btiface_initialized;
} u;

/* Prints the command prompt */
static void cmd_prompt() {
    printf("> ");
    fflush(stdout);
}

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
    printf("\nBluetooth interface %s\n", event == ASSOCIATE_JVM ? "ready" : "finished");
    if (event == ASSOCIATE_JVM) {
        u.btiface_initialized = 1;
        cmd_prompt();
    } else
        u.btiface_initialized = 0;
}

/* Bluetooth interface callbacks */
static bt_callbacks_t btcbs = {
    sizeof(bt_callbacks_t),
    NULL, /* adapter_state_changed_callback */
    NULL, /* adapter_properties_callback */
    NULL, /* remote_device_properties_callback */
    NULL, /* device_found_callback */
    NULL, /* discovery_state_changed_callback */
    NULL, /* pin_request_callback */
    NULL, /* ssp_request_callback */
    NULL, /* bond_state_changed_callback */
    NULL, /* acl_state_changed_callback */
    thread_event_cb, /* Called when the JVM is associated / dissociated */
    NULL, /* dut_mode_recv_callback */
    NULL, /* le_test_mode_callback */
};

/* Initialize the Bluetooth stack */
static void bt_init() {
    int status;
    hw_module_t *module;
    hw_device_t *hwdev;
    bluetooth_device_t *btdev;

    u.btiface_initialized = 0;

    /* Get the Bluetooth module from libhardware */
    status = hw_get_module(BT_STACK_MODULE_ID, (hw_module_t const**) &module);
    if (status < 0) {
        errno = status;
        err(1, "Failed to get the Bluetooth module");
    }
    printf("Bluetooth stack infomation:\n");
    printf("    id = %s\n", module->id);
    printf("    name = %s\n", module->name);
    printf("    author = %s\n", module->author);
    printf("    HAL API version = %d\n", module->hal_api_version);

    /* Get the Bluetooth hardware device */
    status = module->methods->open(module, BT_STACK_MODULE_ID, &hwdev);
    if (status < 0) {
        errno = status;
        err(2, "Failed to get the Bluetooth hardware device");
    }
    printf("Bluetooth device infomation:\n");
    printf("    API version = %d\n", hwdev->version);

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

int main (int argc, char * argv[]) {
    char line[MAX_LINE_SIZE];

    printf("Android Bluetooth control tool version 0.1\n");

    bt_init();

    cmd_prompt();

    while (fgets(line, MAX_LINE_SIZE, stdin)) {
        /* remove linefeed */
        line[strlen(line)-1] = 0;

        cmd_prompt();
    }

    /* Cleanup the Bluetooth interface */
    printf("Processing Bluetooth interface cleanup");
    u.btiface->cleanup();
    while (u.btiface_initialized)
        usleep(10000);

    printf("\n");
    return 0;
}
