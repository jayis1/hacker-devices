/**
 * usb_dev.c — USB Device Stack for Forge-Probe Host Interface
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Implements a USB High-Speed (480 Mbps) vendor-specific device class
 * for command/response communication with the Forge-Probe companion app.
 * Uses 6 bulk endpoints for high-throughput flash dump streaming.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "board.h"

/* USB endpoint configuration */
#define EP_CONTROL              0x00
#define EP_CMD_OUT              0x01    /* Host → Device commands (64 byte) */
#define EP_CMD_IN               0x82    /* Device → Host responses (64 byte) */
#define EP_DATA_OUT             0x03    /* Host → Device data (512 byte HS) */
#define EP_DATA_IN              0x84    /* Device → Host data (512 byte HS) */
#define EP_STREAM_IN            0x85    /* High-speed flash dump stream */

/* USB request buffer */
static uint8_t g_usb_rx_buf[2048];
static volatile uint32_t g_usb_rx_len = 0;
static volatile bool g_usb_has_data = false;

/* USB descriptor */
static const uint8_t device_descriptor[] = {
    0x12,                           /* bLength */
    0x01,                           /* bDescriptorType: Device */
    0x00, 0x02,                     /* bcdUSB: 2.00 */
    0xFF,                           /* bDeviceClass: Vendor-specific */
    0xFF,                           /* bDeviceSubClass */
    0xFF,                           /* bDeviceProtocol */
    0x40,                           /* bMaxPacketSize0: 64 */
    0x34, 0x12,                     /* idVendor: 0x1234 (Forge-Probe) */
    0x01, 0x00,                     /* idProduct: 0x0001 */
    0x00, 0x01,                     /* bcdDevice: 1.00 */
    0x01,                           /* iManufacturer: "jayis1" */
    0x02,                           /* iProduct: "Forge-Probe" */
    0x03,                           /* iSerialNumber */
    0x01                            /* bNumConfigurations */
};

static const char *usb_strings[] = {
    "\x09\x04",                     /* Language: English US */
    "jayis1",                       /* Manufacturer */
    "Forge-Probe v1.0",             /* Product */
    "FP-00000001",                  /* Serial number */
};

/**
 * usb_init: Initialize the USB HS peripheral with ULPI PHY.
 */
void usb_init(void)
{
    /* Enable USB OTG HS clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSEN;

    /* Configure ULPI pins */
    /* ULPI requires: CLK, D0-D7, STP, NXT, DIR */
    mpio_set_af(GPIOA, 5, 10);   /* ULPI_CLK */
    mpio_set_af(GPIOA, 3, 10);   /* ULPI_D0 */
    mpio_set_af(GPIOA, 2, 10);   /* ULPI_D1 */
    mpio_set_af(GPIOA, 0, 10);   /* ULPI_D2 */
    mpio_set_af(GPIOA, 1, 10);   /* ULPI_D3 */
    mpio_set_af(GPIOB, 0, 10);   /* ULPI_D4 */
    mpio_set_af(GPIOB, 1, 10);   /* ULPI_D5 */
    mpio_set_af(GPIOB, 10, 10);  /* ULPI_D6 */
    mpio_set_af(GPIOB, 11, 10);  /* ULPI_D7 */
    mpio_set_af(GPIOC, 0, 10);   /* ULPI_STP */
    mpio_set_af(GPIOC, 1, 10);   /* ULPI_NXT */
    mpio_set_af(GPIOH, 4, 10);   /* ULPI_DIR */

    /* Configure VBUS sensing */
    mpio_set_mode(USB_HS_VBUS_PORT, USB_HS_VBUS_PIN, GPIO_MODE_INPUT);

    /* Reset USB core */
    USB_OTG_HS->GRSTCTL = 1;
    while (USB_OTG_HS->GRSTCTL & 1);

    /* Configure USB speed and PHY */
    USB_OTG_HS->GUSBCFG &= ~(3UL << 6);   /* Reset HS/FS select */
    USB_OTG_HS->GUSBCFG |= (1UL << 7);    /* ULPI PHY */
    USB_OTG_HS->GUSBCFG |= (1UL << 6);    /* HS speed */
    USB_OTG_HS->GUSBCFG |= (1UL << 30);   /* Force host mode? No — device mode */
    USB_OTG_HS->GUSBCFG &= ~(1UL << 30);

    /* Configure VBUS timing */
    USB_OTG_HS->GUSBCFG |= (0x5 << 10);   /* 5 ms VBUS pulse time */

    /* Flush TX and RX FIFOs */
    USB_OTG_HS->GRSTCTL = (0x10 << 6) | 1;  /* Flush RX */
    while (USB_OTG_HS->GRSTCTL & 1);
    USB_OTG_HS->GRSTCTL = (0x20 << 6) | 1;  /* Flush TX */
    while (USB_OTG_HS->GRSTCTL & 1);

    /* Configure device FIFOs */
    USB_OTG_HS->DIEPTXF0 = (0x80 << 16) | 0x40;  /* EP0: 2 KB start, 0.5 KB size */
    USB_OTG_HS->DIEPTXF = 0x00800180UL;          /* EP1 TX: 1.5 KB offset, 0.5 KB size */
    USB_OTG_HS->DIEPTXF = 0x01000280UL;          /* EP2 TX */

    /* Set device address to 0 (unconfigured) */
    USB_OTG_HS->DCFG &= ~(0x7F << 4);

    /* Enable USB global interrupts */
    USB_OTG_HS->GAHBCFG |= 1;             /* Enable global interrupt */
    USB_OTG_HS->GINTMSK |= (1 << 3);      /* Resume */
    USB_OTG_HS->GINTMSK |= (1 << 4);      /* Session request */
    USB_OTG_HS->GINTMSK |= (1 << 12);     /* USB suspend */
    USB_OTG_HS->GINTMSK |= (1 << 17);     /* RX FIFO not empty */
    USB_OTG_HS->GINTMSK |= (1 << 18);     /* TX FIFO empty */
}

/**
 * usb_send: Send a block of data to the host via the data IN endpoint.
 * Returns number of bytes sent.
 */
int32_t usb_send(const uint8_t *data, uint32_t len)
{
    uint32_t packet_size = 512;  /* HS max packet */
    uint32_t remaining = len;
    uint32_t offset = 0;
    int32_t total_sent = 0;

    while (remaining > 0)
    {
        uint32_t chunk = (remaining > packet_size) ? packet_size : remaining;

        /* Wait for TX FIFO available */
        while (!(USB_OTG_HS->DIEPINT[EP_DATA_IN] & (1 << 0)));

        /* Write data to FIFO */
        for (uint32_t i = 0; i < (chunk + 3) / 4; i++)
        {
            uint32_t word = 0;
            if (offset + i * 4 < len)
                memcpy(&word, &data[offset + i * 4], 4);
            USB_OTG_HS->DFIFO[EP_DATA_IN] = word;
        }

        /* Set transfer size and enable */
        USB_OTG_HS->DIEPTSIZ[EP_DATA_IN] = (1 << 19) | chunk;  /* 1 packet, size */
        USB_OTG_HS->DIEPCTL[EP_DATA_IN] |= (1 << 31);          /* Enable */

        remaining -= chunk;
        offset += chunk;
        total_sent += chunk;
    }

    return total_sent;
}

/**
 * usb_receive_nonblocking: Non-blocking read of USB command data.
 * Returns number of bytes received, or 0 if no data available.
 */
int32_t usb_receive_nonblocking(uint8_t *buffer, uint32_t max_len)
{
    if (!g_usb_has_data)
        return 0;

    uint32_t len = (g_usb_rx_len > max_len) ? max_len : g_usb_rx_len;
    memcpy(buffer, g_usb_rx_buf, len);

    g_usb_has_data = false;
    g_usb_rx_len = 0;

    return (int32_t)len;
}

/**
 * USB_OTG_HS_IRQHandler: Main USB interrupt handler.
 * Processes setup packets, data transfers, and control requests.
 */
void USB_OTG_HS_IRQHandler(void)
{
    uint32_t int_status = USB_OTG_HS->GINTSTS;
    uint32_t int_mask = USB_OTG_HS->GINTMSK;

    /* Mask out disabled interrupts */
    int_status &= int_mask;

    /* RX FIFO non-empty */
    if (int_status & (1 << 17))
    {
        uint32_t status = USB_OTG_HS->GRXSTSP;
        uint8_t ep_num = status & 0x0F;
        uint32_t bcnt = (status >> 4) & 0x7FF;
        uint32_t data_pid = (status >> 15) & 0x03;

        if (ep_num == EP_CMD_OUT || ep_num == EP_DATA_OUT)
        {
            /* Read data from RX FIFO */
            for (uint32_t i = 0; i < (bcnt + 3) / 4; i++)
            {
                uint32_t word = USB_OTG_HS->DFIFO[0];
                if (g_usb_rx_len + 4 <= sizeof(g_usb_rx_buf))
                {
                    memcpy(&g_usb_rx_buf[g_usb_rx_len], &word, 4);
                    g_usb_rx_len += (bcnt - i * 4 >= 4) ? 4 : (bcnt % 4);
                }
            }
            g_usb_has_data = true;
        }
    }

    /* USB reset */
    if (int_status & (1 << 2))
    {
        /* Reinitialize endpoints */
        USB_OTG_HS->DCTL &= ~(1 << 0);  /* Clear remote wakeup */
        USB_OTG_HS->DIEPMSK = 0;
        USB_OTG_HS->DOEPMSK = 0;
    }

    /* Suspend */
    if (int_status & (1 << 12))
    {
        /* Enter low-power mode */
    }

    /* Clear all handled interrupts */
    USB_OTG_HS->GINTSTS = int_status;
}

/**
 * usb_connected: Check if USB host is connected and configured.
 */
bool usb_connected(void)
{
    return (USB_OTG_HS->DSTS & (1 << 2)) != 0;  /* Enumeration done */
}