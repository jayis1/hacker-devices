/*
 * hart-bleeder — modem_drv.h
 * HART FSK modem driver interface (MAX13440E / TH8032 / AD5700).
 * The modem IC handles Bell 202 FSK (1200/2200 Hz) <-> UART 1200 baud.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_MODEM_DRV_H
#define HART_BLEEDER_MODEM_DRV_H

#include <stdint.h>

/* ── Modem driver API ───────────────────────────────────────────
 * The MCU talks to the modem over USART2 @ 1200 baud, 8-O-1.
 * /RTS (PA5) enables transmit output; /CD (PA6) indicates carrier.
 * The modem is half-duplex — tx_enable must gate each transmission.
 */

int  modem_init(void);
void modem_tx_enable(int on);
int  modem_rx_ready(void);
int  modem_carrier_detect(void);       /* returns 1 if HART carrier present */
int  modem_write(const uint8_t *buf, uint16_t len);
int  modem_read_timeout(uint32_t ms);
int  modem_avail(void);
void modem_set_baud(uint32_t baud);    /* normally 1200 */
void modem_shutdown(int on);
int  modem_self_test(void);

/* ── Low-level UART (exposed for console / BLE module) ─────────── */
void uart_init(uint32_t base, uint32_t baud);
int  uart_putc(uint32_t base, uint8_t c);
int  uart_getc(uint32_t base);
int  uart_write(uint32_t base, const uint8_t *buf, uint16_t len);
int  uart_read_timeout(uint32_t base, uint8_t *buf, uint16_t n, uint32_t ms);
void uart_flush_rx(uint32_t base);

/* ── System timing (provided by board_init) ───────────────────── */
uint32_t system_millis(void);
void     system_delay_ms(uint32_t ms);
void     system_delay_us(uint32_t us);

#endif /* HART_BLEEDER_MODEM_DRV_H */