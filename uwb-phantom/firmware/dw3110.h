/*
 * dw3110.h — Public interface to the Qorvo DW3110 UWB driver.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This header exposes the small surface used by the application:
 *  - dw3110_init              - power up, read device ID, calibrate
 *  - dw3110_configure_channel - choose channel 5 or 9, PRF, preamble
 *  - dw3110_load_sts          - load AES-128 STS key + IV
 *  - dw3110_set_sts_mode      - off / static / dynamic / advanced
 *  - dw3110_tx                - schedule TX, optionally delayed
 *  - dw3110_rx_start          - enter RX (optionally delayed)
 *  - dw3110_rx_read           - copy a received frame + timestamps
 *  - dw3110_read_distance     - compute TWR distance from timestamps
 *  - dw3110_irq_handler       - ISR for the DW3110 IRQ line
 *
 * All functions return 0 on success and a negative errno-style code on
 * failure.
 */

#ifndef UWB_PHANTOM_DW3110_H
#define UWB_PHANTOM_DW3110_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "board.h"
#include "registers.h"

/* ---- Configuration blocks --------------------------------- */

typedef enum {
    UWB_ROLE_TAG    = 0,   /* initiator of ranging */
    UWB_ROLE_ANCHOR  = 1,   /* responder */
} uwb_role_t;

typedef enum {
    UWB_STS_OFF       = STS_MODE_OFF,
    UWB_STS_STATIC    = STS_MODE_STATIC,
    UWB_STS_DYNAMIC   = STS_MODE_DYNAMIC,
    UWB_STS_ADVANCED  = STS_MODE_ADVANCED,
} uwb_sts_mode_t;

typedef struct {
    uint8_t  channel;        /* 5 or 9 */
    uint8_t  prf;            /* UWB_PRF_16M / UWB_PRF_64M */
    uint8_t  plen;           /* preamble length code */
    uint8_t  sfd;            /* UWB_SFD_4A / UWB_SFD_4Z */
    uint8_t  sts_mode;       /* uwb_sts_mode_t */
    uint8_t  sts_len;        /* 32 / 64 / 128 / 256 symbols */
    uint16_t pan_id;         /* 802.15.4 PAN ID */
    uint16_t short_addr;     /* our short address */
    uint8_t  eui[8];         /* our 64-bit EUI */
    uint16_t ant_delay_tx;  /* DTU */
    uint16_t ant_delay_rx;  /* DTU */
} uwb_config_t;

/* ---- Ranging timestamps for TWR -------------------------- */

typedef struct {
    uint64_t tx_stamp;     /* time the frame left our antenna, DTU */
    uint64_t rx_stamp;     /* time we saw their frame, DTU */
    uint16_t ant_delay_tx; /* antenna delays in effect */
    uint16_t ant_delay_rx;
    uint8_t  sts_quality;  /* 0..255 STS correlation quality */
} uwb_ranging_t;

/* ---- Public API ----------------------------------------- */

int  dw3110_init(const uwb_config_t *cfg);
int  dw3110_configure_channel(const uwb_config_t *cfg);
int  dw3110_load_sts(const uint8_t key[STS_KEY_LEN],
                     const uint8_t iv [STS_IV_LEN]);
void dw3110_set_sts_mode(uwb_sts_mode_t mode);
int  dw3110_set_address(uint16_t pan, uint16_t short_addr,
                        const uint8_t eui[8]);

int  dw3110_tx(const uint8_t *frame, size_t len,
               bool with_sts, bool delayed, uint64_t tx_time_dtu);
int  dw3110_rx_start(bool delayed, uint64_t rx_time_dtu);
int  dw3110_rx_read(uint8_t *frame, size_t cap, size_t *out_len,
                    uwb_ranging_t *out);
void dw3110_irq_handler(void *arg);

/* Convenience helpers used by the relay engine */
int  dw3110_read_sys_status(uint32_t *out);
void dw3110_clear_sys_status(uint32_t mask);
int  dw3110_read_sys_time(uint64_t *out_dtu);

/* ---- Time/distance helpers ------------------------------- */

double dw3110_dtu_to_meters_one_way(int64_t dtu);
double dw3110_dtu_to_meters_round_trip(int64_t dtu);

/* Single-sided TWR:
 *   distance = (t_round - t_reply) / 2 * c
 *
 * ds_twr() implements the symmetric double-sided two-way-ranging
 * formula, which cancels the local clock drift to first order:
 *
 *   ToF = ((Tround1 * Tround2 - Treply1 * Treply2) /
 *          (Tround1 + Tround2 + Treply1 + Treply2))
 *   distance = ToF * c
 */
double dw3110_ss_twr_distance(const uwb_ranging_t *init_tx,
                              const uwb_ranging_t *init_rx,
                              const uwb_ranging_t *resp_tx,
                              const uwb_ranging_t *resp_rx);

double dw3110_ds_twr_distance(const uwb_ranging_t *init_tx,
                              const uwb_ranging_t *resp1_rx,
                              const uwb_ranging_t *resp1_tx,
                              const uwb_ranging_t *init_rx,
                              const uwb_ranging_t *final_tx,
                              const uwb_ranging_t *resp2_rx);

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_DW3110_H */