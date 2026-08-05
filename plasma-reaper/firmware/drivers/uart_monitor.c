/*
 * uart_monitor.c — Target UART response monitor
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Continuously captures the target's UART output using USART3 with
 * DMA. The captured data is used for two purposes:
 *  1. Trigger word detection (for UART-triggered glitches)
 *  2. Response classification (success/failure pattern matching)
 *
 * The DMA stores incoming bytes into a circular buffer. When a glitch
 * is fired, the main loop reads the buffer contents since the glitch
 * to classify the target's response.
 */

#include "uart_monitor.h"
#include "trigger.h"
#include "../registers.h"
#include "../board.h"

/* ---- Private state ------------------------------------------------- */

#define UART_BUF_SIZE 512

static uint8_t  g_rx_buf[UART_BUF_SIZE];
static volatile uint16_t g_rx_head;  /* DMA write position */
static volatile uint16_t g_rx_tail;  /* read position */
static volatile bool g_trigger_word_seen;

/* Snapshot positions for response capture */
static uint16_t g_snapshot_head;

/* ---- Initialization ------------------------------------------------ */

void uart_monitor_init(uint32_t baudrate)
{
    /*
     * Configure USART3 on PD8 (TX) / PD9 (RX) with DMA on RX.
     * The baudrate is configurable to match the target.
     */
    gpio_config(GPIOD, TARGET_UART_TX_PIN, GPIO_MODE_AF, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 7);  /* AF7 for USART3 */
    gpio_config(GPIOD, TARGET_UART_RX_PIN, GPIO_MODE_AF, GPIO_OSPEED_HIGH,
                GPIO_PUPD_PULLUP, 7);

    /* Configure USART3 */
    USART3->CR1 = 0;
    USART3->CR2 = 0;
    USART3->CR3 = USART_CR3_DMAR; /* enable DMA on RX */

    /* Set baudrate: BRR = (SYSCLK / baudrate) for oversample 16.
     * USART3 is on APB1 (138 MHz at 550 MHz SYSCLK). */
    uint32_t usart_clock = 138000000UL;
    USART3->BRR = usart_clock / baudrate;

    /* Enable USART and receiver */
    USART3->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE
                | USART_CR1_IDLEIE; /* enable IDLE interrupt */

    /* Configure DMA1 Stream 1 for USART3 RX */
    DMA1_Stream1->CR = 0;
    DMA1_Stream1->PAR = (uint32_t)&USART3->RDR;
    DMA1_Stream1->M0AR = (uint32_t)g_rx_buf;
    DMA1_Stream1->NDTR = UART_BUF_SIZE;
    DMA1_Stream1->CR = DMA_SxCR_DIR_P2M    /* peripheral to memory */
                     | DMA_SxCR_MINC        /* memory increment */
                     | DMA_SxCR_PSIZE_16    /* actually 8-bit; simplified */
                     | DMA_SxCR_CIRC        /* circular mode */
                     | DMA_SxCR_TCIE        /* transfer complete IRQ */
                     | DMA_SxCR_PRIO_HIGH
                     | DMA_SxCR_EN;         /* enable */

    /* Enable NVIC interrupt for DMA1 Stream1 */
    NVIC_ISER0 |= BIT(DMA1_Stream1_IRQn);
    /* Enable NVIC interrupt for USART3 */
    NVIC_ISER0 |= BIT(USART3_IRQn);

    g_rx_head = 0;
    g_rx_tail = 0;
    g_trigger_word_seen = false;
    g_snapshot_head = 0;
}

/* ---- Reset (clear buffer before a shot) ---------------------------- */

void uart_monitor_reset(void)
{
    g_rx_tail = g_rx_head;
    g_trigger_word_seen = false;
    g_snapshot_head = g_rx_head;
}

/* ---- Get response (read buffer since last reset) ------------------- */

void uart_monitor_get_response(char *buf, uint32_t buf_len)
{
    uint32_t i = 0;
    uint16_t tail = g_rx_tail;

    while (tail != g_rx_head && i < buf_len - 1) {
        buf[i] = (char)g_rx_buf[tail];
        tail = (tail + 1) % UART_BUF_SIZE;
        i++;
    }
    buf[i] = 0; /* null-terminate */
}

/* ---- Trigger word check -------------------------------------------- */

bool uart_monitor_trigger_word_seen(void)
{
    return g_trigger_word_seen;
}

/* ---- USART3 ISR ---------------------------------------------------- */

void uart_monitor_isr(void)
{
    /*
     * Handle USART3 interrupts:
     *  - IDLE flag: the target stopped sending; update head position
     *  - RXNE: byte received (used when DMA is not active)
     *
     * With DMA, the IDLE interrupt is used to detect end of transmission
     * and update the head pointer from the DMA NDTR register.
     */
    if (USART3->ISR & USART_ISR_IDLE) {
        /* Clear IDLE flag by reading ISR then reading RDR */
        (void)USART3->ISR;
        (void)USART3->RDR;

        /* Update head from DMA remaining count */
        uint16_t dma_remaining = DMA1_Stream1->NDTR;
        uint16_t new_head = (UART_BUF_SIZE - dma_remaining) % UART_BUF_SIZE;

        /* Check each new byte for trigger word match */
        uint16_t pos = g_rx_head;
        while (pos != new_head) {
            trigger_check_uart_word(g_rx_buf[pos]);
            pos = (pos + 1) % UART_BUF_SIZE;
        }

        g_rx_head = new_head;
    }
}

/* ---- DMA1 Stream1 ISR (RX transfer complete / half-transfer) ------- */

void uart_monitor_dma_isr(void)
{
    /*
     * DMA circular mode: transfer complete means the DMA wrapped around
     * to the beginning of the buffer. Update head accordingly.
     */
    /* Check transfer complete flag */
    if (DMA1_LISR & BIT(5)) { /* TCIF1 = stream 1 transfer complete */
        /* Clear flag */
        DMA1_LIFCR |= BIT(5);

        /* DMA wrapped to start of buffer */
        uint16_t dma_remaining = DMA1_Stream1->NDTR;
        g_rx_head = (UART_BUF_SIZE - dma_remaining) % UART_BUF_SIZE;
    }

    /* Check half-transfer flag */
    if (DMA1_LISR & BIT(4)) { /* HTIF1 = stream 1 half-transfer */
        DMA1_LIFCR |= BIT(4);
        /* Head is at the half-way point */
        /* g_rx_head = UART_BUF_SIZE / 2; — only if NDTR says so */
    }
}

/* ---- End of file --------------------------------------------------- */