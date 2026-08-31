/*
 * drivers/radio.c - MDIO Wraith companion link encoder
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "radio.h"

#include <stdio.h>
#include <string.h>

void mw_radio_build_heartbeat(const mw_system_t *sys, char *buffer, size_t length)
{
    if (buffer == NULL || length == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, length, "MW|hb|invalid");
        return;
    }

    (void)snprintf(buffer,
                   length,
                   "MW|hb|author=%s|profile=%s|armed=%u|mode=%u|temp=%.1f|current=%.1f|voltage=%.2f|events=%u|captures=%u",
                   MW_AUTHOR,
                   sys->status.active_profile,
                   (unsigned int)sys->status.armed,
                   (unsigned int)sys->status.mode,
                   (double)sys->status.board_temp_c,
                   (double)sys->status.target_current_ma,
                   (double)sys->status.target_voltage_v,
                   (unsigned int)sys->event_count,
                   (unsigned int)sys->capture_count);
}

void mw_radio_build_capture_export(const mw_system_t *sys, char *buffer, size_t length, size_t max_frames)
{
    size_t offset = 0u;
    size_t i;
    size_t count;

    if (buffer == NULL || length == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, length, "MW|capture|none");
        return;
    }

    count = sys->capture_count < max_frames ? sys->capture_count : max_frames;
    offset += (size_t)snprintf(buffer + offset, length - offset, "MW|capture|author=%s|count=%u", MW_AUTHOR, (unsigned int)count);

    for (i = 0u; i < count && offset < length; ++i) {
        const mw_capture_frame_t *frame = &sys->captures[i];
        offset += (size_t)snprintf(buffer + offset,
                                   length - offset,
                                   "|%u:%u:%u:0x%02X:0x%04X:%u:%u:%s",
                                   (unsigned int)frame->timestamp_ms,
                                   (unsigned int)frame->phy_addr,
                                   (unsigned int)frame->devad,
                                   (unsigned int)frame->reg,
                                   (unsigned int)frame->value,
                                   (unsigned int)frame->is_write,
                                   (unsigned int)frame->clause45,
                                   frame->note);
    }

    if (offset >= length) {
        buffer[length - 1u] = '\0';
    }
}

void mw_radio_build_profile_frame(const mw_profile_t *profile, char *buffer, size_t length)
{
    if (buffer == NULL || length == 0u) {
        return;
    }

    if (profile == NULL) {
        (void)snprintf(buffer, length, "MW|profile|none");
        return;
    }

    (void)snprintf(buffer,
                   length,
                   "MW|profile|author=%s|name=%s|cl45=%u|strap=%u|isolate=%u|loopback=%u|budget=%u|trigger_phy=%u|trigger_reg=0x%02X",
                   MW_AUTHOR,
                   profile->name,
                   (unsigned int)profile->allow_clause45,
                   (unsigned int)profile->allow_strap_swap,
                   (unsigned int)profile->allow_isolate_write,
                   (unsigned int)profile->allow_loopback_write,
                   (unsigned int)profile->write_budget,
                   (unsigned int)profile->trigger_phy,
                   (unsigned int)profile->trigger_reg);
}
