/*
 * board.h — FiberPhantom hardware board configuration
 * Target: Raspberry Pi RP2040 (dual Cortex-M0+ @ 133 MHz)
 *
 * Pin assignments, peripheral mappings, and board-level constants
 * for the FiberPhantom covert fiber optic tap implant.
 *
 * Author: jayis1
 * Date:   2026-06-17
 */

#ifndef FIBER_PHANTOM_BOARD_H
#define FIBER_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Board identity ---- */
#define BOARD_NAME          "FiberPhantom"
#define BOARD_VERSION       "1.0"
#define BOARD_AUTHOR        "jayis1"
#define BOARD_SERIAL_DEFAULT "FP-0000-0001"

/* ---- Clock configuration ---- */
#define XOSC_HZ             12000000U   /* 12 MHz external crystal */
#define SYS_CLK_HZ          133000000U  /* 133 MHz system clock */
#define PERI_CLK_HZ         125000000U  /* 125 MHz peripheral clock (from PLL) */

/* ---- GPIO pin assignments (RP2040 QFN-56) ---- */
/*
 * RP2040 has 30 GPIO pins (GPIO0–GPIO29).
 * Pin assignments below are for FiberPhantom board rev 1.0.
 */

/* --- SPI0: FPGA (iCE40 UP5K) interface --- */
#define PIN_FPGA_SCK        2   /* SPI0 SCK → FPGA CCLK (SPI clock) */
#define PIN_FPGA_MOSI       3   /* SPI0 MOSI → FPGA SDI (data to FPGA) */
#define PIN_FPGA_MISO       4   /* SPI0 MISO ← FPGA SDO (data from FPGA) */
#define PIN_FPGA_CS_N       5   /* SPI0 CS → FPGA CS_N (chip select, active low) */
#define PIN_FPGA_CDONE      6   /* FPGA CDONE (configuration done) */
#define PIN_FPGA_CRESET_N   7   /* FPGA CRESET_N (reset, active low) */
#define PIN_FPGA_IRQ        8   /* FPGA interrupt (frame ready, MITM match) */

/* --- SPI1: MicroSD card --- */
#define PIN_SD_SCK          10  /* SPI1 SCK → SD card SCK */
#define PIN_SD_MOSI         11  /* SPI1 MOSI → SD card MOSI (CMD) */
#define PIN_SD_MISO         12  /* SPI1 MISO ← SD card MISO (DAT0) */
#define PIN_SD_CS_N         13  /* SPI1 CS → SD card CS_N */
#define PIN_SD_DETECT       14  /* SD card detect switch (active low) */

/* --- UART1: nRF52840 BLE module --- */
#define PIN_BLE_TX          16  /* UART1 TX → nRF52840 RX */
#define PIN_BLE_RX          17  /* UART1 RX ← nRF52840 TX */
#define PIN_BLE_CTS         18  /* UART1 CTS ← nRF52840 RTS (flow control) */
#define PIN_BLE_RTS         19  /* UART1 RTS → nRF52840 CTS */
#define PIN_BLE_NRESET      20  /* nRF52840 RESET (active low, open drain) */

/* --- I2C0: APD bias DAC + power monitors --- */
#define PIN_I2C_SCL         20  /* I2C0 SCL → DAC5311 SCL */
#define PIN_I2C_SDA         21  /* I2C0 SDA → DAC5311 SDA */

/* --- APD bias control --- */
#define PIN_APD_BIAS_DAC    22  /* DAC SPI CS for APD bias voltage DAC */
#define PIN_APD_BOOST_EN    23  /* Boost converter enable (65V APD bias) */
#define PIN_APD_LEVEL_ADC   26  /* ADC0 input — TIA output level monitor */

/* --- VCSEL modulation control --- */
#define PIN_VCSEL_EN        24  /* VCSEL enable (high = active) */
#define PIN_VCSEL_MOD_P     25  /* VCSEL modulation positive (FPGA-driven) */
#define PIN_VCSEL_MOD_N     15  /* VCSEL modulation negative (complementary) */

/* --- Mode selection straps (read at boot) --- */
#define PIN_MODE_STRAP0     27  /* Mode strap bit 0 */
#define PIN_MODE_STRAP1     28  /* Mode strap bit 1 */

/* --- Status LED (stealth indicator) --- */
#define PIN_LED_R           9   /* LED red channel */
#define PIN_LED_G           29  /* LED green channel (GPIO29 is the last valid pin) */

/* --- USB-C (RP2040 built-in USB device) --- */
/* USB pins are fixed: GPIO28 (D+) and GPIO27 (D-), handled by USB peripheral */

/* ---- Deployment modes (from mode straps) ---- */
typedef enum {
    MODE_BEND_TAP       = 0,  /* Non-invasive bend coupling, read-only */
    MODE_INLINE_COUPLE  = 1,  /* Semi-invasive inline splice, tap + MITM */
    MODE_SFP_PASS_THRU  = 2,  /* SFP+ pass-through, full access */
    MODE_STANDBY        = 3,  /* Idle/charging, no optical activity */
} deploy_mode_t;

/* ---- Operating states ---- */
typedef enum {
    STATE_BOOT          = 0,
    STATE_CONFIG_FPGA   = 1,
    STATE_IDLE          = 2,
    STATE_CAPTURING     = 3,
    STATE_MITM_ACTIVE   = 4,
    STATE_SD_FULL       = 5,
    STATE_LOW_BATTERY   = 6,
    STATE_ERROR         = 0xFF,
} op_state_t;

/* ---- Link rate detection ---- */
typedef enum {
    LINK_DOWN            = 0,
    LINK_1G              = 1,  /* 1000BASE-X */
    LINK_10G             = 2,  /* 10GBASE-R */
    LINK_FC_8G           = 3,  /* Fibre Channel 8G */
    LINK_FC_16G          = 4,  /* Fibre Channel 16G */
    LINK_UNKNOWN         = 0xFF,
} link_rate_t;

/* ---- Constants ---- */
#define MITM_RULE_MAX           64    /* Maximum MITM rules in FPGA rule RAM */
#define PCAP_MAX_FILE_SIZE_MB   4096  /* Max single PCAP file: 4 GB */
#define SD_SECTOR_SIZE          512
#define PCAP_BLOCK_SIZE        4096   /* Write granularity to SD card */
#define BLE_UART_BAUD           921600
#define FPGA_SPI_HZ             48000000U  /* 48 MHz SPI to FPGA */
#define SD_SPI_HZ               24000000U  /* 24 MHz SPI to SD card */

/* ---- APD bias voltage range (for InGaAs APD) ---- */
#define APD_BIAS_MIN_MV         30000U  /* 30.0 V minimum bias */
#define APD_BIAS_MAX_MV         80000U  /* 80.0 V maximum bias */
#define APD_BIAS_DEFAULT_MV     65000U  /* 65.0 V default (optimal for C30659) */
#define APD_BIAS_STEP_MV        100U    /* 0.1 V adjustment step */

/* ---- VCSEL modulation parameters ---- */
#define VCSEL_BIAS_MA_DEFAULT   5      /* 5 mA DC bias current */
#define VCSEL_MOD_MA_DEFAULT    8      /* 8 mA modulation current */

/* ---- Battery monitoring (ADC) ---- */
#define BATT_ADC_FULL            3100  /* ADC counts for full battery (~4.2V) */
#define BATT_ADC_EMPTY           2000  /* ADC counts for empty battery (~2.8V) */
#define BATT_ADC_CHARGING        3150  /* ADC counts when charging (~4.3V) */

/* ---- Capture statistics ---- */
typedef struct {
    uint32_t total_frames;       /* Total frames captured */
    uint32_t dropped_frames;     /* Frames dropped due to buffer full */
    uint32_t mitm_modified;      /* Frames modified by MITM engine */
    uint32_t mitm_dropped;       /* Frames dropped by MITM engine */
    uint32_t bytes_captured;     /* Total bytes written to SD */
    uint32_t crc_errors;         /* Frames with CRC errors */
    link_rate_t link_rate;       /* Detected link rate */
    uint32_t uptime_sec;         /* Uptime in seconds */
} capture_stats_t;

/* ---- System status (reported to app over BLE) ---- */
typedef struct {
    deploy_mode_t mode;          /* Current deployment mode */
    op_state_t   state;          /* Current operating state */
    uint8_t       battery_pct;   /* Battery percentage (0–100) */
    uint8_t       charging;      /* 1 if USB charging */
    uint8_t       sd_inserted;    /* 1 if SD card present */
    uint8_t       sd_free_mb_lo;  /* Free space in MB (low 8 bits, for BLE) */
    uint8_t       sd_free_mb_hi; /* Free space in MB (high 8 bits) */
    uint8_t       ble_connected; /* 1 if BLE connected to app */
    uint8_t       fpga_ready;    /* 1 if FPGA configured and running */
    link_rate_t   link_rate;     /* Detected optical link rate */
    capture_stats_t stats;      /* Capture statistics */
} sys_status_t;

/* ---- MITM rule types ---- */
typedef enum {
    MITM_RULE_PASS        = 0,  /* Match frame, log only, pass through */
    MITM_RULE_DROP        = 1,  /* Match frame, drop it */
    MITM_RULE_MODIFY      = 2,  /* Match frame, replace specified bytes */
    MITM_RULE_INJECT_AFTER = 3, /* Match frame, inject additional frame after */
    MITM_RULE_INJECT_BEFORE = 4, /* Match frame, inject frame before */
    MITM_RULE_DELAY       = 5,  /* Match frame, delay by N microseconds */
} mitm_rule_type_t;

/* ---- MITM rule structure (matches FPGA rule RAM layout) ---- */
typedef struct {
    uint8_t  type;              /* mitm_rule_type_t */
    uint8_t  enabled;           /* 1 = rule active */
    uint16_t match_offset;      /* Byte offset in frame to start matching */
    uint16_t match_length;      /* Number of bytes to match */
    uint8_t  match_data[32];    /* Pattern to match (up to 32 bytes) */
    uint8_t  match_mask[32];    /* Bitwise mask applied before comparison */
    uint16_t replace_offset;    /* Byte offset to start replacement */
    uint8_t  replace_data[32]; /* Replacement data */
    uint16_t replace_length;    /* Length of replacement data */
    uint16_t delay_us;          /* Delay in microseconds (for RULE_DELAY) */
} mitm_rule_t;

_Static_assert(sizeof(mitm_rule_t) == 108, "mitm_rule_t must be 108 bytes (FPGA rule RAM)");

/* ---- PCAP file header (libpcap format) ---- */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /* 0xa1b2c3d4 for microsecond timestamps */
    uint16_t version_major;  /* 2 */
    uint16_t version_minor;  /* 4 */
    int32_t  thiszone;       /* GMT correction, usually 0 */
    uint32_t sigfigs;        /* Timestamp accuracy, usually 0 */
    uint32_t snaplen;        /* Max captured packet length */
    uint32_t network;        /* Link-layer type: 1 = Ethernet */
} pcap_header_t;

/* ---- PCAP packet record header ---- */
typedef struct __attribute__((packed)) {
    uint32_t ts_sec;         /* Timestamp seconds */
    uint32_t ts_usec;        /* Timestamp microseconds */
    uint32_t incl_len;      /* Captured length */
    uint32_t orig_len;       /* Original packet length */
} pcap_record_header_t;

#define PCAP_MAGIC          0xa1b2c3d4U
#define PCAP_VERSION_MAJOR  2
#define PCAP_VERSION_MINOR  4
#define PCAP_SNAPLEN        65535
#define PCAP_NETWORK_ETHER 1  /* LINKTYPE_ETHERNET */
#define PCAP_NETWORK_FC    196 /* LINKTYPE_FIBRE_CHANNEL */

/* ---- BLE protocol constants ---- */
#define BLE_SERVICE_UUID       "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_TX_CHAR_UUID       "6e400002-b5a3-f393-e0a9-e50e24dcca9e" /* Write (app→dev) */
#define BLE_RX_CHAR_UUID       "6e400003-b5a3-f393-e0a9-e50e24dcca9e" /* Notify (dev→app) */
#define BLE_CHUNK_SIZE         20   /* Max BLE payload per notification */
#define BLE_MAX_MSG_SIZE       256  /* Max reassembled message size */

/* ---- Firmware version ---- */
#define FW_VERSION_MAJOR       1
#define FW_VERSION_MINOR       0
#define FW_VERSION_PATCH       0

#endif /* FIBER_PHANTOM_BOARD_H */