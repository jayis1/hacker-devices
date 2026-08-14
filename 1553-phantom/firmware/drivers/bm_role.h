/*
 * bm_role.h — Bus Monitor role
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef BM_ROLE_H
#define BM_ROLE_H

#include <stdint.h>
#include "fpga_link.h"

void bm_role_init(void);
void bm_role_start(void);
void bm_role_stop(void);
void bm_role_tick(uint32_t now_ms);
void bm_role_cli(int argc, char argv[8][32]);
void bm_role_on_word(int ch, const decoded_word_t *w, uint32_t ts);
uint32_t bm_role_faults(int kind);   /* 0=gap 1=rto 2=parity 3=sync */

#endif /* BM_ROLE_H */