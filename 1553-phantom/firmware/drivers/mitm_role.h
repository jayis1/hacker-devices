/*
 * mitm_role.h — Inline MITM role
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef MITM_ROLE_H
#define MITM_ROLE_H

#include <stdint.h>
#include "fpga_link.h"

void mitm_role_init(void);
void mitm_role_start(void);
void mitm_role_stop(void);
void mitm_role_tick(uint32_t now_ms);
void mitm_role_cli(int argc, char argv[8][32]);
void mitm_role_on_word(int ch, const decoded_word_t *w, uint32_t ts);
int  mitm_role_rule_count(void);

#endif /* MITM_ROLE_H */