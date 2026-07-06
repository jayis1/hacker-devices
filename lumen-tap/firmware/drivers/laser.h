/*
 * lumen-tap/firmware/drivers/laser.h
 * Laser driver + safety interlock management for LumenTap.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#ifndef LUMEN_TAP_LASER_H
#define LUMEN_TAP_LASER_H

#include <stdint.h>
#include "../board.h"

/* Laser state */
typedef struct {
    uint8_t  enabled;       /* 1 = emission permitted by all interlocks */
    uint8_t  pwm_duty;      /* 0..255 (capped to LTM_LASER_PWR_MAX) */
    uint8_t  requested;     /* operator-requested duty (before cap) */
    uint8_t  class1_cap;    /* 1 = enforcing Class 1 cap */
    uint16_t ambient_cnt;   /* latest TSL257 reading */
    uint32_t on_time_ms;    /*累计 emission time */
} ltm_laser_t;

extern volatile ltm_laser_t g_laser;

void laser_init(void);
void laser_set_power(uint8_t duty);      /* 0..255; capped to Class 1 */
void laser_arm(uint8_t arm);             /* operator arm/disarm */
void laser_update_ambient(void);         /* read TSL257, update permit */
int  laser_is_emitting(void);
void laser_emergency_off(void);          /* immediate cutoff, no checks */

/* Called every 1ms from SysTick; updates interlock + on-time counter. */
void laser_tick_ms(void);

#endif /* LUMEN_TAP_LASER_H */