/*
 * i3c_bus.c - I3C Poltergeist bus simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "i3c_bus.h"
#include "../registers.h"

static void ip_i3c_capture(ip_system_t *sys,
                           uint8_t target_index,
                           uint8_t address,
                           uint16_t ccc,
                           uint16_t reg,
                           uint16_t value,
                           uint8_t is_write,
                           uint8_t is_i2c_compat,
                           const char *note)
{
    size_t slot;

    if (sys == NULL || sys->capture_count >= IP_MAX_CAPTURES) {
        return;
    }

    slot = sys->capture_count++;
    sys->captures[slot].timestamp_ms = sys->tick_ms;
    sys->captures[slot].target_index = target_index;
    sys->captures[slot].address = address;
    sys->captures[slot].ccc = ccc;
    sys->captures[slot].reg = reg;
    sys->captures[slot].value = value;
    sys->captures[slot].is_write = is_write;
    sys->captures[slot].is_i2c_compat = is_i2c_compat;
    (void)snprintf(sys->captures[slot].note,
                   sizeof(sys->captures[slot].note),
                   "%s",
                   note != NULL ? note : "capture");
    sys->status.captures_taken = (uint32_t)sys->capture_count;
}

static void ip_i3c_touch_target(ip_system_t *sys, uint8_t target_index, uint16_t reg, uint16_t value)
{
    if (sys == NULL || target_index >= sys->target_count) {
        return;
    }

    sys->targets[target_index].last_reg = reg;
    sys->targets[target_index].last_value = value;
}

const char *ip_i3c_ccc_name(uint16_t ccc)
{
    switch (ccc) {
    case IP_CCC_ENTDAA:
        return "ENTDAA";
    case IP_CCC_RSTDAA:
        return "RSTDAA";
    case IP_CCC_ENEC:
        return "ENEC";
    case IP_CCC_DISEC:
        return "DISEC";
    case IP_CCC_SETMWL:
        return "SETMWL";
    case IP_CCC_GETMWL:
        return "GETMWL";
    case IP_CCC_SETMRL:
        return "SETMRL";
    case IP_CCC_GETMRL:
        return "GETMRL";
    case IP_CCC_GETPID:
        return "GETPID";
    case IP_CCC_GETBCR:
        return "GETBCR";
    case IP_CCC_GETDCR:
        return "GETDCR";
    case IP_CCC_SETDASA:
        return "SETDASA";
    case IP_CCC_SETAASA:
        return "SETAASA";
    case IP_CCC_DEFSLVS:
        return "DEFSLVS";
    default:
        return "CCC-UNKNOWN";
    }
}

void ip_i3c_init_targets(ip_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    sys->target_count = 5u;
    (void)memset(sys->targets, 0, sizeof(sys->targets));

    (void)snprintf(sys->targets[0].name, sizeof(sys->targets[0].name), "%s", "imu-array");
    sys->targets[0].target_class = IP_TARGET_SENSOR;
    sys->targets[0].static_address = 0x68u;
    sys->targets[0].dynamic_address = 0x12u;
    sys->targets[0].pid_hi = 0x61u;
    sys->targets[0].pid_lo = 0x1201u;
    sys->targets[0].supports_i3c = 1u;
    sys->targets[0].supports_legacy_i2c = 1u;
    sys->targets[0].accepts_ibi = 1u;
    sys->targets[0].allows_hotjoin = 1u;
    sys->targets[0].awake = 1u;

    (void)snprintf(sys->targets[1].name, sizeof(sys->targets[1].name), "%s", "touch-bridge");
    sys->targets[1].target_class = IP_TARGET_TOUCH;
    sys->targets[1].static_address = 0x2Au;
    sys->targets[1].dynamic_address = 0x18u;
    sys->targets[1].pid_hi = 0x71u;
    sys->targets[1].pid_lo = 0x2302u;
    sys->targets[1].supports_i3c = 1u;
    sys->targets[1].supports_legacy_i2c = 1u;
    sys->targets[1].accepts_ibi = 1u;
    sys->targets[1].allows_hotjoin = 0u;
    sys->targets[1].awake = 1u;

    (void)snprintf(sys->targets[2].name, sizeof(sys->targets[2].name), "%s", "pmic-shadow");
    sys->targets[2].target_class = IP_TARGET_PMIC;
    sys->targets[2].static_address = 0x30u;
    sys->targets[2].dynamic_address = 0x1Cu;
    sys->targets[2].pid_hi = 0x7Au;
    sys->targets[2].pid_lo = 0x4104u;
    sys->targets[2].supports_i3c = 1u;
    sys->targets[2].supports_legacy_i2c = 0u;
    sys->targets[2].accepts_ibi = 0u;
    sys->targets[2].allows_hotjoin = 0u;
    sys->targets[2].awake = 1u;

    (void)snprintf(sys->targets[3].name, sizeof(sys->targets[3].name), "%s", "ec-proxy");
    sys->targets[3].target_class = IP_TARGET_EC;
    sys->targets[3].static_address = 0x42u;
    sys->targets[3].dynamic_address = 0x22u;
    sys->targets[3].pid_hi = 0x55u;
    sys->targets[3].pid_lo = 0x4401u;
    sys->targets[3].supports_i3c = 1u;
    sys->targets[3].supports_legacy_i2c = 1u;
    sys->targets[3].accepts_ibi = 0u;
    sys->targets[3].allows_hotjoin = 0u;
    sys->targets[3].secure_role = 1u;
    sys->targets[3].awake = 1u;

    (void)snprintf(sys->targets[4].name, sizeof(sys->targets[4].name), "%s", "secure-haptics");
    sys->targets[4].target_class = IP_TARGET_HAPTIC;
    sys->targets[4].static_address = 0x5Au;
    sys->targets[4].dynamic_address = 0x29u;
    sys->targets[4].pid_hi = 0x63u;
    sys->targets[4].pid_lo = 0x5509u;
    sys->targets[4].supports_i3c = 1u;
    sys->targets[4].supports_legacy_i2c = 1u;
    sys->targets[4].accepts_ibi = 1u;
    sys->targets[4].allows_hotjoin = 1u;
    sys->targets[4].secure_role = 1u;
    sys->targets[4].awake = 1u;

    sys->status.target_present = 1u;
}

void ip_i3c_run_inventory(ip_system_t *sys)
{
    size_t i;

    if (sys == NULL) {
        return;
    }

    for (i = 0u; i < sys->target_count; ++i) {
        char note[IP_MAX_CAPTURE_NOTE];
        sys->tick_ms += IP_TICK_MS;
        (void)snprintf(note, sizeof(note), "inventory:%s", sys->targets[i].name);
        ip_i3c_capture(sys,
                       (uint8_t)i,
                       sys->targets[i].dynamic_address,
                       IP_CCC_GETPID,
                       IP_REG_DEVICE_ID,
                       (uint16_t)((sys->targets[i].pid_hi << 8) ^ (sys->targets[i].pid_lo & 0xFFu)),
                       0u,
                       0u,
                       note);
    }
}

void ip_i3c_capture_boot_sequence(ip_system_t *sys)
{
    size_t i;

    if (sys == NULL) {
        return;
    }

    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys, IP_TARGET_ANY, 0x7Eu, IP_CCC_RSTDAA, 0u, 0u, 1u, 0u, "broadcast-rstdaa");
    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys, IP_TARGET_ANY, 0x7Eu, IP_CCC_ENTDAA, 0u, 0u, 1u, 0u, "broadcast-entdaa");

    for (i = 0u; i < sys->target_count; ++i) {
        char note[IP_MAX_CAPTURE_NOTE];
        sys->tick_ms += IP_TICK_MS;
        (void)snprintf(note, sizeof(note), "daa:%s", sys->targets[i].name);
        ip_i3c_capture(sys,
                       (uint8_t)i,
                       sys->targets[i].dynamic_address,
                       IP_CCC_SETDASA,
                       IP_REG_DEVICE_ID,
                       (uint16_t)(((uint16_t)sys->targets[i].static_address << 8) | sys->targets[i].dynamic_address),
                       1u,
                       0u,
                       note);
    }

    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys, IP_TARGET_ANY, 0x7Eu, IP_CCC_ENEC, IP_REG_INT_MASK, 0x0001u, 1u, 0u, "enable-events");
}

uint16_t ip_i3c_legacy_read(ip_system_t *sys, uint8_t target_index, uint16_t reg)
{
    ip_target_t *target;
    uint16_t value;
    const char *note;

    if (sys == NULL || target_index >= sys->target_count) {
        return 0u;
    }

    target = &sys->targets[target_index];
    value = 0u;
    note = "legacy-read-blocked";

    if (target->supports_legacy_i2c) {
        if (reg == IP_REG_DEVICE_ID) {
            value = (uint16_t)(((uint16_t)target->pid_hi << 8) | (target->pid_lo & 0xFFu));
        } else if (reg == IP_REG_STATUS) {
            value = (uint16_t)((target->supports_i3c ? 0x0001u : 0u) |
                               (target->accepts_ibi ? 0x0002u : 0u) |
                               (target->allows_hotjoin ? 0x0004u : 0u) |
                               (target->awake ? 0x0010u : 0u));
        } else if (reg == IP_REG_FW_REV) {
            value = (uint16_t)(0x0100u + target_index);
        } else if (reg == IP_REG_SENSOR_VECTOR) {
            value = (uint16_t)(0x2000u + (target_index * 0x31u));
        } else {
            value = (uint16_t)(target->last_value ^ reg ^ (uint16_t)(target->dynamic_address << 4));
        }
        note = "legacy-read";
    } else {
        sys->status.anomalies++;
    }

    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys,
                   target_index,
                   target->static_address,
                   0u,
                   reg,
                   value,
                   0u,
                   1u,
                   note);
    ip_i3c_touch_target(sys, target_index, reg, value);
    return value;
}

int ip_i3c_legacy_write(ip_system_t *sys, const ip_profile_t *profile, uint8_t target_index, uint16_t reg, uint16_t value)
{
    ip_target_t *target;

    if (sys == NULL || profile == NULL || target_index >= sys->target_count) {
        return 0;
    }

    target = &sys->targets[target_index];
    if (!target->supports_legacy_i2c || !profile->allow_downgrade_probe) {
        sys->status.anomalies++;
        sys->tick_ms += IP_TICK_MS;
        ip_i3c_capture(sys,
                       target_index,
                       target->static_address,
                       0u,
                       reg,
                       value,
                       1u,
                       1u,
                       "legacy-write-blocked");
        return 0;
    }

    ip_i3c_touch_target(sys, target_index, reg, value);
    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys,
                   target_index,
                   target->static_address,
                   0u,
                   reg,
                   value,
                   1u,
                   1u,
                   "legacy-write");
    return 1;
}

int ip_i3c_inject_hotjoin(ip_system_t *sys, const ip_profile_t *profile, uint8_t target_index)
{
    if (sys == NULL || profile == NULL || target_index >= sys->target_count) {
        return 0;
    }

    if (!profile->allow_hotjoin_inject || !sys->targets[target_index].allows_hotjoin) {
        sys->status.anomalies++;
        sys->tick_ms += IP_TICK_MS;
        ip_i3c_capture(sys,
                       target_index,
                       sys->targets[target_index].dynamic_address,
                       0u,
                       IP_REG_STATUS,
                       0u,
                       1u,
                       0u,
                       "hotjoin-blocked");
        return 0;
    }

    sys->status.hotjoin_seen = 1u;
    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys,
                   target_index,
                   sys->targets[target_index].dynamic_address,
                   0u,
                   IP_REG_STATUS,
                   0x00A5u,
                   1u,
                   0u,
                   "synthetic-hotjoin");
    return 1;
}

int ip_i3c_replay_ibi(ip_system_t *sys, const ip_profile_t *profile, uint8_t target_index, uint16_t vector)
{
    if (sys == NULL || profile == NULL || target_index >= sys->target_count) {
        return 0;
    }

    if (!profile->allow_ibi_replay || !sys->targets[target_index].accepts_ibi) {
        sys->status.anomalies++;
        sys->tick_ms += IP_TICK_MS;
        ip_i3c_capture(sys,
                       target_index,
                       sys->targets[target_index].dynamic_address,
                       0u,
                       IP_REG_INT_STATUS,
                       vector,
                       1u,
                       0u,
                       "ibi-blocked");
        return 0;
    }

    sys->status.ibi_seen = 1u;
    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys,
                   target_index,
                   sys->targets[target_index].dynamic_address,
                   0u,
                   IP_REG_INT_STATUS,
                   vector,
                   1u,
                   0u,
                   "ibi-replay");
    return 1;
}

int ip_i3c_suppress_ccc(ip_system_t *sys, const ip_profile_t *profile, uint16_t ccc)
{
    if (sys == NULL || profile == NULL) {
        return 0;
    }

    if (!profile->allow_ccc_suppression) {
        sys->status.anomalies++;
        sys->tick_ms += IP_TICK_MS;
        ip_i3c_capture(sys, IP_TARGET_ANY, 0x7Eu, ccc, 0u, 0u, 1u, 0u, "ccc-suppress-blocked");
        return 0;
    }

    sys->tick_ms += IP_TICK_MS;
    ip_i3c_capture(sys, IP_TARGET_ANY, 0x7Eu, ccc, 0u, 0u, 1u, 0u, "ccc-suppressed");
    return 1;
}

void ip_i3c_describe_capture(const ip_system_t *sys, char *buffer, size_t buffer_size)
{
    const ip_capture_t *last;
    const char *target_name;

    if (buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (sys == NULL || sys->capture_count == 0u) {
        (void)snprintf(buffer, buffer_size, "captures=0");
        return;
    }

    last = &sys->captures[sys->capture_count - 1u];
    target_name = (last->target_index < sys->target_count) ? sys->targets[last->target_index].name : "broadcast";

    if (last->ccc != 0u) {
        (void)snprintf(buffer,
                       buffer_size,
                       "captures=%u last=%s ccc=%s addr=0x%02X note=%s",
                       (unsigned int)sys->capture_count,
                       target_name,
                       ip_i3c_ccc_name(last->ccc),
                       (unsigned int)last->address,
                       last->note);
    } else {
        (void)snprintf(buffer,
                       buffer_size,
                       "captures=%u last=%s reg=0x%04X value=0x%04X compat=%u note=%s",
                       (unsigned int)sys->capture_count,
                       target_name,
                       (unsigned int)last->reg,
                       (unsigned int)last->value,
                       (unsigned int)last->is_i2c_compat,
                       last->note);
    }
}

void ip_i3c_print_inventory(const ip_system_t *sys)
{
    size_t i;

    if (sys == NULL) {
        return;
    }

    printf("[inventory]\n");
    for (i = 0u; i < sys->target_count; ++i) {
        const ip_target_t *target = &sys->targets[i];
        printf("  idx=%u name=%s static=0x%02X dynamic=0x%02X i3c=%u legacy=%u ibi=%u hotjoin=%u secure=%u\n",
               (unsigned int)i,
               target->name,
               (unsigned int)target->static_address,
               (unsigned int)target->dynamic_address,
               (unsigned int)target->supports_i3c,
               (unsigned int)target->supports_legacy_i2c,
               (unsigned int)target->accepts_ibi,
               (unsigned int)target->allows_hotjoin,
               (unsigned int)target->secure_role);
    }
    printf("\n");
}
