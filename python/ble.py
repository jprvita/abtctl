#!/usr/bin/python
# -*- coding: utf-8 -*-

##
#  ble.py -- Python bindings for libble
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

## Callback types
enable_cb_t = CFUNCTYPE(None)
adapter_state_cb_t = CFUNCTYPE(None, c_ubyte)
scan_cb_t = CFUNCTYPE(None, POINTER(c_ubyte), c_int, POINTER(c_ubyte))
connect_cb_t = CFUNCTYPE(None, POINTER(c_ubyte), c_int, c_int)
bond_state_cb_t = CFUNCTYPE(None, POINTER(c_ubyte), c_ubyte, c_int)
rssi_cb_t = CFUNCTYPE(None, c_int, c_int, c_int)
gatt_found_cb_t = CFUNCTYPE(None, c_int, c_int, POINTER(c_ubyte), c_int)
gatt_finished_cb_t = CFUNCTYPE(None, c_int, c_int)
gatt_response_cb_t = CFUNCTYPE(None, c_int, c_int, POINTER(c_ubyte), c_ushort, c_ushort, c_int)
gatt_notification_register_cb_t = CFUNCTYPE(None, c_int, c_int, c_int, c_int)
gatt_notification_cb_t = CFUNCTYPE(None, c_int, c_int, POINTER(c_ubyte), c_ushort, c_ubyte)

## BLE callbacks structure
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

## Functions
def bda_from_string(s): # '01:23:45:67:89:0A'
    l = s.split(':')
    return (6 * c_ubyte)(int(l[0], 16), int(l[1], 16), int(l[2], 16), int(l[3], 16), int(l[4], 16), int(l[5], 16))

def uuid_from_string(s): # '01234567-89AB-CDEF-GHIJ-KLMNOPQRSTUV'
    if s is None:
        return POINTER(c_ubyte)()
    return (16 * c_ubyte)(int(s[34:36], 16), int(s[32:34], 16), int(s[30:32], 16), int(s[28:30], 16), int(s[26:28], 16), int(s[24:26], 16), int(s[21:23], 16), int(s[19:21], 16), int(s[16:18], 16), int(s[14:16], 16), int(s[11:13], 16), int(s[9:11], 16), int(s[6:8], 16), int(s[4:6], 16), int(s[2:4], 16), int(s[0:2], 16))

def hex_string_to_ubyte_pointer(s, l):
    p = (l * c_ubyte)()
    for i in range(0, len(s), 2):
        p[i/2] = int(s[i:i+2], 16)
    return p

def enable(cbs):
    if cbs is None:
        return -1
    libble.ble_enable(cbs)

disable = libble.ble_disable
start_scan = libble.ble_start_scan
stop_scan = libble.ble_stop_scan

def connect(address):
    libble.ble_connect(bda_from_string(address))

def disconnect(address):
    libble.ble_disconnect(bda_from_string(address))

def pair(address):
    libble.ble_pair(bda_from_string(address))

def cancel_pairing(address):
    libble.ble_pair(bda_from_string(address))

def remove_bond(address):
    libble.ble_remove_bond(bda_from_string(address))

read_remote_rssi = libble.ble_read_remote_rssi

def gatt_discover_services(conn_id, uuid):
    u = uuid_from_string(uuid)
    libble.ble_gatt_discover_services(conn_id, u)

gatt_get_included_services = libble.ble_gatt_get_included_services
gatt_discover_characteristics = libble.ble_gatt_discover_characteristics
gatt_discover_descriptors = libble.ble_gatt_discover_descriptors
gatt_read_char = libble.ble_gatt_read_char
gatt_read_desc = libble.ble_gatt_read_desc

def gatt_write_cmd_char(conn_id, char_id, auth, value, l):
    v = hex_string_to_ubyte_pointer(value, l)
    libble.ble_gatt_write_cmd_char(conn_id, char_id, auth, v, l)

def gatt_write_req_char(conn_id, char_id, auth, value, l):
    v = hex_string_to_ubyte_pointer(value, l)
    libble.ble_gatt_write_req_char(conn_id, char_id, auth, v, l)

def gatt_write_cmd_desc(conn_id, desc_id, auth, value, l):
    v = hex_string_to_ubyte_pointer(value, l)
    libble.ble_gatt_write_cmd_desc(conn_id, desc_id, auth, v, l)

def gatt_write_req_desc(conn_id, desc_id, auth, value, l):
    v = hex_string_to_ubyte_pointer(value, l)
    libble.ble_gatt_write_req_desc(conn_id, desc_id, auth, v, l)

def gatt_prep_write_char(conn_id, char_id, auth, value, l):
    v = hex_string_to_ubyte_pointer(value, l)
    libble.ble_gatt_prep_write_char

def gatt_prep_write_desc(conn_id, desc_id, auth, value, l):
    v = hex_string_to_ubyte_pointer(value, l)
    libble.ble_gatt_prep_write_desc

gatt_execute_write = libble.ble_gatt_execute_write
gatt_register_char_notification = libble.ble_gatt_register_char_notification
gatt_unregister_char_notification = libble.ble_gatt_unregister_char_notification

## Utils

# Stack state
enabled = 0

def py_enable_cb(): # void (void)
    global enabled
    enabled = 1
    print "BLE Enabled"

def py_adapter_state_cb(state): # void (uint8_t state)
    print "Adapter state changed to %u" % state
    if (state == 0):
        global enabled
        enabled = 0

def py_scan_cb(address, rssi, adv_data): # void (const uint8_t *address, int rssi, const uint8_t *adv_data)
    print "Found %02X:%02X:%02X:%02X:%02X:%02X RSSI %d" % (address[0], address[1], address[2], address[3], address[4], address[5], rssi)

def py_connect_cb(address, conn_id, status): # void (const uint8_t *address, int conn_id, int status):
    print "%02X:%02X:%02X:%02X:%02X:%02X connected: conn_id %d status %d" % (address[0], address[1], address[2], address[3], address[4], address[5], conn_id, status)

def py_disconnect_cb(address, conn_id, status): # void (const uint8_t *address, int conn_id, int status):
    print "%02X:%02X:%02X:%02X:%02X:%02X disconnected: conn_id %d status %d" % (address[0], address[1], address[2], address[3], address[4], address[5], conn_id, status)

def py_bond_state_cb(address, state, status): # void (const uint8_t *address, ble_bond_state_t state, int status)
    print "%02X:%02X:%02X:%02X:%02X:%02X bond state changed: state %d status %d" % (address[0], address[1], address[2], address[3], address[4], address[5], state, status)

def py_rssi_cb(conn_id, rssi, status): # void (int conn_id, int rssi, int status)
    print "Dev conn_id %d RSSI %d status %d" % (conn_id, rssi, status)

def py_srvc_found_cb(conn_id, elem_id, uuid, props): # void (int conn_id, int id, const uint8_t *uuid, int props)
    print "Dev conn_id %d service %d found: UUID %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x props %d" % (conn_id, elem_id, uuid[15], uuid[14], uuid[13], uuid[12], uuid[11], uuid[10], uuid[9], uuid[8], uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0], props)

def py_srvc_finished_cb(conn_id, status): # void (int conn_id, int status)
    print "Dev conn_id %d service discovery finished: status %d" % (conn_id, status)

def py_char_found_cb(conn_id, elem_id, uuid, props): # void (int conn_id, int id, const uint8_t *uuid, int props)
    print "Dev conn_id %d characteristic %d found: UUID %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x props %d" % (conn_id, elem_id, uuid[15], uuid[14], uuid[13], uuid[12], uuid[11], uuid[10], uuid[9], uuid[8], uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0], props)

def py_char_finished_cb(conn_id, status): # void (int conn_id, int status)
    print "Dev conn_id %d characteristic discovery finished: status %d" % (conn_id, status)

def py_desc_found_cb(conn_id, elem_id, uuid, props): # void (int conn_id, int id, const uint8_t *uuid, int props)
    print "Dev conn_id %d descriptor %d found: UUID %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x props %d" % (conn_id, elem_id, uuid[15], uuid[14], uuid[13], uuid[12], uuid[11], uuid[10], uuid[9], uuid[8], uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0], props)

def py_desc_finished_cb(conn_id, status): # void (int conn_id, int status)
    print "Dev conn_id %d descriptor discovery finished: status %d" % (conn_id, status)

def py_char_read_cb(conn_id, elem_id, value, value_len, value_type, status): # void (int conn_id, int id, const uint8_t *value, uint16_t value_len, uint16_t value_type, int status)
    print "Dev conn_id %d characteristic %d read status %d:" % (conn_id, elem_id, status),
    for i in range(value_len):
        print " %02X" % value[i],
    print

def py_desc_read_cb(conn_id, elem_id, value, value_len, value_type, status): # void (int conn_id, int id, const uint8_t *value, uint16_t value_len, uint16_t value_type, int status)
    print "Dev conn_id %d descriptor %d read status %d:" % (conn_id, elem_id, status),
    for i in range(value_len):
        print " %02X" % value[i],
    print

def py_char_write_cb(conn_id, elem_id, value, value_len, value_type, status): # void (int conn_id, int id, const uint8_t *value, uint16_t value_len, uint16_t value_type, int status)
    print "Dev conn_id %d characteristic %d written status %d:" % (conn_id, elem_id, status),
    for i in range(value_len):
        print " %02X" % value[i],
    print

def py_desc_write_cb(conn_id, elem_id, value, value_len, value_type, status): # void (int conn_id, int id, const uint8_t *value, uint16_t value_len, uint16_t value_type, int status)
    print "Dev conn_id %d descriptor %d written status %d:" % (conn_id, elem_id, status),
    for i in range(value_len):
        print " %02X" % value[i],
    print

def py_char_notification_register_cb(conn_id, char_id, registered, status): # void (int conn_id, int char_id, int registered, int status)
    if (registered):
        action = "registered"
    else:
        action = "unregistered"
    print "Dev conn_id %d %s notifications for characteristic %d status %d" % (conn_id, action, char_id, status)

def py_char_notification_cb(conn_id, char_id, value, value_len, is_indication): # void (int conn_id, int char_id, const uint8_t *value, uint16_t value_len, uint8_t is_indication)
    print "Dev conn_id %d notification for characteristic %d status %d:" % (conn_id, char_id, status),
    for i in range(value_len):
        print " %02X" % value[i],
    print

cbs = ble_cbs_t(enable_cb_t(py_enable_cb),
                adapter_state_cb_t(py_adapter_state_cb),
                scan_cb_t(py_scan_cb),
                connect_cb_t(py_connect_cb),
                connect_cb_t(py_disconnect_cb),
                bond_state_cb_t(py_bond_state_cb),
                rssi_cb_t(py_rssi_cb),
                gatt_found_cb_t(py_srvc_found_cb),
                gatt_finished_cb_t(py_srvc_finished_cb),
                gatt_found_cb_t(py_char_found_cb),
                gatt_finished_cb_t(py_char_finished_cb),
                gatt_found_cb_t(py_desc_found_cb),
                gatt_finished_cb_t(py_desc_finished_cb),
                gatt_response_cb_t(py_char_read_cb),
                gatt_response_cb_t(py_desc_read_cb),
                gatt_response_cb_t(py_char_write_cb),
                gatt_response_cb_t(py_desc_write_cb),
                gatt_notification_register_cb_t(py_char_notification_register_cb),
                gatt_notification_cb_t(py_char_notification_cb))

__all__ = [libble, enable_cb_t, adapter_state_cb_t, scan_cb_t, connect_cb_t,
           bond_state_cb_t, rssi_cb_t, gatt_found_cb_t, gatt_finished_cb_t,
           gatt_response_cb_t, gatt_notification_register_cb_t,
           gatt_notification_cb_t, ble_cbs_t, enable, disable, start_scan,
           stop_scan, connect, disconnect, pair, remove_bond, read_remote_rssi,
           gatt_discover_services, gatt_discover_characteristics,
           gatt_discover_descriptors, gatt_read_char, gatt_read_desc,
           gatt_write_cmd_char, gatt_write_req_char, gatt_write_cmd_desc,
           gatt_write_req_desc, gatt_register_char_notification,
           gatt_unregister_char_notification]
