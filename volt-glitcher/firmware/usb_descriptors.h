/*
 * usb_descriptors.h — USB device descriptors for Volt Glitcher
 * STM32F407 OTG FS full-speed USB configuration
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ========================================================================
 * USB VID/PID
 * ======================================================================== */

#define USB_VID                  0x1209    /* pid.codes — open source VID */
#define USB_PID                  0xAC11    /* Volt Glitcher PID */
#define USB_DEVICE_VERSION       0x0100    /* 1.0.0 BCD */

/* ========================================================================
 * Device Descriptor (18 bytes)
 * ======================================================================== */

static const uint8_t device_descriptor[] = {
    0x12,                           /* bLength = 18 */
    0x01,                           /* bDescriptorType = DEVICE */
    0x00, 0x02,                     /* bcdUSB = 2.00 */
    0x00,                           /* bDeviceClass = Use interface class */
    0x00,                           /* bDeviceSubClass */
    0x00,                           /* bDeviceProtocol */
    0x40,                           /* bMaxPacketSize0 = 64 bytes */
    0x09, 0x12,                     /* idVendor = 0x1209 */
    0x11, 0xAC,                     /* idProduct = 0xAC11 */
    0x00, 0x01,                     /* bcdDevice = 1.00 */
    0x01,                           /* iManufacturer = string index 1 */
    0x02,                           /* iProduct = string index 2 */
    0x03,                           /* iSerialNumber = string index 3 */
    0x01                            /* bNumConfigurations = 1 */
};

/* ========================================================================
 * Configuration Descriptor (9 bytes) + Interface + Endpoint descriptors
 * Total configuration: 1 interface, 3 endpoints
 *   EP1 OUT (0x01): Command channel (64 bytes, bulk)
 *   EP2 IN  (0x82): Response channel (64 bytes, bulk)
 *   EP3 IN  (0x83): Streaming data / trigger events (64 bytes, interrupt)
 * ======================================================================== */

#define CFG_DESC_TOTAL_LEN       57U

static const uint8_t config_descriptor[] = {
    /* --- Configuration Descriptor (9 bytes) --- */
    0x09,                           /* bLength */
    0x02,                           /* bDescriptorType = CONFIGURATION */
    CFG_DESC_TOTAL_LEN, 0x00,      /* wTotalLength */
    0x01,                           /* bNumInterfaces = 1 */
    0x01,                           /* bConfigurationValue = 1 */
    0x00,                           /* iConfiguration = no string */
    0x80,                           /* bmAttributes = bus-powered */
    0xFA,                           /* bMaxPower = 500 mA (250 * 2) */

    /* --- Interface Descriptor (9 bytes) --- */
    0x09,                           /* bLength */
    0x04,                           /* bDescriptorType = INTERFACE */
    0x00,                           /* bInterfaceNumber = 0 */
    0x00,                           /* bAlternateSetting = 0 */
    0x03,                           /* bNumEndpoints = 3 */
    0xFF,                           /* bInterfaceClass = Vendor Specific */
    0xFF,                           /* bInterfaceSubClass = Vendor Specific */
    0xFF,                           /* bInterfaceProtocol = Vendor Specific */
    0x04,                           /* iInterface = string index 4 */

    /* --- EP1 OUT: Command endpoint (7 bytes) --- */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType = ENDPOINT */
    0x01,                           /* bEndpointAddress = OUT 1 */
    0x02,                           /* bmAttributes = Bulk */
    0x40, 0x00,                     /* wMaxPacketSize = 64 */
    0x00,                           /* bInterval = 0 (bulk, ignore) */

    /* --- EP2 IN: Response endpoint (7 bytes) --- */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType = ENDPOINT */
    0x82,                           /* bEndpointAddress = IN 2 */
    0x02,                           /* bmAttributes = Bulk */
    0x40, 0x00,                     /* wMaxPacketSize = 64 */
    0x00,                           /* bInterval = 0 */

    /* --- EP3 IN: Interrupt / event notification (7 bytes) --- */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType = ENDPOINT */
    0x83,                           /* bEndpointAddress = IN 3 */
    0x03,                           /* bmAttributes = Interrupt */
    0x40, 0x00,                     /* wMaxPacketSize = 64 */
    0x01,                           /* bInterval = 1 ms (polling) */
};

/* ========================================================================
 * String Descriptors
 * ======================================================================== */

/* String 0: Language ID descriptor */
static const uint8_t string0_descriptor[] = {
    0x04,       /* bLength = 4 */
    0x03,       /* bDescriptorType = STRING */
    0x09, 0x04  /* wLANGID = English (US) */
};

/* String 1: Manufacturer */
static const uint8_t string1_descriptor[] = {
    0x1C,       /* bLength = 28 */
    0x03,       /* bDescriptorType = STRING */
    'H', 0x00, 'a', 0x00, 'c', 0x00, 'k', 0x00,
    'e', 0x00, 'r', 0x00, ' ', 0x00, 'D', 0x00,
    'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00,
    'e', 0x00, 's', 0x00
};

/* String 2: Product */
static const uint8_t string2_descriptor[] = {
    0x1E,       /* bLength = 30 */
    0x03,       /* bDescriptorType = STRING */
    'V', 0x00, 'o', 0x00, 'l', 0x00, 't', 0x00,
    'G', 0x00, 'l', 0x00, 'i', 0x00, 't', 0x00,
    'c', 0x00, 'h', 0x00, 'e', 0x00, 'r', 0x00,
    ' ', 0x00, 'P', 0x00, 'r', 0x00, 'o', 0x00
};

/* String 3: Serial number — will be patched at runtime with STM32 UID */
static const uint8_t string3_descriptor[] = {
    0x1A,       /* bLength = 26 */
    0x03,       /* bDescriptorType = STRING */
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00
};

/* String 4: Interface name */
static const uint8_t string4_descriptor[] = {
    0x2A,       /* bLength = 42 */
    0x03,       /* bDescriptorType = STRING */
    'V', 0x00, 'o', 0x00, 'l', 0x00, 't', 0x00,
    'G', 0x00, 'l', 0x00, 'i', 0x00, 't', 0x00,
    'c', 0x00, 'h', 0x00, 'e', 0x00, 'r', 0x00,
    ' ', 0x00, 'C', 0x00, 'o', 0x00, 'n', 0x00,
    't', 0x00, 'r', 0x00, 'o', 0x00, 'l', 0x00
};

/* ========================================================================
 * Descriptor table for runtime lookup
 * ======================================================================== */

typedef struct {
    const uint8_t *data;
    uint8_t  length;
} usb_string_entry_t;

static const usb_string_entry_t usb_strings[] = {
    { string0_descriptor, sizeof(string0_descriptor) },
    { string1_descriptor, sizeof(string1_descriptor) },
    { string2_descriptor, sizeof(string2_descriptor) },
    { string3_descriptor, sizeof(string3_descriptor) },
    { string4_descriptor, sizeof(string4_descriptor) },
};

#define USB_STRING_COUNT    (sizeof(usb_strings) / sizeof(usb_strings[0]))

/* ========================================================================
 * USB Protocol Command Structure (vendor requests via EP1 OUT)
 * ======================================================================== */

/* Command packet format (64 bytes, sent to EP1 OUT):
 *   [0]    CMD byte (USB_CMD_xxx)
 *   [1]    Sub-command / mode
 *   [2:3]  Parameter 1 (little-endian)
 *   [4:5]  Parameter 2 (little-endian)
 *   [6:7]  Parameter 3 (little-endian)
 *   [8:9]  Parameter 4 (little-endian)
 *   [10]   Checksum (XOR of bytes 0-9)
 *   [11:63] Payload (for large transfers like waveform data)
 */

#define USB_CMD_PKT_SIZE      64U
#define USB_CMD_IDX           0U
#define USB_SUBCMD_IDX        1U
#define USB_PARAM1_IDX        2U
#define USB_PARAM2_IDX        4U
#define USB_PARAM3_IDX        6U
#define USB_PARAM4_IDX        8U
#define USB_CHECKSUM_IDX      10U
#define USB_PAYLOAD_START      11U
#define USB_PAYLOAD_MAX_LEN    53U    /* 64 - 11 */

/* Response packet format (64 bytes, from EP2 IN):
 *   [0]    CMD echo
 *   [1]    Status (0=OK, nonzero=error code)
 *   [2:3]  Result 1
 *   [4:5]  Result 2
 *   [6:9]  Result 3 (32-bit)
 *   [10]   Checksum
 *   [11:63] Response payload
 */

#define USB_RESP_STATUS_OK       0x00
#define USB_RESP_STATUS_ERROR    0x01
#define USB_RESP_STATUS_BUSY      0x02
#define USB_RESP_STATUS_INVALID   0x03
#define USB_RESP_STATUS_FAULT     0x04
#define USB_RESP_STATUS_TIMEOUT   0x05
#define USB_RESP_STATUS_UNARMED   0x06
#define USB_RESP_STATUS_NO_FPGA   0x07

/* ========================================================================
 * Glitch Configuration Packet (USB_CMD_SET_GLITCH_CFG payload)
 * ======================================================================== */

typedef struct __attribute__((packed)) {
    uint8_t  mode;              /* glitch_mode_t */
    uint8_t  shape;             /* glitch_shape_t */
    uint8_t  trigger_mode;      /* trigger_mode_t */
    uint8_t  trigger_edge;      /* 0=rising, 1=falling, 2=both */
    uint16_t delay_ns;          /* Trigger-to-glitch delay in ns */
    uint16_t width_ns;          /* Glitch pulse width in ns */
    uint16_t repeat_count;      /* Number of glitch pulses in burst */
    uint16_t repeat_delay_ns;   /* Inter-pulse delay in ns */
    uint16_t em_amplitude;      /* EM pulse DAC value (0-1023) */
    uint16_t clk_phase_offset; /* Clock glitch phase offset in ps */
    uint8_t  uart_pattern[4];  /* UART trigger match pattern */
    uint8_t  uart_mask;        /* UART pattern match mask */
    uint8_t  jtag_state;       /* JTAG TAP state to trigger on */
    uint8_t  gpio_trigger_pin; /* GPIO trigger pin select */
    uint8_t  padding[3];       /* Alignment padding */
    uint32_t vcc_target_mv;    /* Target VCC in mV (for voltage glitch) */
    uint32_t max_current_ma;   /* Max allowed target current */
    uint8_t  auto_arm;         /* Auto-rearm after glitch fire */
    uint8_t  safety_limit;     /* Safety cutoff enable */
    uint8_t  reserved[2];
} usb_glitch_config_t;

/* ========================================================================
 * Results/Status Packet (USB_CMD_GET_RESULTS response payload)
 * ======================================================================== */

typedef struct __attribute__((packed)) {
    uint32_t glitch_count;     /* Total glitches fired since arm */
    uint32_t trigger_count;   /* Total trigger events detected */
    uint32_t last_timestamp;  /* Timestamp of last trigger (FPGA clock cycles) */
    int16_t  target_vcc_mv;   /* Measured target VCC at glitch time */
    int16_t  shunt_current_ma;/* Measured shunt current at glitch time */
    uint16_t fault_flags;     /* Fault status bits */
    uint8_t  fpga_status;     /* FPGA STATUS register echo */
    uint8_t  mode_active;     /* Currently active mode */
    uint8_t  armed;           /* 1 if armed, 0 if disarmed */
    uint8_t  temperature;     /* Board temperature in °C (approx) */
    uint16_t success_count;   /* User-marked "interesting" glitch count */
    uint16_t padding;
} usb_glitch_results_t;

/* ========================================================================
 * Event Notification (EP3 IN interrupt packet, 8 bytes)
 * ======================================================================== */

typedef struct __attribute__((packed)) {
    uint8_t  event_type;
#define USB_EVT_TRIGGER_DETECTED  0x01
#define USB_EVT_GLITCH_FIRED      0x02
#define USB_EVT_FAULT             0x03
#define USB_EVT_CALIBRATION_DONE  0x04
#define USB_EVT_OVERCURRENT       0x05
#define USB_EVT_OVERTEMP          0x06
#define USB_EVT_FPGA_READY        0x07
    uint8_t  event_data;      /* Mode-specific data */
    uint16_t event_timestamp_lo; /* Low 16 bits of timestamp */
    uint16_t event_timestamp_hi; /* High 16 bits of timestamp */
    uint8_t  reserved[2];
} usb_event_t;

/* ========================================================================
 * EEPROM Profile Storage Layout
 * ======================================================================== */

/* Each profile is 64 bytes, stored in EEPROM starting at offset 0x0000 */
#define EEPROM_PROFILE_SIZE     64U
#define EEPROM_MAX_PROFILES     16U     /* 16 profiles * 64 bytes = 1024 bytes */
#define EEPROM_PROFILE_MAGIC    0x5647  /* "VG" */

typedef struct __attribute__((packed)) {
    uint16_t magic;             /* Must be EEPROM_PROFILE_MAGIC */
    uint8_t  profile_id;       /* Profile slot 0-15 */
    uint8_t  name_len;          /* Length of profile name (max 16) */
    char     name[16];          /* Human-readable profile name */
    usb_glitch_config_t config; /* Full glitch configuration */
    uint8_t  checksum;          /* XOR of all preceding bytes */
} eeprom_profile_t;

/* Device settings stored after profile area */
#define EEPROM_SETTINGS_OFFSET  (EEPROM_MAX_PROFILES * EEPROM_PROFILE_SIZE)
#define EEPROM_SETTINGS_MAGIC   0x5345   /* "SE" */

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint16_t version;
    uint8_t  active_profile;    /* Currently selected profile ID */
    uint8_t  boot_mode;         /* Default boot mode */
    uint8_t  fan_speed;         /* Default fan PWM (0-255) */
    uint8_t  lcd_brightness;    /* LCD brightness (if present) */
    uint8_t  usb_vid[2];        /* Custom USB VID override */
    uint8_t  usb_pid[2];        /* Custom USB PID override */
    uint8_t  calibration_valid; /* 1 if calibration data present */
    int16_t  vcc_offset_mv;     /* VCC measurement offset calibration */
    int16_t  current_offset_ma; /* Current measurement offset calibration */
    uint32_t total_glitches;    /* Lifetime glitch counter */
    uint32_t power_on_cycles;   /* Lifetime power-on cycles */
    uint8_t  checksum;
} eeprom_settings_t;

/* ========================================================================
 * Waveform RAM Layout (in FPGA BRAM)
 * ======================================================================== */

/* Waveform data is uploaded to FPGA BRAM for custom glitch shapes.
 * Each entry is a 16-bit value representing the glitch amplitude at
 * a given time step. Resolution is 100ps per step.
 * Maximum 1024 samples per waveform (fits in 2KB BRAM). */

#define WAVEFORM_MAX_SAMPLES    1024U
#define WAVEFORM_SAMPLE_SIZE    2U    /* 16-bit per sample */

/* Waveform control register bits */
#define WAVEFORM_CTRL_STOP      0x00
#define WAVEFORM_CTRL_PLAY      0x01
#define WAVEFORM_CTRL_LOOP      0x02
#define WAVEFORM_CTRL_TRIGGER   0x04  /* Start on trigger */

#endif /* USB_DESCRIPTORS_H */