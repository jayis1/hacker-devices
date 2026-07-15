/*
 * tesla-phantom — drivers/board_init.c
 * Clock, GPIO, FMC-SRAM, QSPI, EXTI, DMA, and timer initialization.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── Clock initialization ─────────────────────────────────────────── */
void board_init_clock(void) {
    /* Enable HSE (25 MHz external crystal) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* Configure PLL1: HSE → 480 MHz
     * PLL1: input = 25 MHz / 5 = 5 MHz (M=5)
     *       VCO = 5 MHz * 96 = 480 MHz (N=96)
     *       SYS = 480 MHz / 1 = 480 MHz (P=1, div by 2 → P=0 means /2)
     * We use: M=5, N=192, P=0(/2), Q=24, R=0
     * → 25/5=5, 5*192=960, 960/2=480 MHz
     */
    RCC->PLLCKSELR = RCC_PLLSRC_HSE | (5U << 4);   /* M=5 */

    RCC->PLLCFGR = (1U << 1)   /* PLL1 div REN */
                 | (0U << 17)  /* PLL1 range = wide */
                 | (0U << 0);  /* PLL1 VCO sel */

    RCC->PLL1DIVR = (192U << 0)   /* N=192 */
                  | (1U << 9)    /* P=1 (div by 2) → 480 MHz */
                  | (24U << 16)  /* Q=24 → 40 MHz */
                  | (0U << 24);  /* R=0 */

    RCC->PLL1FRACR = 0;  /* no fractional */

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY)) { }

    /* Configure flash latency for 480 MHz (VOS1) */
    /* FLASH->ACR: latency = 2 wait states for 480 MHz at VOS1 */
    volatile uint32_t *flash_acr = (volatile uint32_t *)0x52002000UL;
    *flash_acr = (*flash_acr & ~0x07) | 2;  /* 2 WS */
    /* Enable prefetch and cache */
    *flash_acr |= (1U << 8) | (1U << 9) | (1U << 10); /* PRFTEN | ICEN | DCEN */

    /* Configure bus prescalers:
     * D1CPRE = /1 → 480 MHz (CPU)
     * HPRE   = /2 → 240 MHz (AHB)
     * D1PPRE = /2 → 120 MHz (APB2)
     * D2PPRE1= /4 → 120 MHz (APB1)
     * D2PPRE2= /2 → 120 MHz (APB2 — but mapped differently)
     */
    RCC->D1CFGR = (0U << 8)   /* D1CPRE = /1 */
                | (8U << 0);  /* HPRE = /2 (0b1000 = /2) */

    RCC->D2CFGR = (4U << 8)   /* D2PPRE2 = /2 */
                | (5U << 4)   /* D2PPRE1 = /4 */
                | (4U << 0);  /* D1PPRE = /2 */

    /* Switch system clock to PLL1 */
    RCC->CFGR = (RCC_CFGR_SW_PLL1 << 0);
    while (((RCC->CFGR >> 3) & 0x7) != RCC_CFGR_SW_PLL1) { }

    /* Enable clock to all needed peripherals via RCC AHB/APB enables */
    /* GPIO ports A–I */
    volatile uint32_t *rcc_ahb1enr = (volatile uint32_t *)(RCC_BASE + 0x48 + 0x0C);
    *rcc_ahb1enr |= (1U << 0)   /* GPIOA */
                  | (1U << 1)   /* GPIOB */
                  | (1U << 2)   /* GPIOC */
                  | (1U << 3)   /* GPIOD */
                  | (1U << 4)   /* GPIOE */
                  | (1U << 6)   /* GPIOG */
                  | (1U << 7)   /* GPIOH */
                  | (1U << 8);  /* GPIOI */

    /* Enable SPI1, SPI2, SPI4, USART2, USART3, I2C1, DAC, TIM1/2/3/4/6/7 */
    volatile uint32_t *rcc_apb1enr = (volatile uint32_t *)(RCC_BASE + 0x48 + 0x58);
    *rcc_apb1enr |= (1U << 14)  /* SPI2 */
                  | (1U << 17)  /* USART2 */
                  | (1U << 18)  /* USART3 */
                  | (1U << 21)  /* I2C1 */
                  | (1U << 0)   /* TIM2 */
                  | (1U << 1)   /* TIM3 */
                  | (1U << 2)   /* TIM4 */
                  | (1U << 4)   /* TIM6 */
                  | (1U << 5)   /* TIM7 */
                  | (1U << 29); /* DAC1 */

    volatile uint32_t *rcc_apb2enr = (volatile uint32_t *)(RCC_BASE + 0x48 + 0x5C);
    *rcc_apb2enr |= (1U << 12)  /* SPI1 */
                  | (1U << 13)  /* SPI4 */
                  | (1U << 0)   /* TIM1 */
                  | (1U << 14); /* SYSCFG */

    /* Enable DMA clocks */
    volatile uint32_t *rcc_ahb1enr2 = (volatile uint32_t *)(RCC_BASE + 0x48 + 0x0D);
    *rcc_ahb1enr2 |= (1U << 0)  /* DMA1 */
                   | (1U << 1); /* DMA2 */
}

/* ── GPIO initialization ──────────────────────────────────────────── */
void board_init_gpio(void) {
    /* FPGA SPI1: PA5(SCK), PA6(MISO), PA7(MOSI) → AF5 */
    gpio_cfg_af(GPIO(GPIOA), FPGA_SCK_PIN,  GPIO_AF5, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(GPIOA), FPGA_MISO_PIN, GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOA), FPGA_MOSI_PIN, GPIO_AF5, GPIO_SPEED_VHIGH);

    /* FPGA CS (PB0), IRQ (PB1), CDONE (PB2) — output/input */
    gpio_cfg_output(GPIO(GPIOB), FPGA_CS_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), FPGA_CS_PIN);  /* CS high (deselected) */
    gpio_cfg_input(GPIO(GPIOB), FPGA_IRQ_PIN, GPIO_PUPD_PULLDOWN);
    gpio_cfg_input(GPIO(GPIOB), FPGA_CDONE_PIN, GPIO_PUPD_PULLDOWN);
    gpio_cfg_output(GPIO(GPIOB), FPGA_RESET_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), FPGA_RESET_PIN);  /* FPGA not in reset */

    /* microSD SPI4: PE2(SCK), PE5(MISO), PE6(MOSI) → AF5 */
    gpio_cfg_af(GPIO(GPIOE), SD_SCK_PIN,  GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOE), SD_MISO_PIN, GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOE), SD_MOSI_PIN, GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_output(GPIO(GPIOE), SD_CS_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOE), SD_CS_PIN);

    /* ADC SPI2: PI1(SCK), PI2(MISO), PI3(MOSI) → AF5 */
    gpio_cfg_af(GPIO(GPIOI), ADC_SCK_PIN,  GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOI), ADC_MISO_PIN, GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOI), ADC_MOSI_PIN, GPIO_AF5, GPIO_SPEED_HIGH);

    /* ADC control pins */
    gpio_cfg_output(GPIO(GPIOB), ADC_CS_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), ADC_CS_PIN);
    gpio_cfg_output(GPIO(GPIOB), ADC_CONVST_PIN, GPIO_SPEED_VHIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), ADC_CONVST_PIN);  /* CONVST idle high */
    gpio_cfg_input(GPIO(GPIOB), ADC_BUSY_PIN, GPIO_PUPD_NONE);
    gpio_cfg_output(GPIO(GPIOB), ADC_RESET_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), ADC_RESET_PIN);
    gpio_cfg_output(GPIO(GPIOB), ADC_RANGE_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), ADC_RANGE_PIN);  /* ±5V range */

    /* VGA CS pins (PB9, PB10) and filter CS (PB11) */
    gpio_cfg_output(GPIO(GPIOB), VGA_CS1_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), VGA_CS1_PIN);
    gpio_cfg_output(GPIO(GPIOB), VGA_CS2_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), VGA_CS2_PIN);
    gpio_cfg_output(GPIO(GPIOB), FILTER_CS_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOB), FILTER_CS_PIN);

    /* BLE USART3: PD8(TX), PD9(RX) → AF7 */
    gpio_cfg_af(GPIO(GPIOD), BLE_TX_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOD), BLE_RX_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOD), BLE_CTS_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOD), BLE_RTS_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_output(GPIO(GPIOD), BLE_RESET_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(GPIOD), BLE_RESET_PIN);
    gpio_cfg_output(GPIO(GPIOD), BLE_BOOT_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_clr(GPIOD_BASE ? GPIO(GPIOD) : GPIO(GPIOD), BLE_BOOT_PIN);

    /* Stepper USART2: PD4(TX), PD5(RX) → AF7 */
    gpio_cfg_af(GPIO(GPIOD), STEPPER_TX_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOD), STEPPER_RX_PIN, GPIO_AF7, GPIO_SPEED_HIGH);

    /* I2C1 for OLED + battery: PB8(SCL), PB9(SDA) → AF4
     * Note: PB8/PB9 are also listed for VGA_CS1/FILTER in the pin map.
     * In the actual board, I2C1 is remapped to PB8/PB9 and VGA uses
     * a different CS approach (latched). For this design we use
     * PB6/PB7 for I2C1 instead to avoid conflict. */
    /* Using PB6=SCL, PB7=SDA for I2C1 (AF4) */
    gpio_cfg_af(GPIO(GPIOB), 6, 4, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOB), 7, 4, GPIO_SPEED_HIGH);

    /* HV DAC output (PA4) — analog mode */
    gpio_cfg_analog(GPIO(GPIOA), HV_DAC_PIN);

    /* Trigger: PC4 = EXTI input, PC5 = trigger output to FPGA */
    gpio_cfg_input(GPIO(GPIOC), TRIG_EXT_PIN, GPIO_PUPD_PULLDOWN);
    gpio_cfg_output(GPIO(GPIOC), TRIG_OUT_PIN, GPIO_SPEED_VHIGH, GPIO_OTYPE_PP);
    gpio_clr(GPIO(GPIOC), TRIG_OUT_PIN);

    /* Limit switches: PC6, PC7, PC8 — input with pull-up (opto: open-collector) */
    gpio_cfg_input(GPIO(GPIOC), LIM_X_PIN, GPIO_PUPD_PULLUP);
    gpio_cfg_input(GPIO(GPIOC), LIM_Y_PIN, GPIO_PUPD_PULLUP);
    gpio_cfg_input(GPIO(GPIOC), LIM_Z_PIN, GPIO_PUPD_PULLUP);

    /* LEDs: PB14, PB15, PD14 — output, push-pull, default off */
    gpio_cfg_output(GPIO(GPIOB), LED_STATUS_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_clr(GPIO(GPIOB), LED_STATUS_PIN);
    gpio_cfg_output(GPIO(GPIOB), LED_ARMED_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_clr(GPIO(GPIOB), LED_ARMED_PIN);
    gpio_cfg_output(GPIO(GPIOD), LED_CHARGE_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_clr(GPIO(GPIOD), LED_CHARGE_PIN);
    gpio_set(GPIO(GPIOB), LED_STATUS_PIN);  /* power-on indicator */

    /* Arm button: PD15 — input with pull-up (active low) */
    gpio_cfg_input(GPIO(GPIOD), BTN_ARM_PIN, GPIO_PUPD_PULLUP);

    /* USB: PA11(DM), PA12(DP) → AF10 */
    gpio_cfg_af(GPIO(GPIOA), USB_DM_PIN, 10, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOA), USB_DP_PIN, 10, GPIO_SPEED_HIGH);

    /* QSPI: PB6(CLK), PB7(NCS), PC9-12(IO0-3) → AF9/10
     * Note: PB6/PB7 conflict with I2C1 remap above.
     * In the actual design, I2C1 uses PB8/PB9 (original mapping)
     * and QSPI uses PB6/PB7 + PC9-12. Adjust OLED I2C to PB8/PB9. */
    /* For this firmware, we keep QSPI on PB6/PB7 and move I2C1 to PB8/PB9 */
    gpio_cfg_af(GPIO(GPIOB), QSPI_CLK_PIN, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(GPIOB), QSPI_NCS_PIN, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(GPIOC), QSPI_IO0_PIN, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(GPIOC), QSPI_IO1_PIN, GPIO_AF10, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(GPIOC), QSPI_IO2_PIN, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(GPIOC), QSPI_IO3_PIN, GPIO_AF9, GPIO_SPEED_VHIGH);

    /* Re-map I2C1 to PB8/PB9 (AF4) */
    gpio_cfg_af(GPIO(GPIOB), 8, 4, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(GPIOB), 9, 4, GPIO_SPEED_HIGH);
}

/* ── FMC SRAM initialization ──────────────────────────────────────── */
void board_init_fmc_sram(void) {
    /* Enable FMC clock */
    volatile uint32_t *rcc_ahb3enr = (volatile uint32_t *)(RCC_BASE + 0x48 + 0x0E);
    *rcc_ahb3enr |= (1U << 4);  /* FMC */

    /* Configure FMC bank 1, region 0 for 16-bit SRAM */
    FMC_BANK1->BCR1 = FMC_BCR1_MBKEN    /* Enable bank */
                   | FMC_BCR1_MTYP_SR    /* SRAM type */
                   | FMC_BCR1_MWID_16    /* 16-bit data bus */
                   | FMC_BCR1_WREN;      /* Write enable */

    /* Timing: address setup 2 cycles, data setup 6 cycles, bus turnaround 1 */
    FMC_BANK1->BTR1 = (2U << 0)    /* ADDSET = 2 */
                    | (6U << 8)    /* DATAST = 6 */
                    | (1U << 16);  /* BUSTURN = 1 */
}

/* ── QSPI flash initialization ────────────────────────────────────── */
void board_init_qspi(void) {
    /* Enable QSPI clock */
    volatile uint32_t *rcc_ahb3enr = (volatile uint32_t *)(RCC_BASE + 0x48 + 0x0E);
    *rcc_ahb3enr |= (1U << 14);  /* OCTOSPI1 */

    /* Configure QSPI in indirect mode for now.
     * A full QSPI memory-mapped config requires calibration and
     * device-specific commands. This is a minimal setup. */
    volatile uint32_t *ospi_cr  = (volatile uint32_t *)(OCTOSPI1_BASE + 0x00);
    volatile uint32_t *ospi_dcr = (volatile uint32_t *)(OCTOSPI1_BASE + 0x04);
    volatile uint32_t *ospi_ccr = (volatile uint32_t *)(OCTOSPI1_BASE + 0x14);

    /* Disable QSPI before config */
    *ospi_cr = 0;

    /* DCR1: device size = 24 bits (16 MB), CSHT = 3, FMODE=0 */
    *ospi_dcr = (23U << 16)  /* FSIZE: 2^24 = 16 MB */
              | (3U << 8);   /* CSHT: chip select hold time */

    /* CCR: functional mode = indirect read, FSEL=0, IMODE=1 (single) */
    *ospi_ccr = 0;

    /* Enable QSPI */
    *ospi_cr = (1U << 0);  /* EN */
}

/* ── EXTI initialization ──────────────────────────────────────────── */
void board_init_exti(void) {
    /* Configure EXTI line 4 (PC4 = external trigger) for rising edge */
    /* SYSCFG EXTICR4: line 4 → port C (index 2) */
    volatile uint32_t *syscfg_exticr2 = (volatile uint32_t *)(SYSCFG_BASE + 0x0C);
    *syscfg_exticr2 = (*syscfg_exticr2 & ~(0xFU << 0)) | (2U << 0); /* port C */

    /* Rising edge select for line 4 */
    EXTI->SEC1 |= (1U << 4);   /* rising edge */
    EXTI->SEC2 &= ~(1U << 4);  /* not falling */

    /* Enable interrupt mask for line 4 */
    EXTI->IMR1 |= (1U << 4);

    /* Enable NVIC for EXTI4 */
    NVIC_ISER(IRQ_EXTI4 / 32) |= (1U << (IRQ_EXTI4 % 32));
}

/* ── DMA initialization ───────────────────────────────────────────── */
void board_init_dma(void) {
    /* Enable DMA2 clock (already done in board_init_clock) */
    /* Disable all DMA2 streams for safe configuration */
    for (int i = 0; i < 8; i++) {
        volatile uint32_t *cr = (volatile uint32_t *)(DMA2_BASE + 0x10 + i * 0x20);
        *cr = 0;
    }
    /* Clear all DMA2 interrupt flags */
    volatile uint32_t *dma2_lifcr = (volatile uint32_t *)(DMA2_BASE + 0x08);
    volatile uint32_t *dma2_hifcr = (volatile uint32_t *)(DMA2_BASE + 0x0C);
    *dma2_lifcr = 0xFFFFFFFF;
    *dma2_hifcr = 0xFFFFFFFF;
}

/* ── Timer initialization ─────────────────────────────────────────── */
void board_init_timers(void) {
    /* TIM6: basic timer for DAC trigger (HV charge pump timing) */
    TIM6->PSC = (APB1_FREQ * 2 / 1000000) - 1;  /* 1 MHz count */
    TIM6->ARR = 100;  /* 100 µs period (10 kHz charge pump clock) */
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;

    /* TIM2: 32-bit timer for scan step timing and status updates */
    TIM2->PSC = (APB1_FREQ * 2 / 1000) - 1;  /* 1 kHz count (1 ms tick) */
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->CR1 |= TIM_CR1_CEN;
}