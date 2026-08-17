/*
 * board_init.c — board bring-up for Pulse-Reaper
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal hand-written bring-up for the STM32H743 — clocks, pinmux,
 * NVIC, SysTick. No ST HAL dependency.
 */

#include "board.h"
#include "registers.h"
#include "board_init.h"

/* ----------------------------------------------------------------------- */
/*  Clock tree — HSE 8 MHz -> PLL1 -> SYS 480 MHz, HCLK 240, APB1/2 120      */
/* ----------------------------------------------------------------------- */

void board_clock_init(void) {
    /* 1. Enable HSE and wait for ready */
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0u) { /* spin */ }

    /* 2. Configure PLL1: input divider M=1, N=120, P=2, Q=5, R=2.
     *    VCO = HSE/M * N = 8 * 120 = 960 MHz.
     *    SYS = VCO/P = 480 MHz.
     */
    RCC->PLLCKSELR = (1u << 0);              /* DIVM1 = 1, PLLSRC = HSE */
    RCC->PLLCFGR   = (1u << 1);             /* PLL1 input range, vco range */
    /* N1 = 120 (0-based 0x77) */
    RCC->PLL1DIVR = (0u) | (119u << 8u);    /* P=2 -> N1DIVP=1; N1=119 (0-based) */
    /* Q1=5 (0-based 4), R1=2 (0-based 1) */
    RCC->PLL1DIVR |= (4u << 16u) | (1u << 24u);

    /* 3. Set D1/D2/D3 dividers before switching.
     *    D1 = SYS/2 = 240 (DIV1=2 -> /2)
     *    D2 = HCLK  (DIV2=2)
     *    D3 = /2
     */
    RCC->D1CFGR = 2u;    /* D1 div = /2 */
    RCC->D2CFGR = 2u;    /* D2 div = /2 */
    RCC->D3CFGR = 2u;    /* D3 div = /2 */

    /* 4. Enable PLL1 and wait */
    RCC->CR |= RCC_CR_PLL1ON;
    while ((RCC->CR & RCC_CR_PLL1RDY) == 0u) { /* spin */ }

    /* 5. Switch SYSCLK to PLL1 */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SWS_MASK) | RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != (RCC_CFGR_SW_PLL1 << 3u)) { /* spin */ }

    /* 6. Update DWT cycles-per-us for delay loop (HCLK = 240 MHz) */
    g_dwt_cycles_per_us = 240u;
}

/* ----------------------------------------------------------------------- */
/*  GPIO pin configuration helpers                                          */
/* ----------------------------------------------------------------------- */

static void gpio_config(gpio_reg_t *port, uint32_t pin,
                        uint32_t mode, uint32_t otype,
                        uint32_t ospeed, uint32_t pupd, uint32_t af) {
    port->MODER   = (port->MODER   & ~(3u << (pin * 2u)))  | (mode  << (pin * 2u));
    port->OTYPER  = (port->OTYPER  & ~(1u << pin))         | (otype << pin);
    port->OSPEEDR = (port->OSPEEDR & ~(3u << (pin * 2u))) | (ospeed << (pin * 2u));
    port->PUPDR   = (port->PUPDR   & ~(3u << (pin * 2u))) | (pupd  << (pin * 2u));
    if (pin < 8u) {
        port->AFRL = (port->AFRL & ~(0xFu << (pin * 4u))) | (af << (pin * 4u));
    } else {
        port->AFRH = (port->AFRH & ~(0xFu << ((pin - 8u) * 4u))) | (af << ((pin - 8u) * 4u));
    }
}

/* Enable the clock to a GPIO port.
 * On STM32H7 the GPIO ports are on AHB4 (RCC AHB4ENR, offset not mapped in our
 * minimal RCC struct above; we access it as a raw register for correctness. */
#define RCC_AHB4ENR_ADDR   (RCC_BASE + 0x0E0u)
static inline void gpio_port_clk_enable(gpio_reg_t *port) {
    volatile uint32_t *enr = (volatile uint32_t *)RCC_AHB4ENR_ADDR;
    if      (port == GPIOA) *enr |= (1u << 0);
    else if (port == GPIOB) *enr |= (1u << 1);
    else if (port == GPIOC) *enr |= (1u << 2);
    else if (port == GPIOD) *enr |= (1u << 3);
    else if (port == GPIOE) *enr |= (1u << 4);
}

/* ----------------------------------------------------------------------- */
/*  Pinmux                                                                  */
/* ----------------------------------------------------------------------- */

void board_gpio_init(void) {
    gpio_port_clk_enable(GPIOA);
    gpio_port_clk_enable(GPIOB);
    gpio_port_clk_enable(GPIOC);
    gpio_port_clk_enable(GPIOD);
    gpio_port_clk_enable(GPIOE);

    /* FPGA SPI1: PA5 CK, PA6 MISO, PB5 MOSI, PB4 CS, AF5 */
    gpio_config(GPIOA, 5u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 5u);
    gpio_config(GPIOA, 6u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 5u);
    gpio_config(GPIOB, 5u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 5u);
    /* CS is manual GPIO */
    gpio_config(GPIOB, 4u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 0u);
    GPIOB->BSRR = (1u << FPGA_CS_PIN);     /* deassert CS (set high) */

    /* FPGA control: IRQ, CDONE (input), CRESET (output) */
    gpio_config(GPIOC, 4u,  GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_DOWN, 0u);
    gpio_config(GPIOC, 5u,  GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_DOWN, 0u);
    gpio_config(GPIOC, 13u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_MED, 0, 0u);
    GPIOC->BSRR = (1u << FPGA_CRESET_PIN); /* hold FPGA in reset */

    /* BLE UART (USART3): PC10 TX, PC11 RX, PC12 CTS, PD2 RTS, AF7 */
    gpio_config(GPIOC, 10u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO_PUPD_NONE, 7u);
    gpio_config(GPIOC, 11u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO_PUPD_NONE, 7u);
    gpio_config(GPIOC, 12u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO_PUPD_NONE, 7u);
    gpio_config(GPIOD, 2u,  GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO_PUPD_NONE, 7u);
    /* BLE reset (PC14) */
    gpio_config(GPIOC, 14u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, 0, 0u);
    GPIOC->BRR = (1u << 14u);   /* assert nRESET (low) for now */

    /* MicroSD (SDMMC1): PC8 CK, PC9 CMD, PC6 D0, PC7 D1, PB6 D2, PB7 D3 */
    gpio_config(GPIOC, 8u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 12u);
    gpio_config(GPIOC, 9u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 12u);
    gpio_config(GPIOC, 6u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 12u);
    gpio_config(GPIOC, 7u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 12u);
    gpio_config(GPIOB, 6u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 12u);
    gpio_config(GPIOB, 7u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 12u);
    /* card detect PB15 (input pull-up) */
    gpio_config(GPIOB, 15u, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_UP, 0u);

    /* OLED I2C1: PB8 SCL, PB9 SDA, AF4 */
    gpio_config(GPIOB, 8u, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_UP, 4u);
    gpio_config(GPIOB, 9u, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_UP, 4u);

    /* SPI NOR flash SPI2: PB13 SCK, PB14 MISO, PB2 MOSI, PB1 CS, AF5 */
    gpio_config(GPIOB, 13u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 5u);
    gpio_config(GPIOB, 14u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 5u);
    gpio_config(GPIOB, 2u,  GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_NONE, 5u);
    gpio_config(GPIOB, 1u,  GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH, GPIO_PUPD_UP, 0u);
    GPIOB->BSRR = (1u << NOR_CS_PIN);

    /* USB-C CDC (USB1 OTG FS): PA11 DM, PA12 DP, AF10 */
    gpio_config(GPIOA, 11u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO_PUPD_NONE, 10u);
    gpio_config(GPIOA, 12u, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO_PUPD_NONE, 10u);

    /* Clamp front-end control (GPIOD) */
    gpio_config(GPIOD, 3u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, 0, 0u);  /* gain */
    gpio_config(GPIOD, 4u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, 0, 0u);  /* coupling */
    gpio_config(GPIOD, 5u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, 0, 0u);  /* inject en */
    gpio_config(GPIOD, 6u, GPIO_MODE_INPUT,  0, 0, GPIO_PUPD_DOWN, 0u);               /* jaw closed */
    gpio_config(GPIOD, 7u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, 0, 0u);  /* TDR discharge */
    GPIOD->BRR = (1u << 5u);  /* inject disabled */
    GPIOD->BRR = (1u << 7u);  /* TDR path safe */

    /* Buttons (PE0, PE1) — input pull-up, active low */
    gpio_config(GPIOE, 0u, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_UP, 0u);
    gpio_config(GPIOE, 1u, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_UP, 0u);

    /* Status LEDs (PE2 red, PE3 green) — output, default off */
    gpio_config(GPIOE, 2u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, 0, 0u);
    gpio_config(GPIOE, 3u, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, 0, 0u);
    GPIOE->BRR = (1u << 2u) | (1u << 3u);

    /* Fuel gauge / charger I2C4: PD12 SCL, PD13 SDA, AF4 */
    gpio_config(GPIOD, 12u, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_UP, 4u);
    gpio_config(GPIOD, 13u, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH, GPIO_PUPD_UP, 4u);
}

/* ----------------------------------------------------------------------- */
/*  NVIC priority config                                                    */
/* ----------------------------------------------------------------------- */

void board_nvic_init(void) {
    /* Set all priorities to default 0; per-IRQ tuning can be added later.
     * Enable FPU: CPACR CP10/CP11 full access.
     */
    SCB_CPACR |= (0xFu << 20u);   /* CP10+CP11 full access */
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");

    /* Set SysTick priority to lowest (15) */
    REG8(NVIC_IPR0 + (IRQ_EXTI0 & 0xFF)) = 0u;  /* placeholder */
}

/* ----------------------------------------------------------------------- */
/*  SysTick — 1 kHz                                                         */
/* ----------------------------------------------------------------------- */

void board_systick_init(void) {
    SYST_RVR = (BOARD_HCLK_MHZ * 1000u) - 1u;   /* 240 MHz -> 240000 cycles */
    SYST_CVR = 0u;
    SYST_CSR = 0x7u;    /* CLKSOURCE=processor, TICKINT=1, ENABLE=1 */
}

/* ----------------------------------------------------------------------- */
/*  DMA                                                                     */
/* ----------------------------------------------------------------------- */

void board_dma_init(void) {
    /* DMA1/DMA2 clocks on AHB1 (RCC AHB1ENR, offset not in minimal struct) */
    volatile uint32_t *ahb1enr = (volatile uint32_t *)(RCC_BASE + 0x0D8u);
    *ahb1enr |= (1u << 0);  /* DMA1EN */
    *ahb1enr |= (1u << 1);  /* DMA2EN */
}

/* ----------------------------------------------------------------------- */
/*  Board init (top level)                                                  */
/* ----------------------------------------------------------------------- */

void board_init(void) {
    /* FPU + cache enable first */
    SCB_CPACR |= (0xFu << 20u);
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");

    /* Enable DWT cycle counter */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
    DWT_CYCCNT = 0u;

    board_clock_init();
    board_gpio_init();
    board_dma_init();
    board_nvic_init();
    board_systick_init();
}

/* ----------------------------------------------------------------------- */
/*  Board-level clamp helpers                                               */
/* ----------------------------------------------------------------------- */

void clamp_set_coupling(clamp_coupling_t c) {
    /* 2-bit value on PD4 */
    uint32_t v = (uint32_t)c;
    GPIOD->ODR = (GPIOD->ODR & ~(3u << CLAMP_COUPLING_PIN)) | ((v & 3u) << CLAMP_COUPLING_PIN);
}

void clamp_set_gain(clamp_gain_t g) {
    /* 2-bit value on PD3 */
    uint32_t v = (uint32_t)g;
    GPIOD->ODR = (GPIOD->ODR & ~(3u << CLAMP_GAIN_SEL_PIN)) | ((v & 3u) << CLAMP_GAIN_SEL_PIN);
}

void clamp_inject_enable(int on) {
    if (on) {
        GPIOD->BSRR = (1u << CLAMP_INJECT_EN_PIN);
    } else {
        GPIOD->BRR  = (1u << CLAMP_INJECT_EN_PIN);
    }
}

int clamp_jaw_closed(void) {
    return (int)((GPIOD->IDR >> CLAMP_JAW_CLOSED_PIN) & 1u);
}

void clamp_tdr_discharge(void) {
    /* Pulse the discharge gate to bleed the TDR path before arming */
    GPIOD->BSRR = (1u << CLAMP_TDR_DISCHARGE_PIN);
    board_delay_us(20);
    GPIOD->BRR  = (1u << CLAMP_TDR_DISCHARGE_PIN);
}

void led_set(int red, int green) {
    if (red)   GPIOE->BSRR = (1u << LED_R_PIN);
    else      GPIOE->BRR  = (1u << LED_R_PIN);
    if (green) GPIOE->BSRR = (1u << LED_G_PIN);
    else      GPIOE->BRR  = (1u << LED_G_PIN);
}