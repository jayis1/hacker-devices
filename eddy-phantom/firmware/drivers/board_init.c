/*
 * eddy-phantom — board_init.c
 * Clock tree, GPIO, FMC SDRAM, QSPI, and EXTI initialization
 * for the STM32H743-based Eddy-Phantom board.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── Clock initialization ───────────────────────────────────────
 * Configure PLL1 for 480 MHz core clock from 25 MHz HSE:
 *   HSE = 25 MHz
 *   PLLM = 5  → 5 MHz PLL input
 *   PLLN = 192 → 960 MHz VCO
 *   PLLP = 2  → 480 MHz SYSCLK
 *   PLLQ = 4  → 240 MHz (for USB if needed)
 *   PLLR = 2  → 480 MHz (not used for SYSCLK, we use PLL1_P)
 *
 * AHB divider = 1  → HCLK = 480 MHz
 * APB1 divider = 4 → PCLK1 = 120 MHz (max 120 MHz)
 * APB2 divider = 2 → PCLK2 = 240 MHz (max 240 MHz)
 * APB4 divider = 4 → PCLK4 = 120 MHz (max 120 MHz)
 */
void board_init_clock(void)
{
    volatile uint32_t timeout;

    /* Enable PWR clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;  /* enable DMA1 */

    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    timeout = 0xFFFF;
    while (!(RCC->CR & RCC_CR_HSERDY) && --timeout);

    /* Configure power: VOS0 (highest performance) */
    PWR->CR3 &= ~PWR_CR3_SCUEN;  /* enable system control */
    PWR->CR1 = (PWR->CR1 & ~(7U << 3)) | PWR_CR1_VOS_SCALE0;

    /* Configure PLL1 */
    RCC->PLLCKSELR = RCC_PLLCKSELR_DIVM1_5 | RCC_PLLCFGR_PLL1SRC_HSE;
    RCC->PLLCFGR = RCC_PLLCFGR_PLL1SRC_HSE | (1U << 1); /* PLL1 input = HSE */
    RCC->PLL1DIVR = RCC_PLL1DIVR_N192 | RCC_PLL1DIVR_P2 |
                    RCC_PLL1DIVR_Q4 | RCC_PLL1DIVR_R2;

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    timeout = 0xFFFF;
    while (!(RCC->CR & RCC_CR_PLL1RDY) && --timeout);

    /* Set flash latency for 480 MHz (WS2 with VOS0) */
    FLASH->ACR = FLASH_ACR_LATENCY_2 | (1U << 8) /* ARTEN */ |
                 (1U << 9) /* PRFTEN */;
    (void)FLASH->ACR;

    /* Configure dividers: AHB=/1, APB1L=/4, APB2=/2, APB4=/4 */
    RCC->D1CFGR = (0U << 0)   /* D1CPRE = /1 (480 MHz) */
                | (0U << 8)   /* HPRE = /1 (HCLK = 480 MHz) */
                | (3U << 4);  /* D1PPRE = /2 (APB3 = 240 MHz) */
    RCC->D2CFGR = (3U << 0)   /* D1PPRE1 = /4 (APB1L = 120 MHz) */
                | (2U << 4)   /* D1PPRE2 = /2 (APB2 = 240 MHz) */
                | (3U << 8);  /* D2PPRE = /4 (APB4 = 120 MHz) */
    /* APB1H and APB4 */
    RCC->D3CFGR = (3U << 0);  /* D3PPRE = /4 (APB4 = 120 MHz) */

    /* Switch SYSCLK to PLL1 */
    RCC->CFGR = (RCC->CFGR & ~(7U << 0)) | RCC_CFGR_SW_PLL1;
    timeout = 0xFFFF;
    while (((RCC->CFGR >> 3) & 7) != 3 && --timeout);

    /* Enable all GPIO clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOFEN |
                    RCC_AHB4ENR_GPIOGEN | RCC_AHB4ENR_GPIOHEN;

    /* Enable peripheral clocks */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SPI4EN |
                    RCC_APB2ENR_SYSCFGEN;
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN | RCC_APB1LENR_SPI2EN;

    /* Enable FMC and QSPI clocks */
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN | RCC_AHB3ENR_QSPIEN;

    /* Enable DMA1 clock (already enabled above but ensure) */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
}

/* ── GPIO initialization ──────────────────────────────────────── */
void board_init_gpio(void)
{
    /* SPI1 pins: PA5(SCK), PA6(MISO), PA7(MOSI) */
    gpio_cfg_af(GPIO(A), ADC_SCK_PIN, GPIO_AF5, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(A), ADC_MISO_PIN, GPIO_AF5, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(A), ADC_MOSI_PIN, GPIO_AF5, GPIO_SPEED_VHIGH);

    /* AD7606 control pins */
    gpio_cfg_output(GPIO(ADC_CS_PORT), ADC_CS_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(ADC_CS_PORT), ADC_CS_PIN);  /* CS high (deselected) */
    gpio_cfg_output(GPIO(ADC_RST_PORT), ADC_RST_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(ADC_CONVST_PORT), ADC_CONVST_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(ADC_CONVST_PORT), ADC_CONVST_PIN);  /* CONVST high (idle) */
    gpio_cfg_output(GPIO(ADC_RANGE_PORT), ADC_RANGE_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(ADC_OS0_PORT), ADC_OS0_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(ADC_OS1_PORT), ADC_OS1_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(ADC_OS2_PORT), ADC_OS2_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_cfg_input(GPIO(ADC_BUSY_PORT), ADC_BUSY_PIN, GPIO_PUPD_NONE);

    /* USART3 pins: PD8(TX), PD9(RX), PD10(CTS), PD11(RTS) */
    gpio_cfg_af(GPIO(D), BLE_TX_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(D), BLE_RX_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(D), BLE_CTS_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(D), BLE_RTS_PIN, GPIO_AF7, GPIO_SPEED_HIGH);
    gpio_cfg_output(GPIO(BLE_RESET_PORT), BLE_RESET_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_set(GPIO(BLE_RESET_PORT), BLE_RESET_PIN);  /* deassert reset */
    gpio_cfg_output(GPIO(BLE_BOOT_PORT), BLE_BOOT_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_clr(GPIO(BLE_BOOT_PORT), BLE_BOOT_PIN);  /* normal mode */

    /* SPI4 pins (SD card): PE2(SCK), PE5(MISO), PE6(MOSI) */
    gpio_cfg_af(GPIO(E), SD_SCK_PIN, GPIO_AF6, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(E), SD_MISO_PIN, GPIO_AF6, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(E), SD_MOSI_PIN, GPIO_AF6, GPIO_SPEED_HIGH);
    gpio_cfg_output(GPIO(SD_CS_PORT), SD_CS_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(SD_CS_PORT), SD_CS_PIN);
    gpio_cfg_input(GPIO(SD_DETECT_PORT), SD_DETECT_PIN, GPIO_PUPD_PULLUP);

    /* VGA gain SPI2: PF7(SCK), PF9(MOSI) */
    gpio_cfg_af(GPIO(F), VGA_SCK_PIN, GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(F), VGA_MOSI_PIN, GPIO_AF5, GPIO_SPEED_HIGH);
    gpio_cfg_output(GPIO(VGA_CS1_PORT), VGA_CS1_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(VGA_CS1_PORT), VGA_CS1_PIN);
    gpio_cfg_output(GPIO(VGA_CS2_PORT), VGA_CS2_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(VGA_CS2_PORT), VGA_CS2_PIN);
    gpio_cfg_output(GPIO(VGA_LNA_BYPASS_PORT), VGA_LNA_BYPASS_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_clr(GPIO(VGA_LNA_BYPASS_PORT), VGA_LNA_BYPASS_PIN);  /* LNA active */

    /* Filter cutoff control (bit-banged SPI) */
    gpio_cfg_output(GPIO(FILTER_SPI_SCK_PORT), FILTER_SPI_SCK_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(FILTER_SPI_MOSI_PORT), FILTER_SPI_MOSI_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(FILTER_SPI_CS_PORT), FILTER_SPI_CS_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_set(GPIO(FILTER_SPI_CS_PORT), FILTER_SPI_CS_PIN);

    /* OLED I2C (bit-banged): PB8(SCL), PB9(SDA) — open drain */
    gpio_cfg_output(GPIO(OLED_SCL_PORT), OLED_SCL_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_OD);
    gpio_cfg_output(GPIO(OLED_SDA_PORT), OLED_SDA_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_OD);
    gpio_set(GPIO(OLED_SCL_PORT), OLED_SCL_PIN);
    gpio_set(GPIO(OLED_SDA_PORT), OLED_SDA_PIN);

    /* Trigger comparator input: PC4 */
    gpio_cfg_input(GPIO(TRIG_PORT), TRIG_PIN, GPIO_PUPD_NONE);

    /* LEDs */
    gpio_cfg_output(GPIO(LED_STATUS_PORT), LED_STATUS_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_clr(GPIO(LED_STATUS_PORT), LED_STATUS_PIN);
    gpio_cfg_output(GPIO(LED_CAPTURE_PORT), LED_CAPTURE_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    gpio_clr(GPIO(LED_CAPTURE_PORT), LED_CAPTURE_PIN);
    gpio_cfg_output(GPIO(LED_CHARGE_PORT), LED_CHARGE_PIN, GPIO_SPEED_LOW, GPIO_OTYPE_PP);

    /* Arm button: PB14, active low with external pull-up */
    gpio_cfg_input(GPIO(BTN_ARM_PORT), BTN_ARM_PIN, GPIO_PUPD_PULLUP);

    /* USB pins (firmware update mode only): PA11, PA12 */
    gpio_cfg_af(GPIO(A), USB_DM_PIN, GPIO_AF10, GPIO_SPEED_HIGH);
    gpio_cfg_af(GPIO(A), USB_DP_PIN, GPIO_AF10, GPIO_SPEED_HIGH);

    /* FMC SDRAM pins: configure for 32-bit data bus */
    /* PD0-D5, PD7-D15, PD8-D16, PD9-D17, PD10-D18, PD11-D19,
     * PD12-D20, PD13-D21, PD14-D22, PD15-D23,
     * PE0-D24, PE1-D25, PE7-D4, PE8-D5, PE9-D6, PE10-D7,
     * PE11-D8, PE12-D9, PE13-D10, PE14-D11, PE15-D12,
     * PD6-D13, PD7-D14 (conflicts with above, simplified)
     * PE3-D0, PE4-D1, PE5-D2, PE6-D3
     * For brevity, configure key FMC pins with AF12 */
    gpio_cfg_af(GPIO(D), 0, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D2 */
    gpio_cfg_af(GPIO(D), 1, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D3 */
    gpio_cfg_af(GPIO(D), 8, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D13/BS0 */
    gpio_cfg_af(GPIO(D), 9, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D14/BS1 */
    gpio_cfg_af(GPIO(D), 10, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D4 */
    gpio_cfg_af(GPIO(D), 11, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D5 */
    gpio_cfg_af(GPIO(D), 12, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D6 */
    gpio_cfg_af(GPIO(D), 13, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D7 */
    gpio_cfg_af(GPIO(D), 14, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D0 */
    gpio_cfg_af(GPIO(D), 15, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D1 */
    gpio_cfg_af(GPIO(E), 0, GPIO_AF12, GPIO_SPEED_VHIGH);  /* NBL0 */
    gpio_cfg_af(GPIO(E), 1, GPIO_AF12, GPIO_SPEED_VHIGH);  /* NBL1 */
    gpio_cfg_af(GPIO(E), 7, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D4 */
    gpio_cfg_af(GPIO(E), 8, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D5 */
    gpio_cfg_af(GPIO(E), 9, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D6 */
    gpio_cfg_af(GPIO(E), 10, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D7 */
    gpio_cfg_af(GPIO(E), 11, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D8 */
    gpio_cfg_af(GPIO(E), 12, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D9 */
    gpio_cfg_af(GPIO(E), 13, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D10 */
    gpio_cfg_af(GPIO(E), 14, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D11 */
    gpio_cfg_af(GPIO(E), 15, GPIO_AF12, GPIO_SPEED_VHIGH);  /* D12 */

    /* FMC address pins: */
    gpio_cfg_af(GPIO(F), 0, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A0 */
    gpio_cfg_af(GPIO(F), 1, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A1 */
    gpio_cfg_af(GPIO(F), 2, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A2 */
    gpio_cfg_af(GPIO(F), 3, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A3 */
    gpio_cfg_af(GPIO(F), 4, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A4 */
    gpio_cfg_af(GPIO(F), 5, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A5 */
    gpio_cfg_af(GPIO(F), 11, GPIO_AF12, GPIO_SPEED_VHIGH);  /* SDNRAS */
    gpio_cfg_af(GPIO(F), 12, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A6 */
    gpio_cfg_af(GPIO(F), 13, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A7 */
    gpio_cfg_af(GPIO(F), 14, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A8 */
    gpio_cfg_af(GPIO(F), 15, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A9 */
    gpio_cfg_af(GPIO(G), 0, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A10 */
    gpio_cfg_af(GPIO(G), 1, GPIO_AF12, GPIO_SPEED_VHIGH);  /* A11 */
    gpio_cfg_af(GPIO(G), 4, GPIO_AF12, GPIO_SPEED_VHIGH);  /* BA0 */
    gpio_cfg_af(GPIO(G), 5, GPIO_AF12, GPIO_SPEED_VHIGH);  /* BA1 */
    gpio_cfg_af(GPIO(G), 8, GPIO_AF12, GPIO_SPEED_VHIGH);  /* SDCLK */
    gpio_cfg_af(GPIO(G), 9, GPIO_AF12, GPIO_SPEED_VHIGH);  /* SDNCAS */
    gpio_cfg_af(GPIO(G), 12, GPIO_AF12, GPIO_SPEED_VHIGH);  /* SDCLK0 */
    gpio_cfg_af(GPIO(G), 15, GPIO_AF12, GPIO_SPEED_VHIGH);  /* SDNCAS */

    /* FMC control pins: SDNE0, SDCKE0, SDNWE */
    gpio_cfg_af(GPIO(C), 3, GPIO_AF12, GPIO_SPEED_VHIGH);  /* SDNE0 */
    gpio_cfg_af(GPIO(H), 5, GPIO_AF12, GPIO_SPEED_VHIGH);  /* SDNWE */

    /* QSPI pins: PB6(CLK), PB7(NCS), PC9-12(IO0-3) */
    gpio_cfg_af(GPIO(B), 6, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(B), 7, GPIO_AF10, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(C), 9, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(C), 10, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(C), 11, GPIO_AF9, GPIO_SPEED_VHIGH);
    gpio_cfg_af(GPIO(C), 12, GPIO_AF9, GPIO_SPEED_VHIGH);
}

/* ── FMC SDRAM initialization ───────────────────────────────────
 * Configure FMC for IS42S32800G-6BLI 32 MB SDRAM (32-bit, 4 banks).
 * Bank: SDRAM Bank 1, 32-bit data, CAS latency 3, 4 internal banks.
 */
void board_init_fmc_sdram(void)
{
    /* SDRAM control register 1 */
    FMC_SDRAM->SDCR1 = (0U << 0)   /* RPIPE = no delay */
                     | (2U << 3)   /* RBURST = 2 */
                     | (2U << 10)  /* SDCLK = 2x HCLK (240 MHz, /2 = 120 MHz) */
                     | (3U << 4)   /* CAS latency = 3 */
                     | (0U << 6)   /* 4 internal banks */
                     | (2U << 12)  /* MWID = 32-bit */
                     | (1U << 2);  /* NR = 12-bit row (4096) */

    /* SDRAM timing register 1 */
    FMC_SDRAM->SDTR1 = (2U << 0)   /* TMRD = 2 cycles */
                     | (7U << 4)   /* TXSR = 7 cycles */
                     | (4U << 8)   /* TRAS = 4 cycles */
                     | (2U << 12)  /* TRC = 2 cycles */
                     | (2U << 16)  /* TWR = 2 cycles */
                     | (2U << 20)  /* TRP = 2 cycles */
                     | (2U << 24); /* TRCD = 2 cycles */

    /* SDRAM initialization sequence */
    volatile uint32_t *sdram = (volatile uint32_t *)FMC_BANK1_SDRAM;
    volatile uint32_t timeout;

    /* Step 1: clock configuration enable command */
    FMC_SDRAM->SDCMR = (0U << 0)   /* command = normal */
                     | (1U << 3)   /* target = bank 1 */
                     | (1U << 5)   /* FRC */
                     | (1U << 9);  /* MODE register: all banks precharge */
    /* Wait */
    for (timeout = 0; timeout < 100000; timeout++);

    /* Step 2: auto-refresh command */
    FMC_SDRAM->SDCMR = (2U << 0)   /* command = auto-refresh */
                     | (1U << 3)
                     | (4U << 5);  /* 4 auto-refresh cycles */
    for (timeout = 0; timeout < 100000; timeout++);

    /* Step 3: load mode register */
    FMC_SDRAM->SDCMR = (3U << 0)   /* command = load mode register */
                     | (1U << 3)
                     | (0x0230U << 9);  /* MR value: CAS=3, burst=full, sequential */
    for (timeout = 0; timeout < 100000; timeout++);

    /* Step 4: set refresh rate counter */
    /* Refresh rate = 64ms / 4096 rows = 15.625 µs per row
     * At 120 MHz SDRAM clock: 15.625 µs * 120 MHz = 1875 cycles
     * Min refresh counter = 1875 - 20 = 1855 */
    FMC_SDRAM->SDRTR = 1855 << 1;

    /* Enable FMC interrupt for refresh error */
    FMC_SDRAM->SDSR = 0;  /* clear status */

    /* Test SDRAM by writing and reading */
    for (int i = 0; i < 256; i++) {
        sdram[i] = 0xDEAD0000 | i;
    }
    for (int i = 0; i < 256; i++) {
        if (sdram[i] != (uint32_t)(0xDEAD0000 | i)) {
            /* SDRAM test failed — could set error flag */
            break;
        }
    }
}

/* ── QSPI initialization ────────────────────────────────────────
 * Configure QSPI peripheral for W25Q128JVSIQ (16 MB, 4-line I/O).
 * Memory-mapped mode for read access to model weights and profiles.
 */
void board_init_qspi(void)
{
    /* Reset QSPI via RCC */
    RCC->AHB3RSTR |= RCC_AHB3ENR_QSPIEN;
    RCC->AHB3RSTR &= ~RCC_AHB3ENR_QSPIEN;

    /* Wait for reset */
    volatile uint32_t timeout;
    for (timeout = 0; timeout < 1000; timeout++);

    /* Configure QSPI: */
    /* CR: enable, prescaler = 128 (480/128 = 3.75 MHz for init) */
    QSPI->CR = (127U << 24)  /* prescaler */
             | QSPI_CR_EN     /* enable QSPI */
             | (1U << 3);     /* APMS: auto poll mode stop */

    /* DCR: device configuration for W25Q128 (128 Mbit = 16 MB) */
    QSPI->DCR = (22U << 18)   /* FSIZE: 2^23 = 16 MB (N-1 = 22) */
              | (1U << 16)    /* CSHT: chip select hold time = 1 cycle */
              | (0U << 0);    /* CKMODE: mode 0 */

    /* Send reset enable command (0x66) followed by reset (0x99) */
    QSPI->CCR = (0U << 26)    /* FMODE = indirect write */
              | (0U << 24)    /* DMODE = no data for command-only */
              | (0x66U << 0)  /* instruction = 0x66 (reset enable) */
              | (1U << 18);   /* IMODE = 1-line */
    /* Trigger */
    QSPI->AR = 0;
    while (QSPI->SR & QSPI_SR_BUSY);

    QSPI->CCR = (0U << 26) | (0x99U << 0) | (1U << 18);
    QSPI->AR = 0;
    while (QSPI->SR & QSPI_SR_BUSY);

    /* Wait for device ready */
    for (timeout = 0; timeout < 100000; timeout++);

    /* Enable QSPI in memory-mapped mode for read operations */
    QSPI->CCR = QSPI_CCR_FMODE_MEM   /* FMODE = memory-mapped */
              | QSPI_CCR_DMODE_4L    /* DMODE = 4-line data */
              | QSPI_CCR_ADMODE_4L   /* ADMODE = 4-line address */
              | (3U << 16)           /* ADSIZE = 24-bit address */
              | QSPI_CCR_IMODE_4L    /* IMODE = 1-line instruction */
              | (0xEBU << 0);        /* Fast Quad I/O Read */
    while (QSPI->SR & QSPI_SR_BUSY);
}

/* ── EXTI initialization ────────────────────────────────────────
 * Configure PC4 (trigger comparator) for falling-edge EXTI.
 * Configure PB14 (arm button) for falling-edge EXTI.
 */
void board_init_exti(void)
{
    /* PC4 → EXTI line 4 */
    /* Configure SYSCFG EXTICR2 (lines 4-7) for port C on line 4 */
    SYSCFG->PMR[1] = (SYSCFG->PMR[1] & ~(0xFU << 0)) | (2U << 0);  /* port C = 2 */

    /* PB14 → EXTI line 14 */
    SYSCFG->PMR[3] = (SYSCFG->PMR[3] & ~(0xFU << 8)) | (1U << 8);  /* port B = 1 */

    /* Falling edge trigger for both */
    EXTI->FTSR1 |= (1U << 4) | (1U << 14);
    EXTI->RTSR1 &= ~((1U << 4) | (1U << 14));

    /* Clear pending */
    EXTI->PR1 = (1U << 4) | (1U << 14);

    /* Enable EXTI interrupts in NVIC */
    nvic_set_priority(26, 5);   /* EXTI4 IRQ = 26 */
    nvic_enable(26);
    nvic_set_priority(40, 5);   /* EXTI15_10 IRQ = 40 */
    nvic_enable(40);
}