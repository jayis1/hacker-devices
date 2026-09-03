/*
 * SmartPack Phantom SMBus driver interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef SMARTPACK_SMBUS_BUS_H
#define SMARTPACK_SMBUS_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../board.h"

typedef struct {
    uint8_t command;
    bool is_block_read;
    uint8_t length;
    uint8_t data[SP_MAX_MANUF_BLOCK];
    uint16_t word;
    bool mutated;
    bool ack;
    char note[48];
} sp_bus_reply_t;

void smbus_bus_init(void);
void smbus_bus_reset_stats(sp_bus_stats_t *stats);
const char *smbus_bus_command_name(uint8_t command);
void smbus_bus_set_mutation_enabled(bool enabled);
void smbus_bus_set_alert(bool asserted, sp_bus_stats_t *stats);
void smbus_bus_mark_error(sp_bus_stats_t *stats, uint8_t command);
void smbus_bus_prepare_reply(const sp_runtime_t *runtime,
                             uint8_t command,
                             sp_bus_reply_t *reply);
void smbus_bus_apply_delay_policy(sp_bus_reply_t *reply, uint32_t delay_ms);
void smbus_bus_apply_status_mask(sp_bus_reply_t *reply, uint16_t status_mask);
void smbus_bus_record_exchange(sp_bus_stats_t *stats, const sp_bus_reply_t *reply);
size_t smbus_bus_format_reply(const sp_bus_reply_t *reply, char *out, size_t out_len);

#endif
