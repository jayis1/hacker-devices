/*
 * TRACE-REAPER — tamper driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef TAMPER_H
#define TAMPER_H

#include <stdint.h>

typedef void (*tamper_cb_t)(void);

void tamper_init(tamper_cb_t cb);
int  tamper_check(void);
int  tamper_is_triggered(void);
void tamper_clear(void);
void tamper_zeroize(void *p, uint32_t n);

#endif /* TAMPER_H */