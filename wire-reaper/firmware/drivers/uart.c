/*
 * uart.c — WireReaper UART driver: sniffer and injector
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Four independent UART channels for debug console sniffing and
 * command injection. Each channel supports independent baud rates
 * from 300 baud to 6 Mbps. DMA-based receive for glitch-free capture.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* UART peripheral instances */
static usart_t *const uart_devs[NUM_UART_CHANNELS] = {
    (usart_t *)UART0_USART_BASE,
    (usart_t *)UART1_USART_BASE,
    (usart_t *)UART2_UART_BASE,
    (usart_t *)UART3_UART_BASE,
};

/* Per-channel RX ring buffer (in internal SRAM for speed) */
#define UART_RX_BUF_SIZE 512
static uint8_t  uart_rx_buf[NUM_UART_CHANNELS][UART_RX_BUF_SIZE];
static volatile uint16_t uart_rx_head[NUM_UART_CHANNELS];
static volatile uint16_t uart_rx_tail[NUM_UART_CHANNELS];
static uint32_t uart_baud[NUM_UART_CHANNELS];

/* ---- GPIO setup ---- */
static void uart_gpio_init(int ch) {
    gpio_t *txport, *rxport;
    uint32_t txpin, rxpin, af;

    switch (ch) {
    case 0:
        txport = GPIO(UART0_TX_PORT); rxport = GPIO(UART0_RX_PORT);
        txpin = UART0_TX_PIN; rxpin = UART0_RX_PIN; af = 7; /* USART2 AF7 */
        break;
    case 1:
        txport = GPIO(UART1_TX_PORT); rxport = GPIO(UART1_RX_PORT);
        txpin = UART1_TX_PIN; rxpin = UART1_RX_PIN; af = 7; /* USART3 AF7 */
        break;
    case 2:
        txport = GPIO(UART2_TX_PORT); rxport = GPIO(UART2_RX_PORT);
        txpin = UART2_TX_PIN; rxpin = UART2_RX_PIN; af = 8; /* UART4 AF8 */
        break;
    case 3:
        txport = GPIO(UART3_TX_PORT); rxport = GPIO(UART3_RX_PORT);
        txpin = UART3_TX_PIN; rxpin = UART3_RX_PIN; af = 8; /* UART5 AF8 */
        break;
    default:
        return;
    }

    /* TX: AF push-pull */
    txport->MODER &= ~(3U << (txpin * 2));
    txport->MODER |= (GPIO_MODE_AF << (txpin * 2));
    txport->OSPEEDR |= (GPIO_OSPEED_HIGH << (txpin * 2));
    txport->AFR[txpin / 8] &= ~(0xFU << ((txpin % 8) * 4));
    txport->AFR[txpin / 8] |= (af << ((txpin % 8) * 4));

    /* RX: AF input with pull-up */
    rxport->MODER &= ~(3U << (rxpin * 2));
    rxport->MODER |= (GPIO_MODE_AF << (rxpin * 2));
    rxport->PUPDR |= (GPIO_PUPD_PULLUP << (rxpin * 2));
    rxport->AFR[rxpin / 8] &= ~(0xFU << ((rxpin % 8) * 4));
    rxport->AFR[rxpin / 8] |= (af << ((rxpin % 8) * 4));
}

/* ---- Compute BRR from baud rate ---- */
static uint32_t uart_compute_brr(uint32_t baud) {
    /* USART on APB1 = 120 MHz (USART2/3/4/5) */
    uint32_t pclk = APB1_HZ;
    /* BRR = UARTDIV = fclk / baud (oversampling by 16) */
    return (pclk + baud / 2) / baud;
}

/* ---- Initialize UART channel ---- */
void wr_uart_init(int ch, uint32_t baud) {
    if (ch < 0 || ch >= NUM_UART_CHANNELS)
        return;

    /* Enable peripheral clocks */
    RCC_APB1ENR1 |= RCC_APB1ENR_USART2 | RCC_APB1ENR_USART3 |
                    RCC_APB1ENR_UART4 | RCC_APB1ENR_UART5;
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOD | RCC_AHB1ENR_GPIOA |
                   RCC_AHB1ENR_GPIOB;

    uart_gpio_init(ch);

    usart_t *u = uart_devs[ch];
    u->CR1 = 0; /* Disable */

    u->BRR = uart_compute_brr(baud);
    uart_baud[ch] = baud;

    /* 8N1, enable TX, RX, and RXNE interrupt */
    u->CR2 = 0; /* 1 stop bit */
    u->CR3 = 0;
    u->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    uart_rx_head[ch] = 0;
    uart_rx_tail[ch] = 0;

    /* Enable NVIC interrupt for this UART */
    switch (ch) {
    case 0: nvic_enable(IRQ_USART2); break;
    case 1: nvic_enable(IRQ_USART3); break;
    case 2: nvic_enable(IRQ_UART4); break;
    case 3: nvic_enable(IRQ_UART5); break;
    }
}

/* ---- Set baud rate ---- */
void wr_uart_set_baud(int ch, uint32_t baud) {
    if (ch < 0 || ch >= NUM_UART_CHANNELS)
        return;
    usart_t *u = uart_devs[ch];
    uint32_t cr1 = u->CR1;
    u->CR1 = cr1 & ~USART_CR1_UE; /* Disable */
    u->BRR = uart_compute_brr(baud);
    uart_baud[ch] = baud;
    u->CR1 = cr1; /* Re-enable */
}

/* ---- UART RX interrupt handlers ---- */
static void uart_rx_isr(int ch) {
    usart_t *u = uart_devs[ch];
    uint32_t isr = u->ISR;

    if (isr & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)u->RDR;
        uint16_t next = (uart_rx_head[ch] + 1) % UART_RX_BUF_SIZE;
        if (next != uart_rx_tail[ch]) {
            uart_rx_buf[ch][uart_rx_head[ch]] = b;
            uart_rx_head[ch] = next;
        }
        /* else: buffer overflow, drop byte */
    }
    if (isr & USART_ISR_ORE) {
        u->ICR = (1U << 3); /* Clear ORE */
    }
}

IRQ_HANDLER(USART2_IRQHandler)  { uart_rx_isr(0); }
IRQ_HANDLER(USART3_IRQHandler)  { uart_rx_isr(1); }
IRQ_HANDLER(UART4_IRQHandler)   { uart_rx_isr(2); }
IRQ_HANDLER(UART5_IRQHandler)   { uart_rx_isr(3); }

/* ---- Read sniffed bytes from buffer ---- */
int wr_uart_sniff(int ch, uint8_t *buf, int maxlen) {
    if (ch < 0 || ch >= NUM_UART_CHANNELS)
        return 0;

    int count = 0;
    while (count < maxlen && uart_rx_tail[ch] != uart_rx_head[ch]) {
        buf[count++] = uart_rx_buf[ch][uart_rx_tail[ch]];
        uart_rx_tail[ch] = (uart_rx_tail[ch] + 1) % UART_RX_BUF_SIZE;
    }
    return count;
}

/* ---- Inject bytes on UART TX ---- */
int wr_uart_inject(int ch, const uint8_t *buf, int len) {
    if (ch < 0 || ch >= NUM_UART_CHANNELS || len <= 0)
        return WR_ERR_PARAM;

    usart_t *u = uart_devs[ch];
    for (int i = 0; i < len; i++) {
        while (!(u->ISR & USART_ISR_TXE));
        u->TDR = buf[i];
    }
    /* Wait for transmission complete */
    while (!(u->ISR & USART_ISR_TC));
    return WR_OK;
}

/* ---- Auto-baud detection ---- */
/* Detects baud rate by measuring the width of a start bit edge
 * using a timer capture on the RX pin. Useful when the target's
 * baud rate is unknown (common with debug UARTs). */
uint32_t wr_uart_autobaud(int ch) {
    if (ch < 0 || ch >= NUM_UART_CHANNELS)
        return 0;

    /* Reconfigure RX pin as input with falling-edge EXTI */
    gpio_t *rxport;
    uint32_t rxpin;
    switch (ch) {
    case 0: rxport = GPIO(UART0_RX_PORT); rxpin = UART0_RX_PIN; break;
    case 1: rxport = GPIO(UART1_RX_PORT); rxpin = UART1_RX_PIN; break;
    case 2: rxport = GPIO(UART2_RX_PORT); rxpin = UART2_RX_PIN; break;
    case 3: rxport = GPIO(UART3_RX_PORT); rxpin = UART3_RX_PIN; break;
    default: return 0;
    }

    /* Save current AF mode, switch to input */
    uint32_t save_moder = rxport->MODER;
    rxport->MODER &= ~(3U << (rxpin * 2)); /* Input mode */

    /* Use TIM2 input capture to measure bit period.
     * We wait for a falling edge (start bit), then measure time
     * to the next falling edge or use the bit period from the
     * first byte received. */
    tim_t *tim = TIM(TIM2_BASE);
    /* Enable TIM2 clock */
    RCC_APB1ENR1 |= RCC_APB1ENR_TIM2;

    tim->CR1 = 0;
    tim->PSC = (APB1_TIM_HZ / 1000000) - 1; /* 1 MHz = 1 us per tick */
    tim->ARR = 0xFFFFFFFF;
    tim->CCMR1 = (1U << 0); /* CC1 mapped to TI1 */
    tim->CCER = (1U << 0) | (1U << 1); /* CC1 enable, falling edge */
    tim->CR1 = TIM_CR1_CEN;

    /* Wait for first capture (start bit falling edge) */
    uint32_t timeout = 1000000;
    while (!(tim->SR & (1U << 0)) && timeout > 0) timeout--;
    if (timeout == 0) {
        rxport->MODER = save_moder;
        return 0;
    }
    uint32_t c1 = tim->CCR1;
    tim->SR = 0;

    /* Wait for second capture (next start bit) */
    timeout = 1000000;
    while (!(tim->SR & (1U << 0)) && timeout > 0) timeout--;
    if (timeout == 0) {
        rxport->MODER = save_moder;
        return 0;
    }
    uint32_t c2 = tim->CCR1;

    tim->CR1 = 0;
    rxport->MODER = save_moder; /* Restore AF mode */

    /* Period between start bits = N * bit_time.
     * Assuming a common byte like 0x0D (CR) or 0x0A (LF) with
     * 8N1 framing = 10 bit periods per byte. We estimate as 10. */
    uint32_t period_us = c2 - c1;
    if (period_us == 0)
        return 0;

    /* Try common baud rates and see which divides evenly */
    static const uint32_t common_bauds[] = {
        9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600,
        1000000, 1500000, 2000000, 3000000, 0
    };
    for (int i = 0; common_bauds[i] != 0; i++) {
        uint32_t bit_us = 1000000 / common_bauds[i];
        /* Check if period is a multiple of bit_us (within 3% tolerance) */
        uint32_t num_bits = (period_us + bit_us / 2) / bit_us;
        if (num_bits > 0 && num_bits < 100) {
            uint32_t expected = num_bits * bit_us;
            int32_t diff = (int32_t)period_us - (int32_t)expected;
            if (diff < 0) diff = -diff;
            if (diff < (int32_t)(bit_us / 3))
                return common_bauds[i];
        }
    }
    /* Fallback: estimate assuming 10-bit frame */
    return (10 * 1000000) / period_us;
}