/*
 * usb_cdc.c — USB-C CDC console
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * Tiny USB CDC console for configuration and raw register dump. The command
 * parser lives in main.c (usb_console_handle); this module just provides
 * buffered line I/O and a register-dump helper.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"
#include "usb_cdc.h"

__attribute__((weak)) int usb_write(const uint8_t *d, uint32_t l) { (void)d; (void)l; return (int)l; }
__attribute__((weak)) int usb_read(uint8_t *b, uint32_t l) { (void)b; (void)l; return 0; }

/* These are implemented in spad_driver.c; declare here for the dump. */
extern uint16_t fpga_read(uint8_t addr);

int usb_cdc_init(void) { return 0; }

int usb_cdc_write(const uint8_t *data, uint32_t len)
{
    return usb_write(data, len);
}

int usb_cdc_read(uint8_t *buf, uint32_t max)
{
    return usb_read(buf, max);
}

void usb_cdc_puts(const char *s)
{
    usb_write((const uint8_t *)s, (uint32_t)strlen(s));
}

void usb_cdc_dump_regs(void)
{
    char line[48];
    for (uint8_t a = 0; a <= REG_MAX_ADDR; a += 4) {
        int n = snprintf(line, sizeof(line),
                         "%02X: %04X %04X %04X %04X\r\n",
                         a, fpga_read(a), fpga_read(a + 1),
                         fpga_read(a + 2), fpga_read(a + 3));
        usb_write((const uint8_t *)line, (uint32_t)n);
    }
}