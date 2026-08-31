/*
 * drivers/mdio_bus.c - MDIO bus model and capture helpers
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "mdio_bus.h"

#include <stdio.h>
#include <string.h>

#include "../registers.h"
#include "safety.h"

static void mw_record_event(mw_system_t *sys,
                            mw_event_code_t code,
                            mw_channel_t channel,
                            mw_risk_t risk,
                            const char *message)
{
    size_t index;

    if (sys == NULL || sys->event_count >= MW_MAX_EVENTS) {
        return;
    }

    index = sys->event_count++;
    sys->events[index].timestamp_ms = sys->tick_ms;
    sys->events[index].code = code;
    sys->events[index].channel = channel;
    sys->events[index].risk = risk;
    (void)snprintf(sys->events[index].message,
                   sizeof(sys->events[index].message),
                   "%s",
                   message != NULL ? message : "none");
}

static void mw_record_capture(mw_system_t *sys,
                              uint8_t phy,
                              uint8_t devad,
                              uint16_t reg,
                              uint16_t value,
                              uint8_t is_write,
                              uint8_t clause45,
                              const char *note)
{
    size_t index;

    if (sys == NULL || sys->capture_count >= MW_CAPTURE_FRAMES) {
        return;
    }

    index = sys->capture_count++;
    sys->captures[index].timestamp_ms = sys->tick_ms;
    sys->captures[index].phy_addr = phy;
    sys->captures[index].devad = devad;
    sys->captures[index].reg = reg;
    sys->captures[index].value = value;
    sys->captures[index].is_write = is_write;
    sys->captures[index].clause45 = clause45;
    (void)snprintf(sys->captures[index].note,
                   sizeof(sys->captures[index].note),
                   "%s",
                   note != NULL ? note : "capture");
}

static void mw_seed_phy(mw_phy_t *phy,
                        uint8_t addr,
                        const char *label,
                        uint16_t id1,
                        uint16_t id2,
                        uint16_t bmcr,
                        uint16_t bmsr,
                        uint8_t clause45)
{
    size_t i;

    if (phy == NULL) {
        return;
    }

    (void)memset(phy, 0, sizeof(*phy));
    phy->phy_addr = addr;
    phy->phyidr1 = id1;
    phy->phyidr2 = id2;
    phy->bmcr = bmcr;
    phy->bmsr = bmsr;
    phy->has_clause45 = clause45;
    phy->link_up = (uint8_t)((bmsr & MW_BMSR_LINK_STATUS) != 0u);
    phy->oui_hi = (uint8_t)((id1 >> 10) & 0x3Fu);
    phy->oui_mid = (uint16_t)(((id1 & 0x03FFu) << 6) | ((id2 >> 10) & 0x003Fu));
    (void)snprintf(phy->phy_label, sizeof(phy->phy_label), "%s", label);

    for (i = 0u; i < MW_MAX_SNAPSHOT_REGS; ++i) {
        phy->vendor_regs[i] = (uint16_t)(0x0100u + (uint16_t)(addr * 0x10u) + (uint16_t)i);
    }
}

static mw_phy_t *mw_lookup_phy_mutable(mw_system_t *sys, uint8_t phy)
{
    size_t i;

    if (sys == NULL) {
        return NULL;
    }

    for (i = 0u; i < sys->phy_count; ++i) {
        if (sys->phys[i].phy_addr == phy) {
            return &sys->phys[i];
        }
    }

    return NULL;
}

const mw_phy_t *mw_mdio_lookup_phy(const mw_system_t *sys, uint8_t phy)
{
    size_t i;

    if (sys == NULL) {
        return NULL;
    }

    for (i = 0u; i < sys->phy_count; ++i) {
        if (sys->phys[i].phy_addr == phy) {
            return &sys->phys[i];
        }
    }

    return NULL;
}

void mw_mdio_init(mw_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    sys->phy_count = 4u;
    mw_seed_phy(&sys->phys[0], 0u, "uplink-phy", 0x2000u, 0xA231u, MW_BMCR_AUTONEG_EN | MW_BMCR_DUPLEX, MW_BMSR_LINK_STATUS | MW_BMSR_AUTONEG_COMPLETE | MW_BMSR_AUTONEG_ABLE, 1u);
    mw_seed_phy(&sys->phys[1], 1u, "mgmt-phy", 0x0022u, 0x1631u, MW_BMCR_AUTONEG_EN, MW_BMSR_AUTONEG_ABLE | MW_BMSR_EXT_CAPABILITY, 1u);
    mw_seed_phy(&sys->phys[2], 2u, "poe-sidecar", 0x0141u, 0x0DD0u, MW_BMCR_AUTONEG_EN | MW_BMCR_DUPLEX, MW_BMSR_LINK_STATUS | MW_BMSR_AUTONEG_ABLE, 0u);
    mw_seed_phy(&sys->phys[3], 7u, "backplane-phy", 0x03A1u, 0xB5A0u, MW_BMCR_AUTONEG_EN | MW_BMCR_DUPLEX, MW_BMSR_LINK_STATUS | MW_BMSR_AUTONEG_COMPLETE | MW_BMSR_EXT_STATUS, 1u);
    sys->status.discovered_phys = (uint16_t)sys->phy_count;

    mw_record_event(sys, MW_EVENT_BOOT, MW_CHANNEL_CONTROL, MW_RISK_LOW, "mdio-bus-initialized");
}

void mw_mdio_scan(mw_system_t *sys)
{
    size_t i;
    char message[MW_MAX_MESSAGE];

    if (sys == NULL) {
        return;
    }

    for (i = 0u; i < sys->phy_count; ++i) {
        const mw_phy_t *phy = &sys->phys[i];
        (void)snprintf(message,
                       sizeof(message),
                       "scan phy=%u label=%s link=%u clause45=%u id=%04X:%04X",
                       (unsigned int)phy->phy_addr,
                       phy->phy_label,
                       (unsigned int)phy->link_up,
                       (unsigned int)phy->has_clause45,
                       (unsigned int)phy->phyidr1,
                       (unsigned int)phy->phyidr2);
        mw_record_event(sys, MW_EVENT_SCAN, MW_CHANNEL_MDIO, MW_RISK_LOW, message);
        mw_record_capture(sys, phy->phy_addr, 0u, MW_REG_PHYIDR1, phy->phyidr1, 0u, 0u, "scan-id1");
        mw_record_capture(sys, phy->phy_addr, 0u, MW_REG_PHYIDR2, phy->phyidr2, 0u, 0u, "scan-id2");
    }
}

static uint16_t mw_clause22_read(const mw_phy_t *phy, uint16_t reg)
{
    if (phy == NULL) {
        return 0xFFFFu;
    }

    switch (reg) {
    case MW_REG_BMCR:
        return phy->bmcr;
    case MW_REG_BMSR:
        return phy->bmsr;
    case MW_REG_PHYIDR1:
        return phy->phyidr1;
    case MW_REG_PHYIDR2:
        return phy->phyidr2;
    case MW_REG_ANER:
        return phy->has_clause45 ? 0x0001u : 0x0000u;
    case MW_REG_VENDOR_PAGE:
        return 0x0004u;
    default:
        return phy->vendor_regs[reg % MW_MAX_SNAPSHOT_REGS];
    }
}

static uint16_t mw_clause45_read(const mw_phy_t *phy, uint8_t devad, uint16_t reg)
{
    if (phy == NULL) {
        return 0xFFFFu;
    }

    if (devad == MW_DEVAD_PMA && reg == 0x0001u) {
        return (uint16_t)(0x8000u | (uint16_t)phy->link_up);
    }
    if (devad == MW_DEVAD_AUTONEG && reg == 0x8000u) {
        return (uint16_t)(0x1200u | (uint16_t)phy->link_up);
    }
    if (devad == MW_DEVAD_VENDOR1) {
        return phy->vendor_regs[reg % MW_MAX_SNAPSHOT_REGS];
    }

    return (uint16_t)(0x0400u | ((uint16_t)devad << 4) | (reg & 0x000Fu));
}

static void mw_evaluate_trigger(mw_system_t *sys, uint8_t phy, uint16_t reg, uint16_t value)
{
    char message[MW_MAX_MESSAGE];
    uint16_t mask;
    uint16_t expected;

    if (sys == NULL) {
        return;
    }

    if (sys->status.active_profile[0] == '\0') {
        return;
    }

    mask = 0u;
    expected = 0u;
    {
        size_t i;
        extern const mw_profile_t *mw_profile_find(const char *name);
        const mw_profile_t *profile = mw_profile_find(sys->status.active_profile);
        if (profile == NULL) {
            return;
        }
        if (!(profile->trigger_phy == MW_TRIGGER_PHY_ANY || profile->trigger_phy == phy)) {
            return;
        }
        if (!(profile->trigger_reg == MW_TRIGGER_REG_ANY || profile->trigger_reg == reg)) {
            return;
        }
        mask = profile->trigger_value_mask;
        expected = profile->trigger_value_expected;
        i = 0u;
        (void)i;
    }

    if ((value & mask) == expected) {
        sys->status.trigger_seen = 1u;
        (void)snprintf(message,
                       sizeof(message),
                       "trigger phy=%u reg=0x%02X value=0x%04X",
                       (unsigned int)phy,
                       (unsigned int)reg,
                       (unsigned int)value);
        mw_record_event(sys, MW_EVENT_TRIGGER_HIT, MW_CHANNEL_TRIGGER, MW_RISK_MEDIUM, message);
    }
}

uint16_t mw_mdio_read(mw_system_t *sys, uint8_t phy, uint8_t devad, uint16_t reg, uint8_t clause45)
{
    const mw_phy_t *entry;
    uint16_t value;
    char message[MW_MAX_MESSAGE];

    if (sys == NULL) {
        return 0xFFFFu;
    }

    entry = mw_mdio_lookup_phy(sys, phy);
    if (entry == NULL) {
        ++sys->status.anomalies;
        mw_record_event(sys, MW_EVENT_ANOMALY, MW_CHANNEL_MDIO, MW_RISK_MEDIUM, "read-missing-phy");
        return 0xFFFFu;
    }

    value = clause45 ? mw_clause45_read(entry, devad, reg) : mw_clause22_read(entry, reg);
    (void)snprintf(message,
                   sizeof(message),
                   "read phy=%u dev=%u reg=0x%02X value=0x%04X clause45=%u",
                   (unsigned int)phy,
                   (unsigned int)devad,
                   (unsigned int)reg,
                   (unsigned int)value,
                   (unsigned int)clause45);
    mw_record_event(sys, MW_EVENT_READ, MW_CHANNEL_MDIO, MW_RISK_LOW, message);
    mw_record_capture(sys, phy, devad, reg, value, 0u, clause45, "read");
    mw_evaluate_trigger(sys, phy, reg, value);
    return value;
}

static void mw_apply_write_side_effects(mw_phy_t *phy, uint16_t reg, uint16_t value)
{
    if (phy == NULL) {
        return;
    }

    if (reg == MW_REG_BMCR) {
        phy->bmcr = value;
        phy->link_up = (uint8_t)(((value & MW_BMCR_ISOLATE) == 0u) && ((value & MW_BMCR_LOOPBACK) == 0u) && phy->link_up);
        if ((value & MW_BMCR_RESTART_AUTONEG) != 0u) {
            phy->bmsr |= MW_BMSR_AUTONEG_COMPLETE;
        }
        if ((value & MW_BMCR_ISOLATE) != 0u) {
            phy->bmsr &= (uint16_t)~MW_BMSR_LINK_STATUS;
        }
        if ((value & MW_BMCR_LOOPBACK) != 0u) {
            phy->bmsr |= MW_BMSR_LINK_STATUS;
            phy->link_up = 1u;
        }
        return;
    }

    if (reg == MW_REG_VENDOR_PAGE) {
        phy->vendor_regs[0] = value;
        return;
    }

    phy->vendor_regs[reg % MW_MAX_SNAPSHOT_REGS] = value;
}

int mw_mdio_write(mw_system_t *sys, const mw_profile_t *profile, uint8_t phy, uint8_t devad, uint16_t reg, uint16_t value, uint8_t clause45)
{
    mw_phy_t *entry;
    char reason[64];
    char message[MW_MAX_MESSAGE];

    (void)devad;
    (void)clause45;

    if (sys == NULL || profile == NULL) {
        return 0;
    }

    entry = mw_lookup_phy_mutable(sys, phy);
    if (entry == NULL) {
        ++sys->status.anomalies;
        mw_record_event(sys, MW_EVENT_ANOMALY, MW_CHANNEL_MDIO, MW_RISK_MEDIUM, "write-missing-phy");
        return 0;
    }

    if (!mw_safety_can_write(sys, profile, reg, value, reason, sizeof(reason))) {
        (void)snprintf(message,
                       sizeof(message),
                       "write-block phy=%u reg=0x%02X value=0x%04X reason=%s",
                       (unsigned int)phy,
                       (unsigned int)reg,
                       (unsigned int)value,
                       reason);
        mw_record_event(sys, MW_EVENT_GUARD_BLOCK, MW_CHANNEL_CONTROL, MW_RISK_MEDIUM, message);
        return 0;
    }

    mw_apply_write_side_effects(entry, reg, value);
    ++sys->status.writes_applied;

    (void)snprintf(message,
                   sizeof(message),
                   "write phy=%u reg=0x%02X value=0x%04X profile=%s",
                   (unsigned int)phy,
                   (unsigned int)reg,
                   (unsigned int)value,
                   profile->name);
    mw_record_event(sys, MW_EVENT_WRITE, MW_CHANNEL_MDIO, MW_RISK_MEDIUM, message);
    mw_record_capture(sys, phy, devad, reg, value, 1u, clause45, "write");

    if (reg == MW_REG_VENDOR_PAGE && profile->allow_strap_swap) {
        mw_record_event(sys, MW_EVENT_STRAP_SWAP, MW_CHANNEL_STRAP, MW_RISK_HIGH, "strap-shadow-page-select");
    }

    return 1;
}

void mw_mdio_snapshot_vendor(mw_system_t *sys, uint8_t phy)
{
    mw_phy_t *entry;
    size_t i;
    char message[MW_MAX_MESSAGE];

    if (sys == NULL) {
        return;
    }

    entry = mw_lookup_phy_mutable(sys, phy);
    if (entry == NULL) {
        return;
    }

    for (i = 0u; i < MW_MAX_SNAPSHOT_REGS; ++i) {
        mw_record_capture(sys,
                          entry->phy_addr,
                          MW_DEVAD_VENDOR1,
                          (uint16_t)i,
                          entry->vendor_regs[i],
                          0u,
                          1u,
                          "vendor-snapshot");
    }

    (void)snprintf(message,
                   sizeof(message),
                   "snapshot phy=%u vendor-regs=%u",
                   (unsigned int)phy,
                   (unsigned int)MW_MAX_SNAPSHOT_REGS);
    mw_record_event(sys, MW_EVENT_CAPTURE, MW_CHANNEL_MDIO, MW_RISK_LOW, message);
}

void mw_mdio_capture_summary(const mw_system_t *sys, char *buffer, size_t length)
{
    size_t offset = 0u;
    size_t i;

    if (buffer == NULL || length == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, length, "capture=none");
        return;
    }

    offset += (size_t)snprintf(buffer + offset,
                               length - offset,
                               "captures=%u author=%s",
                               (unsigned int)sys->capture_count,
                               MW_AUTHOR);

    for (i = 0u; i < sys->capture_count && i < 6u && offset < length; ++i) {
        const mw_capture_frame_t *frame = &sys->captures[i];
        offset += (size_t)snprintf(buffer + offset,
                                   length - offset,
                                   " | %u phy%u reg0x%02X=0x%04X %s",
                                   (unsigned int)frame->timestamp_ms,
                                   (unsigned int)frame->phy_addr,
                                   (unsigned int)frame->reg,
                                   (unsigned int)frame->value,
                                   frame->note);
    }

    if (offset >= length) {
        buffer[length - 1u] = '\0';
    }
}
