/**
 * @file board.h
 * @author jayis1
 * @brief Board-level pin definitions and configuration for SATA Phantom
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * Hardware Revision
 * =================================================================== */
#define BOARD_REVISION_MAJOR   1
#define BOARD_REVISION_MINOR   0
#define BOARD_NAME             "SATA Phantom v1.0"
#define BOARD_MANUFACTURER     "jayis1"

/* ===================================================================
 * FPGA Interface (SPI Slave on Gowin GW1N-LV1)
 * =================================================================== */

/* SPI bus to/from FPGA */
#define PIN_FPGA_SPI_MOSI      GPIO_NUM_11
#define PIN_FPGA_SPI_MISO      GPIO_NUM_12
#define PIN_FPGA_SPI_SCLK      GPIO_NUM_13
#define PIN_FPGA_SPI_CS        GPIO_NUM_10      /* Active low chip select */

/* FPGA control signals */
#define PIN_FPGA_IRQ           GPIO_NUM_14      /* FPGA → ESP32: frame match interrupt */
#define PIN_FPGA_RESET         GPIO_NUM_15      /* ESP32 → FPGA: reset */
#define PIN_FPGA_BUSY          GPIO_NUM_16      /* FPGA → ESP32: processing busy flag */
#define PIN_FPGA_DONE          GPIO_NUM_17      /* FPGA → ESP32: configuration done */

/* SPI clock speed for FPGA communication */
#define FPGA_SPI_CLOCK_HZ      (40 * 1000 * 1000)  /* 40 MHz */

/* SPI transaction timeouts (in FreeRTOS ticks, portTICK_PERIOD_MS = 10 ms) */
#define FPGA_SPI_TX_TIMEOUT    pdMS_TO_TICKS(100)
#define FPGA_SPI_RX_TIMEOUT    pdMS_TO_TICKS(100)
#define FPGA_SPI_CMD_TIMEOUT   pdMS_TO_TICKS(500)

/* FPGA register address space */
#define FPGA_REG_STATUS        0x00    /* Read: status flags */
#define FPGA_REG_CTRL          0x01    /* Write: control register */
#define FPGA_REG_RULE_COUNT    0x02    /* Read/Write: number of active rules */
#define FPGA_REG_RULE_ADDR_LO  0x04    /* Rule LBA range start (low 32 bits) */
#define FPGA_REG_RULE_ADDR_HI  0x05    /* Rule LBA range start (high 32 bits) */
#define FPGA_REG_RULE_MASK_LO  0x06    /* Rule LBA range mask (low 32 bits) */
#define FPGA_REG_RULE_MASK_HI  0x07    /* Rule LBA range mask (high 32 bits) */
#define FPGA_REG_RULE_OPCODE   0x08    /* Rule SATA opcode filter */
#define FPGA_REG_RULE_ACTION   0x09    /* Rule action: 0=monitor, 1=drop, 2=modify, 3=inject */
#define FPGA_REG_RULE_LOAD     0x0A    /* Write: load current rule into rule slot */
#define FPGA_REG_FIFO_STATUS   0x10    /* Read: injection FIFO status */
#define FPGA_REG_FIFO_CTRL     0x11    /* Write: injection FIFO control */
#define FPGA_REG_FIFO_DATA     0x12    /* Write: push data to injection FIFO */
#define FPGA_REG_SCRATCH_ADDR  0x20    /* Read/Write: scratch buffer address */
#define FPGA_REG_SCRATCH_DATA  0x21    /* Read/Write: scratch buffer data port */
#define FPGA_REG_CAPTURE_CTRL  0x30    /* Write: capture start/stop */
#define FPGA_REG_CAPTURE_LEN   0x31    /* Read: captured frame length */
#define FPGA_REG_VERSION       0xFE    /* Read: firmware version register */
#define FPGA_REG_SCRATCH       0xFF    /* Read/Write: general purpose scratch */

/* Status register bit definitions */
#define FPGA_STATUS_RULE_MATCH    (1 << 0)  /* A rule matched the last frame */
#define FPGA_STATUS_FIFO_FULL     (1 << 1)  /* Injection FIFO is full */
#define FPGA_STATUS_FIFO_EMPTY    (1 << 2)  /* Injection FIFO is empty */
#define FPGA_STATUS_CRC_ERROR     (1 << 3)  /* Frame CRC error detected */
#define FPGA_STATUS_OOB_DETECT    (1 << 4)  /* OOB signal detected (COMINIT/COMWAKE) */
#define FPGA_STATUS_LINK_UP       (1 << 5)  /* SATA link is established */
#define FPGA_STATUS_SPEED_MASK    (3 << 6)  /* Negotiated speed: 0=1.5, 1=3.0, 2=6.0 Gbps */
#define FPGA_STATUS_BUSY          (1 << 8)  /* FPGA processing a frame */
#define FPGA_STATUS_CONFIG_DONE   (1 << 9)  /* FPGA configuration complete */

/* Control register bit definitions */
#define FPGA_CTRL_RESET            (1 << 0)  /* Reset FPGA logic (self-clearing) */
#define FPGA_CTRL_ENABLE_MONITOR   (1 << 1)  /* Enable frame monitoring */
#define FPGA_CTRL_ENABLE_MODIFY    (1 << 2)  /* Enable frame modification */
#define FPGA_CTRL_ENABLE_CAPTURE   (1 << 3)  /* Enable frame capture to scratch */
#define FPGA_CTRL_PASSTHROUGH      (1 << 4)  /* Force passthrough (disable all filtering) */
#define FPGA_CTRL_ZERO_JITTER      (1 << 5)  /* Enable constant-delay mode */
#define FPGA_CTRL_LOOPBACK         (1 << 6)  /* Internal loopback (test mode) */

/* ===================================================================
 * SATA Bus Monitoring (from FPGA status)
 * =================================================================== */
typedef enum {
    SATA_SPEED_1_5 = 0,   /* SATA Gen1 — 1.5 Gbps */
    SATA_SPEED_3_0 = 1,   /* SATA Gen2 — 3.0 Gbps */
    SATA_SPEED_6_0 = 2,   /* SATA Gen3 — 6.0 Gbps */
    SATA_SPEED_UNKNOWN = 3
} sata_speed_t;

typedef enum {
    SATA_DIR_READ  = 0,   /* Drive → Host */
    SATA_DIR_WRITE = 1,   /* Host → Drive */
    SATA_DIR_OTHER = 2
} sata_direction_t;

/* SATA FIS types */
#define FIS_TYPE_REG_H2D     0x27  /* Register - Host to Device */
#define FIS_TYPE_REG_D2H     0x34  /* Register - Device to Host */
#define FIS_TYPE_DMA_ACTIVATE 0x39 /* DMA Activate */
#define FIS_TYPE_DMA_SETUP   0x41  /* DMA Setup */
#define FIS_TYPE_DATA        0x46  /* Data FIS */
#define FIS_TYPE_BIST        0x58  /* BIST Activate */
#define FIS_TYPE_PIO_SETUP   0x5F  /* PIO Setup */
#define FIS_TYPE_DEV_BITS    0xA1  /* Device Bits */

/* ===================================================================
 * e-Paper Display (GDEH0154D67)
 * =================================================================== */
#define PIN_EPD_BUSY          GPIO_NUM_7
#define PIN_EPD_RESET         GPIO_NUM_8
#define PIN_EPD_DC            GPIO_NUM_18
#define PIN_EPD_CS            GPIO_NUM_9
#define PIN_EPD_SCLK          GPIO_NUM_3
#define PIN_EPD_MOSI          GPIO_NUM_46

/* ===================================================================
 * Battery Monitor (MAX17048)
 * =================================================================== */
#define PIN_BAT_I2C_SDA       GPIO_NUM_1
#define PIN_BAT_I2C_SCL       GPIO_NUM_2
#define BAT_I2C_PORT          I2C_NUM_0
#define MAX17048_ADDR         0x36   /* 7-bit I2C address */
#define BAT_ALERT_PIN         GPIO_NUM_5  /* MAX17048 ALRT output */

/* ===================================================================
 * USB-UART (CP2102N) — Debug Console
 * =================================================================== */
#define PIN_DEBUG_UART_TX     GPIO_NUM_43
#define PIN_DEBUG_UART_RX     GPIO_NUM_44
#define DEBUG_UART_PORT       UART_NUM_0
#define DEBUG_UART_BAUD       115200
#define DEBUG_UART_BUF_SIZE   2048

/* ===================================================================
 * WiFi / BLE Antenna (ESP32-S3 internal)
 * =================================================================== */
#define PIN_WIFI_ANTENNA_SEL  GPIO_NUM_21  /* If external antenna switch used */

/* ===================================================================
 * LED Indicators
 * =================================================================== */
#define PIN_LED_STATUS        GPIO_NUM_6    /* RGB LED data / single GPIO */

/* RGB LED (if WS2812 / NeoPixel) */
#define LED_TYPE_RGB
#define LED_PIN_WS2812        GPIO_NUM_48
#define LED_COUNT             1

/* ===================================================================
 * Power Management
 * =================================================================== */
#define PIN_PWR_5V_EN         GPIO_NUM_35   /* Enable 5V rail from SATA */
#define PIN_PWR_3V3_EN        GPIO_NUM_36   /* Enable 3.3V rail */
#define PIN_PWR_1V2_EN        GPIO_NUM_37   /* Enable 1.2V FPGA core */
#define PIN_PWR_BAT_EN        GPIO_NUM_38   /* Enable battery boost */
#define PIN_PWR_SATA_DETECT   GPIO_NUM_39   /* SATA power rail present */
#define PIN_PWR_USB_DETECT    GPIO_NUM_40   /* USB-C power present */

/* Power source enum */
typedef enum {
    PWR_SOURCE_SATA_5V = 0,
    PWR_SOURCE_SATA_12V = 1,
    PWR_SOURCE_USB_C = 2,
    PWR_SOURCE_BATTERY = 3,
    PWR_SOURCE_NONE = 4
} power_source_t;

/* ===================================================================
 * Operation Mode
 * =================================================================== */
typedef enum {
    MODE_TRANSPARENT = 0,   /* Passthrough only — no inspection */
    MODE_MONITOR    = 1,    /* Inspect and alert only */
    MODE_ACTIVE     = 2,    /* Inspect and modify/inject */
    MODE_EXFIL      = 3,    /* Active + exfiltrate over RF */
    MODE_DEEP_SLEEP = 4,    /* RTC-only, wake on OOB */
    MODE_USB_CONFIG = 5     /* USB enumeration for firmware update */
} operation_mode_t;

/* ===================================================================
 * Rule Structure (transmitted to FPGA)
 * =================================================================== */
typedef struct __attribute__((packed)) {
    uint64_t lba_start;          /* LBA range start */
    uint64_t lba_mask;           /* LBA range mask (bitwise AND for match) */
    uint8_t  opcode;             /* SATA command opcode (0x00 = any) */
    uint8_t  opcode_mask;        /* Opcode match mask */
    uint8_t  direction;          /* 0=read, 1=write, 0xFF=both */
    uint8_t  action;             /* 0=monitor, 1=drop, 2=corrupt, 3=inject_after */
    uint8_t  data_pattern[16];   /* Content match pattern (first 16 bytes) */
    uint8_t  data_mask[16];      /* Content match mask */
    uint8_t  priority;           /* Rule priority (0 = highest) */
    uint8_t  enabled;            /* 1 = active */
} fpga_rule_t;

/* Maximum number of hardware rules supported by FPGA */
#define FPGA_MAX_RULES          16

/* ===================================================================
 * Exfiltration Configuration
 * =================================================================== */
#define EXFIL_CHUNK_SIZE        1024    /* Data chunk size for exfiltration */
#define EXFIL_MAX_QUEUE         128     /* Max queued chunks waiting to send */
#define EXFIL_WIFI_CHANNEL_MIN  1
#define EXFIL_WIFI_CHANNEL_MAX  11      /* 2.4 GHz band (avoid DFS) */
#define EXFIL_MAC_ROTATE_INTERVAL_S  300  /* Rotate MAC every 5 minutes */

/* ===================================================================
 * Version Info (reported via API)
 * =================================================================== */
#define FW_VERSION_MAJOR       1
#define FW_VERSION_MINOR       0
#define FW_VERSION_PATCH       0
#define FW_VERSION_STRING      "1.0.0"
#define FW_BUILD_DATE          __DATE__
#define FW_BUILD_TIME          __TIME__

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
