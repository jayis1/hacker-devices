/*
 * pldm_codec.c
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include "pldm_codec.h"

const char *pldm_codec_message_name(const mw_message_t *message) {
    if (message == NULL) {
        return "null";
    }
    switch (message->command_code) {
        case 0x04U: return "PLDM get TID";
        case 0x50U: return "PLDM sensor reading";
        case 0x84U: return "SPDM get version";
        case 0x91U: return "PLDM firmware stage";
        case 0x92U: return "endpoint clone probe";
        case 0x0AU: return "endpoint advertisement";
        default: return "generic MCTP payload";
    }
}

uint32_t pldm_codec_measure(const mw_message_t *message) {
    uint32_t acc = 0x13579BDFU;
    if (message == NULL) {
        return acc;
    }
    acc ^= ((uint32_t)message->eid_src << 24);
    acc ^= ((uint32_t)message->eid_dst << 16);
    acc ^= ((uint32_t)message->command_code << 8);
    acc ^= (uint32_t)message->payload_len;
    for (uint8_t i = 0U; i < message->payload_len; ++i) {
        acc = (acc << 5) | (acc >> 27);
        acc ^= message->payload[i];
        acc += (uint32_t)(i * 17U + 3U);
    }
    return acc;
}

void pldm_codec_rewrite_version_floor(mw_message_t *message, uint8_t floor_value) {
    if (message == NULL || message->payload_len < 2U) {
        return;
    }
    message->payload[1] = floor_value;
}

void pldm_codec_rewrite_sensor(mw_message_t *message, uint8_t state_value) {
    if (message == NULL || message->payload_len == 0U) {
        return;
    }
    message->payload[message->payload_len - 1U] = state_value;
}

void pldm_codec_scramble_payload(mw_message_t *message, uint8_t seed) {
    if (message == NULL) {
        return;
    }
    for (uint8_t i = 0U; i < message->payload_len; ++i) {
        message->payload[i] ^= (uint8_t)(seed + (uint8_t)(i * 13U));
    }
}

void pldm_codec_print_message(const mw_message_t *message) {
    if (message == NULL) {
        return;
    }
    printf("[message] src=%u dst=%u tag=%u cmd=0x%02x name=%s len=%u integ=%u payload=",
           message->eid_src,
           message->eid_dst,
           message->tag,
           message->command_code,
           pldm_codec_message_name(message),
           message->payload_len,
           message->integrity);
    for (uint8_t i = 0U; i < message->payload_len; ++i) {
        printf("%02x", message->payload[i]);
        if ((i + 1U) < message->payload_len) {
            putchar(':');
        }
    }
    putchar('\n');
}
