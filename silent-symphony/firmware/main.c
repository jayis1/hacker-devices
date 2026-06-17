/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Main Entry Point — Scheduler & Command Dispatch
 *
 * Initializes all subsystems, runs the main control loop,
 * and dispatches BLE commands to the appropriate modulation
 * and capture subsystems.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "registers.h"
#include "board.h"
#include "drivers/ultrasonic_driver.h"
#include "drivers/amp_driver.h"
#include "drivers/codec_driver.h"
#include "drivers/ble_bridge.h"
#include "drivers/capture_buffer.h"
#include <string.h>

/* =========================================================================
 * Module Version
 * ========================================================================= */
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0

/* =========================================================================
 * Global State
 * ========================================================================= */

/* Device mode */
static volatile enum device_mode g_mode = MODE_IDLE;
static volatile enum tx_power_level g_tx_power = TX_POWER_MED;
static volatile enum rx_gain_level g_rx_gain = RX_GAIN_MED;

/* I²S audio buffers (ping-pong, aligned for DMA) */
__attribute__((aligned(32)))
static int16_t g_tx_buf0[I2S_TX_BUFFER_SIZE];
__attribute__((aligned(32)))
static int16_t g_tx_buf1[I2S_TX_BUFFER_SIZE];
__attribute__((aligned(32)))
static int16_t g_rx_buf0[I2S_RX_BUFFER_SIZE];
__attribute__((aligned(32)))
static int16_t g_rx_buf1[I2S_RX_BUFFER_SIZE];

/* Modulator/Demodulator states */
static fsk_modulator_t    g_fsk_mod;
static fsk_demodulator_t  g_fsk_dem;
static ook_modulator_t    g_ook_mod;
static ook_demodulator_t  g_ook_dem;
static whisper_modulator_t g_whisper_mod;

/* SysTick counter */
static volatile uint32_t g_systick_ms = 0;
static volatile uint32_t g_uptime_s = 0;

/* =========================================================================
 * Forward declarations
 * ========================================================================= */
static void sys_clock_init(void);
static void gpio_init(void);
static void systick_init(void);
static void nvic_init(void);
static void SystemInit(void);

/* =========================================================================
 * System Clock Initialization
 * =========================================================================
 * 
 * Configure STM32H743 for:
 *   - HSE 25 MHz → PLL1 → 480 MHz CPU / 240 MHz AXI / 120 MHz APB
 *   - I2S clocks from APB2
 * ========================================================================= */
static void sys_clock_init(void)
{
    /* Enable HSI (64 MHz) as intermediate clock source */
    /* HSE bypass: 25 MHz crystal on PH5/PH6 */
    /* For simplicity, we start with HSI and set up PLL. */

    /* Step 1: Configure power supply for VOS1 (high performance) */
    uint32_t pwr_cr3 = mmio_read32(PWR_CR3);
    pwr_cr3 |= (1U << 8) | (1U << 9); /* SCUEN = 1, LDOEN = 1, BYPASS = 0 */
    mmio_write32(PWR_CR3, pwr_cr3);

    /* Wait for regulators ready */
    uint32_t timeout = 10000;
    while (!(mmio_read32(PWR_D3CR) & (1U << 13)) && --timeout) /* VOS ready */
        ;

    /* Select VOS1 (Scale 1 mode, max performance) */
    uint32_t d3cr = mmio_read32(PWR_D3CR);
    d3cr &= ~(3U << 14);   /* Clear VOS bits */
    d3cr |= (1U << 14);     /* VOS = 01 (Scale 1) */
    mmio_write32(PWR_D3CR, d3cr);

    /* Step 2: Configure Flash wait states for 480 MHz */
    /* RM0433: 480 MHz @ VOS1 requires 4 wait states */
    mmio_write32(FLASH_BASE + FLASH_ACR, 0x00000004);  /* 4 WS, ART enabled? */

    /* Step 3: Configure RCC for HSI → PLL → 480 MHz */
    /* HSI = 64 MHz. PLL1_M = 4, PLL1_N = 60, PLL1_P = 2 → VCO = 960, P = 480, Q = 240 */
    /* divM = /4 → 16 MHz VCO input */
    /* N = 60 → 960 MHz VCO */
    /* P = /2 → 480 MHz CPU  */
    /* Q = /4 → 240 MHz AXI  */
    /* R = /4 → 240 MHz */

    /* Wait for HSI ready */
    while (!(mmio_read32(RCC_CR) & (1U << 2))) /* HSIRDY */
        ;

    /* Configure PLL1 */
    /* PLL1CFGR: PLL1SRC = 1 (HSI), PLL1M = 4, PLL1N = 60 */
    uint32_t pll_cfgr = (1U << 27)   /* PLL1SRC = HSI (bit 26? Actually: 00=HSI, 01=HSE for PLL1) */
                       | (4U << 0)   /* PLL1M = 4 (div 4) */
                       | (60U << 8)  /* PLL1N = 60 */
                       | (2U << 24); /* PLL1PEN = enable P division */
    mmio_write32(RCC_BASE + 0x0C, pll_cfgr); /* PLL1DIVR at offset 0x0C */

    /* Wait for VCO ready */
    while (!(mmio_read32(RCC_BASE + 0x0C) & (1U << 31))) /* PLL1RDY? Depends on exact reg */
        ;

    /* Enable PLL1P output */
    mmio_set_bits(RCC_BASE + 0x0C, (1U << 24)); /* PLL1PEN */

    /* Step 4: Switch system clock to PLL1P */
    uint32_t cfgr = mmio_read32(RCC_CFGR);
    cfgr &= ~(7U << 4);  /* Clear SW bits */
    cfgr |= (3U << 4);    /* SW = PLL1 (3) */
    mmio_write32(RCC_CFGR, cfgr);

    /* Wait for clock switch */
    while ((mmio_read32(RCC_CFGR) & (7U << 8)) != (3U << 8)) /* SWS = PLL1 */
        ;

    /* Step 5: Configure AHB, APB prescalers */
    /* AHB = /2 (from 480 → 240), APB1 = /2 (240 → 120), APB2 = /2 (240 → 120) */
    cfgr = mmio_read32(RCC_CFGR);
    cfgr &= ~((7U << 7) | (7U << 4) | (7U << 10) | (7U << 13) | (7U << 16) | (3U << 19));
    cfgr |= (1U << 7)    /* D1CPRE = /2 (AXI = 240 MHz) */
          | (4U << 10)   /* HPRE = /2 (AHB = 240 MHz) */
          | (4U << 13)   /* PPRE1 = /2 (APB1 = 120 MHz) */
          | (4U << 16)   /* PPRE2 = /2 (APB2 = 120 MHz) */
          | (4U << 19);  /* PPRE3 = /2 (APB3 = 120 MHz) */
    mmio_write32(RCC_CFGR, cfgr);

    /* Step 6: Configure I2S clock source */
    /* I2S1 on APB2 (already 120 MHz), I2S3 on APB1H (already 120 MHz) */
}

/* =========================================================================
 * GPIO Initialization
 * ========================================================================= */
static void gpio_init(void)
{
    /* Enable GPIO clocks */
    mmio_set_bits(RCC_AHB4ENR, RCC_AHB4ENR_GPIOA | RCC_AHB4ENR_GPIOB
                              | RCC_AHB4ENR_GPIOC | RCC_AHB4ENR_GPIOE);

    /* --- LED pins (PE0-PE6) as output, push-pull, low speed --- */
    uint32_t moder = 0;
    for (int pin = 0; pin <= 6; pin++)
        moder |= (GPIO_MODE_OUTPUT << (pin * 2));
    mmio_write32(GPIOE_BASE + GPIO_MODER, moder);
    mmio_write32(GPIOE_BASE + GPIO_OTYPER, 0); /* Push-pull */
    mmio_write32(GPIOE_BASE + GPIO_OSPEEDR, (GPIO_SPEED_LOW << 0)  | (GPIO_SPEED_LOW << 2)
                                           | (GPIO_SPEED_LOW << 4)  | (GPIO_SPEED_LOW << 6)
                                           | (GPIO_SPEED_LOW << 8)  | (GPIO_SPEED_LOW << 10)
                                           | (GPIO_SPEED_LOW << 12));
    mmio_write32(GPIOE_BASE + GPIO_PUPDR, 0); /* No pull */

    /* Turn on RGB LED green to indicate startup */
    mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_RGB_G));             /* Green on */
    mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_RGB_R + 16))  /* Red off */
                                         | ((uint32_t)1U << (PIN_RGB_B + 16))); /* Blue off */

    /* Turn off Tx/Rx/Beacon/Fault LEDs */
    mmio_write32(GPIOE_BASE + GPIO_BSRR,
                  ((uint32_t)1U << (PIN_LED_TX + 16))
                | ((uint32_t)1U << (PIN_LED_RX + 16))
                | ((uint32_t)1U << (PIN_LED_BEACON + 16))
                | ((uint32_t)1U << (PIN_LED_FAULT + 16)));

    /* --- Button pins as input with pull-up --- */
    moder = mmio_read32(GPIOA_BASE + GPIO_MODER);
    moder &= ~(3U << (PIN_BTN_PWR * 2));
    moder |= (GPIO_MODE_INPUT << (PIN_BTN_PWR * 2));
    mmio_write32(GPIOA_BASE + GPIO_MODER, moder);

    uint32_t pupdr = mmio_read32(GPIOA_BASE + GPIO_PUPDR);
    pupdr |= (GPIO_PULL_UP << (PIN_BTN_PWR * 2));
    mmio_write32(GPIOA_BASE + GPIO_PUPDR, pupdr);

    moder = mmio_read32(GPIOB_BASE + GPIO_MODER);
    moder &= ~((3U << (PIN_BTN_MODE * 2)) | (3U << (PIN_BTN_CAPT * 2)));
    moder |= (GPIO_MODE_INPUT << (PIN_BTN_MODE * 2))
           | (GPIO_MODE_INPUT << (PIN_BTN_CAPT * 2));
    mmio_write32(GPIOB_BASE + GPIO_MODER, moder);

    pupdr = mmio_read32(GPIOB_BASE + GPIO_PUPDR);
    pupdr |= (GPIO_PULL_UP << (PIN_BTN_MODE * 2))
           | (GPIO_PULL_UP << (PIN_BTN_CAPT * 2));
    mmio_write32(GPIOB_BASE + GPIO_PUPDR, pupdr);

    moder = mmio_read32(GPIOC_BASE + GPIO_MODER);
    moder &= ~(3U << (PIN_BTN_RST * 2));
    moder |= (GPIO_MODE_INPUT << (PIN_BTN_RST * 2));
    mmio_write32(GPIOC_BASE + GPIO_MODER, moder);

    pupdr = mmio_read32(GPIOC_BASE + GPIO_PUPDR);
    pupdr |= (GPIO_PULL_UP << (PIN_BTN_RST * 2));
    mmio_write32(GPIOC_BASE + GPIO_PUPDR, pupdr);
}

/* =========================================================================
 * SysTick Initialization — 1 ms interval
 * ========================================================================= */
static void systick_init(void)
{
    /* SysTick_RVR = CPU_FREQ / 1000 - 1 = 480000 - 1 */
    mmio_write32(SYSTICK_RVR, 480000 - 1);
    mmio_write32(SYSTICK_CVR, 0);
    mmio_write32(SYSTICK_CSR, SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE);
}

/* =========================================================================
 * SysTick Interrupt Handler
 * ========================================================================= */
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    g_systick_ms++;

    /* Update uptime every 1000 ticks */
    if (g_systick_ms >= 1000) {
        g_systick_ms = 0;
        g_uptime_s++;
        g_ble.status.uptime_s = g_uptime_s;
    }
}

/* =========================================================================
 * DMA Interrupt Handlers (I²S Rx — half/transfer complete)
 * ========================================================================= */

/* DMA2 Stream0 (I²S3 Rx) — half transfer */
void DMA2_Stream0_IRQHandler(void) __attribute__((interrupt));
void DMA2_Stream0_IRQHandler(void)
{
    uint32_t dma_base = DMA2_BASE;
    uint8_t stream = 0;
    uint32_t lisr = mmio_read32(dma_base + DMA_LISR);

    /* Half-transfer interrupt */
    if (lisr & (1U << 1)) { /* HTIF0 */
        /* Process first half of buffer */
        if (g_mode >= MODE_RX_FSK && g_mode <= MODE_RX_WHISPER) {
            float samples[256];
            for (int i = 0; i < I2S_RX_BUFFER_SIZE; i++) {
                samples[i] = (float)g_rx_buf0[i] / 32768.0f;
            }

            if (g_mode == MODE_RX_FSK) {
                for (int i = 0; i < I2S_RX_BUFFER_SIZE; i++)
                    fsk_demod_process(&g_fsk_dem, samples[i]);
            } else if (g_mode == MODE_RX_OOK) {
                for (int i = 0; i < I2S_RX_BUFFER_SIZE; i++)
                    ook_demod_process(&g_ook_dem, samples[i]);
            }

            /* Store raw data to PSRAM if capture active */
            if (g_capture_ring.active) {
                /* Simple copy to PSRAM ring buffer */
                uint32_t write_idx = g_capture_ring.write_index;
                uint32_t copy_bytes = I2S_RX_BUFFER_SIZE * 2; /* 16-bit samples */
                if (write_idx + copy_bytes < PSRAM_CAPTURE_SIZE * 2) {
                    memcpy((uint8_t *)(PSRAM_CAPTURE_BASE) + write_idx,
                           g_rx_buf0, copy_bytes);
                    g_capture_ring.write_index = write_idx + copy_bytes;
                    g_capture_ring.bytes_filled += copy_bytes;
                } else {
                    g_capture_ring.overflow = 1;
                }
            }

            /* Blink Rx LED */
            mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_LED_RX));
        }

        mmio_write32(dma_base + DMA_LIFCR, (1U << 1)); /* Clear HTIF0 */
    }

    /* Transfer complete interrupt */
    if (lisr & (1U << 0)) { /* TCIF0 */
        /* Process second half of buffer */
        if (g_mode >= MODE_RX_FSK && g_mode <= MODE_RX_WHISPER) {
            float samples[256];
            for (int i = 0; i < I2S_RX_BUFFER_SIZE; i++) {
                samples[i] = (float)g_rx_buf1[i] / 32768.0f;
            }

            if (g_mode == MODE_RX_FSK) {
                for (int i = 0; i < I2S_RX_BUFFER_SIZE; i++)
                    fsk_demod_process(&g_fsk_dem, samples[i]);
            } else if (g_mode == MODE_RX_OOK) {
                for (int i = 0; i < I2S_RX_BUFFER_SIZE; i++)
                    ook_demod_process(&g_ook_dem, samples[i]);
            }

            if (g_capture_ring.active) {
                uint32_t write_idx = g_capture_ring.write_index;
                uint32_t copy_bytes = I2S_RX_BUFFER_SIZE * 2;
                if (write_idx + copy_bytes < PSRAM_CAPTURE_SIZE * 2) {
                    memcpy((uint8_t *)(PSRAM_CAPTURE_BASE) + write_idx,
                           g_rx_buf1, copy_bytes);
                    g_capture_ring.write_index = write_idx + copy_bytes;
                    g_capture_ring.bytes_filled += copy_bytes;
                } else {
                    g_capture_ring.overflow = 1;
                }
            }
        }

        mmio_write32(dma_base + DMA_LIFCR, (1U << 0)); /* Clear TCIF0 */
    }

    /* Error interrupt */
    if (lisr & (1U << 3)) { /* TEIF0 */
        mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_LED_FAULT));
        mmio_write32(dma_base + DMA_LIFCR, (1U << 3));
    }
}

/* DMA2 Stream1 (I²S1 Tx) */
void DMA2_Stream1_IRQHandler(void) __attribute__((interrupt));
void DMA2_Stream1_IRQHandler(void)
{
    uint32_t dma_base = DMA2_BASE;
    uint32_t hisr = mmio_read32(dma_base + DMA_HISR);

    /* Half-transfer (fill first half) */
    if (hisr & (1U << 1)) { /* HTIF1 */
        if (g_mode == MODE_TX_FSK) {
            for (int i = 0; i < I2S_TX_BUFFER_SIZE; i++)
                g_tx_buf0[i] = (int16_t)(fsk_mod_tick(&g_fsk_mod) * 32767.0f);
        } else if (g_mode == MODE_TX_OOK) {
            for (int i = 0; i < I2S_TX_BUFFER_SIZE; i++)
                g_tx_buf0[i] = (int16_t)(ook_mod_tick(&g_ook_mod) * 32767.0f);
        } else if (g_mode == MODE_TX_WHISPER) {
            for (int i = 0; i < I2S_TX_BUFFER_SIZE; i++)
                g_tx_buf0[i] = (int16_t)(whisper_mod_tick(&g_whisper_mod) * 32767.0f);
        }
        mmio_write32(dma_base + DMA_HIFCR, (1U << 1));
    }

    /* Transfer complete (fill second half) */
    if (hisr & (1U << 0)) { /* TCIF1 */
        if (g_mode == MODE_TX_FSK) {
            for (int i = 0; i < I2S_TX_BUFFER_SIZE; i++)
                g_tx_buf1[i] = (int16_t)(fsk_mod_tick(&g_fsk_mod) * 32767.0f);
            if (fsk_mod_is_done(&g_fsk_mod)) {
                g_mode = MODE_IDLE;
                mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_LED_TX + 16)));
            }
        } else if (g_mode == MODE_TX_OOK) {
            for (int i = 0; i < I2S_TX_BUFFER_SIZE; i++)
                g_tx_buf1[i] = (int16_t)(ook_mod_tick(&g_ook_mod) * 32767.0f);
            if (ook_mod_is_done(&g_ook_mod)) {
                g_mode = MODE_IDLE;
                mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_LED_TX + 16)));
            }
        } else if (g_mode == MODE_TX_WHISPER) {
            for (int i = 0; i < I2S_TX_BUFFER_SIZE; i++)
                g_tx_buf1[i] = (int16_t)(whisper_mod_tick(&g_whisper_mod) * 32767.0f);
            if (whisper_mod_is_done(&g_whisper_mod)) {
                g_mode = MODE_IDLE;
                mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_LED_TX + 16)));
            }
        }

        /* Blink Tx LED */
        mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_LED_TX));

        mmio_write32(dma_base + DMA_HIFCR, (1U << 0));
    }

    if (hisr & (1U << 3)) { /* TEIF1 */
        mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_LED_FAULT));
        mmio_write32(dma_base + DMA_HIFCR, (1U << 3));
    }
}

/* =========================================================================
 * USART5 Interrupt Handler — BLE UART data
 * ========================================================================= */
void USART5_IRQHandler(void) __attribute__((interrupt));
void USART5_IRQHandler(void)
{
    uint32_t isr = mmio_read32(USART5_BASE + USART_ISR);

    if (isr & USART_ISR_RXNE) {
        /* Byte handled in main loop via ble_bridge_process_rx() */
        /* Just clear flag by reading RDR */
        (void)mmio_read32(USART5_BASE + USART_RDR);
    }

    /* Error handling */
    if (isr & (USART_ISR_FE | USART_ISR_PE)) {
        uint32_t icr = 0;
        if (isr & USART_ISR_FE) icr |= (1U << 6);  /* FECF */
        if (isr & USART_ISR_PE) icr |= (1U << 7);  /* PECF */
        mmio_write32(USART5_BASE + USART_ICR, icr);
    }
}

/* =========================================================================
 * NVIC Configuration
 * ========================================================================= */
static void nvic_init(void)
{
    /* Set interrupt priority groups (4 bits, no sub-priority) */
    uint32_t aircr = mmio_read32(SCB_AIRCR);
    aircr = (0x5FAUL << 16) | (3U << 8); /* PRIGROUP = 3 (bits 8-10): 4 bits for preempt, 0 for sub */
    mmio_write32(SCB_AIRCR, aircr);

    /* Enable interrupts */
    NVIC_ISER(1) = (1U << (IRQ_DMA2_STREAM0 % 32)) | (1U << (IRQ_DMA2_STREAM1 % 32));
    NVIC_ISER(1) |= (1U << (IRQ_USART5 % 32));
    NVIC_ISER(1) |= (1U << (IRQ_TIM3 % 32)) | (1U << (IRQ_TIM4 % 32));

    /* Set priorities */
    /* I²S Rx DMA: priority 0 (highest) */
    NVIC_IPR(IRQ_DMA2_STREAM0) = (NVIC_IPR(IRQ_DMA2_STREAM0) & ~0xFF) | (0 << 4);
    /* I²S Tx DMA: priority 1 */
    NVIC_IPR(IRQ_DMA2_STREAM1) = (NVIC_IPR(IRQ_DMA2_STREAM1) & ~0xFF) | (1 << 4);
    /* SysTick: priority 2 */
    /* Already set by systick_init via SHPR3 */
    mmio_write32(SCB_SHPR3, (mmio_read32(SCB_SHPR3) & ~0xFF000000) | (3 << 22));
    /* USART5: priority 3 */
    NVIC_IPR(IRQ_USART5) = (NVIC_IPR(IRQ_USART5) & ~0xFF) | (3 << 4);
}

/* =========================================================================
 * SystemInit — called by startup code
 * ========================================================================= */
void SystemInit(void)
{
    /* Called by the startup file before _start */
    /* Minimal: FPU enable */
    uint32_t cpacr = 0xE000ED88;
    mmio_write32(cpacr, 0x00F00000); /* CP10, CP11 full access */

    /* Enable ICACHE and DCACHE if available */
    /* For Cortex-M7: enable I/D cache via system control block */
}

/* =========================================================================
 * Delay (busy-wait, in milliseconds)
 * ========================================================================= */
static void delay_ms(uint32_t ms)
{
    uint32_t start = g_systick_ms;
    uint32_t remaining = ms;
    
    while (remaining > 0) {
        uint32_t current = g_systick_ms;
        if (current != start) {
            remaining--;
            start = current;
        }
        __asm__ volatile("wfi"); /* Sleep between ticks */
    }
}

/* =========================================================================
 * BLE Command Processor
 * ========================================================================= */
static void process_ble_commands(void)
{
    if (!ble_bridge_cmd_pending())
        return;

    uint16_t cmd;
    uint8_t payload[BLE_MAX_PAYLOAD];
    uint16_t payload_len;

    if (ble_bridge_get_cmd(&cmd, payload, &payload_len) != 0)
        return;

    switch (cmd) {
    case BLE_CMD_GET_STATUS: {
        /* Update battery level from ADC (placeholder) */
        /* In production, read internal ADC channel connected to battery divider */
        g_ble.status.battery_mv = 3900; /* Placeholder reading */
        g_ble.status.battery_pct = (g_ble.status.battery_mv - BATTERY_EMPTY_MV)
                                 * 100U / (BATTERY_FULL_MV - BATTERY_EMPTY_MV);
        if (g_ble.status.battery_pct > 100) g_ble.status.battery_pct = 100;
        
        g_ble.status.mode = (uint8_t)g_mode;
        g_ble.status.storage_used = capture_ring_get_size();
        
        ble_bridge_send_status();
        break;
    }

    case BLE_CMD_SET_MODE: {
        if (payload_len < 1) break;
        uint8_t new_mode = payload[0];
        
        /* Stop current operation */
        if (g_mode >= MODE_TX_FSK && g_mode <= MODE_TX_WHISPER) {
            i2s_tx_dma_stop();
            mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_LED_TX + 16)));
        }
        if (g_mode >= MODE_RX_FSK && g_mode <= MODE_RX_WHISPER) {
            i2s_rx_dma_stop();
            capture_ring_stop();
            mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_LED_RX + 16)));
        }

        g_mode = (enum device_mode)new_mode;

        switch (g_mode) {
        case MODE_TX_FSK:
            amp_set_tx_power(g_tx_power);
            i2s_tx_dma_start(g_tx_buf0, g_tx_buf1, I2S_TX_BUFFER_SIZE);
            break;

        case MODE_TX_OOK:
            amp_set_tx_power(g_tx_power);
            i2s_tx_dma_start(g_tx_buf0, g_tx_buf1, I2S_TX_BUFFER_SIZE);
            break;

        case MODE_TX_WHISPER:
            amp_set_tx_power(TX_POWER_WHISPER);
            i2s_tx_dma_start(g_tx_buf0, g_tx_buf1, I2S_TX_BUFFER_SIZE);
            break;

        case MODE_RX_FSK:
        case MODE_RX_OOK:
        case MODE_RX_WHISPER:
            amp_set_rx_gain(g_rx_gain);
            capture_ring_start();
            i2s_rx_dma_start(g_rx_buf0, g_rx_buf1, I2S_RX_BUFFER_SIZE);
            break;

        case MODE_IDLE:
        default:
            g_mode = MODE_IDLE;
            break;
        }

        /* Update BLE status */
        g_ble.status.mode = (uint8_t)g_mode;
        ble_bridge_send_status();
        break;
    }

    case BLE_CMD_SET_FSK_PARAMS: {
        if (payload_len < 8) break;
        float f_mark  = (float)(payload[0] | ((uint16_t)payload[1] << 8));
        float f_space = (float)(payload[2] | ((uint16_t)payload[3] << 8));
        uint32_t baud = payload[4] | ((uint16_t)payload[5] << 8);
        float amp     = (float)payload[6] / 100.0f;

        fsk_mod_init(&g_fsk_mod, f_mark, f_space, baud, amp, I2S_TX_SAMPLE_RATE);
        fsk_demod_init(&g_fsk_dem, f_mark, f_space, baud, I2S_RX_SAMPLE_RATE);
        break;
    }

    case BLE_CMD_SET_OOK_PARAMS: {
        if (payload_len < 6) break;
        float carrier = (float)(payload[0] | ((uint16_t)payload[1] << 8));
        uint32_t bit_us = payload[2] | ((uint16_t)payload[3] << 8) | ((uint32_t)payload[4] << 16);
        float amp = (float)payload[5] / 100.0f;

        ook_mod_init(&g_ook_mod, carrier, bit_us, amp, I2S_TX_SAMPLE_RATE);
        ook_demod_init(&g_ook_dem, carrier, bit_us, I2S_RX_SAMPLE_RATE);
        break;
    }

    case BLE_CMD_TX_MESSAGE: {
        if (payload_len < 1 || payload_len > MAX_MESSAGE_LEN) break;

        if (g_mode == MODE_TX_FSK) {
            fsk_mod_load_message(&g_fsk_mod, payload, payload_len);
        } else if (g_mode == MODE_TX_OOK) {
            ook_mod_load_message(&g_ook_mod, payload, payload_len);
        } else if (g_mode == MODE_TX_WHISPER) {
            whisper_mod_load_message(&g_whisper_mod, payload, payload_len);
        }
        break;
    }

    case BLE_CMD_RX_START: {
        /* Already handled by SET_MODE; if already in Rx mode, ensure capture */
        if (g_mode >= MODE_RX_FSK && g_mode <= MODE_RX_WHISPER) {
            capture_ring_start();
        }
        break;
    }

    case BLE_CMD_RX_STOP: {
        capture_ring_stop();
        break;
    }

    case BLE_CMD_RX_DATA: {
        /* Send captured/demodulated data */
        if (fsk_demod_frame_ready(&g_fsk_dem)) {
            uint8_t data[128];
            uint16_t len;
            if (fsk_demod_get_frame(&g_fsk_dem, data, &len) == 0) {
                ble_bridge_respond(BLE_CMD_RX_DATA, data, len);
                fsk_demod_clear_frame(&g_fsk_dem);
            }
        } else if (ook_demod_frame_ready(&g_ook_dem)) {
            uint8_t data[128];
            uint16_t len;
            if (ook_demod_get_frame(&g_ook_dem, data, &len) == 0) {
                ble_bridge_respond(BLE_CMD_RX_DATA, data, len);
                ook_demod_clear_frame(&g_ook_dem);
            }
        } else {
            /* No data available */
            uint8_t empty = 0;
            ble_bridge_respond(BLE_CMD_RX_DATA, &empty, 0);
        }
        break;
    }

    case BLE_CMD_RX_SPECTRUM: {
        /* Compute power spectrum across 18-45 kHz in 256 bins */
        /* Uses current Rx buffer to compute energy distribution */
        uint8_t spectrum[256];
        float bin_width = (45000.0f - 18000.0f) / 256.0f;

        for (int i = 0; i < 256; i++) {
            float freq = 18000.0f + bin_width * (float)i;
            goertzel_state_t gs;
            goertzel_init(&gs, freq, GOERTZEL_N, I2S_RX_SAMPLE_RATE);
            
            for (int j = 0; j < GOERTZEL_N; j++) {
                goertzel_process(&gs, (float)g_rx_buf0[j % I2S_RX_BUFFER_SIZE] / 32768.0f);
            }
            
            float energy = goertzel_get_energy(&gs);
            /* Convert to dB-like scale: val = 255 * min(1, energy * scale) */
            float db_val = 10.0f * log10f(energy + 1e-10f);
            db_val = (db_val + 60.0f) / 60.0f;  /* Normalize -60 to 0 dB → 0 to 1 */
            if (db_val < 0.0f) db_val = 0.0f;
            if (db_val > 1.0f) db_val = 1.0f;
            spectrum[i] = (uint8_t)(db_val * 255.0f);
        }

        ble_bridge_respond(BLE_CMD_RX_SPECTRUM, spectrum, 256);
        break;
    }

    case BLE_CMD_RX_SIGNAL_QUALITY: {
        uint8_t quality[4];
        float snr = fsk_demod_get_snr(&g_fsk_dem);
        uint8_t snr_val = (uint8_t)((snr + 30.0f) * 255.0f / 60.0f); /* Map -30 to +30 dB */
        if (snr_val > 255) snr_val = 255;
        quality[0] = snr_val;
        quality[1] = (uint8_t)g_tx_power;
        quality[2] = (uint8_t)g_rx_gain;
        quality[3] = g_ble.status.signal_quality;
        ble_bridge_respond(BLE_CMD_RX_SIGNAL_QUALITY, quality, 4);
        break;
    }

    case BLE_CMD_SET_POWER: {
        if (payload_len < 1) break;
        g_tx_power = (enum tx_power_level)payload[0];
        amp_set_tx_power(g_tx_power);
        g_ble.status.tx_power = (uint8_t)g_tx_power;
        break;
    }

    case BLE_CMD_SET_GAIN: {
        if (payload_len < 1) break;
        g_rx_gain = (enum rx_gain_level)payload[0];
        amp_set_rx_gain(g_rx_gain);
        g_ble.status.rx_gain = (uint8_t)g_rx_gain;
        break;
    }

    case BLE_CMD_STORE_CAPTURE: {
        uint8_t mod_type = (payload_len > 0) ? payload[0] : 0;
        uint8_t quality  = (payload_len > 1) ? payload[1] : 50;
        int slot = capture_save_to_flash(mod_type, quality);
        if (slot >= 0) {
            uint8_t resp[1] = {(uint8_t)slot};
            ble_bridge_respond(BLE_CMD_STORE_CAPTURE, resp, 1);
        } else {
            ble_bridge_respond(BLE_CMD_STORE_CAPTURE, NULL, 0);
        }
        break;
    }

    case BLE_CMD_RETRIEVE_CAPTURE: {
        if (payload_len < 1) break;
        uint8_t slot = payload[0];
        uint8_t data[2048];
        uint32_t len;
        if (capture_load_from_flash(slot, data, &len) == 0) {
            /* Send in chunks if needed */
            ble_bridge_respond(BLE_CMD_RETRIEVE_CAPTURE, data, len > BLE_MAX_PAYLOAD ? BLE_MAX_PAYLOAD : (uint16_t)len);
        } else {
            ble_bridge_respond(BLE_CMD_RETRIEVE_CAPTURE, NULL, 0);
        }
        break;
    }

    case BLE_CMD_ERASE_STORAGE: {
        /* Only erase capture data, not config */
        capture_ring_stop();
        qspi_erase_block_64k(CAPTURE_PARTITION_DATA);
        /* Clear metadata */
        uint8_t zeros[CAPTURE_METADATA_ENTRY_SIZE];
        memset(zeros, 0, sizeof(zeros));
        for (int i = 0; i < CAPTURE_MAX_SLOTS; i++) {
            uint32_t addr = CAPTURE_PARTITION_METADATA + (i * CAPTURE_METADATA_ENTRY_SIZE);
            qspi_erase_sector(addr);
            qspi_write(addr, zeros, sizeof(zeros));
        }
        ble_bridge_respond(BLE_CMD_ERASE_STORAGE, NULL, 0);
        break;
    }

    case BLE_CMD_RESET: {
        /* System reset */
        uint32_t aircr = mmio_read32(SCB_AIRCR);
        mmio_write32(SCB_AIRCR, 0x5FA << 16 | SCB_AIRCR_SYSRESETREQ);
        while (1) ;
        break;
    }

    default:
        /* Unknown command — ignore */
        break;
    }

    ble_bridge_cmd_consumed();
}

/* =========================================================================
 * Main Entry Point
 * ========================================================================= */
void _start(void)
{
    /* === System Initialization === */
    sys_clock_init();
    systick_init();
    gpio_init();
    nvic_init();

    /* Initialize green LED — indicate boot in progress */
    mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_RGB_G));
    mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_RGB_R + 16)));
    mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_RGB_B + 16)));

    /* === Peripheral Init === */
    /* Initialize amplifier controls */
    amp_max98357a_init();
    amp_ad8694_init();

    /* Initialize codec */
    struct codec_config codec_cfg = WM8731_CONFIG_INIT;
    codec_cfg.sample_rate = I2S_TX_SAMPLE_RATE;
    codec_init(&codec_cfg, I2S_TX_SAMPLE_RATE);

    /* Initialize I²S audio streams */
    i2s_tx_init(I2S_TX_SAMPLE_RATE, 16, 1);
    i2s_rx_init(I2S_RX_SAMPLE_RATE, 16);

    /* Initialize BLE bridge */
    ble_bridge_init();

    /* Initialize storage */
    qspi_init();
    psram_init();

    /* Initialize modulators/demodulators with defaults */
    fsk_mod_init(&g_fsk_mod, FSK_FREQ_MARK, FSK_FREQ_SPACE, FSK_BAUD_DEFAULT, 0.8f, I2S_TX_SAMPLE_RATE);
    fsk_demod_init(&g_fsk_dem, FSK_FREQ_MARK, FSK_FREQ_SPACE, FSK_BAUD_DEFAULT, I2S_RX_SAMPLE_RATE);
    ook_mod_init(&g_ook_mod, OOK_CARRIER_FREQ, OOK_BIT_DURATION_US, 0.8f, I2S_TX_SAMPLE_RATE);
    ook_demod_init(&g_ook_dem, OOK_CARRIER_FREQ, OOK_BIT_DURATION_US, I2S_RX_SAMPLE_RATE);
    whisper_mod_init(&g_whisper_mod, 0.6f, I2S_TX_SAMPLE_RATE, 3.0f);

    /* Pre-charge Tx and Rx buffers with silence */
    memset(g_tx_buf0, 0, sizeof(g_tx_buf0));
    memset(g_tx_buf1, 0, sizeof(g_tx_buf1));
    memset(g_rx_buf0, 0, sizeof(g_rx_buf0));
    memset(g_rx_buf1, 0, sizeof(g_rx_buf1));

    /* Set default power and gain */
    amp_set_tx_power(TX_POWER_MED);
    amp_set_rx_gain(RX_GAIN_MED);

    /* Activate pre-amp for Rx */
    amp_ad8694_set_shutdown(0);

    /* === Boot Complete === */
    /* Turn green LED solid to indicate ready */
    /* (Already on from init) */

    /* === Main Loop === */
    while (1) {
        /* Process BLE RX data */
        ble_bridge_process_rx();

        /* Process any pending BLE commands */
        process_ble_commands();

        /* Check for received frames */
        if (fsk_demod_frame_ready(&g_fsk_dem)) {
            /* Notify app via BLE */
            uint8_t data[128];
            uint16_t len;
            if (fsk_demod_get_frame(&g_fsk_dem, data, &len) == 0) {
                ble_bridge_send(BLE_CMD_RX_DATA, data, len);
                fsk_demod_clear_frame(&g_fsk_dem);
            }
        }

        if (ook_demod_frame_ready(&g_ook_dem)) {
            uint8_t data[128];
            uint16_t len;
            if (ook_demod_get_frame(&g_ook_dem, data, &len) == 0) {
                ble_bridge_send(BLE_CMD_RX_DATA, data, len);
                ook_demod_clear_frame(&g_ook_dem);
            }
        }

        /* Periodic status update (every 1000 ms) */
        static uint32_t last_status_ms = 0;
        if (g_systick_ms - last_status_ms >= 1000) {
            last_status_ms = g_systick_ms;
            ble_bridge_update_status();

            /* Update battery LED color based on level */
            if (g_ble.status.battery_pct < 15) {
                /* Red = low battery */
                mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_RGB_R));
                mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_RGB_G + 16)));
            } else {
                /* Green = OK */
                mmio_write32(GPIOE_BASE + GPIO_BSRR, (1U << PIN_RGB_G));
                mmio_write32(GPIOE_BASE + GPIO_BSRR, ((uint32_t)1U << (PIN_RGB_R + 16)));
            }

            /* Send periodic status updates to BLE (every 5 seconds) */
            static uint32_t ble_status_interval = 0;
            ble_status_interval++;
            if (ble_status_interval >= 5) {
                ble_status_interval = 0;
                ble_bridge_send_status();
            }
        }

        /* Check for mode button press (debounced) */
        static uint32_t last_btn_ms = 0;
        if (g_systick_ms - last_btn_ms >= 200) { /* 200ms debounce */
            uint32_t btn_mode = mmio_read32(GPIOB_BASE + GPIO_IDR);
            if (!(btn_mode & (1U << PIN_BTN_MODE))) {
                /* Mode button pressed — cycle through modes */
                g_mode++;
                if (g_mode > MODE_SCAN) g_mode = MODE_IDLE;
                g_ble.status.mode = (uint8_t)g_mode;
                last_btn_ms = g_systick_ms;
            }

            /* Capture button */
            uint32_t btn_capt = mmio_read32(GPIOB_BASE + GPIO_IDR);
            if (!(btn_capt & (1U << PIN_BTN_CAPT))) {
                if (g_capture_ring.active) {
                    capture_ring_stop();
                } else if (g_mode >= MODE_RX_FSK && g_mode <= MODE_RX_WHISPER) {
                    capture_ring_start();
                }
                last_btn_ms = g_systick_ms;
            }
        }

        /* Sleep when idle */
        if (g_mode == MODE_IDLE) {
            __asm__ volatile("wfi");
        }
    }
}