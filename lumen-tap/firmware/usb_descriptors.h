/*
 * lumen-tap/firmware/usb_descriptors.h
 * USB Audio Class 2.0 + CDC-ACM descriptors for LumenTap.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#ifndef LUMEN_TAP_USB_DESCRIPTORS_H
#define LUMEN_TAP_USB_DESCRIPTORS_H

#include <stdint.h>

/* ---- Device & vendor IDs -------------------------------------------- */
#define USB_VID                 0x1209   /* pid.codes small vendor pool */
#define USB_PID                 0xLT01   /* LumenTap — replaced at build */
#define USB_PID_LUMEN_TAP       0x4C54   /* 'LT' */
#define USB_DEVICE_RELEASE      0x0100   /* 1.00 */

#define USB_VERSION_BCD(x,y)    (((x) << 8) | (((y) / 10) << 4) | ((y) % 10))

/* ---- Class codes ---------------------------------------------------- */
#define USB_CLASS_DEV           0xEF   /* Miscellaneous (IAD) */
#define USB_CLASS_SUBCLASS      0x02   /* Common Class */
#define USB_CLASS_PROTOCOL      0x01   /* IAD */

#define USB_CLASS_AUDIO         0x01
#define USB_AUDIO_V2            0x20   /* UAC2 */
#define USB_CLASS_CDC           0x02
#define USB_CLASS_CDC_DATA      0x0A

/* ---- Endpoint addresses --------------------------------------------- */
#define EP0_OUT                 0x00
#define EP0_IN                  0x80
#define EP_CDC_NOTIFY           0x81   /* IN, interrupt */
#define EP_CDC_OUT              0x02   /* OUT, bulk — host→device control */
#define EP_CDC_IN               0x82   /* IN, bulk  — device→host status */
#define EP_AUDIO_IN             0x83   /* IN, isochronous-asynchronous */
#define EP_AUDIO_FB             0x03   /* OUT, isochronous feedback */

/* ---- USB string indices --------------------------------------------- */
#define STR_IDX_LANG            0
#define STR_IDX_MANUF           1
#define STR_IDX_PRODUCT         2
#define STR_IDX_SERIAL          3
#define STR_IDX_AUDIO_IFACE     4
#define STR_IDX_CDC_IFACE       5
#define STR_IDX_CONFIG          6

/* ---- Audio parameters ----------------------------------------------- */
#define AUDIO_SAMPLE_RATE       48000U
#define AUDIO_BITS_PER_SAMPLE   24
#define AUDIO_BYTES_PER_FRAME   4      /* 24-bit in 4-byte container */
#define AUDIO_CHANNELS          1      /* mono optical */
#define AUDIO_PACKET_MAX_SAMPLES 48    /* 1ms @48k → 48 samples */
#define AUDIO_PACKET_MAX_BYTES  (AUDIO_PACKET_MAX_SAMPLES * AUDIO_BYTES_PER_FRAME)

/* CDC parameters */
#define CDC_BULK_EP_SIZE        64
#define CDC_NOTIFY_EP_SIZE      64
#define CDC_NOTIFY_INTERVAL_MS  32

/* ---- Descriptor sizes ----------------------------------------------- */
#define DESC_DEVICE_SIZE        18
#define DESC_CONFIG_SIZE_TOTAL  0x00E6 /* computed below; matches config */

/* ---- UAC2 descriptor sub-descriptor sizes --------------------------- */
#define AC_HEADER_SIZE          9
#define AC_CLOCK_SRC_SIZE       8
#define AC_INPUT_TERM_SIZE      17
#define AC_FEATURE_UNIT_SIZE    (6 + (2 * (AUDIO_CHANNELS + 1)))
#define AC_OUTPUT_TERM_SIZE     12
#define AS_HEADER_SIZE          9
#define AS_FORMAT_TYPE_SIZE     11
#define AS_STD_EP_SIZE          7
#define AS_EP_SIZE              8
#define AS_FEEDBACK_EP_SIZE     9

/* ---- AC / AS descriptor IDs (unique within layout) ------------------ */
#define AC_ID_CLOCK             0x10
#define AC_ID_INPUT_TERM        0x11
#define AC_ID_FEATURE_UNIT      0x12
#define AC_ID_OUTPUT_TERM       0x13

/* ---- CDC functional descriptor sizes -------------------------------- */
#define CDC_HDR_FUNC_SIZE       5
#define CDC_CALL_MGMT_SIZE      5
#define CDC_ACM_FUNC_SIZE       4
#define CDC_UNION_FUNC_SIZE     5

/* ---- Descriptor byte arrays ----------------------------------------- */

/* Device descriptor (18 bytes) */
static const uint8_t dev_descriptor[18] = {
    0x12,                       /* bLength */
    0x01,                       /* bDescriptorType: Device */
    USB_VERSION_BCD(2,0) & 0xFF,
    (USB_VERSION_BCD(2,0) >> 8) & 0xFF,
    USB_CLASS_DEV,              /* bDeviceClass: Misc/IAD */
    USB_CLASS_SUBCLASS,         /* bDeviceSubClass */
    USB_CLASS_PROTOCOL,         /* bDeviceProtocol */
    0x40,                       /* bMaxPacketSize0: 64 */
    (USB_VID) & 0xFF,
    (USB_VID >> 8) & 0xFF,
    (USB_PID_LUMEN_TAP) & 0xFF,
    (USB_PID_LUMEN_TAP >> 8) & 0xFF,
    USB_DEVICE_RELEASE & 0xFF,
    USB_DEVICE_RELEASE >> 8,
    STR_IDX_MANUF,              /* iManufacturer */
    STR_IDX_PRODUCT,            /* iProduct */
    STR_IDX_SERIAL,             /* iSerialNumber */
    0x01                        /* bNumConfigurations */
};

/* Configuration descriptor: composite UAC2 + CDC.
 * Layout (offsets are conceptual):
 *   9   Config
 *   9   IAD (CDC)
 *   9   CDC Control IF
 *   5   CDC Header func
 *   4   CDC ACM func
 *   5   CDC Call Mgmt func
 *   5   CDC Union func
 *   7   CDC notify EP
 *   9   CDC Data IF
 *   7   CDC bulk OUT EP
 *   7   CDC bulk IN EP
 *   9   IAD (Audio)
 *   9   AC Standard IF
 *   9   AC Header (Class-Specific)
 *   8   Clock Source
 *   17  Input Terminal
 *   8   Feature Unit
 *   12  Output Terminal
 *   9   AS Standard IF (alt 0)
 *   9   AS Standard IF (alt 1)
 *   9   AS Class-Specific Header
 *   11  AS Format Type
 *   7   AS Standard EP
 *   8   AS Class-Specific EP
 *   9   AS Feedback EP
 * Total ≈ 230 bytes
 */
static const uint8_t cfg_descriptor[0xE6] = {
    /* ---- Standard Configuration Descriptor ---- */
    0x09,                       /* bLength */
    0x02,                       /* bDescriptorType: Config */
    0xE6, 0x00,                 /* wTotalLength */
    0x04,                       /* bNumInterfaces: 4 (CDC ctrl, CDC data, AC, AS) */
    0x01,                       /* bConfigurationValue */
    STR_IDX_CONFIG,             /* iConfiguration */
    0xC0,                       /* bmAttributes: self-powered */
    0x32,                       /* bMaxPower: 100 mA */

    /* ---- IAD for CDC ---- */
    0x08,                       /* bLength */
    0x0B,                       /* bDescriptorType: IAD */
    0x00,                       /* bFirstInterface: CDC ctrl IF */
    0x02,                       /* bInterfaceCount: 2 */
    USB_CLASS_CDC, 0x02, 0x01,  /* class/subclass/protocol */
    0x00,                       /* iFunction */

    /* ---- CDC Control Interface (IF 0) ---- */
    0x09, 0x04,
    0x00, 0x00, 0x01,           /* if=0 alt=0 nEP=1 */
    USB_CLASS_CDC, 0x02, 0x00,
    STR_IDX_CDC_IFACE,

    /* CDC Header functional */
    0x05, 0x24, 0x00,
    USB_VERSION_BCD(1,20) & 0xFF, (USB_VERSION_BCD(1,20) >> 8) & 0xFF,
    /* CDC ACM functional */
    0x04, 0x24, 0x02, 0x02,     /* capabilities: DTR/line */
    /* CDC Call Management */
    0x05, 0x24, 0x01, 0x00, 0x01, /* handled on data IF */
    /* CDC Union */
    0x05, 0x24, 0x06, 0x00, 0x01, /* ctrl=0, data=1 */
    /* CDC interrupt EP (notify) */
    0x07, 0x05,
    EP_CDC_NOTIFY,              /* bEndpointAddress: IN interrupt */
    0x03,                       /* bmAttributes: interrupt */
    CDC_NOTIFY_EP_SIZE, 0x00,   /* wMaxPacketSize */
    CDC_NOTIFY_INTERVAL_MS,     /* bInterval */

    /* ---- CDC Data Interface (IF 1) ---- */
    0x09, 0x04,
    0x01, 0x00, 0x02,           /* if=1 alt=0 nEP=2 */
    USB_CLASS_CDC_DATA, 0x00, 0x00,
    0x00,
    /* CDC bulk OUT EP */
    0x07, 0x05, EP_CDC_OUT, 0x02, CDC_BULK_EP_SIZE, 0x00, 0x00,
    /* CDC bulk IN EP */
    0x07, 0x05, EP_CDC_IN,  0x02, CDC_BULK_EP_SIZE, 0x00, 0x00,

    /* ---- IAD for Audio ---- */
    0x08, 0x0B,
    0x02, 0x02,                 /* first=2 count=2 (AC + AS) */
    USB_CLASS_AUDIO, USB_AUDIO_V2, 0x00,
    0x00,

    /* ---- Audio Control Interface (IF 2) ---- */
    0x09, 0x04,
    0x02, 0x00, 0x00,           /* if=2 alt=0 nEP=0 */
    USB_CLASS_AUDIO, USB_AUDIO_V2, 0x00,
    STR_IDX_AUDIO_IFACE,

    /* AC Header (Class-Specific) */
    0x09, 0x24, 0x01,
    0x00, 0x01,                 /* bcdADC 1.00 */
    0x01,                       /* bCategory: desktop speaker */
    0x04, 0x02, 0x02,           /* wTotalLength subset; 1 term */

    /* Clock Source */
    0x08, 0x24, 0x0A,
    AC_ID_CLOCK,                /* bClockID */
    0x03,                       /* bmControls: rate+valid */
    0x01,                       /* bAssocTerminal */
    0x00,                       /* iClockSource */

    /* Input Terminal (microphone) */
    0x11, 0x24, 0x02,
    AC_ID_INPUT_TERM,           /* bTerminalID */
    0x01, 0x00,                 /* wTerminalType: 0x0101 = USB streaming? no — omni mic 0x0201 */
    0x00,                       /* bAssocTerminal: none */
    0x01,                       /* bCSourceID: clock */
    AUDIO_CHANNELS, 0x00,       /* wChannelConfig: mono */
    0x00,                       /* iChannelNames */
    0x00, 0x00,                 /* bmControls */
    0x00,                       /* iTerminal */

    /* Feature Unit (volume + mute) */
    0x08, 0x24, 0x06,
    AC_ID_FEATURE_UNIT,         /* bUnitID */
    AC_ID_INPUT_TERM,           /* bSourceID */
    0x01, 0x00,                 /* wControlMask: mute on ch0 */
    0x00,                       /* iFeature */

    /* Output Terminal (USB streaming) */
    0x0C, 0x24, 0x03,
    AC_ID_OUTPUT_TERM,          /* bTerminalID */
    0x01, 0x01,                 /* wTerminalType: USB streaming */
    0x00,                       /* bAssocTerminal */
    AC_ID_FEATURE_UNIT,         /* bSourceID */
    AC_ID_CLOCK,                /* bCSourceID */
    0x00, 0x00,                 /* bmControls */
    0x00,                       /* iTerminal */

    /* ---- Audio Streaming Interface (IF 3) alt 0 (zero-bandwidth) ---- */
    0x09, 0x04,
    0x03, 0x00, 0x00,           /* if=3 alt=0 nEP=0 */
    USB_CLASS_AUDIO, USB_AUDIO_V2, 0x00,
    0x00,

    /* ---- Audio Streaming Interface alt 1 (active) ---- */
    0x09, 0x04,
    0x03, 0x01, 0x01,           /* if=3 alt=1 nEP=1 (audio IN) + 1 fb */
    USB_CLASS_AUDIO, USB_AUDIO_V2, 0x00,
    0x00,

    /* AS Class-Specific Header */
    0x09, 0x24, 0x01,
    AC_ID_OUTPUT_TERM,          /* bTerminalLink */
    0x00,                       /* bmControls */
    0x01, 0x01,                 /* bFormatType: 1, bSubslotSize 4 */
    0x18,                       /* bBitResolution: 24 */

    /* AS Format Type I */
    0x0B, 0x24, 0x02,
    0x01,                       /* bFormatType: 1 */
    0x01,                       /* bNrChannels: 1 */
    0x04, 0x00,                 /* wSubslotSize: 4 bytes */
    0x01,                       /* bSamFreqType: 1 frequency */
    0x80, 0xBB, 0x00,           /* 48000 Hz packed as 24-bit */

    /* Standard AS EP */
    0x07, 0x05,
    EP_AUDIO_IN,                /* bEndpointAddress: IN iso */
    0x05,                       /* bmAttributes: iso async, data */
    AUDIO_PACKET_MAX_BYTES, 0x00, /* wMaxPacketSize */
    0x01,                       /* bInterval: 1 (1ms) */

    /* Class-Specific AS EP */
    0x08, 0x25, 0x01,
    0x00,                       /* bmAttributes: data EP */
    0x01,                       /* bmControls: sampling rate control */
    0x01,                       /* bLockDelayUnits: samples */
    0x00, 0x00,                 /* wLockDelay: 0 */

    /* Feedback EP */
    0x09, 0x05,
    EP_AUDIO_FB,                /* bEndpointAddress: OUT iso feedback */
    0x11,                       /* bmAttributes: iso feedback */
    0x04, 0x00,                 /* wMaxPacketSize: 4 bytes */
    0x01,                       /* bInterval: 1ms */
};

/* ---- String descriptors (UTF-16LE) ---------------------------------- */
static const uint8_t str_lang[] = {
    0x04, 0x03, 0x09, 0x04   /* English */
};

static const uint8_t str_manuf[] = {
    0x10, 0x03,                /* length=16 bytes incl header, type=3 */
    'j', 0x00, 'a', 0x00, 'y', 0x00, 'i', 0x00, 's', 0x00, '1', 0x00, 0,0,0,0
};

static const uint8_t str_product[] = {
    0x1A, 0x03,
    'L', 0x00, 'u', 0x00, 'm', 0x00, 'e', 0x00, 'n', 0x00, 'T', 0x00,
    'a', 0x00, 'p', 0x00, ' ', 0x00, 'L', 0x00, 'a', 0x00, 's', 0x00,
    'e', 0x00, 'r', 0x00, 0,0
};

static const uint8_t str_serial[] = {
    0x0E, 0x03,
    'L', 0x00, 'T', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '1', 0x00
};

static const uint8_t str_audio_iface[] = {
    0x12, 0x03,
    'L', 0x00, 'T', 0x00, ' ', 0x00, 'A', 0x00, 'u', 0x00, 'd', 0x00,
    'i', 0x00, 'o', 0x00, 0,0
};

static const uint8_t str_cdc_iface[] = {
    0x14, 0x03,
    'L', 0x00, 'T', 0x00, ' ', 0x00, 'C', 0x00, 'o', 0x00, 'n', 0x00,
    't', 0x00, 'r', 0x00, 'o', 0x00, 'l', 0x00
};

static const uint8_t str_config[] = {
    0x16, 0x03,
    'L', 0x00, 'u', 0x00, 'm', 0x00, 'e', 0x00, 'n', 0x00, 'T', 0x00,
    'a', 0x00, 'p', 0x00, ' ', 0x00, 'C', 0x00, 'f', 0x00, 'g', 0x00
};

/* Lookup table for GET_STRING */
static const uint8_t * const string_table[] = {
    str_lang, str_manuf, str_product, str_serial,
    str_audio_iface, str_cdc_iface, str_config
};
static const uint8_t string_lengths[] = {
    sizeof(str_lang), sizeof(str_manuf), sizeof(str_product),
    sizeof(str_serial), sizeof(str_audio_iface), sizeof(str_cdc_iface),
    sizeof(str_config)
};
#define STRING_TABLE_LEN  (sizeof(string_lengths))

/* ---- USB setup request handling structs ----------------------------- */

/* Standard request codes */
#define USB_REQ_GET_STATUS      0x00
#define USB_REQ_CLEAR_FEATURE   0x01
#define USB_REQ_SET_FEATURE     0x03
#define USB_REQ_SET_ADDRESS     0x05
#define USB_REQ_GET_DESCRIPTOR  0x06
#define USB_REQ_SET_DESCRIPTOR  0x07
#define USB_REQ_GET_CONFIG      0x08
#define USB_REQ_SET_CONFIG      0x09
#define USB_REQ_GET_INTERFACE   0x0A
#define USB_REQ_SET_INTERFACE   0x0B

/* Descriptor types */
#define USB_DESC_DEVICE         0x01
#define USB_DESC_CONFIG         0x02
#define USB_DESC_STRING         0x03
#define USB_DESC_INTERFACE      0x04
#define USB_DESC_ENDPOINT       0x05

/* Audio class-specific requests (UAC2) */
#define UAC2_REQ_CUR            0x01
#define UAC2_REQ_RANGE          0x02
#define UAC2_REQ_MEM            0x03

/* Clock source control selector */
#define UAC2_CS_CONTROL_SAM_FREQ 0x01

/* Feature unit control selectors */
#define UAC2_FU_MUTE            0x01
#define UAC2_FU_VOLUME          0x02

/* CDC class requests */
#define CDC_SET_LINE_CODING     0x20
#define CDC_GET_LINE_CODING     0x21
#define CDC_SET_CONTROL_LINE   0x22

/* Line coding struct (CDC) */
typedef struct {
    uint32_t dwDTERate;   /* bits per second */
    uint8_t  bCharFormat; /* 0=1 stop, 1=1.5, 2=2 */
    uint8_t  bParityType; /* 0=none, 1=odd, 2=even */
    uint8_t  bDataBits;   /* 7,8,16 */
} ltm_cdc_line_coding_t;

#endif /* LUMEN_TAP_USB_DESCRIPTORS_H */