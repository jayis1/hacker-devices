/*
 * main.c — DRAM-Phantom firmware main loop
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Cooperative scheduler + mode state machine for the DRAM-Phantom MCU.
 * The MCU never issues DDR4 commands itself — it dispatches them to the
 * FPGA over SPI. Every destructive command requires a two-key arm
 * (physical button held >=3s AND a CONFIRM token from the app).
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "registers.h"

/* ---- globals ---- */
static volatile uint32_t g_ms = 0;          /* SysTick ms counter */
static device_mode_t g_mode = MODE_IDLE;
static int g_armed = 0;
static char g_arm_token[ARM_TOKEN_LEN + 1]; /* ASCII token shown on OLED */
static uint32_t g_btn_press_ms = 0;         /* when the mode button went down */
static int g_btn_held_long = 0;

/* Snoop ring (drained by main loop into USB/SD) */
#define SNOOP_RING_CAP 256
static uint8_t g_snoop_ring[64 * SNOOP_RING_CAP]; /* 64 bytes per record */
static volatile uint16_t g_snoop_count = 0;

/* ---- SysTick ---- */
void SysTick_Handler(void) {
    g_ms++;
}

uint32_t millis(void) { return g_ms; }

void delay_ms(uint32_t ms) {
    uint32_t t0 = g_ms;
    while ((g_ms - t0) < ms) { /* spin */ }
}

/* ---- Arm token generator ---- */
/* Cheap LCG PRNG seeded from ADC noise — not cryptographic, just a per-session
 * token shown on the OLED so the operator must read it from the device. */
static uint32_t g_rng = 0x5eed;
static uint32_t rng_next(void) {
    g_rng = (g_rng * 1103515245u + 12345u) & 0x7fffffffu;
    return g_rng;
}

static void arm_token_refresh(void) {
    /* Mix in some ADC noise from the battery sense channel */
    uint32_t noise = adc_read(3);  /* BATT_SENSE */
    g_rng ^= (noise << 16) | (noise ^ 0xa5a5);
    rng_next();
    for (int i = 0; i < ARM_TOKEN_LEN; i++) {
        uint32_t v = rng_next() % 36;
        g_arm_token[i] = (char)(v < 10 ? ('0' + v) : ('A' + v - 10));
    }
    g_arm_token[ARM_TOKEN_LEN] = 0;
}

const char *arm_token_get(void) { return g_arm_token; }

/* ---- Mode button handling ---- */
/* EXTI on PA0. We track press duration in the main loop because we want a
 * "hold >=3s" gesture. */
static void mode_button_poll(void) {
    static int last_down = 0;
    int down = (GPIOA->IDR & (1u<<PA0)) ? 0 : 1; /* active low, pull-up */
    if (down && !last_down) {
        g_btn_press_ms = g_ms;
        g_btn_held_long = 0;
    }
    if (down && !g_btn_held_long && (g_ms - g_btn_press_ms) >= 3000) {
        g_btn_held_long = 1;
        /* Long-press: cycle mode, refresh arm token */
        g_mode = (device_mode_t)((g_mode + 1) % MODE_COUNT);
        arm_token_refresh();
        g_armed = 0;
        fpga_disarm();
        fpga_set_mode(g_mode);
        oled_status(g_mode, 0);
    }
    if (!down && last_down && !g_btn_held_long) {
        /* Short press: toggle armed (only valid in destructive modes) */
        if (g_mode == MODE_ROWHAMMER || g_mode == MODE_WARMBOOT || g_mode == MODE_COVERT) {
            g_armed = !g_armed;
            if (g_armed) {
                fpga_arm(g_arm_token);
            } else {
                fpga_disarm();
            }
            oled_status(g_mode, g_armed ? 1 : 0);
        }
    }
    last_down = down;
}

/* ---- Command dispatch from USB/BLE ----
 * Simple text protocol, line-oriented:
 *   STATUS
 *   MODE <mode>
 *   ARM <token>
 *   DISARM
 *   HAMMER <aggressor_row> <count> <pattern_id>
 *   WARMBOOT <base_row> <row_count>
 *   COVERT <bg> <bank>
 *   SNOOP DRAIN <n>
 *   TOKEN
 */
static char g_cmdline[128];
static uint16_t g_cmdlen = 0;

static void cmd_handle_line(char *line);

static void cmd_feed_byte(uint8_t b) {
    if (b == '\r') return;
    if (b == '\n') {
        g_cmdline[g_cmdlen] = 0;
        if (g_cmdlen > 0) cmd_handle_line(g_cmdline);
        g_cmdlen = 0;
        return;
    }
    if (g_cmdlen < sizeof(g_cmdline) - 1) {
        g_cmdline[g_cmdlen++] = (char)b;
    }
}

static void respond(const char *s) {
    usb_cdc_write(s, (uint16_t)strlen(s));
    ble_uart_write(s, (uint16_t)strlen(s));
}

static void cmd_handle_line(char *line) {
    char *tok = strtok(line, " \t");
    if (!tok) return;

    if (!strcmp(tok, "STATUS")) {
        uint32_t st = fpga_status();
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "OK STATUS mode=%d armed=%d fpga=0x%08lX batt=%lu mv token=%s\r\n",
                 (int)g_mode, g_armed, (unsigned long)st,
                 (unsigned long)adc_read(3), g_arm_token);
        respond(buf);
        return;
    }
    if (!strcmp(tok, "TOKEN")) {
        respond("OK TOKEN ");
        respond(g_arm_token);
        respond("\r\n");
        return;
    }
    if (!strcmp(tok, "MODE")) {
        char *a = strtok(NULL, " \t");
        if (!a) { respond("ERR MODE need arg\r\n"); return; }
        int m = atoi(a);
        if (m < 0 || m >= MODE_COUNT) { respond("ERR bad mode\r\n"); return; }
        g_mode = (device_mode_t)m;
        g_armed = 0;
        fpga_disarm();
        fpga_set_mode(g_mode);
        arm_token_refresh();
        oled_status(g_mode, 0);
        respond("OK MODE set\r\n");
        return;
    }
    if (!strcmp(tok, "ARM")) {
        char *a = strtok(NULL, " \t");
        if (!a) { respond("ERR ARM need token\r\n"); return; }
        if (strlen(a) != ARM_TOKEN_LEN) { respond("ERR ARM bad token length\r\n"); return; }
        if (memcmp(a, g_arm_token, ARM_TOKEN_LEN) != 0) {
            respond("ERR ARM token mismatch\r\n");
            return;
        }
        g_armed = 1;
        fpga_arm(g_arm_token);
        respond("OK ARMED\r\n");
        return;
    }
    if (!strcmp(tok, "DISARM")) {
        g_armed = 0;
        fpga_disarm();
        respond("OK DISARMED\r\n");
        return;
    }
    if (!strcmp(tok, "HAMMER")) {
        if (!g_armed || g_mode != MODE_ROWHAMMER) {
            respond("ERR not armed / wrong mode\r\n"); return;
        }
        char *a1 = strtok(NULL, " \t");
        char *a2 = strtok(NULL, " \t");
        char *a3 = strtok(NULL, " \t");
        if (!a1 || !a2 || !a3) { respond("ERR HAMMER args\r\n"); return; }
        uint32_t row = (uint32_t)strtoul(a1, NULL, 0);
        uint32_t cnt  = (uint32_t)strtoul(a2, NULL, 0);
        uint8_t  pat = (uint8_t)strtoul(a3, NULL, 0);
        if (rowhammer_arm(row, cnt, pat) != 0) {
            respond("ERR HAMMER arm fail\r\n"); return;
        }
        respond("OK HAMMER armed\r\n");
        return;
    }
    if (!strcmp(tok, "HAMMER_RUN")) {
        if (!g_armed || g_mode != MODE_ROWHAMMER) {
            respond("ERR not armed\r\n"); return;
        }
        uint32_t vrow = 0, mask = 0;
        if (rowhammer_run(&vrow, &mask) != 0) {
            respond("ERR HAMMER_RUN exec fail\r\n"); return;
        }
        char buf[80];
        snprintf(buf, sizeof(buf), "OK FLIPS victim=%lu mask=0x%08lX\r\n",
                 (unsigned long)vrow, (unsigned long)mask);
        respond(buf);
        return;
    }
    if (!strcmp(tok, "WARMBOOT")) {
        if (!g_armed || g_mode != MODE_WARMBOOT) {
            respond("ERR not armed / wrong mode\r\n"); return;
        }
        char *a1 = strtok(NULL, " \t");
        char *a2 = strtok(NULL, " \t");
        if (!a1 || !a2) { respond("ERR WARMBOOT args\r\n"); return; }
        uint32_t base = (uint32_t)strtoul(a1, NULL, 0);
        uint32_t nrow = (uint32_t)strtoul(a2, NULL, 0);
        if (mem_capture_start(base, nrow) != 0) {
            respond("ERR WARMBOOT start fail\r\n"); return;
        }
        if (mem_capture_drain_to_sd() != 0) {
            respond("ERR WARMBOOT drain fail\r\n"); return;
        }
        respond("OK WARMBOOT done\r\n");
        return;
    }
    if (!strcmp(tok, "COVERT")) {
        if (!g_armed || g_mode != MODE_COVERT) {
            respond("ERR not armed / wrong mode\r\n"); return;
        }
        char *a1 = strtok(NULL, " \t");
        char *a2 = strtok(NULL, " \t");
        if (!a1 || !a2) { respond("ERR COVERT args\r\n"); return; }
        uint8_t bg = (uint8_t)strtoul(a1, NULL, 0);
        uint8_t bn = (uint8_t)strtoul(a2, NULL, 0);
        uint32_t base_ns = 0;
        if (covert_channel_set(bg, bn, &base_ns) != 0) {
            respond("ERR COVERT cfg fail\r\n"); return;
        }
        char buf[80];
        snprintf(buf, sizeof(buf), "OK COVERT baseline=%lu ns\r\n",
                 (unsigned long)base_ns);
        respond(buf);
        return;
    }
    if (!strcmp(tok, "SNOOP") && (tok = strtok(NULL, " \t")) && !strcmp(tok, "DRAIN")) {
        char *a1 = strtok(NULL, " \t");
        if (!a1) { respond("ERR SNOOP DRAIN n\r\n"); return; }
        uint16_t n = (uint16_t)atoi(a1);
        if (n > SNOOP_RING_CAP) n = SNOOP_RING_CAP;
        uint16_t got = 0;
        if (fpga_drain_snoop(g_snoop_ring, n, &got) != 0) {
            respond("ERR SNOOP drain fail\r\n"); return;
        }
        char hdr[48];
        snprintf(hdr, sizeof(hdr), "OK SNOOP %u\r\n", (unsigned)got);
        respond(hdr);
        /* Send raw record bytes (64 bytes each) */
        uint16_t total = got * 64;
        usb_cdc_write(g_snoop_ring, total);
        ble_uart_write(g_snoop_ring, total);
        return;
    }
    if (!strcmp(tok, "HELP")) {
        respond("Commands: STATUS TOKEN MODE <m> ARM <tok> DISARM "
                "HAMMER <row> <cnt> <pat> HAMMER_RUN WARMBOOT <base> <nrow> "
                "COVERT <bg> <bn> SNOOP DRAIN <n> HELP\r\n");
        return;
    }
    respond("ERR unknown\r\n");
}

/* ---- Per-mode service routines ---- */
static void service_snoop(void) {
    /* Drain any new snoop records from the FPGA into SD if FIFO is > half full.
     * The FPGA raises an interrupt (PB2 falling) when its BRAM FIFO has >= 256
     * records; we drain them in 256-record batches. */
    uint32_t st = fpga_status();
    if (st & FSTAT_FIFO_OVF) {
        /* Overflow — log a marker and continue */
        oled_printf(5, "SNOOP OVF!");
    }
    /* Check FPGA interrupt line */
    if (!(GPIOB->IDR & (1u<<PB2))) {
        uint16_t got = 0;
        if (fpga_drain_snoop(g_snoop_ring, SNOOP_RING_CAP, &got) == 0 && got > 0) {
            /* Write to SD as a CSV trace fragment */
            char name[32];
            snprintf(name, sizeof(name), "trace_%lu.csv", (unsigned long)(g_ms / 1000));
            /* Each record: 64 bytes. We emit a compact CSV: bg,bank,row,col,op,t */
            char csv[64 * SNOOP_RING_CAP + 64];
            int off = 0;
            for (uint16_t i = 0; i < got && off < (int)sizeof(csv) - 64; i++) {
                uint8_t *r = g_snoop_ring + i * 64;
                /* record layout: [bg,bank,row(3),col(2),op,t(4)] = 11 bytes used */
                uint8_t bg = r[0], bank = r[1];
                uint32_t row = r[2] | (r[3]<<8) | (r[4]<<16);
                uint16_t col = r[5] | (r[6]<<8);
                uint8_t op = r[7];
                uint32_t t  = r[8] | (r[9]<<8) | (r[10]<<16) | (r[11]<<24);
                off += snprintf(csv + off, sizeof(csv) - off,
                                "%u,%u,%lu,%u,%u,%lu\r\n",
                                bg, bank, (unsigned long)row, col, op, (unsigned long)t);
            }
            sdcard_write_file(name, csv, (uint32_t)off);
            oled_printf(5, "snoop +%u", (unsigned)got);
        }
    }
}

static void service_rowhammer(void) {
    /* In Rowhammer mode we just display the armed state and the running flip
     * count. The actual hammer is triggered by the HAMMER/HAMMER_RUN commands. */
    uint32_t st = fpga_status();
    if (st & FSTAT_FLIP_DETECTED) {
        uint32_t vrow = 0, mask = 0;
        fpga_drain_flip(&vrow, &mask);
        char buf[80];
        snprintf(buf, sizeof(buf), "FLIP row=%lu mask=0x%08lX\r\n",
                 (unsigned long)vrow, (unsigned long)mask);
        respond(buf);
        oled_printf(5, "FLIP r=%lu", (unsigned long)vrow);
        GPIOA->ODR |= (1u<<PA1);  /* light LED */
    }
}

static void service_warmboot(void) {
    /* In warm-boot mode the MCU's job is to monitor refresh watchdog health
     * and battery state. The drain happens inside the WARMBOOT command. */
    uint32_t st = fpga_status();
    if (!(st & FSTAT_REFRESH_ACTIVE)) {
        respond("WARN refresh not active!\r\n");
        oled_printf(5, "REFRESH DOWN");
    }
    uint32_t batt_mv = adc_read(3);
    if (batt_mv < 3300) {
        respond("WARN battery low\r\n");
        oled_printf(6, "BATT %lumV", (unsigned long)batt_mv);
    }
}

static void service_covert(void) {
    /* Periodically read the timing histogram from the FPGA and report. */
    static uint32_t last_report = 0;
    if ((g_ms - last_report) > 1000) {
        last_report = g_ms;
        uint32_t st = fpga_status();
        char buf[64];
        snprintf(buf, sizeof(buf), "COVERT st=0x%08lX\r\n", (unsigned long)st);
        respond(buf);
    }
}

/* ---- main ---- */
int main(void) {
    board_init();

    oled_init();
    oled_clear();
    oled_printf(0, "DRAM-Phantom");
    oled_printf(1, "jayis1 v1.0");
    oled_printf(3, "boot...");

    fpga_init();
    uint32_t st = fpga_status();
    if (!(st & FSTAT_BITSTREAM_OK)) {
        oled_printf(3, "FPGA NO BIT");
        /* spin — without FPGA we can do nothing */
        for (;;) { delay_ms(1000); }
    }
    if (!(st & FSTAT_DDR_PRESENT)) {
        oled_printf(4, "NO DRAM!");
    } else {
        oled_printf(4, "DRAM OK");
    }

    usb_cdc_init();
    ble_uart_init();
    sdcard_init();

    arm_token_refresh();
    oled_status(g_mode, 0);

    /* Main loop: cooperative. Service current mode + poll comms + button. */
    for (;;) {
        mode_button_poll();

        /* Drain any inbound command bytes from USB and BLE */
        uint8_t rxbuf[64];
        int n;
        while ((n = usb_cdc_read(rxbuf, sizeof(rxbuf))) > 0) {
            for (int i = 0; i < n; i++) cmd_feed_byte(rxbuf[i]);
        }
        while ((n = ble_uart_read(rxbuf, sizeof(rxbuf))) > 0) {
            for (int i = 0; i < n; i++) cmd_feed_byte(rxbuf[i]);
        }

        switch (g_mode) {
            case MODE_SNOOP:    service_snoop();    break;
            case MODE_ROWHAMMER: service_rowhammer(); break;
            case MODE_WARMBOOT: service_warmboot();  break;
            case MODE_COVERT:   service_covert();   break;
            default: break;
        }

        /* Blink LED in idle to show we're alive */
        static uint32_t last_blink = 0;
        if (g_mode == MODE_IDLE && (g_ms - last_blink) > 500) {
            last_blink = g_ms;
            GPIOA->ODR ^= (1u<<PA1);
        }

        /* Yield to keep ISR latency bounded */
        __asm__ volatile ("wfi");
    }
}

/* strtok replacement (avoid pulling in libc variants that aren't freestanding-safe) */
char *strtok(char *s, const char *delim) {
    static char *last;
    if (s) last = s;
    if (!last) return NULL;
    /* skip leading delims */
    while (*last && strchr(delim, *last)) last++;
    if (!*last) { return NULL; }
    char *tok = last;
    while (*last && !strchr(delim, *last)) last++;
    if (*last) { *last = 0; last++; }
    return tok;
}