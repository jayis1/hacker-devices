/*
 * sdram.c — WireReaper SDRAM driver via FMC for capture buffer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Initializes the IS42S32800G-6BLI 512 MB SDRAM on the FMC bus
 * and provides the ring-buffer backing store for captured bus
 * transactions. The SDRAM is mapped at 0xD0000000.
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* SDRAM configuration for IS42S32800G-6BLI (32M x 32 x 4 banks) */
/* Bank: SDNE0, 32-bit data bus */

#define SDRAM_REFRESH_RATE   8192   /* cycles per 64 ms */
#define SDRAM_REFRESH_CYCLES (64000 / (SDRAM_REFRESH_RATE))

#define SDCR_CONFIG (  (0 << 0) |  /* SDCLK = 2x HCLK/2 */ \
                       (2 << 3) |  /* RBURST = burst */ \
                       (1 << 5) |  /* RPIPE = 1 */ \
                       (0 << 7) |  /* MWID = 8-bit... actually 32 */ \
                       (3 << 10) | /* NR = 12 rows */ \
                       (2 << 13) ) /* NC = 9 columns */

void wr_sdram_init(void) {
    /* Enable FMC clock */
    RCC_AHB3ENR |= RCC_AHB3ENR_FMC;

    /* Enable GPIO ports for FMC pins */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOD | RCC_AHB1ENR_GPIOE |
                   RCC_AHB1ENR_GPIOF | RCC_AHB1ENR_GPIOG |
                   RCC_AHB1ENR_GPIOH;

    /* Configure FMC GPIO pins as AF12 (FMC alternate function) */
    /* FMC_D0-D31, FMC_A0-A11, FMC_BA0-BA1, FMC_SDNE0, FMC_SDCKE0,
     * FMC_SDNRAS, FMC_SDNCAS, FMC_SDNCLK, FMC_SDNWE, FMC_NBL0-NBL3
     *
     * Pin assignments (STM32H743 LQFP100):
     * PD0: FMC_D2, PD1: FMC_D3, PD8: FMC_D13, PD9: FMC_D14,
     * PD10: FMC_D15, PD14: FMC_D0, PD15: FMC_D1
     * PE0: FMC_NBL0, PE1: FMC_NBL1, PE7-15: FMC_D4-D12
     * PF0: FMC_A0, PF1: FMC_A1, ..., PF15: FMC_A9
     * PG0: FMC_A10, PG1: FMC_A11, PG4: FMC_BA0, PG5: FMC_BA1,
     * PG8: FMC_SDCLK, PG15: FMC_SDNCAS
     * PH2: FMC_SDCKE0, PH3: FMC_SDNE0, PH5: FMC_SDNWE, PH7: FMC_SDNRAS
     */
    gpio_t *ports[] = {GPIO(GPIOD_BASE), GPIO(GPIOE_BASE),
                       GPIO(GPIOF_BASE), GPIO(GPIOG_BASE),
                       GPIO(GPIOH_BASE)};

    /* FMC pin arrays: {port_index, pin_number} */
    struct { uint8_t port; uint8_t pin; } fmc_pins[] = {
        {0, 0}, {0, 1}, {0, 8}, {0, 9}, {0, 10}, {0, 14}, {0, 15}, /* PD */
        {1, 0}, {1, 1}, {1, 7}, {1, 8}, {1, 9}, {1, 10},
        {1, 11}, {1, 12}, {1, 13}, {1, 14}, {1, 15},               /* PE */
        {2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5},
        {2, 12}, {2, 13}, {2, 14}, {2, 15},                         /* PF */
        {3, 0}, {3, 1}, {3, 4}, {3, 5}, {3, 8}, {3, 15},           /* PG */
        {4, 2}, {4, 3}, {4, 5}, {4, 7},                             /* PH */
    };

    for (int i = 0; i < (int)(sizeof(fmc_pins)/sizeof(fmc_pins[0])); i++) {
        gpio_t *g = ports[fmc_pins[i].port];
        uint8_t p = fmc_pins[i].pin;
        g->MODER &= ~(3U << (p * 2));
        g->MODER |= (GPIO_MODE_AF << (p * 2));
        g->OSPEEDR |= (GPIO_OSPEED_VHIGH << (p * 2));
        g->PUPDR &= ~(3U << (p * 2));
        g->AFR[p / 8] &= ~(0xFU << ((p % 8) * 4));
        g->AFR[p / 8] |= (12U << ((p % 8) * 4)); /* AF12 = FMC */
    }

    /* Configure SDRAM controller registers */
    /* SDCR1: Bank 1 config */
    FMC_SDRAM->SDCR1 = (0U << 0) |  /* SDClock = HCLK/2 */
                       (0U << 2) |  /* Burst length = 1 */
                       (2U << 3) |  /* RBURST */
                       (1U << 5) |  /* RPIPE = 1 cycle */
                       (3U << 10) | /* NR = 12-bit row */
                       (2U << 13);  /* NC = 9-bit column */
    FMC_SDRAM->SDCR2 = 0; /* Bank 2 unused */

    /* SDTR1: Bank 1 timing (for 166 MHz SDRAM, conservative) */
    FMC_SDRAM->SDTR1 = (2U << 0)  | /* TMRD = 2 cycles */
                       (7U << 4)  | /* TXSR = 7 cycles */
                       (4U << 8)  | /* TRAS = 4 cycles */
                       (7U << 12) | /* TRC = 7 cycles */
                       (2U << 16) | /* TWR = 2 cycles */
                       (2U << 20) | /* TRP = 2 cycles */
                       (2U << 24);  /* TRCD = 2 cycles */
    FMC_SDRAM->SDTR2 = 0;

    /* ---- SDRAM initialization sequence ---- */
    /* Step 1: Send clock configuration command */
    FMC_SDRAM->SDCMR = FMC_SDCMR_MODE_CLK | (1U << 3); /* Bank 1 */
    /* Delay ~100 us */
    for (volatile int i = 0; i < 10000; i++);

    /* Step 2: Send PALL (precharge all) command */
    FMC_SDRAM->SDCMR = FMC_SDCMR_MODE_PALL | (1U << 3);
    for (volatile int i = 0; i < 10000; i++);

    /* Step 3: Send auto-refresh command (8 cycles) */
    for (int i = 0; i < 8; i++) {
        FMC_SDRAM->SDCMR = FMC_SDCMR_MODE_AUTO | (1U << 3) | (1U << 5);
        for (volatile int j = 0; j < 1000; j++);
    }

    /* Step 4: Send load mode register command */
    /* Mode register: burst length=1, burst type=sequential, CAS latency=3 */
    uint32_t mr_value = 0x23; /* CAS=3, BL=1, sequential */
    FMC_SDRAM->SDCMR = FMC_SDCMR_MODE_WRITE | (1U << 3) |
                       (mr_value << 9);
    for (volatile int i = 0; i < 10000; i++);

    /* Step 5: Set refresh timer */
    FMC_SDRAM->SDRTR = (SDRAM_REFRESH_CYCLES << 1);

    /* SDRAM is now ready at 0xD0000000 */
    /* Quick write/read test */
    volatile uint32_t *test = (volatile uint32_t *)SDRAM_BANK_ADDR;
    test[0] = 0xDEADBEEF;
    test[1] = 0xCAFEBABE;
    if (test[0] != 0xDEADBEEF || test[1] != 0xCAFEBABE) {
        /* SDRAM init failed — would set an error flag */
    }
}

/* ---- Write to SDRAM ---- */
void wr_sdram_write(uint32_t offset, const void *data, int len) {
    volatile uint8_t *dst = (volatile uint8_t *)SDRAM_BANK_ADDR + offset;
    const uint8_t *src = (const uint8_t *)data;
    for (int i = 0; i < len; i++)
        dst[i] = src[i];
}

/* ---- Read from SDRAM ---- */
void wr_sdram_read(uint32_t offset, void *data, int len) {
    volatile uint8_t *src = (volatile uint8_t *)SDRAM_BANK_ADDR + offset;
    uint8_t *dst = (uint8_t *)data;
    for (int i = 0; i < len; i++)
        dst[i] = src[i];
}