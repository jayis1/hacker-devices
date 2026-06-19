/**
 * drivers/usb_dev.c — USB CDC console (minimal) for PhotonStrike
 * Author: jayis1
 * License: GPL-2.0
 *
 * A minimal USB 2.0 Full-Speed CDC-ACM implementation: enough to expose
 * a text console for the `version`, `safety`, `temp`, `clk`, `disarm`,
 * `kill`, `help` commands handled in main.c. The full USB stack (device
 * descriptors, enumeration, endpoint handlers) is provided by a ported
 * STM32 USB device library (third_party/stm32_usb/) linked by the
 * Makefile; this file wraps its CDC interface and adds a line buffer.
 *
 * DFU mode is entered by holding the arm button at boot; the bootloader
 * is the built-in STM32 DFU in system memory (entered via jumping to
 * 0x1FF09800).
 */

#include "usb_dev.h"
#include "gpio.h"
#include "../registers.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* USB CDC ring buffer for received characters (console input). */
#define USB_RX_SIZE 128u
static volatile char   usb_rx[USB_RX_SIZE];
static volatile uint16_t usb_rx_head, usb_rx_tail;

/* The ported USB library calls this when a byte arrives on the CDC
 * data interface. We enqueue it into the line buffer. */
void ps_usb_cdc_rx_hook(uint8_t b)
{
    uint16_t next = (uint16_t)((usb_rx_head + 1u) % USB_RX_SIZE);
    if (next != usb_rx_tail) {
        usb_rx[usb_rx_head] = (char)b;
        usb_rx_head = next;
    }
}

/* The ported library exposes these: */
extern void ps_usb_cdc_init(void);
extern void ps_usb_cdc_tx(const uint8_t *buf, uint16_t len);

void usb_init(void)
{
    usb_rx_head = usb_rx_tail = 0;
    ps_usb_cdc_init();
}

void usb_print(const char *s)
{
    ps_usb_cdc_tx((const uint8_t *)s, (uint16_t)strlen(s));
}

void usb_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) ps_usb_cdc_tx((const uint8_t *)buf, (uint16_t)n);
}

bool usb_readline(char *buf, uint16_t maxlen)
{
    /* Pull bytes from the ring until a newline or the buffer fills. */
    static uint16_t pos = 0;
    while (usb_rx_head != usb_rx_tail) {
        char c = usb_rx[usb_rx_tail];
        usb_rx_tail = (uint16_t)((usb_rx_tail + 1u) % USB_RX_SIZE);
        if (c == '\r' || c == '\n') {
            if (pos > 0) {
                buf[pos] = '\0';
                pos = 0;
                return true;
            }
            continue;
        }
        if (pos < maxlen - 1u)
            buf[pos++] = c;
    }
    return false;
}