/*
 * Dockruptor Reference Firmware Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "registers.h"
#include "drivers/capture.h"
#include "drivers/pd_engine.h"
#include "drivers/policy.h"
#include "drivers/power.h"
#include "drivers/radio.h"

static uint32_t dr_read_status(const dr_context_t *ctx)
{
    uint32_t value = 0U;

    if (ctx->host.attached) {
        value |= DR_STATUS_ATTACHED_HOST;
    }
    if (ctx->dock.attached) {
        value |= DR_STATUS_ATTACHED_DOCK;
    }
    if (ctx->power.safe_mode) {
        value |= DR_STATUS_SAFE_MODE;
    }
    if (ctx->observe_only) {
        value |= DR_STATUS_OBSERVE_ONLY;
    }
    if (ctx->power.wireless_enabled) {
        value |= DR_STATUS_WIRELESS;
    }
    return value;
}

static void render_rules(const dr_context_t *ctx, char *buffer, size_t buffer_len)
{
    size_t i = 0U;
    size_t used = 0U;

    if (buffer_len == 0U) {
        return;
    }

    buffer[0] = '\0';
    for (i = 0; i < ctx->rule_count; ++i) {
        const dr_rule_t *rule = &ctx->rules[i];
        int written = snprintf(buffer + used,
                               buffer_len - used,
                               "%s%s(%s)",
                               i == 0U ? "" : ", ",
                               rule->name,
                               rule->enabled ? "on" : "done");
        if (written < 0 || (size_t)written >= buffer_len - used) {
            break;
        }
        used += (size_t)written;
    }
}

static void print_register_snapshot(const dr_context_t *ctx)
{
    printf("\n[register snapshot]\n");
    printf("0x%04X STATUS           = 0x%08X\n", DR_REG_STATUS, dr_read_status(ctx));
    printf("0x%04X HOST_ROLE        = %u\n", DR_REG_HOST_ROLE, (unsigned)ctx->host.power_role);
    printf("0x%04X DOCK_ROLE        = %u\n", DR_REG_DOCK_ROLE, (unsigned)ctx->dock.power_role);
    printf("0x%04X ACTIVE_MODE      = %u\n", DR_REG_ACTIVE_MODE, (unsigned)ctx->host.mode);
    printf("0x%04X NEGOTIATED_MV    = %u\n", DR_REG_NEGOTIATED_MV, (unsigned)ctx->host.negotiated_mv);
    printf("0x%04X NEGOTIATED_MA    = %u\n", DR_REG_NEGOTIATED_MA, (unsigned)ctx->host.negotiated_ma);
    printf("0x%04X BATTERY_PERCENT  = %u\n", DR_REG_BATTERY_PERCENT, (unsigned)ctx->power.battery_percent);
    printf("0x%04X TEMPERATURE_C    = %u\n", DR_REG_TEMPERATURE_C, (unsigned)ctx->power.board_temp_c);
    printf("0x%04X EVENT_COUNT      = %u\n", DR_REG_EVENT_COUNT, (unsigned)ctx->event_count);
    printf("0x%04X RELAY_STATE      = %u\n", DR_REG_RELAY_STATE, ctx->power.relay_bypass ? 1U : 0U);
}

static void print_header(void)
{
    printf("Dockruptor Reference Firmware Simulation\n");
    printf("Author: jayis1\n");
    printf("Purpose: authorized inline USB-C PD and alt-mode security testing\n\n");
}

static void print_context_summary(const dr_context_t *ctx)
{
    char contract[160];
    char capabilities[192];
    char rules[256];

    dr_pd_emit_contract_summary(ctx, contract, sizeof(contract));
    dr_pd_copy_capabilities(&ctx->dock, capabilities, sizeof(capabilities));
    render_rules(ctx, rules, sizeof(rules));

    printf("[summary]\n");
    printf("contract : %s\n", contract);
    printf("dock caps: %s\n", capabilities);
    printf("rules    : %s\n", rules);
    printf("host id  : %s\n", ctx->host.identity);
    printf("dock id  : %s\n", ctx->dock.identity);
    printf("cable id : %s\n", ctx->host.cable_identity);
    printf("battery  : %u%% %.2fV\n", ctx->power.battery_percent, (double)ctx->power.battery_voltage);
    printf("temp     : %.1fC\n", (double)ctx->power.board_temp_c);
    printf("observe  : %s\n", ctx->observe_only ? "yes" : "no");
    printf("safeMode : %s\n\n", ctx->power.safe_mode ? "yes" : "no");
}

static void print_recent_events(const dr_context_t *ctx)
{
    char log_buffer[4096];

    dr_capture_render_recent(ctx, log_buffer, sizeof(log_buffer), 20U);
    printf("[recent events]\n%s\n", log_buffer);
}

static void print_radio_snapshot(const dr_context_t *ctx)
{
    char buffer[320];

    dr_radio_export_status(ctx, buffer, sizeof(buffer));
    printf("[radio export]\n%s\n\n", buffer);
}

static void seed_manual_annotation(dr_context_t *ctx)
{
    dr_capture_event(ctx,
                     DR_EVENT_INFO,
                     DR_DIR_INTERNAL,
                     "engagement profile loaded: authorized conference-room dock assessment");
    dr_capture_event(ctx,
                     DR_EVENT_INFO,
                     DR_DIR_INTERNAL,
                     "operator note: maintain relay fallback and capture all policy actions");
}

static void run_tick(dr_context_t *ctx)
{
    ctx->tick++;
    dr_pd_simulate_tick(ctx);
    dr_policy_apply(ctx);
    dr_radio_process_tick(ctx);
    dr_power_tick(ctx);
}

static void print_timeline_table(const dr_context_t *ctx)
{
    size_t i = 0U;

    printf("[timeline digest]\n");
    printf("tick  event\n");
    printf("----  ---------------------------------------------------------------\n");
    for (i = 0U; i < ctx->event_count && i < 14U; ++i) {
        printf("%4u  %s\n", (unsigned)ctx->events[i].tick, ctx->events[i].summary);
    }
    printf("\n");
}

int main(void)
{
    dr_context_t ctx;
    size_t i = 0U;

    memset(&ctx, 0, sizeof(ctx));

    print_header();

    dr_capture_init(&ctx);
    dr_pd_init(&ctx);
    dr_policy_init(&ctx);
    dr_power_init(&ctx);
    dr_radio_init(&ctx);
    dr_pd_attach_default_topology(&ctx);
    dr_policy_load_default_profile(&ctx);
    seed_manual_annotation(&ctx);

    print_context_summary(&ctx);

    for (i = 0U; i < 18U; ++i) {
        run_tick(&ctx);
    }

    print_context_summary(&ctx);
    print_recent_events(&ctx);
    print_timeline_table(&ctx);
    print_radio_snapshot(&ctx);
    print_register_snapshot(&ctx);

    printf("\nLegal reminder: Dockruptor is for authorized use only.\n");
    return 0;
}
