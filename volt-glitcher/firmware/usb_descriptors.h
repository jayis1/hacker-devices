#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* USB Device Descriptor */
const uint8_t device_descriptor[] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType */
    0x00, 0x02, /* bcdUSB 2.0 */
    0xEF,       /* bDeviceClass (Miscellaneous) */
    0x02,       /* bDeviceSubClass */
    0x01,       /* bDeviceProtocol (IAD) */
    0x40,       /* bMaxPacketSize0 */
    0x83, 0x04, /* idVendor (ST) */
    0x40, 0x57, /* idProduct */
    0x00, 0x02, /* bcdDevice 2.0 */
    0x01,       /* iManufacturer */
    0x02,       /* iProduct */
    0x03,       /* iSerialNumber */
    0x01        /* bNumConfigurations */
};

#endif // USB_DESCRIPTORS_H
