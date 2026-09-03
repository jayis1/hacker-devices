/*
 * SmartPack Phantom battery profiles
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "battery_profile.h"
#include "../registers.h"

static void fill_strings(sp_battery_profile_t *profile,
                         const char *manufacturer,
                         const char *device_name,
                         const char *chemistry) {
    snprintf(profile->manufacturer, sizeof(profile->manufacturer), "%.31s", manufacturer);
    snprintf(profile->device_name, sizeof(profile->device_name), "%.31s", device_name);
    snprintf(profile->chemistry, sizeof(profile->chemistry), "%.31s", chemistry);
}

void battery_profile_init(sp_battery_profile_t *profile, sp_profile_kind_t kind) {
    if (profile == 0) {
        return;
    }
    memset(profile, 0, sizeof(*profile));
    profile->kind = kind;
    switch (kind) {
        case SP_PROFILE_ENTERPRISE_LAPTOP:
            fill_strings(profile, "Aster Dynamics", "AD-LT92", "Li-Ion");
            profile->design_voltage_mv = 11400u;
            profile->full_charge_capacity_mah = 5400u;
            profile->remaining_capacity_mah = 4300u;
            profile->current_ma = -1450;
            profile->avg_current_ma = -1200;
            profile->temperature_c = 31;
            profile->cycle_count = 126u;
            profile->serial = 0x4912u;
            profile->status_word = 0x0080u;
            profile->charge_limit_ma = 3200u;
            break;
        case SP_PROFILE_RUGGED_TABLET:
            fill_strings(profile, "Granite Mobility", "GM-RG8", "LiFePO4");
            profile->design_voltage_mv = 7600u;
            profile->full_charge_capacity_mah = 6800u;
            profile->remaining_capacity_mah = 5200u;
            profile->current_ma = -920;
            profile->avg_current_ma = -810;
            profile->temperature_c = 29;
            profile->cycle_count = 44u;
            profile->serial = 0x7811u;
            profile->status_word = 0x0040u;
            profile->charge_limit_ma = 2100u;
            break;
        case SP_PROFILE_DRONE_PACK:
            fill_strings(profile, "SkyGrid Labs", "SG-6S4200", "LiPo");
            profile->design_voltage_mv = 22200u;
            profile->full_charge_capacity_mah = 4200u;
            profile->remaining_capacity_mah = 2100u;
            profile->current_ma = -3600;
            profile->avg_current_ma = -2800;
            profile->temperature_c = 37;
            profile->cycle_count = 87u;
            profile->serial = 0x2620u;
            profile->status_word = 0x0100u;
            profile->charge_limit_ma = 4500u;
            break;
        case SP_PROFILE_SERVICE_PACK:
            fill_strings(profile, "Aster Dynamics", "AD-SVC-ENG", "Li-Ion");
            profile->design_voltage_mv = 11400u;
            profile->full_charge_capacity_mah = 6000u;
            profile->remaining_capacity_mah = 5900u;
            profile->current_ma = -400;
            profile->avg_current_ma = -350;
            profile->temperature_c = 28;
            profile->cycle_count = 3u;
            profile->serial = 0x9001u;
            profile->status_word = 0x0000u;
            profile->charge_limit_ma = 3400u;
            profile->maintenance_flag = true;
            break;
        case SP_PROFILE_COUNTERFEIT_CLONE:
        default:
            fill_strings(profile, "Aster Dynamic", "AD-LT92X", "Li-Ionn");
            profile->design_voltage_mv = 11220u;
            profile->full_charge_capacity_mah = 3900u;
            profile->remaining_capacity_mah = 2700u;
            profile->current_ma = -1500;
            profile->avg_current_ma = -1500;
            profile->temperature_c = 41;
            profile->cycle_count = 1u;
            profile->serial = 0x1111u;
            profile->status_word = 0x0400u;
            profile->charge_limit_ma = 900u;
            break;
    }
}

const char *battery_profile_name(sp_profile_kind_t kind) {
    switch (kind) {
        case SP_PROFILE_ENTERPRISE_LAPTOP: return "enterprise-laptop";
        case SP_PROFILE_RUGGED_TABLET: return "rugged-tablet";
        case SP_PROFILE_DRONE_PACK: return "drone-pack";
        case SP_PROFILE_SERVICE_PACK: return "service-pack";
        case SP_PROFILE_COUNTERFEIT_CLONE: return "counterfeit-clone";
        default: return "unknown";
    }
}

uint8_t battery_profile_relative_soc(const sp_battery_profile_t *profile) {
    uint32_t pct;
    if (profile == 0 || profile->full_charge_capacity_mah == 0u) {
        return 0u;
    }
    pct = (uint32_t)profile->remaining_capacity_mah * 100u / (uint32_t)profile->full_charge_capacity_mah;
    if (pct > 100u) {
        pct = 100u;
    }
    return (uint8_t)pct;
}

uint16_t battery_profile_runtime_to_empty(const sp_battery_profile_t *profile) {
    uint32_t current;
    if (profile == 0) {
        return 0u;
    }
    current = (uint32_t)(profile->current_ma < 0 ? -profile->current_ma : profile->current_ma);
    if (current < 100u) {
        current = 100u;
    }
    return (uint16_t)(((uint32_t)profile->remaining_capacity_mah * 60u) / current);
}

uint16_t battery_profile_average_time_to_empty(const sp_battery_profile_t *profile) {
    uint32_t current;
    if (profile == 0) {
        return 0u;
    }
    current = (uint32_t)(profile->avg_current_ma < 0 ? -profile->avg_current_ma : profile->avg_current_ma);
    if (current < 100u) {
        current = 100u;
    }
    return (uint16_t)(((uint32_t)profile->remaining_capacity_mah * 60u) / current);
}

uint16_t battery_profile_manufacturer_date(const sp_battery_profile_t *profile) {
    (void)profile;
    return (uint16_t)(((2026u - 1980u) << 9) | (9u << 5) | 3u);
}

uint16_t battery_profile_command_word(const sp_battery_profile_t *profile, uint8_t command) {
    if (profile == 0) {
        return 0u;
    }
    switch (command) {
        case SP_SBS_TEMPERATURE: return (uint16_t)((profile->temperature_c + 273) * 10);
        case SP_SBS_VOLTAGE: return profile->design_voltage_mv;
        case SP_SBS_CURRENT: return (uint16_t)profile->current_ma;
        case SP_SBS_AVG_CURRENT: return (uint16_t)profile->avg_current_ma;
        case SP_SBS_RELATIVE_SOC:
        case SP_SBS_ABSOLUTE_SOC: return battery_profile_relative_soc(profile);
        case SP_SBS_REMAINING_CAP: return profile->remaining_capacity_mah;
        case SP_SBS_FULL_CHARGE_CAP:
        case SP_SBS_DESIGN_CAPACITY: return profile->full_charge_capacity_mah;
        case SP_SBS_RUNTIME_EMPTY: return battery_profile_runtime_to_empty(profile);
        case SP_SBS_AVG_TIME_EMPTY: return battery_profile_average_time_to_empty(profile);
        case SP_SBS_CYCLE_COUNT: return profile->cycle_count;
        case SP_SBS_DESIGN_VOLTAGE: return profile->design_voltage_mv;
        case SP_SBS_MANUFACTURER_DATE: return battery_profile_manufacturer_date(profile);
        case SP_SBS_SERIAL_NUMBER: return profile->serial;
        case SP_SBS_STATUS: return profile->status_word;
        case SP_VENDOR_SERVICE_FLAGS:
            return (uint16_t)((profile->maintenance_flag ? 0x0001u : 0u) |
                              (profile->shipping_mode ? 0x0002u : 0u) |
                              (profile->permanent_failure ? 0x0004u : 0u));
        case SP_VENDOR_CHARGE_POLICY: return profile->charge_limit_ma;
        default: return 0u;
    }
}

size_t battery_profile_command_block(const sp_battery_profile_t *profile, uint8_t command, uint8_t *out, size_t max_len) {
    const char *src = "";
    size_t len;
    if (profile == 0 || out == 0 || max_len == 0u) {
        return 0u;
    }
    switch (command) {
        case SP_SBS_MANUFACTURER_NAME: src = profile->manufacturer; break;
        case SP_SBS_DEVICE_NAME: src = profile->device_name; break;
        case SP_SBS_DEVICE_CHEMISTRY: src = profile->chemistry; break;
        case SP_SBS_MANUFACTURER_DATA:
            src = profile->maintenance_flag ? "svc=1;auth=ok;trace=signed" : "svc=0;auth=req;trace=enabled";
            break;
        default:
            src = "";
            break;
    }
    len = strlen(src);
    if (len > max_len) {
        len = max_len;
    }
    memcpy(out, src, len);
    return len;
}

void battery_profile_apply_status(sp_battery_profile_t *profile, uint16_t status_word) {
    if (profile == 0) {
        return;
    }
    profile->status_word = status_word;
    profile->shipping_mode = (status_word & 0x0200u) != 0u;
    profile->permanent_failure = (status_word & 0x0400u) != 0u;
}

void battery_profile_set_soc_percent(sp_battery_profile_t *profile, uint8_t soc_percent) {
    if (profile == 0) {
        return;
    }
    if (soc_percent > 100u) {
        soc_percent = 100u;
    }
    profile->remaining_capacity_mah = (uint16_t)(((uint32_t)profile->full_charge_capacity_mah * soc_percent) / 100u);
}

void battery_profile_set_temperature(sp_battery_profile_t *profile, int16_t temp_c) {
    if (profile == 0) {
        return;
    }
    profile->temperature_c = temp_c;
}

void battery_profile_set_charge_limit(sp_battery_profile_t *profile, uint16_t ma) {
    if (profile == 0) {
        return;
    }
    profile->charge_limit_ma = ma;
}

void battery_profile_apply_named_overlay(sp_battery_profile_t *profile, const char *name) {
    if (profile == 0 || name == 0) {
        return;
    }
    if (strcmp(name, "maintenance-mask") == 0) {
        fill_strings(profile, "Aster Dynamics", "AD-SVC-ENG", "Li-Ion");
        profile->maintenance_flag = true;
        profile->serial = 0x9001u;
    } else if (strcmp(name, "low-soc-bait") == 0) {
        battery_profile_set_soc_percent(profile, 2u);
        profile->current_ma = -250;
        profile->avg_current_ma = -180;
    } else if (strcmp(name, "shipping-confusion") == 0) {
        profile->shipping_mode = true;
        profile->status_word |= 0x0200u;
    } else if (strcmp(name, "thermal-trip-spoof") == 0) {
        profile->temperature_c = 78;
        profile->status_word |= 0x2000u;
    } else if (strcmp(name, "charger-limit-swing") == 0) {
        profile->charge_limit_ma = 500u;
        profile->status_word |= 0x0800u;
    }
}
