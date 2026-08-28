/*
 * interposer.c - safety and manipulation engine for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "interposer.h"

void er_interposer_init(er_interposer_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->watchdog_ok = 1u;
    ctx->watchdog_ms = 120u;
}

void er_interposer_arm(er_interposer_t *ctx, uint8_t armed)
{
    ctx->armed = armed ? 1u : 0u;
    if (!ctx->armed) {
        ctx->vwire_pulses_used = 0u;
        ctx->periph_ops_used = 0u;
    }
}

void er_interposer_tick(er_interposer_t *ctx, const er_bus_t *bus, er_status_t *status)
{
    status->armed = ctx->armed;
    status->bypass_enabled = ctx->bypass_enabled;
    status->watchdog_ok = ctx->watchdog_ok;
    status->target_present = 1u;

    if (ctx->armed) {
        uint32_t elapsed = (bus->tick_ms > ctx->last_action_ms) ? (bus->tick_ms - ctx->last_action_ms) : 0u;
        if (elapsed > ctx->watchdog_ms) {
            ctx->watchdog_ok = 0u;
            ctx->bypass_enabled = 1u;
            status->mode = ER_MODE_BYPASS;
            status->bypass_enabled = 1u;
        }
    }
}

uint8_t er_interposer_can_inject_vwire(const er_interposer_t *ctx, const er_profile_t *profile)
{
    return (uint8_t)(ctx->armed && ctx->bypass_enabled == 0u && profile->allow_vwire_inject &&
                     ctx->vwire_pulses_used < profile->max_vwire_pulses);
}

uint8_t er_interposer_can_delay_flash(const er_interposer_t *ctx, const er_profile_t *profile)
{
    return (uint8_t)(ctx->armed && ctx->bypass_enabled == 0u && profile->allow_flash_delay && ctx->watchdog_ok);
}

uint8_t er_interposer_can_replay_peripheral(const er_interposer_t *ctx, const er_profile_t *profile)
{
    return (uint8_t)(ctx->armed && ctx->bypass_enabled == 0u && profile->allow_peripheral_replay &&
                     ctx->periph_ops_used < profile->max_peripheral_ops);
}

void er_interposer_note_action(er_interposer_t *ctx, uint32_t now_ms)
{
    ctx->last_action_ms = now_ms;
    ctx->watchdog_ok = 1u;
}

void er_interposer_force_bypass(er_interposer_t *ctx, er_status_t *status, const char *reason, er_event_t *event_out)
{
    ctx->armed = 0u;
    ctx->bypass_enabled = 1u;
    ctx->watchdog_ok = 0u;
    status->mode = ER_MODE_BYPASS;
    status->bypass_enabled = 1u;
    event_out->code = ER_EVENT_ROLLBACK;
    event_out->channel = ER_CH_GPIO;
    event_out->risk = ER_RISK_LOW;
    snprintf(event_out->message, sizeof(event_out->message), "Bypass asserted: %s", reason);
}
