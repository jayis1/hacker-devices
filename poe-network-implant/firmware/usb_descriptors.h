/*
 * usb_descriptors.h — USB Descriptors for DFU Recovery Mode
 * PhantomBridge PoE Network Implant
 *
 * When BOOT0 is HIGH, the STM32H743 enters the built-in System Memory
 * bootloader which presents a DFU (USB) or UART interface for firmware
 * recovery. These descriptors are for reference / custom bootloader builds.
 *
 * Note: The STM32 built-in bootloader has its own fixed descriptors.
 * These are provided for a custom USB DFU bootloader if desired.
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* VID/PID for custom DFU bootloader */
#define USB_VID                 0x0483  /* STMicroelectronics */
#define USB_PID                 0xDF11  /* DFU Mode */
#define USB_LANGID              0x0409  /* English (US) */
#define USB_VERSION             0x0200  /* USB 2.0 */

/* Device descriptor (18 bytes) */
static const uint8_t usb_device_desc[] = {
    0x12,       /* bLength: 18 bytes */
    0x01,       /* bDescriptorType: DEVICE */
    0x00, 0x02, /* bcdUSB: 2.00 */
    0x00,       /* bDeviceClass: Defined at interface level */
    0x00,       /* bDeviceSubClass */
    0x00,       /* bDeviceProtocol */
    0x40,       /* bMaxPacketSize0: 64 bytes */
    0x83, 0x04, /* idVendor: 0x0483 (STMicroelectronics) */
    0x11, 0xDF, /* idProduct: 0xDF11 (DFU Mode) */
    0x00, 0x01, /* bcdDevice: 1.00 */
    0x01,       /* iManufacturer: Index 1 */
    0x02,       /* iProduct: Index 2 */
    0x03,       /* iSerialNumber: Index 3 */
    0x01        /* bNumConfigurations: 1 */
};

/* Configuration descriptor (9 + 9 + 9 = 27 bytes) */
static const uint8_t usb_config_desc[] = {
    /* Configuration descriptor */
    0x09,       /* bLength: 9 bytes */
    0x02,       /* bDescriptorType: CONFIGURATION */
    0x1B, 0x00, /* wTotalLength: 27 bytes */
    0x01,       /* bNumInterfaces: 1 */
    0x01,       /* bConfigurationValue: 1 */
    0x00,       /* iConfiguration: none */
    0x80,       /* bmAttributes: Bus powered */
    0x32,       /* bMaxPower: 100mA (self-powered from PoE) */

    /* Interface descriptor */
    0x09,       /* bLength: 9 bytes */
    0x04,       /* bDescriptorType: INTERFACE */
    0x00,       /* bInterfaceNumber: 0 */
    0x00,       /* bAlternateSetting: 0 */
    0x00,       /* bNumEndpoints: 0 (DFU uses EP0 only) */
    0xFE,       /* bInterfaceClass: Application Specific */
    0x01,       /* bInterfaceSubClass: Device Firmware Upgrade */
    0x02,       /* bInterfaceProtocol: DFU Mode (not Runtime) */
    0x00,       /* iInterface: none */

    /* DFU Functional descriptor */
    0x09,       /* bLength: 9 bytes */
    0x21,       /* bDescriptorType: DFU FUNCTIONAL */
    0x0B,       /* bmAttributes: Will Detach, Manifestation Tolerant, Download, Upload */
    0xFF, 0x00, /* wDetachTimeOut: 255ms */
    0x00, 0x02, /* wTransferSize: 512 bytes */
    0x1A, 0x01  /* bcdDFUVersion: 1.1a */
};

/* String descriptor 0: Language ID */
static const uint8_t usb_string0[] = {
    0x04,       /* bLength: 4 bytes */
    0x03,       /* bDescriptorType: STRING */
    0x09, 0x04  /* wLANGID: English (US) = 0x0409 */
};

/* String descriptor 1: Manufacturer */
static const uint8_t usb_string1[] = {
    0x1E,       /* bLength: 30 bytes */
    0x03,       /* bDescriptorType: STRING */
    'P', 0x00, 'h', 0x00, 'a', 0x00, 'n', 0x00,
    't', 0x00, 'o', 0x00, 'm', 0x00, 'B', 0x00,
    'r', 0x00, 'i', 0x00, 'd', 0x00, 'g', 0x00,
    'e', 0x00, 0x00, 0x00
};

/* String descriptor 2: Product */
static const uint8_t usb_string2[] = {
    0x2C,       /* bLength: 44 bytes */
    0x03,       /* bDescriptorType: STRING */
    'P', 0x00, 'o', 0x00, 'E', 0x00, ' ', 0x00,
    'N', 0x00, 'e', 0x00, 't', 0x00, 'w', 0x00,
    'o', 0x00, 'r', 0x00, 'k', 0x00, ' ', 0x00,
    'I', 0x00, 'm', 0x00, 'p', 0x00, 'l', 0x00,
    'a', 0x00, 'n', 0x00, 't', 0x00, 0x00, 0x00
};

/* String descriptor 3: Serial (device-unique) */
/* Placeholder — real serial read from STM32 UID registers at 0x1FF1E800 */
static const uint8_t usb_string3[] = {
    0x1A,       /* bLength: 26 bytes */
    0x03,       /* bDescriptorType: STRING */
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, 0x00, 0x00
};

/* DFU Status structure */
typedef struct __attribute__((packed)) {
    uint8_t  bStatus;        /* 0=OK, 1=errTarget, 2=errFile, etc. */
    uint8_t  bwPollTimeout[3]; /* Min time between polls (ms, 24-bit LE) */
    uint8_t  bState;         /* Current DFU state */
    uint8_t  iString;        /* String index for error */
} usb_dfu_status_t;

/* DFU States */
#define DFU_STATE_IDLE                  0x00
#define DFU_STATE_DNLOAD_SYNC          0x02
#define DFU_STATE_DNBUSY                0x04
#define DFU_STATE_DNLOAD_COMPLETE       0x05
#define DFU_STATE_MANIFEST_SYNC         0x06
#define DFU_STATE_MANIFEST              0x07
#define DFU_STATE_MANIFEST_WAIT_RESET   0x08
#define DFU_STATE_UPLOAD_IDLE           0x09
#define DFU_STATE_ERROR                 0x0A

#endif /* USB_DESCRIPTORS_H */