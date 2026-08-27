/*
 * lldp.c - LLDP / LLDP-MED helpers for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include "lldp.h"

static size_t write_tlv(uint8_t type, const uint8_t *payload, size_t payload_len, uint8_t *out, size_t offset, size_t out_len)
{
    if (offset + payload_len + 2 > out_len || payload_len > 511u) {
        return offset;
    }
    uint16_t header = (uint16_t)(((uint16_t)type << 9) | (payload_len & 0x1FFu));
    out[offset++] = (uint8_t)(header >> 8);
    out[offset++] = (uint8_t)(header & 0xFFu);
    memcpy(&out[offset], payload, payload_len);
    return offset + payload_len;
}

pw_lldp_profile_t pw_lldp_default_profile(const char *system_name, float requested_power_w)
{
    pw_lldp_profile_t p;
    memset(&p, 0, sizeof(p));
    snprintf(p.chassis_id, sizeof(p.chassis_id), "pw-inline-01");
    snprintf(p.port_id, sizeof(p.port_id), "eth-inline-a");
    snprintf(p.system_name, sizeof(p.system_name), "%s", system_name ? system_name : "PoE Whisper");
    snprintf(p.platform, sizeof(p.platform), "inline-poe-research");
    p.requested_power_w = requested_power_w;
    p.priority = 2;
    return p;
}

size_t pw_lldp_build_advertisement(const pw_lldp_profile_t *profile, uint8_t *out, size_t out_len)
{
    size_t off = 0;
    off = write_tlv(1, (const uint8_t *)profile->chassis_id, strlen(profile->chassis_id), out, off, out_len);
    off = write_tlv(2, (const uint8_t *)profile->port_id, strlen(profile->port_id), out, off, out_len);
    off = write_tlv(5, (const uint8_t *)profile->system_name, strlen(profile->system_name), out, off, out_len);
    off = write_tlv(6, (const uint8_t *)profile->platform, strlen(profile->platform), out, off, out_len);

    char power[48];
    snprintf(power, sizeof(power), "power=%.1fW priority=%u", profile->requested_power_w, profile->priority);
    off = write_tlv(127, (const uint8_t *)power, strlen(power), out, off, out_len);
    out[off++] = 0;
    out[off++] = 0;
    return off;
}

int pw_lldp_parse_summary(const uint8_t *buf, size_t len, pw_lldp_profile_t *out)
{
    size_t off = 0;
    memset(out, 0, sizeof(*out));
    while (off + 2 <= len) {
        uint16_t header = (uint16_t)((buf[off] << 8) | buf[off + 1]);
        uint8_t type = (uint8_t)(header >> 9);
        uint16_t tlv_len = header & 0x1FFu;
        off += 2;
        if (off + tlv_len > len) {
            return -1;
        }
        if (type == 0) {
            break;
        } else if (type == 1) {
            snprintf(out->chassis_id, sizeof(out->chassis_id), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 2) {
            snprintf(out->port_id, sizeof(out->port_id), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 5) {
            snprintf(out->system_name, sizeof(out->system_name), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 6) {
            snprintf(out->platform, sizeof(out->platform), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 127) {
            sscanf((const char *)&buf[off], "power=%fW priority=%hhu", &out->requested_power_w, &out->priority);
        }
        off += tlv_len;
    }
    return 0;
}

void pw_lldp_format_summary(const pw_lldp_profile_t *profile, char *out, size_t out_len)
{
    snprintf(out, out_len, "LLDP chassis=%s port=%s system=%s power=%.1fW prio=%u",
             profile->chassis_id,
             profile->port_id,
             profile->system_name,
             profile->requested_power_w,
             profile->priority);
}
