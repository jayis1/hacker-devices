/*
 * main.c — Joule-Phantom firmware entry point and main loop.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Joule-Phantom is an inline smart-battery SMBus/PMBus MITM implant.
 * It sits between a host system (laptop / drone / IoT controller) and
 * its smart battery pack, transparently bridging the SMBus link while
 * capturing, modifying and injecting SBS telemetry.
 *
 * Architecture:
 *
 *   ┌────────┐  SMBus   ┌──────────────┐  SMBus   ┌──────────┐
 *   │  HOST  │◄────────►│ Joule-Phantom│◄────────►│ BATTERY  │
 *   └────────┘  (I2C1)  └──────┬───────┘  (I2C2)  └──────────┘
 *                              │ UART
 *                              ▼
 *                        ┌──────────┐   BLE GATT
 *                        │ ESP32-C3 │◄──────────► Operator App
 *                        └──────────┘
 *
 * Main loop responsibilities:
 *   1. Bridge host-initiated SMBus transactions to the battery.
 *   2. Apply MITM rules to responses.
 *   3. Capture all frames into a ring buffer.
 *   4. Stream captured frames over BLE to the operator app.
 *   5. Process inbound BLE commands (rules, mode, inject, glitch).
 *   6. Run safety interlocks (temp / voltage / current ceilings).
 *   7. Heartbeat LED (covert, infrequent).
 */

#include "board.h"
#include "registers.h"
#include "smbus_port.h"
#include "ble_link.h"
#include "spoof_engine.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Global tick counter (1 ms)                                        */
/* ------------------------------------------------------------------ */
static volatile uint32_t g_ms_tick = 0;

extern void TIM2_IRQHandler(void);
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR = ~TIM_SR_UIF;
        g_ms_tick++;
    }
}

uint32_t board_millis(void) { return g_ms_tick; }

void board_delay_ms(uint32_t ms)
{
    uint32_t start = g_ms_tick;
    while ((g_ms_tick - start) < ms) { /* WFI in real silicon */ }
}

/* ------------------------------------------------------------------ */
/*  Clock + peripheral initialisation                                 */
/* ------------------------------------------------------------------ */
static void clock_init(void)
{
    /* Enable HSI16 */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY)) { }

    /* Configure PLL: source = HSI16, M=1, N=10, R=2 -> 80 MHz */
    RCC_PLLCFGR = (1u << 0)        /* PLLSRC = HSI16 */
                | (0u << 4)        /* PLLM = 1 */
                | (10u << 8)       /* PLLN = 10 */
                | (0u << 25);      /* PLLR = 2 */
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY)) { }

    /* Flash latency 4 wait states for 80 MHz (set via FLASH ACER) */
    /* (Simplified: in real CMSIS this is FLASH->ACR |= 4)         */

    /* Switch SYSCLK to PLL */
    RCC_CFGR = (3u << 0);   /* SW = PLL */
    while (((RCC_CFGR >> 2) & 0x3) != 3) { }   /* wait SWS = PLL */
}

static void gpio_init(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;

    /* I2C1 pins: PA6=SCL, PA7=SDA -> AF4, open-drain, pull-up */
    GPIOA->MODER  &= ~((3u << 12) | (3u << 14));
    GPIOA->MODER  |=  (GPIO_MODE_AF << 12) | (GPIO_MODE_AF << 14);
    GPIOA->OTYPER |=  (GPIO_OTYPE_OD << 6) | (GPIO_OTYPE_OD << 7);
    GPIOA->PUPDR  |=  (GPIO_PUPD_PU << 12) | (GPIO_PUPD_PU << 14);
    GPIOA->AFRL   &= ~((0xFu << 24) | (0xFu << 28));
    GPIOA->AFRL   |=  ((GPIO_AF4 << 24) | (GPIO_AF4 << 28));

    /* I2C2 pins: PB13=SCL, PB14=SDA -> AF4, open-drain, pull-up */
    GPIOB->MODER  &= ~((3u << 26) | (3u << 28));
    GPIOB->MODER  |=  (GPIO_MODE_AF << 26) | (GPIO_MODE_AF << 28);
    GPIOB->OTYPER |=  (GPIO_OTYPE_OD << 13) | (GPIO_OTYPE_OD << 14);
    GPIOB->PUPDR  |=  (GPIO_PUPD_PU << 26) | (GPIO_PUPD_PU << 28);
    GPIOB->AFRH   &= ~((0xFu << 4) | (0xFu << 8));
    GPIOB->AFRH   |=  ((GPIO_AF4 << 4) | (GPIO_AF4 << 8));

    /* Status LED: PB15 -> output push-pull, low speed */
    GPIOB->MODER  &= ~(3u << 30);
    GPIOB->MODER  |=  (GPIO_MODE_OUTPUT << 30);
    GPIOB->OTYPER &= ~(1u << 15);
    GPIOB->PUPDR  &= ~(3u << 30);

    /* Battery present: PA0 -> input pull-up */
    GPIOA->MODER &= ~(3u << 0);
    GPIOA->PUPDR |= (GPIO_PUPD_PU << 0);

    /* SMBALERT host: PA1 -> input pull-up */
    GPIOA->MODER &= ~(3u << 2);
    GPIOA->PUPDR |= (GPIO_PUPD_PU << 2);

    /* Glitch output: PA5 -> output push-pull */
    GPIOA->MODER &= ~(3u << 10);
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << 10);
    GPIOA->OTYPER &= ~(1u << 5);

    /* ADC sense pins: PA4 (VBAT), PA8 (therm) -> analog */
    GPIOA->MODER &= ~((3u << 8) | (3u << 16));
    GPIOA->MODER |=  (GPIO_MODE_ANALOG << 8) | (GPIO_MODE_ANALOG << 16);
}

static void systick_init(void)
{
    /* TIM2 as 1 ms tick: PCLK=80 MHz, PSC=79999 -> 1 kHz */
    RCC_APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    TIM2->PSC = 7999;
    TIM2->ARR = 10;
    TIM2->DIER = TIM_DIER_UIE;
    TIM2->CR1 = TIM_CR1_CEN;
    NVIC_ISER(0) = (1u << IRQ_TIM2);
}

static void adc_init(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_ADCEN;
    /* Minimal ADC enable; real silicon needs deep calibration sequence.
     * We provide a working placeholder that enables and starts. */
    ADC_CR |= (1u << 28);   /* ADVREGEN enable */
    board_delay_ms(1);
    ADC_CR |= (1u << 31);   /* ADCAL start calibration */
    while (ADC_CR & (1u << 31)) { }
    ADC_CR |= (1u << 0);    /* ADEN */
    while (!(ADC_ISR & (1u << 0))) { }
}

uint16_t board_adc_read(uint8_t ch)
{
    ADC_SQR1 = (1u << 0) | ((uint32_t)ch << 6);   /* L=1, SQ1=ch */
    ADC_CR  |= (1u << 2);                          /* ADSTART */
    while (!(ADC_ISR & (1u << 2))) { }             /* wait EOC */
    return (uint16_t)ADC_DR;
}

/* ------------------------------------------------------------------ */
/*  Board-level helpers                                               */
/* ------------------------------------------------------------------ */
void board_init(void)
{
    clock_init();
    gpio_init();
    systick_init();
    adc_init();
    /* Disable UCPD standby (L4 specific) to save power */
    PWR_CR3 |= PWR_CR3_UCPD_DBDIS;
}

void board_led_set(uint8_t on)
{
    if (on) GPIOB->BSRR = (1u << 15);
    else    GPIOB->BRR  = (1u << 15);
}

void board_led_toggle(void)
{
    GPIOB->ODR ^= (1u << 15);
}

int board_battery_present(void)
{
    /* Active-low on most smart-battery connectors */
    return ((GPIOA->IDR & (1u << PIN_BATTERY_PRESENT)) == 0) ? 1 : 0;
}

int board_smbalert_host(void)
{
    return ((GPIOA->IDR & (1u << PIN_SMBALERT_HOST)) == 0) ? 1 : 0;
}

void board_glitch_pulse(uint32_t us)
{
    if (us > GLITCH_PULSE_MAX_US) us = GLITCH_PULSE_MAX_US;
    GPIOA->BSRR = (1u << PIN_GLITCH_OUT);     /* high */
    /* Busy-loop delay (us) at 80 MHz: ~80 cycles/us */
    for (volatile uint32_t i = 0; i < us * 80u; i++) { __asm volatile("nop"); }
    GPIOA->BRR = (1u << PIN_GLITCH_OUT);      /* low */
}

/* ------------------------------------------------------------------ */
/*  Safety interlock — abort spoofing if physical limits exceeded     */
/* ------------------------------------------------------------------ */
static void safety_interlock(void)
{
    /* Read real battery voltage (not spoofed) from ADC sense pin */
    uint16_t v_raw = board_adc_read(ADC_VBAT_SENSE);
    /* 12-bit ADC, VREF=3.0V, divider ratio 1:4 -> mV = raw*3000*4/4095 */
    uint32_t v_mv = ((uint32_t)v_raw * 12000u) / 4095u;
    if (v_mv > SAFE_VOLT_MAX_MV) {
        /* Real voltage exceeds ceiling: drop all spoof rules and revert
         * to pass-through mode to avoid dangerous charging behaviour. */
        spoof_clear();
        ble_set_mode(MODE_PASSTHROUGH);
        uint8_t alert = 0xFF;
        ble_send(BLE_MSG_NACK, &alert, 1);
    }

    /* Temperature interlock via thermistor ADC */
    uint16_t t_raw = board_adc_read(ADC_THERM_SENSE);
    /* Approx: 10 kOhm NTC; raw > 3500 => very hot */
    if (t_raw > 3500) {
        spoof_clear();
        ble_set_mode(MODE_PASSTHROUGH);
    }
}

/* ------------------------------------------------------------------ */
/*  Stream captured frames to operator over BLE                       */
/* ------------------------------------------------------------------ */
static void stream_captures(void)
{
    smb_frame_t f;
    while (smbus_capture_pop(&f) == 0) {
        uint8_t pkt[64];
        uint8_t off = 0;
        /* Pack: ts(4) + port(1) + dir(1) + addr(1) + cmd(1) +
         *       wlen(1) + rlen(1) + flags(1) + wbuf + rbuf */
        pkt[off++] = (uint8_t)(f.ts_ms & 0xFF);
        pkt[off++] = (uint8_t)(f.ts_ms >> 8);
        pkt[off++] = (uint8_t)(f.ts_ms >> 16);
        pkt[off++] = (uint8_t)(f.ts_ms >> 24);
        pkt[off++] = (uint8_t)f.port;
        pkt[off++] = (uint8_t)f.dir;
        pkt[off++] = f.addr;
        pkt[off++] = f.cmd;
        pkt[off++] = f.wlen;
        pkt[off++] = f.rlen;
        pkt[off++] = f.flags;
        for (uint8_t i = 0; i < f.wlen && off < sizeof(pkt); i++)
            pkt[off++] = f.wbuf[i];
        for (uint8_t i = 0; i < f.rlen && off < sizeof(pkt); i++)
            pkt[off++] = f.rbuf[i];
        ble_send(BLE_MSG_FRAME, pkt, off);
    }
}

/* ------------------------------------------------------------------ */
/*  Background battery poll (master-mode probe every 5 s)             */
/* ------------------------------------------------------------------ */
static void background_poll(void)
{
    static uint32_t last_poll = 0;
    if ((board_millis() - last_poll) < 5000) return;
    last_poll = board_millis();
    if (!board_battery_present()) return;

    /* Read real telemetry from the battery for operator visibility */
    uint16_t soc = 0, volt = 0, curr = 0, temp = 0;
    if (smbus_read_word(SMB_PORT_BATT, 0x0B, SBS_REL_STATE_OF_CHARGE, &soc) == 0) {
        /* pack into a status-like frame */
    }
    smbus_read_word(SMB_PORT_BATT, 0x0B, SBS_VOLTAGE, &volt);
    smbus_read_word(SMB_PORT_BATT, 0x0B, SBS_CURRENT, &curr);
    smbus_read_word(SMB_PORT_BATT, 0x0B, SBS_TEMPERATURE, &temp);

    /* Emit a synthetic capture frame with real telemetry */
    smb_frame_t f;
    memset(&f, 0, sizeof(f));
    f.ts_ms = board_millis();
    f.port  = SMB_PORT_BATT;
    f.dir   = SMB_DIR_HOST_READ;
    f.addr  = 0x0B;
    f.cmd   = SBS_REL_STATE_OF_CHARGE;
    f.rbuf[0] = (uint8_t)(soc & 0xFF);
    f.rbuf[1] = (uint8_t)(soc >> 8);
    f.rlen = 2;
    smbus_capture_push(&f);

    /* If in SPOOF_FULL mode, keep the spoof profile active */
    if (ble_get_mode() == MODE_SPOOF_FULL) {
        spoof_profile_full_charge();
    }
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */
int main(void)
{
    board_init();
    smbus_init();
    ble_init();

    /* Listen on host side as SBS default battery address 0x0B */
    smbus_listen(SMB_PORT_HOST, 0x0B);

    /* Default to pass-through (capture-only) for safety */
    ble_set_mode(MODE_PASSTHROUGH);

    uint32_t led_last = 0;

    for (;;) {
        /* 1. Process inbound BLE commands */
        ble_poll();

        /* 2. Stream captured frames to operator */
        stream_captures();

        /* 3. Background battery telemetry poll */
        background_poll();

        /* 4. Safety interlocks */
        safety_interlock();

        /* 5. Periodic status beacon */
        ble_status_beacon();

        /* 6. Covert heartbeat LED (very slow, off by default) */
        if (ble_get_mode() != MODE_STANDBY) {
            if ((board_millis() - led_last) > LED_HEARTBEAT_MS) {
                led_last = board_millis();
                board_led_toggle();
            }
        }

        /* 7. Enter low-power wait-for-interrupt (real silicon: WFI) */
        __asm volatile("wfi");
    }

    return 0;   /* never reached */
}

/* ------------------------------------------------------------------ */
/*  Vector table redirect (minimal)                                   */
/*  In real build the startup file provides the full table; these     */
/*  weak overrides route the SMBus and UART ISRs to our handlers.     */
/* ------------------------------------------------------------------ */
extern void I2C1_EV_IRQHandler(void) __attribute__((weak, alias("smbus_host_ev_isr")));
extern void I2C1_ER_IRQHandler(void) __attribute__((weak, alias("smbus_host_er_isr")));
extern void I2C2_EV_IRQHandler(void) __attribute__((weak, alias("smbus_batt_ev_isr")));
extern void I2C2_ER_IRQHandler(void) __attribute__((weak, alias("smbus_batt_er_isr")));