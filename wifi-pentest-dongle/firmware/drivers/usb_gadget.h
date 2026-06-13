/*
 * WFP-100 — USB Composite Gadget Driver Header
 * CDC-ECM (Ethernet) + CDC-ACM (Serial Console) composite gadget.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef WFP100_USB_GADGET_H
#define WFP100_USB_GADGET_H

#include <linux/usb/composite.h>
#include <linux/usb/cdc.h>

/* USB descriptor indices */
#define WFP100_STRING_IDX_MANUFACTURER    1
#define WFP100_STRING_IDX_PRODUCT         2
#define WFP100_STRING_IDX_SERIAL           3

/* ECM MAC address (locally administered) */
#define WFP100_ECM_MAC_BYTE0    0x02
#define WFP100_ECM_MAC_BYTE1    0x1A
#define WFP100_ECM_MAC_BYTE2    0x2B
#define WFP100_ECM_MAC_BYTE3    0x3C
#define WFP100_ECM_MAC_BYTE4    0x4D
#define WFP100_ECM_MAC_BYTE5    0x5E

/* Interface numbers */
#define WFP100_IF_ECM_CTRL       0
#define WFP100_IF_ECM_DATA       1
#define WFP100_IF_ACM_CTRL       2
#define WFP100_IF_ACM_DATA       3
#define WFP100_IF_NUM_INTERFACES 4

/* Endpoint addresses */
#define WFP100_EP_ECM_NOTIFY     0x81
#define WFP100_EP_ECM_IN         0x82
#define WFP100_EP_ECM_OUT        0x02
#define WFP100_EP_ACM_NOTIFY     0x83
#define WFP100_EP_ACM_IN         0x84
#define WFP100_EP_ACM_OUT        0x04

/* Maximum packet sizes */
#define WFP100_EP_BULK_MAXPKT    512    /* USB 2.0 HS bulk max */
#define WFP100_EP_INT_MAXPKT     8      /* Interrupt endpoint max */
#define WFP100_EP_NOTIFY_INTERVAL 9     /* 2^(9-1) = 256 μs */

/* Network device name */
#define WFP100_NET_DEV_NAME      "wfp0"

/* Serial device name */
#define WFP100_TTY_DEV_NAME      "ttyGS0"

#endif /* WFP100_USB_GADGET_H */