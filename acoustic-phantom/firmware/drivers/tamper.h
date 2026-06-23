/*
 * ACOUSTIC-PHANTOM — Tamper detection driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Accelerometer-based (LIS2DH12) and switch-based physical tamper
 * detection. On tamper event, triggers zeroization of all sensitive
 * state via the callback registered in main.c.
 */
#ifndef TAMPER_H
#define TAMPER_H

void  tamper_init(void);
void  tamper_check(void);  /* Called periodically from main loop */
uint8_t tamper_was_triggered(void);

#endif /* TAMPER_H */