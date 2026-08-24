/*
 * PoE policy engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NAC_MIRAGE_POE_H
#define NAC_MIRAGE_POE_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

typedef struct {
    uint8_t detected_class;
    uint16_t allocated_mw;
    uint16_t instantaneous_mw;
    bool glitch_armed;
    bool brownout_active;
    uint32_t glitch_at_ms;
} nm_poe_state_t;

void poe_init(void);
void poe_configure(const nm_policy_t *policy);
void poe_tick(uint32_t now_ms, nm_runtime_status_t *status);
void poe_observe_draw(uint16_t milliwatts, uint32_t now_ms);
const nm_poe_state_t *poe_state(void);
void poe_force_class(uint8_t class_id, uint16_t budget_mw);

#endif
