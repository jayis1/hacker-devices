/*
 * main.c — RF Transceiver Tool Main Entry Point
 *
 * Initializes all hardware peripherals and runs the main application
 * loop handling USB CDC commands for RF transceiver control.
 *
 * Hardware: STM32F401CCU6
 *   - SPI1: CC1101 Sub-GHz transceiver
 *   - SPI2: nRF24L01+ 2.4 GHz transceiver
 *   - I2C1: SSD1306 OLED display
 *   - USB OTG FS: CDC virtual serial port
 *   - GPIO: LEDs, buttons
 */

#include "board.h"
#include "registers.h"

/* Forward declarations */
static void system_clock_config(void);
static void gpio_init(void);
static void spi1_init(void);
static void spi2_init(void);
static void i2c1_init(void);
static void usb_init(void);
static void systick_init(void);
static void button_debounce_init(void);

/* Global state */
static volatile uint32_t systick_ms = 0;
static volatile uint8_t btn1_debounced = 1;  /* Active-low, default high (released) */
static volatile uint8_t btn2_debounced = 1;
static volatile uint32_t btn1_last_change = 0;
static volatile uint32_t btn2_last_change = 0;

/* Application mode */
typedef enum {
    MODE_IDLE = 0,
    MODE_SUBGHZ_RX,
    MODE_SUBGHZ_TX,
    MODE_SUBGHZ_SCAN,
    MODE_NRF24_RX,
    MODE_NRF24_TX,
    MODE_NRF24_SCAN,
    MODE_SUBGHZ_SNIFF,
    MODE_NRF24_SNIFF,
} app_mode_t;

static volatile app_mode_t current_mode = MODE_IDLE;

/* USB RX/TX ring buffers */
static volatile uint8_t usb_rx_buf[USB_RX_BUFFER_SIZE];
static volatile uint32_t usb_rx_head = 0;
static volatile uint32_t usb_rx_tail = 0;
static volatile uint8_t usb_tx_buf[USB_TX_BUFFER_SIZE];
static volatile uint32_t usb_tx_len = 0;

/* ========================================================================
 * SysTick Handler — 1ms tick
 * ======================================================================== */
void SysTick_Handler(void)
{
    systick_ms++;
    
    /* Button debounce (10ms stable required) */
    uint8_t btn1_raw = BTN1_PRESSED() ? 0 : 1;
    uint8_t btn2_raw = BTN2_PRESSED() ? 0 : 1;
    
    if (btn1_raw != btn1_debounced) {
        if ((systick_ms - btn1_last_change) > 10) {
            btn1_debounced = btn1_raw;
        }
        btn1_last_change = systick_ms;
    }
    
    if (btn2_raw != btn2_debounced) {
        if ((systick_ms - btn2_last_change) > 10) {
            btn2_debounced = btn2_raw;
        }
        btn2_last_change = systick_ms;
    }
    
    /* LED blink patterns based on mode */
    static uint32_t led_tick = 0;
    led_tick++;
    
    switch (current_mode) {
    case MODE_IDLE:
        /* Both LEDs off */
        LED1_OFF();
        LED2_OFF();
        break;
    case MODE_SUBGHZ_RX:
    case MODE_SUBGHZ_SCAN:
    case MODE_SUBGHZ_SNIFF:
        /* LED1 slow blink (250ms) */
        if (led_tick % 250 == 0) LED1_TOGGLE();
        LED2_OFF();
        break;
    case MODE_SUBGHZ_TX:
        /* LED1 solid on */
        LED1_ON();
        LED2_OFF();
        break;
    case MODE_NRF24_RX:
    case MODE_NRF24_SCAN:
    case MODE_NRF24_SNIFF:
        /* LED2 slow blink (250ms) */
        LED1_OFF();
        if (led_tick % 250 == 0) LED2_TOGGLE();
        break;
    case MODE_NRF24_TX:
        /* LED2 solid on */
        LED1_OFF();
        LED2_ON();
        break;
    }
}

/* ========================================================================
 * Delay using SysTick
 * ======================================================================== */
static void delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms) {
        /* Busy wait */
    }
}

static void delay_us(uint32_t us)
{
    /* Approximate microsecond delay using SysTick or NOP loop */
    /* At 84 MHz, 84 NOP cycles ≈ 1 µs */
    volatile uint32_t count = us * 21; /* 84/4 = 21 iterations per µs */
    while (count--) {
        __asm volatile("nop");
    }
}

/* ========================================================================
 * System Clock Configuration
 * ======================================================================== */
static void system_clock_config(void)
{
    /* Enable power interface clock */
    RCC_APB1ENR |= RCC_APB1ENR_PWREN;
    
    /* Set voltage regulator to scale 2 for 84 MHz */
    PWR_CR &= ~PWR_CR_VOS_MASK;
    PWR_CR |= PWR_CR_VOS_SCALE_2;
    while (!(PWR_CSR & PWR_CSR_VOSRDY))
        ;
    
    /* Enable HSE */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY))
        ;
    
    /* Configure Flash: 2 wait states, prefetch, I-cache, D-cache */
    FLASH_ACR = (2U << FLASH_ACR_LATENCY_SHIFT) | FLASH_ACR_PRFTEN
              | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    
    /* Configure PLL: HSE/8 * 336 / 4 = 84 MHz SYSCLK, 48 MHz USB */
    RCC_PLLCFGR = (8U << RCC_PLLCFGR_PLLM_SHIFT)     /* PLLM = 8 */
                | (336U << RCC_PLLCFGR_PLLN_SHIFT)     /* PLLN = 336 */
                | (0U << RCC_PLLCFGR_PLLP_SHIFT)       /* PLLP = 2 (0=2, 1=4, 2=6, 3=8) */
                | RCC_PLLCFGR_PLLSRC_HSE               /* PLL source = HSE */
                | (7U << RCC_PLLCFGR_PLLQ_SHIFT);      /* PLLQ = 7 */
    
    /* Enable PLL */
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY))
        ;
    
    /* Set AHB/APB prescalers before switching clock */
    RCC_CFGR &= ~RCC_CFGR_HPRE_MASK;
    RCC_CFGR |= RCC_CFGR_HPRE_DIV1;      /* AHB = SYSCLK / 1 = 84 MHz */
    
    RCC_CFGR &= ~RCC_CFGR_PPRE1_MASK;
    RCC_CFGR |= RCC_CFGR_PPRE1_DIV2;      /* APB1 = 42 MHz */
    
    RCC_CFGR &= ~RCC_CFGR_PPRE2_MASK;
    RCC_CFGR |= RCC_CFGR_PPRE2_DIV1;      /* APB2 = 84 MHz */
    
    /* Switch SYSCLK to PLL */
    RCC_CFGR &= ~RCC_CFGR_SW_MASK;
    RCC_CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL)
        ;
}

/* ========================================================================
 * GPIO Initialization
 * ======================================================================== */
static void gpio_init(void)
{
    /* Enable GPIO clocks */
    RCC_AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    
    /* --- PA0: BTN1 input, pull-up --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (0 * 2));       /* Input */
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3U << (0 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1U << (0 * 2));          /* Pull-up */
    
    /* --- PA1: BTN2 input, pull-up --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (1 * 2));
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3U << (1 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1U << (1 * 2));
    
    /* --- PA3: CC1101 RESETn, output, push-pull, high speed --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (3 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (1U << (3 * 2));         /* Output */
    GPIOx_OTYPER(GPIOA_BASE) &= ~(1U << 3);              /* Push-pull */
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3U << (3 * 2));        /* Very high speed */
    GPIOx_ODR(GPIOA_BASE) |= (1U << 3);                   /* High (not in reset) */
    
    /* --- PA4: CC1101 CSN, output, push-pull, high speed (bit-banged) --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (4 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (1U << (4 * 2));          /* Output */
    GPIOx_OTYPER(GPIOA_BASE) &= ~(1U << 4);
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3U << (4 * 2));
    GPIOx_ODR(GPIOA_BASE) |= (1U << 4);                   /* High (deselected) */
    
    /* --- PA5: SPI1_SCK, AF5 --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (5 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2U << (5 * 2));          /* AF mode */
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xFU << (5 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (5U << (5 * 4));           /* AF5 = SPI1 */
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3U << (5 * 2));
    
    /* --- PA6: SPI1_MISO, AF5, pull-up --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (6 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2U << (6 * 2));
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xFU << (6 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (5U << (6 * 4));
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3U << (6 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1U << (6 * 2));
    
    /* --- PA7: SPI1_MOSI, AF5 --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (7 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2U << (7 * 2));
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xFU << (7 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (5U << (7 * 4));
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3U << (7 * 2));
    
    /* --- PA8: CC1101 GDO0, input (default) --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (8 * 2));        /* Input */
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3U << (8 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1U << (8 * 2));          /* Pull-up */
    
    /* --- PA9: CC1101 GDO2, input --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (9 * 2));
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3U << (9 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1U << (9 * 2));
    
    /* --- PA11: USB_DM, AF10 --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (11 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2U << (11 * 2));
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xFU << ((11) * 4));
    GPIOx_AFRH(GPIOA_BASE) &= ~(0xFU << ((11 - 8) * 4));
    GPIOx_AFRH(GPIOA_BASE) |= (10U << ((11 - 8) * 4));   /* AF10 = USB */
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3U << (11 * 2));
    
    /* --- PA12: USB_DP, AF10 --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (12 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2U << (12 * 2));
    GPIOx_AFRH(GPIOA_BASE) &= ~(0xFU << ((12 - 8) * 4));
    GPIOx_AFRH(GPIOA_BASE) |= (10U << ((12 - 8) * 4));
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3U << (12 * 2));
    
    /* --- PA15: nRF24L01+ CE, output, push-pull --- */
    GPIOx_MODER(GPIOA_BASE) &= ~(3U << (15 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (1U << (15 * 2));         /* Output */
    GPIOx_OTYPER(GPIOA_BASE) &= ~(1U << 15);
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3U << (15 * 2));
    GPIOx_ODR(GPIOA_BASE) &= ~(1U << 15);                 /* Low (disabled) */
    
    /* --- PB0: LED1, output, push-pull --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (0 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (1U << (0 * 2));
    GPIOx_OTYPER(GPIOB_BASE) &= ~(1U << 0);
    GPIOx_ODR(GPIOB_BASE) &= ~(1U << 0);                  /* Off */
    
    /* --- PB1: LED2, output, push-pull --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (1 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (1U << (1 * 2));
    GPIOx_OTYPER(GPIOB_BASE) &= ~(1U << 1);
    GPIOx_ODR(GPIOB_BASE) &= ~(1U << 1);                  /* Off */
    
    /* --- PB3: nRF24L01+ IRQ, input, pull-up --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (3 * 2));
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3U << (3 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1U << (3 * 2));
    
    /* --- PB6: I2C1_SCL, AF4, open-drain, pull-up --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (6 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2U << (6 * 2));
    GPIOx_AFRL(GPIOB_BASE) &= ~(0xFU << (6 * 4));
    GPIOx_AFRL(GPIOB_BASE) |= (4U << (6 * 4));            /* AF4 = I2C1 */
    GPIOx_OTYPER(GPIOB_BASE) |= (1U << 6);                 /* Open-drain */
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3U << (6 * 2));
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3U << (6 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1U << (6 * 2));
    
    /* --- PB7: I2C1_SDA, AF4, open-drain, pull-up --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (7 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2U << (7 * 2));
    GPIOx_AFRL(GPIOB_BASE) &= ~(0xFU << (7 * 4));
    GPIOx_AFRL(GPIOB_BASE) |= (4U << (7 * 4));
    GPIOx_OTYPER(GPIOB_BASE) |= (1U << 7);
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3U << (7 * 2));
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3U << (7 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1U << (7 * 2));
    
    /* --- PB12: nRF24L01+ CSN, output, push-pull, high speed --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (12 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (1U << (12 * 2));
    GPIOx_OTYPER(GPIOB_BASE) &= ~(1U << 12);
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3U << (12 * 2));
    GPIOx_ODR(GPIOB_BASE) |= (1U << 12);                  /* High (deselected) */
    
    /* --- PB13: SPI2_SCK, AF5 --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (13 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2U << (13 * 2));
    GPIOx_AFRH(GPIOB_BASE) &= ~(0xFU << ((13 - 8) * 4));
    GPIOx_AFRH(GPIOB_BASE) |= (5U << ((13 - 8) * 4));    /* AF5 = SPI2 */
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3U << (13 * 2));
    
    /* --- PB14: SPI2_MISO, AF5, pull-up --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (14 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2U << (14 * 2));
    GPIOx_AFRH(GPIOB_BASE) &= ~(0xFU << ((14 - 8) * 4));
    GPIOx_AFRH(GPIOB_BASE) |= (5U << ((14 - 8) * 4));
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3U << (14 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1U << (14 * 2));
    
    /* --- PB15: SPI2_MOSI, AF5 --- */
    GPIOx_MODER(GPIOB_BASE) &= ~(3U << (15 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2U << (15 * 2));
    GPIOx_AFRH(GPIOB_BASE) &= ~(0xFU << ((15 - 8) * 4));
    GPIOx_AFRH(GPIOB_BASE) |= (5U << ((15 - 8) * 4));
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3U << (15 * 2));
}

/* ========================================================================
 * SPI1 Initialization (CC1101, APB2 = 84 MHz)
 * ======================================================================== */
static void spi1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;
    
    /* Disable SPI1 first */
    SPIx_CR1(SPI1_BASE) &= ~SPI_CR1_SPE;
    
    /* Configure SPI1: Master, Mode 0, MSB-first, 8-bit, software NSS, /8 prescaler */
    /* APB2 = 84 MHz, /8 = 10.5 MHz (within CC1101 10 MHz spec) */
    SPIx_CR1(SPI1_BASE) = SPI_CR1_MSTR     /* Master mode */
                         | SPI_CR1_SSM       /* Software NSS management */
                         | SPI_CR1_SSI       /* Internal NSS = high */
                         | (2U << SPI_CR1_BR_SHIFT);  /* BR = /8 */
    
    SPIx_CR2(SPI1_BASE) = 0;  /* No DMA, no NSS pulse */
    
    /* Enable SPI1 */
    SPIx_CR1(SPI1_BASE) |= SPI_CR1_SPE;
}

/* ========================================================================
 * SPI2 Initialization (nRF24L01+, APB1 = 42 MHz)
 * ======================================================================== */
static void spi2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_SPI2EN;
    
    /* Disable SPI2 first */
    SPIx_CR2(SPI2_BASE) &= ~SPI_CR1_SPE;
    
    /* Configure SPI2: Master, Mode 0, MSB-first, 8-bit, software NSS, /4 prescaler */
    /* APB1 = 42 MHz, /4 = 10.5 MHz (within nRF24L01+ 10 MHz spec) */
    SPIx_CR1(SPI2_BASE) = SPI_CR1_MSTR
                         | SPI_CR1_SSM
                         | SPI_CR1_SSI
                         | (1U << SPI_CR1_BR_SHIFT);  /* BR = /4 */
    
    SPIx_CR2(SPI2_BASE) = 0;
    
    /* Enable SPI2 */
    SPIx_CR1(SPI2_BASE) |= SPI_CR1_SPE;
}

/* ========================================================================
 * I2C1 Initialization (SSD1306, APB1 = 42 MHz, Fast Mode 400 kHz)
 * ======================================================================== */
static void i2c1_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
    
    /* Disable I2C1 to configure */
    I2Cx_CR1(I2C1_BASE) &= ~I2C_CR1_PE;
    
    /* Set peripheral clock frequency (must match APB1 = 42 MHz) */
    I2Cx_CR2(I2C1_BASE) = 42U;  /* 42 MHz */
    
    /* Configure CCR for 400 kHz fast mode:
     * CCR = Freq / (2 * SCL) = 42 / (2 * 0.4) = 52.5 → use 52 */
    I2Cx_CCR(I2C1_BASE) = I2C_CCR_FS | 52U;
    
    /* Maximum rise time: 300 ns → TRISE = 42 MHz * 0.3 µs + 1 = 13 */
    I2Cx_TRISE(I2C1_BASE) = 13U;
    
    /* Enable I2C1 */
    I2Cx_CR1(I2C1_BASE) |= I2C_CR1_PE | I2C_CR1_ACK;
}

/* ========================================================================
 * SysTick Initialization (1ms tick at 84 MHz)
 * ======================================================================== */
static void systick_init(void)
{
    /* SysTick: 84 MHz / 84000 = 1 kHz (1 ms) */
    SYSTICK_RVR = 84000U - 1U;
    SYSTICK_CVR = 0U;
    SYSTICK_CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;
}

/* ========================================================================
 * Simple USB CDC stub (full USB OTG FS implementation requires
 * significant code; this provides the framework for the CDC class)
 * ======================================================================== */
static void usb_init(void)
{
    /* Enable USB OTG FS clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    
    /* Full USB CDC initialization would go here:
     * 1. Configure OTG FS global registers
     * 2. Configure device mode (DCFG)
     * 3. Set USB address to 0
     * 4. Enable endpoints (EP0 control, EP1 CDC data, EP2 CDC notify)
     * 5. Enable interrupts
     * 6. Connect to host (D+ pull-up)
     *
     * This is implemented via the usb_descriptors.h and a
     * lightweight USB stack in the companion library.
     */
}

/* ========================================================================
 * Main Application Loop
 * ======================================================================== */
int main(void)
{
    /* Step 1: Configure system clocks (HSE → PLL → 84 MHz SYSCLK) */
    system_clock_config();
    
    /* Step 2: Initialize GPIOs */
    gpio_init();
    
    /* Step 3: Initialize SPI1 (CC1101) */
    spi1_init();
    
    /* Step 4: Initialize SPI2 (nRF24L01+) */
    spi2_init();
    
    /* Step 5: Initialize I2C1 (SSD1306) */
    i2c1_init();
    
    /* Step 6: Initialize SysTick (1ms tick) */
    systick_init();
    
    /* Step 7: Initialize USB CDC */
    usb_init();
    
    /* Step 8: Reset CC1101 and nRF24L01+ to known state */
    /* CC1101 hardware reset pulse */
    CC1101_RESET_HIGH();
    delay_ms(1);
    CC1101_RESET_LOW();
    delay_us(50);
    CC1101_RESET_HIGH();
    delay_ms(1);
    
    /* nRF24L01+ power-on reset: CE low, CSN high */
    NRF24_CE_LOW();
    NRF24_CSN_HIGH();
    
    /* Turn off both LEDs briefly to indicate boot */
    LED1_ON();
    LED2_ON();
    delay_ms(200);
    LED1_OFF();
    LED2_OFF();
    
    /* Main application loop */
    while (1) {
        /* Handle button presses */
        static uint8_t prev_btn1 = 1, prev_btn2 = 1;
        uint8_t cur_btn1 = btn1_debounced;
        uint8_t cur_btn2 = btn2_debounced;
        
        /* BTN1: Cycle mode */
        if (cur_btn1 == 0 && prev_btn1 == 1) {
            /* Button was just pressed */
            current_mode = (app_mode_t)(((int)current_mode + 1) % 9);
        }
        
        /* BTN2: Execute action (start RX/TX/scan) */
        if (cur_btn2 == 0 && prev_btn2 == 1) {
            /* Toggle active/idle within current mode */
            if (current_mode != MODE_IDLE) {
                /* For now, just toggle LED to acknowledge */
                LED1_TOGGLE();
                LED2_TOGGLE();
                delay_ms(100);
                LED1_TOGGLE();
                LED2_TOGGLE();
            }
        }
        
        prev_btn1 = cur_btn1;
        prev_btn2 = cur_btn2;
        
        /* Process USB commands (simplified loop) */
        /* In production, the USB ISR fills usb_rx_buf and the
         * main loop parses commands based on the protocol defined
         * in app/utils/protocol.js */
        
        /* Small delay to prevent busy loop */
        delay_ms(10);
    }
    
    return 0;  /* Never reached */
}