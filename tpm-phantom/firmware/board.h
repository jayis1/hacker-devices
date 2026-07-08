/*
 * tpm-phantom — board.h
 * Pin assignments and hardware constants for the tpm-phantom board.
 *
 * Author: jayis1
 * License: GPL-2.0
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef TPM_PHANTOM_BOARD_H
#define TPM_PHANTOM_BOARD_H

#include "registers.h"

/* ===================================================================
 * Board identification
 * =================================================================== */
#define BOARD_NAME            "tpm-phantom"
#define BOARD_REV             "1.0"
#define BOARD_AUTHOR          "jayis1"

/* ===================================================================
 * Clock configuration — HSE 25 MHz → PLL → 480 MHz SYSCLK
 * (STM32H743 RM0433 reference PLL setup)
 * =================================================================== */
#define HSE_VALUE_HZ          25000000UL
#define HSI_VALUE_HZ          64000000UL
#define CSI_VALUE_HZ          4000000UL

#define SYSCLK_HZ             480000000UL
#define AHB_CLK_HZ            240000000UL   /* D1CPRE /2 */
#define APB1_CLK_HZ           120000000UL   /* PCLK1 = D2PPRE1 /2 */
#define APB2_CLK_HZ           120000000UL   /* PCLK2 = D2PPRE2 /2 */
#define APB4_CLK_HZ           120000000UL   /* D3PPRE /2 */

/* PLL1: HSE /5 * 96 /2 = 480 MHz (VCO 960 MHz) */
#define PLL1_M                5
#define PLL1_N                96
#define PLL1_P                2
#define PLL1_Q                8
#define PLL1_R                2

/* ===================================================================
 * LED and button pins
 * =================================================================== */
#define LED_STATUS_PORT       GPIOE
#define LED_STATUS_PIN        1            /* PE1 — green status */
#define LED_CAPTURE_PORT      GPIOE
#define LED_CAPTURE_PIN       2            /* PE2 — blue capture active */
#define LED_ERROR_PORT        GPIOE
#define LED_ERROR_PIN         3            /* PE3 — red error */
#define LED_LPC_PORT          GPIOE
#define LED_LPC_PIN           4            /* PE4 — LPC mode indicator */

#define BUTTON_PORT           GPIOC
#define BUTTON_PIN            13           /* PC13 — user button (active low) */

/* ===================================================================
 * LPC TPM capture pins (GPIO bus-snoop mode)
 *
 *  LCLK  — PA8  (TIM1_CH1 alt function for clock-counting fallback)
 *  LFRAME — PA9
 *  LAD[0] — PB0
 *  LAD[1] — PB1
 *  LAD[2] — PB2
 *  LAD[3] — PB3
 *  LPCPD# — PA10 (power-down / LPME#)
 *  SERIRQ — PB4
 *  CLKRUN# — PA15 (optional, pulled low by host)
 * =================================================================== */
#define LPC_LCLK_PORT         GPIOA
#define LPC_LCLK_PIN          8
#define LPC_LFRAME_PORT       GPIOA
#define LPC_LFRAME_PIN        9
#define LPC_LAD0_PORT         GPIOB
#define LPC_LAD0_PIN          0
#define LPC_LAD1_PORT         GPIOB
#define LPC_LAD1_PIN          1
#define LPC_LAD2_PORT         GPIOB
#define LPC_LAD2_PIN          2
#define LPC_LAD3_PORT         GPIOB
#define LPC_LAD3_PIN          3
#define LPC_LPCPD_PORT        GPIOA
#define LPC_LPCPD_PIN         10
#define LPC_SERIRQ_PORT       GPIOB
#define LPC_SERIRQ_PIN        4
#define LPC_CLKRUN_PORT       GPIOA
#define LPC_CLKRUN_PIN        15

/* ===================================================================
 * SPI TPM capture pins (SPI slave mode on SPI2)
 *
 *  CS#   — PB12 (SPI2_NSS, GPIO rising-edge interrupt)
 *  SCLK  — PB10 (SPI2_SCK)
 *  COPI  — PB15 (SPI2_MOSI)
 *  CIPO  — PB14 (SPI2_MISO, driven by phantom for injection)
 *  IRQ#  — PA6  (TPM interrupt, monitored)
 * =================================================================== */
#define SPI_TPM_CS_PORT       GPIOB
#define SPI_TPM_CS_PIN        12
#define SPI_TPM_SCK_PORT      GPIOB
#define SPI_TPM_SCK_PIN       10
#define SPI_TPM_MOSI_PORT     GPIOB
#define SPI_TPM_MOSI_PIN      15
#define SPI_TPM_MISO_PORT     GPIOB
#define SPI_TPM_MISO_PIN      14
#define SPI_TPM_IRQ_PORT      GPIOA
#define SPI_TPM_IRQ_PIN       6

#define SPI_TPM_INST          SPI2
#define SPI_TPM_DMA_RX_STREAM DMA1_S3
#define SPI_TPM_DMA_RX_CHAN   0x9     /* channel 9 for SPI2_RX */
#define SPI_TPM_DMA_TX_STREAM DMA1_S4 /* defined in registers.h via DMA1 stream 4 */
#define SPI_TPM_DMA_TX_CHAN   0x9

/* ===================================================================
 * USB CDC (OTG-HS in FS physical mode) — PA11 (DM), PA12 (DP)
 * =================================================================== */
#define USB_DM_PORT           GPIOA
#define USB_DM_PIN            11
#define USB_DP_PORT           GPIOA
#define USB_DP_PIN            12

/* ===================================================================
 * BLE bridge UART (USART3) — PB10_TX→PB11_RX on nRF52840 side
 * (PB10 is repurposed; SPI2_SCK uses PB10 — see note in README)
 * Actually: BLE uses PD8_TX / PD9_RX (USART3 alt function AF7)
 * =================================================================== */
#define BLE_UART_INST         USART3
#define BLE_UART_TX_PORT      GPIOD
#define BLE_UART_TX_PIN       8
#define BLE_UART_RX_PORT      GPIOD
#define BLE_UART_RX_PIN       9
#define BLE_BAUDRATE          115200UL
#define BLE_nRST_PORT         GPIOD
#define BLE_nRST_PIN          10
#define BLE_IRQ_PORT          GPIOD
#define BLE_IRQ_PIN           11

/* ===================================================================
 * MicroSD (SDMMC1) — data capture storage
 *  PC8  SD_D0
 *  PC9  SD_D1
 *  PC10 SD_D2
 *  PC11 SD_D3
 *  PC12 SD_CLK
 *  PD2  SD_CMD
 * =================================================================== */
#define SD_D0_PORT            GPIOC
#define SD_D0_PIN             8
#define SD_CLK_PORT           GPIOC
#define SD_CLK_PIN            12
#define SD_CMD_PORT           GPIOD
#define SD_CMD_PIN            2
#define SD_DETECT_PORT        GPIOC
#define SD_DETECT_PIN         7            /* active-low card detect */

/* ===================================================================
 * Capture buffer sizes
 * =================================================================== */
#define CAPTURE_BUF_SIZE      (256 * 1024)  /* 256 KB ring in SRAM */
#define LPC_FRAME_MAX         64            /* max bytes per LPC transaction */
#define SPI_CHUNK_SIZE        64            /* SPI DMA chunk */
#define CAPTURE_EVENT_QUEUE   512           /* events before flush */

/* ===================================================================
 * Mode definitions
 * =================================================================== */
typedef enum {
    CAPTURE_MODE_IDLE = 0,
    CAPTURE_MODE_LPC  = 1,
    CAPTURE_MODE_SPI  = 2,
    CAPTURE_MODE_INJECT_LPC = 3,
    CAPTURE_MODE_INJECT_SPI = 4
} capture_mode_t;

/* ===================================================================
 * GPIO helper functions
 * =================================================================== */
static inline void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    uint32_t pos = pin * 2;
    uint32_t m = port->MODER;
    m &= ~(3UL << pos);
    m |= ((uint32_t)mode & 3UL) << pos;
    port->MODER = m;
}

static inline void gpio_set_otype(GPIO_TypeDef *port, uint8_t pin, uint8_t ot)
{
    port->OTYPER = (port->OTYPER & ~(1UL << pin)) | ((uint32_t)ot << pin);
}

static inline void gpio_set_speed(GPIO_TypeDef *port, uint8_t pin, uint8_t sp)
{
    uint32_t pos = pin * 2;
    port->OSPEEDR = (port->OSPEEDR & ~(3UL << pos)) | ((uint32_t)sp << pos);
}

static inline void gpio_set_pupd(GPIO_TypeDef *port, uint8_t pin, uint8_t pupd)
{
    uint32_t pos = pin * 2;
    port->PUPDR = (port->PUPDR & ~(3UL << pos)) | ((uint32_t)pupd << pos);
}

static inline void gpio_set_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af)
{
    uint32_t pos = (pin < 8) ? (pin * 4) : ((pin - 8) * 4);
    if (pin < 8)
        port->AFRL = (port->AFRL & ~(0xFUL << pos)) | ((uint32_t)af << pos);
    else
        port->AFRH = (port->AFRH & ~(0xFUL << pos)) | ((uint32_t)af << pos);
}

static inline void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t val)
{
    if (val)
        port->BSRR = 1UL << pin;
    else
        port->BSRR = 1UL << (pin + 16);
}

static inline uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR >> pin) & 1UL;
}

/* LED helpers */
static inline void led_status_on(void)   { gpio_write(LED_STATUS_PORT,  LED_STATUS_PIN,  1); }
static inline void led_status_off(void)  { gpio_write(LED_STATUS_PORT,  LED_STATUS_PIN,  0); }
static inline void led_capture_on(void)  { gpio_write(LED_CAPTURE_PORT, LED_CAPTURE_PIN, 1); }
static inline void led_capture_off(void) { gpio_write(LED_CAPTURE_PORT, LED_CAPTURE_PIN, 0); }
static inline void led_error_on(void)    { gpio_write(LED_ERROR_PORT,   LED_ERROR_PIN,   1); }
static inline void led_error_off(void)   { gpio_write(LED_ERROR_PORT,   LED_ERROR_PIN,   0); }
static inline void led_lpc_on(void)      { gpio_write(LED_LPC_PORT,     LED_LPC_PIN,     1); }
static inline void led_lpc_off(void)     { gpio_write(LED_LPC_PORT,     LED_LPC_PIN,     0); }

static inline void led_error_blink(void)
{
    for (int i = 0; i < 6; i++) {
        led_error_on();
        for (volatile int d = 0; d < 200000; d++) __asm__("nop");
        led_error_off();
        for (volatile int d = 0; d < 200000; d++) __asm__("nop");
    }
}

#endif /* TPM_PHANTOM_BOARD_H */