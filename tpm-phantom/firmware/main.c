/*
 * tpm-phantom — firmware/main.c
 * Main firmware for the tpm-phantom TPM bus analyzer.
 *
 * This file initializes all hardware peripherals, runs the main capture
 * loop, processes commands from the wire protocol (USB CDC + BLE), and
 * streams captured TPM transactions to the host and MicroSD card.
 *
 * Author: jayis1
 * License: GPL-2.0
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "board.h"
#include "registers.h"
#include "drivers/lpc_driver.h"
#include "drivers/spi_tpm_driver.h"
#include "drivers/usb_cdc.h"
#include "drivers/ble_bridge.h"
#include "drivers/sd_capture.h"
#include "drivers/wire_protocol.h"
#include <string.h>

/* ===================================================================
 * Global state
 * =================================================================== */
static volatile capture_mode_t g_mode = CAPTURE_MODE_IDLE;
static volatile uint8_t g_capturing = 0;
static volatile uint32_t g_boot_ticks = 0;

/* Address filter */
static volatile uint32_t g_filter_mask = 0;
static volatile uint32_t g_filter_val = 0;
static volatile uint8_t  g_filter_enabled = 0;

/* TX scratch buffer for wire protocol */
static uint8_t g_tx_scratch[300];

/* ===================================================================
 * Forward declarations
 * =================================================================== */
static void clock_init(void);
static void gpio_init(void);
static void handle_wire_cmd(uint8_t cmd, uint8_t *payload, uint16_t len);
static void ble_rx_handler(uint8_t cmd, uint8_t *payload, uint8_t len);
static void stream_transaction_lpc(const lpc_transaction_t *t);
static void stream_transaction_spi(const spi_tpm_transaction_t *t);
static void send_status_periodic(void);
static void watchdog_init(void);
static void watchdog_kick(void);

/* ===================================================================
 * Reset handler (entry point)
 * =================================================================== */
int main(void)
{
    /* Disable interrupts during init */
    __asm__("cpsid i");

    /* Initialize clock tree: HSE 25 MHz → 480 MHz */
    clock_init();

    /* Initialize GPIO (LEDs, button) */
    gpio_init();

    /* Turn on status LED to indicate boot */
    led_status_on();

    /* Initialize timestamp timer */
    lpc_timer_init();

    /* Initialize USB CDC */
    usb_cdc_init();

    /* Initialize BLE bridge */
    ble_bridge_init();
    ble_bridge_set_handler(ble_rx_handler);

    /* Initialize LPC capture engine */
    lpc_capture_init();

    /* Initialize SPI TPM capture engine */
    spi_tpm_capture_init();

    /* Try to init SD card (optional) */
    uint8_t sd_status = sd_capture_init();
    if (sd_status != SD_OK) {
        led_error_blink();
    }

    /* Initialize wire protocol parser */
    wire_protocol_init(handle_wire_cmd);

    /* Initialize watchdog (8 second timeout) */
    watchdog_init();

    /* Enable interrupts */
    __asm__("cpsie i");

    /* Boot blink pattern */
    for (int i = 0; i < 3; i++) {
        led_status_on();
        for (volatile int d = 0; d < 500000; d++) __asm__("nop");
        led_status_off();
        for (volatile int d = 0; d < 500000; d++) __asm__("nop");
    }
    led_status_on();

    /* ===================================================================
     * Main loop
     * =================================================================== */
    uint32_t last_status_tick = 0;
    uint32_t loop_count = 0;

    while (1) {
        loop_count++;
        g_boot_ticks++;

        /* Poll USB */
        usb_cdc_poll();

        /* Process any received USB data */
        uint8_t usb_rx[64];
        uint16_t n = usb_cdc_recv(usb_rx, sizeof(usb_rx));
        if (n > 0) {
            for (uint16_t i = 0; i < n; i++)
                wire_protocol_feed(usb_rx[i]);
        }

        /* If capturing, drain transaction queues */
        if (g_capturing) {
            if (g_mode == CAPTURE_MODE_LPC) {
                lpc_transaction_t t;
                while (lpc_pop_transaction(&t)) {
                    if (g_filter_enabled &&
                        ((t.address & g_filter_mask) != g_filter_val))
                        continue;
                    stream_transaction_lpc(&t);
                }
            } else if (g_mode == CAPTURE_MODE_SPI) {
                spi_tpm_transaction_t t;
                while (spi_tpm_pop_transaction(&t)) {
                    if (g_filter_enabled &&
                        ((t.address & g_filter_mask) != (g_filter_val & 0xFFFFFF)))
                        continue;
                    stream_transaction_spi(&t);
                }
            }
        }

        /* Send periodic status (every ~1 second = 10000 ticks @ 10kHz loop) */
        if (g_boot_ticks - last_status_tick > 10000) {
            last_status_tick = g_boot_ticks;
            send_status_periodic();
        }

        /* Kick watchdog */
        if (loop_count % 1000 == 0)
            watchdog_kick();

        /* Button press → toggle capture (standalone mode) */
        if (!gpio_read(BUTTON_PORT, BUTTON_PIN)) {
            /* Debounce */
            for (volatile int d = 0; d < 100000; d++) __asm__("nop");
            if (!gpio_read(BUTTON_PORT, BUTTON_PIN)) {
                if (g_capturing) {
                    g_capturing = 0;
                    lpc_capture_stop();
                    spi_tpm_capture_stop();
                    led_capture_off();
                } else {
                    g_capturing = 1;
                    if (g_mode == CAPTURE_MODE_LPC) {
                        lpc_capture_start();
                    } else if (g_mode == CAPTURE_MODE_SPI) {
                        spi_tpm_capture_start();
                    }
                    led_capture_on();
                }
                /* Wait for release */
                while (!gpio_read(BUTTON_PORT, BUTTON_PIN)) watchdog_kick();
            }
        }
    }

    return 0;
}

/* ===================================================================
 * Clock initialization
 * HSE 25 MHz → PLL1 → SYSCLK 480 MHz, AHB 240, APB1/2 120, APB4 120
 * =================================================================== */
static void clock_init(void)
{
    /* Enable HSE */
    SET_BIT(RCC_CR, RCC_CR_HSEON);
    while (!(RCC_CR & RCC_CR_HSERDY)) ;

    /* Configure voltage scale (VOS0 for 480 MHz) */
    PWR_CR3 |= PWR_CR3_LDOEN;
    PWR_D3CR = (3U << PWR_D3CR_VOS_SHIFT);  /* VOS0 */
    while (!(PWR_D3CR & BIT(13))) ;  /* VOSRDY */

    /* Configure PLL1 */
    RCC_PLLCFGR = 0;
    RCC_PLLCFGR |= (PLL1_M);                   /* M = 5 */
    RCC_PLLCFGR |= (PLL1_N << 8);              /* N = 96 */
    RCC_PLLCFGR |= (0 << 17);                  /* P = 2 (0 → /2) */
    RCC_PLLCFGR |= (1U << 24);                 /* PLL1 input = HSE */
    RCC_PLLCFGR |= BIT(0);                      /* Enable PLL1 */

    SET_BIT(RCC_CR, RCC_CR_PLL1ON);
    while (!(RCC_CR & RCC_CR_PLL1RDY)) ;

    /* Configure bus prescalers */
    RCC_D1CFGR = (0x0 << 0);   /* D1CPRE = /1 (480 MHz CPU) → use /2 for safety */
    RCC_D1CFGR = (8 << 0);     /* HPRE /2 → 240 MHz AHB */
    RCC_D2CFGR = (4 << 0) | (4 << 8);  /* PPRE1 /2, PPRE2 /2 → 120 MHz */
    RCC_D3CFGR = (4 << 0);    /* D3PPRE /2 → 120 MHz */

    /* Set flash latency for 240 MHz AHB (15 wait states for VOS0) */
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | 2;

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (RCC_CFGR & ~(0x7)) | 3;  /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 0x7) != 3) ;

    /* Enable CSI for I/O compensation */
    SET_BIT(RCC_CR, RCC_CR_CSION);
    while (!(RCC_CR & RCC_CR_CSIRDY)) ;
    SET_BIT(SYSCFG_PMCR, BIT(16));  /* enable I/O compensation */
}

/* ===================================================================
 * GPIO initialization for LEDs and button
 * =================================================================== */
static void gpio_init(void)
{
    /* Enable GPIO clocks (A, B, C, D, E) */
    SET_BIT(RCC_AHB4ENR, BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4));

    /* Status LED: PE1 */
    gpio_set_mode(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_OTYPE_PP);
    gpio_set_speed(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_SPEED_LOW);

    /* Capture LED: PE2 */
    gpio_set_mode(LED_CAPTURE_PORT, LED_CAPTURE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(LED_CAPTURE_PORT, LED_CAPTURE_PIN, GPIO_OTYPE_PP);

    /* Error LED: PE3 */
    gpio_set_mode(LED_ERROR_PORT, LED_ERROR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(LED_ERROR_PORT, LED_ERROR_PIN, GPIO_OTYPE_PP);

    /* LPC mode LED: PE4 */
    gpio_set_mode(LED_LPC_PORT, LED_LPC_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(LED_LPC_PORT, LED_LPC_PIN, GPIO_OTYPE_PP);

    /* Button: PC13 (input, pull-up, active low) */
    gpio_set_mode(BUTTON_PORT, BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(BUTTON_PORT, BUTTON_PIN, GPIO_PUPD_PU);
}

/* ===================================================================
 * Wire protocol command handler (from USB and BLE)
 * =================================================================== */
static void handle_wire_cmd(uint8_t cmd, uint8_t *payload, uint16_t len)
{
    switch (cmd) {
    case WIRE_CMD_GET_STATUS: {
        uint32_t total = lpc_total_transactions + spi_tpm_total_transactions;
        uint32_t tpm = lpc_tpm_transactions;
        uint16_t n = wire_pack_status(g_tx_scratch, g_mode, g_capturing, total, tpm);
        usb_cdc_send(g_tx_scratch, n);
        ble_bridge_send(WIRE_RSP_STATUS, &g_tx_scratch[5], n - 7);
        break;
    }

    case WIRE_CMD_START_CAP: {
        if (len < 2) break;
        uint8_t mode = payload[0];
        uint8_t flags = payload[1];
        g_mode = (capture_mode_t)mode;
        g_capturing = 1;

        if (g_mode == CAPTURE_MODE_LPC) {
            lpc_capture_start();
            led_lpc_on();
        } else if (g_mode == CAPTURE_MODE_SPI) {
            spi_tpm_capture_start();
            led_lpc_off();
        }
        led_capture_on();
        break;
    }

    case WIRE_CMD_STOP_CAP: {
        g_capturing = 0;
        lpc_capture_stop();
        spi_tpm_capture_stop();
        led_capture_off();
        break;
    }

    case WIRE_CMD_GET_STATS: {
        wire_stats_t s;
        s.lpc_total = lpc_total_transactions;
        s.lpc_tpm = lpc_tpm_transactions;
        s.spi_total = spi_tpm_total_transactions;
        s.spi_reads = spi_tpm_read_count;
        s.spi_writes = spi_tpm_write_count;
        s.spi_bytes = spi_tpm_bytes_captured;
        s.sd_blocks = sd_capture_blocks_written();
        s.sd_bytes = sd_total_bytes;
        s.lpc_serirq = lpc_serirq_count;
        uint16_t n = wire_pack_stats(g_tx_scratch, &s);
        usb_cdc_send(g_tx_scratch, n);
        ble_bridge_send(WIRE_RSP_STATS, &g_tx_scratch[5], n - 7);
        break;
    }

    case WIRE_CMD_SET_FILTER: {
        if (len < 8) break;
        g_filter_mask = payload[0] | (payload[1] << 8) |
                        (payload[2] << 16) | (payload[3] << 24);
        g_filter_val  = payload[4] | (payload[5] << 8) |
                        (payload[6] << 16) | (payload[7] << 24);
        g_filter_enabled = 1;
        break;
    }

    case WIRE_CMD_INJECT_LPC: {
        if (len < 3) break;
        uint16_t addr = payload[0] | (payload[1] << 8);
        uint8_t data = payload[2];
        lpc_inject_io_write(addr, data);
        break;
    }

    case WIRE_CMD_INJECT_SPI: {
        if (len < 1) break;
        uint8_t dlen = payload[0];
        if (dlen > 64 || len < (uint16_t)(1 + dlen)) break;
        spi_tpm_inject_read_response(&payload[1], dlen);
        break;
    }

    case WIRE_CMD_SET_LED: {
        if (len < 2) break;
        uint8_t led = payload[0];
        uint8_t state = payload[1];
        switch (led) {
        case 0: state ? led_status_on() : led_status_off(); break;
        case 1: state ? led_capture_on() : led_capture_off(); break;
        case 2: state ? led_error_on() : led_error_off(); break;
        case 3: state ? led_lpc_on() : led_lpc_off(); break;
        }
        break;
    }

    case WIRE_CMD_RESET: {
        lpc_reset_stats();
        spi_tpm_reset_stats();
        sd_capture_reset();
        break;
    }

    case WIRE_CMD_SD_STATUS: {
        uint8_t info[8];
        info[0] = sd_capture_ready();
        uint32_t blocks = sd_capture_blocks_written();
        info[1] = (uint8_t)(blocks & 0xFF);
        info[2] = (uint8_t)(blocks >> 8);
        info[3] = (uint8_t)(blocks >> 16);
        info[4] = (uint8_t)(blocks >> 24);
        uint32_t errs = sd_capture_errors();
        info[5] = (uint8_t)(errs & 0xFF);
        info[6] = (uint8_t)(errs >> 8);
        info[7] = (uint8_t)(errs >> 16);
        uint16_t n = wire_pack(g_tx_scratch, WIRE_RSP_SD_INFO, info, 8);
        usb_cdc_send(g_tx_scratch, n);
        break;
    }

    case WIRE_CMD_FLUSH_SD: {
        sd_capture_flush();
        break;
    }

    case WIRE_CMD_GET_VERSION: {
        uint16_t n = wire_pack_version(g_tx_scratch);
        usb_cdc_send(g_tx_scratch, n);
        ble_bridge_send(WIRE_RSP_VERSION, &g_tx_scratch[5], n - 7);
        break;
    }

    default:
        /* Unknown command — send error */
        g_tx_scratch[0] = cmd;
        g_tx_scratch[1] = 0x01;  /* ERR_UNKNOWN_CMD */
        uint16_t n = wire_pack(g_tx_scratch, WIRE_RSP_ERROR, g_tx_scratch, 2);
        usb_cdc_send(g_tx_scratch, n);
        break;
    }
}

/* ===================================================================
 * BLE RX handler — forwards BLE commands to wire protocol
 * =================================================================== */
static void ble_rx_handler(uint8_t cmd, uint8_t *payload, uint8_t len)
{
    /* BLE frames use same command set; feed into wire handler directly */
    handle_wire_cmd(cmd, payload, len);
}

/* ===================================================================
 * Stream captured LPC transaction to USB, BLE, and SD
 * =================================================================== */
static void stream_transaction_lpc(const lpc_transaction_t *t)
{
    uint16_t n = wire_pack_lpc_tx(g_tx_scratch, t);

    /* Send via USB CDC */
    if (usb_cdc_configured())
        usb_cdc_send(g_tx_scratch, n);

    /* Send via BLE (payload only) */
    ble_bridge_send(WIRE_RSP_TRANSACTION, &g_tx_scratch[5], n - 7);

    /* Log to SD card */
    if (sd_capture_ready())
        sd_capture_append(g_tx_scratch, n);
}

/* ===================================================================
 * Stream captured SPI TPM transaction to USB, BLE, and SD
 * =================================================================== */
static void stream_transaction_spi(const spi_tpm_transaction_t *t)
{
    uint16_t n = wire_pack_spi_tx(g_tx_scratch, t);

    if (usb_cdc_configured())
        usb_cdc_send(g_tx_scratch, n);

    ble_bridge_send(WIRE_RSP_TRANSACTION, &g_tx_scratch[5], n - 7);

    if (sd_capture_ready())
        sd_capture_append(g_tx_scratch, n);
}

/* ===================================================================
 * Send periodic status update
 * =================================================================== */
static void send_status_periodic(void)
{
    uint32_t total = lpc_total_transactions + spi_tpm_total_transactions;
    uint32_t tpm = lpc_tpm_transactions;
    uint16_t n = wire_pack_status(g_tx_scratch, g_mode, g_capturing, total, tpm);
    if (usb_cdc_configured())
        usb_cdc_send(g_tx_scratch, n);
    ble_bridge_send(WIRE_RSP_STATUS, &g_tx_scratch[5], n - 7);
}

/* ===================================================================
 * Independent watchdog (IWDG) — 8 second timeout
 * =================================================================== */
static void watchdog_init(void)
{
    /* Enable LSI (needed for IWDG) */
    SET_BIT(RCC_CSR, BIT(0));  /* LSION — actually in RCC_CSR at offset 0x100 */
    IWDG_KR = 0xCCCC;         /* Enable IWDG */
    IWDG_KR = 0x5555;         /* Allow register access */
    IWDG_PR = 0x4;            /* Prescaler /64 → 625 Hz */
    IWDG_RLR = 0x0FFF;        /* Reload ~6.5 seconds */
    IWDG_KR = 0xAAAA;        /* Reload */
}

static void watchdog_kick(void)
{
    IWDG_KR = 0xAAAA;
}

/* ===================================================================
 * EXTI handlers — dispatch to LPC and SPI TPM drivers
 * In a real build these are mapped via the startup vector table.
 * =================================================================== */

/* EXTI9_5 (covers PA8 LCLK, PA9 LFRAME, PA10 LPCPD#) */
void EXTI9_5_IRQHandler(void)
{
    /* Check LFRAME# (PA9) */
    if (EXTI_PR & BIT(9)) {
        EXTI_PR = BIT(9);
        uint8_t level = gpio_read(LPC_LFRAME_PORT, LPC_LFRAME_PIN);
        lpc_on_lframe_edge(level);
    }
    /* LCLK (PA8) rising edge */
    if (EXTI_PR & BIT(8)) {
        EXTI_PR = BIT(8);
        lpc_on_lclk_rising();
    }
    /* LPCPD# (PA10) */
    if (EXTI_PR & BIT(10)) {
        EXTI_PR = BIT(10);
        /* Power-down event — stop capture */
        g_capturing = 0;
        led_capture_off();
    }
}

/* EXTI15_10 (covers PB12 SPI CS#, PB4 SERIRQ) */
void EXTI15_10_IRQHandler(void)
{
    /* SPI CS# (PB12) */
    if (EXTI_PR & BIT(12)) {
        EXTI_PR = BIT(12);
        uint8_t level = gpio_read(SPI_TPM_CS_PORT, SPI_TPM_CS_PIN);
        spi_tpm_on_cs_edge(level);
    }
    /* SERIRQ (PB4) */
    if (EXTI_PR & BIT(4)) {
        EXTI_PR = BIT(4);
        lpc_on_serirq_edge();
    }
}

/* EXTI register (simplified — actual EXTI is at 0x58000000) */
#define EXTI_IMR1   (*(volatile uint32_t *)(0x58000000 + 0x00))
#define EXTI_EMR1   (*(volatile uint32_t *)(0x58000000 + 0x04))
#define EXTI_RTSR1  (*(volatile uint32_t *)(0x58000000 + 0x08))
#define EXTI_FTSR1  (*(volatile uint32_t *)(0x58000000 + 0x0C))
#define EXTI_SWIER1 (*(volatile uint32_t *)(0x58000000 + 0x10))
#define EXTI_PR     (*(volatile uint32_t *)(0x58000000 + 0x14))

/* ===================================================================
 * System init stub (called from startup before main)
 * =================================================================== */
void SystemInit(void)
{
    /* SCB->CPACR: enable FPU */
    *(volatile uint32_t *)0xE000ED88 = 0x00F00000;
    /* Cache: enable I-Cache and D-Cache */
    *(volatile uint32_t *)0xE000EDFC |= (3U << 16);
}