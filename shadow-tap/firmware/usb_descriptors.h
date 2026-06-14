/*
 * usb_descriptors.h — ShadowTap USB descriptors (for recovery/boot mode)
 * The i.MX RT1062 has USB OTG used only for Serial Downloader recovery.
 * No runtime USB device functionality — this is for reference only.
 */

#ifndef SHADOWTAP_USB_DESCRIPTORS_H
#define SHADOWTAP_USB_DESCRIPTORS_H

#include <stdint.h>

/* VID/PID for recovery mode (NXP ROM default) */
#define USB_VID_NXP         0x15A2U
#define USB_PID_RT1062_ROM  0x0073U

/* Device descriptor (12 bytes for FS) */
static const uint8_t usb_device_descriptor[] = {
    0x12,                           /* bLength */
    0x01,                           /* bDescriptorType: Device */
    0x00, 0x02,                     /* bcdUSB: 2.00 */
    0x00,                           /* bDeviceClass: Interface defined */
    0x00,                           /* bDeviceSubClass */
    0x00,                           /* bDeviceProtocol */
    0x40,                           /* bMaxPacketSize0: 64 */
    0xA2, 0x15,                     /* idVendor: NXP */
    0x73, 0x00,                     /* idProduct: RT1062 ROM */
    0x00, 0x01,                     /* bcdDevice: 1.00 */
    0x01,                           /* iManufacturer: 1 */
    0x02,                           /* iProduct: 2 */
    0x03,                           /* iSerialNumber: 3 */
    0x01                            /* bNumConfigurations: 1 */
};

/* Configuration descriptor (9 bytes) */
static const uint8_t usb_config_descriptor[] = {
    0x09,                           /* bLength */
    0x02,                           /* bDescriptorType: Configuration */
    0x09, 0x00,                     /* wTotalLength: 9 */
    0x01,                           /* bNumInterfaces: 1 */
    0x01,                           /* bConfigurationValue: 1 */
    0x00,                           /* iConfiguration */
    0x80,                           /* bmAttributes: Bus powered */
    0x32                            /* bMaxPower: 100 mA */
};

/* String descriptor 0: Language IDs */
static const uint8_t usb_string0[] = {
    0x04, 0x03, 0x09, 0x04          /* English (US) */
};

/* String descriptor 1: Manufacturer */
static const uint8_t usb_string1[] = {
    0x1A, 0x03,
    'H', 0, 'a', 0, 'c', 0, 'k', 0, 'e', 0, 'r', 0,
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 's', 0
};

/* String descriptor 2: Product */
static const uint8_t usb_string2[] = {
    0x14, 0x03,
    'S', 0, 'h', 0, 'a', 0, 'd', 0, 'o', 0, 'w', 0,
    'T', 0, 'a', 0, 'p', 0
};

/* String descriptor 3: Serial (placeholder) */
static const uint8_t usb_string3[] = {
    0x14, 0x03,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
    '0', 0, '1', 0, '0', 0
};

#endif /* SHADOWTAP_USB_DESCRIPTORS_H */