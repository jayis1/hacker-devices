/*
 * board.h — Prism-Tap Board Configuration
 * MIPI CSI-2 / DSI Camera & Display Interface Tap & Injection Implant
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Pin assignments, clock configuration, and hardware constants for the
 * Prism-Tap board (STM32H730VB + iCE40-UP5K + MIPI bridges + nRF52840).
 */
#ifndef PRISM_TAP_BOARD_H
#define PRISM_TAP_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Firmware version ---- */
#define FW_VERSION_MAJOR  1
#define FW_VERSION_MINOR  0
#define FW_VERSION_PATCH  0
#define FW_VERSION_STRING "1.0.0"

/* ---- Crystal / Clock tree ---- */
#define HSE_VALUE          25000000ULL   /* 25 MHz external crystal   */
#define LSE_VALUE          32768ULL      /* 32.768 kHz LSE for RTC     */
#define SYSCLK_FREQ        550000000ULL  /* 550 MHz max for STM32H730  */
#define AHB_FREQ           275000000ULL  /* HCLK = SYSCLK / 2          */
#define APB1_FREQ          137500000ULL  /* APB1 = AHB / 2             */
#define APB2_FREQ          137500000ULL  /* APB2 = AHB / 2             */

/* ---- GPIO port base addresses (simplified) ---- */
#define GPIOA_BASE   0x58020000U
#define GPIOB_BASE   0x58020400U
#define GPIOC_BASE   0x58020800U
#define GPIOD_BASE   0x58020C00U
#define GPIOE_BASE   0x58021000U
#define GPIOH_BASE   0x58021C00U

/* ---- Pin assignments ---- */
/*
 * STM32H730VB pin map:
 *
 *  PA0  — BOOT0 (pulled low, boot from flash)
 *  PA9  — USART1_TX (debug console, 115200 baud)
 *  PA10 — USART1_RX
 *  PA11 — USB_DM
 *  PA12 — USB_DP
 *  PA13 — SWDIO
 *  PA14 — SWCLK
 *  PB2  — FPGA_CDONE (FPGA config done status input)
 *  PB3  — FPGA_SPI_SCK  (SPI3 SCK to iCE40)
 *  PB4  — FPGA_SPI_MISO  (SPI3 MISO)
 *  PB5  — FPGA_SPI_MOSI  (SPI3 MOSI)
 *  PB6  — FPGA_CRESET_B   (FPGA config reset, active low)
 *  PB7  — FPGA_CSS_B      (FPGA chip select, active low)
 *  PB8  — I2C1_SCL  (MIPI bridge I2C bus)
 *  PB9  — I2C1_SDA
 *  PB10 — UART4_TX (to nRF52840 BLE module)
 *  PB11 — UART4_RX (from nRF52840)
 *  PB12 — SD_DETECT (microSD card detect, active low)
 *  PB13 — SD_SPI_SCK  (SPI2 to SD card)
 *  PB14 — SD_SPI_MISO
 *  PB15 — SD_SPI_MOSI
 *  PC4  — SD_CS (SD card SPI chip select)
 *  PC6  — NOR_SPI_SCK (SPI1 to W25Q256 SPI NOR)
 *  PC7  — NOR_SPI_MISO
 *  PC8  — NOR_SPI_MOSI
 *  PC9  — NOR_CS
 *  PC10 — OLED_I2C_SCL (I2C3)
 *  PC11 — OLED_I2C_SDA
 *  PC13 — LED_STATUS (active low, green)
 *  PC14 — LED_CAPTURE (active low, blue)
 *  PC15 — LED_INJECT  (active low, red)
 *  PH0  — OSC_IN  (25 MHz HSE)
 *  PH1  — OSC_OUT
 *  PH3  — BOOT0 (alt)
 */

/* ---- LED definitions ---- */
#define LED_STATUS_PORT   GPIOC_BASE
#define LED_STATUS_PIN    13
#define LED_CAPTURE_PORT  GPIOC_BASE
#define LED_CAPTURE_PIN   14
#define LED_INJECT_PORT   GPIOC_BASE
#define LED_INJECT_PIN    15

#define LED_ON(port, pin)   /* GPIO BSRR set bit16+pin to reset (active low) */
#define LED_OFF(port, pin)  /* GPIO BSRR set pin to set       */

/* ---- FPGA interface pins ---- */
#define FPGA_SCK_PORT   GPIOB_BASE
#define FPGA_SCK_PIN    3
#define FPGA_MISO_PORT  GPIOB_BASE
#define FPGA_MISO_PIN   4
#define FPGA_MOSI_PORT  GPIOB_BASE
#define FPGA_MOSI_PIN   5
#define FPGA_CRESET_PORT GPIOB_BASE
#define FPGA_CRESET_PIN  6
#define FPGA_CSS_PORT   GPIOB_BASE
#define FPGA_CSS_PIN    7
#define FPGA_CDONE_PORT GPIOB_BASE
#define FPGA_CDONE_PIN  2

/* ---- I2C bus for MIPI bridges ---- */
#define BRIDGE_I2C_PORT_SCL  GPIOB_BASE
#define BRIDGE_I2C_PIN_SCL   8
#define BRIDGE_I2C_PORT_SDA  GPIOB_BASE
#define BRIDGE_I2C_PIN_SDA   9
#define BRIDGE_I2C_ADDR_TC358746  0x0E   /* 7-bit, shifted << 1 by driver */
#define BRIDGE_I2C_ADDR_TC358748  0x0E   /* configurable; both on same bus w/ ADDR pin */
#define BRIDGE_I2C_ADDR_ADV7480   0x70
#define BRIDGE_I2C_ADDR_TC358762  0x0F

/* ---- BLE UART (to nRF52840) ---- */
#define BLE_UART_TX_PORT  GPIOB_BASE
#define BLE_UART_TX_PIN   10
#define BLE_UART_RX_PORT  GPIOB_BASE
#define BLE_UART_RX_PIN   11
#define BLE_UART_BAUD      921600

/* ---- SD card SPI ---- */
#define SD_SPI_PORT_SCK   GPIOB_BASE
#define SD_SPI_PIN_SCK    13
#define SD_SPI_PORT_CS    GPIOC_BASE
#define SD_SPI_PIN_CS     4
#define SD_DETECT_PORT    GPIOB_BASE
#define SD_DETECT_PIN     12

/* ---- SPI NOR flash ---- */
#define NOR_SPI_PORT_SCK  GPIOC_BASE
#define NOR_SPI_PIN_SCK   6
#define NOR_SPI_PORT_CS   GPIOC_BASE
#define NOR_SPI_PIN_CS    9

/* ---- OLED ---- */
#define OLED_I2C_PORT_SCL GPIOC_BASE
#define OLED_I2C_PIN_SCL  10
#define OLED_I2C_PORT_SDA GPIOC_BASE
#define OLED_I2C_PIN_SDA  11
#define OLED_I2C_ADDR     0x3C   /* 7-bit SSD1306 address */
#define OLED_WIDTH        128
#define OLED_HEIGHT       64

/* ---- USB-C ---- */
#define USB_DM_PORT  GPIOA_BASE
#define USB_DM_PIN   11
#define USB_DP_PORT  GPIOA_BASE
#define USB_DP_PIN   12

/* ---- Operating modes ---- */
typedef enum {
    MODE_IDLE = 0,         /* bridges powered down, MCU on standby       */
    MODE_PASSTHROUGH,      /* tap active, no capture, no injection         */
    MODE_CAPTURE_ONLY,     /* tap + capture to SD, no injection           */
    MODE_INJECT_ONLY,      /* tap + injection, no capture                 */
    MODE_FULL_MITM,        /* tap + capture + injection simultaneously    */
    MODE_CAPTURE_LOW_POWER,/* one-directional tap, reduced clock, no Tx  */
    MODE_DIAGNOSTIC,       /* link test / loopback / self-test             */
} prism_mode_t;

/* ---- Frame format enumeration ---- */
typedef enum {
    FMT_RAW8  = 0,
    FMT_RAW10 = 1,
    FMT_RAW12 = 2,
    FMT_YUV422 = 3,
    FMT_RGB888 = 4,
    FMT_JPEG   = 5,
} frame_format_t;

/* ---- Capture configuration ---- */
typedef struct {
    uint8_t          active;       /* 1 = currently capturing                 */
    frame_format_t   format;       /* pixel format of captured frames         */
    uint16_t         width;        /* frame width  in pixels                  */
    uint16_t         height;       /* frame height in pixels                  */
    uint32_t         interval_ms;  /* 0 = every frame; N = every N ms        */
    uint32_t         max_frames;   /* 0 = unlimited; N = stop after N frames  */
    uint32_t         frames_captured;
    uint32_t         bytes_written;
    uint32_t         last_frame_ts;
    uint8_t          jpeg_compress; /* 1 = hardware JPEG encode before write  */
} capture_config_t;

/* ---- Injection configuration ---- */
typedef enum {
    INJECT_FULL_REPLACE = 0,  /* Replace entire frame with injected content */
    INJECT_OVERLAY,           /* Overlay injected content on original frame  */
    INJECT_SELECTIVE,         /* Replace only when trigger condition met     */
} inject_mode_t;

typedef struct {
    uint8_t          active;       /* 1 = currently injecting                */
    inject_mode_t    mode;         /* injection strategy                     */
    uint16_t         overlay_x;    /* overlay position X (for OVERLAY mode)  */
    uint16_t         overlay_y;    /* overlay position Y                     */
    uint16_t         overlay_w;    /* overlay width                           */
    uint16_t         overlay_h;    /* overlay height                         */
    uint32_t         frame_count;  /* frames injected so far                 */
    uint32_t         trigger_interval; /* inject every N frames (SELECTIVE)  */
} inject_config_t;

/* ---- Timing manipulation ---- */
typedef struct {
    uint8_t          enabled;
    int32_t          delay_ns;     /* add delay in ns to each frame          */
    uint8_t          jitter_pct;   /* 0-100: random jitter percentage        */
    uint8_t          drop_pattern; /* bitmask: 1=drop, 0=pass, cycles          */
    uint32_t         frames_dropped;
} timing_config_t;

/* ---- Device status ---- */
typedef struct {
    uint8_t          mode;         /* current prism_mode_t                   */
    uint8_t          battery_pct;  /* 0-100                                  */
    uint16_t         battery_mv;   /* battery voltage in mV                  */
    uint8_t          sd_present;   /* 1 = SD card inserted                   */
    uint8_t          ble_connected;/* 1 = BLE C2 link up                     */
    uint8_t          usb_connected;/* 1 = USB-C connected                    */
    uint8_t          fpga_ready;   /* 1 = FPGA configured and running       */
    uint8_t          csi_link_up;  /* 1 = CSI-2 input link established      */
    uint8_t          dsi_link_up;  /* 1 = DSI input link established        */
    uint32_t         uptime_ms;    /* milliseconds since boot               */
    capture_config_t capture;
    inject_config_t  inject;
    timing_config_t  timing;
} device_status_t;

/* ---- Global status (defined in main.c) ---- */
extern device_status_t g_status;

/* ---- Utility macros ---- */
#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))
#define ARRAY_SIZE(a)    (sizeof(a) / sizeof((a)[0]))
#define UNUSED(x)        ((void)(x))

/* ---- Boot stage tracking ---- */
typedef enum {
    BOOT_STAGE_INIT = 0,
    BOOT_STAGE_CLOCKS,
    BOOT_STAGE_GPIO,
    BOOT_STAGE_SPI_NOR,
    BOOT_STAGE_FPGA_LOAD,
    BOOT_STAGE_BRIDGES,
    BOOT_STAGE_SD,
    BOOT_STAGE_BLE,
    BOOT_STAGE_USB,
    BOOT_STAGE_OLED,
    BOOT_STAGE_DONE,
} boot_stage_t;

extern boot_stage_t g_boot_stage;

#endif /* PRISM_TAP_BOARD_H */

/* ---- End of board.h ----
 * Author: jayis1
 */