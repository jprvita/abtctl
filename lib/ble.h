#ifndef __BLE_H__
#define __BLE_H__

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

/** @file
 *
 * \mainpage
 *
 * \section intro_sec Introduction
 *
 * This document describes the libble API, a BLE library for Android which sits
 * on top of libhardware. The objective of this library is to abstract away
 * some libhardware specifics, exposing only what is necessary to create BLE
 * bindings to higher-level languages.
 *
 * Due to Bluedroid and/or libhardware architectural restrictions no more than
 * one process can access the Bluetooth resources on the system at the same
 * time. That means that only one program that makes use of libble can run on
 * the system at a certain time and that Bluetooth should be disabled in the
 * Android GUI (if running).
 */

/**
 * Type that represents a callback function to inform that BLE is enabled and
 * available to be used.
 */
typedef void (*ble_enable_cb_t)(void);

/**
 * Type that represents a callback function to inform an adapter state change.
 *
 * @param state The new state of the adapter: 0 disabled, 1 enabled.
 */
typedef void (*ble_adapter_state_cb_t)(uint8_t state);

/**
 * Type that represents a callback function to notify of a new found device
 * during a scanning session.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the found device, where the
 *                most-significant byte is on position 0 and the
 *                least-sifnificant byte is on position 5.
 * @param rssi The RSSI of the found device.
 * @param adv_data A pointer to the advertising data of the found device.
 */
typedef void (*ble_scan_cb_t)(const uint8_t *address, int rssi,
                              const uint8_t *adv_data);

/**
 * Type that represents a callback function to notify of a new connection with
 * a BLE device or a disconnection from a device.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the remote device, where the
 *                most-significant byte is on position 0 and the
 *                least-sifnificant byte is on position 5.
 * @param conn_id An identifier of the connected remote device. If this is
 *                nonpositive it means the device is not connected.
 * @param status The status in which the connect operation has finished.
 */
typedef void (*ble_connect_cb_t)(const uint8_t *address, int conn_id,
                                 int status);

/**
 * List of callbacks for BLE operations.
 */
typedef struct ble_cbs {
    ble_enable_cb_t enable_cb;
    ble_adapter_state_cb_t adapter_state_cb;
    ble_scan_cb_t scan_cb;
    ble_connect_cb_t connect_cb;
    ble_connect_cb_t disconnect_cb;
} ble_cbs_t;

/**
 * Initialize the BLE stack and necessary interfaces and power on the adapter.
 *
 * @param cbs List of callbacks for BLE operations.
 *
 * @return 0 on success.
 * @return Negative value on failure.
 */
int ble_enable(ble_cbs_t cbs);

/**
 * Power off the adapter, cleanup the BLE features and shutdown the stack.
 *
 * @return 0 on success.
 * @return Negative value on failure.
 */
int ble_disable();

/**
 * Starts a LE scan procedure on the adapter.
 *
 * Scan will run indefinitelly until ble_scan_stop() is called.
 *
 * @return 0 if scan has been started.
 * @return 1 if adapter is already scanning.
 * @return -1 if failed to request scan start.
 */
int ble_start_scan();

/**
 * Stops a running LE scan procedure on the adapter.
 *
 * @return 0 if scan has been stopped.
 * @return 1 if adapter is not scanning.
 * @return -1 if failed to request scan stop.
 */
int ble_stop_scan();

/**
 * Connects to a BLE device.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the remote device a connection should be
 *                requested, where the most-significant byte is on position 0
 *                and the least-sifnificant byte is on position 5.
 *
 * @return 0 if connection has been successfully requested.
 * @return -1 if failed to request connection.
 */
int ble_connect(const uint8_t *address);

/**
 * Disconnects from a BLE device.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the remote device a disconnection should
 *                be requested, where the most-significant byte is on position
 *                0 and the least-sifnificant byte is on position 5.
 *
 * @return 0 if disconnection has been successfully requested.
 * @return -1 if failed to request disconnection.
 */
int ble_disconnect(const uint8_t *address);
#endif
