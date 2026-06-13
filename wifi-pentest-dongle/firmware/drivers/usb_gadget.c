/*
 * WFP-100 — USB Composite Gadget Driver Implementation
 * CDC-ECM (Ethernet) + CDC-ACM (Serial) composite USB gadget.
 *
 * When the WFP-100 is plugged into a host via USB-C, it appears as:
 *   - CDC-ECM network interface (wfp0): for packet capture data
 *   - CDC-ACM serial port (ttyGS0): for debug console and command interface
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/usb/cdc.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include "usb_gadget.h"

/* Module metadata */
MODULE_AUTHOR("WFP-100 Project");
MODULE_DESCRIPTION("WFP-100 USB Composite Gadget: CDC-ECM + CDC-ACM");
MODULE_LICENSE("GPL");

/* ========== String Descriptors ========== */

static struct usb_string strings_dev[] = {
    { WFP100_STRING_IDX_MANUFACTURER, "Hacker Devices" },
    { WFP100_STRING_IDX_PRODUCT,      "WFP-100 WiFi Pentester" },
    { WFP100_STRING_IDX_SERIAL,        "WFP100-2026001" },
    {  }  /* terminator */
};

static struct usb_gadget_strings stringtab_dev = {
    .language = 0x0409,  /* en-us */
    .strings  = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
    &stringtab_dev,
    NULL,
};

/* ========== CDC-ECM (Ethernet) Function ========== */

static struct usb_function_instance *ecm_fi;
static struct usb_function *ecm_f;
static struct net_device *wfp_netdev;

/* ECM interface class descriptor */
static struct usb_interface_descriptor ecm_control_intf = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = WFP100_IF_ECM_CTRL,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_COMM,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ECM,
    .bInterfaceProtocol = USB_CDC_PROTO_NONE,
    .iInterface         = 4,
};

static struct usb_interface_descriptor ecm_data_intf = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = WFP100_IF_ECM_DATA,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = USB_CLASS_CDC_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 5,
};

/* ========== CDC-ACM (Serial) Function ========== */

static struct usb_function_instance *acm_fi;
static struct usb_function *acm_f;

/* ========== Composite Device Configuration ========== */

static struct usb_configuration wfp100_config_driver = {
    .label          = "WFP-100 Composite",
    .bConfigurationValue = 1,
    .iConfiguration = 6,
    .bmAttributes   = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .MaxPower       = 500,  /* 500mA */
};

/* ========== Device Descriptor ========== */

static struct usb_device_descriptor wfp100_device_desc = {
    .bLength            = USB_DT_DEVICE_SIZE,
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = cpu_to_le16(0x0200),
    .bDeviceClass       = USB_CLASS_MISC,
    .bDeviceSubClass    = 0x02,
    .bDeviceProtocol    = 0x01,
    .bMaxPacketSize0    = 64,
    .idVendor           = cpu_to_le16(0x1209),
    .idProduct          = cpu_to_le16(0xF100),
    .bcdDevice          = cpu_to_le16(0x0100),
    .iManufacturer      = WFP100_STRING_IDX_MANUFACTURER,
    .iProduct           = WFP100_STRING_IDX_PRODUCT,
    .iSerialNumber      = WFP100_STRING_IDX_SERIAL,
    .bNumConfigurations = 1,
};

/* ========== Bind/Unbind ========== */

static int wfp100_bind(struct usb_composite_dev *cdev)
{
    int ret;

    /* Allocate ECM function instance */
    ecm_fi = usb_get_function_instance("ecm");
    if (IS_ERR(ecm_fi))
        return PTR_ERR(ecm_fi);

    /* Set MAC address */
    if (ecm_fi && ecm_fi->set_inst_name) {
        struct f_ecm_opts *ecm_opts =
            container_of(ecm_fi, struct f_ecm_opts, func_inst);
        ecm_opts->mac = (struct ether_addr[]) {
            { .ether_addr_octet = {
                WFP100_ECM_MAC_BYTE0, WFP100_ECM_MAC_BYTE1,
                WFP100_ECM_MAC_BYTE2, WFP100_ECM_MAC_BYTE3,
                WFP100_ECM_MAC_BYTE4, WFP100_ECM_MAC_BYTE5
            }}
        };
    }

    ecm_f = usb_get_function(ecm_fi);
    if (IS_ERR(ecm_f)) {
        ret = PTR_ERR(ecm_f);
        goto err_ecm_fi;
    }

    /* Allocate ACM function instance */
    acm_fi = usb_get_function_instance("acm");
    if (IS_ERR(acm_fi)) {
        ret = PTR_ERR(acm_fi);
        goto err_ecm_f;
    }

    acm_f = usb_get_function(acm_fi);
    if (IS_ERR(acm_f)) {
        ret = PTR_ERR(acm_f);
        goto err_acm_fi;
    }

    /* Add functions to configuration */
    ret = usb_add_function(&wfp100_config_driver, ecm_f);
    if (ret)
        goto err_acm_f;

    ret = usb_add_function(&wfp100_config_driver, acm_f);
    if (ret)
        goto err_acm_f;

    /* Add configuration */
    ret = usb_add_config(cdev, &wfp100_config_driver);
    if (ret)
        goto err_acm_f;

    dev_info(&cdev->gadget->dev,
             "WFP-100 composite gadget: CDC-ECM + CDC-ACM\n");
    return 0;

err_acm_f:
    usb_put_function(acm_f);
err_acm_fi:
    usb_put_function_instance(acm_fi);
err_ecm_f:
    usb_put_function(ecm_f);
err_ecm_fi:
    usb_put_function_instance(ecm_fi);
    return ret;
}

static int wfp100_unbind(struct usb_composite_dev *cdev)
{
    usb_put_function(acm_f);
    usb_put_function_instance(acm_fi);
    usb_put_function(ecm_f);
    usb_put_function_instance(ecm_fi);
    return 0;
}

/* ========== Composite Driver ========== */

static struct usb_composite_driver wfp100_driver = {
    .name       = "wfp100_gadget",
    .dev        = &wfp100_device_desc,
    .strings    = dev_strings,
    .bind       = wfp100_bind,
    .unbind     = wfp100_unbind,
};

module_usb_composite_driver(wfp100_driver);