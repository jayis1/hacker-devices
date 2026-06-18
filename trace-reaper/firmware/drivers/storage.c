/*
 * TRACE-REAPER — encrypted session storage driver
 *
 * Persists completed session results (recovered key + per-byte rho + summary)
 * to the QSPI NOR and to microSD if present. Records are encrypted with an
 * AES-CTR keystream derived from the operator passkey (v1 uses the same
 * XOR-keystream as trace_buf for simplicity; the design notes the upgrade
 * path).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "storage.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* QSPI second half is reserved for session records (after the trace ring). */
#define SESSION_BASE   (QSPI_MEM_BASE + (4UL * 1024UL * 1024UL))
#define SESSION_SIZE    (4UL * 1024UL * 1024UL)
#define MAX_SESSIONS    64
#define REC_SIZE        (sizeof(session_record_t))

typedef struct {
    uint8_t  session_id[SESSION_ID_LEN];
    char     label[32];
    uint32_t started_ms;
    uint32_t finished_ms;
    uint32_t traces_used;
    uint8_t  cipher;
    uint8_t  leak_model;
    uint8_t  recovered_bytes;
    uint8_t  confidence_ok;
    uint8_t  best_key[KEY_BYTES_AES256];
    uint8_t  best_hyp[KEY_BYTES_AES256];
    uint8_t  rho_x100[KEY_BYTES_AES256]; /* rho * 100, 0..127 */
    uint8_t  crc8;
} session_record_t;

static session_record_t *s_records = (session_record_t *)SESSION_BASE;
static uint8_t g_passkey[16];
static uint8_t g_session_count = 0;

static uint8_t crc8_calc(const uint8_t *p, uint32_t n)
{
    uint8_t c = 0;
    for (uint32_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++)
            c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
    }
    return c;
}

static void enc_block(uint8_t *p, uint32_t n, uint32_t salt)
{
    for (uint32_t i = 0; i < n; i++)
        p[i] ^= g_passkey[(i + salt) & 15];
}

void storage_init(const uint8_t passkey[16])
{
    memcpy(g_passkey, passkey, 16);
    /* Count existing records by scanning for valid magic (CRC matches) */
    g_session_count = 0;
    for (uint32_t i = 0; i < MAX_SESSIONS; i++) {
        session_record_t *r = &s_records[i];
        uint8_t c = crc8_calc((const uint8_t *)r, sizeof(*r) - 1);
        if (c == r->crc8) g_session_count++;
    }
}

uint8_t storage_count(void) { return g_session_count; }

/* Encrypt in place before write, decrypt after read. We XOR the whole
 * record with the keystream salted by its slot index.
 */
int storage_save(const session_result_t *res, const session_cfg_t *cfg,
                 uint32_t ts_now)
{
    if (g_session_count >= MAX_SESSIONS) return -1;
    session_record_t r;
    memset(&r, 0, sizeof(r));
    memcpy(r.session_id, cfg->session_id, SESSION_ID_LEN);
    memcpy(r.label, cfg->label, sizeof(r.label));
    r.started_ms = res->started_ms;
    r.finished_ms = res->finished_ms;
    r.traces_used = res->traces_used;
    r.cipher = (uint8_t)cfg->cipher;
    r.leak_model = (uint8_t)cfg->model;
    r.recovered_bytes = res->recovered_bytes;
    r.confidence_ok = res->confidence_ok;
    memcpy(r.best_key, res->best_key, KEY_BYTES_AES256);
    memcpy(r.best_hyp, res->best_hyp, KEY_BYTES_AES256);
    for (int i = 0; i < KEY_BYTES_AES256; i++) {
        int v = (int)(res->rho[i] * 100.0f);
        if (v < 0) v = 0; if (v > 127) v = 127;
        r.rho_x100[i] = (uint8_t)v;
    }
    r.crc8 = crc8_calc((const uint8_t *)&r, sizeof(r) - 1);

    /* Encrypt the record body (everything except session_id and crc) */
    enc_block(((uint8_t *)&r) + SESSION_ID_LEN,
              sizeof(r) - SESSION_ID_LEN - 1, g_session_count);

    memcpy(&s_records[g_session_count], &r, sizeof(r));
    g_session_count++;
    (void)ts_now;
    return (int)(g_session_count - 1);
}

int storage_load(uint32_t idx, session_result_t *res, session_cfg_t *cfg,
                 char *label_out)
{
    if (idx >= g_session_count) return -1;
    session_record_t r = s_records[idx];
    /* Decrypt */
    enc_block(((uint8_t *)&r) + SESSION_ID_LEN,
              sizeof(r) - SESSION_ID_LEN - 1, idx);
    /* Verify CRC */
    uint8_t c = crc8_calc((const uint8_t *)&r, sizeof(r) - 1);
    if (c != r.crc8) return -2;

    if (res) {
        res->started_ms = r.started_ms;
        res->finished_ms = r.finished_ms;
        res->traces_used = r.traces_used;
        res->recovered_bytes = r.recovered_bytes;
        res->confidence_ok = r.confidence_ok;
        memcpy(res->best_key, r.best_key, KEY_BYTES_AES256);
        memcpy(res->best_hyp, r.best_hyp, KEY_BYTES_AES256);
        for (int i = 0; i < KEY_BYTES_AES256; i++)
            res->rho[i] = r.rho_x100[i] / 100.0f;
    }
    if (cfg) {
        cfg->cipher = (cipher_id_t)r.cipher;
        cfg->model = (leak_model_t)r.leak_model;
        memcpy(cfg->session_id, r.session_id, SESSION_ID_LEN);
        memcpy(cfg->label, r.label, sizeof(r.label));
    }
    if (label_out) memcpy(label_out, r.label, 32);
    return 0;
}

void storage_wipe_all(void)
{
    memset((void *)SESSION_BASE, 0, SESSION_SIZE);
    g_session_count = 0;
}