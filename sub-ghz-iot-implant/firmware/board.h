/*
 * board.h — Substation Sub-GHz IoT Gateway Implant
 * Hardware definitions for CC1352P-based board
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Memory Map
 * ============================================ */
#define FLASH_BASE          0x00000000  /* 1 MB internal flash */
#define FLASH_SIZE          0x00100000
#define SRAM_BASE           0x20000000  /* 128 KB internal SRAM */
#define SRAM_SIZE           0x00020000
#define SDRAM_BASE          0x60000000  /* 64 MB external SDRAM */
#define SDRAM_SIZE          0x04000000
#define SPI_FLASH_BASE      0x70000000  /* 16 MB external SPI flash (memory-mapped) */
#define SPI_FLASH_SIZE      0x01000000
#define PERIPH_BASE         0x40000000  /* Peripheral registers */

/* ============================================
 * Clock Configuration
 * ============================================ */
#define SYSTEM_CLOCK_HZ     48000000    /* 48 MHz main system clock */
#define XOSC_HF_FREQ_HZ     24000000   /* 24 MHz high-frequency crystal */
#define XOSC_LF_FREQ_HZ     32768       /* 32.768 kHz low-frequency crystal */
#define SSI0_CLK_HZ          24000000   /* 24 MHz SDRAM SPI clock */
#define SSI1_CLK_HZ          48000000   /* 48 MHz Flash SPI clock */
#define I2C_CLK_HZ            1000000    /* 1 MHz I2C fast mode+ */
#define UART0_BAUD            115200     /* Debug console baud rate */
#define UART1_BAUD            115200     /* Expansion UART baud rate */

/* ============================================
 * GPIO Pin Definitions
 * ============================================ */

/* SDRAM Interface (SSI0) */
#define PIN_SDRAM_DQ0        0
#define PIN_SDRAM_DQ1        1
#define PIN_SDRAM_DQ2        2
#define PIN_SDRAM_DQ3        3
#define PIN_SDRAM_DQ4        4
#define PIN_SDRAM_DQ5        5
#define PIN_SDRAM_DQ6        6
#define PIN_SDRAM_DQ7        7
#define PIN_SDRAM_CLK        21
#define PIN_SDRAM_CS         22
#define PIN_SDRAM_DQS        23
#define PIN_SDRAM_DQ8        30
#define PIN_SDRAM_DQ9        31
#define PIN_SDRAM_DQ10       32
#define PIN_SDRAM_DQ11       33
#define PIN_SDRAM_DQ12       34
#define PIN_SDRAM_DQ13       35
#define PIN_SDRAM_DQ14       36
#define PIN_SDRAM_DQ15       37

/* SPI Flash (SSI1) */
#define PIN_FLASH_CLK        8
#define PIN_FLASH_MOSI       9
#define PIN_FLASH_MISO       10
#define PIN_FLASH_CS         11

/* MicroSD (SSI3) */
#define PIN_SD_CS            12
#define PIN_SD_CLK           13
#define PIN_SD_MOSI          14
#define PIN_SD_MISO          15
#define PIN_SD_CD            28

/* I2C (ATECC608A) */
#define PIN_I2C_SDA          19
#define PIN_I2C_SCL          20

/* UART0 (Debug) */
#define PIN_UART0_TX         24
#define PIN_UART0_RX          25

/* UART1 (Expansion) */
#define PIN_UART1_TX         26
#define PIN_UART1_RX         27

/* USB */
#define PIN_USB_DP           28
#define PIN_USB_DM           29

/* Status LED (WS2812B) */
#define PIN_WS2812B          16

/* Buttons */
#define PIN_BTN_MODE         17
#define PIN_BTN_ACTION       18

/* Debug (SWD) */
#define PIN_SWD_CLK          45
#define PIN_SWD_IO           46
#define PIN_SWD_TDI          47
#define PIN_SWD_TDO          48
#define PIN_RESET             -1  /* Dedicated RESET pin */

/* ============================================
 * Peripheral Base Addresses
 * ============================================ */
#define SSI0_BASE            0x40008000  /* SDRAM SPI */
#define SSI1_BASE            0x4000A000  /* Flash SPI */
#define SSI2_BASE            0x4000C000  /* I2C alternate */
#define SSI3_BASE            0x4000E000  /* SD Card SPI */
#define UART0_BASE           0x40023000
#define UART1_BASE           0x40024000
#define GPIO_BASE            0x40022000
#define I2C0_BASE            0x40002000
#define USB_BASE             0x40010000
#define RFC_DBELL_BASE       0x40037000
#define RFC_PWR_BASE         0x40037100
#define UDMA0_BASE           0x4000F000
#define TIMER0_BASE          0x40000000
#define TIMER1_BASE          0x40000400
#define WDT_BASE             0x40008000
#define RTC_BASE             0x40091000
#define FLASH_CTRL_BASE      0x400D0000
#define VIMS_BASE            0x40034000

/* ============================================
 * SDRAM Configuration
 * ============================================ */
#define SDRAM_RING_SIZE      (4 * 1024 * 1024)  /* 4 MB for packet ring buffer */
#define SDRAM_PCAP_START     0x60000000           /* Start of pcap storage area */
#define SDRAM_PCAP_SIZE      (60 * 1024 * 1024)   /* 60 MB pcap storage */

/* ============================================
 * SPI Flash Partitions
 * ============================================ */
#define FLASH_FW_OFFSET      0x00000000   /* 960 KB firmware */
#define FLASH_CFG_OFFSET     0x000F0000   /* 64 KB config area */
#define FLASH_PCAP_OFFSET    0x000F8000   /* 32 KB pcap index */

/* ============================================
 * ATECC608A Configuration
 * ============================================ */
#define ATECC_I2C_ADDR       0x60         /* 7-bit I2C address */
#define ATECC_WAKEUP_FREQ    100000       /* 100 kHz for wakeup */
#define ATECC_OP_FREQ        1000000      /* 1 MHz for normal ops */

/* ============================================
 * Radio Configuration Defaults
 * ============================================ */
#define RADIO_DEFAULT_FREQ   868000000    /* 868 MHz (EU Sub-GHz) */
#define RADIO_DEFAULT_POWER   14          /* +14 dBm default TX power */
#define RADIO_MAX_POWER       20          /* +20 dBm maximum TX power */
#define RADIO_CHANNEL_BW     200000       /* 200 kHz default channel BW */

/* ============================================
 * BLE Configuration
 * ============================================ */
#define BLE_DEVICE_NAME      "SUBSTATION"
#define BLE_ADV_INTERVAL_MS  100          /* 100 ms advertising interval */
#define BLE_CONN_INTERVAL_MS 30           /* 30 ms connection interval */
#define BLE_MTU_SIZE         244          /* BLE 5.0 DLE MTU */

/* ============================================
 * USB Configuration
 * ============================================ */
#define USB_VID              0x1209      /* pid.codes */
#define USB_PID              0x5UB5      /* SUBSTN */
#define USB_MANUFACTURER     "Hacker Devices"
#define USB_PRODUCT          "Substation"
#define USB_CDC_BAUD          115200

/* ============================================
 * Button Debounce
 * ============================================ */
#define BTN_DEBOUNCE_MS      50          /* 50 ms debounce time */

/* ============================================
 * LED Patterns (WS2812B)
 * ============================================ */
#define LED_COLOR_IDLE       0x000020    /* Dim blue */
#define LED_COLOR_SNIFFING   0x002000    /* Dim green */
#define LED_COLOR_TX         0x200000    /* Red */
#define LED_COLOR_ERROR      0x404000    /* Yellow */
#define LED_COLOR_BLE_CONN   0x002040    /* Cyan */
#define LED_COLOR_USB_CONN   0x400040    /* Purple */

/* ============================================
 * Battery
 * ============================================ */
#define BAT_MV_FULL          4200        /* 4.2V fully charged */
#define BAT_MV_EMPTY         3000        /* 3.0V cutoff */
#define BAT_CAPACITY_MAH    1000        /* 1000 mAh */

/* ============================================
 * Function Prototypes
 * ============================================ */
void board_init_clocks(void);
void board_init_gpio(void);
void board_init_peripherals(void);
void board_init_sdramp(void);
void board_led_set(uint32_t color);
void board_led_off(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */