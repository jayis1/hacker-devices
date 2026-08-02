/*
 * board.h — Board-level init and runtime configuration for the BACnet Phantom.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * board_init() is called once at boot and brings up every peripheral in a
 * deterministic order so that the safety interlocks (jumper, tamper loop,
 * eFuse fault) are armed before any radio or bus is allowed to emit.
 * main.c orchestrates the FreeRTOS task table; this file owns the hardware.
 */
#ifndef BACNET_PHANTOM_BOARD_H
#define BACNET_PHANTOM_BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Operational modes (mutually exclusive) ---------------------------- */
typedef enum {
    BP_MODE_PASSIVE     = 0,  /* sniff only, zero emission (default)        */
    BP_MODE_DISCOVER    = 1,  /* emit Who-Is, capture I-Am                 */
    BP_MODE_DUMP        = 2,  /* ReadPropertyMultiple against known devices */
    BP_MODE_INJECT      = 3,  /* WriteProperty (interlock-gated)           */
    BP_MODE_BRIDGE      = 4,  /* BACnet/IP <-> MS/TP router, impersonation  */
    BP_MODE_FUZZ        = 5,  /* replay malformed NPDU from flash           */
    BP_MODE_TAMPERED    = 0xFE,/* zeroised; refuses all commands           */
} bp_mode_t;

/* ---- Top-level runtime state ------------------------------------------- */
typedef struct {
    bp_mode_t   mode;
    uint32_t    device_instance;    /* BACnet device-instance (randomised)   */
    uint32_t    vendor_id;         /* 0 = non-standard vendor, jayis1         */
    uint8_t     network_number_ip;  /* BACnet/IP net (0 = local)             */
    uint8_t     network_number_mstp;/* MS/TP net (operator assigned)         */
    uint16_t    ack_timeout_ms;
    bool        write_bypass;       /* cached copy of jumper state           */
    bool        bridge_active;      /* router impersonation on/off           */
    uint16_t    capture_count;      /* NPDU frames captured this session    */
    uint16_t    write_count;       /* WriteProperty emissions this session  */
    uint16_t    revert_count;      /* watchdog-driven reverts this session  */
} bp_state_t;

extern bp_state_t g_state;

/* ---- Public API -------------------------------------------------------- */
esp_err_t board_init(void);         /* one-time boot */
esp_err_t board_set_mode(bp_mode_t new_mode);

/* Peripherals exposed to the rest of the firmware */
esp_err_t board_w5500_send_raw(const uint8_t *frame, size_t len);
esp_err_t board_w5500_recv_raw(uint8_t *buf, size_t cap, size_t *out_len, int timeout_ms);

esp_err_t board_mstp_tx(const uint8_t *frame, size_t len);
int       board_mstp_rx(uint8_t *buf, size_t cap, int timeout_ms);

void      board_oled_render(const char *line1, const char *line2, const char *line3);
void      board_led_set(uint8_t r, uint8_t g, uint8_t b);
uint16_t  board_battery_mv(void);
bool      board_tamper_triggered(void);

/* Safety: called before any WriteProperty emission */
bool      board_write_permitted(uint16_t object_type);

/* Zeroise on tamper */
void      board_zeroize(void);

#ifdef __cplusplus
}
#endif
#endif /* BACNET_PHANTOM_BOARD_H */