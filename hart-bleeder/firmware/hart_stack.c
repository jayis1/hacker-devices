/*
 * hart-bleeder — hart_stack.c
 * Implementation of the HART protocol stack: framing, checksums,
 * transaction management, polling, and bus enumeration for the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "hart_stack.h"
#include "modem_drv.h"
#include <string.h>

hart_stack_t g_hart;

/* ── Internal helpers ─────────────────────────────────────────── */

static uint32_t millis(void) { return system_millis(); }

static void delay_ms(uint32_t ms) { system_delay_ms(ms); }

/* ── Checksum ─────────────────────────────────────────────────── */
uint8_t hart_checksum(const uint8_t *buf, uint16_t len)
{
    uint8_t c = 0;
    for (uint16_t i = 0; i < len; i++) c ^= buf[i];
    return c;
}

/* ── Frame builder ────────────────────────────────────────────── */
int hart_build_frame(uint8_t *buf, uint16_t *len,
                     uint8_t addr, uint8_t cmd,
                     const uint8_t *data, uint16_t data_len,
                     int long_frame, int is_response)
{
    uint16_t i = 0;

    /* Preamble: minimum 5 bytes of 0xFF; we send 5 for short, 8 for long */
    uint8_t pre = long_frame ? 8 : 5;
    for (uint8_t p = 0; p < pre; p++) buf[i++] = 0xFF;

    /* Start byte */
    buf[i++] = long_frame ? HART_LONG_START : HART_SHORT_START;

    /* Address byte — short frame: bit7 = primary master, bits0-4 = poll addr.
     * Long frame: 38-bit unique ID packed, bit7 of first byte = primary master.
     */
    if (long_frame) {
        buf[i++] = (addr & 0x3F) | (g_hart.master_role ? 0 : 0x80);
    } else {
        buf[i++] = (addr & 0x3F) | (g_hart.master_role ? 0 : 0x80);
    }

    /* Command byte — responses set bit7 */
    buf[i++] = is_response ? (cmd | HART_RSP_BITS) : cmd;

    /* Byte count */
    if (long_frame) {
        /* Long frame: bytecnt = 2 (status) + data_len + ... */
        buf[i++] = (uint8_t)(2 + data_len + 2);  /* status(2) + data + ... */
        /* Expansion status bytes (0 = no error on tx) */
        buf[i++] = 0x00;
        buf[i++] = 0x00;
    } else {
        buf[i++] = (uint8_t)(data_len);
    }

    /* Data */
    if (data && data_len) {
        memcpy(&buf[i], data, data_len);
        i += data_len;
    }

    /* Checksum covers from start byte onward */
    uint8_t chk = hart_checksum(&buf[pre], i - pre);
    buf[i++] = chk;

    *len = i;
    return 0;
}

/* ── Frame parser ────────────────────────────────────────────── */
int hart_parse_frame(const uint8_t *buf, uint16_t len, hart_frame_t *f)
{
    if (len < 6) return -1;
    memset(f, 0, sizeof(*f));

    /* Skip preambles */
    uint16_t i = 0;
    while (i < len && buf[i] == 0xFF) i++;
    if (i >= len) return -2;          /* all preambles, no data */

    uint16_t pre_cnt = i;
    f->start = buf[i++];

    if (f->start != HART_SHORT_START && f->start != HART_LONG_START)
        return -3;

    f->is_long = (f->start == HART_LONG_START);

    if (i >= len) return -4;
    f->addr = buf[i++];

    if (i >= len) return -5;
    f->cmd = buf[i++];
    f->is_response = (f->cmd & HART_RSP_BITS) ? 1 : 0;
    f->cmd &= ~HART_RSP_BITS;

    if (i >= len) return -6;
    f->bytecnt = buf[i++];

    if (f->is_long) {
        /* 2 expansion status bytes come first in the body */
        if (i + 2 > len) return -7;
        f->status[0] = buf[i++];
        f->status[1] = buf[i++];
        uint16_t dl = (f->bytecnt > 2) ? (f->bytecnt - 2) : 0;
        if (dl > HART_MAX_FRAME) return -8;
        if (i + dl > len - 1) return -9;     /* -1 for trailing chk */
        memcpy(f->data, &buf[i], dl);
        f->data_len = dl;
        i += dl;
    } else {
        uint16_t dl = f->bytecnt;
        if (dl > HART_MAX_FRAME) return -10;
        if (i + dl > len - 1) return -11;
        memcpy(f->data, &buf[i], dl);
        f->data_len = dl;
        i += dl;
    }

    if (i >= len) return -12;
    f->chk = buf[i];
    /* Verify checksum over start..last data byte */
    f->chk_calc = hart_checksum(&buf[pre_cnt], i - pre_cnt);
    if (f->chk != f->chk_calc) {
        g_hart.stats_crc_err++;
        return -13;
    }
    return 0;
}

/* ── Low-level send / recv via modem driver ──────────────────── */
int hart_send_raw(const uint8_t *buf, uint16_t len)
{
    modem_tx_enable(1);
    int rc = modem_write(buf, len);
    modem_tx_enable(0);
    g_hart.stats_tx++;
    return rc;
}

int hart_recv_frame(hart_frame_t *f, uint32_t timeout_ms)
{
    uint8_t rbuf[HART_MAX_FRAME + 32];
    uint16_t n = 0;
    uint32_t t0 = millis();
    int saw_preamble = 0;

    while (millis() - t0 < timeout_ms) {
        int b = modem_read_timeout(timeout_ms);
        if (b < 0) break;
        if (n >= sizeof(rbuf)) { n = 0; saw_preamble = 0; }
        rbuf[n++] = (uint8_t)b;
        if (b == 0xFF) saw_preamble = 1;
        /* On start byte after preamble, we know structure; read bytecnt. */
        if (n > 1 && saw_preamble &&
            (rbuf[n-1] == HART_SHORT_START || rbuf[n-1] == HART_LONG_START)) {
            /* Read addr + cmd + bytecnt */
            while (n < (uint16_t)(n /* placeholder */) && modem_avail() == 0) {
                int x = modem_read_timeout(20);
                if (x < 0) goto parse;
                rbuf[n++] = (uint8_t)x;
                if (n >= 5) break;
            }
            /* now compute expected length */
            uint8_t bc = rbuf[4];
            uint16_t need = (rbuf[3] == HART_LONG_START) ? (5 + 2 + bc + 1)
                                                         : (5 + bc + 1);
            while (n < need) {
                int x = modem_read_timeout(50);
                if (x < 0) break;
                rbuf[n++] = (uint8_t)x;
            }
            break;
        }
    }
parse:
    if (n < 6) { g_hart.stats_timeout++; return -1; }
    int rc = hart_parse_frame(rbuf, n, f);
    if (rc == 0) g_hart.stats_rx++;
    return rc;
}

/* ── Send command and wait for response ──────────────────────── */
int hart_send_cmd(uint8_t addr, uint8_t cmd,
                  const uint8_t *payload, uint16_t len,
                  hart_frame_t *resp, uint32_t timeout_ms)
{
    uint8_t txbuf[HART_MAX_FRAME + 32];
    uint16_t txlen = 0;
    int long_frame = (cmd == HART_CMD_READ_UNIQUE_ID) ? 1 : 0;

    hart_build_frame(txbuf, &txlen, addr, cmd, payload, len,
                     long_frame, 0);

    for (uint16_t attempt = 0; attempt < HART_MAX_TX_RETRIES; attempt++) {
        hart_send_raw(txbuf, txlen);
        delay_ms(g_hart.interframe_ms);
        if (resp) {
            int rc = hart_recv_frame(resp, timeout_ms);
            if (rc == 0) return 0;
        } else {
            return 0;  /* fire-and-forget */
        }
        delay_ms(20 * (attempt + 1));
    }
    g_hart.stats_timeout++;
    return -1;
}

/* ── Bus enumeration (Command 0 sweep) ───────────────────────── */
int hart_poll_addr(uint8_t poll_addr, hart_device_t *dev)
{
    hart_frame_t resp;
    int rc = hart_send_cmd(poll_addr, HART_CMD_READ_UNIQUE_ID,
                           NULL, 0, &resp, 800);
    if (rc != 0) return -1;
    if (resp.data_len >= 11) {
        /* cmd 0 response: 25-byte expanded device type + unique ID.
         * Format: [0] code(0xFE) [1] mfr [2] devtype
         * [3..7] unique ID (5 bytes) [8] pream # [9..14] tag [15..26] desc
         * Short response only returns 11 bytes (cmd 0): code, mfr, type, id(5), pream
         */
        if (dev) {
            dev->manufacturer = resp.data[1];
            dev->dev_type     = resp.data[2];
            memcpy(dev->long_addr, &resp.data[3], 5);
            dev->poll_addr = poll_addr;
            dev->cmd_0_done = 1;
        }
    }
    return 0;
}

int hart_poll_burst(uint8_t poll_addr)
{
    hart_frame_t resp;
    return hart_send_cmd(poll_addr, HART_CMD_POLL_SUBDEVICE,
                         NULL, 0, &resp, 800);
}

int hart_enumerate_bus(void)
{
    g_hart.n_devices = 0;
    for (uint8_t a = 0; a < 64; a++) {
        hart_device_t dev;
        memset(&dev, 0, sizeof(dev));
        if (hart_poll_addr(a, &dev) == 0) {
            if (g_hart.n_devices < HART_MAX_POLLED_DEVICES)
                g_hart.devices[g_hart.n_devices++] = dev;
        }
    }
    return g_hart.n_devices;
}

/* ── Init ────────────────────────────────────────────────────── */
int hart_init(void)
{
    memset(&g_hart, 0, sizeof(g_hart));
    g_hart.master_role = 0;       /* primary master by default */
    g_hart.interframe_ms = 30;    /* HART spec: ~20-30 ms between frames */
    modem_init();
    return 0;
}

void hart_set_master(uint8_t primary)
{
    g_hart.master_role = primary ? 0 : 1;
}

uint32_t hart_get_interframe(void)        { return g_hart.interframe_ms; }
void     hart_set_interframe(uint32_t ms) { g_hart.interframe_ms = ms; }

/* ── Pretty-printers ─────────────────────────────────────────── */
const char *hart_cmd_name(uint8_t cmd)
{
    switch (cmd) {
    case 0:  return "Read Unique ID";
    case 1:  return "Read Primary Variable";
    case 2:  return "Read Loop Current + % Range";
    case 3:  return "Read Dynamic Var + Loop Current";
    case 6:  return "Poll Sub-Device";
    case 9:  return "Read PV Status";
    case 11: return "Read Unique ID (assoc)";
    case 12: return "Read Tag, Descriptor, Date";
    case 13: return "Read Tag, Descriptor, Date";
    case 14: return "Read PV Sensor Info";
    case 17: return "Write Tag";
    case 35: return "Write Primary Variable";
    case 48: return "Read Additional Device Info";
    default: return "Unknown";
    }
}

const char *hart_status_str(uint8_t code)
{
    switch (code) {
    case 0:  return "OK";
    case 2:  return "Invalid Command";
    case 3:  return "Too Few Data Bytes";
    case 5:  return "Too Many Data Bytes";
    case 6:  return "Busy";
    case 7:  return "Diagnostic";
    case 0x0E: return "Hardware Failure";
    case 0x10: return "Mode Reject";
    case 0x12: return "Command Too Long";
    case 0x21: return "Access Restricted";
    default: return "Other";
    }
}