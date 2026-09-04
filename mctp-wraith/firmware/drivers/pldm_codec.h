/*
 * pldm_codec.h
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef MCTP_WRAITH_PLDM_CODEC_H
#define MCTP_WRAITH_PLDM_CODEC_H

#include <stddef.h>
#include <stdint.h>
#include "../board.h"

const char *pldm_codec_message_name(const mw_message_t *message);
uint32_t pldm_codec_measure(const mw_message_t *message);
void pldm_codec_rewrite_version_floor(mw_message_t *message, uint8_t floor_value);
void pldm_codec_rewrite_sensor(mw_message_t *message, uint8_t state_value);
void pldm_codec_scramble_payload(mw_message_t *message, uint8_t seed);
void pldm_codec_print_message(const mw_message_t *message);

#endif
