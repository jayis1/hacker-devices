/*
 * board_init.h — board bring-up prototypes
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_BOARD_INIT_H
#define PULSEREAPER_BOARD_INIT_H

#include <stdint.h>
#include "board.h"

void board_init(void);          /* full bring-up */
void board_clock_init(void);    /* HSE + PLL1 -> 480 MHz */
void board_gpio_init(void);     /* all pinmux */
void board_dma_init(void);      /* DMA1/DMA2 stream alloc */
void board_nvic_init(void);     /* priority config */
void board_systick_init(void); /* 1 kHz SysTick */

#endif