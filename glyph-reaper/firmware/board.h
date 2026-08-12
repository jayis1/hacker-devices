/**
 * @file board.h
 * @brief GLYPH-REAPER board pin definitions and hardware configuration
 *
 * Complete pin mapping for nRF52840-QIAA on GLYPH-REAPER Rev A flexible PCB.
 * All pins defined with their primary function and alternate functions.
 *
 * Board: GLYPH-REAPER Rev A
 * MCU:   nRF52840-QIAA-R7 (Cortex-M4F @ 64 MHz, 1MB Flash, 256KB RAM)
 * FPGA:  iCE40UP5K-SG48 (5.3K LUTs, reconfigurable display capture)
 * DSP:   ESP32-S3-MINI-1 (dual-core 240MHz, OCR + compression)
 * Date:  2026-08-12
 * Author: jayis1
 *
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * CLOCK CONFIGURATION
 *===========================================================================*/

#define BOARD_HFCLK_FREQ_HZ         64000000UL   /* 64 MHz (32 MHz crystal × PLL) */
#define BOARD_LFCLK_FREQ_HZ         32768UL      /* 32.768 kHz crystal */
#define BOARD_TIMESTAMP_FREQ_HZ     1000000UL    /* 1 MHz for µs-precision frame timestamps */

/*===========================================================================
 * PIN DEFINITIONS — PORT 0 (nRF52840)
 *===========================================================================*/

/* HF Crystal (32 MHz) */
#define BOARD_PIN_XL1               0    /* P0.00 — 32 MHz crystal input */
#define BOARD_PIN_XL2               1    /* P0.01 — 32 MHz crystal output */

/* Analog inputs */
#define BOARD_PIN_AIN0              2    /* P0.02 — ADC input 0 (spare) */
#define BOARD_PIN_AIN1              3    /* P0.03 — ADC input 1 (spare) */
#define BOARD_PIN_VBAT_SENSE        4    /* P0.04 — Battery voltage sense (VBAT/2 divider) */
#define BOARD_PIN_AIN3              5    /* P0.05 — ADC input 3 (FPGA temp sense) */

/* SPI0 — FPGA interface (configuration + status) */
#define BOARD_PIN_FPGA_SPI_MOSI     6    /* P0.06 — SPIM0 MOSI → iCE40 SDI */
#define BOARD_PIN_FPGA_SPI_MISO     7    /* P0.07 — SPIM0 MISO ← iCE40 SDO */
#define BOARD_PIN_FPGA_SPI_SCK      8    /* P0.08 — SPIM0 SCK → iCE40 SCK */
#define BOARD_PIN_FPGA_CSN          9    /* P0.09 — SPIM0 CSN → iCE40 CSB */
#define BOARD_PIN_FPGA_CDONE        10   /* P0.10 — iCE40 CDONE (config done) */
#define BOARD_PIN_FPGA_CRSTB        11   /* P0.11 — iCE40 CRSTB (config reset) */

/* SPI1 — ESP32-S3 DSP interface (frame data + commands) */
#define BOARD_PIN_DSP_SPI_MOSI      12   /* P0.12 — SPIM1 MOSI → ESP32-S3 MOSI */
#define BOARD_PIN_DSP_SPI_MISO      13   /* P0.13 — SPIM1 MISO ← ESP32-S3 MISO */
#define BOARD_PIN_DSP_SPI_SCK       14   /* P0.14 — SPIM1 SCK → ESP32-S3 SCK */
#define BOARD_PIN_DSP_SPI_CSN       15   /* P0.15 — SPIM1 CSN → ESP32-S3 CS */
#define BOARD_PIN_DSP_HANDSHAKE     16   /* P0.16 — DSP data-ready handshake */
#define BOARD_PIN_DSP_RESET         17   /* P0.17 — ESP32-S3 EN (reset) */

/* QSPI — External PSRAM + Flash */
#define BOARD_PIN_QSPI_CSN_PSRAM    18   /* P0.18 — QSPI CS# for APS6404L PSRAM */
#define BOARD_PIN_QSPI_CSN_FLASH    19   /* P0.19 — QSPI CS# for W25Q128JV Flash */
#define BOARD_PIN_QSPI_IO0          20   /* P0.20 — QSPI IO0 / D0 */
#define BOARD_PIN_QSPI_IO1          21   /* P0.21 — QSPI IO1 / D1 */
#define BOARD_PIN_QSPI_IO2          22   /* P0.22 — QSPI IO2 / D2 */
#define BOARD_PIN_QSPI_IO3          23   /* P0.23 — QSPI IO3 / D3 */
#define BOARD_PIN_QSPI_CLK          24   /* P0.24 — QSPI SCK */

/* I²C — Accelerometer + spare */
#define BOARD_PIN_I2C_SDA           26   /* P0.26 — TWIM0 SDA → LIS2DH12 */
#define BOARD_PIN_I2C_SCL           27   /* P0.27 — TWIM0 SCL → LIS2DH12 */

/* FPGA frame data interface (parallel capture from FPGA) */
#define BOARD_PIN_FRAME_DATA_0      28   /* P0.28 — Frame data bus bit 0 */
#define BOARD_PIN_FRAME_DATA_1      29   /* P0.29 — Frame data bus bit 1 */
#define BOARD_PIN_FRAME_DATA_2      30   /* P0.30 — Frame data bus bit 2 */
#define BOARD_PIN_FRAME_DATA_3      31   /* P0.31 — Frame data bus bit 3 */

/*===========================================================================
 * PIN DEFINITIONS — PORT 1 (nRF52840)
 *===========================================================================*/

#define BOARD_PIN_FRAME_DATA_4      0    /* P1.00 — Frame data bus bit 4 */
#define BOARD_PIN_FRAME_DATA_5      1    /* P1.01 — Frame data bus bit 5 */
#define BOARD_PIN_FRAME_DATA_6      2    /* P1.02 — Frame data bus bit 6 */
#define BOARD_PIN_FRAME_DATA_7      3    /* P1.03 — Frame data bus bit 7 */
#define BOARD_PIN_FRAME_HSYNC       4    /* P1.04 — HSYNC from FPGA */
#define BOARD_PIN_FRAME_VSYNC       5    /* P1.05 — VSYNC from FPGA */
#define BOARD_PIN_FRAME_PCLK        6    /* P1.06 — Pixel clock from FPGA */
#define BOARD_PIN_FRAME_DE          7    /* P1.07 — Data Enable from FPGA */

/* Power management */
#define BOARD_PIN_PWR_HARVEST_EN    8    /* P1.08 — Load switch: harvested power enable */
#define BOARD_PIN_PWR_COINCELL_EN   9    /* P1.09 — Load switch: coin cell enable */
#define BOARD_PIN_DCDC_EN           10   /* P1.10 — TPS62740 DC/DC enable */

/* User interface */
#define BOARD_PIN_LED_RED           11   /* P1.11 — Red status LED (active high) */
#define BOARD_PIN_LED_GRN           12   /* P1.12 — Green activity LED */
#define BOARD_PIN_LED_BLU           13   /* P1.13 — Blue BLE connected LED */
#define BOARD_PIN_BTN_USER          14   /* P1.14 — User button (active low, pull-up) */

/* SD card (optional) */
#define BOARD_PIN_SD_CLK            15   /* P1.15 — SD card SPI clock */
#define BOARD_PIN_SD_MOSI           16   /* P1.16 — SD card SPI MOSI */
#define BOARD_PIN_SD_MISO           17   /* P1.17 — SD card SPI MISO */
#define BOARD_PIN_SD_CS             18   /* P1.18 — SD card SPI CS */
#define BOARD_PIN_SD_DETECT         19   /* P1.19 — SD card detect (active low) */

/*===========================================================================
 * FIXED-FUNCTION PINS (nRF52840)
 *===========================================================================*/

#define BOARD_PIN_SWDIO             65   /* SWD data I/O */
#define BOARD_PIN_SWDCLK            66   /* SWD clock input */
#define BOARD_PIN_USB_DPLUS         59   /* USB D+ */
#define BOARD_PIN_USB_DMINUS        60   /* USB D- */
#define BOARD_PIN_USB_VBUS          58   /* USB VBUS detect */
#define BOARD_PIN_XC1               63   /* 32.768 kHz crystal input */
#define BOARD_PIN_XC2               64   /* 32.768 kHz crystal output */

/*===========================================================================
 * FPGA INTERFACE CONFIGURATION
 *===========================================================================*/

#define FPGA_SPI_INSTANCE           0       /* SPIM0 */
#define FPGA_SPI_FREQ_HZ            16000000UL /* 16 MHz for config + status */
#define FPGA_SPI_MODE               0       /* CPOL=0, CPHA=0 */
#define FPGA_SPI_CSN_PIN            BOARD_PIN_FPGA_CSN
#define FPGA_SPI_SCK_PIN            BOARD_PIN_FPGA_SPI_SCK
#define FPGA_SPI_MOSI_PIN           BOARD_PIN_FPGA_SPI_MOSI
#define FPGA_SPI_MISO_PIN           BOARD_PIN_FPGA_SPI_MISO
#define FPGA_CDONE_PIN              BOARD_PIN_FPGA_CDONE
#define FPGA_CRSTB_PIN              BOARD_PIN_FPGA_CRSTB

/* FPGA bitstream storage offsets in NOR Flash */
#define FPGA_BITSTREAM_MIPI_DSI     0x00000000UL   /* MIPI DSI decoder */
#define FPGA_BITSTREAM_RGB_PARALLEL 0x00020000UL   /* RGB parallel decoder */
#define FPGA_BITSTREAM_SPI_LCD      0x00040000UL   /* SPI LCD decoder */
#define FPGA_BITSTREAM_LVDS         0x00060000UL   /* LVDS decoder */
#define FPGA_BITSTREAM_SIZE         0x00020000UL   /* 128 KB per bitstream */

/*===========================================================================
 * DSP INTERFACE CONFIGURATION
 *===========================================================================*/

#define DSP_SPI_INSTANCE            1       /* SPIM1 */
#define DSP_SPI_FREQ_HZ             32000000UL /* 32 MHz for frame data transfer */
#define DSP_SPI_MODE                0       /* CPOL=0, CPHA=0 */
#define DSP_SPI_CSN_PIN             BOARD_PIN_DSP_SPI_CSN
#define DSP_SPI_SCK_PIN             BOARD_PIN_DSP_SPI_SCK
#define DSP_SPI_MOSI_PIN            BOARD_PIN_DSP_SPI_MOSI
#define DSP_SPI_MISO_PIN            BOARD_PIN_DSP_SPI_MISO
#define DSP_HANDSHAKE_PIN           BOARD_PIN_DSP_HANDSHAKE
#define DSP_RESET_PIN               BOARD_PIN_DSP_RESET

/*===========================================================================
 * FRAME DATA INTERFACE (FPGA → MCU parallel bus)
 *===========================================================================*/

/* 8-bit parallel pixel data + sync signals from FPGA */
#define FRAME_DATA_PINS { BOARD_PIN_FRAME_DATA_0, BOARD_PIN_FRAME_DATA_1, \
                          BOARD_PIN_FRAME_DATA_2, BOARD_PIN_FRAME_DATA_3, \
                          BOARD_PIN_FRAME_DATA_4, BOARD_PIN_FRAME_DATA_5, \
                          BOARD_PIN_FRAME_DATA_6, BOARD_PIN_FRAME_DATA_7 }
#define FRAME_HSYNC_PIN            BOARD_PIN_FRAME_HSYNC
#define FRAME_VSYNC_PIN            BOARD_PIN_FRAME_VSYNC
#define FRAME_PCLK_PIN             BOARD_PIN_FRAME_PCLK
#define FRAME_DE_PIN               BOARD_PIN_FRAME_DE

/* GPIOTE channels for frame interface */
#define GPIOTE_CH_FRAME_PCLK       0    /* PCLK edge trigger for pixel capture */
#define GPIOTE_CH_FRAME_VSYNC      1    /* VSYNC edge for frame start/end */
#define GPIOTE_CH_FRAME_HSYNC      2    /* HSYNC edge for line start/end */
#define GPIOTE_CH_DSP_HANDSHAKE    3    /* DSP data-ready handshake */
#define GPIOTE_CH_BTN_USER         4    /* User button */
#define GPIOTE_CH_SD_DETECT        5    /* SD card detect */

/*===========================================================================
 * QSPI CONFIGURATION
 *===========================================================================*/

#define QSPI_FREQ_HZ                64000000UL  /* 64 MHz QSPI clock */
#define QSPI_CSN_PSRAM_PIN          BOARD_PIN_QSPI_CSN_PSRAM
#define QSPI_CSN_FLASH_PIN          BOARD_PIN_QSPI_CSN_FLASH
#define QSPI_SCK_PIN                BOARD_PIN_QSPI_CLK
#define QSPI_IO0_PIN                BOARD_PIN_QSPI_IO0
#define QSPI_IO1_PIN                BOARD_PIN_QSPI_IO1
#define QSPI_IO2_PIN                BOARD_PIN_QSPI_IO2
#define QSPI_IO3_PIN                BOARD_PIN_QSPI_IO3

/* PSRAM (APS6404L-3SQR-SN) — 8 MB frame buffer */
#define PSRAM_SIZE                  (8 * 1024 * 1024)
#define PSRAM_READ_OPCODE           0xEB    /* Fast Read Quad I/O */
#define PSRAM_WRITE_OPCODE          0x38    /* Write Quad I/O */
#define PSRAM_ENTER_QUAD_OPCODE     0x35    /* Enter Quad Mode */
#define PSRAM_DUMMY_CYCLES          6

/* NOR Flash (W25Q128JVSIQ) — 16 MB config + bitstreams + frame history */
#define FLASH_SIZE                  (16 * 1024 * 1024)
#define FLASH_READ_OPCODE           0xEB
#define FLASH_WRITE_OPCODE          0x32
#define FLASH_ERASE_4K_OPCODE       0x20
#define FLASH_ERASE_32K_OPCODE      0x52
#define FLASH_ERASE_64K_OPCODE      0xD8
#define FLASH_ERASE_CHIP_OPCODE     0xC7
#define FLASH_READ_STATUS_OPCODE    0x05
#define FLASH_WRITE_ENABLE_OPCODE   0x06
#define FLASH_DUMMY_CYCLES          8

/*===========================================================================
 * I²C CONFIGURATION (Accelerometer)
 *===========================================================================*/

#define I2C_INSTANCE                0       /* TWIM0 */
#define I2C_FREQ_HZ                 400000UL /* 400 kHz */
#define I2C_SDA_PIN                 BOARD_PIN_I2C_SDA
#define I2C_SCL_PIN                 BOARD_PIN_I2C_SCL
#define ACCEL_I2C_ADDR              0x19    /* LIS2DH12 SA0=1 */

/*===========================================================================
 * POWER MANAGEMENT
 *===========================================================================*/

#define PWR_HARVEST_EN_PIN          BOARD_PIN_PWR_HARVEST_EN
#define PWR_COINCELL_EN_PIN         BOARD_PIN_PWR_COINCELL_EN
#define PWR_DCDC_EN_PIN             BOARD_PIN_DCDC_EN

/* Power sources */
typedef enum {
    POWER_SOURCE_NONE = 0,
    POWER_SOURCE_COINCELL,
    POWER_SOURCE_HARVESTED,
    POWER_SOURCE_USB
} power_source_t;

/* Battery thresholds (mV) */
#define VBAT_FULL_MV                3200    /* CR2032 full ~3.2V */
#define VBAT_LOW_MV                 2700    /* 20% remaining */
#define VBAT_CRITICAL_MV            2400    /* Imminent shutdown */
#define VBAT_DIVIDER_RATIO          2       /* 2:1 voltage divider */
#define VBAT_ADC_REF_MV             600     /* Internal 0.6V reference */
#define VBAT_ADC_GAIN               (1.0f / 6.0f)
#define VBAT_ADC_MAX_MV             3600

/*===========================================================================
 * USER INTERFACE
 *===========================================================================*/

#define LED_RED_PIN                 BOARD_PIN_LED_RED
#define LED_GRN_PIN                 BOARD_PIN_LED_GRN
#define LED_BLU_PIN                 BOARD_PIN_LED_BLU
#define BTN_USER_PIN                BOARD_PIN_BTN_USER
#define LED_ACTIVE_HIGH             1
#define BTN_DEBOUNCE_MS             50

/*===========================================================================
 * SD CARD CONFIGURATION
 *===========================================================================*/

#define SD_SPI_INSTANCE             2       /* SPIM2 (if available) or bitbang */
#define SD_CS_PIN                   BOARD_PIN_SD_CS
#define SD_CLK_PIN                  BOARD_PIN_SD_CLK
#define SD_MOSI_PIN                 BOARD_PIN_SD_MOSI
#define SD_MISO_PIN                 BOARD_PIN_SD_MISO
#define SD_DETECT_PIN               BOARD_PIN_SD_DETECT

/*===========================================================================
 * TIMER CONFIGURATION
 *===========================================================================*/

#define TIMESTAMP_TIMER_INSTANCE    0   /* TIMER0 — 1 MHz µs counter */
#define FRAME_TIMER_INSTANCE        1   /* TIMER1 — frame rate control */
#define WDT_KICK_TIMER_INSTANCE     2   /* TIMER2 — watchdog kick */

/*===========================================================================
 * RTC CONFIGURATION
 *===========================================================================*/

#define SYSTEM_RTC_INSTANCE         0   /* RTC0 — system tick */
#define BLE_RTC_INSTANCE            1   /* RTC1 — BLE SoftDevice */
#define APP_RTC_INSTANCE            2   /* RTC2 — application timer */

/*===========================================================================
 * WATCHDOG CONFIGURATION
 *===========================================================================*/

#define WDT_TIMEOUT_SEC             8
#define WDT_RELOAD_CHANNEL          0

/*===========================================================================
 * BLE CONFIGURATION
 *===========================================================================*/

#define BLE_DEVICE_NAME             "GLYPH-REAPER"
#define BLE_MANUFACTURER_NAME       "jayis1"
#define BLE_ADV_INTERVAL_MS         100
#define BLE_CONN_INTERVAL_MIN_MS    7.5
#define BLE_CONN_INTERVAL_MAX_MS    15
#define BLE_SLAVE_LATENCY           0
#define BLE_SUP_TIMEOUT_MS          4000
#define BLE_TX_POWER_DBM            0
#define BLE_NUS_SERVICE_UUID        0xFE40      /* Custom service UUID */
#define BLE_NUS_TX_CHAR_UUID        0xFE41      /* Frame data TX */
#define BLE_NUS_RX_CHAR_UUID        0xFE42      /* Command RX */
#define BLE_NUS_MAX_DATA_LEN        244         /* With DLE enabled */

/* Encryption */
#define BLE_AES_KEY_SIZE            32      /* 256-bit AES key */
#define BLE_AES_GCM_TAG_SIZE        16      /* GCM authentication tag */
#define BLE_AES_IV_SIZE             12      /* 96-bit IV */
#define BLE_FRAME_KEY_ROTATION      100     /* Re-key every 100 frames */

/*===========================================================================
 * USB CONFIGURATION
 *===========================================================================*/

#define USB_VID                     0x1915  /* Nordic Semiconductor */
#define USB_PID                     0xFE40  /* GLYPH-REAPER */
#define USB_BCD_DEVICE              0x0100  /* Rev 1.0 */
#define USB_MANUFACTURER_STRING     "jayis1"
#define USB_PRODUCT_STRING          "GLYPH-REAPER"
#define USB_CDC_BAUD_RATE           921600
#define USB_CDC_DATA_BITS           8
#define USB_CDC_STOP_BITS           1
#define USB_CDC_PARITY              0
#define USB_EP_BULK_IN              1
#define USB_EP_BULK_OUT             1
#define USB_EP_INTERRUPT_IN         2
#define USB_BULK_MAX_PACKET_SIZE    64
#define USB_INTERRUPT_MAX_PACKET_SIZE 16

/*===========================================================================
 * MEMORY MAP
 *===========================================================================*/

/* Internal Flash */
#define FLASH_APP_START_ADDR        0x00026000UL
#define FLASH_APP_SIZE              (872 * 1024)
#define FLASH_DFU_SETTINGS_ADDR     0x0006E000UL
#define FLASH_CONFIG_PAGE_ADDR      0x0006E000UL

/* Internal RAM */
#define RAM_APP_START_ADDR          0x2000B000UL
#define RAM_APP_SIZE                (212 * 1024)
#define RAM_HEAP_SIZE               (32 * 1024)

/* External PSRAM — 8 MB frame buffer allocation */
#define PSRAM_BASE_ADDR             0x00800000UL
#define PSRAM_FRAME_CURRENT         0x00000000UL   /* Current frame buffer (2 MB max) */
#define PSRAM_FRAME_CURRENT_SIZE    (2 * 1024 * 1024)
#define PSRAM_FRAME_PREVIOUS        0x00200000UL   /* Previous frame for delta (2 MB) */
#define PSRAM_FRAME_PREVIOUS_SIZE   (2 * 1024 * 1024)
#define PSRAM_DELTA_BUFFER          0x00400000UL   /* Delta-compressed output (1 MB) */
#define PSRAM_DELTA_BUFFER_SIZE     (1 * 1024 * 1024)
#define PSRAM_OCR_TEXT_BUFFER       0x00500000UL   /* OCR text results (512 KB) */
#define PSRAM_OCR_TEXT_SIZE         (512 * 1024)
#define PSRAM_FRAME_QUEUE           0x00580000UL   /* Frame queue for BLE streaming (512 KB) */
#define PSRAM_FRAME_QUEUE_SIZE      (512 * 1024)
#define PSRAM_SCRATCH               0x00600000UL   /* Scratch/working memory (2 MB) */
#define PSRAM_SCRATCH_SIZE          (2 * 1024 * 1024)

/* External NOR Flash — 16 MB allocation */
#define NOR_FLASH_BASE_ADDR         0x01000000UL
#define NOR_FLASH_FPGA_BITSTREAMS   0x00000000UL   /* 4 × 128 KB bitstreams (512 KB) */
#define NOR_FLASH_BITSTREAMS_SIZE   (512 * 1024)
#define NOR_FLASH_OCR_MODEL         0x00080000UL   /* OCR CNN model (~200 KB) */
#define NOR_FLASH_OCR_MODEL_SIZE    (512 * 1024)
#define NOR_FLASH_CONFIG            0x00100000UL   /* Device configuration (64 KB) */
#define NOR_FLASH_CONFIG_SIZE       (64 * 1024)
#define NOR_FLASH_FRAME_HISTORY     0x00110000UL   /* Frame metadata history (1 MB) */
#define NOR_FLASH_FRAME_HISTORY_SIZE (1 * 1024 * 1024)
#define NOR_FLASH_DFU_IMAGE         0x00210000UL   /* OTA firmware images (1 MB) */
#define NOR_FLASH_DFU_IMAGE_SIZE    (1 * 1024 * 1024)
#define NOR_FLASH_RESERVED          0x00310000UL   /* Reserved (12.9 MB) */
#define NOR_FLASH_RESERVED_SIZE     (12700 * 1024)

/*===========================================================================
 * DISPLAY PROTOCOL CONFIGURATION
 *===========================================================================*/

typedef enum {
    DISPLAY_PROTO_NONE = 0,
    DISPLAY_PROTO_MIPI_DSI,       /* MIPI DSI, 1-4 lanes */
    DISPLAY_PROTO_RGB_PARALLEL,   /* RGB parallel, 8/16/18/24-bit */
    DISPLAY_PROTO_SPI_LCD,        /* SPI LCD (ILI9341/ST7789/SSD1306) */
    DISPLAY_PROTO_LVDS            /* LVDS single/dual link */
} display_protocol_t;

typedef enum {
    COLOR_DEPTH_RGB565 = 16,
    COLOR_DEPTH_RGB888 = 24,
    COLOR_DEPTH_YUV422 = 16
} color_depth_t;

typedef enum {
    TRIGGER_CONTINUOUS = 0,       /* Capture every frame */
    TRIGGER_ON_CHANGE,            /* Capture only when frame changes */
    TRIGGER_ON_MOTION,            /* Capture on accelerometer motion */
    TRIGGER_SCHEDULED             /* Capture at scheduled intervals */
} trigger_mode_t;

/* Maximum supported resolution */
#define MAX_DISPLAY_WIDTH           1920
#define MAX_DISPLAY_HEIGHT          1080
#define MAX_FRAME_SIZE_RGB565       (MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT * 2)
#define MAX_FRAME_SIZE_RGB888       (MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT * 3)

/* Capture rate limits */
#define MAX_CAPTURE_FPS             30
#define MIN_CAPTURE_FPS             1
#define DEFAULT_CAPTURE_FPS         15

/*===========================================================================
 * COMPRESSION CONFIGURATION
 *===========================================================================*/

#define COMPRESSION_RLE_THRESHOLD   4     /* RLE if run >= 4 pixels */
#define DELTA_BLOCK_SIZE             8     /* 8×8 pixel blocks for delta */
#define MAX_DELTA_RATIO              0.95  /* Max fraction of changed blocks to send full frame */

/*===========================================================================
 * MACROS
 *===========================================================================*/

/* GPIO manipulation */
#define GPIO_OUT_SET(port, pin)     NRF_P##port->OUTSET = (1UL << (pin))
#define GPIO_OUT_CLR(port, pin)     NRF_P##port->OUTCLR = (1UL << (pin))
#define GPIO_OUT_TGL(port, pin)     NRF_P##port->OUT ^= (1UL << (pin))
#define GPIO_IN_READ(port, pin)     ((NRF_P##port->IN >> (pin)) & 1UL)

/* LED control */
#define LED_RED_ON()                GPIO_OUT_SET(1, BOARD_PIN_LED_RED)
#define LED_RED_OFF()               GPIO_OUT_CLR(1, BOARD_PIN_LED_RED)
#define LED_RED_TGL()               GPIO_OUT_TGL(1, BOARD_PIN_LED_RED)
#define LED_GRN_ON()                GPIO_OUT_SET(1, BOARD_PIN_LED_GRN)
#define LED_GRN_OFF()               GPIO_OUT_CLR(1, BOARD_PIN_LED_GRN)
#define LED_GRN_TGL()               GPIO_OUT_TGL(1, BOARD_PIN_LED_GRN)
#define LED_BLU_ON()                GPIO_OUT_SET(1, BOARD_PIN_LED_BLU)
#define LED_BLU_OFF()               GPIO_OUT_CLR(1, BOARD_PIN_LED_BLU)

/* FPGA control */
#define FPGA_RESET_ASSERT()         GPIO_OUT_CLR(0, FPGA_CRSTB_PIN)
#define FPGA_RESET_RELEASE()        GPIO_OUT_SET(0, FPGA_CRSTB_PIN)
#define FPGA_CDONE_IS_HIGH()        GPIO_IN_READ(0, FPGA_CDONE_PIN)

/* DSP control */
#define DSP_RESET_ASSERT()          GPIO_OUT_CLR(0, DSP_RESET_PIN)
#define DSP_RESET_RELEASE()         GPIO_OUT_SET(0, DSP_RESET_PIN)
#define DSP_HANDSHAKE_IS_HIGH()     GPIO_IN_READ(0, DSP_HANDSHAKE_PIN)

/* Power control */
#define PWR_HARVEST_ENABLE()        GPIO_OUT_SET(1, PWR_HARVEST_EN_PIN)
#define PWR_HARVEST_DISABLE()       GPIO_OUT_CLR(1, PWR_HARVEST_EN_PIN)
#define PWR_COINCELL_ENABLE()       GPIO_OUT_SET(1, PWR_COINCELL_EN_PIN)
#define PWR_COINCELL_DISABLE()      GPIO_OUT_CLR(1, PWR_COINCELL_EN_PIN)
#define PWR_DCDC_ENABLE()           GPIO_OUT_SET(1, PWR_DCDC_EN_PIN)
#define PWR_DCDC_DISABLE()          GPIO_OUT_CLR(1, PWR_DCDC_EN_PIN)

/* Button reading */
#define BTN_IS_PRESSED()            (GPIO_IN_READ(1, BOARD_PIN_BTN_USER) == 0)
#define SD_IS_PRESENT()             (GPIO_IN_READ(1, BOARD_PIN_SD_DETECT) == 0)

/*===========================================================================
 * FUNCTION PROTOTYPES
 *===========================================================================*/

/**
 * @brief Initialize all board GPIO pins to their default states
 */
void board_gpio_init(void);

/**
 * @brief Initialize board-specific clocks (HFCLK, LFCLK)
 */
void board_clock_init(void);

/**
 * @brief Initialize power management (DC/DC, load switches, source selection)
 */
void board_power_init(void);

/**
 * @brief Read battery voltage in millivolts
 * @return Battery voltage in mV
 */
uint16_t board_battery_read_mv(void);

/**
 * @brief Get current power source
 * @return Active power source enum
 */
power_source_t board_get_power_source(void);

/**
 * @brief Get board temperature from nRF52840 internal TEMP sensor
 * @return Temperature in °C (signed)
 */
int8_t board_temperature_read(void);

/**
 * @brief Enter system OFF mode (deepest sleep, wake on pin reset or BLE)
 */
void board_system_off(void);

/**
 * @brief Get unique device ID from FICR
 * @param id_out Buffer to store 8-byte device ID
 */
void board_get_device_id(uint8_t id_out[8]);

/**
 * @brief Select active power source (coin cell or harvested)
 * @param source Power source to activate
 */
void board_select_power_source(power_source_t source);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */