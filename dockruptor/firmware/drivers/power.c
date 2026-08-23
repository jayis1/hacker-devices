/*
 * Dockruptor Power and Safety
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>

#include "power.h"
#include "capture.h"

void dr_power_init(dr_context_t *ctx)
{
    ctx->power.battery_percent = 96U;
    ctx->power.battery_voltage = 4.08f;
    ctx->power.board_temp_c = 31.5f;
    ctx->power.relay_bypass = false;
    ctx->power.wireless_enabled = true;
    ctx->power.safe_mode = false;
}

void dr_power_assert_bypass(dr_context_t *ctx, const char *reason)
{
    char buffer[DR_MAX_TEXT];

    ctx->power.relay_bypass = true;
    ctx->power.safe_mode = true;
    ctx->observe_only = true;
    snprintf(buffer, sizeof(buffer), "relay bypass asserted: %s", reason);
    dr_capture_event(ctx, DR_EVENT_SAFETY, DR_DIR_INTERNAL, buffer);
}

void dr_power_tick(dr_context_t *ctx)
{
    char buffer[DR_MAX_TEXT];

    if (!ctx->power.relay_bypass && (ctx->tick % 8U) == 0U && ctx->power.battery_percent > 14U) {
        ctx->power.battery_percent--;
        ctx->power.battery_voltage -= 0.01f;
    }

    if (!ctx->power.safe_mode && (ctx->tick % 5U) == 0U) {
        ctx->power.board_temp_c += 0.2f;
    } else if (ctx->power.safe_mode && ctx->power.board_temp_c > 32.0f) {
        ctx->power.board_temp_c -= 0.4f;
    }

    if (ctx->power.board_temp_c > 46.0f && !ctx->power.safe_mode) {
        dr_power_assert_bypass(ctx, "thermal threshold exceeded");
    }

    if (ctx->power.battery_percent < 18U && !ctx->power.safe_mode) {
        dr_power_assert_bypass(ctx, "battery reserve low");
    }

    if ((ctx->tick % 10U) == 0U) {
        snprintf(buffer,
                 sizeof(buffer),
                 "battery=%u%% %.2fV temp=%.1fC bypass=%s",
                 ctx->power.battery_percent,
                 (double)ctx->power.battery_voltage,
                 (double)ctx->power.board_temp_c,
                 ctx->power.relay_bypass ? "yes" : "no");
        dr_capture_event(ctx, DR_EVENT_POWER, DR_DIR_INTERNAL, buffer);
    }
}
