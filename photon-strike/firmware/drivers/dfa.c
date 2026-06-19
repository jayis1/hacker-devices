/**
 * drivers/dfa.c — Piret-Quisquater AES-128 DFA solver
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements the classic Piret-Quisquater (2003) differential fault
 * attack on AES-128:
 *
 *   A single-byte fault injected just before the last MixColumns of
 *   the last round propagates through ShiftRows + AddRoundKey (there
 *   is no MixColumns in the final round) into exactly four ciphertext
 *   bytes — one column of the state. For each candidate fault value
 *   f (1..255) and each of the four affected key bytes k0..k3, the
 *   relation
 *
 *       C_i'  = SubBytes( ShiftRows( state_before_last_SR ) )_i  XOR  K_i
 *
 *   yields a small set of consistent (k0,k1,k2,k3) candidates. With
 *   one good faulted pair, the key space for those four bytes shrinks
 *   to ~2^8; a second faulted pair in a different column recovers the
 *   next four bytes; four pairs total recover all 16 key bytes.
 *
 * This implementation is fixed-point C, no dynamic allocation, and runs
 * in milliseconds on the Cortex-M7. It is invoked by the scan loop
 * every time a SINGLE_BYTE or clean MULTI_BYTE fault is captured.
 *
 * Reference:
 *   G. Piret and J.-J. Quisquater, "A Differential Fault Attack
 *   Technique against SPN Structures, with Application to AES, MERKLE-
 *   HELLMAN and Other Instances," Eurocrypt 2003.
 */

#include "dfa.h"
#include <string.h>

/* ─── AES S-box & inverse S-box ───────────────────────────────────────── */
static const uint8_t sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t inv_sbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

/* GF(2^8) multiply by 2 (xtime) and general multiply. */
static uint8_t xtime(uint8_t x)
{
    return (uint8_t)((x << 1) ^ ((x >> 7) * 0x1B));
}
static uint8_t gmul(uint8_t a, uint8_t b)
{
    uint8_t r = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1u) r ^= a;
        a = xtime(a);
        b >>= 1;
    }
    return r;
}

/* ─── Piret-Quisquater single-fault solver for one column ───────────────
 * Given the correct ciphertext C and faulted ciphertext C', and the
 * AES state byte index where the fault was injected (0..15), compute
 * the set of candidate last-round-key bytes for the four affected
 * ciphertext positions.
 *
 * Fault propagation (Piret-Quisquater):
 *   The fault f at state byte s (column c = s/4, row r = s%4) before
 *   the last MixColumns propagates through the last ShiftRows into
 *   four ciphertext bytes. The four positions are determined by the
 *   ShiftRows rotation of column c.
 *
 * For each candidate fault value f in 1..255:
 *   delta = f (a single nonzero byte at position s, zero elsewhere)
 *   After the last MixColumns (which DOES happen for the faulted
 *   state, because the fault is injected *before* it):
 *     delta' = MixColumns(delta)
 *   After the last ShiftRows + AddRoundKey:
 *     C' XOR C = ShiftRows(delta')
 *   So for each of the 4 affected ciphertext bytes:
 *     diff_i = (C' XOR C)_i  =  (ShiftRows(MixColumns(delta)))_i
 *   We solve for the key byte k_i by:
 *     k_i = inv_sbox( C_i  XOR  (ShiftRows(MixColumns(delta)))_i )  XOR  ... 
 *   Actually the cleaner relation is:
 *     Let MC = MixColumns applied to a state with only byte s = f.
 *     Let SR = ShiftRows(MC).
 *     Then  C'_i = SubBytes(state_i) XOR k_i
 *     and   C_i  = SubBytes(state_i) XOR k_i   (correct)
 *     so    C'_i XOR C_i = SR_i
 *   The key byte for position i is recovered by inverting the last
 *   round on the correct ciphertext:
 *     k_i = C_i XOR inv_SubBytes(state_i)
 *   But state_i is unknown. Instead we use the relation:
 *     SubBytes^{-1}(C_i  XOR k_i) = SubBytes^{-1}(C'_i XOR k_i) XOR SR_i
 *   For each candidate (f, k_i):
 *     if  SubBytes^{-1}(C_i  XOR k_i) XOR SubBytes^{-1}(C'_i XOR k_i) == SR_i
 *         then (f, k_i) is consistent.
 *   We collect the intersection across all 4 positions; for a genuine
 *   single-byte fault there is exactly one f and one k per position.
 */
static void solve_column(const uint8_t C[16], const uint8_t Cp[16],
                         uint8_t fault_byte_idx,
                         uint8_t out_key[4])
{
    /* The four affected ciphertext positions, given the fault byte's
     * column c = fault_byte_idx / 4. After ShiftRows the column is
     * spread to rows. We compute the diff vector SR (4 bytes at the
     * positions corresponding to the column). */
    uint8_t c = fault_byte_idx / 4;
    /* ShiftRows maps column c to positions (row r, col (c - r) mod 4)
     * in the standard AES state layout. The 4 affected ciphertext
     * byte indices (in row-major byte order: byte = row + 4*col) are: */
    uint8_t pos[4];
    for (uint8_t r = 0; r < 4; r++) {
        uint8_t col = (uint8_t)((c + r) & 3u);   /* inverse ShiftRows */
        pos[r] = (uint8_t)(r + 4u * col);
    }

    /* For each candidate fault value f (1..255), compute the 4-byte
     * MixColumns output for a state with only byte (r=fault_byte_idx%4,
     * col=c) set to f. MixColumns of a single nonzero byte at row r0
     * produces 4 bytes in column c:
     *   mc[0] = 2*f if r0=0, else 3*f if r0=1, else f if r0=2, else f if r0=3 ... 
     * The standard MixColumns matrix is:
     *   [2 3 1 1]
     *   [1 2 3 1]
     *   [1 1 2 3]
     *   [3 1 1 2]
     * so a fault at row r0 in column c contributes:
     *   mc[r] = M[r][r0] * f
     */
    uint8_t r0 = fault_byte_idx % 4;
    static const uint8_t M[4][4] = {
        {2,3,1,1},
        {1,2,3,1},
        {1,1,2,3},
        {3,1,1,2}
    };

    /* The four ciphertext diffs after ShiftRows are MC shifted: the
     * diff at ciphertext position pos[r] is mc[r] = M[r][r0]*f. */
    uint8_t key_cands[4][256];
    uint8_t key_ncands[4];
    for (uint8_t r = 0; r < 4; r++) key_ncands[r] = 0;

    for (uint16_t f = 1; f <= 255; f++) {
        /* mc[r] for this f */
        uint8_t mc[4];
        for (uint8_t r = 0; r < 4; r++)
            mc[r] = gmul(M[r][r0], (uint8_t)f);

        /* For each of the 4 positions, find key bytes k consistent with
         *   SubBytes^{-1}(C[pos]  XOR k) XOR SubBytes^{-1}(Cp[pos] XOR k) == mc[r]
         */
        uint8_t cand[4][256];
        uint8_t ncand[4] = {0,0,0,0};
        for (uint8_t r = 0; r < 4; r++) {
            uint8_t Ci  = C[pos[r]];
            uint8_t Cpi = Cp[pos[r]];
            for (uint16_t k = 0; k < 256; k++) {
                uint8_t a = inv_sbox[(uint8_t)(Ci  ^ (uint8_t)k)];
                uint8_t b = inv_sbox[(uint8_t)(Cpi ^ (uint8_t)k)];
                if ((uint8_t)(a ^ b) == mc[r]) {
                    cand[r][ncand[r]++] = (uint8_t)k;
                }
            }
            if (ncand[r] == 0) goto next_f;   /* this f is impossible */
        }

        /* f is consistent. Merge candidates into the global set. We
         * only keep candidates that appear for *some* consistent f. */
        for (uint8_t r = 0; r < 4; r++) {
            for (uint8_t i = 0; i < ncand[r]; i++) {
                uint8_t k = cand[r][i];
                bool found = false;
                for (uint8_t j = 0; j < key_ncands[r]; j++)
                    if (key_cands[r][j] == k) { found = true; break; }
                if (!found && key_ncands[r] < 255)
                    key_cands[r][key_ncands[r]++] = k;
            }
        }
    next_f:;
    }

    /* If a position has exactly one candidate, it's the key byte. */
    for (uint8_t r = 0; r < 4; r++) {
        if (key_ncands[r] == 1)
            out_key[r] = key_cands[r][0];
        else
            out_key[r] = 0xFFu;   /* ambiguous */
    }
}

/* ─── Public API ──────────────────────────────────────────────────────── */
void dfa_init(ps_dfa_state_t *s)
{
    memset(s, 0, sizeof(*s));
    memset(s->key_candidates, 0xFF, 16);
    memset(s->key_unique, 0, 16);
}

void dfa_feed(ps_dfa_state_t *s, const uint8_t *correct,
              const uint8_t *faulted, uint16_t len)
{
    if (len < 16) return;
    memcpy(s->correct_ct,  correct, 16);
    memcpy(s->faulted_ct, faulted, 16);
    s->fault_count++;
}

int dfa_solve(ps_dfa_state_t *s, uint8_t out_key[16])
{
    /* Locate the single faulted byte: the byte that differs between
     * correct and faulted ciphertexts *before* ShiftRows propagation.
     * Because the fault propagates through ShiftRows into 4 bytes, we
     * actually need to know the injection byte index. In the scan
     * context we try all 16 possible injection indices and keep the
     * one that yields unique key bytes. */
    if (s->fault_count == 0) {
        memcpy(out_key, s->key_candidates, 16);
        int u = 0;
        for (int i = 0; i < 16; i++) if (s->key_unique[i]) u++;
        return u;
    }

    /* Try each possible injection byte; pick the one that resolves the
     * most still-unknown key bytes. */
    int best_unique = 0;
    uint8_t best_key[16];
    memcpy(best_key, s->key_candidates, 16);

    for (uint8_t fi = 0; fi < 16; fi++) {
        uint8_t col_key[4];
        solve_column(s->correct_ct, s->faulted_ct, fi, col_key);
        uint8_t c = fi / 4;
        /* The four affected key-byte positions match the ShiftRows
         * spread of column c, same as the pos[] computation above. */
        for (uint8_t r = 0; r < 4; r++) {
            uint8_t col = (uint8_t)((c + r) & 3u);
            uint8_t pos = (uint8_t)(r + 4u * col);
            if (col_key[r] != 0xFFu) {
                if (!s->key_unique[pos]) {
                    s->key_candidates[pos] = col_key[r];
                    s->key_unique[pos] = 1u;
                } else if (s->key_candidates[pos] != col_key[r]) {
                    /* contradiction — mark ambiguous again */
                    s->key_unique[pos] = 0u;
                    s->key_candidates[pos] = 0xFFu;
                }
            }
        }

        int u = 0;
        for (int i = 0; i < 16; i++) if (s->key_unique[i]) u++;
        if (u > best_unique) {
            best_unique = u;
            memcpy(best_key, s->key_candidates, 16);
        }
    }

    memcpy(out_key, s->key_candidates, 16);
    return best_unique;
}