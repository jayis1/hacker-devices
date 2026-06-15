/*
 * main.c — Infrared Phantom firmware entry point
 * STM32H743VIT6 Cortex-M7 @ 480 MHz
 *
 * IR security research toolkit: capture, decode, modify, replay, fuzz
 */

#include "board.h"
#include "registers.h"
#include "drivers/ir_receiver.h"
#include "drivers/ir_transmitter.h"
#include "drivers/w25q128.h"
#include "drivers/ssd1306.h"

/* Global state */
static volatile uint32_t g_system_ticks = 0;
static volatile uint8_t g_mode = MODE_IDLE;
static volatile uint8_t g_usb_connected = 0;
static volatile uint8_t g_ble_connected = 0;

/* DMA buffers in DTCM (zero-wait-state access) */
static volatile uint16_t g_adc_buffer[ADC_BUF_SIZE / 2];   /* ADC circular buffer */
static volatile uint32_t g_tim_buffer[IR_TIM_BUF_SIZE / 4]; /* TIM capture buffer */

/* Protocol engine state */
static volatile uint32_t g_frames_captured = 0;
static volatile uint32_t g_dma_half_irq = 0;
static volatile uint32_t g_dma_full_irq = 0;

/* Forward declarations */
void SystemInit(void);
void SystemClock_Config(void);
void board_init(void);
void board_gpio_init(void);
void board_peripheral_init(void);
void board_usb_init(void);
void oled_show_status(void);
void cmd_process(uint8_t *buf, uint16_t len);
void hard_fault_handler(void) __attribute__((naked, noreturn));

/* ========================================
 * System initialization
 * ======================================== */

void SystemInit(void) {
    /* Enable FPU */
    SCB->CPACR |= (0xFU << 20);  /* Enable CP10 and CP11 */

    /* Enable POWER and SYSCFG clocks */
    RCC->APB4ENR |= RCC_APB4ENR_PWREN | RCC_APB4ENR_SYSCFGEN;

    /* Configure voltage scale: SCALE1 for 480 MHz */
    PWR->CR3 = (PWR->CR3 & ~(3U << PWR_CR3_VOS_Pos)) | PWR_CR3_VOS_0;

    /* Wait for voltage scaling to complete */
    while (!(PWR->CSR1 & PWR_CSR1_VOSRDY))
        ;

    /* Configure main clocks */
    SystemClock_Config();

    /* Enable instruction and data caches */
    SCB->CCR |= (1U << 16) | (1U << 17);  /* ICACHE, DCACHE enable */

    /* Enable all GPIO clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                     RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                     RCC_AHB4ENR_GPIOEEN;

    /* Configure all GPIO pins */
    board_gpio_init();

    /* Configure all peripherals */
    board_peripheral_init();
}

void SystemClock_Config(void) {
    /* Enable HSE (25 MHz external crystal) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    /* Configure PLL1: HSE=25MHz, M=5, N=192, P=2 → SYSCLK=480MHz
     * VCO_IN = 25/5 = 5 MHz
     * VCO_OUT = 5 * 192 = 960 MHz
     * SYSCLK = 960/2 = 480 MHz
     */
    RCC->PLLCKSELR = (5U << RCC_PLLCKSELR_DIVM1_Pos) |
                      RCC_PLLCKSELR_PLL1SRC_HSE;

    RCC->PLLCFGR = (192U << 0) |   /* PLL1N = 192 */
                    (0U << 8) |     /* PLL1P = 2 */
                    (4U << 16) |    /* PLL1R = 8 */
                    (0U << 24);     /* PLL1Q = 2 */

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY))
        ;

    /* Set flash wait states: 7 WS for 480 MHz at 1.8V */
    FLASH->ACR = (7U << 0) |   /* LATENCY = 7 */
                 (1U << 4) |   /* PRFTEN (prefetch enable) */
                 (1U << 8) |   /* ICEN (instruction cache) */
                 (1U << 9);    /* DCEN (data cache) */

    /* Configure bus prescalers */
    /* AHB = SYSCLK/1 = 480 MHz (HPRE = 0) */
    /* APB2 = SYSCLK/2 = 240 MHz (D1PPRE = 0b100) */
    /* APB1L = SYSCLK/4 = 120 MHz (D2PPRE1 = 0b101) */
    /* APB4 = SYSCLK/4 = 120 MHz (D3PPRE = 0b101) */

    /* Switch system clock to PLL1 */
    RCC->CFGR = (3U << 0);  /* SW = PLL1 */
    while (((RCC->CFGR >> 2) & 0x7) != 3)
        ;
}

void board_gpio_init(void) {
    /* Configure PA0: nRF_INT — input, pull-down */
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->PUPDR &= ~(3U << (0 * 2));
    GPIOA->PUPDR |= (2U << (0 * 2));  /* Pull-down */

    /* Configure PA1: USER_BTN — input, pull-up */
    GPIOA->MODER &= ~(3U << (1 * 2));
    GPIOA->PUPDR &= ~(3U << (1 * 2));
    GPIOA->PUPDR |= (1U << (1 * 2));  /* Pull-up */

    /* Configure PA2: SPI1_MOSI — AF5 */
    GPIOA->MODER &= ~(3U << (2 * 2));
    GPIOA->MODER |= (2U << (2 * 2));
    GPIOA->OSPEEDR |= (3U << (2 * 2));  /* Very high speed */
    GPIOA->AFR[0] &= ~(0xFU << (2 * 4));
    GPIOA->AFR[0] |= (5U << (2 * 4));   /* AF5 = SPI1 */

    /* Configure PA3: SPI1_MISO — AF5 */
    GPIOA->MODER &= ~(3U << (3 * 2));
    GPIOA->MODER |= (2U << (3 * 2));
    GPIOA->AFR[0] &= ~(0xFU << (3 * 4));
    GPIOA->AFR[0] |= (5U << (3 * 4));

    /* Configure PA4: SPI1_NSS — output (manual CS) */
    GPIOA->MODER &= ~(3U << (4 * 2));
    GPIOA->MODER |= (1U << (4 * 2));  /* Output */
    GPIOA->ODR |= (1U << 4);          /* CS high (inactive) */
    GPIOA->OSPEEDR |= (3U << (4 * 2));

    /* Configure PA5: SPI1_SCK — AF5 */
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |= (2U << (5 * 2));
    GPIOA->OSPEEDR |= (3U << (5 * 2));
    GPIOA->AFR[0] &= ~(0xFU << (5 * 4));
    GPIOA->AFR[0] |= (5U << (5 * 4));

    /* Configure PA6: IR_DIGITAL (TIM3 input capture) — AF2 */
    GPIOA->MODER &= ~(3U << (6 * 2));
    GPIOA->MODER |= (2U << (6 * 2));
    GPIOA->AFR[0] &= ~(0xFU << (6 * 4));
    GPIOA->AFR[0] |= (2U << (6 * 4));  /* AF2 = TIM3 */

    /* Configure PA7: IR_ANALOG (ADC1_IN7) — analog */
    GPIOA->MODER &= ~(3U << (7 * 2));
    GPIOA->MODER |= (3U << (7 * 2));  /* Analog mode */

    /* Configure PA9: USART1_TX — AF7 */
    GPIOA->MODER &= ~(3U << (9 * 2));
    GPIOA->MODER |= (2U << (9 * 2));
    GPIOA->OSPEEDR |= (3U << (9 * 2));
    GPIOA->AFR[1] &= ~(0xFU << ((9 - 8) * 4));
    GPIOA->AFR[1] |= (7U << ((9 - 8) * 4));

    /* Configure PA10: USART1_RX — AF7 */
    GPIOA->MODER &= ~(3U << (10 * 2));
    GPIOA->MODER |= (2U << (10 * 2));
    GPIOA->AFR[1] &= ~(0xFU << ((10 - 8) * 4));
    GPIOA->AFR[1] |= (7U << ((10 - 8) * 4));

    /* Configure PD0: nRF_RESET — output, active-low */
    GPIOD->MODER &= ~(3U << (0 * 2));
    GPIOD->MODER |= (1U << (0 * 2));
    GPIOD->ODR |= (1U << 0);  /* High (not in reset) */

    /* Configure PD1: nRF_RTS — output */
    GPIOD->MODER &= ~(3U << (1 * 2));
    GPIOD->MODER |= (1U << (1 * 2));

    /* Configure PD2: IR_PWM (TIM3_CH1 PWM) — AF2 */
    GPIOD->MODER &= ~(3U << (2 * 2));
    GPIOD->MODER |= (2U << (2 * 2));
    GPIOD->OSPEEDR |= (3U << (2 * 2));
    GPIOD->AFR[0] &= ~(0xFU << (2 * 4));
    GPIOD->AFR[0] |= (2U << (2 * 4));

    /* Configure PE0: nRF_CTS — input */
    GPIOE->MODER &= ~(3U << (0 * 2));

    /* Configure PE1: IR_LED_EN — output */
    GPIOE->MODER &= ~(3U << (1 * 2));
    GPIOE->MODER |= (1U << (1 * 2));
    GPIOE->ODR &= ~(1U << 1);  /* Low (LEDs off) */

    /* Configure PE2: LED_STATUS — output */
    GPIOE->MODER &= ~(3U << (2 * 2));
    GPIOE->MODER |= (1U << (2 * 2));
    GPIOE->ODR &= ~(1U << 2);  /* Off */

    /* Configure PE3: LED_ERROR — output */
    GPIOE->MODER &= ~(3U << (3 * 2));
    GPIOE->MODER |= (1U << (3 * 2));
    GPIOE->ODR &= ~(1U << 3);  /* Off */

    /* Configure PE4: IR_PWM_GATE (TIM3_CH2) — AF2 */
    GPIOE->MODER &= ~(3U << (4 * 2));
    GPIOE->MODER |= (2U << (4 * 2));
    GPIOE->OSPEEDR |= (3U << (4 * 2));
    GPIOE->AFR[0] &= ~(0xFU << (4 * 4));
    GPIOE->AFR[0] |= (2U << (4 * 4));

    /* Configure PE5: OLED_CS — output, active-low */
    GPIOE->MODER &= ~(3U << (5 * 2));
    GPIOE->MODER |= (1U << (5 * 2));
    GPIOE->ODR |= (1U << 5);  /* CS high (inactive) */

    /* Configure PB6: OLED_RST — output */
    GPIOB->MODER &= ~(3U << (6 * 2));
    GPIOB->MODER |= (1U << (6 * 2));
    GPIOB->ODR |= (1U << 6);  /* High (not in reset) */

    /* Configure PB7: OLED_DC — output */
    GPIOB->MODER &= ~(3U << (7 * 2));
    GPIOB->MODER |= (1U << (7 * 2));
}

void board_peripheral_init(void) {
    /* Enable peripheral clocks */
    RCC->APB1LENR |= RCC_APB1LENR_TIM2EN | RCC_APB1LENR_TIM3EN |
                      RCC_APB1LENR_SPI3EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SPI2EN |
                     RCC_APB2ENR_ADC1EN;
    RCC->APB1LENR |= RCC_APB1LENR_USART1EN;

    /* Initialize SPI1 for W25Q128 */
    w25q128_init();

    /* Initialize SPI2 for SSD1306 OLED */
    ssd1306_init();

    /* Initialize IR receiver */
    ir_receiver_init();

    /* Initialize IR transmitter */
    ir_transmitter_init();

    /* Initialize USART1 for nRF52840 BLE */
    /* 115200 baud, 8N1, hardware flow control */
    USART1->BRR = USART1_CLK_FREQ / 115200U;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    USART1->CR3 = (1U << 7) | (1U << 8);  /* CTSE, RTSE */

    /* Show startup on OLED */
    ssd1306_clear();
    ssd1306_draw_string(0, 0, "IR Phantom v1.0");
    ssd1306_draw_string(0, 2, "Initializing...");
}

/* ========================================
 * SysTick — 1 ms tick
 * ======================================== */

void SysTick_Handler(void) {
    g_system_ticks++;

    /* Toggle status LED every 500 ms in idle */
    if (g_mode == MODE_IDLE && (g_system_ticks % 500) == 0) {
        LED_STATUS_TOGGLE();
    }

    /* Check user button */
    if (!(GPIOA->IDR & (1U << USER_BTN_PIN))) {
        /* Button pressed — cycle modes */
        static uint32_t btn_debounce = 0;
        if (g_system_ticks - btn_debounce > 200) {
            btn_debounce = g_system_ticks;
            /* Advance to next mode */
            g_mode = (g_mode + 1) % 6;
        }
    }
}

/* ========================================
 * Main loop
 * ======================================== */

int main(void) {
    /* Initialize hardware */
    SystemInit();

    /* Verify SPI NOR flash */
    uint32_t jedec_id = w25q128_read_jedec_id();
    if (jedec_id != W25Q_JEDEC_EXPECTED) {
        LED_ERROR_ON();
        ssd1306_draw_string(0, 2, "Flash ERROR!");
    }

    /* Start in capture mode */
    g_mode = MODE_IDLE;
    oled_show_status();

    /* Enable SysTick (1 ms interval) */
    SysTick_Config(SYSCLK_FREQ / 1000U);

    /* Main super-loop */
    while (1) {
        switch (g_mode) {
        case MODE_IDLE:
            /* Wait for commands via USB/BLE */
            __WFI();
            break;

        case MODE_CAPTURE:
            /* IR capture handled by DMA + IRQ */
            ir_receiver_poll();
            break;

        case MODE_TRANSMIT:
            /* IR transmit on demand */
            ir_transmitter_poll();
            break;

        case MODE_FUZZ:
            /* IR fuzz engine */
            ir_receiver_poll();
            ir_transmitter_poll();
            break;

        case MODE_ANALYZE:
            /* FFT and cluster analysis */
            ir_receiver_poll();
            break;

        case MODE_BRIDGE:
            /* IR ↔ BLE bridge */
            ir_receiver_poll();
            break;

        default:
            g_mode = MODE_IDLE;
            break;
        }

        /* Update OLED display every 100 ms */
        if ((g_system_ticks % 100) == 0) {
            oled_show_status();
        }
    }
}

/* ========================================
 * OLED status display
 * ======================================== */

void oled_show_status(void) {
    ssd1306_clear();

    /* Line 0: Mode */
    const char *mode_str;
    switch (g_mode) {
    case MODE_IDLE:      mode_str = "IDLE"; break;
    case MODE_CAPTURE:   mode_str = "CAPTURE"; break;
    case MODE_TRANSMIT:  mode_str = "TRANSMIT"; break;
    case MODE_FUZZ:      mode_str = "FUZZ"; break;
    case MODE_ANALYZE:   mode_str = "ANALYZE"; break;
    case MODE_BRIDGE:    mode_str = "BRIDGE"; break;
    default:             mode_str = "???"; break;
    }
    ssd1306_draw_string(0, 0, mode_str);

    /* Line 1: Frame count */
    char buf[22];
    snprintf(buf, sizeof(buf), "Frames: %lu", g_frames_captured);
    ssd1306_draw_string(0, 1, buf);

    /* Line 2: Protocol (detected) */
    ssd1306_draw_string(0, 2, "Proto: ---");

    /* Line 3: Connection status */
    snprintf(buf, sizeof(buf), "USB:%c BLE:%c",
             g_usb_connected ? 'Y' : 'N',
             g_ble_connected ? 'Y' : 'N');
    ssd1306_draw_string(0, 3, buf);
}

/* ========================================
 * Fault handlers
 * ======================================== */

void hard_fault_handler(void) {
    LED_ERROR_ON();
    LED_STATUS_OFF();
    while (1)
        ;
}

void NMI_Handler(void) {
    while (1)
        ;
}

void MemManage_Handler(void) {
    LED_ERROR_ON();
    while (1)
        ;
}

void BusFault_Handler(void) {
    LED_ERROR_ON();
    while (1)
        ;
}

void UsageFault_Handler(void) {
    LED_ERROR_ON();
    while (1)
        ;
}