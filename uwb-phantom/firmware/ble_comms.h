/*
 * ble_comms.h — BLE GATT service for companion app control.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UWB_PHANTOM_BLE_H
#define UWB_PHANTOM_BLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Authorised target record — HMAC-SHA256-signed by the companion app.
 * The firmware refuses to enter any TX mode until at least one record
 * is loaded.  This is a tamper-resistant (not tamper-proof) guard. */
typedef struct {
    uint8_t  eui[8];              /* target EUI-64 */
    char     label[24];           /* human label, e.g. "BMW iX VIN..." */
    uint8_t  hmac[32];           /* HMAC-SHA256 over (eui||label||nonce) */
    uint64_t nonce;              /* anti-replay */
} target_record_t;

int  ble_comms_init(void);
void ble_comms_advertise(bool on);
void ble_comms_notify_event(const char *json);
void ble_comms_notify_log(const uint8_t *pcap_chunk, size_t n);

bool ble_comms_target_loaded(void);
bool ble_comms_check_target(const uint8_t eui[8]);
int  ble_comms_add_target(const target_record_t *t);
int  ble_comms_list_targets(target_record_t *out, size_t cap, size_t *n);
void ble_comms_clear_targets(void);

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_BLE_H */