/*
 * WFP-100 — USB Composite Gadget Descriptors
 * CDC-ECM (Ethernet) + CDC-ACM (Serial Console)
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef WFP100_USB_DESCRIPTORS_H
#define WFP100_USB_DESCRIPTORS_H

#include <stdint.h>

/* USB Vendor and Product IDs */
#define WFP100_USB_VENDOR_ID    0x1209   /* pid.codes — open source */
#define WFP100_USB_PRODUCT_ID   0xF100   /* WFP-100 WiFi Pentester */
#define WFP100_USB_VERSION_BCD  0x0100   /* v1.0.0 */

/* USB Device Descriptor */
static const struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __packed usb_device_descriptor = {
    .bLength            = 18,
    .bDescriptorType    = 1,      /* DEVICE */
    .bcdUSB             = 0x0200, /* USB 2.0 */
    .bDeviceClass       = 0xEF,   /* Misc */
    .bDeviceSubClass    = 0x02,   /* Composite */
    .bDeviceProtocol    = 0x01,   /* IAD */
    .bMaxPacketSize0    = 64,
    .idVendor           = WFP100_USB_VENDOR_ID,
    .idProduct          = WFP100_USB_PRODUCT_ID,
    .bcdDevice          = WFP100_USB_VERSION_BCD,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

/* Configuration descriptor (composite: CDC-ECM + CDC-ACM) */
#define USB_CONFIG_TOTAL_LENGTH  98  /* Total config descriptor length */

/* CDC-ECM interface numbers */
#define USB_IF_ECM_CTRL          0
#define USB_IF_ECM_DATA          1
#define USB_IF_ACM_CTRL          2
#define USB_IF_ACM_DATA          3

/* CDC-ECM MAC address (locally administered) */
static const uint8_t wfp100_ecm_mac[6] = {
    0x02, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E  /* Locally administered */
};

/* String descriptors */
static const uint8_t usb_string_lang[] = {
    0x04, 0x03, 0x09, 0x04  /* English (US) */
};

static const uint8_t usb_string_manufacturer[] = {
    26, 0x03,
    'H', 0, 'a', 0, 'c', 0, 'k', 0, 'e', 0, 'r', 0, ' ', 0,
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 's', 0
};

static const uint8_t usb_string_product[] = {
    18, 0x03,
    'W', 0, 'F', 0, 'P', 0, '-', 0, '1', 0, '0', 0, '0', 0,
    ' ', 0, 'P', 0, 'e', 0, 'n', 0, 't', 0, 'e', 0, 's', 0,
    't', 0, 'e', 0, 'r', 0
};

static const uint8_t usb_string_serial[] = {
    22, 0x03,
    'W', 0, 'F', 0, 'P', 0, '1', 0, '0', 0, '0', 0,
    '2', 0, '0', 0, '2', 0, '6', 0, '0', 0, '1', 0
};

/* Endpoint addresses */
#define USB_EP_ECM_NOTIFY       0x81   /* EP1 IN, interrupt */
#define USB_EP_ECM_IN           0x82   /* EP2 IN, bulk */
#define USB_EP_ECM_OUT          0x02   /* EP2 OUT, bulk */
#define USB_EP_ACM_NOTIFY       0x83   /* EP3 IN, interrupt */
#define USB_EP_ACM_IN           0x84   /* EP4 IN, bulk */
#define USB_EP_ACM_OUT          0x04   /* EP4 OUT, bulk */

#endif /* WFP100_USB_DESCRIPTORS_H */