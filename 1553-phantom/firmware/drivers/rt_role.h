/*
 * rt_role.h — Remote Terminal emulator
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef RT_ROLE_H
#define RT_ROLE_H

#include <stdint.h>
#include "fpga_link.h"

void     rt_role_init(void);
void     rt_role_start(void);
void     rt_role_stop(void);
void     rt_role_tick(uint32_t now_ms);
void     rt_role_cli(int argc, char argv[8][32]);
void     rt_role_on_command(int ch, const decoded_word_t *cmd);  /* from capture */
int      rt_role_emulated_count(void);
void     rt_role_panic(void);

#endif /* RT_ROLE_H */