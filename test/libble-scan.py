#!/usr/bin/python
# -*- coding: utf-8 -*-

##
#  libble-scan -- Enables BLE and scans for devices for 30s
#
#  Copyright (C) 2013 João Paulo Rechi Vita
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

from ctypes import *

libble = CDLL("libble.so")

enable_cb_t = CFUNCTYPE(None)
adapter_state_cb_t = CFUNCTYPE(None, c_ubyte)
scan_cb_t = CFUNCTYPE(None, 6 * c_ubyte, c_int, POINTER(c_ubyte))
connect_cb_t = CFUNCTYPE(None, 6 * c_ubyte, c_int, c_int)
bond_state_cb_t = CFUNCTYPE(None, 6 * c_ubyte, c_ubyte, c_int)
rssi_cb_t = CFUNCTYPE(None, c_int, c_int, c_int)
gatt_found_cb_t = CFUNCTYPE(None, c_int, c_int, 16 * c_ubyte, c_int)
gatt_finished_cb_t = CFUNCTYPE(None, c_int, c_int)
gatt_response_cb_t = CFUNCTYPE(None, c_int, c_int, POINTER(c_ubyte), c_ushort, c_ushort, c_int)
gatt_notification_register_cb_t = CFUNCTYPE(None, c_int, c_int, c_int, c_int)
gatt_notification_cb_t = CFUNCTYPE(None, c_int, c_int, POINTER(c_ubyte), c_ushort, c_ubyte)

class ble_cbs_t(Structure):
    _fields_ = [
        ("enable_cb", enable_cb_t),
        ("adapter_state_cb", adapter_state_cb_t),
        ("scan_cb", scan_cb_t),
        ("connect_cb", connect_cb_t),
        ("disconnect_cb", connect_cb_t),
        ("bond_state_cb", bond_state_cb_t),
        ("rssi_cb", rssi_cb_t),
        ("srvc_found_cb", gatt_found_cb_t),
        ("srvc_finished_cb", gatt_finished_cb_t),
        ("char_found_cb", gatt_found_cb_t),
        ("char_finished_cb", gatt_finished_cb_t),
        ("desc_found_cb", gatt_found_cb_t),
        ("desc_finished_cb", gatt_finished_cb_t),
        ("char_read_cb", gatt_response_cb_t),
        ("desc_read_cb", gatt_response_cb_t),
        ("char_write_cb", gatt_response_cb_t),
        ("desc_write_cb", gatt_response_cb_t),
        ("char_notification_register_cb", gatt_notification_register_cb_t),
        ("char_notification_cb", gatt_notification_cb_t)
    ]

if (__name__ == "__main__"):
    import time

    enabled = 0

    # void (void)
    def py_enable_cb():
        global enabled
        enabled = 1
        print "BLE Enabled."

    # void (uint8_t state)
    def py_adapter_state_cb(state):
        print "Adapter state changed: %u." % state

    # void (const uint8_t *address, int rssi, const uint8_t *adv_data)
    def py_scan_cb(address, rssi, adv_data):
        print "Device found: %X:%X:%X:%X:%X:%X, RSSI %d" % (address[0], address[1], address[2], address[3], address[4], address[5], rssi)

    # void (const uint8_t *address, int conn_id, int status):
    def py_connect_cb(address, conn_id, status):
        pass

    # void (const uint8_t *address, ble_bond_state_t state, int status)
    def py_bond_state_cb(address, state, status):
        pass

    # void (int conn_id, int rssi, int status)
    def py_rssi_cb(conn_id, rssi, status):
        pass

    # void (int conn_id, int id, const uint8_t *uuid, int props)
    def py_gatt_found_cb(conn_id, id, uuid, props):
        pass

    # void (int conn_id, int status)
    def py_gatt_finished_cb(conn_id, status):
        pass

    # void (int conn_id, int id, const uint8_t *value, uint16_t value_len,
    #       uint16_t value_type, int status)
    def py_gatt_response_cb(conn_id, id, value, value_len, value_type, status):
        pass

    # void (int conn_id, int char_id, int registered, int status)
    def py_gatt_notification_register_cb(conn_id, char_id, registered, status):
        pass

    # void (int conn_id, int char_id, const uint8_t *value, uint16_t value_len,
    #       uint8_t is_indication)
    def py_gatt_notification_cb(conn_id, char_id, value, value_len,
                                is_indication):
        pass


    ble_cbs = ble_cbs_t(enable_cb_t(py_enable_cb),
                        adapter_state_cb_t(py_adapter_state_cb),
                        scan_cb_t(py_scan_cb))

    libble.ble_enable(ble_cbs)
    while (enabled == 0):
        time.sleep(1)

    print "Starting BLE scanning for 30s: %d" % libble.ble_start_scan()
    time.sleep(30);
    print "Stopping BLE scanning: %d" % libble.ble_stop_scan()

    print "Disabling the adapter: %d" % libble.ble_disable()
    time.sleep(2);
