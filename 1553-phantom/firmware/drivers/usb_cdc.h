/*
 * usb_cdc.h — USB CDC ACM API + freestanding libc helpers
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>

void usb_cdc_init(void);
void usb_cdc_isr(void);
int  usb_cdc_getc(void);
void usb_cdc_putc(char c);
void usb_cdc_puts(const char *s);
void usb_cdc_flush(void);

/* Freestanding libc replacements (shared across the firmware) */
int      atoi_lite(const char *s);
uint32_t hex32(const char *s);
int      itoa_dec(int v, char *buf, int sz);
int      utoa_dec(unsigned v, char *buf, int sz);

/* snprintf_lite is declared in main.c but used widely; declare here too */
typedef __builtin_va_list va_list_t;
#define va_start_lite(v,l) __builtin_va_start(v,l)
#define va_end_lite(v)      __builtin_va_end(v)
#define va_arg_lite(v,t)   __builtin_va_arg(v,t)
int snprintf_lite(char *buf, int sz, const char *fmt, ...);

#endif /* USB_CDC_H */