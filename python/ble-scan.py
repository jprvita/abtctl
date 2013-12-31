#!/usr/bin/python
# -*- coding: utf-8 -*-

##
#  ble-scan.py -- Scan for BLE devices using the Python bindings for libble
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

import ble
import time

ble.enable(ble.cbs)
while (ble.enabled == 0):
    time.sleep(1)

print "Starting BLE scanning for 30s: %d" % ble.start_scan()
time.sleep(30);
print "Stopping BLE scanning: %d" % ble.stop_scan()

print "Disabling the adapter: %d" % ble.disable()
time.sleep(2);
