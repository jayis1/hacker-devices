/*
 * SmartPack Phantom SMBus model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "smbus_bus.h"
#include "battery_profile.h"
#include "auth_chip.h"
#include "../registers.h"

static bool g_mutation_enabled;

void smbus_bus_init(void) {
    g_mutation_enabled = false;
}

void smbus_bus_reset_stats(sp_bus_stats_t *stats) {
    if (stats == 0) {
        return;
    }
    memset(stats, 0, sizeof(*stats));
    stats->transparent_bypass = true;
}

const char *smbus_bus_command_name(uint8_t command) {
    switch (command) {
        case SP_SBS_MANUFACTURER_ACCESS: return "manufacturer-access";
        case SP_SBS_TEMPERATURE: return "temperature";
        case SP_SBS_VOLTAGE: return "voltage";
        case SP_SBS_CURRENT: return "current";
        case SP_SBS_AVG_CURRENT: return "avg-current";
        case SP_SBS_RELATIVE_SOC: return "relative-soc";
        case SP_SBS_ABSOLUTE_SOC: return "absolute-soc";
        case SP_SBS_REMAINING_CAP: return "remaining-capacity";
        case SP_SBS_FULL_CHARGE_CAP: return "full-charge-capacity";
        case SP_SBS_RUNTIME_EMPTY: return "runtime-to-empty";
        case SP_SBS_AVG_TIME_EMPTY: return "avg-time-to-empty";
        case SP_SBS_STATUS: return "battery-status";
        case SP_SBS_CYCLE_COUNT: return "cycle-count";
        case SP_SBS_DESIGN_CAPACITY: return "design-capacity";
        case SP_SBS_DESIGN_VOLTAGE: return "design-voltage";
        case SP_SBS_MANUFACTURER_DATE: return "manufacturer-date";
        case SP_SBS_SERIAL_NUMBER: return "serial-number";
        case SP_SBS_MANUFACTURER_NAME: return "manufacturer-name";
        case SP_SBS_DEVICE_NAME: return "device-name";
        case SP_SBS_DEVICE_CHEMISTRY: return "device-chemistry";
        case SP_SBS_MANUFACTURER_DATA: return "manufacturer-data";
        case SP_VENDOR_AUTH_CHALLENGE: return "auth-challenge";
        case SP_VENDOR_AUTH_RESPONSE: return "auth-response";
        case SP_VENDOR_SERVICE_FLAGS: return "service-flags";
        case SP_VENDOR_CHARGE_POLICY: return "charge-policy";
        default: return "unknown";
    }
}

void smbus_bus_set_mutation_enabled(bool enabled) {
    g_mutation_enabled = enabled;
}

void smbus_bus_set_alert(bool asserted, sp_bus_stats_t *stats) {
    if (stats == 0) {
        return;
    }
    stats->alert_line = asserted;
    if (asserted) {
        stats->alert_assertions++;
    }
}

void smbus_bus_mark_error(sp_bus_stats_t *stats, uint8_t command) {
    if (stats == 0) {
        return;
    }
    stats->command_errors++;
    stats->last_command = command;
}

static void prepare_auth_reply(const sp_runtime_t *runtime, sp_bus_reply_t *reply) {
    bool accepted = false;
    if (runtime == 0 || reply == 0) {
        return;
    }
    if (reply->command == SP_VENDOR_AUTH_CHALLENGE) {
        reply->word = (uint16_t)(runtime->auth.last_challenge & 0xffffu);
        snprintf(reply->note, sizeof(reply->note), "challenge=0x%08x", runtime->auth.last_challenge);
    } else {
        reply->word = (uint16_t)(auth_chip_respond((sp_auth_state_t *)&runtime->auth,
                                                   runtime->auth.last_challenge,
                                                   &accepted) & 0xffffu);
        snprintf(reply->note,
                 sizeof(reply->note),
                 "%s",
                 accepted ? "auth accepted" : "auth failed");
    }
}

void smbus_bus_prepare_reply(const sp_runtime_t *runtime,
                             uint8_t command,
                             sp_bus_reply_t *reply) {
    size_t len;
    if (runtime == 0 || reply == 0) {
        return;
    }
    memset(reply, 0, sizeof(*reply));
    reply->command = command;
    reply->ack = true;
    reply->mutated = false;

    switch (command) {
        case SP_SBS_MANUFACTURER_NAME:
        case SP_SBS_DEVICE_NAME:
        case SP_SBS_DEVICE_CHEMISTRY:
        case SP_SBS_MANUFACTURER_DATA:
            reply->is_block_read = true;
            len = battery_profile_command_block(&runtime->profile, command, reply->data, sizeof(reply->data));
            reply->length = (uint8_t)len;
            snprintf(reply->note, sizeof(reply->note), "%s", smbus_bus_command_name(command));
            break;
        case SP_VENDOR_AUTH_CHALLENGE:
        case SP_VENDOR_AUTH_RESPONSE:
            prepare_auth_reply(runtime, reply);
            break;
        default:
            reply->word = battery_profile_command_word(&runtime->profile, command);
            snprintf(reply->note, sizeof(reply->note), "%s", smbus_bus_command_name(command));
            break;
    }

    if (g_mutation_enabled && command == SP_SBS_STATUS) {
        reply->word ^= 0x0001u;
        reply->mutated = true;
    }
    if (g_mutation_enabled && command == SP_SBS_MANUFACTURER_DATA && reply->length > 0u) {
        memcpy(reply->data, "svc=1;trace=lab;flag=mut", 25u);
        reply->length = 25u;
        reply->mutated = true;
    }
}

void smbus_bus_apply_delay_policy(sp_bus_reply_t *reply, uint32_t delay_ms) {
    if (reply == 0 || delay_ms == 0u) {
        return;
    }
    reply->mutated = true;
    snprintf(reply->note, sizeof(reply->note), "delay=%ums %s", delay_ms, smbus_bus_command_name(reply->command));
}

void smbus_bus_apply_status_mask(sp_bus_reply_t *reply, uint16_t status_mask) {
    if (reply == 0) {
        return;
    }
    if (!reply->is_block_read) {
        reply->word |= status_mask;
        reply->mutated = true;
    }
}

void smbus_bus_record_exchange(sp_bus_stats_t *stats, const sp_bus_reply_t *reply) {
    if (stats == 0 || reply == 0) {
        return;
    }
    stats->host_queries++;
    stats->pack_replies++;
    stats->last_command = reply->command;
    if (reply->mutated) {
        stats->mutated_replies++;
    }
    if (reply->is_block_read) {
        stats->block_reads++;
    }
}

size_t smbus_bus_format_reply(const sp_bus_reply_t *reply, char *out, size_t out_len) {
    size_t i;
    size_t used;
    if (reply == 0 || out == 0 || out_len == 0u) {
        return 0u;
    }
    if (!reply->is_block_read) {
        return (size_t)snprintf(out,
                                out_len,
                                "%s word=0x%04x mutated=%u note=%s",
                                smbus_bus_command_name(reply->command),
                                reply->word,
                                reply->mutated ? 1u : 0u,
                                reply->note);
    }
    used = (size_t)snprintf(out,
                            out_len,
                            "%s block=\"",
                            smbus_bus_command_name(reply->command));
    for (i = 0u; i < reply->length && used + 2u < out_len; ++i) {
        used += (size_t)snprintf(out + used, out_len - used, "%c", (char)reply->data[i]);
    }
    if (used + 32u < out_len) {
        used += (size_t)snprintf(out + used,
                                 out_len - used,
                                 "\" mutated=%u note=%s",
                                 reply->mutated ? 1u : 0u,
                                 reply->note);
    }
    return used;
}
