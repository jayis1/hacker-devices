/*
 * lumen-tap/firmware/main.c
 * LumenTap firmware main loop, clock setup, ISR dispatch, and CDC
 * command interpreter for the laser-Doppler acoustic eavesdropping tool.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "board.h"
#include "registers.h"
#include "usb_descriptors.h"
#include "drivers/dsp.h"
#include "drivers/laser.h"
#include "drivers/audio.h"
#include "drivers/sdlog.h"

/* ---- SysTick (1 ms) ----------------------------------------------- */
static volatile uint32_t g_tick = 0;
static volatile uint32_t g_wdog_kick = 0;

static void systick_init(void)
{
    SYSTICK_RVR = (SYSCLK_HZ / 1000) - 1;
    SYSTICK_CVR = 0;
    SYSTICK_CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSRC;
}

/* ---- Clock setup: HSE → PLL → 480 MHz on STM32H7 ------------------- */
static void clock_init(void)
{
    /* Enable HSE (bypass if no crystal — assume board has 25 MHz xtal) */
    RCC_CR |= (1U << 16);              /* HSEON */
    while (!(RCC_CR & (1U << 17))) {}  /* HSERDY */

    /* Configure VOS scale 1 (highest performance) via PWR D3CR */
    PWR_D3CR = (3U << 14);             /* VOS0/3 scale 1 on H7 */
    while (!(PWR_D3CR & (1U << 13))) {}

    /* PLL1: input = HSE/5 = 5 MHz, N=192, P=2 → 480 MHz, M=5, Q=240 */
    RCC_PLLCFGR = (5U << 4) | (192U << 8) | (0U << 16) | (2U << 24) |
                  (240U << 21) | (1U << 0);   /* PLL1ENABLE */
    RCC_CR |= (1U << 24);              /* PLL1ON */
    while (!(RCC_CR & (1U << 25))) {}  /* PLL1RDY */

    /* Flash latency for 480 MHz: VOS0 → 4 wait states on H7 */
    FLASH_ACR = (4U << 0) | (1U << 8); /* LATENCY=4, PRFTEN */

    /* Switch SYSCLK to PLL1 (CFGR SW = 3) */
    RCC_CFGR = (3U << 0);
    while (((RCC_CFGR >> 3) & 0x7) != 3) {}

    /* APB dividers: APB1=/4 (120MHz), APB2=/2 (240MHz? H7 D1 domain) */
    /* On H7 these are D1CFGR fields; simplified: */
    *(volatile uint32_t *)(RCC_BASE + 0x18) = (4U << 8) | (5U << 4) | (4U << 0);
}

/* ---- GPIO + board init -------------------------------------------- */
static void gpio_global_init(void)
{
    /* enable all GPIO banks */
    RCC_AHB4ENR |= (1U << 0) | (1U << 1) | (1U << 2) | (1U << 3) | (1U << 4);
}

void board_init(void)
{
    clock_init();
    systick_init();
    gpio_global_init();
    laser_init();
    audio_init();
    sdlog_init();
}

/* ---- CDC command interpreter -------------------------------------- */
/* Minimal JSON-ish line protocol: {"cmd":"xxx",...}\n */
static char cmd_line[256];
static int  cmd_line_pos = 0;

extern ltm_dsp_config_t *audio_get_dsp_config(void);
extern ltm_dsp_state_t  *audio_get_dsp_state(void);

static void parse_and_exec(const char *line)
{
    /* Tiny field extractor: looks for "key":"value" or "key":number */
    char key[32], val[64];
    const char *p = line;
    ltm_dsp_config_t *cfg = audio_get_dsp_config();
    ltm_dsp_state_t  *st  = audio_get_dsp_state();

    /* find "cmd" */
    if (!strstr(line, "\"cmd\"")) return;
    /* extract value after "cmd" */
    const char *c = strstr(line, "\"cmd\"");
    c = strchr(c + 5, ':');
    if (!c) return;
    c = strchr(c + 1, '"');
    if (!c) return;
    const char *e = strchr(c + 1, '"');
    int cl = e ? (int)(e - c - 1) : 0;
    if (cl <= 0 || cl >= (int)sizeof(val)) return;
    memcpy(val, c + 1, cl); val[cl] = 0;

    if (strcmp(val, "set_power") == 0) {
        const char *np = strstr(line, "\"duty\"");
        if (np) {
            np = strchr(np + 6, ':');
            int d = atoi(np + 1);
            laser_set_power((uint8_t)d);
        }
    } else if (strcmp(val, "arm") == 0) {
        laser_arm(1);
    } else if (strcmp(val, "disarm") == 0) {
        laser_arm(0);
    } else if (strcmp(val, "set_demod") == 0) {
        const char *np = strstr(line, "\"mode\"");
        if (np) {
            np = strchr(np + 6, ':');
            int m = atoi(np + 1);
            cfg->demod_mode = (ltm_demod_mode_t)m;
        }
    } else if (strcmp(val, "set_bandpass") == 0) {
        const char *lp = strstr(line, "\"low\"");
        const char *hp = strstr(line, "\"high\"");
        if (lp && hp) {
            float lo = (float)atof(strchr(lp + 5, ':') + 1);
            float hi = (float)atof(strchr(hp + 6, ':') + 1);
            dsp_set_bandpass(st, cfg, lo, hi);
        }
    } else if (strcmp(val, "set_noise") == 0) {
        const char *np = strstr(line, "\"depth\"");
        if (np) {
            np = strchr(np + 7, ':');
            cfg->noise_depth = (float)atof(np + 1);
            if (cfg->noise_depth < 0.0f) cfg->noise_depth = 0.0f;
            if (cfg->noise_depth > 1.0f) cfg->noise_depth = 1.0f;
        }
    } else if (strcmp(val, "set_bypass") == 0) {
        const char *np = strstr(line, "\"on\"");
        if (np) {
            np = strchr(np + 4, ':');
            cfg->bypass = (uint8_t)atoi(np + 1);
        }
    } else if (strcmp(val, "record_start") == 0) {
        ltm_session_hdr_t hdr;
        memset(&hdr, 0, sizeof(hdr));
        const char *op = strstr(line, "\"operator\"");
        if (op) {
            op = strchr(op + 10, '"');
            const char *oe = strchr(op + 1, '"');
            int n = oe ? (int)(oe - op - 1) : 0;
            if (n > 0 && n < (int)sizeof(hdr.operator)) {
                memcpy(hdr.operator, op + 1, n);
            }
        }
        hdr.start_tick_ms = g_tick;
        sdlog_begin_session(&hdr);
    } else if (strcmp(val, "record_stop") == 0) {
        sdlog_end_session();
    } else if (strcmp(val, "status") == 0) {
        /* handled below as periodic status */
    }
    (void)key;
}

static void process_cdc(void)
{
    uint8_t buf[64];
    int n = cdc_read(buf, sizeof(buf));
    for (int i = 0; i < n; ++i) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            if (cmd_line_pos > 0) {
                cmd_line[cmd_line_pos] = 0;
                parse_and_exec(cmd_line);
                cmd_line_pos = 0;
            }
        } else if (cmd_line_pos < (int)sizeof(cmd_line) - 1) {
            cmd_line[cmd_line_pos++] = buf[i];
        }
    }
}

/* ---- Periodic status JSON ----------------------------------------- */
static void send_status(void)
{
    static uint32_t last = 0;
    if (g_tick - last < 250) return;   /* 4 Hz */
    last = g_tick;

    ltm_dsp_state_t *st = audio_get_dsp_state();
    ltm_sdlog_stats_t sd_st;
    sdlog_get_stats(&sd_st);

    static char json[256];
    int len = snprintf(json, sizeof(json),
        "{\"type\":\"status\",\"tick\":%lu,\"laser\":%u,\"pwm\":%u,"
        "\"ambient\":%u,\"arm\":%u,\"rms_in\":%.4f,\"rms_out\":%.4f,"
        "\"snr_db\":%.2f,\"demod\":%u,\"bp_lo\":%.1f,\"bp_hi\":%.1f,"
        "\"noise\":%.2f,\"gain\":%.3f,\"sd_state\":%u,\"sd_blk\":%lu}\n",
        (unsigned long)g_tick,
        (unsigned)laser_is_emitting(),
        (unsigned)g_laser.pwm_duty,
        (unsigned)g_laser.ambient_cnt,
        (unsigned)g_safety.arm_key_hw,
        st->last_rms_in, st->last_rms_out, st->last_snr_db,
        (unsigned)(audio_get_dsp_config()->demod_mode),
        (double)audio_get_dsp_config()->bp_low_hz,
        (double)audio_get_dsp_config()->bp_high_hz,
        (double)audio_get_dsp_config()->noise_depth,
        (double)st->agc_gain,
        (unsigned)sdlog_state(),
        (unsigned long)sd_st.blocks_written
    );
    cdc_send_status(json, len);
}

/* ---- Watchdog kick ----------------------------------------------- */
static void watchdog_kick(void)
{
    /* In production this would refresh the IWDG; here we just track it. */
    g_wdog_kick = g_tick;
    g_safety.watchdog_ok = 1;
}

/* ---- Interrupt handlers (weak aliases in startup) ----------------- */
void DMA1_Stream0_IRQHandler(void) { audio_dma_isr(); }

void SysTick_Handler(void)
{
    g_tick++;
    laser_tick_ms();
    /* watchdog window: if no kick in 2 s, mark unsafe */
    if (g_tick - g_wdog_kick > 2000) g_safety.watchdog_ok = 0;
}

/* ---- Main loop ---------------------------------------------------- */
int main(void)
{
    board_init();

    /* start ADC capture + DMA */
    audio_start_capture();

    /* default: laser off, but armed standby (key controls HW enable) */
    laser_set_power(0);

    /* main super-loop */
    uint32_t led_blink = 0;
    while (1) {
        process_cdc();
        audio_pump_usb();
        send_status();
        watchdog_kick();

        /* slow LED blink when idle */
        if (++led_blink > 500000) {
            led_blink = 0;
            if (!laser_is_emitting()) {
                GPIOB->ODR ^= (1U << PIN_LED_STATUS);
            }
        }
    }
    return 0;
}