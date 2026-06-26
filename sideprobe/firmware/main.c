/*
 * main.c — SideProbe firmware main loop & command dispatcher
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * SideProbe is intentionally RTOS-free: the super-loop polls the BLE/USB
 * command queues, runs the capture/attack state machine, and streams results.
 * This keeps the deterministic timing of the CPA correlation loop free of
 * scheduler jitter — critical for side-channel work.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "registers.h"

/* Driver prototypes (drivers/*.c) */
extern void board_init(void);
extern void fpga_init(void);
extern void adc_init(uint32_t sample_rate_hz);
extern int  adc_arm_trigger(uint8_t src, uint32_t samples_per_trace, uint32_t n_traces);
extern int  adc_capture_one(int16_t *out, uint32_t samples);
extern int  adc_capture_batch(uint32_t n_traces, int16_t *trace_buf);
extern void em_probe_set_modality(uint8_t modality);
extern void em_probe_set_gain_db(int16_t gain_db);
extern int  em_probe_calibrate(void);
extern void target_io_init(void);
extern int  target_send_plaintext(const uint8_t *pt, uint32_t len);
extern int  target_recv_ciphertext(uint8_t *ct, uint32_t len);
extern int  ble_uart_init(void);
extern int  ble_uart_recv(uint8_t *buf, uint32_t maxlen, uint32_t timeout_ms);
extern int  ble_uart_send(const uint8_t *buf, uint32_t len);
extern int  usb_cdc_init(void);
extern int  usb_cdc_recv(uint8_t *buf, uint32_t maxlen, uint32_t timeout_ms);
extern int  usb_cdc_send(const uint8_t *buf, uint32_t len);
extern int  sdcard_init(void);
extern int  sdcard_log_trace(const int16_t *samples, uint32_t nsamp, uint32_t trace_idx);
extern int  sdcard_log_result(const uint8_t *key, uint32_t keylen,
                              const uint8_t *conf, uint32_t n_traces);

/* CPA attack engine (drivers/cpa.c) */
typedef struct {
    uint8_t  model;            /* MODEL_SBOX_OUT / MODEL_HAMMING_WEIGHT ... */
    uint8_t  key_bytes;        /* 16 for AES-128 */
    uint32_t n_traces;
    uint32_t samples_per_trace;
    int16_t *trace_buf;        /* n_traces * samples_per_trace, SDRAM */
    uint8_t *plaintexts;       /* n_traces * 16, SDRAM */
    uint8_t *ciphertexts;      /* n_traces * 16 (optional) */
    /* output */
    uint8_t  recovered_key[32];
    uint8_t  converged[32];     /* 1 if a byte has converged */
    float    corr_best[32];     /* best correlation per byte */
    float    corr_second[32];   /* second-best correlation per byte */
    uint32_t traces_done;
    uint32_t checkpoint;       /* recompute corr every `checkpoint` traces */
} cpa_state_t;

extern int  cpa_init(cpa_state_t *st, uint8_t model, uint8_t key_bytes,
                     uint32_t n_traces, uint32_t samples_per_trace,
                     int16_t *trace_buf, uint8_t *plaintexts);
extern int  cpa_feed_trace(cpa_state_t *st, uint32_t trace_idx);
extern int  cpa_compute(cpa_state_t *st);   /* recompute correlations */
extern int  cpa_converged(const cpa_state_t *st, float margin);

/* OLED (minimal, polled) */
extern void oled_init(void);
extern void oled_clear(void);
extern void oled_draw_text(uint8_t x, uint8_t y, const char *s);
extern void oled_draw_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill);
extern void oled_refresh(void);

/* ---- Global state ---- */
static cpa_state_t g_cpa;
static uint8_t  g_modality = MODALITY_POWER;
static int16_t  g_gain_db = 20;        /* EM LNA gain */
static uint8_t  g_trigger_src = TRIG_SRC_ANALOG;
static uint32_t g_sample_rate = 1000000u;
static uint32_t g_n_traces = TRACES_PER_ATTACK;
static uint8_t  g_model = MODEL_SBOX_OUT;
static float    g_margin_threshold = 3.0f;
static uint8_t  g_plaintext_mode = 0;  /* 0=RANDOM, 1=DRIVE uart, 2=LISTEN */

/* SDRAM-backed buffers (allocated from SDRAM via FMC) */
static int16_t *g_trace_buf = (int16_t *)SDRAM_BASE_ADDR;
static uint8_t *g_pt_buf   = (uint8_t *)(SDRAM_BASE_ADDR + (64u * 1024u * 1024u));
static uint8_t *g_ct_buf    = (uint8_t *)(SDRAM_BASE_ADDR + (80u * 1024u * 1024u));

/* ---- Command protocol (text, line-oriented over BLE/USB) ----
 *
 * Commands:
 *   STATUS                      -> "OK <modality> <gain> <rate> <ntraces> <sd_kb_free>"
 *   SET MODALITY <POWER|EM>
 *   SET GAIN <db>
 *   SET RATE <hz>
 *   SET NTRACES <n>
 *   SET MODEL <SBOX|HW|HD>
 *   SET TRIG <ANALOG|GPIO|UART|FREE>
 *   SET MARGIN <f>
 *   SET PTMODE <RANDOM|DRIVE|LISTEN>
 *   CAL
 *   CAPTURE ONE                  -> streams one trace (downsampled) as hex
 *   ATTACK START                 -> begins CPA, streams per-byte results
 *   ATTACK STOP
 *   ATTACK STATUS                -> progress + partial key
 *   KEY                          -> current best-guess key hex
 *   DUMP <trace_idx>             -> raw 16-bit trace over USB bulk
 *
 * Responses are text lines terminated with \n. Binary bulk goes over a
 * separate USB CDC interface marker (not shown here).
 */
static int parse_uint(const char *s, uint32_t *out) {
    *out = 0;
    while (*s >= '0' && *s <= '9') { *out = *out * 10 + (*s - '0'); s++; }
    return (*s == '\0' || *s == ' ' || *s == '\n' || *s == '\r') ? 0 : -1;
}

static int parse_float(const char *s, float *out) {
    *out = strtof(s, NULL);
    return 0;
}

static void send_line(int (*send)(const uint8_t *, uint32_t), const char *s) {
    send((const uint8_t *)s, (uint32_t)strlen(s));
    send((const uint8_t *)"\n", 1);
}

static void fmt_hex(uint8_t *dst, const uint8_t *src, uint32_t n) {
    static const char hex[] = "0123456789abcdef";
    for (uint32_t i = 0; i < n; i++) {
        dst[2*i]   = hex[src[i] >> 4];
        dst[2*i+1] = hex[src[i] & 0xF];
    }
    dst[2*n] = 0;
}

/* ---- Attack state machine ---- */
static volatile uint8_t g_attack_running = 0;
static volatile uint8_t g_attack_stop_req = 0;

static void attack_run_stream(int (*send)(const uint8_t *, uint32_t)) {
    char line[160];
    uint8_t keyhex[2 * KEY_BYTES_AES128 + 1];

    /* Generate random plaintexts if needed */
    if (g_plaintext_mode == 0) {
        /* Cheap LFSR PRNG seeded from a free-running counter; deterministic
           across power cycles is acceptable for CPA (plaintexts need only be
           known, not random — but variety helps convergence). */
        uint32_t lfsr = 0xDEADBEEFu ^ (SysTick_read() & 0xFFFFFFFFu);
        for (uint32_t i = 0; i < g_n_traces * 16; i += 4) {
            lfsr = (lfsr << 1) | (__builtin_parity(lfsr & 0x80000062u));
            g_pt_buf[i+0] = (lfsr >> 0) & 0xFF;
            g_pt_buf[i+1] = (lfsr >> 8) & 0xFF;
            g_pt_buf[i+2] = (lfsr >> 16) & 0xFF;
            g_pt_buf[i+3] = (lfsr >> 24) & 0xFF;
        }
    }

    if (cpa_init(&g_cpa, g_model, KEY_BYTES_AES128, g_n_traces,
                 TRACE_SAMPLES, g_trace_buf, g_pt_buf) != 0) {
        send_line(send, "ERR cpa_init");
        return;
    }
    g_cpa.checkpoint = 256;
    adc_init(g_sample_rate);
    adc_arm_trigger(g_trigger_src, TRACE_SAMPLES, g_n_traces);

    g_attack_running = 1;
    g_attack_stop_req = 0;

    for (uint32_t t = 0; t < g_n_traces && !g_attack_stop_req; t++) {
        /* If DRIVE mode, push this trace's plaintext to the target */
        if (g_plaintext_mode == 1) {
            if (target_send_plaintext(&g_pt_buf[t * 16], 16) != 0) {
                send_line(send, "ERR pt_send");
                break;
            }
        }
        /* Capture one trace into trace_buf[t] */
        if (adc_capture_one(&g_trace_buf[t * TRACE_SAMPLES],
                            TRACE_SAMPLES) != 0) {
            send_line(send, "ERR capture");
            break;
        }
        /* If DRIVE/LISTEN, optionally read ciphertext back */
        if (g_plaintext_mode != 0) {
            target_recv_ciphertext(&g_ct_buf[t * 16], 16);
        }
        /* Feed hypothesis accumulator */
        cpa_feed_trace(&g_cpa, t);

        /* Periodic checkpoint: recompute correlations, stream progress */
        if (((t + 1) % g_cpa.checkpoint) == 0) {
            cpa_compute(&g_cpa);
            g_cpa.traces_done = t + 1;
            fmt_hex(keyhex, g_cpa.recovered_key, KEY_BYTES_AES128);
            int n = snprintf(line, sizeof(line),
                "PROG %lu key=%s conv=%u margin=%.2f",
                (unsigned long)(t + 1), (char *)keyhex,
                (unsigned)cpa_converged(&g_cpa, g_margin_threshold),
                g_cpa.corr_best[0] / (g_cpa.corr_second[0] + 1e-9f));
            (void)n;
            send_line(send, line);

            /* OLED live update: bar chart of best-correlation per byte */
            oled_clear();
            oled_draw_text(0, 0, "CPA running");
            for (uint8_t b = 0; b < 16; b++) {
                uint8_t fill = (uint8_t)(g_cpa.corr_best[b] * 64.0f);
                if (fill > 64) fill = 64;
                oled_draw_bar((uint8_t)(b * 8), 16, 6, 32, fill);
            }
            oled_refresh();

            /* Early stop if all bytes converged with margin */
            if (cpa_converged(&g_cpa, g_margin_threshold) ==
                KEY_BYTES_AES128) {
                fmt_hex(keyhex, g_cpa.recovered_key, KEY_BYTES_AES128);
                snprintf(line, sizeof(line),
                    "CONVERGED key=%s traces=%lu",
                    (char *)keyhex, (unsigned long)(t + 1));
                send_line(send, line);
                sdcard_log_result(g_cpa.recovered_key, KEY_BYTES_AES128,
                                  g_cpa.converged, t + 1);
                break;
            }
        }
    }

    g_attack_running = 0;
    if (!g_attack_stop_req) {
        cpa_compute(&g_cpa);
        fmt_hex(keyhex, g_cpa.recovered_key, KEY_BYTES_AES128);
        snprintf(line, sizeof(line), "DONE key=%s",
                 (char *)keyhex);
        send_line(send, line);
    } else {
        send_line(send, "STOPPED");
    }
}

/* SysTick helper used above; defined in board_init.c */
extern uint32_t SysTick_read(void);

/* ---- Command handler ---- */
static void handle_command(const char *cmd, int (*send)(const uint8_t *, uint32_t)) {
    char line[160];

    if (strncmp(cmd, "STATUS", 6) == 0) {
        snprintf(line, sizeof(line), "OK modality=%s gain=%d rate=%lu ntraces=%lu model=%u trig=%u",
                 g_modality == MODALITY_POWER ? "POWER" : "EM",
                 (int)g_gain_db, (unsigned long)g_sample_rate,
                 (unsigned long)g_n_traces, g_model, g_trigger_src);
        send_line(send, line);
    } else if (strncmp(cmd, "SET MODALITY ", 13) == 0) {
        if (strstr(cmd, "EM")) { g_modality = MODALITY_EM; em_probe_set_modality(MODALITY_EM); }
        else { g_modality = MODALITY_POWER; em_probe_set_modality(MODALITY_POWER); }
        send_line(send, "OK");
    } else if (strncmp(cmd, "SET GAIN ", 9) == 0) {
        int16_t g = (int16_t)atoi(cmd + 9);
        g_gain_db = g; em_probe_set_gain_db(g);
        send_line(send, "OK");
    } else if (strncmp(cmd, "SET RATE ", 9) == 0) {
        uint32_t r; if (parse_uint(cmd + 9, &r) == 0) { g_sample_rate = r; send_line(send, "OK"); }
        else send_line(send, "ERR");
    } else if (strncmp(cmd, "SET NTRACES ", 12) == 0) {
        uint32_t n; if (parse_uint(cmd + 12, &n) == 0 && n <= TRACE_MAX_IN_SDRAM) {
            g_n_traces = n; send_line(send, "OK");
        } else send_line(send, "ERR");
    } else if (strncmp(cmd, "SET MODEL ", 9) == 0) {
        if (strstr(cmd, "SBOX")) g_model = MODEL_SBOX_OUT;
        else if (strstr(cmd, "HW")) g_model = MODEL_HAMMING_WEIGHT;
        else if (strstr(cmd, "HD")) g_model = MODEL_HAMMING_DISTANCE;
        send_line(send, "OK");
    } else if (strncmp(cmd, "SET TRIG ", 9) == 0) {
        if (strstr(cmd, "ANALOG")) g_trigger_src = TRIG_SRC_ANALOG;
        else if (strstr(cmd, "GPIO")) g_trigger_src = TRIG_SRC_EXT_GPIO;
        else if (strstr(cmd, "UART")) g_trigger_src = TRIG_SRC_UART_SIG;
        else g_trigger_src = TRIG_SRC_FREERUN;
        send_line(send, "OK");
    } else if (strncmp(cmd, "SET MARGIN ", 11) == 0) {
        float m; if (parse_float(cmd + 11, &m) == 0) { g_margin_threshold = m; send_line(send, "OK"); }
        else send_line(send, "ERR");
    } else if (strncmp(cmd, "SET PTMODE ", 11) == 0) {
        if (strstr(cmd, "DRIVE")) g_plaintext_mode = 1;
        else if (strstr(cmd, "LISTEN")) g_plaintext_mode = 2;
        else g_plaintext_mode = 0;
        send_line(send, "OK");
    } else if (strncmp(cmd, "CAL", 3) == 0) {
        int r = em_probe_calibrate();
        snprintf(line, sizeof(line), "CAL %s gain=%d", r == 0 ? "OK" : "FAIL", (int)g_gain_db);
        send_line(send, line);
    } else if (strncmp(cmd, "CAPTURE ONE", 11) == 0) {
        int16_t tmp[TRACE_SAMPLES];
        adc_init(g_sample_rate);
        adc_arm_trigger(g_trigger_src, TRACE_SAMPLES, 1);
        if (adc_capture_one(tmp, TRACE_SAMPLES) == 0) {
            /* Stream a 1-in-8 downsampled preview as hex (256 samples) */
            char hexbuf[2 * 256 + 8];
            uint32_t hp = 0;
            hexbuf[hp++] = 'T'; /* 'T' marker for "trace" */
            for (uint32_t i = 0; i < 256; i++) {
                uint16_t v = (uint16_t)tmp[i * 8] ^ 0x8000u; /* bias to unsigned */
                const char *h = "0123456789abcdef";
                hexbuf[hp++] = h[(v >> 12) & 0xF];
                hexbuf[hp++] = h[(v >> 8) & 0xF];
                hexbuf[hp++] = h[(v >> 4) & 0xF];
                hexbuf[hp++] = h[v & 0xF];
            }
            hexbuf[hp] = 0;
            send_line(send, hexbuf);
        } else send_line(send, "ERR capture");
    } else if (strncmp(cmd, "ATTACK START", 12) == 0) {
        send_line(send, "ACK attack_start");
        attack_run_stream(send);
    } else if (strncmp(cmd, "ATTACK STOP", 11) == 0) {
        g_attack_stop_req = 1;
        send_line(send, "ACK stop_req");
    } else if (strncmp(cmd, "ATTACK STATUS", 13) == 0) {
        snprintf(line, sizeof(line), "STATUS running=%u traces_done=%lu",
                 g_attack_running, (unsigned long)g_cpa.traces_done);
        send_line(send, line);
    } else if (strncmp(cmd, "KEY", 3) == 0) {
        char keyhex[2 * KEY_BYTES_AES128 + 1];
        fmt_hex((uint8_t *)keyhex, g_cpa.recovered_key, KEY_BYTES_AES128);
        snprintf(line, sizeof(line), "KEY %s", keyhex);
        send_line(send, line);
    } else if (strncmp(cmd, "HELP", 4) == 0) {
        send_line(send, "SideProbe v1.0 (jayis1)");
        send_line(send, "Commands: STATUS SET <MODALITY|GAIN|RATE|NTRACES|MODEL|TRIG|MARGIN|PTMODE>");
        send_line(send, "          CAL CAPTURE ONE ATTACK START|STOP|STATUS KEY");
    } else {
        send_line(send, "ERR unknown");
    }
}

/* ---- Main loop ---- */
int main(void) {
    board_init();
    fpga_init();
    target_io_init();
    ble_uart_init();
    usb_cdc_init();
    sdcard_init();
    oled_init();
    em_probe_set_modality(g_modality);
    em_probe_set_gain_db(g_gain_db);

    oled_clear();
    oled_draw_text(0, 0, "SideProbe v1.0");
    oled_draw_text(0, 12, "jayis1");
    oled_refresh();

    static char cmd_buf[160];
    uint32_t cmd_len = 0;

    for (;;) {
        /* Poll BLE first (primary control), then USB CDC (fallback / bulk) */
        uint8_t b;
        int n = ble_uart_recv(&b, 1, 5);
        if (n <= 0) {
            n = usb_cdc_recv(&b, 1, 5);
        }
        if (n > 0) {
            if (b == '\n' || b == '\r') {
                if (cmd_len > 0) {
                    cmd_buf[cmd_len] = 0;
                    int (*snd)(const uint8_t *, uint32_t) =
                        (n == 1 && ble_uart_recv(&b, 0, 0) >= 0) ? ble_uart_send : usb_cdc_send;
                    /* Heuristic: route response to whichever link sent the cmd.
                       (For simplicity we always try BLE first; if it fails, USB.) */
                    handle_command(cmd_buf, ble_uart_send);
                    cmd_len = 0;
                }
            } else if (cmd_len < sizeof(cmd_buf) - 1) {
                cmd_buf[cmd_len++] = (char)b;
            }
        }
        /* Low-power WFI between polls when no bytes arrive quickly */
        if (n <= 0) __asm__ volatile("wfi");
    }
}

/* Weak SysTick stub in case board_init doesn't define one (it does). */
__attribute__((weak)) uint32_t SysTick_read(void) { return 0; }