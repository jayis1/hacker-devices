/*
 * board.h — Hardware pin map and constants for UWB-PHANTOM.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Pin assignments are for an ESP32-S3-WROOM-1-N16R8 host paired with a
 * Qorvo DW3110 UWB transceiver.  Pins were chosen so that the high-speed
 * SPI to the DW3110 lives on IO_MUX-capable GPIOs (no USB-JTAG conflict)
 * and so that the SPI flash/PSRAM of the WROOM module is untouched.
 *
 * The mapping is intentionally conservative: every peripheral can be
 * bit-banged or remapped in software if a board revision changes a pin.
 */

#ifndef UWB_PHANTOM_BOARD_H
#define UWB_PHANTOM_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ---- Host MCU identity ----------------------------------------- */

#define BOARD_NAME              "UWB-PHANTOM"
#define BOARD_AUTHOR             "jayis1"
#define BOARD_FW_VERSION_MAJOR   1
#define BOARD_FW_VERSION_MINOR   0
#define BOARD_FW_VERSION_PATCH   0
#define BOARD_FW_VERSION_STRING  "1.0.0"

/* ---- DW3110 UWB transceiver (SPI) ------------------------------ */
/*
 *  The DW3110 is the only SPI peripheral that needs >20 MHz; we route it
 *  through IO_MUX and use the dedicated SPI2 (FSPI) controller for it.
 *  A 40 MHz SCK is well within DW3110 spec with short PCB traces.
 */

#define UWB_SPI_HOST            2       /* FSPI host on ESP32-S3 */
#define UWB_SPI_SCK_GPIO        12
#define UWB_SPI_MISO_GPIO       13
#define UWB_SPI_MOSI_GPIO       11
#define UWB_SPI_CS_GPIO          10
#define UWB_RST_GPIO             9       /* DW3110 RSTn, active low */
#define UWB_IRQ_GPIO             14      /* DW3110 IRQ, active high */
#define UWB_SPI_FREQ_HZ          (40 * 1000 * 1000)
#define UWB_SPI_POLL_US          20

/* ---- SSD1306 OLED display (I2C) ------------------------------- */

#define OLED_I2C_HOST            0
#define OLED_I2C_SCL_GPIO         5
#define OLED_I2C_SDA_GPIO         4
#define OLED_I2C_FREQ_HZ          (400 * 1000)
#define OLED_I2C_ADDR             0x3C
#define OLED_WIDTH                128
#define OLED_HEIGHT               64
#define OLED_PAGES               (OLED_HEIGHT / 8)

/* ---- User buttons (active-low, internal pull-up) -------------- */

#define BTN_MODE_GPIO             6
#define BTN_UP_GPIO               7
#define BTN_DOWN_GPIO            15
#define BTN_DEBOUNCE_MS           30
#define BTN_REPEAT_MS            400

/* ---- MAX17048 fuel gauge (I2C) ------------------------------- */

#define FG_I2C_HOST              0       /* shared with OLED */
#define FG_I2C_ADDR              0x36

/* ---- WS2812 status LED -------------------------------------- */

#define LED_GPIO                  8       /* RMT channel 0 */
#define LED_COUNT                  1

/* ---- MicroSD (SPI, share FSPI bus via separate CS) ---------- */

#define SD_SPI_HOST              UWB_SPI_HOST
#define SD_SPI_CS_GPIO            16
#define SD_SPI_FREQ_HZ            (20 * 1000 * 1000)

/* ---- USB-C / UART console ---------------------------------- */

#define CONSOLE_UART_NUM          0       /* USB-CDC, no GPIO */
#define CONSOLE_BAUD              115200

/* ---- Optional second DW3110 on expansion header ------------ */

#define UWB2_SPI_CS_GPIO          17
#define UWB2_IRQ_GPIO             18
#define UWB2_RST_GPIO             21

/* ---- UWB band/channel definitions ---------------------------- */
/*
 *  We support both 802.15.4z channels 5 and 9 (the two most common in
 *  automotive digital-key and Find My deployments).  RF channel numbers
 *  map to centre frequencies: 5 -> 6489.6 MHz, 9 -> 7988.8 MHz.
 */

#define UWB_CHANNEL_5             5
#define UWB_CHANNEL_9             9
#define UWB_DEFAULT_CHANNEL       UWB_CHANNEL_9

#define UWB_PRF_16M              16       /* 16 MHz PRF (legacy) */
#define UWB_PRF_64M              64       /* 64 MHz PRF (recommended) */
#define UWB_DEFAULT_PRF          UWB_PRF_64M

#define UWB_STS_LEN_32            32
#define UWB_STS_LEN_64            64
#define UWB_STS_LEN_128          128
#define UWB_STS_LEN_256          256
#define UWB_DEFAULT_STS_LEN      UWB_STS_LEN_64

#define UWB_PLEN_64              0x21     /* 64-symbol preamble (16 MHz PRF) */
#define UWB_PLEN_128             0x81     /* 128-symbol */
#define UWB_PLEN_256             0x91     /* 256-symbol */
#define UWB_PLEN_512             0x31     /* 512-symbol */
#define UWB_PLEN_1024            0x71     /* 1024-symbol */
#define UWB_PLEN_2048            0x61     /* 2048-symbol */
#define UWB_DEFAULT_PLEN         UWB_PLEN_128

#define UWB_SFD_4A               0x01     /* IEEE 802.15.4a SFD */
#define UWB_SFD_4Z              0x02     /* IEEE 802.15.4z SFD (with STS) */

/* ---- Timing ------------------------------------------------- */
/*
 *  DW3110 device time units (DTU): 1 DTU = 1 / (499.2e6 * 128) s
 *  ≈ 15.65 ps.  Convert DTU -> metres using the speed-of-light factor.
 *  Light travels ~2.99792458e8 m/s, so 1 DTU ≈ 4.69 mm one-way,
 *  ≈ 2.346 mm round-trip.
 */

#define UWB_DTU_PER_SEC          (499200000ULL * 128ULL)
#define UWB_DTU_PER_US           (UWB_DTU_PER_SEC / 1000000ULL)
#define UWB_SPEED_OF_LIGHT_M_S   299792458.0
#define UWB_M_PER_DTU_ONEWAY     (UWB_SPEED_OF_LIGHT_M_S / (double)UWB_DTU_PER_SEC)

/* ---- Battery / power --------------------------------------- */

#define BATTERY_NOMINAL_MV       3700
#define BATTERY_FULL_MV           4200
#define BATTERY_EMPTY_MV          3300
#define BATTERY_LOW_PCT           15

/* ---- Application-level defaults ----------------------------- */

#define LOG_RING_CAPACITY        (1 << 14) /* 16K frames */
#define PCAP_SNAPLEN              256
#define TARGET_RECORD_MAX         8

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_BOARD_H */