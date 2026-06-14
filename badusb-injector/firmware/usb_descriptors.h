/*
 * PHANTOM — USB HID Descriptors
 * RP2040 USB HID Emulation Payload Injector
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 *
 * USB descriptors for composite HID device (Keyboard + Mouse + Consumer Control)
 * and Mass Storage Class (stealth mode).
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/*
 * USB Vendor/Product IDs
 * Using custom VID for Hacker Devices
 */
#define USB_VID                     0x1A86
#define USB_PID_HID                 0x5001  /* HID composite mode */
#define USB_PID_MSC                 0x5002  /* Mass storage mode */
#define USB_VERSION                 0x0100  /* v1.0.0 */

/*
 * USB descriptor types
 */
#define USB_DT_DEVICE               0x01
#define USB_DT_CONFIG               0x02
#define USB_DT_STRING               0x03
#define USB_DT_INTERFACE            0x04
#define USB_DT_ENDPOINT             0x05
#define USB_DT_HID                  0x21
#define USB_DT_REPORT               0x22
#define USB_DT_PHYSICAL             0x23

/*
 * USB class codes
 */
#define USB_CLASS_PER_INTERFACE      0x00
#define USB_CLASS_HID                0x03
#define USB_CLASS_MSC                0x08
#define USB_CLASS_MISC               0xEF
#define USB_CLASS_VENDOR             0xFF

/*
 * USB subclass/protocol
 */
#define USB_SUBCLASS_HID_BOOT       0x01
#define USB_PROTOCOL_HID_KEYBOARD   0x01
#define USB_PROTOCOL_HID_MOUSE      0x02
#define USB_SUBCLASS_MSC_SCSI       0x06
#define USB_PROTOCOL_MSC_BOT        0x50
#define USB_SUBCLASS_MISC_IAD       0x02
#define USB_PROTOCOL_MISC_COMMON     0x01

/*
 * Endpoint directions
 */
#define USB_DIR_OUT                  0x00
#define USB_DIR_IN                   0x80

/*
 * Endpoint types
 */
#define USB_ENDPOINT_TYPE_CONTROL    0x00
#define USB_ENDPOINT_TYPE_ISOCH      0x01
#define USB_ENDPOINT_TYPE_BULK       0x02
#define USB_ENDPOINT_TYPE_INTERRUPT  0x03

/*
 * Endpoint numbers
 */
#define EP0                          0   /* Control */
#define EP1_IN                       1   /* Keyboard */
#define EP2_IN                       2   /* Mouse */
#define EP3_IN                       3   /* Consumer Control */
#define EP4_OUT                      4   /* Keyboard LED state */
#define EP5_IN                       5   /* MSC bulk-in */
#define EP5_OUT                      5   /* MSC bulk-out */

/*
 * Configuration descriptor values
 */
#define CONFIG_ATTR_BUS_POWERED      (1 << 7)
#define CONFIG_ATTR_SELF_POWERED     (1 << 6)
#define CONFIG_ATTR_REMOTE_WAKEUP    (1 << 5)

/* ============================================================
 * HID Configuration (Attack Mode)
 * Composite: Keyboard + Mouse + Consumer Control
 * ============================================================ */

/*
 * Configuration descriptor for HID composite device
 * Total size: 9 + 9+9+9+7 + 9+9+7 + 9+9+7 = 93 bytes
 */
static const uint8_t hid_config_descriptor[] = {
    /* Configuration descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_CONFIG,  /* bDescriptorType */
    0x5D, 0x00, /* wTotalLength: 93 bytes */
    0x03,       /* bNumInterfaces: 3 */
    0x01,       /* bConfigurationValue */
    0x00,       /* iConfiguration */
    CONFIG_ATTR_BUS_POWERED,  /* bmAttributes: bus-powered */
    100 / 2,   /* bMaxPower: 100 mA */

    /* Interface Association Descriptor (IAD) for composite */
    0x08,       /* bLength */
    0x0B,       /* bDescriptorType: IAD */
    0x00,       /* bFirstInterface: 0 */
    0x03,       /* bInterfaceCount: 3 */
    USB_CLASS_HID,  /* bFunctionClass: HID */
    0x00,       /* bFunctionSubClass */
    0x00,       /* bFunctionProtocol */

    /* ---- Interface 0: HID Keyboard ---- */
    /* Interface descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_INTERFACE,  /* bDescriptorType */
    0x00,       /* bInterfaceNumber: 0 */
    0x00,       /* bAlternateSetting */
    0x02,       /* bNumEndpoints: 2 (EP1 IN, EP4 OUT) */
    USB_CLASS_HID,  /* bInterfaceClass: HID */
    USB_SUBCLASS_HID_BOOT,  /* bInterfaceSubClass: Boot */
    USB_PROTOCOL_HID_KEYBOARD,  /* bInterfaceProtocol: Keyboard */
    0x00,       /* iInterface */

    /* HID descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_HID, /* bDescriptorType: HID */
    0x11, 0x01, /* bcdHID: 1.11 */
    0x00,       /* bCountryCode: not localized */
    0x01,       /* bNumDescriptors: 1 */
    USB_DT_REPORT,  /* bDescriptorType: Report */
    sizeof(keyboard_report_descriptor), 0x00,  /* wDescriptorLength */

    /* Endpoint 1 IN: Keyboard report (7 bytes) */
    0x07,       /* bLength */
    USB_DT_ENDPOINT,  /* bDescriptorType */
    EP1_IN | USB_DIR_IN,  /* bEndpointAddress */
    USB_ENDPOINT_TYPE_INTERRUPT,  /* bmAttributes: Interrupt */
    0x08, 0x00, /* wMaxPacketSize: 8 bytes */
    0x01,       /* bInterval: 1 ms */

    /* Endpoint 4 OUT: Keyboard LED state (1 byte) */
    0x07,       /* bLength */
    USB_DT_ENDPOINT,  /* bDescriptorType */
    EP4_OUT | USB_DIR_OUT,  /* bEndpointAddress */
    USB_ENDPOINT_TYPE_INTERRUPT,  /* bmAttributes: Interrupt */
    0x01, 0x00, /* wMaxPacketSize: 1 byte */
    0x01,       /* bInterval: 1 ms */

    /* ---- Interface 1: HID Mouse ---- */
    /* Interface descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_INTERFACE,  /* bDescriptorType */
    0x01,       /* bInterfaceNumber: 1 */
    0x00,       /* bAlternateSetting */
    0x01,       /* bNumEndpoints: 1 (EP2 IN) */
    USB_CLASS_HID,  /* bInterfaceClass: HID */
    USB_SUBCLASS_HID_BOOT,  /* bInterfaceSubClass: Boot */
    USB_PROTOCOL_HID_MOUSE,  /* bInterfaceProtocol: Mouse */
    0x00,       /* iInterface */

    /* HID descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_HID, /* bDescriptorType: HID */
    0x11, 0x01, /* bcdHID: 1.11 */
    0x00,       /* bCountryCode */
    0x01,       /* bNumDescriptors: 1 */
    USB_DT_REPORT,  /* bDescriptorType */
    sizeof(mouse_report_descriptor), 0x00,  /* wDescriptorLength */

    /* Endpoint 2 IN: Mouse report (4 bytes) */
    0x07,       /* bLength */
    USB_DT_ENDPOINT,  /* bDescriptorType */
    EP2_IN | USB_DIR_IN,  /* bEndpointAddress */
    USB_ENDPOINT_TYPE_INTERRUPT,  /* bmAttributes: Interrupt */
    0x04, 0x00, /* wMaxPacketSize: 4 bytes */
    0x01,       /* bInterval: 1 ms */

    /* ---- Interface 2: HID Consumer Control ---- */
    /* Interface descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_INTERFACE,  /* bDescriptorType */
    0x02,       /* bInterfaceNumber: 2 */
    0x00,       /* bAlternateSetting */
    0x01,       /* bNumEndpoints: 1 (EP3 IN) */
    USB_CLASS_HID,  /* bInterfaceClass: HID */
    0x00,       /* bInterfaceSubClass: No boot */
    0x00,       /* bInterfaceProtocol: No protocol */
    0x00,       /* iInterface */

    /* HID descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_HID, /* bDescriptorType: HID */
    0x11, 0x01, /* bcdHID: 1.11 */
    0x00,       /* bCountryCode */
    0x01,       /* bNumDescriptors: 1 */
    USB_DT_REPORT,  /* bDescriptorType */
    sizeof(consumer_report_descriptor), 0x00,  /* wDescriptorLength */

    /* Endpoint 3 IN: Consumer control report (2 bytes) */
    0x07,       /* bLength */
    USB_DT_ENDPOINT,  /* bDescriptorType */
    EP3_IN | USB_DIR_IN,  /* bEndpointAddress */
    USB_ENDPOINT_TYPE_INTERRUPT,  /* bmAttributes: Interrupt */
    0x02, 0x00, /* wMaxPacketSize: 2 bytes */
    0x01,       /* bInterval: 1 ms */
};

/*
 * HID Keyboard Report Descriptor
 * Boot-compatible, 8-byte report: modifier + reserved + 6 keycodes
 */
static const uint8_t keyboard_report_descriptor[] = {
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x06,        /* Usage (Keyboard) */
    0xA1, 0x01,        /* Collection (Application) */

    /* Modifier byte: Ctrl, Shift, Alt, GUI */
    0x05, 0x07,        /* Usage Page (Key Codes) */
    0x19, 0xE0,        /* Usage Minimum (224) */
    0x29, 0xE7,        /* Usage Maximum (231) */
    0x15, 0x00,        /* Logical Minimum (0) */
    0x25, 0x01,        /* Logical Maximum (1) */
    0x75, 0x01,        /* Report Size (1) */
    0x95, 0x08,        /* Report Count (8) */
    0x81, 0x02,        /* Input (Data, Variable, Absolute) */

    /* Reserved byte */
    0x95, 0x01,        /* Report Count (1) */
    0x75, 0x08,        /* Report Size (8) */
    0x81, 0x01,        /* Input (Constant) */

    /* LED output report: NumLock, CapsLock, ScrollLock, etc. */
    0x05, 0x08,        /* Usage Page (LEDs) */
    0x19, 0x01,        /* Usage Minimum (1) */
    0x29, 0x05,        /* Usage Maximum (5) */
    0x15, 0x00,        /* Logical Minimum (0) */
    0x25, 0x01,        /* Logical Maximum (1) */
    0x75, 0x01,        /* Report Size (1) */
    0x95, 0x05,        /* Report Count (5) */
    0x91, 0x02,        /* Output (Data, Variable, Absolute) */

    /* LED padding */
    0x95, 0x01,        /* Report Count (1) */
    0x75, 0x03,        /* Report Size (3) */
    0x91, 0x01,        /* Output (Constant) */

    /* Keycode bytes: 6 keys maximum */
    0x05, 0x07,        /* Usage Page (Key Codes) */
    0x19, 0x00,        /* Usage Minimum (0) */
    0x29, 0xDD,        /* Usage Maximum (221) */
    0x15, 0x00,        /* Logical Minimum (0) */
    0x25, 0xDD,        /* Logical Maximum (221) */
    0x75, 0x08,        /* Report Size (8) */
    0x95, 0x06,        /* Report Count (6) */
    0x81, 0x00,        /* Input (Data, Array) */

    0xC0                /* End Collection */
};

/*
 * HID Mouse Report Descriptor
 * Boot-compatible, 4-byte report: buttons + X + Y + wheel
 */
static const uint8_t mouse_report_descriptor[] = {
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x02,        /* Usage (Mouse) */
    0xA1, 0x01,        /* Collection (Application) */
    0x09, 0x01,        /* Usage (Pointer) */
    0xA1, 0x00,        /* Collection (Physical) */

    /* Buttons: 3 buttons (left, right, middle) */
    0x05, 0x09,        /* Usage Page (Button) */
    0x19, 0x01,        /* Usage Minimum (1) */
    0x29, 0x03,        /* Usage Maximum (3) */
    0x15, 0x00,        /* Logical Minimum (0) */
    0x25, 0x01,        /* Logical Maximum (1) */
    0x75, 0x01,        /* Report Size (1) */
    0x95, 0x03,        /* Report Count (3) */
    0x81, 0x02,        /* Input (Data, Variable, Absolute) */

    /* Button padding */
    0x75, 0x05,        /* Report Size (5) */
    0x95, 0x01,        /* Report Count (1) */
    0x81, 0x01,        /* Input (Constant) */

    /* X and Y displacement: -127 to +127 */
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x30,        /* Usage (X) */
    0x09, 0x31,        /* Usage (Y) */
    0x15, 0x81,        /* Logical Minimum (-127) */
    0x25, 0x7F,        /* Logical Maximum (127) */
    0x75, 0x08,        /* Report Size (8) */
    0x95, 0x02,        /* Report Count (2) */
    0x81, 0x06,        /* Input (Data, Variable, Relative) */

    /* Wheel displacement */
    0x09, 0x38,        /* Usage (Wheel) */
    0x15, 0x81,        /* Logical Minimum (-127) */
    0x25, 0x7F,        /* Logical Maximum (127) */
    0x75, 0x08,        /* Report Size (8) */
    0x95, 0x01,        /* Report Count (1) */
    0x81, 0x06,        /* Input (Data, Variable, Relative) */

    0xC0,               /* End Collection (Physical) */
    0xC0                /* End Collection (Application) */
};

/*
 * HID Consumer Control Report Descriptor
 * Media keys: play/pause, volume, etc.
 */
static const uint8_t consumer_report_descriptor[] = {
    0x05, 0x0C,        /* Usage Page (Consumer) */
    0x09, 0x01,        /* Usage (Consumer Control) */
    0xA1, 0x01,        /* Collection (Application) */

    /* Consumer keys */
    0x15, 0x00,        /* Logical Minimum (0) */
    0x25, 0x01,        /* Logical Maximum (1) */
    0x75, 0x01,        /* Report Size (1) */
    0x95, 0x10,        /* Report Count (16) */
    0x0A, 0x00, 0x00,  /* Usage (0) — will be overridden */
    0x0A, 0xCD, 0x00,  /* Usage (Play/Pause) */
    0x0A, 0xB5, 0x00,  /* Usage (Scan Next) */
    0x0A, 0xB6, 0x00,  /* Usage (Scan Previous) */
    0x0A, 0xB7, 0x00,  /* Usage (Stop) */
    0x0A, 0xE9, 0x00,  /* Usage (Volume Up) */
    0x0A, 0xEA, 0x00,  /* Usage (Volume Down) */
    0x0A, 0xE2, 0x00,  /* Usage (Mute) */
    0x81, 0x02,        /* Input (Data, Variable, Absolute) */

    0xC0                /* End Collection */
};

/* Simplified consumer report descriptor (bit-field style) */
static const uint8_t consumer_report_desc_simple[] = {
    0x05, 0x0C,        /* Usage Page (Consumer) */
    0x09, 0x01,        /* Usage (Consumer Control) */
    0xA1, 0x01,        /* Collection (Application) */
    0x15, 0x00,        /* Logical Minimum (0) */
    0x26, 0xFF, 0x03,  /* Logical Maximum (1023) */
    0x19, 0x00,        /* Usage Minimum (0) */
    0x2A, 0xFF, 0x03,  /* Usage Maximum (1023) */
    0x75, 0x10,        /* Report Size (16) */
    0x95, 0x01,        /* Report Count (1) */
    0x81, 0x00,        /* Input (Data, Array) */
    0xC0                /* End Collection */
};

/* Override consumer report descriptor with simple version */
#define consumer_report_descriptor consumer_report_desc_simple

/*
 * USB Device Descriptor (HID mode)
 */
static const uint8_t hid_device_descriptor[] = {
    0x12,       /* bLength: 18 */
    USB_DT_DEVICE,  /* bDescriptorType */
    0x00, 0x02, /* bcdUSB: 2.0 */
    USB_CLASS_MISC,  /* bDeviceClass: Miscellaneous */
    USB_SUBCLASS_MISC_IAD,  /* bDeviceSubClass: IAD */
    USB_PROTOCOL_MISC_COMMON,  /* bDeviceProtocol: Common */
    0x40,       /* bMaxPacketSize0: 64 */
    USB_VID & 0xFF, USB_VID >> 8,  /* idVendor */
    USB_PID_HID & 0xFF, USB_PID_HID >> 8,  /* idProduct */
    USB_VERSION & 0xFF, USB_VERSION >> 8,  /* bcdDevice */
    0x01,       /* iManufacturer */
    0x02,       /* iProduct */
    0x03,       /* iSerialNumber */
    0x01,       /* bNumConfigurations */
};

/*
 * USB String Descriptors
 */
static const uint8_t string_descriptor_0[] = {
    0x04,       /* bLength */
    USB_DT_STRING,  /* bDescriptorType */
    0x09, 0x04, /* wLANGID: English (US) */
};

/* Manufacturer: "Hacker Devices" */
static const uint8_t string_descriptor_1[] = {
    26,         /* bLength */
    USB_DT_STRING,
    'H', 0, 'a', 0, 'c', 0, 'k', 0, 'e', 0, 'r', 0,
    ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 's', 0
};

/* Product: "PHANTOM HID Injector" */
static const uint8_t string_descriptor_2[] = {
    54,         /* bLength */
    USB_DT_STRING,
    'P', 0, 'H', 0, 'A', 0, 'N', 0, 'T', 0, 'O', 0, 'M', 0,
    ' ', 0, 'H', 0, 'I', 0, 'D', 0, ' ', 0, 'I', 0, 'n', 0, 'j', 0,
    'e', 0, 'c', 0, 't', 0, 'o', 0, 'r', 0
};

/* Serial number (populated at runtime from RP2040 UID) */
static uint8_t string_descriptor_3[26];  /* "RP2040-XXXXXXXX" */

/* String descriptor array */
static const uint8_t *string_descriptors[] = {
    string_descriptor_0,
    string_descriptor_1,
    string_descriptor_2,
    string_descriptor_3,
};

/*
 * MSC Configuration (Stealth Mode)
 * Simple mass storage device for file transfer
 */
static const uint8_t msc_config_descriptor[] = {
    /* Configuration descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_CONFIG,  /* bDescriptorType */
    0x20, 0x00, /* wTotalLength: 32 bytes */
    0x01,       /* bNumInterfaces: 1 */
    0x01,       /* bConfigurationValue */
    0x00,       /* iConfiguration */
    CONFIG_ATTR_BUS_POWERED,  /* bmAttributes: bus-powered */
    100 / 2,   /* bMaxPower: 100 mA */

    /* Interface descriptor (9 bytes) */
    0x09,       /* bLength */
    USB_DT_INTERFACE,  /* bDescriptorType */
    0x00,       /* bInterfaceNumber: 0 */
    0x00,       /* bAlternateSetting */
    0x02,       /* bNumEndpoints: 2 (bulk in/out) */
    USB_CLASS_MSC,  /* bInterfaceClass: Mass Storage */
    USB_SUBCLASS_MSC_SCSI,  /* bInterfaceSubClass: SCSI */
    USB_PROTOCOL_MSC_BOT,  /* bInterfaceProtocol: Bulk-Only Transport */
    0x00,       /* iInterface */

    /* Endpoint 5 IN: Bulk data (64 bytes) */
    0x07,       /* bLength */
    USB_DT_ENDPOINT,  /* bDescriptorType */
    EP5_IN | USB_DIR_IN,  /* bEndpointAddress */
    USB_ENDPOINT_TYPE_BULK,  /* bmAttributes: Bulk */
    0x40, 0x00, /* wMaxPacketSize: 64 bytes */
    0x00,       /* bInterval: N/A for bulk */

    /* Endpoint 5 OUT: Bulk data (64 bytes) */
    0x07,       /* bLength */
    USB_DT_ENDPOINT,  /* bDescriptorType */
    EP5_OUT | USB_DIR_OUT,  /* bEndpointAddress */
    USB_ENDPOINT_TYPE_BULK,  /* bmAttributes: Bulk */
    0x40, 0x00, /* wMaxPacketSize: 64 bytes */
    0x00,       /* bInterval: N/A for bulk */
};

/*
 * MSC Device Descriptor (Stealth Mode)
 */
static const uint8_t msc_device_descriptor[] = {
    0x12,       /* bLength: 18 */
    USB_DT_DEVICE,  /* bDescriptorType */
    0x00, 0x02, /* bcdUSB: 2.0 */
    USB_CLASS_MSC,  /* bDeviceClass: Mass Storage */
    USB_SUBCLASS_MSC_SCSI,  /* bDeviceSubClass */
    USB_PROTOCOL_MSC_BOT,  /* bDeviceProtocol */
    0x40,       /* bMaxPacketSize0: 64 */
    USB_VID & 0xFF, USB_VID >> 8,  /* idVendor */
    USB_PID_MSC & 0xFF, USB_PID_MSC >> 8,  /* idProduct */
    USB_VERSION & 0xFF, USB_VERSION >> 8,  /* bcdDevice */
    0x01,       /* iManufacturer */
    0x04,       /* iProduct (different string) */
    0x03,       /* iSerialNumber */
    0x01,       /* bNumConfigurations */
};

/* Stealth mode product string: "PHANTOM Flash Drive" */
static const uint8_t string_descriptor_4[] = {
    46,         /* bLength */
    USB_DT_STRING,
    'P', 0, 'H', 0, 'A', 0, 'N', 0, 'T', 0, 'O', 0, 'M', 0,
    ' ', 0, 'F', 0, 'l', 0, 'a', 0, 's', 0, 'h', 0, ' ', 0,
    'D', 0, 'r', 0, 'i', 0, 'v', 0, 'e', 0
};

#endif /* USB_DESCRIPTORS_H */