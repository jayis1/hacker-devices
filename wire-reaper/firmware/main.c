/*
 * main.c — WireReaper main firmware entry point and scheduler
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * This is the bare-metal main loop for the WireReaper multi-bus
 * embedded peripheral infiltrator. It initializes all hardware,
 * runs a cooperative scheduler, and dispatches commands from
 * USB CDC or BLE UART.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* ---- Driver prototypes ---- */
extern void wr_clock_init(void);
extern void wr_gpio_init(void);
extern void wr_sdram_init(void);
extern void wr_fpga_init(void);
extern void wr_ble_init(void);
extern void wr_usb_init(void);
extern void wr_sdcard_init(void);
extern void wr_oled_init(void);
extern void wr_i2c_init(int ch);
extern void wr_spi_init(int ch);
extern void wr_uart_init(int ch, uint32_t baud);
extern void wr_onewire_init(void);

extern int  wr_i2c_scan(int ch, uint8_t *found, int max);
extern int  wr_i2c_read(int ch, uint8_t addr, uint8_t reg,
                         uint8_t *buf, int len);
extern int  wr_i2c_write(int ch, uint8_t addr, uint8_t reg,
                          const uint8_t *buf, int len);
extern int  wr_i2c_emulate(int ch, uint8_t addr);

extern int  wr_spi_read(int ch, const uint8_t *cmd, int cmdlen,
                         uint8_t *buf, int len);
extern int  wr_spi_write(int ch, const uint8_t *cmd, int cmdlen,
                          const uint8_t *data, int datalen);
extern int  wr_spi_emulate(int ch, const uint8_t *resp, int resplen);

extern int  wr_uart_sniff(int ch, uint8_t *buf, int maxlen);
extern int  wr_uart_inject(int ch, const uint8_t *buf, int len);
extern void wr_uart_set_baud(int ch, uint32_t baud);

extern int  wr_onewire_scan(uint8_t *rom_ids, int max);
extern int  wr_onewire_read(int idx, uint8_t *buf, int offset, int len);

extern void wr_fpga_load_bitstream(const uint8_t *bits, int len);
extern void wr_fpga_set_trigger(int ch, uint32_t mask, uint32_t value);

extern void wr_capture_start(uint32_t chan_mask);
extern void wr_capture_stop(void);
extern int  wr_capture_read(uint8_t *buf, int maxlen);

extern void wr_ble_send(const uint8_t *data, int len);
extern int  wr_ble_recv(uint8_t *buf, int maxlen);
extern void wr_usb_send(const uint8_t *data, int len);
extern int  wr_usb_recv(uint8_t *buf, int maxlen);
extern int  wr_usb_configured(void);

extern void wr_sdcard_write(const char *fn, const uint8_t *data, int len);

extern void wr_oled_show(int line, const char *text);
extern void wr_oled_clear(void);

/* ---- Command opcodes (must match protocol.js) ---- */
#define CMD_CAPTURE_START   0x01
#define CMD_CAPTURE_STOP    0x02
#define CMD_CAPTURE_STREAM  0x03
#define CMD_I2C_SCAN        0x10
#define CMD_I2C_READ        0x11
#define CMD_I2C_WRITE       0x12
#define CMD_I2C_EMULATE     0x13
#define CMD_SPI_READ        0x20
#define CMD_SPI_WRITE       0x21
#define CMD_SPI_EMULATE     0x22
#define CMD_UART_SNIFF      0x30
#define CMD_UART_INJECT     0x31
#define CMD_UART_SET_BAUD   0x32
#define CMD_ONEWIRE_SCAN    0x40
#define CMD_ONEWIRE_READ    0x41
#define CMD_REPLAY_LOAD     0x50
#define CMD_REPLAY_RUN      0x51
#define CMD_FUZZ_START      0x60
#define CMD_FUZZ_STOP       0x61
#define CMD_STATUS          0x70
#define CMD_VERSION         0x71
#define CMD_RESET           0xFF

/* ---- Capture record format ---- */
#define REC_BUS_I2C     0
#define REC_BUS_SPI     1
#define REC_BUS_UART    2
#define REC_BUS_1WIRE   3

typedef struct __attribute__((packed)) {
    uint8_t  bus;       /* REC_BUS_* */
    uint8_t  channel;   /* channel index */
    uint8_t  rw;        /* 0=read, 1=write */
    uint8_t  flags;     /* ACK/NACK/error bits */
    uint16_t addr;      /* I2C addr / SPI CS / UART line */
    uint16_t reg;       /* register / command byte */
    uint32_t timestamp; /* microsecond tick */
    uint16_t datalen;   /* payload length */
    uint8_t  data[6];   /* inline data (up to 6 bytes inline) */
} capture_record_t;

/* ---- Global state ---- */
static struct {
    uint32_t capture_chan_mask;
    int      capturing;
    int      streaming;
    uint32_t capture_count;
    uint32_t buffer_write_idx;
    uint32_t buffer_read_idx;
    int      usb_connected;
    int      ble_connected;
    uint8_t  fuzz_active;
    uint8_t  fuzz_channel;
    uint8_t  fuzz_bus;
    uint32_t fuzz_iter;
    uint32_t uptime_ms;
} g_state;

/* ---- Capture ring buffer in SDRAM ---- */
static volatile capture_record_t *g_capture_buf =
    (volatile capture_record_t *)SDRAM_BANK_ADDR;

/* ---- SysTick counter (1 ms) ---- */
static volatile uint32_t g_ticks_ms = 0;

IRQ_HANDLER(SysTick_Handler) {
    g_ticks_ms++;
    g_state.uptime_ms = g_ticks_ms;
}

static uint32_t ticks_us(void) {
    uint32_t ms1 = g_ticks_ms;
    uint32_t ms2;
    uint32_t val;
    do {
        ms2 = ms1;
        val = SYST_CVR;
        ms1 = g_ticks_ms;
    } while (ms1 != ms2);
    /* SYST_RVR is reload value; counter is down-counting */
    uint32_t reload = SYST_RVR;
    uint32_t frac = (reload - val) / (reload / 1000);
    return ms1 * 1000 + frac;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = g_ticks_ms;
    while ((g_ticks_ms - start) < ms)
        ;
}

/* ---- SysTick init: 1 ms tick from 240 MHz AHB ---- */
static void systick_init(void) {
    SYST_RVR = (HCLK_HZ / 1000) - 1;
    SYST_CVR = 0;
    SYST_CSR = SYSTICK_CLKSRC | SYSTICK_TICKINT | SYSTICK_ENABLE;
}

/* ---- SDRAM capture buffer management ---- */
static void capture_push(const capture_record_t *rec) {
    if (g_state.capture_count >= MAX_RECORDS) {
        /* Overwrite oldest (ring buffer) */
        g_state.buffer_write_idx =
            (g_state.buffer_write_idx + 1) % MAX_RECORDS;
        if (g_state.buffer_read_idx == g_state.buffer_write_idx)
            g_state.buffer_read_idx =
                (g_state.buffer_read_idx + 1) % MAX_RECORDS;
    } else {
        g_state.capture_count++;
    }

    memcpy((void *)&g_capture_buf[g_state.buffer_write_idx],
           rec, sizeof(capture_record_t));
    g_state.buffer_write_idx =
        (g_state.buffer_write_idx + 1) % MAX_RECORDS;
}

static int capture_pop(capture_record_t *rec) {
    if (g_state.capture_count == 0)
        return 0;
    memcpy(rec, (void *)&g_capture_buf[g_state.buffer_read_idx],
           sizeof(capture_record_t));
    g_state.buffer_read_idx =
        (g_state.buffer_read_idx + 1) % MAX_RECORDS;
    g_state.capture_count--;
    return 1;
}

void wr_capture_start(uint32_t chan_mask) {
    g_state.capture_chan_mask = chan_mask;
    g_state.capturing = 1;
    g_state.capture_count = 0;
    g_state.buffer_write_idx = 0;
    g_state.buffer_read_idx = 0;
    wr_oled_show(3, "CAP: ACTIVE");
}

void wr_capture_stop(void) {
    g_state.capturing = 0;
    wr_oled_show(3, "CAP: STOPPED");
}

int wr_capture_read(uint8_t *buf, int maxlen) {
    int total = 0;
    capture_record_t rec;
    while (total + (int)sizeof(capture_record_t) <= maxlen) {
        if (!capture_pop(&rec))
            break;
        memcpy(buf + total, &rec, sizeof(capture_record_t));
        total += sizeof(capture_record_t);
    }
    return total;
}

/* ---- I2C scan result callback into capture ---- */
static void i2c_scan_callback(int ch, uint8_t addr, int found) {
    capture_record_t rec = {0};
    rec.bus = REC_BUS_I2C;
    rec.channel = ch;
    rec.rw = 0;
    rec.flags = found ? 0x01 : 0x00;  /* 0x01 = ACK (present) */
    rec.addr = addr;
    rec.timestamp = ticks_us();
    if (g_state.capturing)
        capture_push(&rec);
}

/* ---- Fuzzing engine ---- */
static uint32_t rng_state = 0xDEADBEEF;

static uint32_t xorshift32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static void fuzz_i2c(int ch) {
    uint8_t addr = (xorshift32() & 0x7F);
    uint8_t reg = (xorshift32() & 0xFF);
    uint8_t val = (xorshift32() & 0xFF);
    uint8_t buf[4];
    int len = 1 + (xorshift32() & 3);
    for (int i = 0; i < len; i++)
        buf[i] = xorshift32() & 0xFF;

    int ret = wr_i2c_write(ch, addr, reg, buf, len);

    capture_record_t rec = {0};
    rec.bus = REC_BUS_I2C;
    rec.channel = ch;
    rec.rw = 1;
    rec.flags = (ret == WR_OK) ? 0x01 : 0x02;  /* ACK or NACK */
    rec.addr = addr;
    rec.reg = reg;
    rec.timestamp = ticks_us();
    rec.datalen = len > 6 ? 6 : len;
    memcpy(rec.data, buf, rec.datalen);
    if (g_state.capturing)
        capture_push(&rec);

    g_state.fuzz_iter++;
}

static void fuzz_spi(int ch) {
    uint8_t cmd[4];
    int cmdlen = 1 + (xorshift32() & 3);
    for (int i = 0; i < cmdlen; i++)
        cmd[i] = xorshift32() & 0xFF;

    uint8_t resp[8];
    wr_spi_read(ch, cmd, cmdlen, resp, 8);

    capture_record_t rec = {0};
    rec.bus = REC_BUS_SPI;
    rec.channel = ch;
    rec.rw = 0;
    rec.reg = cmd[0];
    rec.timestamp = ticks_us();
    rec.datalen = cmdlen > 6 ? 6 : cmdlen;
    memcpy(rec.data, cmd, rec.datalen);
    if (g_state.capturing)
        capture_push(&rec);

    g_state.fuzz_iter++;
}

static void fuzz_uart(int ch) {
    uint8_t buf[16];
    int len = 1 + (xorshift32() & 15);
    for (int i = 0; i < len; i++)
        buf[i] = xorshift32() & 0xFF;

    wr_uart_inject(ch, buf, len);

    capture_record_t rec = {0};
    rec.bus = REC_BUS_UART;
    rec.channel = ch;
    rec.rw = 1;
    rec.timestamp = ticks_us();
    rec.datalen = len > 6 ? 6 : len;
    memcpy(rec.data, buf, rec.datalen);
    if (g_state.capturing)
        capture_push(&rec);

    g_state.fuzz_iter++;
}

static void fuzz_step(void) {
    if (!g_state.fuzz_active)
        return;
    switch (g_state.fuzz_bus) {
    case CH_TYPE_I2C:
        fuzz_i2c(g_state.fuzz_channel);
        break;
    case CH_TYPE_SPI:
        fuzz_spi(g_state.fuzz_channel);
        break;
    case CH_TYPE_UART:
        fuzz_uart(g_state.fuzz_channel);
        break;
    default:
        break;
    }
    /* Inter-fuzz delay: 2-10 ms to avoid locking the bus */
    delay_ms(2 + (xorshift32() & 7));
}

/* ---- Command processing ---- */
static void send_response(const uint8_t *data, int len) {
    if (g_state.usb_connected && wr_usb_configured())
        wr_usb_send(data, len);
    else if (g_state.ble_connected)
        wr_ble_send(data, len);
}

static void send_status(void) {
    uint8_t resp[20];
    resp[0] = CMD_STATUS;
    resp[1] = 0; /* status OK */
    resp[2] = g_state.capturing;
    resp[3] = g_state.streaming;
    resp[4] = (g_state.capture_count >> 0)  & 0xFF;
    resp[5] = (g_state.capture_count >> 8)  & 0xFF;
    resp[6] = (g_state.capture_count >> 16) & 0xFF;
    resp[7] = (g_state.capture_count >> 24) & 0xFF;
    resp[8] = g_state.fuzz_active;
    resp[9] = g_state.fuzz_iter & 0xFF;
    resp[10] = (g_state.fuzz_iter >> 8) & 0xFF;
    resp[11] = (g_state.fuzz_iter >> 16) & 0xFF;
    resp[12] = (g_state.fuzz_iter >> 24) & 0xFF;
    resp[13] = (g_state.uptime_ms >> 0) & 0xFF;
    resp[14] = (g_state.uptime_ms >> 8) & 0xFF;
    resp[15] = (g_state.uptime_ms >> 16) & 0xFF;
    resp[16] = (g_state.uptime_ms >> 24) & 0xFF;
    resp[17] = 100; /* battery % placeholder */
    resp[18] = NUM_TOTAL_CHANNELS;
    resp[19] = 0;
    send_response(resp, 20);
}

static void send_version(void) {
    const char *ver = "WireReaper v" FW_VERSION_STRING " by " WR_AUTHOR;
    uint8_t resp[64];
    int len = 0;
    resp[len++] = CMD_VERSION;
    resp[len++] = WR_VERSION_MAJOR;
    resp[len++] = WR_VERSION_MINOR;
    resp[len++] = WR_VERSION_PATCH;
    /* capabilities bitmask */
    resp[len++] = (NUM_I2C_CHANNELS) | (NUM_SPI_CHANNELS << 4);
    resp[len++] = (NUM_UART_CHANNELS) | (NUM_ONEWIRE_CHANS << 4);
    resp[len++] = 0; /* reserved */
    resp[len++] = 0; /* reserved */
    for (int i = 0; ver[i] && len < 63; i++)
        resp[len++] = ver[i];
    send_response(resp, len);
}

static void process_command(const uint8_t *cmd, int len) {
    if (len < 1)
        return;

    uint8_t opcode = cmd[0];
    uint8_t resp[256];
    int rlen = 0;

    switch (opcode) {
    case CMD_VERSION:
        send_version();
        break;

    case CMD_STATUS:
        send_status();
        break;

    case CMD_CAPTURE_START:
        if (len >= 5) {
            uint32_t mask = cmd[1] | (cmd[2] << 8) |
                            (cmd[3] << 16) | (cmd[4] << 24);
            wr_capture_start(mask);
            resp[0] = CMD_CAPTURE_START;
            resp[1] = WR_OK;
            send_response(resp, 2);
        }
        break;

    case CMD_CAPTURE_STOP:
        wr_capture_stop();
        resp[0] = CMD_CAPTURE_STOP;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;

    case CMD_CAPTURE_STREAM: {
        g_state.streaming = 1;
        uint8_t sbuf[STREAM_CHUNK_SIZE * 2];
        int n = wr_capture_read(sbuf, sizeof(sbuf));
        if (n > 0) {
            sbuf[n] = 0; /* not needed, but bounds-safe */
            send_response(sbuf, n);
        }
        resp[0] = CMD_CAPTURE_STREAM;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;
    }

    case CMD_I2C_SCAN: {
        if (len < 2) break;
        int ch = cmd[1];
        uint8_t found[128];
        memset(found, 0, sizeof(found));
        int n = wr_i2c_scan(ch, found, 128);
        resp[0] = CMD_I2C_SCAN;
        resp[1] = WR_OK;
        resp[2] = ch;
        resp[3] = n & 0xFF;
        for (int i = 0; i < n && rlen < 250; i++)
            resp[4 + i] = found[i];
        send_response(resp, 4 + n);
        break;
    }

    case CMD_I2C_READ: {
        if (len < 5) break;
        int ch = cmd[1];
        uint8_t addr = cmd[2];
        uint8_t reg = cmd[3];
        int rlen2 = cmd[4];
        if (rlen2 > 250) rlen2 = 250;
        uint8_t rbuf[250];
        int ret = wr_i2c_read(ch, addr, reg, rbuf, rlen2);
        resp[0] = CMD_I2C_READ;
        resp[1] = (ret == WR_OK) ? 0 : 1;
        resp[2] = ret & 0xFF;
        if (ret == WR_OK)
            memcpy(resp + 3, rbuf, rlen2);
        send_response(resp, 3 + (ret == WR_OK ? rlen2 : 0));
        break;
    }

    case CMD_I2C_WRITE: {
        if (len < 5) break;
        int ch = cmd[1];
        uint8_t addr = cmd[2];
        uint8_t reg = cmd[3];
        int wlen = len - 4;
        if (wlen > 250) wlen = 250;
        int ret = wr_i2c_write(ch, addr, reg, cmd + 4, wlen);
        resp[0] = CMD_I2C_WRITE;
        resp[1] = (ret == WR_OK) ? 0 : 1;
        send_response(resp, 2);
        break;
    }

    case CMD_I2C_EMULATE: {
        if (len < 3) break;
        int ch = cmd[1];
        uint8_t addr = cmd[2];
        wr_i2c_emulate(ch, addr);
        resp[0] = CMD_I2C_EMULATE;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;
    }

    case CMD_SPI_READ: {
        if (len < 4) break;
        int ch = cmd[1];
        int cmdlen = cmd[2];
        int rlen2 = cmd[3];
        if (cmdlen > 16) cmdlen = 16;
        if (rlen2 > 240) rlen2 = 240;
        uint8_t rbuf[240];
        int ret = wr_spi_read(ch, cmd + 4, cmdlen, rbuf, rlen2);
        resp[0] = CMD_SPI_READ;
        resp[1] = (ret == WR_OK) ? 0 : 1;
        if (ret == WR_OK)
            memcpy(resp + 2, rbuf, rlen2);
        send_response(resp, 2 + (ret == WR_OK ? rlen2 : 0));
        break;
    }

    case CMD_SPI_WRITE: {
        if (len < 4) break;
        int ch = cmd[1];
        int cmdlen = cmd[2];
        int dlen = len - 4 - cmdlen;
        if (dlen < 0) dlen = 0;
        wr_spi_write(ch, cmd + 4, cmdlen, cmd + 4 + cmdlen, dlen);
        resp[0] = CMD_SPI_WRITE;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;
    }

    case CMD_SPI_EMULATE: {
        if (len < 3) break;
        int ch = cmd[1];
        int rlen2 = len - 2;
        wr_spi_emulate(ch, cmd + 2, rlen2);
        resp[0] = CMD_SPI_EMULATE;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;
    }

    case CMD_UART_SNIFF: {
        if (len < 2) break;
        int ch = cmd[1];
        uint8_t sbuf[128];
        int n = wr_uart_sniff(ch, sbuf, 128);
        resp[0] = CMD_UART_SNIFF;
        resp[1] = ch;
        resp[2] = n & 0xFF;
        if (n > 0)
            memcpy(resp + 3, sbuf, n);
        send_response(resp, 3 + n);
        break;
    }

    case CMD_UART_INJECT: {
        if (len < 2) break;
        int ch = cmd[1];
        int dlen = len - 2;
        wr_uart_inject(ch, cmd + 2, dlen);
        resp[0] = CMD_UART_INJECT;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;
    }

    case CMD_UART_SET_BAUD: {
        if (len < 6) break;
        int ch = cmd[1];
        uint32_t baud = cmd[2] | (cmd[3] << 8) |
                        (cmd[4] << 16) | (cmd[5] << 24);
        wr_uart_set_baud(ch, baud);
        resp[0] = CMD_UART_SET_BAUD;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;
    }

    case CMD_ONEWIRE_SCAN: {
        uint8_t roms[8 * 16];
        int n = wr_onewire_scan(roms, 16);
        resp[0] = CMD_ONEWIRE_SCAN;
        resp[1] = n & 0xFF;
        for (int i = 0; i < n * 8 && rlen < 250; i++)
            resp[2 + i] = roms[i];
        send_response(resp, 2 + n * 8);
        break;
    }

    case CMD_ONEWIRE_READ: {
        if (len < 4) break;
        int idx = cmd[1];
        int offset = cmd[2];
        int rlen2 = cmd[3];
        if (rlen2 > 250) rlen2 = 250;
        uint8_t rbuf[250];
        int ret = wr_onewire_read(idx, rbuf, offset, rlen2);
        resp[0] = CMD_ONEWIRE_READ;
        resp[1] = (ret == WR_OK) ? 0 : 1;
        if (ret == WR_OK)
            memcpy(resp + 2, rbuf, rlen2);
        send_response(resp, 2 + (ret == WR_OK ? rlen2 : 0));
        break;
    }

    case CMD_FUZZ_START: {
        if (len < 4) break;
        g_state.fuzz_bus = cmd[1];
        g_state.fuzz_channel = cmd[2];
        g_state.fuzz_active = 1;
        g_state.fuzz_iter = 0;
        wr_oled_show(3, "FUZZ: ACTIVE");
        resp[0] = CMD_FUZZ_START;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;
    }

    case CMD_FUZZ_STOP:
        g_state.fuzz_active = 0;
        wr_oled_show(3, "FUZZ: STOPPED");
        resp[0] = CMD_FUZZ_STOP;
        resp[1] = WR_OK;
        send_response(resp, 2);
        break;

    case CMD_REPLAY_RUN: {
        /* Replay would load from SD card; stub to stop capture & signal */
        resp[0] = CMD_REPLAY_RUN;
        resp[1] = WR_ERR_NOT_CONNECTED; /* no replay buffer loaded */
        send_response(resp, 2);
        break;
    }

    case CMD_RESET:
        resp[0] = CMD_RESET;
        resp[1] = WR_OK;
        send_response(resp, 2);
        delay_ms(50);
        /* Soft reset via NVIC system reset */
        REG32(0xE000ED0C) = 0x5FA0004;
        break;

    default:
        resp[0] = opcode;
        resp[1] = 0xFF; /* unknown command */
        send_response(resp, 2);
        break;
    }
}

/* ---- Command buffer and parsing ---- */
static uint8_t  cmd_buf[CMD_BUF_SIZE];
static int      cmd_buf_len = 0;
static uint8_t  cmd_len_expected = 0;

static void poll_commands(void) {
    uint8_t tmp[CMD_BUF_SIZE];
    int n;

    /* Try USB first, then BLE */
    if (g_state.usb_connected && wr_usb_configured()) {
        n = wr_usb_recv(tmp, sizeof(tmp));
    } else {
        n = wr_ble_recv(tmp, sizeof(tmp));
    }

    if (n <= 0)
        return;

    for (int i = 0; i < n; i++) {
        if (cmd_buf_len == 0) {
            /* First byte = opcode, second byte = payload length */
            cmd_buf[cmd_buf_len++] = tmp[i];
        } else if (cmd_buf_len == 1) {
            cmd_len_expected = tmp[i];
            cmd_buf[cmd_buf_len++] = tmp[i];
        } else {
            cmd_buf[cmd_buf_len++] = tmp[i];
            if (cmd_buf_len >= cmd_len_expected + 2) {
                process_command(cmd_buf, cmd_buf_len);
                cmd_buf_len = 0;
                cmd_len_expected = 0;
            }
            if (cmd_buf_len >= CMD_BUF_SIZE) {
                /* Overflow: reset */
                cmd_buf_len = 0;
            }
        }
    }
}

/* ---- Streaming captured data to host ---- */
static void poll_stream(void) {
    if (!g_state.streaming || !g_state.capturing)
        return;
    if (g_state.capture_count == 0)
        return;

    uint8_t sbuf[STREAM_CHUNK_SIZE];
    int n = wr_capture_read(sbuf, sizeof(sbuf));
    if (n > 0)
        send_response(sbuf, n);
}

/* ---- OLED status display ---- */
static void update_display(void) {
    static uint32_t last_update = 0;
    if (g_ticks_ms - last_update < 200)
        return;
    last_update = g_ticks_ms;

    char line[22];
    wr_oled_show(0, "WireReaper jayis1");
    snprintf(line, sizeof(line), "BUF: %lu/%lu",
             (unsigned long)g_state.capture_count,
             (unsigned long)MAX_RECORDS);
    wr_oled_show(1, line);

    const char *mode = "IDLE";
    if (g_state.fuzz_active)       mode = "FUZZ";
    else if (g_state.capturing)    mode = "CAPTURE";
    else if (g_state.streaming)    mode = "STREAM";
    snprintf(line, sizeof(line), "MODE: %s", mode);
    wr_oled_show(2, line);

    snprintf(line, sizeof(line), "UP: %lus BAT:--",
             (unsigned long)(g_state.uptime_ms / 1000));
    wr_oled_show(5, line);
}

/* ---- Button handling (simplified) ---- */
static void poll_buttons(void) {
    static uint32_t last_btn_time = 0;
    if (g_ticks_ms - last_btn_time < 50)
        return;
    last_btn_time = g_ticks_ms;

    gpio_t *e = GPIO(BTN_PORT);
    uint32_t idr = e->IDR;

    /* Trigger button toggles capture */
    if (!(idr & (1U << BTN_TRIGGER_PIN))) {
        if (g_state.capturing)
            wr_capture_stop();
        else
            wr_capture_start(0xFFFFFFFF);
        delay_ms(300); /* debounce */
    }
}

/* ---- Main ---- */
int main(void) {
    /* 1. Initialize hardware */
    wr_clock_init();
    systick_init();
    wr_gpio_init();
    wr_sdram_init();
    wr_fpga_init();
    wr_ble_init();
    wr_usb_init();
    wr_sdcard_init();
    wr_oled_init();

    /* 2. Initialize bus channels */
    for (int i = 0; i < NUM_I2C_CHANNELS; i++)
        wr_i2c_init(i);
    for (int i = 0; i < NUM_SPI_CHANNELS; i++)
        wr_spi_init(i);
    for (int i = 0; i < NUM_UART_CHANNELS; i++)
        wr_uart_init(i, 115200);
    wr_onewire_init();

    /* 3. Show boot screen */
    wr_oled_clear();
    wr_oled_show(0, "WireReaper");
    wr_oled_show(1, "by jayis1");
    wr_oled_show(2, "v" FW_VERSION_STRING);
    wr_oled_show(3, "Initializing...");
    delay_ms(500);
    wr_oled_clear();

    /* 4. Main cooperative scheduler loop */
    uint32_t last_fuzz = 0;
    while (1) {
        /* High-priority: command processing */
        poll_commands();

        /* Medium-priority: streaming captured data */
        poll_stream();

        /* Medium-priority: fuzzing step */
        if (g_state.fuzz_active) {
            fuzz_step();
        }

        /* Low-priority: UI updates */
        poll_buttons();
        update_display();

        /* Yield: brief delay to reduce power */
        if (!g_state.fuzz_active)
            delay_ms(1);
    }

    return 0; /* never reached */
}