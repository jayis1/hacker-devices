/*
 * main.c — Volt Glitcher main firmware
 * STM32F407VGT6 + Lattice iCE40HX1K voltage glitch injection platform
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#include "board.h"
#include "registers.h"
#include "usb_descriptors.h"
#include "drivers/fpga.h"
#include "drivers/glitch.h"

/* ---- Global State ---- */
static volatile uint32_t g_tick_ms = 0;
static volatile uint8_t  g_armed = 0;
static volatile uint8_t  g_mode = MODE_IDLE;
static volatile uint8_t  g_fpga_ready = 0;
static volatile uint32_t g_glitch_count = 0;
static volatile uint32_t g_trigger_count = 0;
static volatile uint32_t g_success_count = 0;
static volatile uint16_t g_fault_flags = 0;

static glitch_config_t   g_glitch_cfg;
static usb_glitch_results_t g_results;

/* USB endpoint buffers */
static uint8_t g_ep1_out_buf[USB_CMD_PKT_SIZE];
static uint8_t g_ep2_in_buf[USB_CMD_PKT_SIZE];
static uint8_t g_ep3_in_buf[8];

/* UART trigger pattern matching state */
static volatile uint8_t  g_uart_match_idx = 0;
static volatile uint8_t  g_uart_partial_match = 0;

/* ---- SysTick Handler (1 ms tick) ---- */
void SysTick_Handler(void) {
    g_tick_ms++;

    /* Blink glitch LED when armed */
    if (g_armed && (g_tick_ms % 250 == 0)) {
        GPIOD->ODR ^= (1UL << 13);  /* Toggle PD13 glitch LED */
    }
}

/* ---- USART1 IRQ — UART trigger pattern matching ---- */
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t byte = (uint8_t)(USART1->DR & 0xFF);

        /* Feed byte to FPGA pattern matcher */
        if (g_fpga_ready && g_armed) {
            fpga_feed_uart_trigger(byte);
        }

        /* Software pattern matcher (backup) */
        if (g_glitch_cfg.trigger_mode == TRIG_UART_PATTERN && g_armed) {
            uint8_t mask = g_glitch_cfg.uart_mask;
            uint8_t pattern = g_glitch_cfg.uart_pattern[g_uart_match_idx];

            if ((byte & mask) == (pattern & mask)) {
                g_uart_match_idx++;
                if (g_uart_match_idx >= 4 ||
                    g_glitch_cfg.uart_pattern[g_uart_match_idx] == 0x00) {
                    /* Full match — fire trigger */
                    g_uart_match_idx = 0;
                    g_trigger_count++;
                    led_trigger_set(1);
                    if (g_armed) {
                        glitch_fire_on_trigger();
                    }
                }
            } else {
                g_uart_match_idx = 0;
            }
        }
    }
}

/* ---- EXTI0 — GPIO trigger on PB0 (TRIG0) ---- */
void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1UL << 0)) {
        EXTI->PR = (1UL << 0);
        g_trigger_count++;
        led_trigger_set(1);

        if (g_armed) {
            glitch_fire_on_trigger();
        }
    }
}

/* ---- TIM2 IRQ — Glitch pulse complete ---- */
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR = 0;
        TIM2->CR1 &= ~TIM_CR1_CEN;  /* Stop timer */

        /* De-assert glitch output */
        switch (g_mode) {
        case MODE_VOLTAGE_GLITCH:
            GPIOC->ODR &= ~(1UL << 6);   /* PC6 low — Q1 gate off */
            GPIOC->ODR &= ~(1UL << 7);   /* PC7 low — gate driver disable */
            break;
        case MODE_EM_GLITCH:
            GPIOC->ODR &= ~(1UL << 9);   /* PC9 low — Q2 gate off */
            GPIOC->ODR &= ~(1UL << 10);  /* PC10 low — EM disable */
            break;
        case MODE_CLOCK_GLITCH:
            GPIOB->ODR &= ~(1UL << 14);  /* PB14 low — Q3 gate off */
            GPIOB->ODR &= ~(1UL << 15);  /* PB15 low — clock glitch disable */
            break;
        default:
            break;
        }

        g_glitch_count++;
        led_glitch_set(0);

        /* Auto-rearm if configured */
        if (g_glitch_cfg.auto_arm) {
            g_armed = 1;
        } else {
            g_armed = 0;
            led_power_set(1);
        }

        /* Send USB event */
        usb_send_event(USB_EVT_GLITCH_FIRED, g_mode, g_tick_ms);
    }
}

/* ---- TIM3 IRQ — Trigger timestamp capture ---- */
void TIM3_IRQHandler(void) {
    if (TIM3->SR & TIM_SR_CC3IF) {
        TIM3->SR = ~TIM_SR_CC3IF;
        /* Capture timestamp for trigger event */
        uint32_t ts = TIM3->CCR3;
        g_results.last_timestamp = ts;
    }
}

/* ---- ADC1 IRQ — Analog watchdog / end of conversion ---- */
void ADC_IRQHandler(void) {
    if (ADC1->SR & ADC_SR_AWD) {
        ADC1->SR = ~ADC_SR_AWD;
        /* Analog watchdog tripped — possible overcurrent */
        g_fault_flags |= 0x01;
        led_error_set(1);
        usb_send_event(USB_EVT_OVERCURRENT, 0, g_tick_ms);

        /* Emergency: disarm and cut glitch output */
        glitch_emergency_cutoff();
    }
}

/* ========================================================================
 * Clock Configuration
 * ======================================================================== */

static void clock_init(void) {
    /* Enable HSE and wait for ready */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL: HSE = 8 MHz
     * PLLM = 8, PLLN = 336, PLLP = 2 => VCO=336 MHz, PLL=168 MHz
     * PLLQ = 7 => 48 MHz for USB */
    RCC->PLLCFGR = (8UL << 0)    /* PLLM = 8 */
                 | (336UL << 6)  /* PLLN = 336 */
                 | (0UL << 16)   /* PLLP = 2 */
                 | (7UL << 24);  /* PLLQ = 7 */

    /* Enable PLL and wait */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Flash: 5 wait states for 168 MHz at VOS=Scale 1 */
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 5UL;

    /* Select PLL as system clock */
    RCC->CFGR = (2UL << 0);  /* SW = PLL */
    while ((RCC->CFGR & (3UL << 2)) != (2UL << 2));  /* Wait for PLL as SYSCLK */

    /* AHB prescaler = 1, APB1 = 4, APB2 = 2 */
    RCC->CFGR |= (0UL << 4)   /* AHB = /1 */
              | (5UL << 10)    /* APB1 = /4 (42 MHz) */
              | (4UL << 13);   /* APB2 = /2 (84 MHz) */

    /* Enable peripheral clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN
                  | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN
                  | RCC_AHB1ENR_DMA2EN;

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN
                  | RCC_APB1ENR_TIM4EN | RCC_APB1ENR_USART2EN
                  | RCC_APB1ENR_I2C1EN;

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_USART1EN
                  | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_ADC1EN
                  | RCC_APB2ENR_SYSCFGEN;
}

/* ========================================================================
 * GPIO Initialization
 * ======================================================================== */

static void gpio_init(void) {
    /* --- LED pins: PD12-PD15 as output, push-pull, high speed --- */
    GPIOD->MODER &= ~(0xFFUL << 24);        /* Clear PD12-15 mode */
    GPIOD->MODER |= (0x55UL << 24);          /* Output mode for PD12-15 */
    GPIOD->OTYPER &= ~(0xFUL << 12);         /* Push-pull */
    GPIOD->OSPEEDR |= (0xFFUL << 24);        /* Very high speed */
    GPIOD->PUPDR &= ~(0xFFUL << 24);         /* No pull */
    GPIOD->ODR &= ~(0xFUL << 12);            /* All LEDs off */

    /* --- Glitch output pins --- */
    /* PC6: Q1 gate (VCC shunt), PC7: gate driver enable, PC8: fault input */
    GPIOC->MODER &= ~(0xFFUL << 12);
    GPIOC->MODER |= (0x55UL << 12);          /* PC6,7 output; PC8 input */
    GPIOC->OSPEEDR |= (0xFFUL << 12);        /* Very high speed */
    GPIOC->ODR &= ~(0x3UL << 6);            /* PC6,7 low (glitch off) */

    /* PC9: Q2 gate (EM), PC10: EM enable */
    GPIOC->MODER &= ~(0xFUL << 18);
    GPIOC->MODER |= (0x5UL << 18);          /* PC9,10 output */
    GPIOC->OSPEEDR |= (0xFUL << 18);
    GPIOC->ODR &= ~(0x3UL << 9);            /* EM glitch off */

    /* PB14: Q3 gate (clock), PB15: clock glitch enable */
    GPIOB->MODER &= ~(0xFUL << 28);
    GPIOB->MODER |= (0x5UL << 28);          /* PB14,15 output */
    GPIOB->OSPEEDR |= (0xFUL << 28);
    GPIOB->ODR &= ~(0x3UL << 14);           /* Clock glitch off */

    /* --- Trigger inputs --- */
    /* PB0: TRIG0 (GPIO) — input, pull-up */
    GPIOB->MODER &= ~(0x3UL << 0);          /* Input */
    GPIOB->PUPDR |= (0x1UL << 0);           /* Pull-up */

    /* PC13: TRIG3 (manual button) — input */
    GPIOC->MODER &= ~(0x3UL << 26);         /* Input */

    /* --- FPGA SPI pins: PA5(SCK), PB4(MISO), PB5(MOSI), PA4(NSS) --- */
    GPIOA->MODER &= ~(0x3UL << 8);          /* PA4 output (manual CS) */
    GPIOA->MODER |= (0x1UL << 8);
    GPIOA->ODR |= (1UL << 4);              /* CS high (deselected) */

    GPIOA->MODER &= ~(0x3UL << 10);        /* PA5 AF */
    GPIOA->MODER |= (0x2UL << 10);
    GPIOA->AFR[0] &= ~(0xFUL << 20);
    GPIOA->AFR[0] |= (5UL << 20);          /* AF5 = SPI1_SCK */

    GPIOB->MODER &= ~(0x3UL << 8);         /* PB4 AF */
    GPIOB->MODER |= (0x2UL << 8);
    GPIOB->AFR[0] &= ~(0xFUL << 16);
    GPIOB->AFR[0] |= (5UL << 16);          /* AF5 = SPI1_MISO */

    GPIOB->MODER &= ~(0x3UL << 10);        /* PB5 AF */
    GPIOB->MODER |= (0x2UL << 10);
    GPIOB->AFR[0] &= ~(0xFUL << 20);
    GPIOB->AFR[0] |= (5UL << 20);          /* AF5 = SPI1_MOSI */

    /* --- FPGA control pins --- */
    /* PB6: CRESET_B, PB7: CDONE — PB6 output, PB7 input */
    GPIOB->MODER &= ~(0xFUL << 12);
    GPIOB->MODER |= (0x1UL << 12);         /* PB6 output */
    GPIOB->ODR &= ~(1UL << 6);            /* CRESET_B low (FPGA in reset) */

    /* PC1: FPGA_INT (input), PC2: FPGA_WAKE (output), PC3: FPGA_SUSPEND (output) */
    GPIOC->MODER &= ~(0x3UL << 2);         /* PC1 input */
    GPIOC->MODER &= ~(0x3UL << 4);         /* PC2 output */
    GPIOC->MODER |= (0x1UL << 4);
    GPIOC->MODER &= ~(0x3UL << 6);         /* PC3 output */
    GPIOC->MODER |= (0x1UL << 6);
    GPIOC->ODR &= ~((1UL << 2) | (1UL << 3));  /* Wake=low, Suspend=low */

    /* --- ADC pins: PA0(IN0), PA5(IN5), PA6(IN6), PA7(IN7) --- */
    GPIOA->MODER &= ~(0xFUL << 0);         /* PA0 analog */
    GPIOA->MODER |= (0x3UL << 0);
    GPIOA->MODER &= ~(0xFFUL << 10);       /* PA5,6,7 analog */
    GPIOA->MODER |= (0xFFUL << 10);

    /* --- USART1: PA9(TX), PA10(RX) — trigger UART --- */
    GPIOA->MODER &= ~(0x3UL << 18);        /* PA9 AF */
    GPIOA->MODER |= (0x2UL << 18);
    GPIOA->AFR[1] &= ~(0xFUL << 4);
    GPIOA->AFR[1] |= (7UL << 4);           /* AF7 = USART1_TX */

    GPIOA->MODER &= ~(0x3UL << 20);        /* PA10 AF */
    GPIOA->MODER |= (0x2UL << 20);
    GPIOA->AFR[1] &= ~(0xFUL << 8);
    GPIOA->AFR[1] |= (7UL << 8);           /* AF7 = USART1_RX */

    /* --- Debug UART: PA2(TX), PA3(RX) --- */
    GPIOA->MODER &= ~(0x3UL << 4);         /* PA2 AF */
    GPIOA->MODER |= (0x2UL << 4);
    GPIOA->AFR[0] &= ~(0xFUL << 8);
    GPIOA->AFR[0] |= (7UL << 8);           /* AF7 = USART2_TX */

    GPIOA->MODER &= ~(0x3UL << 6);         /* PA3 AF */
    GPIOA->MODER |= (0x2UL << 6);
    GPIOA->AFR[0] &= ~(0xFUL << 12);
    GPIOA->AFR[0] |= (7UL << 12);          /* AF7 = USART2_RX */

    /* --- Power rail enable pins --- */
    GPIOA->MODER |= (0x55UL << 2);         /* PA1-4 output (reg enables) */
    GPIOA->ODR &= ~(0xFUL << 1);           /* All regulators off initially */
}

/* ========================================================================
 * USART Initialization
 * ======================================================================== */

static void usart1_init(void) {
    /* USART1 on APB2 = 84 MHz. BRR = fCLK / baud */
    USART1->BRR = 84000000UL / TRIG1_UART_BAUD;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    NVIC->IP[USART1_IRQn] = 0x60;  /* Priority 6 */
    NVIC->ISER[USART1_IRQn >> 5] = (1UL << (USART1_IRQn & 0x1F));
}

static void usart2_init(void) {
    /* USART2 on APB1 = 42 MHz */
    USART2->BRR = 42000000UL / DEBUG_UART_BAUD;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE;
}

/* ========================================================================
 * Timer Initialization
 * ======================================================================== */

static void tim2_init(void) {
    /* TIM2: Glitch pulse timer (32-bit, APB1 = 42 MHz, *2 = 84 MHz) */
    TIM2->PSC = 0;       /* No prescaler — 84 MHz clock */
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->DIER = TIM_DIER_UIE;  /* Update interrupt on overflow */
    TIM2->CR1 = 0;
    NVIC->IP[TIM2_IRQn] = 0x10;  /* High priority for glitch timing */
    NVIC->ISER[TIM2_IRQn >> 5] = (1UL << (TIM2_IRQn & 0x1F));
}

static void tim3_init(void) {
    /* TIM3: Trigger timestamp capture (16-bit) */
    TIM3->PSC = 83;      /* 84 MHz / 84 = 1 MHz → 1μs resolution */
    TIM3->ARR = 0xFFFF;
    TIM3->CCMR1 = (1UL << 8);  /* CC2 = input, IC2 mapped to TI2 */
    TIM3->CCER = TIM_CCER_CC2E;
    TIM3->DIER = TIM_DIER_CC2IE;
    TIM3->CR1 = TIM_CR1_CEN;
    NVIC->IP[TIM3_IRQn] = 0x40;
    NVIC->ISER[TIM3_IRQn >> 5] = (1UL << (TIM3_IRQn & 0x1F));
}

/* ========================================================================
 * ADC Initialization
 * ======================================================================== */

static void adc1_init(void) {
    /* Enable ADC voltage regulator (T_VREFINT_EN) */
    ADC1->CR2 = 0;
    for (volatile int i = 0; i < 1000; i++);  /* Wait for regulator */

    ADC1->CR2 = ADC_CR2_ADON;

    /* Sample time: 56 cycles for channels 0,5,6,7 */
    ADC1->SMPR2 = (3UL << 0)   /* CH0: 56 cycles */
               | (3UL << 15)   /* CH5: 56 cycles */
               | (3UL << 18)   /* CH6: 56 cycles */
               | (3UL << 21);  /* CH7: 56 cycles */

    /* Analog watchdog on CH0 (target VCC) — threshold configurable */
    ADC1->CR1 = ADC_CR1_RES_12BIT | ADC_CR1_AWDIE | ADC_CR1_AWDCH_MASK;

    NVIC->IP[ADC_IRQn] = 0x30;
    NVIC->ISER[ADC_IRQn >> 5] = (1UL << (ADC_IRQn & 0x1F));
}

/* ========================================================================
 * SPI1 Initialization (FPGA interface)
 * ======================================================================== */

static void spi1_init(void) {
    SPI1->CR1 = SPI_CR1_MSTR           /* Master mode */
              | SPI_CR1_BR_DIV4         /* 84/4 = 21 MHz */
              | SPI_CR1_SSM             /* Software NSS */
              | SPI_CR1_SSI             /* Internal NSS high */
              | SPI_CR1_CPOL            /* Clock idle high (mode 3) */
              | SPI_CR1_CPHA;           /* Clock phase 1 */

    SPI1->CR1 |= SPI_CR1_SPE;          /* Enable */
}

/* ========================================================================
 * EXTI Initialization (GPIO trigger on PB0)
 * ======================================================================== */

static void exti_init(void) {
    /* Map PB0 to EXTI0 */
    SYSCFG->EXTICR[0] &= ~(0xFUL << 0);
    SYSCFG->EXTICR[0] |= (1UL << 0);    /* EXTI0 = Port B */

    EXTI->IMR |= (1UL << 0);            /* Unmask EXTI0 */
    EXTI->RTSR |= (1UL << 0);           /* Rising edge trigger */
    EXTI->FTSR |= (1UL << 0);           /* Falling edge trigger */

    NVIC->IP[EXTI0_IRQn] = 0x20;       /* Priority 2 */
    NVIC->ISER[EXTI0_IRQn >> 5] = (1UL << (EXTI0_IRQn & 0x1F));
}

/* ========================================================================
 * USB Event Notification
 * ======================================================================== */

void usb_send_event(uint8_t type, uint8_t data, uint32_t timestamp) {
    usb_event_t *evt = (usb_event_t *)g_ep3_in_buf;
    evt->event_type = type;
    evt->event_data = data;
    evt->event_timestamp_lo = (uint16_t)(timestamp & 0xFFFF);
    evt->event_timestamp_hi = (uint16_t)(timestamp >> 16);
    evt->reserved[0] = 0;
    evt->reserved[1] = 0;

    /* Queue for EP3 IN transfer (simplified — real impl uses USB OTG FS driver) */
    (void)evt;
}

/* ========================================================================
 * USB Command Processing
 * ======================================================================== */

static uint8_t cmd_checksum(const uint8_t *buf) {
    uint8_t csum = 0;
    for (int i = 0; i < USB_CHECKSUM_IDX; i++) {
        csum ^= buf[i];
    }
    return csum;
}

static void usb_send_response(uint8_t cmd, uint8_t status,
                               const uint8_t *payload, uint8_t len) {
    g_ep2_in_buf[0] = cmd;
    g_ep2_in_buf[1] = status;
    if (payload && len > 0 && len <= USB_PAYLOAD_MAX_LEN) {
        for (int i = 0; i < len; i++) {
            g_ep2_in_buf[USB_PAYLOAD_START + i] = payload[i];
        }
    }
    g_ep2_in_buf[USB_CHECKSUM_IDX] = cmd_checksum(g_ep2_in_buf);
    /* EP2 IN transfer would be triggered here via OTG FS driver */
}

static void process_usb_command(const uint8_t *buf) {
    uint8_t cmd = buf[USB_CMD_IDX];
    uint8_t subcmd = buf[USB_SUBCMD_IDX];
    uint16_t p1 = (uint16_t)(buf[USB_PARAM1_IDX] | (buf[USB_PARAM1_IDX + 1] << 8));
    uint16_t p2 = (uint16_t)(buf[USB_PARAM2_IDX] | (buf[USB_PARAM2_IDX + 1] << 8));

    /* Verify checksum */
    if (cmd_checksum(buf) != buf[USB_CHECKSUM_IDX]) {
        usb_send_response(cmd, USB_RESP_STATUS_INVALID, NULL, 0);
        return;
    }

    switch (cmd) {
    case USB_CMD_GET_STATUS: {
        uint8_t status = (g_armed ? 0x01 : 0x00)
                       | (g_fpga_ready ? 0x02 : 0x00)
                       | (g_fault_flags ? 0x04 : 0x00);
        usb_send_response(cmd, USB_RESP_STATUS_OK, &status, 1);
        break;
    }

    case USB_CMD_SET_GLITCH_CFG: {
        if (g_armed) {
            usb_send_response(cmd, USB_RESP_STATUS_BUSY, NULL, 0);
            break;
        }
        usb_glitch_config_t *cfg = (usb_glitch_config_t *)&buf[USB_PAYLOAD_START];
        glitch_config_from_usb(cfg, &g_glitch_cfg);
        g_mode = g_glitch_cfg.mode;
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    case USB_CMD_GET_GLITCH_CFG: {
        usb_send_response(cmd, USB_RESP_STATUS_OK,
                          (const uint8_t *)&g_glitch_cfg,
                          sizeof(g_glitch_cfg));
        break;
    }

    case USB_CMD_ARM: {
        if (!g_fpga_ready) {
            usb_send_response(cmd, USB_RESP_STATUS_NO_FPGA, NULL, 0);
            break;
        }
        if (g_mode == MODE_IDLE) {
            usb_send_response(cmd, USB_RESP_STATUS_INVALID, NULL, 0);
            break;
        }
        g_armed = 1;
        g_glitch_count = 0;
        g_trigger_count = 0;
        glitch_arm(&g_glitch_cfg);
        led_power_set(1);
        led_glitch_set(1);
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    case USB_CMD_DISARM: {
        g_armed = 0;
        glitch_disarm();
        led_all_off();
        led_power_set(1);
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    case USB_CMD_FIRE: {
        if (!g_armed) {
            usb_send_response(cmd, USB_RESP_STATUS_UNARMED, NULL, 0);
            break;
        }
        glitch_fire_manual();
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    case USB_CMD_GET_RESULTS: {
        g_results.glitch_count = g_glitch_count;
        g_results.trigger_count = g_trigger_count;
        g_results.success_count = (uint16_t)g_success_count;
        g_results.fault_flags = g_fault_flags;
        g_results.armed = g_armed;
        g_results.mode_active = g_mode;
        g_results.target_vcc_mv = (int16_t)adc_read_target_vcc_mv();
        g_results.shunt_current_ma = (int16_t)adc_read_shunt_current_ma();
        usb_send_response(cmd, USB_RESP_STATUS_OK,
                          (const uint8_t *)&g_results,
                          sizeof(g_results));
        break;
    }

    case USB_CMD_SET_TRIGGER: {
        if (g_armed) {
            usb_send_response(cmd, USB_RESP_STATUS_BUSY, NULL, 0);
            break;
        }
        g_glitch_cfg.trigger_mode = subcmd;
        g_glitch_cfg.trigger_edge = (uint8_t)(p1 & 0xFF);
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    case USB_CMD_FPGA_WRITE: {
        if (!g_fpga_ready) {
            usb_send_response(cmd, USB_RESP_STATUS_NO_FPGA, NULL, 0);
            break;
        }
        uint8_t addr = subcmd;
        uint16_t val = p1;
        fpga_write_reg(addr, val);
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    case USB_CMD_FPGA_READ: {
        if (!g_fpga_ready) {
            usb_send_response(cmd, USB_RESP_STATUS_NO_FPGA, NULL, 0);
            break;
        }
        uint8_t addr = subcmd;
        uint16_t val = fpga_read_reg(addr);
        uint8_t resp[2] = { (uint8_t)(val & 0xFF), (uint8_t)(val >> 8) };
        usb_send_response(cmd, USB_RESP_STATUS_OK, resp, 2);
        break;
    }

    case USB_CMD_FPGA_LOAD_BIT: {
        /* Trigger FPGA bitstream load from flash */
        board_power_fpga_off();
        delay_ms(10);
        board_power_fpga_on();
        delay_ms(50);
        if (fpga_configure() == 0) {
            g_fpga_ready = 1;
            usb_send_event(USB_EVT_FPGA_READY, 0, g_tick_ms);
            usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        } else {
            g_fpga_ready = 0;
            usb_send_response(cmd, USB_RESP_STATUS_FAULT, NULL, 0);
        }
        break;
    }

    case USB_CMD_ADC_READ: {
        uint8_t chan = subcmd;
        uint16_t raw = adc_read_channel(chan);
        int32_t mv = adc_to_mv(raw);
        uint8_t resp[4] = {
            (uint8_t)(raw & 0xFF), (uint8_t)(raw >> 8),
            (uint8_t)(mv & 0xFF), (uint8_t)(mv >> 8)
        };
        usb_send_response(cmd, USB_RESP_STATUS_OK, resp, 4);
        break;
    }

    case USB_CMD_CALIBRATE: {
        g_mode = MODE_CALIBRATION;
        glitch_calibrate();
        g_mode = MODE_IDLE;
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    case USB_CMD_RESET: {
        g_armed = 0;
        g_mode = MODE_IDLE;
        g_fault_flags = 0;
        glitch_disarm();
        led_all_off();
        usb_send_response(cmd, USB_RESP_STATUS_OK, NULL, 0);
        break;
    }

    default:
        usb_send_response(cmd, USB_RESP_STATUS_INVALID, NULL, 0);
        break;
    }
}

/* ========================================================================
 * Board-level Implementations
 * ======================================================================== */

void board_init(void) {
    clock_init();
    gpio_init();
    spi1_init();
    usart1_init();
    usart2_init();
    tim2_init();
    tim3_init();
    adc1_init();
    exti_init();

    /* Enable SysTick: 168 MHz / 168000 = 1000 Hz (1 ms) */
    SysTick_Config(168000UL);
}

void board_deinit(void) {
    /* Disable all timers */
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM3->CR1 &= ~TIM_CR1_CEN;
    TIM4->CR1 &= ~TIM_CR1_CEN;

    /* Disable UARTs */
    USART1->CR1 = 0;
    USART2->CR1 = 0;

    /* Power down FPGA */
    board_power_fpga_off();

    /* All LEDs off */
    GPIOD->ODR &= ~(0xFUL << 12);

    /* All glitch outputs off */
    GPIOC->ODR &= ~((1UL << 6) | (1UL << 7) | (1UL << 9) | (1UL << 10));
    GPIOB->ODR &= ~((1UL << 14) | (1UL << 15));
}

void board_power_fpga_on(void) {
    /* Power sequence: 1.2V core first, then 2.5V PLL, then 1.8V aux, then 3.3V IO */
    GPIOA->ODR |= (1UL << 1);  /* Enable 1.2V regulator U2 */
    delay_ms(5);

    /* Wait for 1.2V power good */
    uint32_t timeout = g_tick_ms + 100;
    while (!(GPIOB->IDR & (1UL << 8))) {
        if (g_tick_ms > timeout) break;
    }

    GPIOA->ODR |= (1UL << 4);  /* Enable 2.5V PLL regulator U5 */
    delay_ms(2);
    GPIOA->ODR |= (1UL << 3);  /* Enable 1.8V aux regulator U4 */
    delay_ms(2);
    GPIOA->ODR |= (1UL << 2);  /* Enable 3.3V IO regulator U3 */
    delay_ms(5);

    /* Wait for 3.3V power good */
    timeout = g_tick_ms + 100;
    while (!(GPIOB->IDR & (1UL << 9))) {
        if (g_tick_ms > timeout) break;
    }
}

void board_power_fpga_off(void) {
    /* Reverse power-down sequence */
    GPIOA->ODR &= ~(1UL << 2);  /* Disable 3.3V IO */
    delay_ms(1);
    GPIOA->ODR &= ~(1UL << 3);  /* Disable 1.8V aux */
    delay_ms(1);
    GPIOA->ODR &= ~(1UL << 4);  /* Disable 2.5V PLL */
    delay_ms(1);
    GPIOA->ODR &= ~(1UL << 1);  /* Disable 1.2V core */
    delay_ms(2);

    /* Assert FPGA reset */
    GPIOB->ODR &= ~(1UL << 6);  /* CRESET_B low */
}

int board_power_good(void) {
    return (GPIOB->IDR & (1UL << 8)) && (GPIOB->IDR & (1UL << 9));
}

void led_power_set(int on) {
    if (on) GPIOD->ODR |= (1UL << 12);
    else    GPIOD->ODR &= ~(1UL << 12);
}

void led_glitch_set(int on) {
    if (on) GPIOD->ODR |= (1UL << 13);
    else    GPIOD->ODR &= ~(1UL << 13);
}

void led_trigger_set(int on) {
    if (on) GPIOD->ODR |= (1UL << 14);
    else    GPIOD->ODR &= ~(1UL << 14);
}

void led_error_set(int on) {
    if (on) GPIOD->ODR |= (1UL << 15);
    else    GPIOD->ODR &= ~(1UL << 15);
}

void led_all_off(void) {
    GPIOD->ODR &= ~(0xFUL << 12);
}

void delay_us(uint32_t us) {
    /* Approximate: 168 MHz = 168 cycles/us, ~4 cycles per iteration */
    volatile uint32_t count = us * 42;
    while (count--) {
        __asm__ volatile ("nop");
    }
}

void delay_ms(uint32_t ms) {
    uint32_t start = g_tick_ms;
    while ((g_tick_ms - start) < ms);
}

uint16_t adc_read_channel(uint8_t channel) {
    ADC1->SQR3 = channel & 0x1F;        /* Select channel */
    ADC1->CR2 |= ADC_CR2_SWSTART;       /* Start conversion */
    while (!(ADC1->SR & ADC_SR_EOC));    /* Wait for EOC */
    return (uint16_t)(ADC1->DR & 0xFFF);
}

int32_t adc_to_mv(uint16_t raw) {
    /* Convert 12-bit ADC reading to millivolts
     * Vref = 3.3V = 3300 mV, full scale = 4095 */
    return (int32_t)((raw * ADC_VREF) / ADC_FULL_SCALE);
}

int32_t adc_read_target_vcc_mv(void) {
    uint16_t raw = adc_read_channel(ADC_TARGET_VCC_CHAN);
    /* Voltage divider: R1=10k, R2=10k → Vtarget = 2 * Vadc */
    return adc_to_mv(raw) * 2;
}

int32_t adc_read_shunt_current_ma(void) {
    uint16_t raw = adc_read_channel(ADC_SHUNT_CURR_CHAN);
    int32_t vadc_mv = adc_to_mv(raw);
    /* INA210: Vout = Vref + (Ishunt * Rshunt * Gain)
     * Ishunt = (Vout - Vref) / (Rshunt * Gain)
     * Rshunt = 0.1 mΩ = 100 μΩ, Gain = 500
     * Current in mA = (Vadc_mV - 1250) * 1000 / (0.1 * 500)
     *              = (Vadc_mV - 1250) * 20 */
    return (vadc_mv - CURRENT_SENSE_VREF) * 20;
}

/* ========================================================================
 * Debug printf (minimal, USART2)
 * ======================================================================== */

void debug_printf(const char *fmt, ...) {
    /* Simplified — just outputs a string without formatting.
     * Full implementation would use va_list with vsnprintf. */
    const char *p = fmt;
    while (*p) {
        while (!(USART2->SR & USART_SR_TXE));
        USART2->DR = (uint16_t)*p;
        p++;
    }
}

/* ========================================================================
 * Main
 * ======================================================================== */

int main(void) {
    /* Initialize all hardware */
    board_init();
    led_power_set(1);

    debug_printf("VoltGlitcher v1.0 booting...\r\n");

    /* Power on and configure FPGA */
    board_power_fpga_on();
    delay_ms(50);

    if (fpga_configure() == 0) {
        g_fpga_ready = 1;
        debug_printf("FPGA configured OK\r\n");
        usb_send_event(USB_EVT_FPGA_READY, 0, g_tick_ms);
    } else {
        g_fpga_ready = 0;
        debug_printf("FPGA config FAILED\r\n");
        led_error_set(1);
    }

    /* Initialize glitch engine */
    glitch_init(&g_glitch_cfg);

    /* Load default profile from EEPROM */
    if (glitch_load_profile(0, &g_glitch_cfg) == 0) {
        g_mode = g_glitch_cfg.mode;
        debug_printf("Profile 0 loaded\r\n");
    }

    /* Main loop: process USB commands and monitor */
    while (1) {
        /* Check for USB EP1 OUT data (simplified polling) */
        /* In production, this is handled by OTG FS IRQ */

        /* Periodic safety monitoring */
        if (g_tick_ms % 100 == 0) {
            /* Check FPGA status */
            if (g_fpga_ready) {
                uint16_t fstatus = fpga_read_reg(FPGA_REG_STATUS);
                if (fstatus & FPGA_STATUS_FAULT) {
                    g_fault_flags |= 0x02;
                    led_error_set(1);
                }
                if (fstatus & FPGA_STATUS_OVERTEMP) {
                    g_fault_flags |= 0x04;
                    usb_send_event(USB_EVT_OVERTEMP, 0, g_tick_ms);
                }
            }

            /* Check power rails */
            if (!board_power_good() && g_fpga_ready) {
                g_fault_flags |= 0x08;
                led_error_set(1);
                glitch_emergency_cutoff();
                debug_printf("Power fault detected\r\n");
            }
        }

        /* Check manual trigger button (PC13) */
        if (!(GPIOC->IDR & (1UL << 13))) {
            delay_ms(TRIG3_BTN_DEBOUNCE_MS);
            if (!(GPIOC->IDR & (1UL << 13))) {
                if (g_glitch_cfg.trigger_mode == TRIG_MANUAL && g_armed) {
                    g_trigger_count++;
                    glitch_fire_on_trigger();
                }
                /* Wait for release */
                while (!(GPIOC->IDR & (1UL << 13)));
            }
        }

        __asm__ volatile ("wfi");  /* Wait for interrupt */
    }

    return 0;
}