/*
 * board.h — USB DMA Phantom board definitions
 * STM32F423CHU6-based Thunderbolt/USB4 DMA attack platform
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock configuration ---- */
#define HSE_VALUE       25000000UL   /* 25 MHz external crystal */
#define SYSCLK_FREQ     120000000UL  /* 120 MHz system clock */
#define HCLK_FREQ       120000000UL  /* 120 MHz AHB clock */
#define APB1_FREQ       60000000UL   /* 60 MHz APB1 (HCLK/2) */
#define APB2_FREQ       60000000UL   /* 60 MHz APB2 (HCLK/2) */
#define SPI4_CLK_FREQ   30000000UL   /* 30 MHz SPI4 (APB1/2) */
#define UART4_BAUD      1000000UL    /* 1 Mbps UART4 to nRF52832 */

/* ---- GPIO pin assignments ---- */

/* Port A */
#define GPIOA_SPI4_SCK      5    /* PA5  - SPI4 clock (flash + XIO2001) */
#define GPIOA_SPI4_MISO      6    /* PA6  - SPI4 MISO */
#define GPIOA_SPI4_MOSI      7    /* PA7  - SPI4 MOSI */
#define GPIOA_UART4_RX       1    /* PA1  - UART4 RX from nRF52832 */
#define GPIOA_UART4_TX       2    /* PA2  - UART4 TX to nRF52832 */
#define GPIOA_UART4_RTS      3    /* PA3  - UART4 RTS */
#define GPIOA_UART4_CTS      4    /* PA4  - UART4 CTS */
#define GPIOA_NRF_RESET      0    /* PA0  - nRF52832 RESET# (active low) */
#define GPIOA_XIO_WAKE       11   /* PA11 - XIO2001 WAKE# */
#define GPIOA_HD3SS_CTL2     12   /* PA12 - HD3SS460 CTL2 */
#define GPIOA_USB_VBUS       13   /* PA13 - SWDIO (also USB VBUS sense) */
#define GPIOA_SWD_CLK        14   /* PA14 - SWCLK */
#define GPIOA_FLASH_WP       15   /* PA15 - Flash WP# */

/* Port B */
#define GPIOB_I2C1_SCL       0    /* PB0  - I2C1 SCL (HD3SS460 + microSD) */
#define GPIOB_I2C1_SDA       1    /* PB1  - I2C1 SDA */
#define GPIOB_SD_CMD          2    /* PB2  - microSD CMD */
#define GPIOB_SD_CLK          10   /* PB10 - microSD CLK */
#define GPIOB_SD_D0           11   /* PB11 - microSD D0 */
#define GPIOB_SD_D1           12   /* PB12 - microSD D1 */
#define GPIOB_USB_VBUS       13   /* PB13 - USB VBUS sense */
#define GPIOB_USB_DM          14   /* PB14 - USB D- */
#define GPIOB_USB_DP          15   /* PB15 - USB D+ */

/* Port C */
#define GPIOC_MODE_BTN       13   /* PC13 - Mode select button (active low) */
#define GPIOC_WS2812_DATA    14   /* PC14 - WS2812B RGB LED data */
#define GPIOC_XIO_PERST      15   /* PC15 - XIO2001 PERST# (active low) */
#define GPIOC_VBUS_SENSE     0    /* PC0  - ADC1_IN10, VBUS voltage sense */
#define GPIOC_SD_DETECT      1    /* PC1  - microSD card detect */
#define GPIOC_XIO_CLKREQ     2    /* PC2  - XIO2001 CLKREQ# */
#define GPIOC_HD3SS_CTL1     3    /* PC3  - HD3SS460 CTL1 */
#define GPIOC_XIO_SPI_CS     4    /* PC4  - XIO2001 SPI CS# */
#define GPIOC_FLASH_CS       5    /* PC5  - W25Q128 SPI CS# */
#define GPIOC_XIO_SERR       6    /* PC6  - XIO2001 SERR# */
#define GPIOC_XIO_INTA       7    /* PC7  - XIO2001 INTA# */
#define GPIOC_HD3SS_CTL3     8    /* PC8  - HD3SS460 CTL3 */
#define GPIOC_HD3SS_CTL4     9    /* PC9  - HD3SS460 CTL4 / FLT# */

/* ---- SPI chip select masks ---- */
#define SPI4_CS_FLASH     (1U << GPIOC_FLASH_CS)   /* PC5 */
#define SPI4_CS_XIO       (1U << GPIOC_XIO_SPI_CS)  /* PC4 */

/* ---- LED patterns ---- */
#define LED_PATTERN_IDLE      0x01   /* Slow blue pulse */
#define LED_PATTERN_DMA_ACTIVE 0x02  /* Fast red flash */
#define LED_PATTERN_BLE_CONN   0x03  /* Solid green */
#define LED_PATTERN_USB_CONN   0x04  /* Cyan pulse */
#define LED_PATTERN_ERROR      0x05  /* Fast red blink */
#define LED_PATTERN_CONFIG     0x06   /* Yellow solid */

/* ---- Operating modes ---- */
#define MODE_STEALTH_DMA    0x01   /* Auto DMA on TBT connect */
#define MODE_INTERACTIVE    0x02   /* Real-time BLE/USB C2 */
#define MODE_CONFIG          0x03   /* USB CDC configuration */
#define MODE_SNIFFER         0x04   /* Passive PCIe TLP sniffing */

/* ---- DMA commands ---- */
#define DMA_CMD_READ         0x01
#define DMA_CMD_WRITE         0x02
#define DMA_CMD_SCAN          0x03
#define DMA_CMD_INJECT        0x04

/* ---- DMA request packet format (over UART/BLE) ---- */
typedef struct __attribute__((packed)) {
    uint8_t  magic;          /* 0xD4 */
    uint8_t  cmd;            /* DMA_CMD_READ/WRITE/SCAN/INJECT */
    uint8_t  seq;            /* Sequence number */
    uint8_t  flags;          /* Flags (0x01=async, 0x02=encrypted) */
    uint64_t host_addr;      /* Target physical address */
    uint32_t length;         /* Transfer length */
    uint8_t  data[];         /* Variable-length payload */
} dma_packet_t;

#define DMA_MAGIC    0xD4
#define DMA_RESP_OK  0xA0
#define DMA_RESP_ERR 0xA1

/* ---- Payload descriptor (stored in W25Q128 sector 0) ---- */
#define PAYLOAD_MAGIC        0x50414C44   /* "PLDD" */
#define PAYLOAD_OFFSET_BASE  0x1000       /* Payloads start at 4 KB */

typedef struct __attribute__((packed)) {
    uint32_t magic;           /* PAYLOAD_MAGIC */
    uint16_t vid;             /* PCIe Vendor ID to emulate */
    uint16_t pid;             /* PCIe Device ID to emulate */
    uint16_t payload_count;   /* Number of stored payloads */
    uint16_t reserved;
    uint32_t payload_offsets[16];
    uint32_t payload_sizes[16];
    uint8_t  hmac_key[32];   /* HMAC-SHA256 key */
    uint8_t  device_name[32]; /* Null-terminated device name */
    uint8_t  config_space[256]; /* PCIe config space template */
} payload_descriptor_t;

/* ---- Function prototypes ---- */
void clock_init(void);
void gpio_init(void);
void led_init(void);
void led_set_pattern(uint8_t pattern);
void mode_button_init(void);
uint8_t mode_button_read(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif /* BOARD_H */