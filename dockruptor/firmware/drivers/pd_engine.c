/*
 * Dockruptor PD Engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "pd_engine.h"
#include "capture.h"

static void fill_capability(dr_capability_t *cap, uint16_t mv, uint16_t ma, bool pps, const char *label)
{
    cap->millivolts = mv;
    cap->milliamps = ma;
    cap->pps = pps;
    snprintf(cap->label, sizeof(cap->label), "%s", label);
}

void dr_pd_init(dr_context_t *ctx)
{
    memset(&ctx->host, 0, sizeof(ctx->host));
    memset(&ctx->dock, 0, sizeof(ctx->dock));

    ctx->host.power_role = DR_ROLE_SINK;
    ctx->host.data_role = DR_ROLE_DRP;
    ctx->host.mode = DR_MODE_USB;
    ctx->host.attached = false;
    snprintf(ctx->host.identity, sizeof(ctx->host.identity), "%s", "Secure laptop host port");
    snprintf(ctx->host.cable_identity, sizeof(ctx->host.cable_identity), "%s", "USB4 active cable");

    ctx->dock.power_role = DR_ROLE_SOURCE;
    ctx->dock.data_role = DR_ROLE_DRP;
    ctx->dock.mode = DR_MODE_USB;
    ctx->dock.attached = false;
    snprintf(ctx->dock.identity, sizeof(ctx->dock.identity), "%s", "Managed dock with DP alt-mode");
    snprintf(ctx->dock.cable_identity, sizeof(ctx->dock.cable_identity), "%s", "USB4 active cable");

    ctx->dock.capability_count = 4U;
    fill_capability(&ctx->dock.capabilities[0], 5000U, 3000U, false, "5V fixed");
    fill_capability(&ctx->dock.capabilities[1], 9000U, 3000U, false, "9V fixed");
    fill_capability(&ctx->dock.capabilities[2], 15000U, 3000U, false, "15V fixed");
    fill_capability(&ctx->dock.capabilities[3], 20000U, 3250U, false, "20V fixed");

    ctx->host.capability_count = 3U;
    fill_capability(&ctx->host.capabilities[0], 5000U, 3000U, false, "default request");
    fill_capability(&ctx->host.capabilities[1], 9000U, 3000U, false, "preferred dock");
    fill_capability(&ctx->host.capabilities[2], 20000U, 3000U, false, "high-power ask");
}

void dr_pd_attach_default_topology(dr_context_t *ctx)
{
    ctx->host.attached = true;
    ctx->dock.attached = true;
    ctx->host.negotiated_mv = 20000U;
    ctx->host.negotiated_ma = 3000U;
    ctx->dock.negotiated_mv = ctx->host.negotiated_mv;
    ctx->dock.negotiated_ma = ctx->host.negotiated_ma;
    ctx->host.mode = DR_MODE_USB;
    ctx->dock.mode = DR_MODE_USB;
    ctx->host.vconn_enabled = false;
    ctx->dock.vconn_enabled = true;
    dr_capture_event(ctx, DR_EVENT_INFO, DR_DIR_INTERNAL, "host and dock attached; baseline 20V contract observed");
}

static void emit_pd_message(dr_context_t *ctx, dr_direction_t direction, const char *summary)
{
    dr_capture_event(ctx, DR_EVENT_PD_MESSAGE, direction, summary);
}

void dr_pd_force_mode(dr_context_t *ctx, dr_mode_t mode)
{
    char buffer[DR_MAX_TEXT];

    ctx->host.mode = mode;
    ctx->dock.mode = mode;
    snprintf(buffer, sizeof(buffer), "mode transition forced to %d", (int)mode);
    emit_pd_message(ctx, DR_DIR_INTERNAL, buffer);
}

void dr_pd_soft_reset(dr_context_t *ctx)
{
    ctx->host.negotiated_mv = 5000U;
    ctx->host.negotiated_ma = 3000U;
    ctx->dock.negotiated_mv = ctx->host.negotiated_mv;
    ctx->dock.negotiated_ma = ctx->host.negotiated_ma;
    emit_pd_message(ctx, DR_DIR_HOST_TO_DOCK, "Soft_Reset forwarded toward dock");
    emit_pd_message(ctx, DR_DIR_DOCK_TO_HOST, "Accept + PS_RDY replayed toward host");
}

static void maybe_raise_altmode(dr_context_t *ctx)
{
    if (ctx->tick >= 4U && ctx->host.mode == DR_MODE_USB) {
        ctx->host.mode = DR_MODE_DP;
        ctx->dock.mode = DR_MODE_DP;
        emit_pd_message(ctx, DR_DIR_DOCK_TO_HOST, "Enter Mode ACK for DisplayPort alt-mode");
    }
}

static void maybe_recover_high_power(dr_context_t *ctx)
{
    if (ctx->tick >= 12U && ctx->host.negotiated_mv < 9000U) {
        ctx->host.negotiated_mv = 9000U;
        ctx->host.negotiated_ma = 2000U;
        ctx->dock.negotiated_mv = ctx->host.negotiated_mv;
        ctx->dock.negotiated_ma = ctx->host.negotiated_ma;
        emit_pd_message(ctx, DR_DIR_DOCK_TO_HOST, "Renegotiated compact 9V/2A contract");
    }
}

void dr_pd_simulate_tick(dr_context_t *ctx)
{
    if (!ctx->host.attached || !ctx->dock.attached) {
        return;
    }

    switch (ctx->tick) {
    case 1U:
        emit_pd_message(ctx, DR_DIR_DOCK_TO_HOST, "Source_Capabilities observed from managed dock");
        break;
    case 2U:
        emit_pd_message(ctx, DR_DIR_HOST_TO_DOCK, "Request selected high-power contract");
        break;
    case 3U:
        emit_pd_message(ctx, DR_DIR_DOCK_TO_HOST, "Accept + PS_RDY completed contract transition");
        break;
    case 6U:
        emit_pd_message(ctx, DR_DIR_DOCK_TO_HOST, "Discover Identity VDM includes managed dock certificate tag");
        break;
    case 8U:
        emit_pd_message(ctx, DR_DIR_DOCK_TO_HOST, "Discover SVIDs advertises DisplayPort and vendor mode");
        break;
    case 10U:
        emit_pd_message(ctx, DR_DIR_HOST_TO_DOCK, "Enter Mode request for DisplayPort alt-mode");
        break;
    default:
        break;
    }

    maybe_raise_altmode(ctx);
    maybe_recover_high_power(ctx);
}

void dr_pd_emit_contract_summary(const dr_context_t *ctx, char *buffer, size_t buffer_len)
{
    snprintf(buffer,
             buffer_len,
             "%umV @ %umA hostMode=%d dockMode=%d cable=%s",
             ctx->host.negotiated_mv,
             ctx->host.negotiated_ma,
             (int)ctx->host.mode,
             (int)ctx->dock.mode,
             ctx->host.cable_identity);
}

void dr_pd_copy_capabilities(const dr_port_state_t *port, char *buffer, size_t buffer_len)
{
    size_t i = 0U;
    size_t used = 0U;

    if (buffer_len == 0U) {
        return;
    }
    buffer[0] = '\0';

    for (i = 0; i < port->capability_count; ++i) {
        const dr_capability_t *cap = &port->capabilities[i];
        int written = snprintf(buffer + used,
                               buffer_len - used,
                               "%s%uV/%uA",
                               i == 0U ? "" : ", ",
                               cap->millivolts / 1000U,
                               cap->milliamps / 1000U);
        if (written < 0 || (size_t)written >= buffer_len - used) {
            break;
        }
        used += (size_t)written;
    }
}
