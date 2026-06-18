/*
 * usb_descriptors.h — USB descriptors for GNSS-Phantom recovery/maintenance mode
 *
 * In DFU/recovery mode, the device presents a USB CDC virtual serial port
 * for firmware updates and direct command access.  This file defines the
 * standard USB descriptors used when the device boots in USB mode (selected
 * by holding the mode button at power-on).
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#ifndef GNSS_PHANTOM_USB_DESCRIPTORS_H
#define GNSS_PHANTOM_USB_DESCRIPTORS_H

#include <stdint.h>

#define USB_VID                 0x1A86  /* jayis1 custom VID */
#define USB_PID                  0xGP01  /* GPX-1 device */
#define USB_PID_VAL             0x6801  /* numeric PID */

/* Device descriptor (18 bytes) */
static const uint8_t device_descriptor[] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType (Device) */
    0x00, 0x02, /* bcdUSB 2.00 */
    0xEF,       /* bDeviceClass (Misc) */
    0x02,       /* bDeviceSubClass (Common Class) */
    0x01,       /* bDeviceProtocol (Interface Association Descriptor) */
    0x40,       /* bMaxPacketSize0 (64) */
    0x86, 0x1A, /* idVendor = 0x1A86 */
    0x01, 0x68, /* idProduct = 0x6801 */
    0x00, 0x01, /* bcdDevice 1.00 */
    0x01,       /* iManufacturer (string index 1) */
    0x02,       /* iProduct (string index 2) */
    0x03,       /* iSerialNumber (string index 3) */
    0x01,       /* bNumConfigurations (1) */
};

/* Configuration descriptor (9 + 8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 = 67 bytes) */
static const uint8_t config_descriptor[] = {
    /* Configuration descriptor */
    0x09,       /* bLength */
    0x02,       /* bDescriptorType (Configuration) */
    0x43, 0x00, /* wTotalLength 67 */
    0x02,       /* bNumInterfaces (2: CDC + IAD) */
    0x01,       /* bConfigurationValue (1) */
    0x00,       /* iConfiguration (no string) */
    0x80,       /* bmAttributes (bus powered) */
    0x32,       /* bMaxPower 100 mA */

    /* Interface Association Descriptor */
    0x08,       /* bLength */
    0x0B,       /* bDescriptorType (IAD) */
    0x00,       /* bFirstInterface (0) */
    0x02,       /* bInterfaceCount (2) */
    0x02,       /* bFunctionClass (CDC) */
    0x02,       /* bFunctionSubClass (ACM) */
    0x01,       /* bFunctionProtocol (AT commands) */
    0x00,       /* iFunction */

    /* CDC Communication Interface */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType (Interface) */
    0x00,       /* bInterfaceNumber (0) */
    0x00,       /* bAlternateSetting (0) */
    0x01,       /* bNumEndpoints (1) */
    0x02,       /* bInterfaceClass (CDC) */
    0x02,       /* bInterfaceSubClass (ACM) */
    0x01,       /* bInterfaceProtocol (AT) */
    0x00,       /* iInterface */

    /* CDC Header Functional Descriptor */
    0x05, 0x24, 0x00, 0x10, 0x01,

    /* CDC Call Management Functional Descriptor */
    0x05, 0x24, 0x01, 0x00, 0x01,

    /* CDC ACM Functional Descriptor */
    0x04, 0x24, 0x02, 0x02,

    /* CDC Union Functional Descriptor */
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* CDC Interrupt Endpoint (IN) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType (Endpoint) */
    0x81,       /* bEndpointAddress (IN 1) */
    0x03,       /* bmAttributes (Interrupt) */
    0x08, 0x00, /* wMaxPacketSize 8 */
    0xFF,       /* bInterval 255 ms */

    /* CDC Data Interface */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType (Interface) */
    0x01,       /* bInterfaceNumber (1) */
    0x00,       /* bAlternateSetting (0) */
    0x02,       /* bNumEndpoints (2) */
    0x0A,       /* bInterfaceClass (Data) */
    0x00,       /* bInterfaceSubClass */
    0x00,       /* bInterfaceProtocol */
    0x00,       /* iInterface */

    /* Bulk IN Endpoint (IN 2) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType (Endpoint) */
    0x82,       /* bEndpointAddress (IN 2) */
    0x02,       /* bmAttributes (Bulk) */
    0x40, 0x00, /* wMaxPacketSize 64 */
    0x00,       /* bInterval */

    /* Bulk OUT Endpoint (OUT 2) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType (Endpoint) */
    0x02,       /* bEndpointAddress (OUT 2) */
    0x02,       /* bmAttributes (Bulk) */
    0x40, 0x00, /* wMaxPacketSize 64 */
    0x00,       /* bInterval */
};

/* String descriptors (UTF-16LE, index 0 = language) */
static const uint8_t string0[] = {
    0x04, 0x03, 0x09, 0x04  /* LANGID: English (US) */
};

/* Manufacturer: "jayis1" */
static const uint8_t string1[] = {
    0x10, 0x03,
    'j', 0x00, 'a', 0x00, 'y', 0x00, 'i', 0x00, 's', 0x00, '1', 0x00,
};

/* Product: "GNSS-Phantom GPX-1" */
static const uint8_t string2[] = {
    0x28, 0x03,
    'G', 0x00, 'N', 0x00, 'S', 0x00, 'S', 0x00, '-', 0x00, 'P', 0x00,
    'h', 0x00, 'a', 0x00, 'n', 0x00, 't', 0x00, 'o', 0x00, 'm', 0x00,
    ' ', 0x00, 'G', 0x00, 'P', 0x00, 'X', 0x00, '-', 0x00, '1', 0x00,
};

/* Serial: "GPX-00001" (placeholder, can be read from OTP) */
static const uint8_t string3[] = {
    0x16, 0x03,
    'G', 0x00, 'P', 0x00, 'X', 0x00, '-', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '1', 0x00,
};

static const uint8_t *string_descriptors[] = {
    string0, string1, string2, string3
};

#define USB_NUM_STRINGS 4

#endif /* GNSS_PHANTOM_USB_DESCRIPTORS_H */