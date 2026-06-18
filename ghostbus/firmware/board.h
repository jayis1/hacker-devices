/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Board Configuration — STM32H563ZIT6 reference board
 *
 * Pin assignments, probe channel routing, and hardware constants.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GHOSTBUS_BOARD_H
#define GHOSTBUS_BOARD_H

#include "registers.h"

/* =========================================================================
 * Firmware version
 * ========================================================================= */

#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION_STRING   "1.0.0"

/* =========================================================================
 * System clock (STM32H563 — PLL1 to 250 MHz on HSI 64 MHz)
 * PLL1: HSI / M=8 * N=125 / P=4 = 64/8*125/4 = 250 MHz
 * ========================================================================= */

#define SYSTEM_CLOCK_HZ     250000000UL
#define HSI_VALUE_HZ        64000000UL
#define HSE_VALUE_HZ        48000000UL

/* =========================================================================
 * Probe channel GPIO map
 *
 * The probe head has 8 pogo-pin channels routed to GPIO.
 * Each channel is software-routable to SWD/JTAG/cJTAG/power-sense.
 *
 * Port   Pin   Channel   Default role
 * GPIOA   PA0   CH0       SWCLK / TCK
 * GPIOA   PA1   CH1       SWDIO / TMS (bidirectional)
 * GPIOB   PB0   CH2       TDI
 * GPIOB   PB1   CH3       TDO
 * GPIOC   PC0   CH4       VRef sense (ADC)
 * GPIOC   PC1   CH5       Target power injection (analog/gate)
 * GPIOC   PC2   CH6       Continuity drive (impedance test)
 * GPIOC   PC3   CH7       GND sense
 * ========================================================================= */

typedef enum {
    PROBE_CH0 = 0,   /* SWCLK / TCK    */
    PROBE_CH1 = 1,   /* SWDIO / TMS   */
    PROBE_CH2 = 2,   /* TDI           */
    PROBE_CH3 = 3,   /* TDO           */
    PROBE_CH4 = 4,   /* VREF sense    */
    PROBE_CH5 = 5,   /* Target power  */
    PROBE_CH6 = 6,   /* Continuity    */
    PROBE_CH7 = 7,   /* GND sense     */
    PROBE_CH_MAX = 8
} probe_channel_t;

/* GPIO port/pin for each channel */
typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
    uint8_t       adc_channel; /* 0xFF if not an ADC channel */
} probe_pinmap_t;

static const probe_pinmap_t probe_pinmap[PROBE_CH_MAX] = {
    [PROBE_CH0] = { GPIOA, 0,  0xFF },     /* SWCLK/TCK  */
    [PROBE_CH1] = { GPIOA, 1,  0xFF },     /* SWDIO/TMS */
    [PROBE_CH2] = { GPIOB, 0,  0xFF },     /* TDI        */
    [PROBE_CH3] = { GPIOB, 1,  0xFF },     /* TDO        */
    [PROBE_CH4] = { GPIOC, 0,  1 },        /* VRef ADC1  */
    [PROBE_CH5] = { GPIOC, 1,  2 },        /* Tgt power ADC2 */
    [PROBE_CH6] = { GPIOC, 2,  3 },        /* Continuity ADC3 */
    [PROBE_CH7] = { GPIOC, 3,  4 },        /* GND ADC4 */
};

/* =========================================================================
 * LED & button
 * ========================================================================= */

#define LED_PORT        GPIOE
#define LED_PIN          13
#define LED_STATUS_R     14  /* RGB red   */
#define LED_STATUS_G     15  /* RGB green */
#define LED_STATUS_B     12  /* RGB blue  */

#define BUTTON_PORT      GPIOC
#define BUTTON_PIN        13

/* =========================================================================
 * BLE co-processor UART bridge (USART1 on PA9/PA10)
 * ========================================================================= */

#define BLE_UART          USART1
#define BLE_UART_BAUD      460800
#define BLE_TX_PORT        GPIOA
#define BLE_TX_PIN         9
#define BLE_RX_PORT        GPIOA
#define BLE_RX_PIN         10
#define BLE_UART_AF         7    /* AF7 = USART1 on STM32H5 */

/* BLE nRST and IRQ lines */
#define BLE_NRST_PORT      GPIOB
#define BLE_NRST_PIN       5
#define BLE_IRQ_PORT       GPIOB
#define BLE_IRQ_PIN        6

/* =========================================================================
 * Debug logging UART (USART3 on PD8/PD9) — for developer console only
 * ========================================================================= */

#define DBG_UART           ((USART_TypeDef *)(PERIPH_BASE_APB1 + 0x4800))
#define DBG_UART_BAUD      115200

/* =========================================================================
 * Battery fuel gauge (MAX17048) on I2C1
 * ========================================================================= */

#define I2C_FUEL_GAUGE     ((I2C_TypeDef *) I2C1_BASE)
#define FUELGAUGE_ADDR     0x36  /* 7-bit MAX17048 I2C address */

/* =========================================================================
 * Crypto session parameters
 * ========================================================================= */

#define SESSION_KEY_LEN    32    /* AES-256 */
#define SESSION_NONCE_LEN   12
#define SESSION_TAG_LEN     16
#define ECDH_PUBKEY_LEN     64    /* P-256 uncompressed X||Y */
#define ECDH_PRIVKEY_LEN    32

/* =========================================================================
 * Memory limits
 * ========================================================================= */

#define BLE_MTU                247
#define BLE_DATA_CHUNK_PAYLOAD 180
#define BLE_DATA_HEADER_LEN    20
#define BLE_MAX_CHUNK_LEN      (BLE_DATA_HEADER_LEN + BLE_DATA_CHUNK_PAYLOAD)

#define FLASH_BLOCK_SIZE       4096
#define FLASH_MAX_DUMP_BYTES   (1U * 1024 * 1024)  /* 1 MB cap per dump */

#define DISCOVERY_TIMEOUT_MS   5000

/* =========================================================================
 * Status codes
 * ========================================================================= */

typedef enum {
    GB_OK = 0,
    GB_ERR_PARAM = -1,
    GB_ERR_TIMEOUT = -2,
    GB_ERR_NO_TARGET = -3,
    GB_ERR_PARITY = -4,
    GB_ERR_LOCKED = -5,
    GB_ERR_RDP = -6,
    GB_ERR_BLE = -7,
    GB_ERR_CRYPTO = -8,
    GB_ERR_NO_MEMORY = -9,
    GB_ERR_UNSUPPORTED = -10,
    GB_ERR_PROTOCOL = -11
} gb_status_t;

/* =========================================================================
 * Protocol enumeration
 * ========================================================================= */

typedef enum {
    PROTO_UNKNOWN = 0,
    PROTO_SWD,
    PROTO_JTAG,
    PROTO_CJTAG,
    PROTO_POWER_ONLY
} probe_protocol_t;

/* =========================================================================
 * SoC identification record (from DP/JTAG IDCODE lookup)
 * ========================================================================= */

typedef struct {
    uint32_t    idcode;
    const char *vendor;
    const char *family;
    uint32_t    flash_base;
    uint32_t    sram_base;
    uint8_t     rdp_register_offset; /* offset into debug peripheral region */
    uint8_t     exploit_id;         /* 0 = none */
} soc_id_record_t;

/* =========================================================================
 * GPIO helper inlines
 * ========================================================================= */

static inline void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    port->MODER = (port->MODER & ~(0x3UL << (pin * 2))) | ((uint32_t)mode << (pin * 2));
}

static inline void gpio_set_pupd(GPIO_TypeDef *port, uint8_t pin, uint8_t pupd)
{
    port->PUPDR = (port->PUPDR & ~(0x3UL << (pin * 2))) | ((uint32_t)pupd << (pin * 2));
}

static inline void gpio_set_otyper(GPIO_TypeDef *port, uint8_t pin, uint8_t otype)
{
    port->OTYPER = (port->OTYPER & ~(0x1UL << pin)) | ((uint32_t)otype << pin);
}

static inline void gpio_set_ospeed(GPIO_TypeDef *port, uint8_t pin, uint8_t ospeed)
{
    port->OSPEEDR = (port->OSPEEDR & ~(0x3UL << (pin * 2))) | ((uint32_t)ospeed << (pin * 2));
}

static inline void gpio_set(GPIO_TypeDef *port, uint8_t pin)
{
    port->BSRR = 1U << pin;
}

static inline void gpio_clear(GPIO_TypeDef *port, uint8_t pin)
{
    port->BRR = 1U << pin;
}

static inline uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (uint8_t)((port->IDR >> pin) & 0x1U);
}

static inline void gpio_set_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af)
{
    if (pin < 8) {
        port->AFRL = (port->AFRL & ~(0xFUL << (pin * 4))) | ((uint32_t)af << (pin * 4));
    } else {
        uint8_t p2 = (uint8_t)(pin - 8);
        port->AFRH = (port->AFRH & ~(0xFUL << (p2 * 4))) | ((uint32_t)af << (p2 * 4));
    }
}

#endif /* GHOSTBUS_BOARD_H */