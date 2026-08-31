/*
 * drivers/profile.c - MDIO Wraith scenario profiles
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "profile.h"

#include <stdio.h>
#include <string.h>

#include "../registers.h"

static const mw_profile_t g_profiles[] = {
    {
        .name = "phy-fingerprint",
        .allow_clause45 = 1,
        .allow_strap_swap = 0,
        .allow_isolate_write = 0,
        .allow_loopback_write = 0,
        .require_arming = 0,
        .write_budget = 0,
        .settle_time_ms = 20,
        .trigger_phy = MW_TRIGGER_PHY_ANY,
        .trigger_reg = MW_TRIGGER_REG_ANY,
        .trigger_value_mask = 0x0000u,
        .trigger_value_expected = 0x0000u,
    },
    {
        .name = "isolated-link-drop",
        .allow_clause45 = 0,
        .allow_strap_swap = 0,
        .allow_isolate_write = 1,
        .allow_loopback_write = 0,
        .require_arming = 1,
        .write_budget = 2,
        .settle_time_ms = 120,
        .trigger_phy = 0u,
        .trigger_reg = MW_REG_BMSR,
        .trigger_value_mask = MW_BMSR_LINK_STATUS,
        .trigger_value_expected = MW_BMSR_LINK_STATUS,
    },
    {
        .name = "loopback-diversion",
        .allow_clause45 = 1,
        .allow_strap_swap = 0,
        .allow_isolate_write = 0,
        .allow_loopback_write = 1,
        .require_arming = 1,
        .write_budget = 3,
        .settle_time_ms = 60,
        .trigger_phy = 1u,
        .trigger_reg = MW_REG_ANER,
        .trigger_value_mask = 0x0001u,
        .trigger_value_expected = 0x0001u,
    },
    {
        .name = "strap-shadow",
        .allow_clause45 = 1,
        .allow_strap_swap = 1,
        .allow_isolate_write = 0,
        .allow_loopback_write = 0,
        .require_arming = 1,
        .write_budget = 1,
        .settle_time_ms = 200,
        .trigger_phy = 2u,
        .trigger_reg = MW_REG_VENDOR_PAGE,
        .trigger_value_mask = 0x00FFu,
        .trigger_value_expected = 0x0004u,
    },
};

size_t mw_profile_count(void)
{
    return sizeof(g_profiles) / sizeof(g_profiles[0]);
}

const mw_profile_t *mw_profile_get(size_t index)
{
    if (index >= mw_profile_count()) {
        return NULL;
    }
    return &g_profiles[index];
}

const mw_profile_t *mw_profile_find(const char *name)
{
    size_t i;

    if (name == NULL) {
        return NULL;
    }

    for (i = 0; i < mw_profile_count(); ++i) {
        if (strcmp(g_profiles[i].name, name) == 0) {
            return &g_profiles[i];
        }
    }

    return NULL;
}

void mw_profile_describe(const mw_profile_t *profile, char *buffer, size_t length)
{
    const char *arming;
    const char *c45;
    const char *strap;

    if (buffer == NULL || length == 0u) {
        return;
    }

    if (profile == NULL) {
        (void)snprintf(buffer, length, "profile=none");
        return;
    }

    arming = profile->require_arming ? "armed" : "passive-ok";
    c45 = profile->allow_clause45 ? "cl45" : "cl22";
    strap = profile->allow_strap_swap ? "strap-swap" : "no-strap-swap";

    (void)snprintf(
        buffer,
        length,
        "profile=%s mode=%s bus=%s writes=%u settle=%ums %s trigger=phy:%u reg:0x%02X mask:0x%04X expect:0x%04X",
        profile->name,
        arming,
        c45,
        profile->write_budget,
        profile->settle_time_ms,
        strap,
        (unsigned int)profile->trigger_phy,
        (unsigned int)profile->trigger_reg,
        (unsigned int)profile->trigger_value_mask,
        (unsigned int)profile->trigger_value_expected);
}
