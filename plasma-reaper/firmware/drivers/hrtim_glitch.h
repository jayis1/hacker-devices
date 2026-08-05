/*
 * hrtim_glitch.h — HRTIM glitch timing driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HRTIM_GLITCH_H
#define HRTIM_GLITCH_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

void hrtim_glitch_init(void);
void hrtim_glitch_program(uint32_t offset_ns, uint32_t width_ns,
                          uint32_t inter_vector_ns, uint8_t vector_mask);
void hrtim_glitch_arm(void);
void hrtim_glitch_disarm(void);
bool hrtim_glitch_fired(void);

/* Convert nanoseconds to HRTIM ticks (184 ps resolution) */
static inline uint32_t ns_to_hrtim_ticks(uint32_t ns)
{
    /* 1 ns = 1000 ps, HRTIM tick = 184 ps → ticks = ns * 1000 / 184 */
    return (uint32_t)((uint64_t)ns * 1000ULL / 184ULL);
}

/* Convert HRTIM ticks to nanoseconds */
static inline uint32_t hrtim_ticks_to_ns(uint32_t ticks)
{
    return (uint32_t)((uint64_t)ticks * 184ULL / 1000ULL);
}

#endif /* HRTIM_GLITCH_H */