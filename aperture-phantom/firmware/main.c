/*
 * aperture-phantom / firmware / main.c
 *
 * Main application for the AperturePhantom MIPI CSI-2 interposer control
 * MCU (STM32H743). Implements the operational state machine, the command
 * dispatcher over BLE/USB, the on-device script engine, trigger matching,
 * the CSI-2 fuzz engine, sensor autoconfiguration, and the capture/replay
 * pipeline driven by DMA between the FPGA bridge FIFOs, SDRAM, and SD card.
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Build: see Makefile (arm-none-eabi-gcc, -mcpu=cortex-m7 -mfpu=... )
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "board.h"
#include "registers.h"

/* ---- forward declarations of file-local helpers ---- */
static int  vsnprintf_(char *out, size_t n, const char *fmt, va_list ap);
static void strncpy_(char *d, const char *s, size_t n);
static uint16_t strnlen_(const char *s, size_t n);
static void capture_one_frame(uint8_t slot);
static int  replay_file_index(int idx);
static void script_step(void);
static void handle_cmd(uint8_t op, uint8_t *p, uint16_t len);
static void poll_links(void);
static void touch_handle(void);
static void check_sensor_power(void);
static void fuzz_engine_tick(void);

/* ---- Globals ---- */
device_state_t g_state;
char g_log[LOG_BUF];
static uint8_t  g_inject_slots[INJECT_SLOTS][MAX_INJECT_FRAME_BYTES];
static uint32_t  g_inject_slot_len[INJECT_SLOTS];
static uint8_t   g_script[1024];
static uint16_t  g_script_len;
static volatile uint8_t g_script_running;
static volatile uint8_t g_script_stop_req;
static uint8_t   g_trigger_ref[64 * 3];      /* 16x16 RGB reference */
static uint16_t  g_trigger_ref_len;
static uint8_t   g_trigger_armed;
static uint16_t  g_trigger_threshold = 32;   /* histogram-diff threshold */
static uint8_t   g_fuzz_strategy;
static uint16_t  g_fuzz_count, g_fuzz_delay;
static volatile uint8_t g_fuzz_running, g_fuzz_stop_req;

/* Capture ring in SDRAM: three frame slots */
#define CAP_SLOTS 3
static uint32_t g_cap_slot_off[CAP_SLOTS];
static uint32_t g_cap_slot_len[CAP_SLOTS];
static uint8_t  g_cap_slot_idx;

/* ---- SysTick (1 ms tick) ---- */
static volatile uint32_t g_ticks_ms;
void SysTick_Handler(void) { g_ticks_ms++; }
static uint32_t millis(void) { return g_ticks_ms; }
static void delay_ms(uint32_t ms) {
    uint32_t t0 = g_ticks_ms;
    while ((g_ticks_ms - t0) < ms) __asm__("wfi");
}

/* ---- Tiny printf-style log (writes to g_log + BLE log + uart) ---- */
static void log_putchar(char c) { (void)c; }
void logf(const char *fmt, ...) {
    /* minimal: copy format into g_log and ship out BLE/UART.
     * Real build links a tiny vf; here we ship a fixed-format fallback. */
    static uint8_t idx;
    va_list ap; va_start(ap, fmt);
    /* use arm-none-eabi's vsnprintf if linked; else truncate */
    vsnprintf_(g_log, LOG_BUF, fmt, ap);
    va_end(ap);
    ble_notify_log(g_log);
    uart_puts(g_log); uart_puts("\r\n");
    (void)idx;
}
/* If newlib nano isn't linked, provide a stub vsnprintf-like wrapper:
 * we only need it for status lines, so use a hand-rolled converter. */
static int vsnprintf_(char *out, size_t n, const char *fmt, va_list ap) {
    char *o = out; size_t left = n; (void)left;
    const char *f = fmt;
    while (*f && (size_t)(o - out) < n) {
        if (*f != '%') { *o++ = *f++; continue; }
        f++;
        if (*f == 's') {
            const char *s = va_arg(ap, const char *);
            while (*s && (size_t)(o - out) < n) *o++ = *s++;
        } else if (*f == 'u') {
            uint32_t v = va_arg(ap, uint32_t);
            char tmp[12]; int k=0;
            if (!v) tmp[k++]='0';
            while (v) { tmp[k++] = '0'+(v%10); v/=10; }
            while (k--) *o++ = tmp[k];
        } else if (*f == 'x') {
            uint32_t v = va_arg(ap, uint32_t);
            const char *hex = "0123456789abcdef";
            for (int i = 28; i >= 0; i -= 4) {
                uint8_t nyb = (v >> i) & 0xF;
                if (nyb || i == 0 || (v >> (i+4))) *o++ = hex[nyb];
            }
        } else if (*f == 'd') {
            int32_t v = va_arg(ap, int32_t);
            if (v < 0) { *o++ = '-'; v = -v; }
            char tmp[12]; int k=0;
            if (!v) tmp[k++]='0';
            while (v) { tmp[k++] = '0'+(v%10); v/=10; }
            while (k--) *o++ = tmp[k];
        }
        f++;
    }
    *o = 0;
    return (int)(o - out);
}

/* ---- Status LED helper ---- */
static void led_status(void) {
    if (!g_state.armed) board_led_set(0, 1, 0);            /* green idle */
    else if (g_state.mode == MODE_CAPTURE) board_led_set(0, 0, 1);
    else if (g_state.mode == MODE_INJECT) board_led_set(1, 0, 0);
    else if (g_state.mode == MODE_REPLAY) board_led_set(1, 0, 1);
    else if (g_state.mode == MODE_FUZZ)   board_led_set(1, 1, 0);
    else board_led_set(0, 1, 0);
}

/* ===================================================================== *
 * Trigger matching: 16x16 downsampled RGB histogram diff vs reference.  *
 * The FPGA delivers one downscaled thumbnail per frame; we compute a    *
 * per-channel 16-bin histogram and compare to the reference with an L1   *
 * distance. If the distance exceeds g_trigger_threshold, the trigger     *
 * fires (meaning "scene changed enough") — used by IF_TRIGGER in scripts*
 * and by low-and-slow inject (only act when something moves into ROI).  *
 * ===================================================================== */
void trigger_set_reference(const uint8_t *downscaled, uint16_t len) {
    if (len > sizeof(g_trigger_ref)) len = sizeof(g_trigger_ref);
    memcpy(g_trigger_ref, downscaled, len);
    g_trigger_ref_len = len;
    g_trigger_armed = 1;
    logf("trigger ref set len=%u\r\n", len);
}

int trigger_match(const uint8_t *frame_down, uint16_t len) {
    if (!g_trigger_armed || !g_trigger_ref_len) return 0;
    uint16_t n = MIN(len, g_trigger_ref_len) / 3; /* RGB triplets */
    if (n == 0) return 0;
    static uint16_t hist_ref[3][16];
    static uint16_t hist_now[3][16];
    static uint8_t have_ref;
    if (!have_ref) {
        memset(hist_ref, 0, sizeof(hist_ref));
        for (uint16_t i = 0; i < n; i++) {
            for (int c = 0; c < 3; c++) {
                uint8_t v = g_trigger_ref[i*3 + c] >> 4; /* 0..15 */
                hist_ref[c][v]++;
            }
        }
        have_ref = 1;
    }
    memset(hist_now, 0, sizeof(hist_now));
    for (uint16_t i = 0; i < n; i++) {
        for (int c = 0; c < 3; c++) {
            uint8_t v = frame_down[i*3 + c] >> 4;
            hist_now[c][v]++;
        }
    }
    uint32_t dist = 0;
    for (int c = 0; c < 3; c++)
        for (int b = 0; b < 16; b++) {
            int d = (int)hist_now[c][b] - (int)hist_ref[c][b];
            if (d < 0) d = -d;
            dist += (uint32_t)d;
        }
    dist /= n;  /* normalize per-pixel */
    return dist > g_trigger_threshold;
}

/* ===================================================================== *
 * Capture pipeline: drain FPGA capture FIFO into SDRAM slot, then to SD *
 * ===================================================================== */
static void capture_one_frame(uint8_t slot) {
    uint32_t off = g_cap_slot_off[slot];
    uint32_t max = 1920u * 1080u; /* worst case */
    uint32_t got = fpga_capture_drain((uint8_t *)(SDRAM_BANK_ADDR + off), max);
    g_cap_slot_len[slot] = got;
    if (got == 0) {
        logf("cap slot %u empty\r\n", slot);
        return;
    }
    g_state.frame_count++;
    g_state.crc_errs = fpga_crc_err_count();
    /* Persist to SD if armed and in CAPTURE mode */
    if (g_state.armed && g_state.mode == MODE_CAPTURE) {
        char name[16];
        /* name: CAPnnnnn.APF */
        uint32_t n = g_state.frame_count & 0xFFFFF;
        for (int i = 4; i >= 0; i--) {
            name[i] = '0' + (n % 10); n /= 10;
        }
        name[5] = '.'; name[6]='A'; name[7]='P'; name[8]='F'; name[9]=0;
        if (sdcard_open_write(name) == 0) {
            apf_header_t hdr;
            memset(&hdr, 0, sizeof(hdr));
            hdr.magic[0]=APF_MAGIC0; hdr.magic[1]=APF_MAGIC1;
            hdr.magic[2]=APF_MAGIC2; hdr.version=APF_VERSION;
            /* approximate width/height from data type & length: assume 640x480 default */
            hdr.width = 640; hdr.height = 480;
            hdr.data_type = g_state.data_type;
            hdr.virtual_channel = 0;
            hdr.ts_ms = millis();
            hdr.payload_len = got;
            sdcard_write((uint8_t *)&hdr, sizeof(hdr));
            sdcard_write((uint8_t *)(SDRAM_BANK_ADDR + off), got);
            sdcard_close();
        }
    }
}

/* ===================================================================== *
 * Replay pipeline: read .APF file from SD, stream into FPGA inject FIFO *
 * ===================================================================== */
static int replay_file_index(int idx) {
    char names[MAX_REPLAY_FILES][24];
    int n = sdcard_list_replay(names, MAX_REPLAY_FILES);
    if (idx < 0 || idx >= n) { logf("replay idx oob n=%u\r\n", n); return -1; }
    if (sdcard_open_read(names[idx]) != 0) { logf("open %s fail\r\n", names[idx]); return -1; }
    apf_header_t hdr;
    if (sdcard_read((uint8_t *)&hdr, sizeof(hdr)) != 0) { logf("hdr read fail\r\n"); return -1; }
    if (hdr.magic[0] != APF_MAGIC0 || hdr.magic[1] != APF_MAGIC1 ||
        hdr.magic[2] != APF_MAGIC2) { logf("bad magic\r\n"); return -1; }
    fpga_write_reg(FPGA_REG_TX_WIDTH, hdr.width);
    fpga_write_reg(FPGA_REG_TX_HEIGHT, hdr.height);
    fpga_write_reg(FPGA_REG_TX_DTYPE, hdr.data_type);
    fpga_write_reg(FPGA_REG_TX_VC, hdr.virtual_channel);
    /* Stream in 8 KB chunks */
    static uint8_t chunk[8192];
    uint32_t remain = hdr.payload_len;
    while (remain) {
        uint32_t want = MIN(remain, sizeof(chunk));
        if (sdcard_read(chunk, want) != 0) { logf("payload read fail\r\n"); return -1; }
        if (fpga_inject_load(chunk, want) < 0) { logf("inj fifo full\r\n"); return -1; }
        remain -= want;
    }
    sdcard_close();
    fpga_inject_trigger();
    return 0;
}

/* ===================================================================== *
 * Sensor autoconfig: probe common camera sensors on CCI side-A         *
 * ===================================================================== */
struct sensor_id {
    uint8_t addr;
    uint16_t id_reg;
    uint16_t id_val;
    const char *name;
};
static const struct sensor_id sensors[] = {
    {0x10, 0x0000, 0x219, "IMX219" },  /* RPi v1 (actually OV5647 below) */
    {0x36, 0x300A, 0x5647, "OV5647" }, /* RPi v1 */
    {0x3C, 0x300A, 0x5640, "OV5640" },
    {0x6C, 0x300A, 0x5645, "OV5645" },
    {0x10, 0x0016, 0x477, "IMX477" },  /* RPi HQ */
    {0x10, 0x0016, 0x029, "IMX296" },
    {0x36, 0x300A, 0x9760, "OV9760" },
    {0x10, 0x0016, 0x027, "IMX273" }
};

void sensor_autoconfig(void) {
    g_state.sensor_addr = 0;
    g_state.sensor_name[0] = 0;
    for (int i = 0; i < (int)(sizeof(sensors)/sizeof(sensors[0])); i++) {
        uint16_t v = 0;
        if (cci_read(CCI_SIDE_A, sensors[i].addr, sensors[i].id_reg, &v) == 0) {
            if (v == sensors[i].id_val ||
                (v & 0x0FFF) == (sensors[i].id_val & 0x0FFF)) {
                g_state.sensor_addr = sensors[i].addr;
                strncpy_(g_state.sensor_name, sensors[i].name, sizeof(g_state.sensor_name));
                logf("sensor: %s @0x%02x id=0x%03x\r\n",
                     sensors[i].name, sensors[i].addr, v);
                return;
            }
        }
    }
    /* fallback: scan */
    uint8_t found[16]; int n = cci_scan(CCI_SIDE_A, found, 16);
    if (n > 0) {
        g_state.sensor_addr = found[0];
        strncpy_(g_state.sensor_name, "UNKNOWN", sizeof(g_state.sensor_name));
        logf("sensor: unknown @0x%02x (scan found %u)\r\n", found[0], n);
    } else {
        logf("sensor: none found\r\n");
    }
}

static void strncpy_(char *d, const char *s, size_t n) {
    size_t i = 0; while (s[i] && i+1 < n) { d[i] = s[i]; i++; }
    d[i] = 0;
}

/* ===================================================================== *
 * CSI-2 fuzz engine: emit malformed packets toward the host receiver    *
 * to find parsing bugs in the ISP/camera kernel driver.                 *
 * Strategies (bitmap):                                                  *
 *   0x01 short line         (truncate line before LE)                   *
 *   0x02 bad data type      (DT = 0xFF)                                  *
 *   0x04 bad VC             (VC = 3 on a 1-VC sensor)                    *
 *   0x08 oversize payload   (line > 8192 bytes)                         *
 *   0x10 bad CRC            (header CRC inverted)                        *
 *   0x20 missing FS         (no frame-start short packet)                *
 *   0x40 missing FE         (no frame-end)                              *
 *   0x80 stray short packet (random generic short mid-frame)            *
 * ===================================================================== */
void fuzz_engine_run(uint8_t strategy, uint16_t count, uint16_t delay_ms) {
    g_fuzz_strategy = strategy;
    g_fuzz_count = count;
    g_fuzz_delay = delay_ms;
    g_fuzz_running = 1;
    g_fuzz_stop_req = 0;
    logf("fuzz start strat=0x%02x count=%u delay=%u\r\n", strategy, count, delay_ms);
}
void fuzz_engine_stop(void) { g_fuzz_stop_req = 1; }

static void fuzz_engine_tick(void) {
    if (!g_fuzz_running) return;
    static uint16_t idx;
    static uint32_t next_at;
    if (g_fuzz_stop_req) {
        fpga_set_mode(MODE_PASSTHROUGH);
        g_fuzz_running = 0;
        logf("fuzz stop @ %u/%u\r\n", idx, g_fuzz_count);
        return;
    }
    if (idx >= g_fuzz_count) {
        fpga_set_mode(MODE_PASSTHROUGH);
        g_fuzz_running = 0;
        logf("fuzz done %u\r\n", g_fuzz_count);
        return;
    }
    if (millis() < next_at) return;

    /* Build a small synthetic frame, then apply requested fuzz corruption. */
    static uint8_t frame[640 * 480];  /* in SDRAM region, statically placed */
    /* fill with a gradient so the ISP has something to chew on */
    for (uint32_t i = 0; i < sizeof(frame); i++)
        frame[i] = (uint8_t)(i & 0xFF);

    uint8_t ctrl_mask = 0;
    if (g_fuzz_strategy & 0x01) ctrl_mask |= FPGA_CTRL_TX_SHORT_LINE;
    if (g_fuzz_strategy & 0x02) ctrl_mask |= FPGA_CTRL_TX_BAD_DT;
    if (g_fuzz_strategy & 0x08) ctrl_mask |= FPGA_CTRL_TX_OVERSIZE;
    if (g_fuzz_strategy & 0x10) ctrl_mask |= FPGA_CTRL_TX_CRC_BAD;
    fpga_set_fuzz_bits(ctrl_mask);
    if (g_fuzz_strategy & 0x04) fpga_write_reg(FPGA_REG_TX_VC, 3);
    else                        fpga_write_reg(FPGA_REG_TX_VC, 0);

    fpga_write_reg(FPGA_REG_TX_WIDTH,  640);
    fpga_write_reg(FPGA_REG_TX_HEIGHT, 480);
    fpga_write_reg(FPGA_REG_TX_DTYPE,  CSI_DT_YUV422_8);
    fpga_inject_load(frame, sizeof(frame));
    if ((g_fuzz_strategy & 0x20) == 0) {  /* if not "missing FS", strobe */
        fpga_inject_trigger();
    }
    if (g_fuzz_strategy & 0x40) {
        /* intentionally skip FE by triggering twice without re-load */
    }
    if (g_fuzz_strategy & 0x80) {
        /* extra stray short packet: write trigger with bad DT then a real one */
        fpga_write_reg(FPGA_REG_TX_DTYPE, CSI_SP_GEN_SHORT_1);
        fpga_inject_trigger();
    }
    idx++;
    next_at = millis() + g_fuzz_delay;
    logf("fuzz %u/%u\r\n", idx, g_fuzz_count);
}

/* ===================================================================== *
 * Script engine: bytecode interpreter for phone-less autonomous ops     *
 * ===================================================================== */
static uint16_t pc;
static uint8_t *bp;
static uint16_t blen;
static uint16_t loop_stack[8];
static uint16_t loop_count[8];
static int loop_sp;

void script_engine_run(const uint8_t *bc, uint16_t len) {
    memcpy(g_script, bc, MIN(len, sizeof(g_script)));
    g_script_len = MIN(len, sizeof(g_script));
    bp = g_script; blen = g_script_len;
    pc = 0; loop_sp = 0; g_script_running = 1; g_script_stop_req = 0;
    logf("script run len=%u\r\n", g_script_len);
}
void script_engine_stop(void) { g_script_stop_req = 1; }
int  script_engine_running(void) { return g_script_running; }

static uint16_t rd_u16(void) {
    uint16_t v = (uint16_t)((bp[pc] << 8) | bp[pc+1]);
    return v;
}
static uint8_t rd_u8(void) { return bp[pc]; }

static void script_step(void) {
    if (!g_script_running) return;
    if (g_script_stop_req) {
        g_script_running = 0;
        fpga_set_mode(MODE_PASSTHROUGH);
        logf("script stop @pc=%u\r\n", pc);
        return;
    }
    if (pc >= blen) { g_script_running = 0; logf("script end\r\n"); return; }
    uint8_t op = bp[pc++];
    switch (op) {
    case OP_NOP: break;
    case OP_WAIT_MS: { uint16_t ms = rd_u16(); pc += 2; delay_ms(ms); break; }
    case OP_WAIT_FRAMES: {
        uint16_t cnt = rd_u16(); pc += 2;
        uint32_t start = fpga_frame_count();
        while ((fpga_frame_count() - start) < cnt && !g_script_stop_req)
            __asm__("wfi");
        break;
    }
    case OP_CAPTURE: {
        uint8_t slot = rd_u8(); pc++;
        uint16_t cnt = rd_u16(); pc += 2;
        fpga_set_mode(MODE_CAPTURE);
        for (uint16_t i = 0; i < cnt && !g_script_stop_req; i++) {
            while (fpga_capture_level() == 0 && !g_script_stop_req) __asm__("wfi");
            capture_one_frame(slot % CAP_SLOTS);
        }
        break;
    }
    case OP_INJECT: {
        uint8_t slot = rd_u8(); pc++;
        if (slot < INJECT_SLOTS && g_inject_slot_len[slot]) {
            fpga_set_mode(MODE_INJECT);
            fpga_write_reg(FPGA_REG_TX_DTYPE, g_state.data_type);
            fpga_inject_load(g_inject_slots[slot], g_inject_slot_len[slot]);
            fpga_inject_trigger();
        }
        break;
    }
    case OP_REPLAY: {
        uint8_t idx = rd_u8(); pc++;
        uint16_t loops = rd_u16(); pc += 2;
        fpga_set_mode(MODE_REPLAY);
        for (uint16_t i = 0; i < loops && !g_script_stop_req; i++) {
            if (replay_file_index(idx) != 0) break;
            delay_ms(33); /* ~30fps */
        }
        break;
    }
    case OP_SENSOR_WRITE: {
        uint8_t reg = rd_u8(); pc++;
        uint16_t val = rd_u16(); pc += 2;
        cci_write(CCI_SIDE_A, g_state.sensor_addr, reg, val);
        break;
    }
    case OP_SENSOR_READ: {
        uint8_t reg = rd_u8(); pc++;
        uint8_t slot = rd_u8(); pc++;
        uint16_t v = 0;
        cci_read(CCI_SIDE_A, g_state.sensor_addr, reg, &v);
        if (slot < INJECT_SLOTS) {
            /* stash low byte into inject slot metadata area */
            g_inject_slots[slot][0] = (uint8_t)v;
            g_inject_slots[slot][1] = (uint8_t)(v >> 8);
        }
        break;
    }
    case OP_SET_MODE: {
        uint8_t m = rd_u8(); pc++;
        fpga_set_mode((op_mode_t)m);
        g_state.mode = (op_mode_t)m;
        break;
    }
    case OP_IF_TRIGGER: {
        uint8_t slot = rd_u8(); pc++;
        /* peek next downscaled frame from capture FIFO and test trigger */
        uint8_t thumb[64 * 3]; uint16_t tl = fpga_capture_drain(thumb, sizeof(thumb));
        int fired = trigger_match(thumb, tl);
        if (!fired) {
            /* skip the next single instruction */
            pc += 1; /* skip opcode byte */
            uint8_t nop = bp[pc-1];
            /* rough: skip a couple args by op type */
            switch (nop) {
            case OP_WAIT_MS: case OP_WAIT_FRAMES: case OP_CAPTURE:
            case OP_REPLAY: case OP_SENSOR_WRITE: case OP_SENSOR_READ:
                pc += 2; break;
            default: pc += 1; break;
            }
        }
        break;
    }
    case OP_TRIGGER_ARM:   g_trigger_armed = 1; break;
    case OP_TRIGGER_DISARM:g_trigger_armed = 0; break;
    case OP_BLE_NOTIFY: { uint8_t code = rd_u8(); pc++; ble_send_cmd(CMD_LOG, &code, 1); break; }
    case OP_LOG: { uint8_t id = rd_u8(); pc++; logf("script log %u\r\n", id); break; }
    case OP_LED: { uint8_t c = rd_u8(); pc++;
        switch (c) { case 0: board_led_set(0,0,0); break;
                     case 1: board_led_set(1,0,0); break;
                     case 2: board_led_set(0,1,0); break;
                     case 3: board_led_set(0,0,1); break; }
        break;
    }
    case OP_MOTOR_PULSE: { uint8_t ms = rd_u8(); pc++; board_motor_pulse(ms); break; }
    case OP_LOOP: {
        uint16_t cnt = rd_u16(); pc += 2;
        if (loop_sp < 8) {
            loop_stack[loop_sp] = pc;        /* address after LOOP */
            loop_count[loop_sp] = cnt;       /* 0 = infinite */
            loop_sp++;
        }
        break;
    }
    case OP_END_LOOP: {
        if (loop_sp > 0) {
            uint16_t remain = loop_count[loop_sp-1];
            if (remain == 0 || remain > 1) {
                if (remain > 1) loop_count[loop_sp-1] = remain - 1;
                pc = loop_stack[loop_sp-1];
            } else {
                loop_sp--;
            }
        }
        break;
    }
    case OP_JUMP: { uint16_t a = rd_u16(); pc = a; break; }
    case OP_HALT: g_script_running = 0; logf("script halt\r\n"); break;
    default: logf("script bad op 0x%02x @%u\r\n", op, pc-1); g_script_running = 0; break;
    }
}

/* ===================================================================== *
 * Command dispatcher: parse framed commands from BLE / USB              *
 * ===================================================================== */
static void handle_cmd(uint8_t op, uint8_t *p, uint16_t len) {
    switch (op) {
    case CMD_PING:
        ble_send_cmd(CMD_PING, (const uint8_t *)"AP", 2);
        break;
    case CMD_GET_STATUS:
        ble_notify_status(fpga_status(), fpga_frame_count(),
                          fpga_crc_err_count(), (uint8_t)g_state.mode,
                          g_state.armed);
        break;
    case CMD_GET_LINK_INFO: {
        uint8_t info[8];
        info[0] = g_state.lanes;
        info[1] = (uint8_t)(g_state.rate_mbps_x10 >> 8);
        info[2] = (uint8_t)(g_state.rate_mbps_x10 & 0xFF);
        info[3] = g_state.data_type;
        info[4] = g_state.sensor_addr;
        info[5] = g_state.battery_pct;
        info[6] = (uint8_t)(g_state.sd_free_kb >> 8);
        info[7] = (uint8_t)(g_state.sd_free_kb & 0xFF);
        ble_send_cmd(CMD_GET_LINK_INFO, info, 8);
        break;
    }
    case CMD_SET_MODE: {
        uint8_t m = p[0];
        g_state.mode = (op_mode_t)m;
        fpga_set_mode((op_mode_t)m);
        led_status();
        logf("mode=%u\r\n", m);
        break;
    }
    case CMD_ARM: {
        g_state.armed = p[0] ? 1 : 0;
        led_status();
        if (g_state.armed) board_motor_pulse(40);
        break;
    }
    case CMD_CAP_START:
        fpga_start_capture();
        logf("cap start\r\n");
        break;
    case CMD_CAP_STOP:
        fpga_stop_capture();
        logf("cap stop\r\n");
        break;
    case CMD_CAP_SNAPSHOT: {
        capture_one_frame(g_cap_slot_idx);
        g_cap_slot_idx = (g_cap_slot_idx + 1) % CAP_SLOTS;
        break;
    }
    case CMD_CAP_STREAM: {
        /* send a downscaled frame over BLE in 128-byte chunks */
        static uint8_t thumb[256];
        uint32_t n = fpga_capture_drain(thumb, sizeof(thumb));
        if (n) ble_send_cmd(CMD_FRAME, thumb, (uint16_t)n);
        break;
    }
    case CMD_INJECT_LOAD: {
        uint8_t slot = p[0];
        if (slot >= INJECT_SLOTS) { ble_send_cmd(CMD_ERROR, 0, 0); break; }
        uint32_t plen = len - 1;
        if (plen > MAX_INJECT_FRAME_BYTES) plen = MAX_INJECT_FRAME_BYTES;
        memcpy(g_inject_slots[slot], p + 1, plen);
        g_inject_slot_len[slot] = plen;
        logf("inject slot %u len=%u\r\n", slot, plen);
        break;
    }
    case CMD_INJECT_NOW: {
        uint8_t slot = p[0];
        if (slot < INJECT_SLOTS && g_inject_slot_len[slot]) {
            g_state.mode = MODE_INJECT; fpga_set_mode(MODE_INJECT); led_status();
            fpga_inject_load(g_inject_slots[slot], g_inject_slot_len[slot]);
            fpga_inject_trigger();
        }
        break;
    }
    case CMD_INJECT_LOOP: {
        uint8_t slot = p[0]; uint16_t period = (p[1] << 8) | p[2];
        g_state.mode = MODE_INJECT; fpga_set_mode(MODE_INJECT); led_status();
        for (int i = 0; i < 60 && !g_fuzz_stop_req; i++) { /* demo: 60 frames */
            if (slot < INJECT_SLOTS && g_inject_slot_len[slot]) {
                fpga_inject_load(g_inject_slots[slot], g_inject_slot_len[slot]);
                fpga_inject_trigger();
            }
            delay_ms(period);
        }
        break;
    }
    case CMD_INJECT_STOP:
        fpga_set_mode(MODE_PASSTHROUGH);
        g_state.mode = MODE_PASSTHROUGH; led_status();
        break;
    case CMD_REPLAY_LIST: {
        char names[MAX_REPLAY_FILES][24];
        int n = sdcard_list_replay(names, MAX_REPLAY_FILES);
        /* send count + names as one payload */
        uint8_t buf[1 + MAX_REPLAY_FILES * 24];
        buf[0] = (uint8_t)n;
        for (int i = 0; i < n; i++) memcpy(buf + 1 + i*24, names[i], 24);
        ble_send_cmd(CMD_REPLAY_LIST, buf, (uint16_t)(1 + n*24));
        break;
    }
    case CMD_REPLAY_START: {
        uint8_t idx = p[0];
        g_state.mode = MODE_REPLAY; fpga_set_mode(MODE_REPLAY); led_status();
        replay_file_index(idx);
        break;
    }
    case CMD_REPLAY_STOP:
        fpga_set_mode(MODE_PASSTHROUGH);
        g_state.mode = MODE_PASSTHROUGH; led_status();
        break;
    case CMD_SCRIPT_LOAD: {
        script_engine_stop();
        script_engine_run(p, len);
        break;
    }
    case CMD_SCRIPT_RUN:
        script_engine_run(bp, blen);
        break;
    case CMD_SCRIPT_STOP:
        script_engine_stop();
        break;
    case CMD_SCRIPT_STATUS: {
        uint8_t s[2] = { g_script_running, (uint8_t)(pc >> 8) };
        ble_send_cmd(CMD_SCRIPT_STATUS, s, 2);
        break;
    }
    case CMD_SENSOR_SCAN: {
        uint8_t found[16];
        int n = cci_scan(CCI_SIDE_A, found, 16);
        uint8_t buf[1 + 16]; buf[0] = (uint8_t)n;
        memcpy(buf + 1, found, (uint16_t)n);
        ble_send_cmd(CMD_SENSOR_SCAN, buf, (uint16_t)(1 + n));
        break;
    }
    case CMD_SENSOR_READ: {
        uint8_t addr = p[0]; uint16_t reg = (p[1] << 8) | p[2]; uint8_t l = p[3];
        uint8_t buf[16]; uint16_t v;
        if (l == 2) {
            cci_read(CCI_SIDE_A, addr, reg, &v);
            buf[0] = v & 0xFF; buf[1] = v >> 8;
            ble_send_cmd(CMD_SENSOR_READ, buf, 2);
        } else {
            cci_read_block(CCI_SIDE_A, addr, reg, buf, l);
            ble_send_cmd(CMD_SENSOR_READ, buf, l);
        }
        break;
    }
    case CMD_SENSOR_WRITE: {
        uint8_t addr = p[0]; uint16_t reg = (p[1] << 8) | p[2]; uint16_t val = (p[3] << 8) | p[4];
        cci_write(CCI_SIDE_A, addr, reg, val);
        break;
    }
    case CMD_SENSOR_AUTOCONFIG:
        sensor_autoconfig();
        ble_send_cmd(CMD_SENSOR_AUTOCONFIG, (uint8_t*)g_state.sensor_name,
                     (uint16_t)strnlen_(g_state.sensor_name, sizeof(g_state.sensor_name)));
        break;
    case CMD_FUZZ_START:
        fuzz_engine_run(p[0], (p[1] << 8) | p[2], (p[3] << 8) | p[4]);
        break;
    case CMD_FUZZ_STOP:
        fuzz_engine_stop();
        break;
    case CMD_TRIGGER_SET:
        trigger_set_reference(p, len);
        break;
    case CMD_TRIGGER_ENABLE:
        g_trigger_armed = 1;
        break;
    case CMD_TRIGGER_DISABLE:
        g_trigger_armed = 0;
        break;
    case CMD_AUTH_CHALLENGE: {
        uint8_t resp[32];
        if (onewire_challenge(p, resp) == 0)
            ble_send_cmd(CMD_AUTH_CHALLENGE, resp, 32);
        else
            ble_send_cmd(CMD_ERROR, 0, 0);
        break;
    }
    default:
        logf("unknown cmd 0x%02x\r\n", op);
        ble_send_cmd(CMD_ERROR, 0, 0);
    }
}

static uint16_t strnlen_(const char *s, size_t n) {
    uint16_t i = 0; while (i < n && s[i]) i++; return i;
}

/* ---- BLE / USB poll: decode frame, dispatch ---- */
static void poll_links(void) {
    static uint8_t buf[64];
    uint8_t op; uint8_t payload[64]; uint16_t plen;
    if (ble_poll_cmd(&op, payload, &plen)) {
        handle_cmd(op, payload, plen);
    }
    /* USB CDC polling is handled in usb_cdc.c event callbacks; here we
     * also poll in case a laptop is attached. */
    usb_cdc_poll();
    (void)buf;
}

/* ---- Capacitive touch handling ---- */
static void touch_handle(void) {
    uint8_t t = board_touch_read();
    static uint8_t last;
    if (t == last) return;
    uint8_t edge = t & ~last;
    last = t;
    if (edge & 0x01) {          /* zone 0: arm/disarm */
        g_state.armed = !g_state.armed;
        led_status();
        board_motor_pulse(30);
        logf("touch0 arm=%u\r\n", g_state.armed);
    }
    if (edge & 0x02) {          /* zone 1: snapshot */
        capture_one_frame(g_cap_slot_idx);
        g_cap_slot_idx = (g_cap_slot_idx + 1) % CAP_SLOTS;
        board_motor_pulse(15);
    }
    if (edge & 0x04) {          /* zone 2: cycle mode */
        g_state.mode = (op_mode_t)((g_state.mode + 1) % 5);
        fpga_set_mode(g_state.mode);
        led_status();
        board_motor_pulse(20);
    }
}

/* ---- Sensor power monitor (INA219 via I²C2) ---- */
static void check_sensor_power(void) {
    static uint32_t next_check;
    if (millis() < next_check) return;
    next_check = millis() + 500;
    /* INA219 at 0x40 on side-B; read shunt voltage register 0x01 */
    uint16_t v;
    int r = cci_read(CCI_SIDE_B, 0x40, 0x01, &v);
    int ok = (r == 0 && v > 0);
    if (ok != g_state.sensor_power_ok) {
        g_state.sensor_power_ok = ok;
        logf("sensor power %s\r\n", ok ? "ok" : "LOST");
        if (!ok && g_state.mode == MODE_INJECT) {
            /* host pulled sensor power; if we were injecting, we can't
             * keep the link alive without sensor clk — bail to passthrough. */
            fpga_set_mode(MODE_PASSTHROUGH);
            g_state.mode = MODE_PASSTHROUGH;
            led_status();
            ble_notify_log("sensor power lost; passthrough");
        }
    }
}

/* ===================================================================== *
 * main                                                                  *
 * ===================================================================== */
int main(void) {
    /* 1. Bring up clocks, GPIO, peripherals */
    board_init();

    /* 2. SysTick 1 ms */
    SYST_RVR = (HCLK_HZ / 1000u) - 1u;
    SYST_CVR = 0;
    SYST_CSR = 0x7; /* enable, interrupt, clk/1 */

    /* 3. Init drivers */
    uart_init(115200);
    uart_puts("\r\nAperturePhantom boot\r\n");
    uart_puts("author: jayis1\r\n");

    ble_init();
    usb_cdc_init();

    sdram_init();
    for (int i = 0; i < CAP_SLOTS; i++) {
        g_cap_slot_off[i] = i * (1920u * 1080u);
        g_cap_slot_len[i] = 0;
    }
    g_cap_slot_idx = 0;

    if (sdcard_init() == 0 && sdcard_mount() == 0) {
        g_state.sd_free_kb = 0; /* TODO read from FATFS */
        logf("sd mounted\r\n");
    } else {
        logf("sd absent or mount fail\r\n");
    }

    fpga_init();
    fpga_reset();
    /* default to passthrough; the FPGA gateware boots in passthrough so the
     * host sees the sensor normally. */
    fpga_set_mode(MODE_PASSTHROUGH);
    g_state.mode = MODE_PASSTHROUGH;

    cci_init(CCI_SIDE_A);
    cci_init(CCI_SIDE_B);
    sensor_autoconfig();

    onewire_init();

    g_state.armed = 0;
    g_state.frame_count = 0;
    g_state.crc_errs = 0;
    led_status();

    logf("aperture-phantom ready; sensor=%s\r\n", g_state.sensor_name);
    ble_notify_log("aperture-phantom ready");

    /* 4. Main loop */
    uint32_t led_toggle = 0;
    while (1) {
        poll_links();
        touch_handle();
        check_sensor_power();
        if (g_script_running) script_step();
        fuzz_engine_tick();

        /* heartbeat: blink the status LED slowly when idle */
        if (millis() - led_toggle > 1000) {
            led_toggle = millis();
            /* gentle blink only when not armed */
            if (!g_state.armed) {
                /* toggle green */
                static uint8_t tog;
                if (tog) board_led_set(0, 1, 0); else board_led_set(0, 0, 0);
                tog ^= 1;
            }
        }
        __asm__("wfi"); /* wait for interrupt (SysTick or UART) */
    }
}

/* ---- Hard fault handler: blink red, attempt to log ---- */
void HardFault_Handler(void) {
    board_led_set(1, 0, 0);
    while (1) { __asm__("bkpt 0"); }
}

/* ---- Vectors stub (the startup file provides the real table) ---- */
void Reset_Handler(void) { main(); }

/* end of file */