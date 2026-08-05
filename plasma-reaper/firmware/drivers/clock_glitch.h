/*
 * clock_glitch.h — Clock glitch driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CLOCK_GLITCH_H
#define CLOCK_GLITCH_H

#include <stdint.h>
#include "../board.h"

void clock_glitch_init(void);
void clock_glitch_configure(uint8_t shape, uint32_t cycle_offset,
                            uint32_t width_ns);
void clock_glitch_passthrough(void);
void clock_glitch_inject_edge(void);
void clock_glitch_suppress_edge(void);

/* Clock shape constants */
#define CLOCK_GLITCH_EXTRA_EDGE   0
#define CLOCK_GLITCH_SUPPRESS     1

/* Clock source select (for TS3A5018 4:1 mux) */
#define CLOCK_SRC_PASSTHROUGH  0x00  /* target's own clock */
#define CLOCK_SRC_FORCED_HIGH  0x01
#define CLOCK_SRC_FORCED_LOW   0x02
#define CLOCK_SRC_INTERNAL_100 0x03  /* onboard 100 MHz oscillator */

#endif /* CLOCK_GLITCH_H */