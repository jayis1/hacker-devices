/*
 * NFC Relay Phantom — Main Application
 * STM32L4S5VIT6-based NFC/RFID security research platform
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 *
 * Modes:
 *   - NFC Reader (ISO 14443A/B, FeliCa, ISO 15693)
 *   - NFC Card Emulation
 *   - NFC Sniffer (frame-level capture)
 *   - NFC Relay (BLE tunnel between two devices)
 *   - 125 kHz RFID Read/Clone (EM4100, HID Prox, T5577)
 */

#include "board.h"
#include "registers.h"
#include "drivers/spi_dma.h"
#include "drivers/pn5180.h"
#include "drivers/em4095.h"

/* ======================================================================
 * Global State
 * ====================================================================== */

typedef enum {
    MODE_IDLE = 0,
    MODE_NFC_READER,
    MODE_NFC_EMULATE,
    MODE_NFC_SNIFFER,
    MODE_NFC_RELAY,
    MODE_RFID_125_READ,
    MODE_RFID_125_CLONE,
    MODE_MENU,
    MODE_SETTINGS,
} op_mode_t;

static volatile op_mode_t g_mode = MODE_IDLE;
static volatile uint32_t g_ticks = 0;         /* 1ms tick counter */
static volatile uint8_t g_nfc_irq_flag = 0;
static volatile uint8_t g_nrf_irq_flag = 0;
static volatile uint8_t g_sw1_flag = 0;
static volatile uint8_t g_sw2_flag = 0;

/* NFC frame buffer (in SRAM3 for DMA access) */
__attribute__((section(".sram3"))) static uint8_t g_nfc_rx_buf[NFC_MAX_FRAME_SIZE];
__attribute__((section(".sram3"))) static uint8_t g_nfc_tx_buf[NFC_MAX_FRAME_SIZE];

/* 125 kHz RFID context */
static rfid_125_context_t g_rfid_ctx;

/* ======================================================================
 * SysTick Handler (1ms)
 * ====================================================================== */
void SysTick_Handler(void) {
    g_ticks++;
}

/* ======================================================================
 * External Interrupt Handlers
 * ====================================================================== */
void EXTI0_IRQHandler(void) {
    /* NRF_IRQ on PC0 */
    if (EXTI->PR1 & (1 << 0)) {
        EXTI->PR1 = (1 << 0);
        g_nrf_irq_flag = 1;
    }
}

void EXTI4_IRQHandler(void) {
    /* PN5180_BUSY on PC4 */
    if (EXTI->PR1 & (1 << 4)) {
        EXTI->PR1 = (1 << 4);
        g_nfc_irq_flag = 1;
    }
}

void EXTI15_10_IRQHandler(void) {
    /* No exti 13 in this range, SW1 on PC13 */
    if (EXTI->PR1 & (1 << 13)) {
        EXTI->PR1 = (1 << 13);
        g_sw1_flag = 1;
    }
}

void EXTI9_5_IRQHandler(void) {
    /* SW2 on PB8 */
    if (EXTI->PR1 & (1 << 8)) {
        EXTI->PR1 = (1 << 8);
        g_sw2_flag = 1;
    }
}

/* ======================================================================
 * Delay Functions
 * ====================================================================== */
static inline void delay_us(uint32_t us) {
    uint32_t start = g_ticks;
    uint32_t wait = us / 1000;
    if (wait == 0) wait = 1;
    while ((g_ticks - start) < wait);
}

static inline void delay_ms(uint32_t ms) {
    uint32_t start = g_ticks;
    while ((g_ticks - start) < ms);
}

/* ======================================================================
 * UART4 (NRF52 Link) — Simple polled I/O
 * ====================================================================== */
static void uart4_init(void) {
    /* Enable USART4 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART4EN;

    /* Configure baud rate: 115200 @ 120 MHz APB1 */
    /* BRR = 120000000 / 115200 = 1041.67 → 1042 */
    USART4->BRR = 1042;

    /* 8N1, enable TX/RX */
    USART4->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart4_putc(char c) {
    while (!(USART4->ISR & USART_ISR_TXE));
    USART4->TDR = c;
}

static char uart4_getc(void) {
    while (!(USART4->ISR & USART_ISR_RXNE));
    return (char)(USART4->RDR & 0xFF);
}

static void uart4_puts(const char *s) {
    while (*s) {
        uart4_putc(*s++);
    }
}

/* ======================================================================
 * I2C1 — Pololed master mode for SSD1306 + BQ25896
 * ====================================================================== */
static void i2c1_init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* Timing for 400 kHz @ 120 MHz: PRESC=5, SCLDEL=3, SDADEL=3, SCLH=14, SCLL=26 */
    I2C1->TIMINGR = (5 << 28) | (3 << 20) | (3 << 16) | (14 << 8) | 26;
    I2C1->CR1 = I2C_CR1_PE;
}

static int i2c1_write(uint8_t addr, const uint8_t *data, uint8_t len) {
    I2C1->CR2 = (addr << 1) | (len << I2C_CR2_NBYTES_Pos) | I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        while (!(I2C1->ISR & I2C_ISR_TXIS));
        I2C1->TXDR = data[i];
    }

    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR = I2C_ISR_STOPF;

    return (I2C1->ISR & I2C_ISR_NACKF) ? -1 : 0;
}

/* ======================================================================
 * SSD1306 OLED Display
 * ====================================================================== */
static uint8_t g_oled_buf[OLED_WIDTH * OLED_PAGES];

static void oled_init(void) {
    /* Hardware reset */
    OLED_RST_PORT->ODR &= ~(1 << OLED_RST_PIN);
    delay_ms(10);
    OLED_RST_PORT->ODR |= (1 << OLED_RST_PIN);
    delay_ms(100);

    const uint8_t init_cmds[] = {
        SSD1306_CMD_DISPLAY_OFF,
        SSD1306_CMD_SET_MUX_RATIO, 0x3F,
        SSD1306_CMD_SET_DISPLAY_ON,
        SSD1306_CMD_SET_SEG_REMAP,
        SSD1306_CMD_SET_COM_SCAN,
        SSD1306_CMD_SET_COM_PINS, 0x12,
        SSD1306_CMD_SET_CONTRAST, 0xCF,
        SSD1306_CMD_SET_PRECHARGE, 0xF1,
        SSD1306_CMD_SET_VCOM, 0x40,
        SSD1306_CMD_SET_OSC_FREQ, 0x80,
        SSD1306_CMD_SET_CHARGE_PUMP, 0x14,
        SSD1306_CMD_SET_MEMORY_MODE, 0x00,
        SSD1306_CMD_SET_START_LINE | 0,
        SSD1306_CMD_SET_COL_ADDR, 0, 127,
        SSD1306_CMD_SET_PAGE_ADDR, 0, 7,
        SSD1306_CMD_DISPLAY_ON,
    };

    for (uint16_t i = 0; i < sizeof(init_cmds); i++) {
        uint8_t cmd[2] = {0x00, init_cmds[i]};  /* Control byte 0x00 = command */
        i2c1_write(SSD1306_I2C_ADDR, cmd, 2);
    }
}

static void oled_clear(void) {
    for (uint16_t i = 0; i < sizeof(g_oled_buf); i++) {
        g_oled_buf[i] = 0x00;
    }
}

static void oled_refresh(void) {
    /* Set column and page address */
    uint8_t cmd1[] = {0x00, SSD1306_CMD_SET_COL_ADDR, 0, 127};
    uint8_t cmd2[] = {0x00, SSD1306_CMD_SET_PAGE_ADDR, 0, 7};
    i2c1_write(SSD1306_I2C_ADDR, cmd1, sizeof(cmd1));
    i2c1_write(SSD1306_I2C_ADDR, cmd2, sizeof(cmd2));

    /* Send data in 16-byte chunks (I2C max) */
    for (uint16_t off = 0; off < sizeof(g_oled_buf); off += 16) {
        uint8_t buf[17];
        buf[0] = 0x40;  /* Data mode */
        for (uint8_t j = 0; j < 16 && (off + j) < sizeof(g_oled_buf); j++) {
            buf[j + 1] = g_oled_buf[off + j];
        }
        i2c1_write(SSD1306_I2C_ADDR, buf, 17);
    }
}

/* ======================================================================
 * Simple 5x7 font for OLED display
 * ====================================================================== */
static const uint8_t font5x7[][5] = {
    /* Space */ {0x00,0x00,0x00,0x00,0x00},
    /* 'N' */   {0x7F,0x08,0x08,0x08,0x7F},
    /* 'F' */   {0x7F,0x40,0x40,0x40,0x40},
    /* 'C' */   {0x3E,0x41,0x41,0x41,0x22},
    /* ... abbreviated ... */
};

static void oled_draw_char(uint8_t x, uint8_t page, char c, const uint8_t font[][5]) {
    (void)c; (void)font;
    /* Placeholder — full font lookup in production */
    for (uint8_t col = 0; col < 5; col++) {
        if (x + col < OLED_WIDTH && page < OLED_PAGES) {
            g_oled_buf[page * OLED_WIDTH + x + col] = 0xFF;  /* Temp: fill block */
        }
    }
}

/* ======================================================================
 * BQ25896 Battery Charger — Simple status read
 * ====================================================================== */
static uint8_t bq25896_read_reg(uint8_t reg) {
    /* Write register address, then read 1 byte */
    uint8_t addr_byte = reg;
    I2C1->CR2 = (BQ25896_I2C_ADDR << 1) | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = addr_byte;

    /* Repeated start, read 1 byte */
    I2C1->CR2 = (BQ25896_I2C_ADDR << 1) | (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_START | I2C_CR2_RD_WRN;
    while (!(I2C1->ISR & I2C_ISR_RXNE));
    uint8_t val = I2C1->RXDR;

    I2C1->CR2 |= I2C_CR2_STOP;
    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR = I2C_ISR_STOPF;

    return val;
}

static uint8_t bq25896_get_battery_pct(void) {
    uint8_t status = bq25896_read_reg(BQ25896_REG_STATUS);
    uint8_t fault = bq25896_read_reg(BQ25896_REG_FAULT);
    (void)status; (void)fault;
    /* Simplified: read VBAT register and approximate percentage */
    return 100;  /* Placeholder */
}

/* ======================================================================
 * EXTI Configuration
 * ====================================================================== */
static void exti_init(void) {
    /* Enable SYSCFG clock */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* PC0 (NRF_IRQ) → EXTI0 */
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0xF) | 0x2;  /* PC0 → EXTI0 */

    /* PC4 (PN5180_BUSY) → EXTI4 */
    SYSCFG->EXTICR[1] = (SYSCFG->EXTICR[1] & ~(0xF << 0)) | (0x2 << 0);  /* PC4 */

    /* PC13 (SW1) → EXTI13 */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xF << 8)) | (0x2 << 8);  /* PC13 */

    /* PB8 (SW2) → EXTI8 */
    SYSCFG->EXTICR[2] = (SYSCFG->EXTICR[2] & ~(0xF << 0)) | (0x1 << 0);  /* PB8 */

    /* Enable EXTI interrupts */
    EXTI->IMR1 = (1 << 0) | (1 << 4) | (1 << 8) | (1 << 13);
    EXTI->RTSR1 = (1 << 0) | (1 << 4) | (1 << 8) | (1 << 13);

    /* Enable NVIC */
    NVIC_ISER0 = (1 << 6)   /* EXTI0 */
               | (1 << 10)   /* EXTI4 */
               | (1 << 23)   /* EXTI9_5 */
               | (1 << 40 % 32]; /* EXTI15_10 */
}

/* ======================================================================
 * Main Application State Machine
 * ====================================================================== */
static void mode_idle(void) {
    oled_clear();
    /* Draw splash: "NFC Phantom v1.0" */
    oled_refresh();

    LED_GREEN_ON();
    delay_ms(500);
    LED_GREEN_OFF();

    /* Wait for user input or BLE command */
    while (g_mode == MODE_IDLE) {
        if (g_sw1_flag) {
            g_sw1_flag = 0;
            g_mode = MODE_MENU;
        }
        if (g_sw2_flag) {
            g_sw2_flag = 0;
            /* Toggle 125 kHz read */
            g_mode = MODE_RFID_125_READ;
        }
        __WFI();
    }
}

static void mode_nfc_reader(void) {
    nfc_context_t ctx;
    pn5180_init();
    pn5180_field_on();

    while (g_mode == MODE_NFC_READER) {
        if (pn5180_detect_card(&ctx)) {
            LED_GREEN_ON();
            /* Send UID via BLE/USB */
            uart4_puts("UID:");
            for (uint8_t i = 0; i < ctx.uid_len; i++) {
                char hex[3];
                hex[0] = "0123456789ABCDEF"[ctx.uid[i] >> 4];
                hex[1] = "0123456789ABCDEF"[ctx.uid[i] & 0xF];
                hex[2] = '\0';
                uart4_puts(hex);
            }
            uart4_puts("\r\n");
            delay_ms(500);
            LED_GREEN_OFF();
        }
        if (g_sw1_flag) {
            g_sw1_flag = 0;
            g_mode = MODE_IDLE;
        }
    }

    pn5180_field_off();
}

static void mode_nfc_emulate(void) {
    /* Set up card emulation with default UID */
    uint8_t emulate_uid[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t atqa[] = {0x04, 0x00};
    pn5180_init();
    pn5180_set_card_emulation(emulate_uid, 4, atqa, 0x08);

    LED_BLUE_ON();
    while (g_mode == MODE_NFC_EMULATE) {
        if (g_sw1_flag) {
            g_sw1_flag = 0;
            g_mode = MODE_IDLE;
        }
        __WFI();
    }
    LED_BLUE_OFF();
}

static void mode_nfc_sniffer(void) {
    pn5180_init();
    pn5180_set_sniffer_mode();

    uint16_t capture_offset = 0;
    LED_GREEN_ON();
    LED_RED_ON();

    while (g_mode == MODE_NFC_SNIFFER) {
        size_t frame_len = 0;
        if (pn5180_recv_frame(g_nfc_rx_buf, &frame_len, 100)) {
            /* Store captured frame to SPI flash */
            /* ... flash write at FLASH_ADDR_CAPTURE + capture_offset ... */
            capture_offset += frame_len;

            /* Forward to BLE */
            uart4_puts("FRAME:");
            for (size_t i = 0; i < frame_len; i++) {
                char hex[3];
                hex[0] = "0123456789ABCDEF"[g_nfc_rx_buf[i] >> 4];
                hex[1] = "0123456789ABCDEF"[g_nfc_rx_buf[i] & 0xF];
                hex[2] = '\0';
                uart4_puts(hex);
            }
            uart4_puts("\r\n");
        }

        if (g_sw1_flag) {
            g_sw1_flag = 0;
            g_mode = MODE_IDLE;
        }
    }

    LED_GREEN_OFF();
    LED_RED_OFF();
}

static void mode_nfc_relay(void) {
    /* Relay mode: forward NFC frames between two Phantom devices via BLE */
    pn5180_init();
    uart4_puts("RELAY:START\r\n");
    LED_BLUE_ON();
    LED_GREEN_ON();

    while (g_mode == MODE_NFC_RELAY) {
        /* Poll for BLE data from paired device */
        /* Forward to PN5180 and vice versa */
        /* Actual relay implementation in production firmware */
        if (g_sw1_flag) {
            g_sw1_flag = 0;
            g_mode = MODE_IDLE;
        }
        __WFI();
    }

    uart4_puts("RELAY:STOP\r\n");
    LED_BLUE_OFF();
    LED_GREEN_OFF();
}

static void mode_rfid_125_read(void) {
    em4095_init();
    em4095_power_on();
    LED_RED_ON();

    while (g_mode == MODE_RFID_125_READ) {
        if (em4095_read_em4100(&g_rfid_ctx)) {
            LED_GREEN_ON();
            uart4_puts("RFID:");
            for (uint8_t i = 0; i < g_rfid_ctx.bit_count / 8; i++) {
                char hex[3];
                hex[0] = "0123456789ABCDEF"[g_rfid_ctx.data[i] >> 4];
                hex[1] = "0123456789ABCDEF"[g_rfid_ctx.data[i] & 0xF];
                hex[2] = '\0';
                uart4_puts(hex);
            }
            uart4_puts("\r\n");
            delay_ms(1000);
            LED_GREEN_OFF();
        }

        if (g_sw1_flag) {
            g_sw1_flag = 0;
            g_mode = MODE_IDLE;
        }
    }

    em4095_power_off();
    LED_RED_OFF();
}

static void mode_rfid_125_clone(void) {
    em4095_init();
    em4095_power_on();
    LED_RED_ON();
    LED_GREEN_ON();

    /* Read, then write to T5577 */
    while (g_mode == MODE_RFID_125_CLONE) {
        if (em4095_read_em4100(&g_rfid_ctx)) {
            uart4_puts("CLONE:READ_OK\r\n");
            if (em4095_write_t5577(&g_rfid_ctx, NULL)) {
                uart4_puts("CLONE:WRITE_OK\r\n");
                LED_BLUE_ON();
                delay_ms(500);
                LED_BLUE_OFF();
            } else {
                uart4_puts("CLONE:WRITE_FAIL\r\n");
            }
        }

        if (g_sw1_flag) {
            g_sw1_flag = 0;
            g_mode = MODE_IDLE;
        }
    }

    em4095_power_off();
    LED_RED_OFF();
    LED_GREEN_OFF();
}

/* ======================================================================
 * Main Entry Point
 * ====================================================================== */
int main(void) {
    /* Step 1: Initialize clocks */
    /* HSE → PLL → 120 MHz SYSCLK (see clock_init in board.h docs) */
    /* (Full clock init code in production — simplified here) */

    /* Enable HSI16 as fallback, then HSE+PLL */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    /* Enable power clock */
    RCC->APB1ENR1 |= (1 << 28);  /* PWREN */

    /* Configure flash wait states for 120 MHz */
    FLASH->ACR = (5 << FLASH_ACR_LATENCY_Pos) | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL: HSE/1 × 60 /2 = 120 MHz, Q=5 for USB 48MHz */
    RCC->PLLCFGR = (1 << 4)    /* PLLM = 1 */
                  | (60 << 8)   /* PLLN = 60 */
                  | (5 << 21)   /* PLLQ = 5 */
                  | (0 << 25)   /* PLLR = 2 */
                  | (1 << 0);   /* PLLSRC = HSE */

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Switch SYSCLK to PLL */
    RCC->CFGR = (RCC->CFGR & ~0x3) | 0x3;  /* PLL as SYSCLK */
    while ((RCC->CFGR & 0xC) != 0xC);       /* Wait for PLL as SYSCLK */

    /* Enable peripheral clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                     RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN |
                     RCC_AHB2ENR_GPIOEEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN | RCC_APB1ENR1_USART1EN |
                      RCC_APB1ENR1_USART4EN | RCC_APB1ENR1_I2C1EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SYSCFGEN;

    /* Step 2: Initialize GPIO */
    /* (GPIO init as defined in board.h — simplified for brevity) */

    /* Step 3: Initialize SysTick (1ms) */
    SysTick_Config(SYSTEM_CLOCK / 1000);

    /* Step 4: Initialize peripherals */
    spi_init();
    uart4_init();
    i2c1_init();

    /* Step 5: Initialize drivers */
    pn5180_reset();
    pn5180_init();

    /* Step 6: Release NRF52832 from reset */
    NRF_RST_LOW();
    delay_ms(1);
    NRF_RST_HIGH();
    delay_ms(100);  /* Wait for SoftDevice init */

    /* Step 7: Initialize display */
    oled_init();
    oled_clear();
    oled_refresh();

    /* Step 8: Enable interrupts */
    exti_init();

    /* Step 9: Show splash */
    LED_GREEN_ON();
    delay_ms(200);
    LED_RED_ON();
    delay_ms(200);
    LED_BLUE_ON();
    delay_ms(200);
    LED_GREEN_OFF();
    LED_RED_OFF();
    LED_BLUE_OFF();

    /* Step 10: Main loop */
    while (1) {
        switch (g_mode) {
            case MODE_IDLE:
                mode_idle();
                break;
            case MODE_NFC_READER:
                mode_nfc_reader();
                break;
            case MODE_NFC_EMULATE:
                mode_nfc_emulate();
                break;
            case MODE_NFC_SNIFFER:
                mode_nfc_sniffer();
                break;
            case MODE_NFC_RELAY:
                mode_nfc_relay();
                break;
            case MODE_RFID_125_READ:
                mode_rfid_125_read();
                break;
            case MODE_RFID_125_CLONE:
                mode_rfid_125_clone();
                break;
            case MODE_MENU:
            case MODE_SETTINGS:
            default:
                g_mode = MODE_IDLE;
                break;
        }
    }

    return 0;
}