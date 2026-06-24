/*
 * lora-phantom / board.h
 * Pin map, peripheral assignments, and board constants for LoRaPhantom.
 * Author: jayis1
 * License: GPL-2.0
 *
 * Hardware:
 *   U1  STM32H743VIT6  (LQFP-100, Cortex-M7 @ 480 MHz)
 *   U2  SX1262IMLTRT   (LoRa transceiver, SPI6)
 *   U3  nRF52840       (BLE backhaul, USART3)
 *   U8  IS42S16160G    (16 MB SDRAM, FMC)
 *   microSD            (SDMMC1)
 *   USB-C              (USB OTG-HS @ FS)
 *
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef LORA_PHANTOM_BOARD_H
#define LORA_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock tree ---- */
#define HSE_VALUE_HZ        25000000UL   /* 25 MHz external crystal on PH0/PH1 */
#define LSE_VALUE_HZ        32768UL      /* 32.768 kHz RTC crystal */
#define SYSCLK_HZ           480000000UL  /* SYSCLK @ 480 MHz (PLL1) */
#define HCLK_HZ             240000000UL  /* HCLK / 2 = 240 MHz (AXI/AHB) */
#define APB1_HZ             120000000UL  /* APB1 = HCLK/2 */
#define APB2_HZ             120000000UL  /* APB2 = HCLK/2 */
#define APB4_HZ             120000000UL  /* APB4 = HCLK/2 */

/* ---- Flash / RAM ---- */
#define FLASH_BASE_ADDR     0x08000000UL
#define FLASH_SIZE_BYTES    (2 * 1024 * 1024)   /* 2 MB */
#define SRAM_DTCM_BASE      0x20000000UL
#define SRAM_DTCM_SIZE      (128 * 1024)         /* 128 KB DTCM */
#define SRAM_ITCM_BASE      0x00000000UL
#define SRAM_ITCM_SIZE      (64 * 1024)          /* 64 KB ITCM */
#define SRAM_AXI_BASE       0x24000000UL
#define SRAM_AXI_SIZE       (512 * 1024)         /* 512 KB AXI SRAM */
#define SDRAM_BASE_ADDR     0xC0000000UL         /* FMC Bank 5/6 SDRAM */
#define SDRAM_SIZE_BYTES    (16 * 1024 * 1024)   /* 16 MB external SDRAM */

/* ---- SX1262 LoRa radio (SPI6) ---- */
#define SX1262_SPI              SPI6
#define SX1262_NSS_PORT         GPIOG
#define SX1262_NSS_PIN          8                 /* PG8  — SPI6_NSS (manual) */
#define SX1262_BUSY_PORT        GPIOG
#define SX1262_BUSY_PIN         10                /* PG10 — BUSY */
#define SX1262_DIO1_PORT        GPIOG
#define SX1262_DIO1_PIN         9                 /* PG9  — DIO1 IRQ */
#define SX1262_RESET_PORT       GPIOB
#define SX1262_RESET_PIN        2                 /* PB2  — NRST */
#define SX1262_ANT_SW_PORT      GPIOB
#define SX1262_ANT_SW_PIN       6                 /* PB6  — RF switch (RX/TX) */
#define SX1262_SPI_CLK_HZ       18000000UL        /* 18 MHz SPI clock */

/* ---- nRF52840 BLE backhaul (USART3) ---- */
#define BLE_UART                USART3
#define BLE_UART_BAUD           115200UL
#define BLE_UART_TX_PORT        GPIOB
#define BLE_UART_TX_PIN         10                /* PB10 — USART3_TX */
#define BLE_UART_RX_PORT        GPIOB
#define BLE_UART_RX_PIN         11                /* PB11 — USART3_RX */
#define BLE_UART_CTS_PORT       GPIOB
#define BLE_UART_CTS_PIN        13                /* PB13 — USART3_CTS */
#define BLE_UART_RTS_PORT       GPIOB
#define BLE_UART_RTS_PIN        14                /* PB14 — USART3_RTS */
#define BLE_RESET_PORT          GPIOB
#define BLE_RESET_PIN           15                /* PB15 — nRF52 RESET */

/* ---- USB-C (USB OTG-HS in FS mode) ---- */
#define USB_OTG                 USB_OTG_HS
#define USB_DM_PORT             GPIOA
#define USB_DM_PIN              11                /* PA11 — USB_DM */
#define USB_DP_PORT             GPIOA
#define USB_DP_PIN              12                /* PA12 — USB_DP */
#define USB_VBUS_PORT           GPIOA
#define USB_VBUS_PIN            9                 /* PA9  — VBUS sense */
#define USB_ID_PORT             GPIOA
#define USB_ID_PIN              10                /* PA10 — OTG ID */

/* ---- microSD (SDMMC1) ---- */
#define SDMMC_DEV               SDMMC1
#define SD_CK_PORT              GPIOC
#define SD_CK_PIN               12                /* PC12 — SDMMC1_CK */
#define SD_CMD_PORT             GPIOD
#define SD_CMD_PIN              2                 /* PD2  — SDMMC1_CMD */
#define SD_D0_PORT              GPIOC
#define SD_D0_PIN               8                 /* PC8  — SDMMC1_D0 */
#define SD_D1_PORT              GPIOC
#define SD_D1_PIN               9                 /* PC9  — SDMMC1_D1 */
#define SD_D2_PORT              GPIOC
#define SD_D2_PIN               10                /* PC10 — SDMMC1_D2 */
#define SD_D3_PORT              GPIOC
#define SD_D3_PIN               11                /* PC11 — SDMMC1_D3 */
#define SD_CD_PORT              GPIOA
#define SD_CD_PIN               5                 /* PA5  — card detect (active low) */

/* ---- User I/O ---- */
#define BTN_CAPTURE_PORT        GPIOC
#define BTN_CAPTURE_PIN         0                 /* PC0 — capture trigger */
#define BTN_MODE_PORT           GPIOC
#define BTN_MODE_PIN            1                 /* PC1 — mode cycle */
#define LED_RF_PORT             GPIOE
#define LED_RF_PIN              0                 /* PE0 — RF activity */
#define LED_BLE_PORT            GPIOE
#define LED_BLE_PIN             1                 /* PE1 — BLE status */
#define LED_USB_PORT            GPIOE
#define LED_USB_PIN             2                 /* PE2 — USB status */
#define LED_ERR_PORT            GPIOE
#define LED_ERR_PIN             3                 /* PE3 — error */

/* ---- OLED (optional, I2C4) ---- */
#define OLED_I2C                I2C4
#define OLED_SCL_PORT           GPIOD
#define OLED_SCL_PIN            12                /* PD12 — I2C4_SCL */
#define OLED_SDA_PORT           GPIOD
#define OLED_SDA_PIN            13                /* PD13 — I2C4_SDA */

/* ---- Battery monitoring (ADC1) ---- */
#define BAT_ADC                 ADC1
#define BAT_ADC_CHANNEL         10                /* ADC1_IN10 = PC0-alt... use PF0 */
#define BAT_ADC_PORT            GPIOF
#define BAT_ADC_PIN             0                 /* PF0 — VBAT/3 divider */
#define BAT_DIVIDER_RATIO       3                 /* 100k/200k: Vbat = ADC * 3 * 3.3/4095 */
#define BAT_FULL_MV             4200
#define BAT_EMPTY_MV            3300

/* ---- Operational limits ---- */
#define CAPTURE_BUF_SIZE        (4 * 1024 * 1024) /* 4 MB ring in SDRAM */
#define PCAP_MAX_PAYLOAD        256
#define MAX_REGIONS             8
#define MAX_CHANNELS            72                /* US915 has 72 uplink channels */
#define KEYSEARCH_DICT_MAX      65536             /* max dictionary entries */
#define REPLAY_QUEUE_MAX        128
#define INJECT_QUEUE_MAX        32

/* ---- Regulatory duty cycle (EU868 default) ---- */
#define DEFAULT_DUTY_CYCLE_PCT  1                 /* 1% EU868 */
#define DEFAULT_TX_POWER_DBM    14
#define MAX_TX_POWER_DBM        22                /* SX1262 internal; 27 with PA */

/* ---- Status LEDs helper ---- */
#define LED_ON(port, pin)       ((port)->BSRR = (1U << (pin)))
#define LED_OFF(port, pin)      ((port)->BSRR = (1U << (pin)) << 16)
#define LED_TOGGLE(port, pin)   ((port)->ODR ^= (1U << (pin)))

/* ---- Button debounce ---- */
#define BTN_DEBOUNCE_MS         50

#endif /* LORA_PHANTOM_BOARD_H */