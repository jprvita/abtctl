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
 * List of callbacks for BLE operations.
 */
typedef struct ble_cbs {
    ble_enable_cb_t enable_cb;
    ble_adapter_state_cb_t adapter_state_cb;
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
#endif
