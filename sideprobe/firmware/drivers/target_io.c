/*
 * drivers/target_io.c — plaintext drive (UART/SPI) + trigger GPIO for the target
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * SideProbe must feed the target a known plaintext for each trace (CPA needs
 * known-plaintext). Two modes:
 *   DRIVE_UART — push 16-byte plaintext over USART1 @ up to 6 Mbaud, then
 *                pulse the trigger-out GPIO to tell the target "go".
 *   DRIVE_SPI  — push 16 bytes over SPI1 to a target in slave mode.
 * The target replies with 16 bytes of ciphertext (optional, used for 2nd-round
 * attacks and to confirm the key).
 */

#include <stdint.h>
#include "../board.h"
#include "../registers.h"

extern void sp_delay_us(volatile uint32_t us);

/* USART1 on PA9(TX)/PA10(RX), AF7 */
static void uart1_init(uint32_t baud) {
    /* Configure PA9, PA10 as AF7 */
    uint32_t moder = GPIO_MODER(GPIOA_BASE);
    moder &= ~((3u << 18) | (3u << 20));
    moder |= (2u << 18) | (2u << 20); /* AF mode */
    GPIO_MODER(GPIOA_BASE) = moder;
    uint32_t afrl = GPIO_AFRL(GPIOA_BASE);
    afrl &= ~(0xFu << (9 * 4 - 32)); /* PA9 in AFRH */
    uint32_t afrh = GPIO_AFRH(GPIOA_BASE);
    afrh &= ~((0xFu << (1 * 4)) | (0xFu << (2 * 4))); /* PA10 (idx2), PA9 (idx1) in AFRH */
    afrh |= (7u << (1 * 4)) | (7u << (2 * 4));       /* AF7 */
    GPIO_AFRH(GPIOA_BASE) = afrh;

    USART_CR1(USART1_BASE) = 0; /* disable */
    USART_BRR(USART1_BASE) = (APB2_HZ / baud);
    USART_CR2(USART1_BASE) = 0;
    USART_CR3(USART1_BASE) = 0;
    USART_CR1(USART1_BASE) = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart1_put(uint8_t b) {
    while (!(USART_ISR(USART1_BASE) & USART_ISR_TXE)) { }
    USART_TDR(USART1_BASE) = b;
}

static int uart1_get(uint8_t *b, uint32_t timeout_us) {
    while (!(USART_ISR(USART1_BASE) & USART_ISR_RXNE)) {
        if (--timeout_us == 0) return -1;
        sp_delay_us(1);
    }
    *b = (uint8_t)USART_RDR(USART1_BASE);
    return 0;
}

/* Pulse the trigger-out line (PC14) high for `us` microseconds to tell the
   target "start crypto now". */
static void trigger_out_pulse(uint32_t us) {
    GPIO_BSRR(GPIOC_BASE) = (1u << 14);          /* high */
    sp_delay_us(us);
    GPIO_BSRR(GPIOC_BASE) = (1u << (14 + 16));  /* low */
}

void target_io_init(void) {
    uart1_init(115200); /* default; raised via SET RATE later */
    /* PC14 trigger-out already configured as output in board_init */
}

int target_send_plaintext(const uint8_t *pt, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) uart1_put(pt[i]);
    trigger_out_pulse(5);
    return 0;
}

int target_recv_ciphertext(uint8_t *ct, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (uart1_get(&ct[i], 50000) != 0) return -1;
    }
    return 0;
}

void target_io_set_baud(uint32_t baud) {
    uart1_init(baud);
}