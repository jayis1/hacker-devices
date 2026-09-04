/*
 * sideband_bus.h
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef MCTP_WRAITH_SIDEBAND_BUS_H
#define MCTP_WRAITH_SIDEBAND_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../board.h"

typedef struct {
    mw_message_t ring[24];
    size_t count;
    size_t read_index;
    uint32_t seq;
    uint32_t delay_budget_ms;
    bool mutate_enabled;
    bool capture_enabled;
} mw_bus_state_t;

void sideband_bus_init(void);
void sideband_bus_set_kind(mw_bus_kind_t kind);
void sideband_bus_enable_mutation(bool enabled);
void sideband_bus_enable_capture(bool enabled);
void sideband_bus_set_delay_budget(uint32_t delay_ms);
void sideband_bus_seed_demo_traffic(void);
bool sideband_bus_next_message(mw_message_t *message);
void sideband_bus_inject_message(const mw_message_t *message);
void sideband_bus_capture_message(const mw_message_t *message, const char *reason);
void sideband_bus_print_capture_summary(void);
const mw_bus_state_t *sideband_bus_state(void);

#endif
