/*
 * board.h — WireReaper board pin map and peripheral assignments
 *
 * Target MCU: STM32H743VIT6 (LQFP100)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * This file defines every GPIO pin, peripheral clock, and board-level
 * configuration constant for the WireReaper multi-bus infiltrator.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Author / version metadata ---- */
#define WR_AUTHOR          "jayis1"
#define WR_VERSION_MAJOR   1
#define WR_VERSION_MINOR   0
#define WR_VERSION_PATCH   0
#define FW_VERSION_STRING  "1.0.0-jayis1"

/* ---- System clock tree (STM32H743) ---- */
#define HSE_FREQ_HZ        8000000UL   /* 8 MHz external crystal */
#define SYSCLK_HZ          480000000UL /* 480 MHz max for H743 */
#define HCLK_HZ            240000000UL /* AHB /2 = 240 MHz */
#define APB1_HZ            120000000UL /* APB1 /2 = 120 MHz */
#define APB2_HZ            120000000UL /* APB2 /2 = 120 MHz */
#define APB1_TIM_HZ        240000000UL /* APB1 timer clock = 2x APB1 */
#define APB2_TIM_HZ        240000000UL /* APB2 timer clock = 2x APB2 */

/* ---- GPIO port base addresses (AHB1) ---- */
#define GPIOA_BASE   0x58020000UL
#define GPIOB_BASE   0x58020400UL
#define GPIOC_BASE   0x58020800UL
#define GPIOD_BASE   0x58020C00UL
#define GPIOE_BASE   0x58021000UL
#define GPIOF_BASE   0x58021400UL
#define GPIOG_BASE   0x58021800UL
#define GPIOH_BASE   0x58021C00UL

/* ---- USART assignments ---- */
/* USART1 = BLE (nRF52840) — PA9(TX)/PA10(RX) @ 1 Mbps */
#define BLE_USART_BASE      0x40011000UL
#define BLE_USART_BAUD      1000000UL
#define BLE_TX_PIN          9
#define BLE_RX_PIN          10
#define BLE_TX_PORT         GPIOA_BASE
#define BLE_RX_PORT         GPIOA_BASE

/* USART2 = UART probe channel 0 — PD5(TX)/PD6(RX) */
#define UART0_USART_BASE    0x40004400UL
#define UART0_TX_PIN        5
#define UART0_RX_PIN        6
#define UART0_TX_PORT       GPIOD_BASE
#define UART0_RX_PORT       GPIOD_BASE

/* USART3 = UART probe channel 1 — PD8(TX)/PD9(RX) */
#define UART1_USART_BASE    0x40004800UL
#define UART1_TX_PIN        8
#define UART1_RX_PIN        9
#define UART1_TX_PORT       GPIOD_BASE
#define UART1_RX_PORT       GPIOD_BASE

/* UART4 = UART probe channel 2 — PA0(TX)/PA1(RX) */
#define UART2_UART_BASE     0x40004C00UL
#define UART2_TX_PIN        0
#define UART2_RX_PIN        1
#define UART2_TX_PORT       GPIOA_BASE
#define UART2_RX_PORT       GPIOA_BASE

/* UART5 = UART probe channel 3 — PB13(TX)/PB14(RX) */
#define UART3_UART_BASE     0x40005000UL
#define UART3_TX_PIN        13
#define UART3_RX_PIN        14
#define UART3_TX_PORT       GPIOB_BASE
#define UART3_RX_PORT       GPIOB_BASE

/* ---- I2C assignments ---- */
/* I2C1 = probe channel 0 — PB6(SCL)/PB7(SDA) */
#define I2C0_BASE           0x40005400UL
#define I2C0_SCL_PIN        6
#define I2C0_SDA_PIN        7
#define I2C0_PORT           GPIOB_BASE

/* I2C2 = probe channel 1 — PB10(SCL)/PB11(SDA) */
#define I2C1_BASE           0x40005800UL
#define I2C1_SCL_PIN        10
#define I2C1_SDA_PIN        11
#define I2C1_PORT           GPIOB_BASE

/* I2C3 = OLED display + fuel gauge (internal) — PC0(SCL)/PC1(SDA) */
#define I2C_INTERNAL_BASE   0x40005C00UL
#define I2C_INT_SCL_PIN     0
#define I2C_INT_SDA_PIN     1
#define I2C_INT_PORT        GPIOC_BASE

/* ---- SPI assignments ---- */
/* SPI1 = FPGA interface — PB3(SCK)/PB4(MISO)/PB5(MOSI)/PA15(NCS) */
#define SPI_FPGA_BASE       0x40013000UL
#define SPI_FPGA_SCK_PIN    3
#define SPI_FPGA_MISO_PIN   4
#define SPI_FPGA_MOSI_PIN   5
#define SPI_FPGA_NCS_PIN    15
#define SPI_FPGA_PORT       GPIOB_BASE
#define SPI_FPGA_NCS_PORT   GPIOA_BASE

/* SPI2 = probe channel 0 — PB12(NCS)/PB13(SCK — shared with UART3 TX via mux)
 * Actually use dedicated pins: PI1(SCK)/PI2(MISO)/PI3(MOSI)/PB12(NCS) */
#define SPI0_BASE           0x40003800UL
#define SPI0_SCK_PIN        1
#define SPI0_MISO_PIN       2
#define SPI0_MOSI_PIN       3
#define SPI0_NCS_PIN        12
#define SPI0_PORT           GPIOI_BASE
#define SPI0_NCS_PORT       GPIOB_BASE

/* SPI3 = probe channel 1 — PC10(SCK)/PC11(MISO)/PC12(MOSI)/PA4(NCS) */
#define SPI1_BASE           0x40003C00UL
#define SPI1_SCK_PIN        10
#define SPI1_MISO_PIN       11
#define SPI1_MOSI_PIN       12
#define SPI1_NCS_PIN        4
#define SPI1_PORT           GPIOC_BASE
#define SPI1_NCS_PORT       GPIOA_BASE

/* ---- 1-Wire ---- */
#define ONEWIRE_PIN         2
#define ONEWIRE_PORT        GPIOC_BASE

/* ---- USB ULPI (USB3300) ---- */
#define USB_BASE            0x40080000UL
/* ULPI pins: PA3(STP), PA5(CLK), PB0(D0), PB1(D1), PB10(D2), PB11(D3),
 * PB12(D4), PB13(D5), PB14(D6), PB15(D7), PC0(NXT), PC1(DIR)
 * These overlap with some probe pins; USB uses ULPI alternate function. */

/* ---- FMC SDRAM ---- */
#define FMC_BASE            0xA0000000UL
#define SDRAM_BANK_ADDR     0xD0000000UL
#define SDRAM_SIZE_BYTES    (512UL * 1024 * 1024)
#define SDRAM_BANK          1  /* Bank 1 = SDNE0 */

/* ---- SD card (SDMMC1) ---- */
#define SDMMC1_BASE         0x40016000UL
#define SD_SDMMC_BASE       SDMMC1_BASE
/* SDMMC1 pins: PC8(D0), PC9(D1), PC10(D2), PC11(D3), PC12(CLK), PD2(CMD) */
#define SD_DETECT_PIN       13
#define SD_DETECT_PORT      GPIOC_BASE

/* ---- OLED display ---- */
#define OLED_I2C_ADDR       0x3C
#define OLED_WIDTH          128
#define OLED_HEIGHT         64

/* ---- Fuel gauge ---- */
#define FUEL_GAUGE_ADDR     0x36

/* ---- Tactile buttons ---- */
#define BTN_UP_PIN          0
#define BTN_DOWN_PIN        1
#define BTN_SELECT_PIN      2
#define BTN_TRIGGER_PIN     3
#define BTN_PORT            GPIOE_BASE

/* ---- LED indicators ---- */
#define LED_STATUS_PIN      1
#define LED_CAPTURE_PIN     2
#define LED_PORT            GPIOE_BASE

/* ---- FPGA config ---- */
#define FPGA_CDONE_PIN      7
#define FPGA_CRESET_PIN     8
#define FPGA_INIT_PIN       9
#define FPGA_GPIO_PORT      GPIOD_BASE

/* ---- Probe header pinout (14-pin 0.1") ----
 *  1: GND          8: GND
 *  2: 3.3V         9: I2C0_SCL
 *  3: I2C0_SDA     10: I2C1_SCL
 *  4: I2C1_SDA     11: SPI0_NCS
 *  5: SPI0_SCK     12: SPI0_MISO
 *  6: SPI0_MOSI    13: SPI1_SCK
 *  7: UART0_TX     14: UART0_RX
 * (1-Wire on separate pad, UART1-3 on secondary header)
 */

/* ---- Channel count limits ---- */
#define NUM_I2C_CHANNELS    2
#define NUM_SPI_CHANNELS    2
#define NUM_UART_CHANNELS   4
#define NUM_ONEWIRE_CHANS   1
#define NUM_TOTAL_CHANNELS  (NUM_I2C_CHANNELS + NUM_SPI_CHANNELS + \
                             NUM_UART_CHANNELS + NUM_ONEWIRE_CHANS)

/* ---- Capture buffer ---- */
#define CAPTURE_BUF_SIZE    SDRAM_SIZE_BYTES
#define CAPTURE_RECORD_SIZE 16  /* bytes per decoded transaction */
#define MAX_RECORDS         (CAPTURE_BUF_SIZE / CAPTURE_RECORD_SIZE)

/* ---- BLE / USB command buffer sizes ---- */
#define CMD_BUF_SIZE        256
#define STREAM_CHUNK_SIZE   64
#define BLE_MTU             247

/* ---- Error codes ---- */
typedef enum {
    WR_OK = 0,
    WR_ERR_BUSY = -1,
    WR_ERR_PARAM = -2,
    WR_ERR_TIMEOUT = -3,
    WR_ERR_NACK = -4,
    WR_ERR_OVERFLOW = -5,
    WR_ERR_NOT_CONNECTED = -6,
    WR_ERR_NO_TARGET = -7,
    WR_ERR_HW_FAULT = -8,
} wr_err_t;

/* ---- Channel types ---- */
typedef enum {
    CH_TYPE_I2C = 0,
    CH_TYPE_SPI = 1,
    CH_TYPE_UART = 2,
    CH_TYPE_ONEWIRE = 3,
} ch_type_t;

/* ---- Channel state ---- */
typedef enum {
    CH_IDLE = 0,
    CH_SNIFFING = 1,
    CH_MASTER = 2,
    CH_SLAVE = 3,
    CH_FUZZING = 4,
} ch_state_t;

#endif /* BOARD_H */