/*
 * board.h — Forge-Probe Hardware Definition Layer
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Pin mappings, clock tree, voltage rail configuration, and FPGA bridge
 * definitions for the Forge-Probe device.
 *
 * MCU: STM32H743IIK6 (BGA176, dual Cortex-M7/M4)
 * FPGA: Lattice iCE40UP5K-SG48
 * Debug IF: JTAG 20-pin + SWD 10-pin + cJTAG 2-wire
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* ─── Clock Tree ──────────────────────────────────────────────────────────── */

#define HSE_FREQ_HZ             25000000UL   /* 25 MHz external crystal */
#define HSI_FREQ_HZ             64000000UL   /* Internal 64 MHz RC */
#define PLL1_M                  5            /* 25 / 5 = 5 MHz VCO input */
#define PLL1_N                  192          /* 5 * 192 = 960 MHz VCO */
#define PLL1_P                  2            /* 960 / 2 = 480 MHz (M7 core) */
#define PLL1_Q                  4            /* 960 / 4 = 240 MHz (AXI/AHB) */
#define PLL1_R                  8            /* 960 / 8 = 120 MHz (APB) */
#define CORE_CLOCK_HZ           480000000UL
#define AXI_CLOCK_HZ            240000000UL
#define APB1_CLOCK_HZ           120000000UL
#define APB2_CLOCK_HZ           120000000UL

/* ─── Voltage Rails (via board-level regulators) ──────────────────────────── */

typedef enum {
    VIO_1V8 = 0,    /* 1.8 V — modern MCUs, FPGAs, low-voltage targets */
    VIO_2V5 = 1,    /* 2.5 V — DDR, older FPGAs */
    VIO_3V3 = 2,    /* 3.3 V — standard logic, most targets */
    VIO_5V0 = 3     /* 5.0 V — legacy, industrial, 5V-logic targets */
} vio_rail_t;

/* Target-supplied VREF input (if target powers the rail) */
#define VREF_ADC_CH             ADC_CHANNEL_5
#define VREF_GPIO_PORT          GPIOA
#define VREF_GPIO_PIN           5

/* Internal voltage rail enables (active high) */
#define EN_1V8_PORT             GPIOI
#define EN_1V8_PIN              0
#define EN_2V5_PORT             GPIOI
#define EN_2V5_PIN              1
#define EN_3V3_PORT             GPIOI
#define EN_3V3_PIN              2
#define EN_5V0_PORT             GPIOI
#define EN_5V0_PIN              3

/* ─── JTAG Interface (Standard 20-pin ARM header) ────────────────────────── */

#define JTAG_TCK_PORT           GPIOB
#define JTAG_TCK_PIN            0
#define JTAG_TMS_PORT           GPIOB
#define JTAG_TMS_PIN            1
#define JTAG_TDI_PORT           GPIOB
#define JTAG_TDI_PIN            2
#define JTAG_TDO_PORT           GPIOB
#define JTAG_TDO_PIN            3
#define JTAG_nTRST_PORT         GPIOB
#define JTAG_nTRST_PIN          4
#define JTAG_nSRST_PORT         GPIOB
#define JTAG_nSRST_PIN          5
#define JTAG_RTCK_PORT          GPIOB
#define JTAG_RTCK_PIN           6

/* Alternative SWD mux on same header pins */
#define SWD_SWCLK_PORT          JTAG_TCK_PORT
#define SWD_SWCLK_PIN           JTAG_TCK_PIN
#define SWD_SWDIO_PORT          JTAG_TMS_PORT
#define SWD_SWDIO_PIN           JTAG_TMS_PIN
#define SWD_nRESET_PORT         JTAG_nSRST_PORT
#define SWD_nRESET_PIN          JTAG_nSRST_PIN

/* 2-wire cJTAG uses SWCLK + SWDIO bidirectional */
#define CJTAG_TCKC_PORT         SWD_SWCLK_PORT
#define CJTAG_TCKC_PIN          SWD_SWCLK_PIN
#define CJTAG_TMSC_PORT         SWD_SWDIO_PORT
#define CJTAG_TMSC_PIN          SWD_SWDIO_PIN

/* ─── High-Speed FPGA Bridge (SPI-based, 60 MHz) ──────────────────────────── */

#define FPGA_CS_PORT            GPIOE
#define FPGA_CS_PIN             0
#define FPGA_SCK_PORT           GPIOE
#define FPGA_SCK_PIN            1
#define FPGA_MOSI_PORT          GPIOE
#define FPGA_MOSI_PIN           2
#define FPGA_MISO_PORT          GPIOE
#define FPGA_MISO_PIN           3
#define FPGA_IRQ_PORT           GPIOE
#define FPGA_IRQ_PIN            4
#define FPGA_CRESET_PORT        GPIOE
#define FPGA_CRESET_PIN         5
#define FPGA_CDONE_PORT         GPIOE
#define FPGA_CDONE_PIN          6
#define FPGA_SSN_PORT           GPIOE
#define FPGA_SSN_PIN            7

/* SPI instance for FPGA bridge */
#define FPGA_SPI_INST           SPI5
#define FPGA_SPI_CLOCK_HZ       60000000UL
#define FPGA_SPI_MODE           SPI_MODE_0

/* ─── Multi-Protocol IO Pins (16 channels, universal) ────────────────────── */

#define MPIO_COUNT              16

typedef struct {
    uint8_t         channel;        /* 0–15 */
    GPIO_TypeDef   *gpio_port;
    uint16_t        gpio_pin;
    uint8_t         af_number;      /* Alternate function for UART/SPI/I2C */
    bool            is_5v_tolerant;
} mpio_pin_t;

extern const mpio_pin_t mpio_pins[MPIO_COUNT];

/* UART muxed across MPIO 0–1 */
#define MPIO_UART_TX            0
#define MPIO_UART_RX            1
#define MPIO_UART_INST          USART1
#define MPIO_UART_BAUD          115200UL

/* SPI muxed across MPIO 2–5 */
#define MPIO_SPI_SCK            2
#define MPIO_SPI_MOSI           3
#define MPIO_SPI_MISO           4
#define MPIO_SPI_CS             5
#define MPIO_SPI_INST           SPI2

/* I2C muxed across MPIO 6–7 */
#define MPIO_I2C_SCL            6
#define MPIO_I2C_SDA            7
#define MPIO_I2C_INST           I2C1

/* 1-Wire on MPIO 8 */
#define MPIO_ONEWIRE            8

/* GPIO-bank modes per channel */
#define MPIO_MODE_GPIO_IN       0
#define MPIO_MODE_GPIO_OUT      1
#define MPIO_MODE_UART          2
#define MPIO_MODE_SPI           3
#define MPIO_MODE_I2C           4
#define MPIO_MODE_ONEWIRE       5
#define MPIO_MODE_PWM_IN        6
#define MPIO_MODE_PWM_OUT       7

/* ─── USB Interface ────────────────────────────────────────────────────────── */

#define USB_HS_PORT             GPIOA
#define USB_HS_DP_PIN           11
#define USB_HS_DM_PIN           12
#define USB_HS_ID_PIN           10
#define USB_HS_VBUS_PIN         9
#define USB_HS_ULPI_CK          GPIOA
#define USB_HS_ULPI_CK_PIN      5
#define USB_HS_INST             USB_OTG_HS
#define USB_HS_EP_COUNT         6
#define USB_HS_FIFO_SIZE        4096UL

/* ─── MicroSD Interface (SDMMC, 4-bit) ────────────────────────────────────── */

#define SDMMC_INST              SDMMC1
#define SDMMC_CK_PORT           GPIOC
#define SDMMC_CK_PIN            12
#define SDMMC_CMD_PORT          GPIOD
#define SDMMC_CMD_PIN           2
#define SDMMC_D0_PORT           GPIOC
#define SDMMC_D0_PIN            8
#define SDMMC_D1_PORT           GPIOC
#define SDMMC_D1_PIN            9
#define SDMMC_D2_PORT           GPIOC
#define SDMMC_D2_PIN            10
#define SDMMC_D3_PORT           GPIOC
#define SDMMC_D3_PIN            11

/* ─── Status & User Interface ──────────────────────────────────────────────── */

#define STATUS_LED_R_PORT       GPIOD
#define STATUS_LED_R_PIN        13
#define STATUS_LED_G_PORT       GPIOD
#define STATUS_LED_G_PIN        14
#define STATUS_LED_B_PORT       GPIOD
#define STATUS_LED_B_PIN        15

#define USER_BUTTON_PORT        GPIOI
#define USER_BUTTON_PIN         8
#define BOOT_BUTTON_PORT        GPIOC
#define BOOT_BUTTON_PIN         13

/* ─── Target Power Monitoring ──────────────────────────────────────────────── */

#define TARGET_VIN_SENSE_PORT   GPIOA
#define TARGET_VIN_SENSE_PIN    4
#define TARGET_CURRENT_SENSE    ADC_CHANNEL_4
#define TARGET_OVERCURRENT_PORT GPIOI
#define TARGET_OVERCURRENT_PIN  4

/* ─── Fault Injection (Glitch) Outputs ─────────────────────────────────────── */

#define GLITCH_TRIGGER_PORT     GPIOI
#define GLITCH_TRIGGER_PIN      5
#define GLITCH_MONITOR_PORT     GPIOI
#define GLITCH_MONITOR_PIN      6

/* ─── Debug UART (console) ─────────────────────────────────────────────────── */

#define CONSOLE_UART_INST       USART3
#define CONSOLE_UART_TX_PORT    GPIOD
#define CONSOLE_UART_TX_PIN     5
#define CONSOLE_UART_RX_PORT    GPIOD
#define CONSOLE_UART_RX_PIN     6
#define CONSOLE_UART_BAUD       115200UL

/* ─── Timing Constants ──────────────────────────────────────────────────────── */

#define JTAG_DEFAULT_CLOCK_HZ   4000000UL     /* 4 MHz TCK (conservative) */
#define JTAG_MAX_CLOCK_HZ       120000000UL   /* 120 MHz with FPGA accel */
#define SWD_DEFAULT_CLOCK_HZ    4000000UL
#define SWD_MAX_CLOCK_HZ        60000000UL
#define CJTAG_DEFAULT_CLOCK_HZ  2000000UL

#define TAP_RESET_CYCLES        5             /* 5 TCK cycles with TMS=1 */
#define IDCODE_POLL_DELAY_MS    10

/* ─── Buffer Sizes ──────────────────────────────────────────────────────────── */

#define JTAG_DR_BUFFER_SIZE     (4 * 1024)    /* 4 KB data register buffer */
#define SWD_BUFFER_SIZE         (2 * 1024)    /* 2 KB SWD transaction buffer */
#define BOUNDARY_SCAN_SIZE      (32 * 1024)   /* 32 KB BS chain capture */
#define LOG_BUFFER_LINES        256
#define SD_CAPTURE_CHUNK        65536UL       /* 64 KB SD write granularity */

/* ─── Boot Behaviour ────────────────────────────────────────────────────────── */

#define WDT_TIMEOUT_MS          30000UL       /* 30 second watchdog */
#define AUTO_DISCOVER_RETRIES   3
#define VOLTAGE_STABILIZE_MS    50

#endif /* BOARD_H */