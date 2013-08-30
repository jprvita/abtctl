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

#include <hardware/bluetooth.h>
#include <hardware/hardware.h>

#define MAX_LINE_SIZE 64

/* Data that have to be acessable by the callbacks */
struct userdata {
} u;

/* Prints the command prompt */
static void cmd_prompt() {
    printf("> ");
    fflush(stdout);
}

/* Initialize the Bluetooth stack */
static void bt_init() {
    int status;
    hw_module_t *module;
    hw_device_t *hwdev;

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

    printf("\n");
    return 0;
}
