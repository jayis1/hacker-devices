/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Discovery Engine — impedance-guided SWD/JTAG pin auto-discovery
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GHOSTBUS_DISCOVERY_DRIVER_H
#define GHOSTBUS_DISCOVERY_DRIVER_H

#include "registers.h"
#include "board.h"

#define DISCOVERY_MAX_CHANNELS 8

typedef struct {
    probe_protocol_t protocol;
    uint8_t swdio_ch;     /* valid if protocol == PROTO_SWD */
    uint8_t swclk_ch;
    uint8_t tdi_ch;       /* valid if protocol == PROTO_JTAG */
    uint8_t tms_ch;
    uint8_t tck_ch;
    uint8_t tdo_ch;
    uint8_t gnd_ch;
    uint8_t vref_ch;
    uint16_t vref_mv;
    uint32_t idcode;
    uint8_t  confidence;   /* 0-100 */
} discovery_result_t;

/* Public API */
void          discovery_init(void);
gb_status_t   discovery_scan(probe_protocol_t *protocols, uint8_t n_protocols,
                              discovery_result_t *result, uint32_t timeout_ms);
uint16_t      discovery_read_channel_voltage(uint8_t channel);
uint16_t      discovery_measure_resistance(uint8_t ch_a, uint8_t ch_b);
uint8_t       discovery_find_gnd(void);
uint8_t       discovery_find_vref(uint8_t gnd_ch);
void          discovery_test_all_channels(void);

#endif /* GHOSTBUS_DISCOVERY_DRIVER_H */