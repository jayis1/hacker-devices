/*
 * Type-C Specter operator radio abstraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef TYPEC_SPECTER_RADIO_H
#define TYPEC_SPECTER_RADIO_H

#include <stdbool.h>
#include <stdint.h>
#include "../board.h"

typedef enum {
    TS_RADIO_CMD_NONE = 0,
    TS_RADIO_CMD_GET_STATUS,
    TS_RADIO_CMD_ARM_SCENARIO,
    TS_RADIO_CMD_ABORT_SCENARIO,
    TS_RADIO_CMD_LOAD_PROFILE,
    TS_RADIO_CMD_SET_LIMITS
} ts_radio_command_kind_t;

typedef struct {
    ts_radio_command_kind_t kind;
    uint32_t arg0;
    char text[TS_STATUS_TEXT_LEN];
} ts_radio_command_t;

void radio_init(void);
void radio_queue_demo_command(ts_radio_command_kind_t kind, uint32_t arg0, const char *text);
bool radio_poll_command(ts_radio_command_t *out);
void radio_publish_status(const ts_runtime_t *runtime);

#endif
