/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Board Pin Definitions and Peripheral Mapping
 *
 * Defines all GPIO pin assignments, alternate function mappings,
 * peripheral channel assignments, and board-level constants.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

/* =========================================================================
 * Pin Mux Table Summary
 * =========================================================================
 *
 * Peripheral  | Signal   | Port | Pin | AF  | Notes
 * ------------|----------|------|-----|-----|---------------------------
 * I2S1 (Tx)   | CK       | PA5  | 5   | AF5 | I2S1_SCK / SPI1_SCK
 *              | SD       | PA7  | 7   | AF5 | I2S1_SD  / SPI1_MOSI
 *              | WS       | PA4  | 4   | AF5 | I2S1_WS  / SPI1_NSS
 *              | MCK      | PA3  | 3   | AF5 | I2S1_MCK (master clock)
 * I2S3 (Rx)   | CK       | PC10 | 10  | AF6 | I2S3_SCK / SPI3_SCK
 *              | SD       | PC12 | 12  | AF6 | I2S3_SD  / SPI3_MOSI
 *              | WS       | PC11 | 11  | AF6 | I2S3_WS  / SPI3_NSS
 *              | MCK      | PA9  | 9   | AF6 | I2S3_MCK
 * USART5 (BLE)| TX       | PB6  | 6   | AF7 | USART5_TX
 *              | RX       | PB5  | 5   | AF7 | USART5_RX
 *              | CTS      | PB4  | 4   | AF7 | USART5_CTS (flow ctrl)
 *              | RTS      | PB3  | 3   | AF7 | USART5_RTS
 * TIM3 (Demo) | CH1      | PB4  | 4   | AF2 | Timer capture (shared w/USART5 CTS — use AFR)
 * TIM4 (Mod)  | CH1      | PB6  | 6   | AF2 | Timer PWM (shared w/USART5 TX — use AFR)
 * QSPI        | BK1_IO0  | PB1  | 1   | AF9 | QSPI data line 0
 *              | BK1_IO1  | PB0  | 0   | AF9 | QSPI data line 1
 *              | BK1_IO2  | PC7  | 7   | AF9 | QSPI data line 2 (or use PE15)
 *              | BK1_IO3  | PC6  | 6   | AF9 | QSPI data line 3
 *              | BK1_CLK  | PB2  | 2   | AF9 | QSPI clock
 *              | BK1_NCS  | PC11 | 11  | AF9 | QSPI chip select
 * LEDs        | RGB_R    | PE0  | 0   | OUT | Red channel
 *              | RGB_G    | PE1  | 1   | OUT | Green channel
 *              | RGB_B    | PE2  | 2   | OUT | Blue channel
 *              | LED_TX   | PE3  | 3   | OUT | Yellow Tx indicator
 *              | LED_RX   | PE4  | 4   | OUT | Yellow Rx indicator
 *              | LED_BEACON| PE5 | 5   | OUT | Yellow Beacon indicator
 *              | LED_FAULT| PE6  | 6   | OUT | Red fault indicator
 * Buttons     | BTN_PWR  | PA0  | 0   | IN  | Power button (active low)
 *              | BTN_MODE | PB12 | 12  | IN  | Mode button
 *              | BTN_CAPT | PB13 | 13  | IN  | Capture button
 *              | BTN_RST  | PC0  | 0   | IN  | Reset (to PWR, or toggle)
 * Amplifier   | AMP_SD   | PC1  | 1   | OUT | MAX98357A shutdown (active low)
 *              | AMP_GAIN | PC2  | 2   | OUT | Gain select (0=low, 1=high)
 * Pre-Amp     | PREAMP_SD| PC3  | 3   | OUT | AD8694 shutdown (active low)
 * WM8731      | CODEC_RST| PB14 | 14  | OUT | Reset line (active low)
 *              | CODEC_CS | PB15 | 15  | OUT | I2C address select — unused (hardwired)
 * I2C4 (Codec)| SCL      | PA13 | 13  | AF4 | I2C4_SCL (also SWD_SWO)
 *              | SDA      | PA14 | 14  | AF4 | I2C4_SDA (also SWD_SWCLK)
 * ========================================================================= */

/* =========================================================================
 * I2S1 (Tx) — PA5(SCK), PA7(SD), PA4(WS), PA3(MCK)
 * ========================================================================= */
#define PIN_I2S1_CK             (5u)
#define PORT_I2S1_CK            GPIOA_BASE
#define PIN_I2S1_SD             (7u)
#define PORT_I2S1_SD            GPIOA_BASE
#define PIN_I2S1_WS             (4u)
#define PORT_I2S1_WS            GPIOA_BASE
#define PIN_I2S1_MCK            (3u)
#define PORT_I2S1_MCK           GPIOA_BASE
#define AF_I2S1                 (5u)        /* AF5 = SPI1/I2S1 */

/* =========================================================================
 * I2S3 (Rx) — PC10(CK), PC12(SD), PC11(WS)
 * ========================================================================= */
#define PIN_I2S3_CK             (10u)
#define PORT_I2S3_CK            GPIOC_BASE
#define PIN_I2S3_SD             (12u)
#define PORT_I2S3_SD            GPIOC_BASE
#define PIN_I2S3_WS             (11u)
#define PORT_I2S3_WS            GPIOC_BASE
#define AF_I2S3                 (6u)        /* AF6 = SPI3/I2S3 */

/* =========================================================================
 * USART5 (BLE module) — PB6(TX), PB5(RX), PB4(CTS), PB3(RTS)
 * ========================================================================= */
#define PIN_USART5_TX           (6u)
#define PORT_USART5_TX          GPIOB_BASE
#define PIN_USART5_RX           (5u)
#define PORT_USART5_RX          GPIOB_BASE
#define PIN_USART5_CTS          (4u)
#define PORT_USART5_CTS         GPIOB_BASE
#define PIN_USART5_RTS          (3u)
#define PORT_USART5_RTS         GPIOB_BASE
#define AF_USART5               (7u)        /* AF7 = USART5 */

/* =========================================================================
 * I2C4 (WM8731 codec control) — PA13(SCL), PA14(SDA)
 * ========================================================================= */
#define PIN_I2C4_SCL            (13u)
#define PORT_I2C4_SCL           GPIOA_BASE
#define PIN_I2C4_SDA            (14u)
#define PORT_I2C4_SDA           GPIOA_BASE
#define AF_I2C4                 (4u)        /* AF4 = I2C4 */

/* =========================================================================
 * QSPI (W25Q128 flash) — PB1(IO0), PB0(IO1), PC7(IO2), PC6(IO3), PB2(CLK), PC11(NCS)
 * ========================================================================= */
#define PIN_QSPI_IO0            (1u)
#define PORT_QSPI_IO0           GPIOB_BASE
#define PIN_QSPI_IO1            (0u)
#define PORT_QSPI_IO1           GPIOB_BASE
#define PIN_QSPI_IO2            (7u)
#define PORT_QSPI_IO2           GPIOC_BASE
#define PIN_QSPI_IO3            (6u)
#define PORT_QSPI_IO3           GPIOC_BASE
#define PIN_QSPI_CLK            (2u)
#define PORT_QSPI_CLK           GPIOB_BASE
#define PIN_QSPI_NCS            (11u)
#define PORT_QSPI_NCS           GPIOC_BASE
#define AF_QSPI                 (9u)        /* AF9 = QuadSPI */

/* =========================================================================
 * LED Indicators — PE0-PE6
 * ========================================================================= */
#define PORT_LEDS               GPIOE_BASE
#define PIN_RGB_R               (0u)
#define PIN_RGB_G               (1u)
#define PIN_RGB_B               (2u)
#define PIN_LED_TX              (3u)
#define PIN_LED_RX              (4u)
#define PIN_LED_BEACON          (5u)
#define PIN_LED_FAULT           (6u)

/* =========================================================================
 * Buttons — PA0, PB12, PB13, PC0
 * ========================================================================= */
#define PORT_BTN_PWR            GPIOA_BASE
#define PIN_BTN_PWR             (0u)

#define PORT_BTN_MODE           GPIOB_BASE
#define PIN_BTN_MODE            (12u)

#define PORT_BTN_CAPT           GPIOB_BASE
#define PIN_BTN_CAPT            (13u)

#define PORT_BTN_RST            GPIOC_BASE
#define PIN_BTN_RST             (0u)

/* =========================================================================
 * Amplifier Controls — PC1, PC2
 * ========================================================================= */
#define PORT_AMP                GPIOC_BASE
#define PIN_AMP_SD              (1u)
#define PIN_AMP_GAIN            (2u)

/* =========================================================================
 * Pre-amp Control — PC3
 * ========================================================================= */
#define PORT_PREAMP             GPIOC_BASE
#define PIN_PREAMP_SD           (3u)

/* =========================================================================
 * WM8731 Codec Control — PB14
 * ========================================================================= */
#define PORT_CODEC              GPIOB_BASE
#define PIN_CODEC_RST           (14u)

/* =========================================================================
 * DMA Stream Assignments
 * ========================================================================= */
#define DMA_I2S_TX_STREAM       DMA2_STREAM1    /* DMA2 Stream1 = SPI1_TX (I2S1 Tx) */
#define DMA_I2S_RX_STREAM       DMA2_STREAM0    /* DMA2 Stream0 = SPI3_RX (I2S3 Rx) */
#define DMA_QSPI_STREAM         DMA1_STREAM3    /* DMA1 Stream3 = QUADSPI */

/* =========================================================================
 * I2S DMA channel/request mapping (DMAMUX)
 * ========================================================================= */
#define DMAMUX_I2S1_TX_CH       (9u)            /* SPI1_TX DMAMUX channel */
#define DMAMUX_I2S3_RX_CH       (12u)           /* SPI3_RX DMAMUX channel */
#define DMAMUX_QSPI_CH          (8u)            /* QUADSPI DMAMUX channel */

/* =========================================================================
 * Sample Rate & Audio Constants
 * ========================================================================= */
#define I2S_TX_SAMPLE_RATE      96000U          /* 96 kHz Tx sample rate */
#define I2S_RX_SAMPLE_RATE      192000U         /* 192 kHz Rx sample rate */
#define I2S_TX_BIT_DEPTH        16              /* 16-bit samples */
#define I2S_RX_BIT_DEPTH        16              /* 16-bit samples */
#define I2S_TX_BUFFER_SIZE      512             /* Samples per half-buffer */
#define I2S_RX_BUFFER_SIZE      1024            /* Samples per half-buffer */
#define I2S_TX_DOUBLE_BUF_SIZE  (I2S_TX_BUFFER_SIZE * 2)
#define I2S_RX_DOUBLE_BUF_SIZE  (I2S_RX_BUFFER_SIZE * 2)

/* Goertzel analysis window — must be power of 2 for efficiency */
#define GOERTZEL_N              1024            /* Samples per Goertzel analysis */
#define GOERTZEL_N_BITS         10              /* log2(GOERTZEL_N) */

/* =========================================================================
 * FSK Modulation Parameters
 * ========================================================================= */
#define FSK_FREQ_MARK           21000U          /* '1' = 21.0 kHz */
#define FSK_FREQ_SPACE          19500U          /* '0' = 19.5 kHz */
#define FSK_BAUD_DEFAULT        20              /* 20 bps default */

#define FSK_FREQ_MIN            18000U          /* Min usable freq */
#define FSK_FREQ_MAX            45000U          /* Max usable freq */

/* =========================================================================
 * OOK Modulation Parameters
 * ========================================================================= */
#define OOK_CARRIER_FREQ        20000U          /* 20 kHz carrier */
#define OOK_BIT_DURATION_US     50000           /* 50 ms per bit (20 bps) */

/* =========================================================================
 * Protocol Constants
 * ========================================================================= */
#define MAX_MESSAGE_LEN         64              /* Max payload bytes per frame */
#define SYNC_PREAMBLE_CYCLES    10              /* 10 cycles at 22 kHz preamble */
#define BARKER_CODE_LEN         4               /* 4-bit Barker sync */
#define BARKER_CODE             0xAU            /* Binary 1010 */
#define CRC16_POLY              0x8005          /* CRC-16-IBM polynomial */

/* =========================================================================
 * Capture Buffer Constants
 * ========================================================================= */
#define QSPI_FLASH_SIZE         0x1000000UL     /* 16 MB */
#define PSRAM_SIZE              0x800000UL      /* 8 MB */
#define CAPTURE_SECTOR_SIZE     0x1000          /* 4 KB sectors */
#define CAPTURE_BLOCK_SIZE      0x10000         /* 64 KB blocks */
#define MAX_CAPTURE_SLOTS       64              /* Max stored captures */

/* =========================================================================
 * BLE Protocol Constants
 * ========================================================================= */
#define BLE_BAUD                115200          /* UART baud to nRF52840 */
#define BLE_PACKET_START        0xAA            /* Packet start byte */
#define BLE_MAX_PAYLOAD         128             /* Max BLE payload bytes */
#define BLE_RESP_TIMEOUT_MS     500             /* Response timeout */

/* =========================================================================
 * Power Management Constants
 * ========================================================================= */
#define BATTERY_FULL_MV         4200            /* Full charge voltage */
#define BATTERY_EMPTY_MV        3200            /* Minimum voltage */
#define BATTERY_LOW_MV          3500            /* Low battery warning */
#define POWER_SLEEP_CURRENT_UA  150             /* Sleep mode current (estimated) */
#define POWER_ACTIVE_CURRENT_UA 180000          /* Active Tx current (estimated) */

/* =========================================================================
 * Mode Definitions
 * ========================================================================= */
enum device_mode {
    MODE_IDLE       = 0,
    MODE_TX_FSK     = 1,
    MODE_TX_OOK     = 2,
    MODE_TX_WHISPER = 3,    /* Spread-spectrum whisper */
    MODE_RX_FSK     = 4,
    MODE_RX_OOK     = 5,
    MODE_RX_WHISPER = 6,
    MODE_BEACON     = 7,
    MODE_SCAN       = 8,    /* Acoustic radar/spectral scan */
    MODE_SLEEP      = 9,
    MODE_DFU        = 10    /* Firmware update */
};

/* =========================================================================
 * Power Level Definitions
 * ========================================================================= */
enum tx_power_level {
    TX_POWER_WHISPER = 0,   /* ~1V pp, ~1m range */
    TX_POWER_LOW     = 1,   /* ~2V pp, ~5m range */
    TX_POWER_MED     = 2,   /* ~3.3V pp, ~10m range */
    TX_POWER_HIGH    = 3    /* ~5V pp, ~20m range */
};

/* =========================================================================
 * Pre-amp Gain Definitions
 * ========================================================================= */
enum rx_gain_level {
    RX_GAIN_LOW     = 0,    /* ×1 (0 dB) */
    RX_GAIN_MED     = 1,    /* ×10 (20 dB) */
    RX_GAIN_HIGH    = 2,    /* ×50 (34 dB) */
    RX_GAIN_AUTO    = 3     /* Automatic gain control */
};

/* =========================================================================
 * Error Codes
 * ========================================================================= */
#define ERR_OK                  0
#define ERR_TIMEOUT             (-1)
#define ERR_NACK                (-2)
#define ERR_CRC                 (-3)
#define ERR_OVERRUN             (-4)
#define ERR_NOT_INIT            (-5)
#define ERR_BUSY                (-6)
#define ERR_INVALID_PARAM       (-7)
#define ERR_QSPI                (-8)
#define ERR_BLE                 (-9)
#define ERR_CODEC               (-10)
#define ERR_NO_DATA             (-11)
#define ERR_STORAGE_FULL        (-12)

/* =========================================================================
 * Timing Constants
 * ========================================================================= */
#define SYSTICK_HZ              1000            /* SysTick at 1 kHz */
#define MS_TO_TICKS(ms)         (ms)            /* 1 tick = 1 ms */
#define US_TO_TICKS(us)         ((us) / 1000)

/* =========================================================================
 * System Clock Frequencies
 * ========================================================================= */
#define HSI_VALUE               64000000UL      /* 64 MHz HSI */
#define HSE_VALUE               25000000UL      /* 25 MHz external crystal */
#define CPU_FREQ                480000000UL     /* 480 MHz CPU */
#define AXI_FREQ                240000000UL     /* 240 MHz AXI bus */
#define APB1_FREQ               120000000UL     /* 120 MHz APB1 */
#define APB2_FREQ               120000000UL     /* 120 MHz APB2 */
#define APB3_FREQ               120000000UL     /* 120 MHz APB3 */
#define QSPI_FREQ               100000000UL     /* 100 MHz QSPI clock */

/* I2S Tx clock = APB2 = 120 MHz */
#define I2S_TX_CLOCK            APB2_FREQ

/* I2S Rx clock = APB1H = 120 MHz */
#define I2S_RX_CLOCK            APB1_FREQ

#endif /* BOARD_H */