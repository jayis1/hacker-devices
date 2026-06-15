/**
 * usb_descriptors.h — USB 3.0 Device Descriptors for eMMC Flash Dumper
 *
 * Complete USB device, configuration, BOS, and string descriptors
 * for a SuperSpeed bulk-only data transfer device.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/*===========================================================================
 * USB Standard Descriptor Types
 *===========================================================================*/

#define USB_DESC_TYPE_DEVICE                    1
#define USB_DESC_TYPE_CONFIGURATION             2
#define USB_DESC_TYPE_STRING                    3
#define USB_DESC_TYPE_INTERFACE                 4
#define USB_DESC_TYPE_ENDPOINT                  5
#define USB_DESC_TYPE_DEVICE_QUALIFIER          6
#define USB_DESC_TYPE_OTHER_SPEED_CONFIG        7
#define USB_DESC_TYPE_BOS                       15
#define USB_DESC_TYPE_DEVICE_CAPABILITY         16

/*===========================================================================
 * USB Device Descriptor
 *===========================================================================*/

#define USB_DEVICE_DESCRIPTOR_SIZE              18

static const uint8_t usb_device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_SIZE,         /* bLength */
    USB_DESC_TYPE_DEVICE,               /* bDescriptorType */
    0x00, 0x03,                         /* bcdUSB = 3.0 (SuperSpeed) */
    0x00,                               /* bDeviceClass (defined in interface) */
    0x00,                               /* bDeviceSubClass */
    0x00,                               /* bDeviceProtocol */
    64,                                 /* bMaxPacketSize0 (512 for SS, but 64 for FS/HS compat) */
    0x4E, 0x4F,                         /* idVendor = 0x4E4F ("NO" — Nous) */
    0x44, 0x46,                         /* idProduct = 0x4644 ("FD" — Flash Dumper) */
    0x00, 0x01,                         /* bcdDevice = 1.00 */
    1,                                  /* iManufacturer (string index 1) */
    2,                                  /* iProduct (string index 2) */
    3,                                  /* iSerialNumber (string index 3) */
    1                                   /* bNumConfigurations */
};

/*===========================================================================
 * USB Configuration Descriptor (SuperSpeed)
 *===========================================================================*/

#define USB_SS_CONFIG_DESC_SIZE                 9
#define USB_SS_INTERFACE_DESC_SIZE              9
#define USB_SS_ENDPOINT_DESC_SIZE               6
#define USB_SS_ENDPOINT_COMPANION_DESC_SIZE      6

#define USB_SS_CONFIG_TOTAL_SIZE \
    (USB_SS_CONFIG_DESC_SIZE + \
     USB_SS_INTERFACE_DESC_SIZE + \
     USB_SS_ENDPOINT_DESC_SIZE + USB_SS_ENDPOINT_COMPANION_DESC_SIZE + \
     USB_SS_ENDPOINT_DESC_SIZE + USB_SS_ENDPOINT_COMPANION_DESC_SIZE)

static const uint8_t usb_ss_config_descriptor[] = {
    /* Configuration Descriptor */
    USB_SS_CONFIG_DESC_SIZE,            /* bLength */
    USB_DESC_TYPE_CONFIGURATION,        /* bDescriptorType */
    USB_SS_CONFIG_TOTAL_SIZE, 0x00,     /* wTotalLength */
    1,                                  /* bNumInterfaces */
    1,                                  /* bConfigurationValue */
    0,                                  /* iConfiguration */
    0x80,                               /* bmAttributes: Bus-powered */
    50,                                 /* bMaxPower: 100 mA (50 × 2mA) */

    /* Interface Descriptor (Interface 0, Alt 0) */
    USB_SS_INTERFACE_DESC_SIZE,         /* bLength */
    USB_DESC_TYPE_INTERFACE,            /* bDescriptorType */
    0,                                  /* bInterfaceNumber */
    0,                                  /* bAlternateSetting */
    2,                                  /* bNumEndpoints (IN + OUT) */
    0xFF,                               /* bInterfaceClass: Vendor-specific */
    0x00,                               /* bInterfaceSubClass */
    0x00,                               /* bInterfaceProtocol */
    0,                                  /* iInterface */

    /* Endpoint Descriptor — Bulk OUT (EP1 OUT) */
    USB_SS_ENDPOINT_DESC_SIZE,          /* bLength */
    USB_DESC_TYPE_ENDPOINT,             /* bDescriptorType */
    0x01,                               /* bEndpointAddress: EP1 OUT */
    0x02,                               /* bmAttributes: Bulk */
    0x00, 0x04,                         /* wMaxPacketSize: 1024 bytes */
    0,                                  /* bInterval */

    /* SuperSpeed Endpoint Companion — Bulk OUT */
    USB_SS_ENDPOINT_COMPANION_DESC_SIZE,/* bLength */
    USB_DESC_TYPE_SS_ENDPOINT_COMPANION,/* bDescriptorType */
    16,                                 /* bMaxBurst: 16 packets per burst */
    0,                                  /* bmAttributes: bulk */
    0x00, 0x00,                         /* wBytesPerInterval: 0 (bulk) */

    /* Endpoint Descriptor — Bulk IN (EP1 IN) */
    USB_SS_ENDPOINT_DESC_SIZE,          /* bLength */
    USB_DESC_TYPE_ENDPOINT,             /* bDescriptorType */
    0x81,                               /* bEndpointAddress: EP1 IN */
    0x02,                               /* bmAttributes: Bulk */
    0x00, 0x04,                         /* wMaxPacketSize: 1024 bytes */
    0,                                  /* bInterval */

    /* SuperSpeed Endpoint Companion — Bulk IN */
    USB_SS_ENDPOINT_COMPANION_DESC_SIZE,/* bLength */
    USB_DESC_TYPE_SS_ENDPOINT_COMPANION,/* bDescriptorType */
    16,                                 /* bMaxBurst: 16 packets per burst */
    0,                                  /* bmAttributes: bulk */
    0x00, 0x00,                         /* wBytesPerInterval: 0 (bulk) */
};

/*===========================================================================
 * USB BOS Descriptor (Binary Object Store)
 *===========================================================================*/

#define USB_BOS_DESC_SIZE                       5
#define USB_SS_DEVICE_CAPABILITY_SIZE           10
#define USB_CONTAINER_ID_CAPABILITY_SIZE        20

#define USB_BOS_TOTAL_SIZE \
    (USB_BOS_DESC_SIZE + \
     USB_SS_DEVICE_CAPABILITY_SIZE + \
     USB_CONTAINER_ID_CAPABILITY_SIZE)

static const uint8_t usb_bos_descriptor[] = {
    /* BOS Descriptor */
    USB_BOS_DESC_SIZE,                  /* bLength */
    USB_DESC_TYPE_BOS,                  /* bDescriptorType */
    USB_BOS_TOTAL_SIZE, 0x00,           /* wTotalLength */
    2,                                  /* bNumDeviceCaps */

    /* SuperSpeed USB Device Capability */
    USB_SS_DEVICE_CAPABILITY_SIZE,      /* bLength */
    USB_DESC_TYPE_DEVICE_CAPABILITY,    /* bDescriptorType */
    0x03,                               /* bDevCapabilityType: SuperSpeed */
    0x00,                               /* bmAttributes: no LTM */
    0x0E, 0x00,                         /* wSpeedsSupported: SS+HS+FS */
    0x02,                               /* bFunctionalitySupport: HS */
    0x0A,                               /* bU1DevExitLat: 10 µs */
    0x00, 0x00,                         /* wU2DevExitLat: 0 */

    /* Container ID Capability */
    USB_CONTAINER_ID_CAPABILITY_SIZE,   /* bLength */
    USB_DESC_TYPE_DEVICE_CAPABILITY,    /* bDescriptorType */
    0x04,                               /* bDevCapabilityType: Container ID */
    0x00,                               /* bReserved */
    /* 16-byte UUID (randomly generated for this device) */
    0x4E, 0x4F, 0x55, 0x53, 0x46, 0x44, 0x45, 0x4D,
    0x4D, 0x43, 0x46, 0x4C, 0x41, 0x53, 0x48, 0x00,
};

/*===========================================================================
 * USB String Descriptors
 *===========================================================================*/

/* Language ID: English (US) */
static const uint8_t usb_string_langid[] = {
    4,                                  /* bLength */
    USB_DESC_TYPE_STRING,               /* bDescriptorType */
    0x09, 0x04,                         /* wLANGID: 0x0409 = English (US) */
};

/* Manufacturer String: "Nous Research" */
static const uint8_t usb_string_manufacturer[] = {
    28,                                 /* bLength */
    USB_DESC_TYPE_STRING,               /* bDescriptorType */
    'N', 0, 'o', 0, 'u', 0, 's', 0, ' ', 0,
    'R', 0, 'e', 0, 's', 0, 'e', 0, 'a', 0, 'r', 0, 'c', 0, 'h', 0,
};

/* Product String: "eMMC Flash Dumper" */
static const uint8_t usb_string_product[] = {
    38,                                 /* bLength */
    USB_DESC_TYPE_STRING,               /* bDescriptorType */
    'e', 0, 'M', 0, 'M', 0, 'C', 0, ' ', 0,
    'F', 0, 'l', 0, 'a', 0, 's', 0, 'h', 0, ' ', 0,
    'D', 0, 'u', 0, 'm', 0, 'p', 0, 'e', 0, 'r', 0,
};

/* Serial Number String: "EMFD-XXXXXXXX" (filled at runtime) */
#define USB_SERIAL_STRING_MAX_LEN       36
static uint8_t usb_string_serial[USB_SERIAL_STRING_MAX_LEN];

/*===========================================================================
 * USB String Descriptor Table
 *===========================================================================*/

typedef struct {
    const uint8_t *descriptor;
    uint8_t length;
} usb_string_entry_t;

static const usb_string_entry_t usb_strings[] = {
    { usb_string_langid,        sizeof(usb_string_langid) },
    { usb_string_manufacturer,  sizeof(usb_string_manufacturer) },
    { usb_string_product,       sizeof(usb_string_product) },
    { usb_string_serial,        0 },  /* Length set at runtime */
};

#define USB_STRING_COUNT        (sizeof(usb_strings) / sizeof(usb_strings[0]))

/*===========================================================================
 * USB Request Codes
 *===========================================================================*/

#define USB_REQ_GET_STATUS          0x00
#define USB_REQ_CLEAR_FEATURE       0x01
#define USB_REQ_SET_FEATURE         0x03
#define USB_REQ_SET_ADDRESS         0x05
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_SET_DESCRIPTOR      0x07
#define USB_REQ_GET_CONFIGURATION   0x08
#define USB_REQ_SET_CONFIGURATION   0x09
#define USB_REQ_GET_INTERFACE       0x0A
#define USB_REQ_SET_INTERFACE       0x0B
#define USB_REQ_SYNCH_FRAME         0x0C
#define USB_REQ_SET_SEL             0x30
#define USB_REQ_SET_ISOCH_DELAY     0x31

/*===========================================================================
 * USB Feature Selectors
 *===========================================================================*/

#define USB_FEAT_ENDPOINT_HALT      0x00
#define USB_FEAT_REMOTE_WAKEUP      0x01
#define USB_FEAT_TEST_MODE          0x02
#define USB_FEAT_U1_ENABLE          0x30
#define USB_FEAT_U2_ENABLE          0x31
#define USB_FEAT_LTM_ENABLE         0x32

/*===========================================================================
 * USB Standard Request Structure
 *===========================================================================*/

typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

/* Request type bits */
#define USB_REQTYPE_DIR_MASK        0x80
#define USB_REQTYPE_DIR_H2D         0x00
#define USB_REQTYPE_DIR_D2H         0x80
#define USB_REQTYPE_TYPE_MASK       0x60
#define USB_REQTYPE_TYPE_STANDARD   0x00
#define USB_REQTYPE_TYPE_CLASS      0x20
#define USB_REQTYPE_TYPE_VENDOR     0x40
#define USB_REQTYPE_RECIP_MASK      0x1F
#define USB_REQTYPE_RECIP_DEVICE    0x00
#define USB_REQTYPE_RECIP_INTERFACE 0x01
#define USB_REQTYPE_RECIP_ENDPOINT  0x02

/*===========================================================================
 * USB Endpoint Configuration
 *===========================================================================*/

#define USB_EP0_IN                  0x80
#define USB_EP0_OUT                 0x00
#define USB_EP1_IN                  0x81
#define USB_EP1_OUT                 0x01

#define USB_EP0_FIFO_SIZE           512
#define USB_EP1_FIFO_SIZE           4096    /* 4 KB for bulk */
#define USB_EP1_MAX_PACKET          1024    /* SuperSpeed bulk */
#define USB_EP1_BURST_SIZE          16

/*===========================================================================
 * USB State Machine
 *===========================================================================*/

typedef enum {
    USB_STATE_DISCONNECTED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESSED,
    USB_STATE_CONFIGURED,
    USB_STATE_SUSPENDED,
} usb_device_state_t;

/*===========================================================================
 * USB Transfer Control Block
 *===========================================================================*/

typedef struct {
    uint8_t  *buffer;
    uint32_t  length;
    uint32_t  transferred;
    uint32_t  remaining;
    bool      active;
    bool      zlp_needed;
} usb_transfer_t;

/*===========================================================================
 * Helper: Build serial number string from unique device ID
 *===========================================================================*/

static inline void usb_build_serial_string(void) {
    /* Use STM32 unique device ID (96-bit) to generate serial */
    uint32_t *uid = (uint32_t *)0x1FF1E800;  /* UID base address */
    uint32_t serial_num = uid[0] ^ uid[1] ^ uid[2];

    usb_string_serial[0] = 34;  /* bLength: 17 chars × 2 bytes + 2 */
    usb_string_serial[1] = USB_DESC_TYPE_STRING;

    /* "EMFD-" prefix */
    usb_string_serial[2] = 'E'; usb_string_serial[3] = 0;
    usb_string_serial[4] = 'M'; usb_string_serial[5] = 0;
    usb_string_serial[6] = 'F'; usb_string_serial[7] = 0;
    usb_string_serial[8] = 'D'; usb_string_serial[9] = 0;
    usb_string_serial[10] = '-'; usb_string_serial[11] = 0;

    /* 8 hex digits */
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        uint8_t nibble = (serial_num >> (28 - i * 4)) & 0x0F;
        usb_string_serial[12 + i * 2] = hex[nibble];
        usb_string_serial[13 + i * 2] = 0;
    }

    /* Update string table length */
    ((usb_string_entry_t *)&usb_strings[3])->length = 34;
}

#endif /* USB_DESCRIPTORS_H */
