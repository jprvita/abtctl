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

/** BLE device bond state. */
typedef enum {
    BLE_BOND_NONE,     /**< There is no bond with the remote device. */
    BLE_BOND_BONDING,  /**< Pairing with the remote device is ongoing. */
    BLE_BOND_BONDED    /**< The remote device is bonded. */
} ble_bond_state_t;

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
 * Type that represents a callback function to notify when the bond state with a
 * BLE device changes.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the remote device, where the
 *                most-significant byte is on position 0 and the
 *                least-sifnificant byte is on position 5.
 * @param state The new bonding state.
 * @param status The status in which the pair or remove bond operation has
 *               finished.
 */
typedef void (*ble_bond_state_cb_t)(const uint8_t *address,
                                    ble_bond_state_t state, int status);

/**
 * Type that represents a callback function to inform the RSSI of the remote
 * BLE device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param rssi The RSSI of the remote device.
 * @param status The status in which the read remote RSSI operation has
 *               finished.
 */
typedef void (*ble_rssi_cb_t)(int conn_id, int rssi, int status);

/**
 * Type that represents a callback function to notify of a new GATT element
 * (service, characteristic or descriptor) found in a BLE device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param id ID of the service, characteristic or descriptor.
 * @param uuid A pointer to a 16 element array representing each part of the
 *             service, characteristic or descriptor UUID.
 * @param props Additional element properties. For services, whether the service
 *              is primary or not: 1 primary, 0 included. Always zero for other
 *              GATT elements.
 */
typedef void (*ble_gatt_found_cb_t)(int conn_id, int id, const uint8_t *uuid,
                                    int props);

/**
 * Type that represents a callback function to notify that a GATT operation has
 * finished.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param status The status in which the operation has finished.
 */
typedef void (*ble_gatt_finished_cb_t)(int conn_id, int status);

/**
 * Type that represents a callback function to forward a GATT operation
 * response.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param id ID of the GATT element that the operation was executed uppon.
 * @param value The value in the operation response.
 * @param value_len The length of the data pointed by the value parameter.
 * @param value_type The type of the data pointed by the value parameter.
 * @param status The status in which the operation has finished.
 *
 * TODO: Document the semantics of value_type
 */
typedef void (*ble_gatt_response_cb_t)(int conn_id, int id,
                                       const uint8_t *value, uint16_t value_len,
                                       uint16_t value_type, int status);

/**
 * Type that represents a callback function to notify that a registration for
 * characteristic notification has finished.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id ID of the characteristic to register for notifications.
 * @param registered Whether notifications for the characteristic has been
 *                   successfully registered: 1 yes, 0 no.
 * @param status The response status of the registration operation.
 */
typedef void (*ble_gatt_notification_register_cb_t)(int conn_id, int char_id,
                                                    int registered, int status);

/**
 * Type that represents a callback function to forward a GATT notification or
 * indication.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id ID of the characteristic that the notification refers to.
 * @param value The new value of the characteristic.
 * @param value_len The length of the data pointed by the value parameter.
 * @param is_indication Whether the received packed is a notification or an
 *                      indication: 0 notification, 1 indication.
 */
typedef void (*ble_gatt_notification_cb_t)(int conn_id, int char_id,
                                           const uint8_t *value,
                                           uint16_t value_len,
                                           uint8_t is_indication);

/**
 * List of callbacks for BLE operations.
 */
typedef struct ble_cbs {
    ble_enable_cb_t enable_cb;
    ble_adapter_state_cb_t adapter_state_cb;
    ble_scan_cb_t scan_cb;
    ble_connect_cb_t connect_cb;
    ble_connect_cb_t disconnect_cb;
    ble_bond_state_cb_t bond_state_cb;
    ble_rssi_cb_t rssi_cb;
    ble_gatt_found_cb_t srvc_found_cb;
    ble_gatt_finished_cb_t srvc_finished_cb;
    ble_gatt_found_cb_t char_found_cb;
    ble_gatt_finished_cb_t char_finished_cb;
    ble_gatt_found_cb_t desc_found_cb;
    ble_gatt_finished_cb_t desc_finished_cb;
    ble_gatt_response_cb_t char_read_cb;
    ble_gatt_response_cb_t desc_read_cb;
    ble_gatt_response_cb_t char_write_cb;
    ble_gatt_response_cb_t desc_write_cb;
    ble_gatt_notification_register_cb_t char_notification_register_cb;
    ble_gatt_notification_cb_t char_notification_cb;
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

/**
 * Pairs with a BLE device.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the remote device a pairing should be
 *                requested, where the most-significant byte is on position 0
 *                and the least-sifnificant byte is on position 5. *
 *
 * @return 0 if pairing has been successfully requested.
 * @return -1 if failed to pair.
 */
int ble_pair(const uint8_t *address);

/**
 * Cancel pairing with a BLE device.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the remote device which the bonding
 *                should be removed, where the most-significant byte is on
 *                position 0 and the least-sifnificant byte is on position 5.
 *
 * @return 0 if pairing has been successfully cancelled.
 * @return -1 if failed to cancel pairing.
 */
int ble_cancel_pairing(const uint8_t *address);

/**
 * Remove bond with a BLE device.
 *
 * @param address A pointer to a 6 element array representing each part of the
 *                Bluetooth address of the remote device which the bonding
 *                should be removed, where the most-significant byte is on
 *                position 0 and the least-sifnificant byte is on position 5.
 *
 * @return 0 if bond has been successfully removed.
 * @return -1 if failed to remove bond.
 */
int ble_remove_bond(const uint8_t *address);

/**
 * Read the RSSI of a remote BLE device.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 */
int ble_read_remote_rssi(int conn_id);

/**
 * Discover services in a BLE device.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param uuid Service UUID: if not NULL then only services with this UUID will
 *             be notified.
 *
 * @return 0 if service discovery has been successfully requested.
 * @return -1 if failed to request service discovery.
 */
int ble_gatt_discover_services(int conn_id, const uint8_t *uuid);

/**
 * Discover characteristics in a service of a BLE device.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param service_id The identifier of the remote service in which the
 *                   characteristic discovery will be run.
 *
 * @return 0 if characteristic discovery has been successfully requested.
 * @return -1 if failed to request characteristic discovery.
 */
int ble_gatt_discover_characteristics(int conn_id, int service_id);

/**
 * Discover descriptors of a characteristic of a BLE device.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id The identifier of the characteristic in which the descriptor
 *                discovery will be run.
 *
 * @return 0 if descriptor discovery has been successfully requested.
 * @return -1 if failed to request descriptor discovery.
 */
int ble_gatt_discover_descriptors(int conn_id, int char_id);

/**
 * Read the value of a characteristic.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id The identifier of the characteristic to be read.
 * @param auth Whether or not link authentication should be requested before
 *             trying to read the characteristic: 1 request, 0 do not request.
 *
 * @return 0 if characteristic read has been successfully requested.
 * @return -1 if failed to request characteristic read.
 */
int ble_gatt_read_char(int conn_id, int char_id, int auth);

/**
 * Read the value of a characteristic descriptor.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param desc_id The identifier of the descriptor to be read.
 * @param auth Whether or not link authentication should be requested before
 *             trying to read the characteristic: 1 request, 0 do not request.
 *
 * @return 0 if characteristic read has been successfully requested.
 * @return -1 if failed to request characteristic read.
 */
int ble_gatt_read_desc(int conn_id, int desc_id, int auth);

/**
 * Write the value of a characteristic using write command (no response).
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id The identifier of the characteristic to be written.
 * @param auth Whether or not link authentication should be requested before
 *             trying to write the characteristic: 1 request, 0 do not request.
 * @param value Pointer to the value that should be written on the
 *              characteristic.
 * @param len The length of the data pointed by the value parameter.
 *
 * @return 0 if characteristic write has been successfully requested.
 * @return -1 if failed to request characteristic write.
 */
int ble_gatt_write_cmd_char(int conn_id, int char_id, int auth,
                            const char *value, int len);

/**
 * Write the value of a characteristic using write request (with response).
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id The identifier of the characteristic to be written.
 * @param auth Whether or not link authentication should be requested before
 *             trying to write the characteristic: 1 request, 0 do not request.
 * @param value Pointer to the value that should be written on the
 *              characteristic.
 * @param len The length of the data pointed by the value parameter.
 *
 * @return 0 if characteristic write has been successfully requested.
 * @return -1 if failed to request characteristic write.
 */
int ble_gatt_write_req_char(int conn_id, int char_id, int auth,
                            const char *value, int len);

/**
 * Write the value of a descriptor using write command (no response).
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param desc_id The identifier of the descriptor to be written.
 * @param auth Whether or not link encryption should be requested before trying
 *             to write the descriptor: 1 request, 0 do not request.
 * @param value Pointer to the value that should be written on the
 *              descriptor.
 * @param len The length of the data pointed by the value parameter.
 *
 * @return 0 if descriptor write has been successfully requested.
 * @return -1 if failed to request descriptor write.
 */
int ble_gatt_write_cmd_desc(int conn_id, int desc_id, int auth,
                            const char *value, int len);

/**
 * Write the value of a descriptor using write request (with response).
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param desc_id The identifier of the descriptor to be written.
 * @param auth Whether or not link encryption should be requested before trying
 *             to write the descriptor: 1 request, 0 do not request.
 * @param value Pointer to the value that should be written on the
 *              descriptor.
 * @param len The length of the data pointed by the value parameter.
 *
 * @return 0 if descriptor write has been successfully requested.
 * @return -1 if failed to request descriptor write.
 */
int ble_gatt_write_req_desc(int conn_id, int desc_id, int auth,
                            const char *value, int len);

/**
 * Prepare to write the value of a characteristic (with response) later, with
 * @func ble_execute_write().
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id The identifier of the characteristic to be written.
 * @param auth Whether or not link encryption should be requested before trying
 *             to write the descriptor: 1 request, 0 do not request.
 * @param value Pointer to the value that should be written on the
 *              descriptor.
 * @param len The length of the data pointed by the value parameter.
 *
 * @return 0 if descriptor write has been successfully requested.
 * @return -1 if failed to request descriptor write.
 */
int ble_gatt_prep_write_char(int conn_id, int char_id, int auth,
                             const char *value, int len);

/**
 * Prepare to write the value of a descriptor (with response) later, with
 * @func ble_execute_write().
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param desc_id The identifier of the descriptor to be written.
 * @param auth Whether or not link encryption should be requested before trying
 *             to write the descriptor: 1 request, 0 do not request.
 * @param value Pointer to the value that should be written on the
 *              descriptor.
 * @param len The length of the data pointed by the value parameter.
 *
 * @return 0 if descriptor write has been successfully requested.
 * @return -1 if failed to request descriptor write.
 */
int ble_gatt_prep_write_desc(int conn_id, int desc_id, int auth,
                             const char *value, int len);

/**
 * Execute or cancel a previously prepared write operation.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param execute Whether the operation should be executed or cancelled:
 * 1 request, 0 do not request.
 *
 * @return 0 if descriptor write has been successfully requested.
 * @return -1 if failed to request descriptor write.
 */
int ble_gatt_execute_write(int conn_id, int execute);

/**
 * Register for notifications of changes in the value of a characteristic.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id The identifier of the characteristic to register for
 *                notifications.
 *
 * @return 0 if registration for characteristic notifications have been
 *           successfully requested.
 * @return -1 if failed to request registration for characteristic
 *            notifications.
 */
int ble_gatt_register_char_notification(int conn_id, int char_id);

/**
 * Unregister for notifications of changes in the value of a characteristic.
 *
 * There should be an active connection with the device.
 *
 * @param conn_id The identifier of the connected remote device.
 * @param char_id The identifier of the characteristic to unregister for
 *                notifications.
 *
 * @return 0 if deregistration for characteristic notifications have been
 *           successfully requested.
 * @return -1 if failed to request deregistration for characteristic
 *            notifications.
 */
int ble_gatt_unregister_char_notification(int conn_id, int char_id);
#endif
