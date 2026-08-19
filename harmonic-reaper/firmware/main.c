/*
 * main.c — Harmonic Reaper wand firmware, top-level entry & scheduler
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Target:  Nordic nRF52840 QFAA (Cortex-M4F, 64 MHz)
 * Build:   make
 * Flash:   make flash
 *
 * Architecture:
 *   This MCU is the wand controller. It does NOT run the FFT/classifier —
 *   that lives in the Spartan-7 FPGA. The MCU owns:
 *     • BLE SoftDevice + GATT C2 interface (NUS-style)
 *     • TX chain (ADF4159 synth + QPL9547 PA + AGC DAC)
 *     • RX chain enable/disable (AD9361, ADL5802/AD9226)
 *     • FPGA boot & command/status over SPI
 *     • OLED UI (SSD1306)
 *     • IMU sweep-angle tagging (ICM-42688)
 *     • Haptic feedback (DRV2605)
 *     • Hit-log storage (NOR flash + microSD)
 *     • USB-C CDC log + DFU
 *
 *   The runtime is a cooperative 1 ms tick scheduler driven by TIMER1.
 *   No preemption; each task is non-blocking and returns in <50 µs.
 */

#include <stdint.h>
#include <string.h>
#include "registers.h"
#include "board.h"

/* ---- driver prototypes (defined in drivers/*.c) ---- */
#include "drivers/adf4159.h"
#include "drivers/qpl9547.h"
#include "drivers/ad9361.h"
#include "drivers/ad9226_if.h"
#include "drivers/fpga_spi.h"
#include "drivers/oled_ssd1306.h"
#include "drivers/imu_icm42688.h"
#include "drivers/ble_c2.h"
#include "drivers/power_mgmt.h"
#include "drivers/storage.h"
#include "drivers/usb_cdc.h"

/* ====================================================================== */
/*  Global state                                                           */
/* ====================================================================== */

typedef struct {
    sweep_mode_t   mode;
    int8_t         tx_power_dbm;     /* commanded TX power before AGC trim */
    uint32_t       pulse_prf_hz;     /* pulse mode PRF                     */
    uint16_t       pulse_width_ns;   /* pulse width for pulsed mode          */
    int8_t         ratio_semi_thresh;  /* +dB to call semiconductor        */
    int8_t         ratio_metal_thresh; /* −dB to call dissimilar metal     */
    uint8_t        hit_stickiness;    /* consecutive samples to confirm   */
    int8_t         agc_target_dbfs;   /* AGC setpoint                      */
    uint8_t        quiet_mode;        /* randomize PRF / minimal UI        */

    /* live readings (updated by FPGA poll task) */
    int16_t        p2_dbfs;           /* 2f0 power, dBFS                   */
    int16_t        p3_dbfs;           /* 3f0 power, dBFS                   */
    int8_t        ratio_db;          /* 10log10(P2/P3)                    */
    uint8_t        classify;          /* 0=none 1=semi 2=metal 3=ambiguous */

    /* hit tracking */
    uint8_t        consecutive_semi;
    uint32_t       hit_count;
    uint32_t       sweep_start_ms;
    uint8_t        armed;             /* TX enabled?                       */

    /* IMU-derived wand heading for spatial tagging */
    int16_t        wand_pitch_deg;
    int16_t        wand_yaw_deg;

    /* battery */
    uint8_t        batt_pct;          /* 0..100                            */
    uint8_t        charging;          /* 1 if VBUS + charge stat asserted   */

    /* flags from BLE/USB */
    uint8_t        cmd_pending;
    uint8_t        export_request;
} reaper_state_t;

static reaper_state_t g;

/* 1 ms scheduler tick counter */
static volatile uint32_t g_tick_ms;

/* ====================================================================== */
/*  Interrupt vectors — TIMER1 → scheduler tick                            */
/* ====================================================================== */

void TIMER1_IRQHandler(void)
{
    /* CC0 compare event → increment scheduler tick */
    if (NRF_EVENT_CHECK(NRF_TIMER1_BASE, 0x140u)) {
        NRF_EVENT_CLEAR(NRF_TIMER1_BASE, 0x140u);
        g_tick_ms++;
    }
}

/* ====================================================================== */
/*  Clock & GPIO init                                                       */
/* ====================================================================== */

static void clock_init(void)
{
    /* Start 32.768 kHz LFCLK from crystal (BLE SoftDevice needs it) */
    CLOCK_LFCLKSRC = CLOCK_LFCLKSRC_XTAL;
    CLOCK_LFCLKSTART = 1u;
    while (!CLOCK_LFCLKSTARTED) { /* spin */ }

    /* Start 64 MHz HFXO — required for SPIM >8 MHz and USB */
    CLOCK_HFCLKSTART = 1u;
    while (!CLOCK_HFCLKSTARTED) { /* spin */ }
}

static void gpio_init(void)
{
    /* LEDs as push-pull outputs, off (active high → clear) */
    uint32_t led_pins[] = { LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN };
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t p = led_pins[i];
        GPIO_PIN_CNF(NRF_GPIO_BASE, p) =
            GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
        GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << p);
    }

    /* FPGA DONE input with pull-up */
    GPIO_PIN_CNF(NRF_GPIO_BASE, FPGA_DONE_PIN) =
        GPIO_CNF_DIR_INPUT | (GPIO_CNF_PULL_UP << 2);

    /* FPGA IRQ input, no pull (FPGA drives) */
    GPIO_PIN_CNF(NRF_GPIO_BASE, FPGA_IRQ_PIN) = GPIO_CNF_DIR_INPUT;

    /* Charger status input, pull-up */
    GPIO_PIN_CNF(NRF_GPIO_BASE, CHARGE_STAT_PIN) =
        GPIO_CNF_DIR_INPUT | (GPIO_CNF_PULL_UP << 2);

    /* SD detect input, pull-up */
    GPIO_PIN_CNF(NRF_GPIO_BASE, SD_DETECT_PIN) =
        GPIO_CNF_DIR_INPUT | (GPIO_CNF_PULL_UP << 2);

    /* TX chain enable / ADC PD / mixer EN — all outputs, low = off */
    uint32_t tx_en_pins[] = { QPL9547_TX_EN_PIN, AD9226_PD_PIN, ADL5802_EN_PIN,
                              ADF4159_CE_PIN, ADF4159_LE_PIN };
    for (uint32_t i = 0; i < 5; i++) {
        GPIO_PIN_CNF(NRF_GPIO_BASE, tx_en_pins[i]) =
            GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
        GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << tx_en_pins[i]);
    }

    /* AGC DAC bits as outputs, all 0 (max attenuation for safety) */
    uint32_t dac_pins[] = { AGC_DAC_D0_PIN, AGC_DAC_D1_PIN, AGC_DAC_D2_PIN,
                            AGC_DAC_D3_PIN, AGC_DAC_D4_PIN, AGC_DAC_D5_PIN };
    for (uint32_t i = 0; i < 6; i++) {
        uint32_t p = dac_pins[i];
        uint32_t port = (p >= 32) ? (NRF_GPIO_BASE + 0x1000u) : NRF_GPIO_BASE;
        uint32_t b = p & 31u;
        GPIO_PIN_CNF(port, b) =
            GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
        GPIO_OUTCLR(port) = (1u << b);
    }
}

/* ====================================================================== */
/*  TIMER1 init — 1 ms tick from 16 MHz HCLK/2 = 8 MHz, /8000 → 1 kHz      */
/* ====================================================================== */

static void timer1_init(void)
{
    TIMER_MODE(NRF_TIMER1_BASE)     = TIMER_MODE_TIMER;
    TIMER_BITMODE(NRF_TIMER1_BASE)  = TIMER_BITMODE_16;
    TIMER_PRESCALER(NRF_TIMER1_BASE) = 0u;       /* /1 → 16 MHz tick */
    TIMER_CC0(NRF_TIMER1_BASE)      = 16000u;     /* 1 ms */
    TIMER_INTENSET(NRF_TIMER1_BASE) = (1u << 16);  /* CC0 compare */

    /* Clear + start */
    NRF_TASK_START(NRF_TIMER1_BASE, 0x00Cu);     /* CLEAR */
    NRF_TASK_START(NRF_TIMER1_BASE, 0x000u);     /* START */

    /* Enable IRQ in NVIC */
    extern void NVIC_EnableIRQ(int);
    NVIC_EnableIRQ(5);  /* TIMER1 = IRQ 5 on nRF52 */
}

/* ====================================================================== */
/*  Scheduler — cooperative, 1 ms tick                                     */
/* ====================================================================== */

static inline uint32_t tick_now(void) { return g_tick_ms; }
static inline uint32_t elapsed_ms(uint32_t since)
{
    return tick_now() - since;
}

typedef struct {
    uint32_t last;
    uint32_t period;
    void (*fn)(void);
} sched_task_t;

/* task function forward declarations */
static void task_fpga_poll(void);     /* 5 ms  — read classifier results */
static void task_ble_pump(void);      /* 10 ms — process BLE commands   */
static void task_oled_refresh(void);  /* 50 ms — UI refresh              */
static void task_imu_read(void);      /* 20 ms — wand heading            */
static void task_agc_loop(void);      /* 10 ms — TX power AGC           */
static void task_battery(void);       /* 1000 ms — battery gauge         */
static void task_storage_flush(void); /* 500 ms — persist hit log         */
static void task_usb_pump(void);      /* 5 ms  — CDC backchannel          */

static sched_task_t tasks[] = {
    { 0,    5,   task_fpga_poll      },
    { 0,    5,   task_usb_pump      },
    { 0,    10,  task_ble_pump       },
    { 0,    10,  task_agc_loop       },
    { 0,    20,  task_imu_read       },
    { 0,    50,  task_oled_refresh   },
    { 0,    500, task_storage_flush  },
    { 0,    1000, task_battery       },
};
#define NUM_TASKS (sizeof(tasks)/sizeof(tasks[0]))

static void scheduler_run(void)
{
    uint32_t now = tick_now();
    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        if (now - tasks[i].last >= tasks[i].period) {
            tasks[i].last = now;
            tasks[i].fn();
        }
    }
}

/* ====================================================================== */
/*  Task implementations                                                    */
/* ====================================================================== */

static void task_fpga_poll(void)
{
    if (g.mode == MODE_IDLE || g.mode == MODE_FAULT) return;

    fpga_result_t r;
    if (!fpga_read_result(&r)) return;   /* classifier not ready */

    g.p2_dbfs = r.p2_dbfs;
    g.p3_dbfs = r.p3_dbfs;
    g.ratio_db = r.ratio_db;

    /* classify */
    if (g.ratio_db >= g.ratio_semi_thresh) {
        g.classify = 1;            /* semiconductor */
        g.consecutive_semi++;
    } else if (g.ratio_db <= g.ratio_metal_thresh) {
        g.classify = 2;            /* dissimilar metal */
        g.consecutive_semi = 0;
    } else {
        g.classify = 3;            /* ambiguous */
        g.consecutive_semi = 0;
    }

    /* hit detection — sticky consecutive semiconductor returns */
    if (g.consecutive_semi >= g.hit_stickiness &&
        g.p2_dbfs >= (g.agc_target_dbfs - 6)) {
        /* new hit! */
        g.hit_count++;
        haptic_pulse(2);            /* double buzz */
        oled_flash_hit(g.p2_dbfs, g.ratio_db, g.hit_count);

        /* log it */
        hit_record_t h = {
            .timestamp_ms = tick_now(),
            .pitch_deg = g.wand_pitch_deg,
            .yaw_deg = g.wand_yaw_deg,
            .p2_dbfs = g.p2_dbfs,
            .p3_dbfs = g.p3_dbfs,
            .ratio_db = g.ratio_db,
            .classify = g.classify,
            .tx_power_dbm = g.tx_power_dbm,
        };
        storage_log_hit(&h);

        /* notify app immediately */
        ble_notify_hit(&h);

        g.consecutive_semi = 0;    /* reset to avoid double-logging */
    }
}

static void task_ble_pump(void)
{
    ble_cmd_t cmd;
    while (ble_dequeue_cmd(&cmd)) {
        switch (cmd.op) {
        case BLE_CMD_SET_MODE:
            g.mode = (sweep_mode_t)cmd.arg0;
            oled_set_mode(g.mode);
            if (g.mode == MODE_SWEEP_CW || g.mode == MODE_SWEEP_PULSED) {
                g.armed = 1;
                ad9361_enable(1);
                adl5802_enable(1);
                ad9226_power_down(0);
                adf4159_enable(1);
                qpl9547_tx_enable(1);
                g.sweep_start_ms = tick_now();
            } else {
                g.armed = 0;
                qpl9547_tx_enable(0);
                adf4159_enable(0);
                ad9226_power_down(1);
                adl5802_enable(0);
                ad9361_enable(0);
            }
            break;
        case BLE_CMD_SET_TX_POWER:
            g.tx_power_dbm = (int8_t)cmd.arg0;
            qpl9547_set_power(g.tx_power_dbm);
            break;
        case BLE_CMD_SET_PULSE:
            g.pulse_prf_hz = cmd.arg0;
            g.pulse_width_ns = cmd.arg1;
            adf4159_set_pulse(g.pulse_prf_hz, g.pulse_width_ns);
            break;
        case BLE_CMD_SET_THRESH:
            g.ratio_semi_thresh = (int8_t)cmd.arg0;
            g.ratio_metal_thresh = (int8_t)cmd.arg1;
            fpga_set_thresholds(g.ratio_semi_thresh, g.ratio_metal_thresh);
            break;
        case BLE_CMD_SET_QUIET:
            g.quiet_mode = cmd.arg0 ? 1 : 0;
            adf4159_set_quiet(g.quiet_mode);
            break;
        case BLE_CMD_QUERY_STATUS: {
            ble_status_t s = {
                .mode = g.mode,
                .p2_dbfs = g.p2_dbfs,
                .p3_dbfs = g.p3_dbfs,
                .ratio_db = g.ratio_db,
                .classify = g.classify,
                .tx_power_dbm = g.tx_power_dbm,
                .batt_pct = g.batt_pct,
                .hit_count = g.hit_count,
            };
            ble_send_status(&s);
            break;
        }
        case BLE_CMD_EXPORT:
            g.export_request = 1;
            break;
        default:
            break;
        }
    }
}

static void task_imu_read(void)
{
    imu_sample_t s;
    if (imu_read(&s)) {
        /* crude pitch/yaw from accel + gyro — enough for polar tagging */
        g.wand_pitch_deg = s.pitch_deg;
        g.wand_yaw_deg = s.yaw_deg;
    }
}

static void task_agc_loop(void)
{
    if (!g.armed) return;
    /* Target the 2f0 receiver level to g.agc_target_dbfs.
     * If P2 is above target, increase AGC attenuation (lower TX power).
     * If below target, decrease attenuation (raise TX power).
     * Step in 1 dB, 100 ms settle between steps (we run every 10 ms but
     * only step every 10th call to allow settling). */
    static uint8_t settle_cnt = 0;
    if (settle_cnt++ < 10) return;
    settle_cnt = 0;

    int16_t err = g.agc_target_dbfs - g.p2_dbfs;   /* + → too low, raise TX */
    if (err > 1)      qpl9547_step_power(+1);
    else if (err < -1) qpl9547_step_power(-1);
}

static void task_oled_refresh(void)
{
    oled_update_live(g.p2_dbfs, g.p3_dbfs, g.ratio_db, g.classify,
                     g.batt_pct, g.charging, g.mode, g.hit_count,
                     g.wand_pitch_deg, g.wand_yaw_deg);
}

static void task_storage_flush(void)
{
    storage_flush_pending();
    if (g.export_request) {
        g.export_request = 0;
        storage_export_to_sd();
        ble_notify_export_done(storage_export_count());
    }
}

static void task_battery(void)
{
    uint16_t vdd_mv = power_read_vdd_mv();
    g.batt_pct = power_mv_to_pct(vdd_mv);
    g.charging = power_is_charging();
    if (g.batt_pct < 10) {
        oled_flash_low_battery(g.batt_pct);
    }
}

static void task_usb_pump(void)
{
    usb_cdc_pump();
}

/* ====================================================================== */
/*  Mode entry helpers                                                      */
/* ====================================================================== */

static void enter_idle(void)
{
    g.mode = MODE_IDLE;
    g.armed = 0;
    qpl9547_tx_enable(0);
    adf4159_enable(0);
    ad9226_power_down(1);
    adl5802_enable(0);
    ad9361_enable(0);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << LED_GREEN_PIN);  /* idle green */
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << LED_RED_PIN);
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << LED_BLUE_PIN);
    oled_set_mode(MODE_IDLE);
}

/* ====================================================================== */
/*  main()                                                                 */
/* ====================================================================== */

int main(void)
{
    /* 1. clocks & GPIO */
    clock_init();
    gpio_init();

    /* 2. timer/scheduler */
    timer1_init();

    /* 3. load defaults */
    memset(&g, 0, sizeof(g));
    g.mode = MODE_IDLE;
    g.tx_power_dbm = DEFAULT_TX_POWER_DBM;
    g.pulse_prf_hz = DEFAULT_PULSE_PRF_HZ;
    g.pulse_width_ns = DEFAULT_PULSE_WIDTH_NS;
    g.ratio_semi_thresh = DEFAULT_RATIO_THRESH_SEMI;
    g.ratio_metal_thresh = DEFAULT_RATIO_THRESH_METAL;
    g.hit_stickiness = DEFAULT_HIT_STICKINESS;
    g.agc_target_dbfs = DEFAULT_AGC_TARGET_DBFS;
    g.quiet_mode = 1;  /* default to quiet (randomized PRF) */

    /* 4. init drivers in dependency order */
    storage_init();
    oled_init();
    imu_init();
    power_init();
    usb_cdc_init();
    adf4159_init();
    qpl9547_init();
    ad9361_init();
    ad9226_if_init();
    fpga_boot();
    fpga_set_thresholds(g.ratio_semi_thresh, g.ratio_metal_thresh);
    ble_init();   /* BLE last — starts advertising once everything is ready */

    enter_idle();
    oled_show_boot();

    /* 5. main loop — cooperative scheduler */
    while (1) {
        scheduler_run();
        __WFI();   /* wait for next IRQ (TIMER1) — lowest power between ticks */
    }

    /* unreachable */
    return 0;
}

/* EOF — main.c — jayis1 */