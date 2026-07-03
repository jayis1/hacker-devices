/*
 * drivers/board_init.c — clock/power/GPIO/peripheral bring-up
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Brings the STM32H743 from reset state to a sane 480 MHz core clock,
 * enables all AHB/APB clocks, configures every GPIO used by RadarPhantom,
 * and starts the SysTick 1 ms timer.
 */
#include "../board.h"
#include "../registers.h"

/* External linker symbols for interrupt vector + data section */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* ---- Reset handler: C entry from startup --------------------------- */
void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;

    SCB_VTOR = (uint32_t)&_estack; /* reassign if needed; placeholder */

    board_init();
    main();   /* never returns */
    while (1) { }
}

/* ---- Dummy handlers ------------------------------------------------ */
void NMI_Handler(void)        { while (1) { } }
void HardFault_Handler(void)  { while (1) { } }
void MemManage_Handler(void)  { while (1) { } }
void BusFault_Handler(void)   { while (1) { } }
void UsageFault_Handler(void){ while (1) { } }
void Default_Handler(void)    { while (1) { } }

/* ---- SysTick ------------------------------------------------------- */
static volatile uint32_t ms_ticks;

void SysTick_Handler(void)
{
    ms_ticks++;
}

uint32_t board_millis(void) { return ms_ticks; }

void board_delay_ms(uint32_t ms)
{
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms) { }
}

/* Busy-wait microsecond delay (approximate, calibrated to 480 MHz core).
 * Loops ~120 cycles per iteration at -O2; 480 cycles/us -> 4 iters/us.
 * Used only for short PLL latch pulses. */
void board_delay_us(uint32_t us)
{
    volatile uint32_t n = us * 4;
    while (n--) { __asm__ volatile ("nop"); }
}

/* ---- Clock tree ---------------------------------------------------- */
static void clocks_init(void)
{
    /* enable FPU (CP10/CP11 full access) */
    SCB_CPACR |= (0xFu << 20);

    /* enable HSE and wait for it */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY)) { }

    /* configure PLL1: HSE / 5 * 96 / 2 = 480 MHz
     *  Ref: 25 MHz / 5 = 5 MHz
     *  VCO: 5 * 96 = 480 MHz
     *  SYS: 480 / 2 = 240 -> actually use /1 for 480
     */
    RCC_PLL1CFGR = (5u << 4)       /* DIVM1 = 5 */
                 | (96u << 8)      /* DIVN1 = 96 */
                 | (0u << 25)      /* DIVP1 = 1 */
                 | (1u << 24)      /* ENABLE_P1 */
                 | (1u << 16);     /* R output enabled */
    RCC_CR |= RCC_CR_PLL1ON;
    while (!(RCC_CR & RCC_CR_PLL1RDY)) { }

    /* set flash latency for 480 MHz (not shown: FLASH_ACR) */
    /* switch system clock to PLL1 */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;
    while (((RCC_CFGR >> 3) & 0x7) != 3) { } /* wait SWS */

    /* enable peripheral clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOB | RCC_AHB4ENR_GPIOC |
                   RCC_AHB4ENR_GPIOD | RCC_AHB4ENR_GPIOE |
                   RCC_AHB4ENR_GPIOF | RCC_AHB4ENR_GPIOG;
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA;
    RCC_APB1ENR |= RCC_APB1ENR_SPI2 | RCC_APB1ENR_SPI3 |
                   RCC_APB1ENR_USART2 | RCC_APB1ENR_USART3;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1 | RCC_APB2ENR_SPI4;
}

/* ---- GPIO configuration ------------------------------------------- */
static void gpio_pin_config(uint32_t base, uint8_t pin, uint8_t mode,
                            uint8_t pupd, uint8_t speed, uint8_t af)
{
    uint32_t m = GPIO_MODER(base);
    m &= ~(3u << (pin * 2));
    m |= (mode << (pin * 2));
    GPIO_MODER(base) = m;

    uint32_t p = GPIO_PUPDR(base);
    p &= ~(3u << (pin * 2));
    p |= (pupd << (pin * 2));
    GPIO_PUPDR(base) = p;

    uint32_t s = GPIO_OSPEEDR(base);
    s &= ~(3u << (pin * 2));
    s |= (speed << (pin * 2));
    GPIO_OSPEEDR(base) = s;

    if (af) {
        if (pin < 8) {
            uint32_t v = GPIO_AFRL(base);
            v &= ~(0xFu << (pin * 4));
            v |= (af << (pin * 4));
            GPIO_AFRL(base) = v;
        } else {
            uint32_t v = GPIO_AFRH(base);
            v &= ~(0xFu << ((pin - 8) * 4));
            v |= (af << ((pin - 8) * 4));
            GPIO_AFRH(base) = v;
        }
    }
}

static void gpio_init_all(void)
{
    /* LEDs: PB0/PB1/PB2 output push-pull */
    gpio_pin_config(GPIOB_BASE, 0, GPIO_MODE_OUT, GPIO_PUPD_NONE, 3, 0);
    gpio_pin_config(GPIOB_BASE, 1, GPIO_MODE_OUT, GPIO_PUPD_NONE, 3, 0);
    gpio_pin_config(GPIOB_BASE, 2, GPIO_MODE_OUT, GPIO_PUPD_NONE, 3, 0);

    /* OLED SPI1: SCK=PB3, MISO=PB4, MOSI=PB5 (AF5) */
    gpio_pin_config(GPIOB_BASE, 3, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI1);
    gpio_pin_config(GPIOB_BASE, 4, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI1);
    gpio_pin_config(GPIOB_BASE, 5, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI1);
    /* OLED ctrl: CS=PA4, DC=PA5, RST=PA6 */
    gpio_pin_config(GPIOA_BASE, 4, GPIO_MODE_OUT, GPIO_PUPD_PU, 1, 0);
    gpio_pin_config(GPIOA_BASE, 5, GPIO_MODE_OUT, GPIO_PUPD_NONE, 1, 0);
    gpio_pin_config(GPIOA_BASE, 6, GPIO_MODE_OUT, GPIO_PUPD_PU, 1, 0);

    /* Joystick: PA0 analog, PC13 input pull-up (press) */
    gpio_pin_config(GPIOA_BASE, 0, GPIO_MODE_AN, GPIO_PUPD_NONE, 0, 0);
    gpio_pin_config(GPIOC_BASE, 13, 0, GPIO_PUPD_PU, 0, 0);

    /* DRFM FPGA SPI2: SCK=PB13, MISO=PB14, MOSI=PB15 (AF5), NSS=PB12 out */
    gpio_pin_config(GPIOB_BASE, 13, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI2);
    gpio_pin_config(GPIOB_BASE, 14, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI2);
    gpio_pin_config(GPIOB_BASE, 15, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI2);
    gpio_pin_config(GPIOB_BASE, 12, GPIO_MODE_OUT, GPIO_PUPD_PU, 3, 0);
    /* CDONE=PC6 input, CRESET=PC7 output */
    gpio_pin_config(GPIOC_BASE, 6, 0, GPIO_PUPD_NONE, 0, 0);
    gpio_pin_config(GPIOC_BASE, 7, GPIO_MODE_OUT, GPIO_PUPD_PU, 3, 0);

    /* LO PLL SPI3: SCK=PC11, MOSI=PC12 (AF6), NSS=PA15 out, LE=PC10 out */
    gpio_pin_config(GPIOC_BASE, 11, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF6_SPI3);
    gpio_pin_config(GPIOC_BASE, 12, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF6_SPI3);
    gpio_pin_config(GPIOA_BASE, 15, GPIO_MODE_OUT, GPIO_PUPD_PU, 3, 0);
    gpio_pin_config(GPIOC_BASE, 10, GPIO_MODE_OUT, GPIO_PUPD_NONE, 3, 0);
    gpio_pin_config(GPIOD_BASE, 2, GPIO_MODE_OUT, GPIO_PUPD_PU, 3, 0); /* RF_EN */

    /* BLE USART3: TX=PD8, RX=PD9 (AF7) */
    gpio_pin_config(GPIOD_BASE, 8, GPIO_MODE_AF, GPIO_PUPD_PU, 3, GPIO_AF7_USART3);
    gpio_pin_config(GPIOD_BASE, 9, GPIO_MODE_AF, GPIO_PUPD_PU, 3, GPIO_AF7_USART3);

    /* USB-C CDC USART2: TX=PD5, RX=PD6 (AF7) */
    gpio_pin_config(GPIOD_BASE, 5, GPIO_MODE_AF, GPIO_PUPD_PU, 3, GPIO_AF7_USART2);
    gpio_pin_config(GPIOD_BASE, 6, GPIO_MODE_AF, GPIO_PUPD_PU, 3, GPIO_AF7_USART2);

    /* SD SPI4: SCK=PE2, MISO=PE5, MOSI=PE6 (AF5), CS=PE4 out, CD=PE3 in */
    gpio_pin_config(GPIOE_BASE, 2, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI1);
    gpio_pin_config(GPIOE_BASE, 5, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI1);
    gpio_pin_config(GPIOE_BASE, 6, GPIO_MODE_AF, GPIO_PUPD_NONE, 3, GPIO_AF5_SPI1);
    gpio_pin_config(GPIOE_BASE, 4, GPIO_MODE_OUT, GPIO_PUPD_PU, 3, 0);
    gpio_pin_config(GPIOE_BASE, 3, 0, GPIO_PUPD_PU, 0, 0);

    /* Battery ADC: PF9 analog */
    gpio_pin_config(GPIOF_BASE, 9, GPIO_MODE_AN, GPIO_PUPD_NONE, 0, 0);

    /* Power enables: PG7 (5V), PG8 (RF) */
    gpio_pin_config(GPIOG_BASE, 7, GPIO_MODE_OUT, GPIO_PUPD_NONE, 3, 0);
    gpio_pin_config(GPIOG_BASE, 8, GPIO_MODE_OUT, GPIO_PUPD_NONE, 3, 0);
}

/* ---- SPI generic --------------------------------------------------- */
void spi_init(uint32_t base, uint32_t baud_div, uint8_t cpol, uint8_t cpha)
{
    SPI_CFG1(base) = baud_div | SPI_CFG1_DSIZE8;
    SPI_CFG2(base) = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_SSI;
    if (cpol) SPI_CFG2(base) |= SPI_CFG2_CPOL;
    if (cpha) SPI_CFG2(base) |= SPI_CFG2_CPHA;
    SPI_CR1(base) |= SPI_CR1_SPE;
}

uint8_t spi_xfer(uint32_t base, uint8_t data)
{
    while (!(SPI_SR(base) & SPI_SR_TXP)) { }
    SPI_DR(base) = data;
    while (!(SPI_SR(base) & SPI_SR_EOT)) { }
    uint8_t rx = SPI_DR(base);
    SPI_SR(base) = 0; /* clear flags */
    return rx;
}

void spi_cs_low(uint32_t port, uint8_t pin)  { GPIO_BSRR(port) = GPIO_BR(pin); }
void spi_cs_high(uint32_t port, uint8_t pin) { GPIO_BSRR(port) = GPIO_BS(pin); }

/* ---- USART generic ------------------------------------------------- */
void uart_init(uint32_t base, uint32_t baud)
{
    USART_CR1(base) = 0;
    USART_CR2(base) = 0;
    USART_CR3(base) = 0;
    /* BRR = APB clock / baud (oversample 16) */
    USART_BRR(base) = (APB1_HZ / baud);
    USART_CR1(base) = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;
}

void uart_putc(uint32_t base, uint8_t c)
{
    while (!(USART_ISR(base) & USART_ISR_TXE)) { }
    USART_TDR(base) = c;
}

uint8_t uart_getc_nb(uint32_t base, uint8_t *c)
{
    if (USART_ISR(base) & USART_ISR_RXNE) {
        *c = USART_RDR(base) & 0xFF;
        return 1;
    }
    return 0;
}

/* ---- Watchdog ------------------------------------------------------ */
static void iwdg_init(void)
{
    IWDG_KR = 0xCCCC;     /* start */
    IWDG_KR = 0x5555;     /* enable write */
    IWDG_PR = 4;          /* /256 */
    IWDG_RLR = 0xFFF;     /* max */
    IWDG_KR = 0xAAAA;     /* reload */
}

void board_watchdog_kick(void)
{
    IWDG_KR = 0xAAAA;
}

/* ---- Power rails --------------------------------------------------- */
void board_power_enable_rf(void)
{
    GPIO_BSRR(GPIOD_BASE) = GPIO_BS(2);   /* LO RF_EN high */
    GPIO_BSRR(GPIOG_BASE) = GPIO_BS(7);   /* 5V buck enable */
    board_delay_ms(20);
    GPIO_BSRR(GPIOG_BASE) = GPIO_BS(8);   /* RF rails enable */
    board_delay_ms(50);                   /* let PLL lock */
}

void board_power_disable_rf(void)
{
    GPIO_BSRR(GPIOG_BASE) = GPIO_BR(8);
    GPIO_BSRR(GPIOD_BASE) = GPIO_BR(2);
    GPIO_BSRR(GPIOG_BASE) = GPIO_BR(7);
}

void board_led_set(uint32_t port, uint8_t pin, uint8_t on)
{
    if (on) GPIO_BSRR(port) = GPIO_BS(pin);
    else    GPIO_BSRR(port) = GPIO_BR(pin);
}

/* ---- Public board init --------------------------------------------- */
void board_init(void)
{
    clocks_init();
    gpio_init_all();

    /* SysTick: 1 ms at 480 MHz core / 8 = 60 MHz reload */
    SYST_RVR = (SYSCLK_HZ / 8 / 1000) - 1;
    SYST_CVR = 0;
    SYST_CSR = 0x7; /* clk/8, enable, interrupt */
    NVIC_ISER0 = (1u << 15); /* SysTick is exception #15 */

    iwdg_init();

    /* All LEDs off initially */
    board_led_set(LED_STATUS_PORT, LED_STATUS_PIN, 0);
    board_led_set(LED_ARM_PORT,    LED_ARM_PIN,    0);
    board_led_set(LED_FAULT_PORT,  LED_FAULT_PIN,  0);

    /* Initial SPI peripheral configs:
     * SPI1 (OLED):     div8, mode 0
     * SPI2 (DRFM):     div8, mode 0
     * SPI3 (LO PLL):   div16, mode 0
     * SPI4 (SD):       div256, mode 0
     */
    spi_init(SPI1_BASE, SPI_CFG1_MBR_DIV8, 0, 0);
    spi_init(SPI2_BASE, SPI_CFG1_MBR_DIV8, 0, 0);
    spi_init(SPI3_BASE, SPI_CFG1_MBR_DIV8, 0, 0);
    spi_init(SPI4_BASE, 6u << 28, 0, 0);     /* div256 for slow SD init */

    /* UARTs */
    uart_init(USART3_BASE, BLE_BAUD);
    uart_init(USART2_BASE, USB_BAUD);
}