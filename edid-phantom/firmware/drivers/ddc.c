/*
 * ddc.c - EDID Phantom DDC/EDID subsystem
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "ddc.h"
#include "log.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static ep_profile_t g_profile;
static ep_edid_image_t g_edid;
static ep_ddc_txn_t g_transactions[EP_MAX_DDC_TRANSACTIONS];
static size_t g_txn_count;

static uint8_t checksum_block(const uint8_t *block) {
    uint32_t sum = 0;
    size_t index;
    for (index = 0; index < 128u; ++index) {
        sum += block[index];
    }
    return (uint8_t)(sum & 0xFFu);
}

static void fill_base_edid(void) {
    static const uint8_t base_template[128] = {
        0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x2D,0x44,0x50,0x48,0x23,0x10,0x00,0x00,
        0x22,0x20,0x01,0x03,0x80,0x34,0x1D,0x78,0x2A,0xCF,0x95,0xA3,0x57,0x4C,0x9C,0x25,
        0x12,0x50,0x54,0x21,0x08,0x00,0x81,0x80,0xA9,0x40,0xB3,0x00,0xD1,0xC0,0x01,0x01,
        0x01,0x01,0x01,0x01,0x56,0x5E,0x00,0xA0,0xA0,0xA0,0x29,0x50,0x30,0x20,0x35,0x00,
        0x55,0x50,0x21,0x00,0x00,0x1A,0x00,0x00,0x00,0xFC,0x00,0x50,0x68,0x61,0x6E,0x74,
        0x6F,0x6D,0x20,0x52,0x65,0x73,0x65,0x61,0x72,0x63,0x68,0x00,0x00,0x00,0xFD,0x00,
        0x18,0x4B,0x1E,0x87,0x3C,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x01,0x5A,0x02,
        0x03,0x1A,0x71,0x47,0x90,0x04,0x03,0x02,0x07,0x16,0x01,0x23,0x09,0x07,0x07,0x00
    };
    memcpy(g_edid.bytes, base_template, sizeof(base_template));
    g_edid.length = sizeof(base_template);
    g_edid.extension_count = 0;
    snprintf(g_edid.monitor_name, sizeof(g_edid.monitor_name), "%s", "PhantomRsch");
    g_edid.bytes[127] = (uint8_t)((256u - checksum_block(g_edid.bytes)) & 0xFFu);
    g_edid.checksum_valid = checksum_block(g_edid.bytes) == 0u;
}

static void apply_profile_mutation(uint16_t seed) {
    uint8_t audio_hint = (uint8_t)(g_profile.preferred_audio_channels & 0x0Fu);
    uint8_t luminance = g_profile.max_luminance_hint;
    uint8_t vendor_mask = g_profile.preserve_vendor_block ? 0xA0u : 0x0Fu;

    g_edid.bytes[20] = (uint8_t)(0x80u | (seed & 0x1Fu));
    g_edid.bytes[23] = (uint8_t)(0x60u + (seed % 20u));
    g_edid.bytes[24] = (uint8_t)(0x20u + ((seed >> 3u) % 30u));
    g_edid.bytes[54] = (uint8_t)(0x40u + (seed % 40u));
    g_edid.bytes[55] = (uint8_t)(0x50u + ((seed >> 1u) % 30u));
    g_edid.bytes[68] = audio_hint;
    g_edid.bytes[69] = luminance;
    g_edid.bytes[70] = vendor_mask;
    g_edid.bytes[95] = (uint8_t)(0x18u + (seed % 10u));
    g_edid.bytes[96] = (uint8_t)(0x40u + (seed % 15u));
    g_edid.bytes[127] = 0u;
    g_edid.bytes[127] = (uint8_t)((256u - checksum_block(g_edid.bytes)) & 0xFFu);
    g_edid.checksum_valid = checksum_block(g_edid.bytes) == 0u;
}

static void push_transaction(uint32_t now_ms, uint8_t address, uint8_t offset, uint8_t length) {
    ep_ddc_txn_t *txn;
    size_t index = g_txn_count;
    size_t i;
    if (index >= EP_MAX_DDC_TRANSACTIONS) {
        memmove(&g_transactions[0], &g_transactions[1], sizeof(g_transactions[0]) * (EP_MAX_DDC_TRANSACTIONS - 1u));
        index = EP_MAX_DDC_TRANSACTIONS - 1u;
    } else {
        g_txn_count++;
    }

    txn = &g_transactions[index];
    memset(txn, 0, sizeof(*txn));
    txn->start_ms = now_ms;
    txn->stop_ms = now_ms + 2u;
    txn->address = address;
    txn->offset = offset;
    txn->length = length;
    txn->acked = 1u;
    for (i = 0; i < length && i < sizeof(txn->data); ++i) {
        txn->data[i] = g_edid.bytes[offset + i];
        txn->checksum = (uint8_t)(txn->checksum + txn->data[i]);
    }

    reg_set_bits(REG_DDC_STATUS, DDC_STATUS_ACTIVITY);
    reg_write(REG_DDC_LAST_ADDR, address);
    reg_write(REG_DDC_LAST_LENGTH, length);
}

void ddc_init(void) {
    memset(&g_profile, 0, sizeof(g_profile));
    memset(g_transactions, 0, sizeof(g_transactions));
    g_txn_count = 0;
    fill_base_edid();
    log_event(EP_EVENT_DDC_CAPTURE, EP_RISK_INFO, "ddc subsystem initialized, checksum=%u", g_edid.checksum_valid);
}

void ddc_set_profile(const ep_profile_t *profile) {
    if (profile == NULL) {
        return;
    }

    g_profile = *profile;
    fill_base_edid();
    if (profile->allow_edid_mutation) {
        apply_profile_mutation(profile->mutate_seed);
        reg_set_bits(REG_SYS_STATUS, SYS_STATUS_MUTATION_ENABLED);
        log_event(EP_EVENT_EDID_MUTATION, EP_RISK_LOW,
                  "profile mutation applied seed=0x%04X audio=%u luminance=%u",
                  profile->mutate_seed,
                  (unsigned)profile->preferred_audio_channels,
                  (unsigned)profile->max_luminance_hint);
    } else {
        reg_clear_bits(REG_SYS_STATUS, SYS_STATUS_MUTATION_ENABLED);
        log_event(EP_EVENT_EDID_MUTATION, EP_RISK_INFO, "profile disabled EDID mutation");
    }

    switch (profile->ddc_mode) {
        case EP_DDC_SNIFF:
            reg_write(REG_DDC_CONTROL, EP_DDC_SNIFF);
            reg_clear_bits(REG_DDC_STATUS, DDC_STATUS_PROXY_ENABLED | DDC_STATUS_EMULATION_ENABLED);
            break;
        case EP_DDC_PROXY:
            reg_write(REG_DDC_CONTROL, EP_DDC_PROXY);
            reg_set_bits(REG_DDC_STATUS, DDC_STATUS_PROXY_ENABLED);
            reg_clear_bits(REG_DDC_STATUS, DDC_STATUS_EMULATION_ENABLED);
            break;
        case EP_DDC_EMULATE:
            reg_write(REG_DDC_CONTROL, EP_DDC_EMULATE);
            reg_set_bits(REG_DDC_STATUS, DDC_STATUS_EMULATION_ENABLED);
            reg_clear_bits(REG_DDC_STATUS, DDC_STATUS_PROXY_ENABLED);
            break;
        case EP_DDC_IDLE:
        default:
            reg_write(REG_DDC_CONTROL, EP_DDC_IDLE);
            reg_clear_bits(REG_DDC_STATUS, DDC_STATUS_PROXY_ENABLED | DDC_STATUS_EMULATION_ENABLED);
            break;
    }
}

void ddc_capture_cycle(uint32_t now_ms, ep_status_t *status) {
    uint8_t offset = (uint8_t)((now_ms / 10u) % 96u);
    uint8_t length = (uint8_t)(8u + ((now_ms / 40u) % 8u));
    if (status == NULL) {
        return;
    }

    push_transaction(now_ms, 0x50u, offset, length);
    status->ddc_captures++;

    if ((status->ddc_captures % 3u) == 0u && g_profile.allow_edid_mutation) {
        apply_profile_mutation((uint16_t)(g_profile.mutate_seed + status->ddc_captures));
        log_event(EP_EVENT_EDID_MUTATION, EP_RISK_MEDIUM,
                  "adaptive mutation on capture=%u checksum_ok=%u",
                  (unsigned)status->ddc_captures,
                  g_edid.checksum_valid);
    }

    log_event(EP_EVENT_DDC_CAPTURE, EP_RISK_INFO,
              "txn=%u addr=0x%02X offset=%u len=%u mode=%u",
              (unsigned)status->ddc_captures,
              0x50u,
              (unsigned)offset,
              (unsigned)length,
              (unsigned)g_profile.ddc_mode);
}

void ddc_force_mutation(uint16_t seed, ep_status_t *status) {
    apply_profile_mutation(seed);
    if (status != NULL) {
        status->mutation_enabled = 1u;
    }
    log_event(EP_EVENT_EDID_MUTATION, EP_RISK_MEDIUM, "manual mutation applied seed=0x%04X", seed);
}

void ddc_build_report(char *buffer, size_t length) {
    if (buffer == NULL || length == 0u) {
        return;
    }

    snprintf(buffer, length,
             "ddc_mode=%u edid_checksum=%u transactions=%u monitor=%s",
             (unsigned)g_profile.ddc_mode,
             (unsigned)g_edid.checksum_valid,
             (unsigned)g_txn_count,
             g_edid.monitor_name);
}

const ep_edid_image_t *ddc_active_edid(void) {
    return &g_edid;
}

const ep_ddc_txn_t *ddc_transaction(size_t index) {
    if (index >= g_txn_count) {
        return NULL;
    }
    return &g_transactions[index];
}

size_t ddc_transaction_count(void) {
    return g_txn_count;
}
