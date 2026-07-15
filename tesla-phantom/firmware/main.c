/*
 * tesla-phantom — main.c
 * Main application logic and state machine for the
 * Electromagnetic Fault Injection & Magnetic Side-Channel Analysis Platform.
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Build: make
 * Target: STM32H743VIT6 (Cortex-M7 @ 480 MHz)
 *
 * The main loop implements a state machine:
 *   BOOT → IDLE → ARMED → CHARGING → PULSE → ANALYZE → IDLE
 *   IDLE → (sca cmd)   → CAPTURE → ANALYZE → IDLE
 *   IDLE → (scan cmd)  → SCAN (loop: MOVE→CHARGE→PULSE→LOG) → IDLE
 *   IDLE → (cal cmd)   → CALIBRATE → IDLE
 *   any  → FAULT → IDLE (after safe discharge)
 */

#include "board.h"
#include "registers.h"

/* ── Global state ─────────────────────────────────────────────────── */
volatile device_state_t   g_state = ST_BOOT;
volatile op_mode_t        g_mode = MODE_EMFI;
volatile trigger_source_t g_trig_src = TRIG_EXTERNAL;
emfi_params_t             g_emfi;
sca_params_t              g_sca;
scan_params_t             g_scan;
position_t                g_position = {0, 0, 0};
config_t                  g_config;
volatile uint8_t          g_fault_flag = 0;
volatile uint16_t         g_hv_voltage = 0;
volatile uint8_t          g_battery_pct = 100;
volatile uint32_t         g_uptime_ms = 0;

/* ── SCA trace buffer in external SRAM ────────────────────────────── */
static int16_t *g_trace_buf = (int16_t *)SDRAM_BASE_ADDR;
static volatile uint32_t g_trace_samples = 0;
static volatile uint8_t  g_capture_done = 0;

/* ── Command buffer ───────────────────────────────────────────────── */
#define CMD_BUF_SIZE 256
static char g_cmd_buf[CMD_BUF_SIZE];
static uint16_t g_cmd_len = 0;

/* ── Interrupt vector table ───────────────────────────────────────── */
extern void reset_handler(void);
void nmi_handler(void)        { while (1) { } }
void hardfault_handler(void)  { while (1) { } }
void memmanage_handler(void)  { while (1) { } }
void busfault_handler(void)   { while (1) { } }
void usagefault_handler(void) { while (1) { } }
void svc_handler(void)        { }
void pend_sv_handler(void)    { }
void systick_handler(void);

__attribute__((section(".isr_vector"), used))
void (* const g_vector_table[])(void) = {
    (void (*)(void))(&_estack),   /* initial stack pointer */
    reset_handler,
    nmi_handler,
    hardfault_handler,
    memmanage_handler,
    busfault_handler,
    usagefault_handler,
    0, 0, 0, 0,                   /* reserved */
    svc_handler,
    pend_sv_handler,
    systick_handler,
    /* IRQ 0–15 handled above; fill remaining with default */
};

/* ── SysTick (1 ms tick) ──────────────────────────────────────────── */
void systick_handler(void) {
    g_uptime_ms++;
}

static void systick_init(void) {
    SYSTICK->LOAD = (SYSCLK_FREQ / 1000) - 1;
    SYSTICK->VAL = 0;
    SYSTICK->CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSRC;
}

uint32_t millis(void) {
    return g_uptime_ms;
}

void delay_ms(uint32_t ms) {
    uint32_t start = g_uptime_ms;
    while ((g_uptime_ms - start) < ms) { }
}

void delay_us(uint32_t us) {
    /* Simple busy-loop: at 480 MHz, ~480 cycles/µs. Each iteration ~4 cycles. */
    volatile uint32_t count = us * 120;
    while (count--) { __asm volatile ("nop"); }
}

/* ── CRC32 (software, polynomial 0xEDB88320) ──────────────────────── */
uint32_t crc32(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320U;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/* ── Reset handler ────────────────────────────────────────────────── */
void reset_handler(void) {
    /* Copy .data from FLASH to RAM */
    extern uint32_t _sidata, _sdata, _edata;
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;

    /* Zero .bss */
    extern uint32_t _sbss, _ebss;
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;

    /* Initialize system */
    main();
}

/* ── Helpers ──────────────────────────────────────────────────────── */
static void set_state(device_state_t s) {
    g_state = s;
    /* Update LEDs based on state */
    if (s == ST_ARMED || s == ST_CHARGING || s == ST_PULSE) {
        gpio_set(GPIOB, LED_ARMED_PIN);
    } else {
        gpio_clr(GPIOB, LED_ARMED_PIN);
    }
    if (s == ST_CHARGING || (s == ST_PULSE && hv_is_charged())) {
        gpio_set(GPIOD, LED_CHARGE_PIN);
    } else {
        gpio_clr(GPIOD, LED_CHARGE_PIN);
    }
}

static void update_oled(void) {
    const char *mode_str = "?";
    switch (g_mode) {
        case MODE_EMFI:    mode_str = "EMFI"; break;
        case MODE_SCA:     mode_str = "SCA";  break;
        case MODE_SCAN:    mode_str = "SCAN"; break;
        case MODE_MONITOR: mode_str = "MON";  break;
    }
    oled_draw_status((g_state == ST_ARMED || g_state == ST_PULSE),
                     g_battery_pct, mode_str);
    oled_draw_position(g_position.x_mm, g_position.y_mm,
                       g_position.z_mm, g_hv_voltage);
    oled_refresh();
}

/* ── EMFI pulse sequence ──────────────────────────────────────────── */
static int execute_emfi_pulse(void) {
    int rc;

    /* 1. Set HV charge voltage and charge the Marx bank */
    rc = hv_set_voltage(g_emfi.voltage);
    if (rc != 0) {
        set_state(ST_FAULT);
        return rc;
    }

    set_state(ST_CHARGING);
    oled_clear();
    oled_draw_string(0, 0, "CHARGING...", 1);
    oled_refresh();

    rc = hv_charge();
    if (rc != 0) {
        hv_discharge();
        set_state(ST_FAULT);
        return rc;
    }

    /* Wait for charge to settle (safety) */
    uint32_t charge_start = millis();
    while (!hv_is_charged()) {
        if ((millis() - charge_start) > 2000) {
            hv_discharge();
            set_state(ST_FAULT);
            return -1;
        }
    }
    g_hv_voltage = hv_read_actual();

    /* 2. Configure FPGA pulse timing */
    rc = fpga_configure_pulse(g_emfi.width_ns, g_emfi.delay_ns,
                               g_emfi.count, g_emfi.interval_us);
    if (rc != 0) {
        hv_discharge();
        set_state(ST_FAULT);
        return rc;
    }

    /* 3. Arm the FPGA trigger */
    rc = fpga_arm();
    if (rc != 0) {
        hv_discharge();
        set_state(ST_FAULT);
        return rc;
    }

    set_state(ST_PULSE);
    oled_clear();
    oled_draw_string(0, 0, "PULSING", 1);
    oled_refresh();

    /* 4. If manual trigger, fire now. Otherwise wait for trigger. */
    if (g_trig_src == TRIG_MANUAL) {
        rc = fpga_fire();
        if (rc != 0) {
            fpga_disarm();
            hv_discharge();
            set_state(ST_FAULT);
            return rc;
        }
    } else {
        /* Wait for FPGA to report fire complete (via IRQ or polling) */
        uint32_t trig_start = millis();
        uint8_t fpga_stat;
        do {
            fpga_stat = fpga_get_status();
            if (fpga_stat & FPGA_ST_FAULT) {
                fpga_disarm();
                hv_discharge();
                set_state(ST_FAULT);
                return -1;
            }
            if (g_trig_src == TRIG_MANUAL) {
                fpga_fire();
                break;
            }
        } while (!(fpga_stat & FPGA_ST_FIRED) &&
                 (millis() - trig_start) < 5000);
    }

    /* 5. Disarm and discharge */
    fpga_disarm();
    delay_ms(10);
    hv_discharge();
    g_hv_voltage = 0;

    /* 6. Log the event */
    burst_record_t rec = {
        .timestamp = millis(),
        .samples = 0,
        .rate_khz = 0,
        .gain_db = 0,
        .trigger_offset = 0,
        .peak_magnitude = 0,
        .fault = FAULT_UNKNOWN
    };
    sdcard_log_burst(&rec);

    set_state(ST_ANALYZE);
    return 0;
}

/* ── SCA capture sequence ─────────────────────────────────────────── */
static int execute_sca_capture(void) {
    int rc;

    set_state(ST_CAPTURE);
    oled_clear();
    oled_draw_string(0, 0, "CAPURING SCA", 1);
    oled_refresh();

    /* Configure ADC */
    adc_set_range(g_sca.range_5v);
    adc_set_vga_gain(g_sca.gain_db);
    adc_set_filter_cutoff(g_sca.filter_cutoff);
    adc_reset();

    /* Arm trigger */
    trigger_arm();

    /* Start DMA capture into external SRAM */
    rc = adc_start_capture(g_trace_buf, g_sca.samples);
    if (rc != 0) {
        trigger_disarm();
        set_state(ST_FAULT);
        return rc;
    }

    /* Wait for capture completion */
    uint32_t cap_start = millis();
    uint32_t timeout_ms = (g_sca.samples / g_sca.rate_khz) + 1000;
    while (g_capture_done == 0) {
        if ((millis() - cap_start) > timeout_ms) {
            trigger_disarm();
            set_state(ST_FAULT);
            return -1;
        }
    }
    g_trace_samples = g_sca.samples;
    trigger_disarm();

    /* Process trace */
    float features[64];
    rc = sca_process_trace(g_trace_buf, g_trace_samples, features);
    if (rc == 0) {
        /* Save to SD */
        sdcard_save_trace("sca_trace", g_trace_buf, g_trace_samples);
    }

    /* Log */
    burst_record_t rec = {
        .timestamp = millis(),
        .samples = g_trace_samples,
        .rate_khz = g_sca.rate_khz,
        .gain_db = g_sca.gain_db,
        .trigger_offset = 0,
        .peak_magnitude = 0,
        .fault = FAULT_NONE
    };
    sdcard_log_burst(&rec);

    set_state(ST_ANALYZE);
    return 0;
}

/* ── Scan (cartography) sequence ──────────────────────────────────── */
static int execute_scan(void) {
    int rc;

    rc = scan_start(&g_scan);
    if (rc != 0) {
        set_state(ST_FAULT);
        return rc;
    }

    set_state(ST_SCAN);
    oled_clear();
    oled_draw_string(0, 0, "SCANNING...", 1);
    oled_refresh();

    while (!scan_is_complete() && g_state == ST_SCAN) {
        rc = scan_step();
        if (rc != 0) {
            scan_stop();
            hv_discharge();
            set_state(ST_FAULT);
            return rc;
        }

        /* Update OLED each step */
        update_oled();

        /* Check for stop command via BLE/USB */
        ble_process();
        if (g_fault_flag) {
            scan_stop();
            hv_discharge();
            g_fault_flag = 0;
            break;
        }
    }

    scan_stop();
    hv_discharge();
    g_hv_voltage = 0;

    /* Export scan map */
    uint8_t map_buf[512];
    scan_export_map(map_buf, sizeof(map_buf));
    sdcard_log_session((const char *)map_buf, sizeof(map_buf));
    ble_send_scan_map(map_buf, sizeof(map_buf));

    set_state(ST_ANALYZE);
    return 0;
}

/* ── Calibration sequence ─────────────────────────────────────────── */
static int execute_calibration(void) {
    set_state(ST_CALIBRATE);
    oled_clear();
    oled_draw_string(0, 0, "CALIBRATING", 1);
    oled_refresh();

    /* Calibrate fluxgate magnetometer offset */
    fluxgate_calibrate();

    /* Auto-range VGA gain using ambient magnetic noise */
    int16_t dummy[1024];
    adc_set_range(1);
    adc_reset();
    adc_start_capture(dummy, 1024);
    delay_ms(50);

    /* Find optimal gain: increase until ADC is ~70% full-scale */
    uint8_t gain = 0;
    for (gain = 0; gain <= 48; gain += 6) {
        adc_set_vga_gain(gain);
        adc_reset();
        adc_read_burst(dummy, 1024);
        int32_t peak = 0;
        for (int i = 0; i < 1024; i++) {
            int32_t v = dummy[i];
            if (v < 0) v = -v;
            if (v > peak) peak = v;
        }
        if (peak > 23000) break;  /* ~70% of 32767 */
    }
    g_sca.gain_db = gain;
    g_sca.filter_cutoff = 50000;  /* 50 kHz default */

    config_save();
    set_state(ST_IDLE);
    return 0;
}

/* ── Forward declarations (local helpers) ────────────────────────── */
static int strcmp_eq(const char *a, const char *b);
static int format_pos(char *buf, float x, float y, float z);
static int int_to_str(char *buf, int val);

/* ── Command parsing ──────────────────────────────────────────────── */

/* Simple JSON-ish command parser (not a full JSON parser).
 * Commands are key-value pairs: {"cmd":"xxx",...}
 * We extract "cmd" and relevant parameters.
 */

static int parse_str(const char *json, const char *key, char *out, int outlen) {
    char pattern[32];
    int ki = 0;
    pattern[ki++] = '"';
    for (const char *k = key; *k && ki < 28; k++)
        pattern[ki++] = *k;
    pattern[ki++] = '"';
    pattern[ki++] = ':';
    pattern[ki++] = '"';
    pattern[ki] = 0;

    const char *p = json;
    while (*p) {
        const char *m = pattern;
        const char *s = p;
        while (*s && *m && *s == *m) { s++; m++; }
        if (*m == 0) {
            /* Found key, extract value */
            int oi = 0;
            while (*s && *s != '"' && oi < outlen - 1)
                out[oi++] = *s++;
            out[oi] = 0;
            return oi;
        }
        p++;
    }
    return 0;
}

static long parse_num(const char *json, const char *key) {
    char pattern[32];
    int ki = 0;
    pattern[ki++] = '"';
    for (const char *k = key; *k && ki < 28; k++)
        pattern[ki++] = *k;
    pattern[ki++] = '"';
    pattern[ki++] = ':';
    pattern[ki] = 0;

    const char *p = json;
    while (*p) {
        const char *m = pattern;
        const char *s = p;
        while (*s && *m && *s == *m) { s++; m++; }
        if (*m == 0) {
            /* Skip whitespace */
            while (*s == ' ') s++;
            long val = 0;
            int neg = 0;
            if (*s == '-') { neg = 1; s++; }
            while (*s >= '0' && *s <= '9') {
                val = val * 10 + (*s - '0');
                s++;
            }
            return neg ? -val : val;
        }
        p++;
    }
    return 0;
}

static void handle_command(const char *cmd, int len) {
    char cmd_name[32] = {0};
    parse_str(cmd, "cmd", cmd_name, sizeof(cmd_name));

    if (strcmp_eq(cmd_name, "emfi_pulse")) {
        g_mode = MODE_EMFI;
        g_emfi.voltage    = (uint16_t)parse_num(cmd, "voltage");
        g_emfi.width_ns   = (uint16_t)parse_num(cmd, "width_ns");
        g_emfi.delay_ns   = (uint32_t)parse_num(cmd, "delay_ns");
        g_emfi.count      = (uint16_t)parse_num(cmd, "count");
        g_emfi.interval_us = (uint32_t)parse_num(cmd, "interval_us");
        g_trig_src = TRIG_MANUAL;
        if (g_emfi.voltage < 50)  g_emfi.voltage = 50;
        if (g_emfi.voltage > 400) g_emfi.voltage = 400;
        execute_emfi_pulse();
    }
    else if (strcmp_eq(cmd_name, "emfi_arm")) {
        g_mode = MODE_EMFI;
        g_emfi.voltage    = (uint16_t)parse_num(cmd, "voltage");
        g_emfi.width_ns   = (uint16_t)parse_num(cmd, "width_ns");
        g_emfi.delay_ns   = (uint32_t)parse_num(cmd, "delay_ns");
        g_trig_src = TRIG_EXTERNAL;
        set_state(ST_ARMED);
        hv_set_voltage(g_emfi.voltage);
        hv_charge();
        fpga_configure_pulse(g_emfi.width_ns, g_emfi.delay_ns, 1, 0);
        fpga_arm();
    }
    else if (strcmp_eq(cmd_name, "emfi_disarm")) {
        fpga_disarm();
        hv_discharge();
        set_state(ST_IDLE);
    }
    else if (strcmp_eq(cmd_name, "sca_capture")) {
        g_mode = MODE_SCA;
        g_sca.samples    = (uint32_t)parse_num(cmd, "samples");
        g_sca.rate_khz   = (uint16_t)parse_num(cmd, "rate_khz");
        g_sca.gain_db    = (uint8_t)parse_num(cmd, "gain_db");
        g_sca.filter_cutoff = 50000;
        if (g_sca.samples > SCA_MAX_SAMPLES)
            g_sca.samples = SCA_MAX_SAMPLES;
        if (g_sca.rate_khz == 0 || g_sca.rate_khz > ADC_MAX_RATE_KSPS)
            g_sca.rate_khz = ADC_MAX_RATE_KSPS;
        execute_sca_capture();
    }
    else if (strcmp_eq(cmd_name, "scan_start")) {
        g_mode = MODE_SCAN;
        g_scan.x0 = (float)parse_num(cmd, "x0") / 1000.0f;
        g_scan.y0 = (float)parse_num(cmd, "y0") / 1000.0f;
        g_scan.x1 = (float)parse_num(cmd, "x1") / 1000.0f;
        g_scan.y1 = (float)parse_num(cmd, "y1") / 1000.0f;
        g_scan.step_mm = (float)parse_num(cmd, "step_mm") / 1000.0f;
        g_scan.z_height = 0.5f;
        g_scan.pulse.voltage = 300;
        g_scan.pulse.width_ns = 50;
        g_scan.pulse.delay_ns = 1000;
        g_scan.pulse.count = 1;
        g_scan.pulses_per_pt = 3;
        execute_scan();
    }
    else if (strcmp_eq(cmd_name, "scan_stop")) {
        g_fault_flag = 1;
    }
    else if (strcmp_eq(cmd_name, "move")) {
        float x = (float)parse_num(cmd, "x") / 1000.0f;
        float y = (float)parse_num(cmd, "y") / 1000.0f;
        float z = (float)parse_num(cmd, "z") / 1000.0f;
        stepper_move_xyz(x, y, z);
        g_position.x_mm = x;
        g_position.y_mm = y;
        g_position.z_mm = z;
    }
    else if (strcmp_eq(cmd_name, "home")) {
        stepper_home_all();
        g_position.x_mm = 0;
        g_position.y_mm = 0;
        g_position.z_mm = 0;
    }
    else if (strcmp_eq(cmd_name, "calibrate")) {
        execute_calibration();
    }
    else if (strcmp_eq(cmd_name, "set_trigger")) {
        int src = (int)parse_num(cmd, "source");
        if (src >= 0 && src <= 3) {
            g_trig_src = (trigger_source_t)src;
            trigger_set_source(g_trig_src);
        }
    }
    else if (strcmp_eq(cmd_name, "set_gain")) {
        g_sca.gain_db = (uint8_t)parse_num(cmd, "gain_db");
        adc_set_vga_gain(g_sca.gain_db);
    }
    else if (strcmp_eq(cmd_name, "status")) {
        ble_send_status((uint8_t)g_state, g_battery_pct, "ok");
    }
    else if (strcmp_eq(cmd_name, "get_position")) {
        char info[64];
        int n = format_pos(info, g_position.x_mm, g_position.y_mm,
                           g_position.z_mm);
        ble_send_status((uint8_t)g_state, g_battery_pct, info);
        (void)n;
    }
    else if (strcmp_eq(cmd_name, "discharge")) {
        hv_discharge();
        g_hv_voltage = 0;
        set_state(ST_IDLE);
    }
}

/* Minimal strcmp (we don't have libc) */
static int strcmp_eq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == 0 && *b == 0);
}

/* Minimal integer-to-string and float-to-string for position info */
static int format_pos(char *buf, float x, float y, float z) {
    /* Format: "x.xmm y.ymm z.zmm" — simplified integer-only */
    int n = 0;
    buf[n++] = 'X';
    n += int_to_str(buf + n, (int)(x * 100));
    buf[n++] = 'Y';
    n += int_to_str(buf + n, (int)(y * 100));
    buf[n++] = 'Z';
    n += int_to_str(buf + n, (int)(z * 100));
    buf[n] = 0;
    return n;
}

static int int_to_str(char *buf, int val) {
    if (val < 0) { *buf++ = '-'; val = -val; }
    char tmp[12];
    int i = 0;
    if (val == 0) tmp[i++] = '0';
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int n = 0;
    while (i > 0) buf[n++] = tmp[--i];
    buf[n] = 0;
    return n;
}

/* ── ADC DMA completion callback ──────────────────────────────────── */
void adc_dma_complete(void) {
    g_capture_done = 1;
}

/* ── External trigger interrupt (EXTI4) ───────────────────────────── */
void trigger_ext_callback(void) {
    /* The FPGA handles the actual pulse timing.
     * This ISR just notifies the main loop if needed. */
    if (g_state == ST_ARMED) {
        /* FPGA will fire automatically on external trigger */
    }
}

/* ── Arm button handler ───────────────────────────────────────────── */
static void check_arm_button(void) {
    static uint32_t last_press = 0;
    static uint8_t debounced = 1;

    if (gpio_get(GPIOD, BTN_ARM_PIN) == 0) {
        if (debounced && (millis() - last_press) > 200) {
            debounced = 0;
            last_press = millis();
            /* Toggle arm/disarm */
            if (g_state == ST_IDLE) {
                if (g_mode == MODE_EMFI) {
                    set_state(ST_ARMED);
                    hv_set_voltage(g_emfi.voltage);
                    hv_charge();
                    fpga_configure_pulse(g_emfi.width_ns,
                                         g_emfi.delay_ns, 1, 0);
                    fpga_arm();
                }
            } else if (g_state == ST_ARMED) {
                fpga_disarm();
                hv_discharge();
                set_state(ST_IDLE);
            }
        }
    } else {
        debounced = 1;
    }
}

/* ── Safety check ─────────────────────────────────────────────────── */
static int safety_check(void) {
    /* Check limit switches — if Z axis is at max, probe is retracted (safe) */
    /* Check HV voltage is within bounds */
    if (g_hv_voltage > 400) {
        hv_discharge();
        set_state(ST_FAULT);
        return -1;
    }
    /* Check battery level */
    if (g_battery_pct < 10 && g_state != ST_IDLE) {
        hv_discharge();
        set_state(ST_FAULT);
        return -1;
    }
    return 0;
}

/* ── Battery monitoring ───────────────────────────────────────────── */
static void update_battery(void) {
    static uint32_t last_update = 0;
    if ((millis() - last_update) > 5000) {
        last_update = millis();
        /* Read MAX17048 fuel gauge via I2C */
        /* Simplified: just keep current value */
        /* In real impl: I2C read of 0x0B register → percentage */
    }
}

/* ── Main ─────────────────────────────────────────────────────────── */
int main(void) {
    /* 1. Initialize hardware */
    board_init_clock();
    board_init_gpio();
    board_init_fmc_sram();
    board_init_qspi();
    board_init_exti();
    board_init_dma();
    board_init_timers();
    systick_init();

    /* 2. Initialize peripherals */
    oled_init();
    oled_clear();
    oled_draw_string(0, 0, "TESLA-PHANTOM", 2);
    oled_draw_string(0, 16, "booting...", 1);
    oled_refresh();

    config_load();
    g_emfi = g_config.emfi;
    g_sca  = g_config.sca;
    g_trig_src = g_config.trig_src;

    fpga_init();
    hv_init();
    stepper_init();
    fluxgate_init();
    adc_init();
    trigger_init();
    sca_init();
    sdcard_init();
    ble_init();
    usb_init();

    /* 3. Safety: discharge any residual HV */
    hv_discharge();

    oled_clear();
    oled_draw_string(0, 0, "TESLA-PHANTOM", 2);
    oled_draw_string(0, 20, "READY", 1);
    oled_refresh();

    set_state(ST_IDLE);

    /* 4. Main loop */
    while (1) {
        /* Process BLE commands */
        ble_process();

        /* Process USB commands */
        usb_process();

        /* Check for incoming command from BLE */
        uint16_t cmd_len = 0;
        uint8_t cmd_id;
        if (ble_recv_cmd(&cmd_id, g_cmd_buf, &cmd_len) > 0) {
            g_cmd_buf[cmd_len] = 0;
            handle_command(g_cmd_buf, cmd_len);
        }

        /* Also check USB for commands */
        int usb_rx = usb_recv(g_cmd_buf, CMD_BUF_SIZE - 1);
        if (usb_rx > 0) {
            g_cmd_buf[usb_rx] = 0;
            handle_command(g_cmd_buf, usb_rx);
        }

        /* Check arm button */
        check_arm_button();

        /* Safety checks */
        safety_check();

        /* Battery monitoring */
        update_battery();

        /* Periodic status update */
        static uint32_t last_status = 0;
        if ((millis() - last_status) > 1000) {
            last_status = millis();
            ble_send_status((uint8_t)g_state, g_battery_pct, "ok");
            update_oled();
        }

        /* State transitions from ANALYZE → IDLE */
        if (g_state == ST_ANALYZE) {
            delay_ms(100);
            set_state(ST_IDLE);
        }

        /* Fault recovery */
        if (g_state == ST_FAULT) {
            hv_discharge();
            fpga_disarm();
            oled_clear();
            oled_draw_string(0, 0, "FAULT!", 2);
            oled_refresh();
            delay_ms(2000);
            set_state(ST_IDLE);
        }

        /* Brief yield to reduce power */
        __asm volatile ("wfi");
    }

    return 0;
}