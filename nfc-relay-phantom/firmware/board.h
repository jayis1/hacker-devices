/*
 * NFC Relay Phantom — Board Definitions
 * STM32L4S5VIT6-based NFC/RFID security research platform
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* ======================================================================
 * Clock Configuration
 * ====================================================================== */
#define HSE_VALUE           8000000UL    /* 8 MHz external crystal */
#define LSE_VALUE           32768UL      /* 32.768 kHz RTC crystal */
#define SYSTEM_CLOCK        120000000UL  /* 120 MHz via PLL */
#define SYSCLK_FREQ         120000000UL
#define AHB_PRESCALER       1            /* AHB = SYSCLK / 1 = 120 MHz */
#define APB1_PRESCALER      1            /* APB1 = AHB / 1 = 120 MHz */
#define APB2_PRESCALER      1            /* APB2 = AHB / 1 = 120 MHz */

/* ======================================================================
 * GPIO Pin Definitions
 * ====================================================================== */

/* --- SPI1 (PN5180) --- */
#define SPI1_NSS_PORT       GPIOA
#define SPI1_NSS_PIN        4
#define SPI1_SCK_PORT       GPIOA
#define SPI1_SCK_PIN        5
#define SPI1_MISO_PORT      GPIOA
#define SPI1_MISO_PIN       6
#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MOSI_PIN       7

/* --- SPI2 (W25Q128) --- */
#define SPI2_NSS_PORT       GPIOB
#define SPI2_NSS_PIN        12
#define SPI2_SCK_PORT       GPIOB
#define SPI2_SCK_PIN        13
#define SPI2_MISO_PORT      GPIOB
#define SPI2_MISO_PIN       14
#define SPI2_MOSI_PORT      GPIOB
#define SPI2_MOSI_PIN       15

/* --- I2C1 (SSD1306 + BQ25896) --- */
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SCL_PIN        6
#define I2C1_SDA_PORT       GPIOB
#define I2C1_SDA_PIN        7

/* --- UART1 (EM4095) --- */
#define UART1_TX_PORT       GPIOA
#define UART1_TX_PIN        9
#define UART1_RX_PORT       GPIOA
#define UART1_RX_PIN        10

/* --- UART4 (NRF52832) --- */
#define UART4_TX_PORT       GPIOA
#define UART4_TX_PIN        0
#define UART4_RX_PORT       GPIOA
#define UART4_RX_PIN        1

/* --- USB --- */
#define USB_DM_PORT         GPIOA
#define USB_DM_PIN          11
#define USB_DP_PORT         GPIOA
#define USB_DP_PIN          12

/* --- PN5180 Control --- */
#define PN5180_BUSY_PORT    GPIOC
#define PN5180_BUSY_PIN     4
#define PN5180_RST_PORT     GPIOC
#define PN5180_RST_PIN      5

/* --- EM4095 Control --- */
#define EM4095_SHD_PORT     GPIOC
#define EM4095_SHD_PIN      1
#define EM4095_MOD_PORT     GPIOC
#define EM4095_MOD_PIN      2

/* --- NRF52832 Control --- */
#define NRF_RST_PORT        GPIOC
#define NRF_RST_PIN         8
#define NRF_IRQ_PORT        GPIOC
#define NRF_IRQ_PIN         0

/* --- OLED --- */
#define OLED_RST_PORT       GPIOE
#define OLED_RST_PIN        0

/* --- W25Q128 Control --- */
#define FLASH_WP_PORT       GPIOD
#define FLASH_WP_PIN        0
#define FLASH_HOLD_PORT     GPIOD
#define FLASH_HOLD_PIN      1

/* --- LEDs --- */
#define LED_GREEN_PORT      GPIOB
#define LED_GREEN_PIN       0
#define LED_RED_PORT        GPIOB
#define LED_RED_PIN         1
#define LED_BLUE_PORT       GPIOB
#define LED_BLUE_PIN        9

/* --- Buttons --- */
#define SW1_BTN_PORT        GPIOC
#define SW1_BTN_PIN         13
#define SW2_BTN_PORT        GPIOB
#define SW2_BTN_PIN         8

/* ======================================================================
 * Peripheral Base Addresses
 * ====================================================================== */
#define GPIOA_BASE          0x40020000UL
#define GPIOB_BASE          0x40020400UL
#define GPIOC_BASE          0x40020800UL
#define GPIOD_BASE          0x40020C00UL
#define GPIOE_BASE          0x40021000UL

#define SPI1_BASE           0x40013000UL
#define SPI2_BASE           0x40003800UL
#define USART1_BASE         0x40011000UL
#define USART4_BASE         0x40004C00UL
#define I2C1_BASE           0x40005400UL
#define DMA1_BASE           0x40020000UL

/* ======================================================================
 * SPI Configuration
 * ====================================================================== */
#define SPI1_CLOCK_HZ      20000000UL   /* 20 MHz for PN5180 */
#define SPI2_CLOCK_HZ      40000000UL   /* 40 MHz for W25Q128 */
#define SPI1_DMA_BUF_SIZE  4096
#define SPI2_DMA_BUF_SIZE  4096

/* ======================================================================
 * UART Configuration
 * ====================================================================== */
#define UART1_BAUD          9600UL       /* EM4095 at 9600 bps */
#define UART4_BAUD          115200UL     /* NRF52 at 115200 bps */

/* ======================================================================
 * I2C Configuration
 * ====================================================================== */
#define I2C1_CLOCK_HZ      400000UL     /* 400 kHz Fast Mode */
#define SSD1306_I2C_ADDR    0x3C
#define BQ25896_I2C_ADDR    0x6A

/* ======================================================================
 * NFC Configuration
 * ====================================================================== */
#define NFC_MAX_FRAME_SIZE  264          /* Max ISO 14443 frame (with CRC) */
#define NFC_RX_TIMEOUT_MS   100
#define NFC_FIELD_ON_TIME   5           /* ms to stabilize RF field */

/* ======================================================================
 * RFID 125 kHz Configuration
 * ====================================================================== */
#define RFID_125_BIT_BUF    512         /* Max raw bit buffer */
#define EM4100_BYTE_LEN    8
#define T5577_WRITE_DELAY   4           /* ms between write commands */

/* ======================================================================
 * SPI Flash Configuration
 * ====================================================================== */
#define FLASH_SECTOR_SIZE   4096
#define FLASH_PAGE_SIZE     256
#define FLASH_TOTAL_SIZE    (16UL * 1024UL * 1024UL)  /* 16 MB */

/* ======================================================================
 * Capture Storage Layout in W25Q128
 * ====================================================================== */
#define FLASH_ADDR_METADATA  0x000000UL  /* Sector 0: metadata */
#define FLASH_ADDR_CAPTURE   0x001000UL  /* Sector 1+: capture data */
#define FLASH_CAPTURE_SIZE   (FLASH_TOTAL_SIZE - FLASH_ADDR_CAPTURE)

/* ======================================================================
 * OLED Configuration
 * ====================================================================== */
#define OLED_WIDTH           128
#define OLED_HEIGHT          64
#define OLED_PAGES           8

/* ======================================================================
 * LED Macros
 * ====================================================================== */
#define LED_GREEN_ON()       (LED_GREEN_PORT->ODR |= (1 << LED_GREEN_PIN))
#define LED_GREEN_OFF()      (LED_GREEN_PORT->ODR &= ~(1 << LED_GREEN_PIN))
#define LED_RED_ON()         (LED_RED_PORT->ODR |= (1 << LED_RED_PIN))
#define LED_RED_OFF()        (LED_RED_PORT->ODR &= ~(1 << LED_RED_PIN))
#define LED_BLUE_ON()        (LED_BLUE_PORT->ODR |= (1 << LED_BLUE_PIN))
#define LED_BLUE_OFF()       (LED_BLUE_PORT->ODR &= ~(1 << LED_BLUE_PIN))

/* ======================================================================
 * Button Macros (active low)
 * ====================================================================== */
#define SW1_PRESSED()        (!(SW1_BTN_PORT->IDR & (1 << SW1_BTN_PIN)))
#define SW2_PRESSED()        (!(SW2_BTN_PORT->IDR & (1 << SW2_BTN_PIN)))

/* ======================================================================
 * NSS Control Macros
 * ====================================================================== */
#define SPI1_NSS_LOW()       (SPI1_NSS_PORT->ODR &= ~(1 << SPI1_NSS_PIN))
#define SPI1_NSS_HIGH()      (SPI1_NSS_PORT->ODR |= (1 << SPI1_NSS_PIN))
#define SPI2_NSS_LOW()       (SPI2_NSS_PORT->ODR &= ~(1 << SPI2_NSS_PIN))
#define SPI2_NSS_HIGH()      (SPI2_NSS_PORT->ODR |= (1 << SPI2_NSS_PIN))

/* ======================================================================
 * PN5180 Control Macros
 * ====================================================================== */
#define PN5180_RST_LOW()     (PN5180_RST_PORT->ODR &= ~(1 << PN5180_RST_PIN))
#define PN5180_RST_HIGH()    (PN5180_RST_PORT->ODR |= (1 << PN5180_RST_PIN))
#define PN5180_BUSY_READ()   ((PN5180_BUSY_PORT->IDR >> PN5180_BUSY_PIN) & 1)

/* ======================================================================
 * EM4095 Control Macros
 * ====================================================================== */
#define EM4095_SHD_LOW()     (EM4095_SHD_PORT->ODR &= ~(1 << EM4095_SHD_PIN))
#define EM4095_SHD_HIGH()   (EM4095_SHD_PORT->ODR |= (1 << EM4095_SHD_PIN))
#define EM4095_MOD_HIGH()   (EM4095_MOD_PORT->ODR |= (1 << EM4095_MOD_PIN))
#define EM4095_MOD_LOW()    (EM4095_MOD_PORT->ODR &= ~(1 << EM4095_MOD_PIN))

/* ======================================================================
 * NRF52832 Control Macros
 * ====================================================================== */
#define NRF_RST_LOW()        (NRF_RST_PORT->ODR &= ~(1 << NRF_RST_PIN))
#define NRF_RST_HIGH()      (NRF_RST_PORT->ODR |= (1 << NRF_RST_PIN))
#define NRF_IRQ_READ()      ((NRF_IRQ_PORT->IDR >> NRF_IRQ_PIN) & 1)

#endif /* BOARD_H */