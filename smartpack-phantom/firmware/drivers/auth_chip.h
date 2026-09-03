/*
 * SmartPack Phantom auth harness interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef SMARTPACK_AUTH_CHIP_H
#define SMARTPACK_AUTH_CHIP_H

#include <stdbool.h>
#include <stdint.h>
#include "../board.h"

void auth_chip_init(sp_auth_state_t *state);
const char *auth_chip_mode_name(sp_auth_mode_t mode);
void auth_chip_set_mode(sp_auth_state_t *state, sp_auth_mode_t mode);
uint32_t auth_chip_issue_challenge(sp_auth_state_t *state, uint32_t seed);
uint32_t auth_chip_respond(sp_auth_state_t *state, uint32_t challenge, bool *accepted);

#endif
