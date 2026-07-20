/*
 * hart-bleeder — modem_drv.c
 * HART FSK modem driver: USART2 @ 1200 baud, 8-O-1, half-duplex,
 * RTS/CD control for the Bell 202 modem IC (MAX13440E / TH8032).
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "modem_drv.h"

/* ── System tick (TIM7 1 ms interrupt, set up in board_init) ──── */
static volatile uint32_t s_tick = 0;
void SysTick_Handler(void) { s_tick++; }
uint32_t system_millis(void) { return s_tick; }

void system_delay_ms(uint32_t ms) {
    uint32_t t0 = s_tick;
    while ((s_tick - t0) < ms) __asm volatile("wfi");
}

void system_delay_us(uint32_t us) {
    /* Rough loop delay at 170 MHz — ~6 cycles per iteration */
    volatile uint32_t n = us * 28;
    while (n--) __asm volatile("nop");
}

/* ── USART helpers ───────────────────────────────────────────── */
void uart_init(uint32_t base, uint32_t baud)
{
    usart_regs_t *u = (usart_regs_t *)base;
    /* Compute BRR for 16x oversampling: BRR = f_pclk / baud.
     * USART2 is on APB1; assume pclk = 42 MHz after PLL config.
     */
    uint32_t pclk;
    if (base == USART1_BASE || base == USART3_BASE || base == UART4_BASE)
        pclk = 42000000UL;
    else
        pclk = 42000000UL;
    u->BRR = (pclk + baud / 2) / baud;

    /* 8 data bits, 1 stop, odd parity (HART uses 8-O-1) */
    u->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_M0;
    /* Odd parity enable: set PCE=1, PS=1 (odd) in CR1 */
    u->CR1 |= (1U << 10) | (1U << 9);   /* PCE | PS(odd) */
    /* No hardware flow control, no half-duplex (we gate with RTS) */
    u->CR2 = 0;
    u->CR3 = 0;
}

int uart_putc(uint32_t base, uint8_t c)
{
    usart_regs_t *u = (usart_regs_t *)base;
    while (!(u->ISR & USART_ISR_TXE)) { }
    u->TDR = c;
    return 1;
}

int uart_getc(uint32_t base)
{
    usart_regs_t *u = (usart_regs_t *)base;
    while (!(u->ISR & USART_ISR_RXNE)) {
        if (u->ISR & (USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE))
            u->ICR = 0xFFFFFFFFU;   /* clear errors */
    }
    return (int)(u->RDR & 0xFF);
}

int uart_write(uint32_t base, const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        if (uart_putc(base, buf[i]) < 0) return -1;
    }
    /* Wait for final TX complete */
    usart_regs_t *u = (usart_regs_t *)base;
    while (!(u->ISR & USART_ISR_TC)) { }
    return len;
}

int uart_read_timeout(uint32_t base, uint8_t *buf, uint16_t n, uint32_t ms)
{
    usart_regs_t *u = (usart_regs_t *)base;
    uint32_t t0 = system_millis();
    uint16_t got = 0;
    while (got < n) {
        while (!(u->ISR & USART_ISR_RXNE)) {
            if (u->ISR & (USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE))
                u->ICR = 0xFFFFFFFFU;
            if ((system_millis() - t0) >= ms) return (int)got;
        }
        buf[got++] = (uint8_t)(u->RDR & 0xFF);
    }
    return (int)got;
}

void uart_flush_rx(uint32_t base)
{
    usart_regs_t *u = (usart_regs_t *)base;
    (void)u->RDR;
    u->ICR = 0xFFFFFFFFU;
}

/* ── Modem-specific functions ─────────────────────────────────── */
int modem_init(void)
{
    /* Configure USART2 @ 1200 baud, 8-O-1 for the HART modem */
    uart_init(USART2_BASE, HART_BAUD);

    /* RTS / CD / SHDN pins are configured in board_init_gpio() */
    gpio_write(GPIO(A), PIN_HART_SHDN, 1);   /* bring modem out of shutdown */
    gpio_write(GPIO(A), PIN_HART_RTS, 1);     /* /RTS high = RX mode (no TX) */
    system_delay_ms(5);                       /* modem settle */

    return modem_self_test();
}

void modem_tx_enable(int on)
{
    /* /RTS active low drives the FSK onto the loop.
     * When /RTS=0, modem transmits; when /RTS=1, modem receives.
     */
    gpio_write(GPIO(A), PIN_HART_RTS, on ? 0 : 1);
    if (on) system_delay_us(200);             /* modem TX settle */
}

int modem_rx_ready(void)
{
    usart_regs_t *u = (usart_regs_t *)USART2_BASE;
    return (u->ISR & USART_ISR_RXNE) ? 1 : 0;
}

int modem_carrier_detect(void)
{
    /* /CD pin active low — return 1 when carrier present */
    return gpio_read(GPIO(A), PIN_HART_CD) ? 0 : 1;
}

int modem_write(const uint8_t *buf, uint16_t len)
{
    return uart_write(USART2_BASE, buf, len);
}

int modem_read_timeout(uint32_t ms)
{
    uint8_t c;
    int n = uart_read_timeout(USART2_BASE, &c, 1, ms);
    return (n == 1) ? (int)c : -1;
}

int modem_avail(void)
{
    return modem_rx_ready();
}

void modem_set_baud(uint32_t baud)
{
    uart_init(USART2_BASE, baud);
}

void modem_shutdown(int on)
{
    gpio_write(GPIO(A), PIN_HART_SHDN, on ? 0 : 1);
}

int modem_self_test(void)
{
    /* Check that carrier-detect line is not stuck active when no traffic. */
    system_delay_ms(2);
    if (modem_carrier_detect()) return 0;    /* false CD = fault */
    return 1;
}