/*
 * ble_c2.h — BLE command/control (Nordic UART Service style)
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_BLE_H
#define HARMONIC_REAPER_BLE_H

#include <stdint.h>
#include "../board.h"

/* Command opcodes (RX from app → wand) */
#define BLE_CMD_SET_MODE        0x01
#define BLE_CMD_SET_TX_POWER    0x02
#define BLE_CMD_SET_PULSE       0x03
#define BLE_CMD_SET_THRESH      0x04
#define BLE_CMD_SET_QUIET       0x05
#define BLE_CMD_QUERY_STATUS    0x06
#define BLE_CMD_EXPORT         0x07

/* Notification opcodes (TX from wand → app) */
#define BLE_NOTIF_STATUS        0x81
#define BLE_NOTIF_HIT           0x82
#define BLE_NOTIF_EXPORT_DONE   0x83

typedef struct {
    uint8_t op;
    uint16_t arg0;
    uint16_t arg1;
} ble_cmd_t;

typedef struct {
    sweep_mode_t mode;
    int16_t p2_dbfs;
    int16_t p3_dbfs;
    int8_t ratio_db;
    uint8_t classify;
    int8_t tx_power_dbm;
    uint8_t batt_pct;
    uint32_t hit_count;
} ble_status_t;

/* hit_record_t is also defined in storage.h; forward-declared here */
struct hit_record;

void ble_init(void);
uint8_t ble_dequeue_cmd(ble_cmd_t *out);
void ble_send_status(const ble_status_t *s);
void ble_notify_hit(const struct hit_record *h);
void ble_notify_export_done(uint32_t count);

#endif /* HARMONIC_REAPER_BLE_H */
/* EOF — ble_c2.h — jayis1 */