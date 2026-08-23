/*
 * Dockruptor Radio Control Plane
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "radio.h"
#include "capture.h"
#include "pd_engine.h"

static const char *mock_commands[] = {
    "profile:travel-dock",
    "tag:baseline",
    "export:telemetry",
    "toggle:observe-only-off",
    "annotate:authorized-lab-engagement"
};

void dr_radio_init(dr_context_t *ctx)
{
    (void)ctx;
}

void dr_radio_export_status(const dr_context_t *ctx, char *buffer, size_t buffer_len)
{
    char contract[96];
    char caps[160];

    dr_pd_emit_contract_summary(ctx, contract, sizeof(contract));
    dr_pd_copy_capabilities(&ctx->dock, caps, sizeof(caps));

    snprintf(buffer,
             buffer_len,
             "tick=%u battery=%u%% temp=%.1fC mode=%d bypass=%s contract={%s} caps={%s}",
             (unsigned)ctx->tick,
             ctx->power.battery_percent,
             (double)ctx->power.board_temp_c,
             (int)ctx->host.mode,
             ctx->power.relay_bypass ? "yes" : "no",
             contract,
             caps);
}

void dr_radio_process_tick(dr_context_t *ctx)
{
    char buffer[DR_MAX_TEXT];
    const char *command = NULL;
    size_t index = 0U;

    if (ctx->tick == 0U || (ctx->tick % 6U) != 0U) {
        return;
    }

    index = (ctx->tick / 6U) - 1U;
    if (index >= (sizeof(mock_commands) / sizeof(mock_commands[0]))) {
        return;
    }

    command = mock_commands[index];
    snprintf(buffer, sizeof(buffer), "radio command processed: %s", command);
    dr_capture_event(ctx, DR_EVENT_COMMAND, DR_DIR_INTERNAL, buffer);

    if (strcmp(command, "toggle:observe-only-off") == 0 && ctx->power.safe_mode) {
        snprintf(buffer, sizeof(buffer), "observe-only unlock rejected while safe-mode active");
        dr_capture_event(ctx, DR_EVENT_SAFETY, DR_DIR_INTERNAL, buffer);
    }
}
