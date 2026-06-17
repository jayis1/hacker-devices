/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Ultrasonic Tx/Rx Driver — Implementation
 *
 * Real-time Goertzel-based FSK/OOK/whisper modulation and
 * demodulation for the ultrasonic transducers.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "ultrasonic_driver.h"
#include <math.h>
#include <string.h>

/* =========================================================================
 * Private Helpers
 * ========================================================================= */

/* PI constant */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* Manchester encoding: 0 → 01, 1 → 10 */
static inline uint8_t manchester_encode(uint8_t bit)
{
    return bit ? 0x2 : 0x1; /* 10b for 1, 01b for 0 */
}

/* Manchester decoding: 01 → 0, 10 → 1 */
static inline int8_t manchester_decode(uint8_t half_a, uint8_t half_b)
{
    if (half_a == 0 && half_b == 1) return 0;
    if (half_a == 1 && half_b == 0) return 1;
    return -1; /* Invalid transition */
}

/* =========================================================================
 * Goertzel Algorithm Implementation
 * ========================================================================= */

void goertzel_init(goertzel_state_t *state, float freq, uint16_t n, float fs)
{
    if (!state || n == 0 || fs <= 0.0f)
        return;

    float k = (float)n * freq / fs;
    float omega = 2.0f * (float)M_PI * k / (float)n;

    state->s_prev  = 0.0f;
    state->s_prev2 = 0.0f;
    state->coeff   = 2.0f * cosf(omega);
    state->n       = n;
    state->count   = 0;
}

void goertzel_process(goertzel_state_t *state, float sample)
{
    if (!state)
        return;

    float q0 = state->coeff * state->s_prev - state->s_prev2 + sample;

    state->s_prev2 = state->s_prev;
    state->s_prev  = q0;
    state->count++;
}

float goertzel_get_energy(goertzel_state_t *state)
{
    if (!state || state->count == 0)
        return 0.0f;

    float real = state->s_prev - state->s_prev2 * cosf(2.0f * (float)M_PI * (float)state->count / (float)state->n);
    float imag = state->s_prev2 * sinf(2.0f * (float)M_PI * (float)state->count / (float)state->n);

    float magnitude = real * real + imag * imag;

    /* Reset state for next block */
    state->s_prev  = 0.0f;
    state->s_prev2 = 0.0f;
    state->count   = 0;

    /* Normalize by N^2 for energy */
    return magnitude / ((float)state->n * (float)state->n);
}

int goertzel_bit_decision(const float *samples, uint16_t n_samples,
                          float freq_mark, float freq_space, float fs,
                          float *out_mark_energy, float *out_space_energy)
{
    goertzel_state_t g_mark, g_space;
    float e_mark = 0.0f, e_space = 0.0f;

    goertzel_init(&g_mark, freq_mark, n_samples, fs);
    goertzel_init(&g_space, freq_space, n_samples, fs);

    for (uint16_t i = 0; i < n_samples; i++) {
        goertzel_process(&g_mark, samples[i]);
        goertzel_process(&g_space, samples[i]);
    }

    e_mark  = goertzel_get_energy(&g_mark);
    e_space = goertzel_get_energy(&g_space);

    if (out_mark_energy)  *out_mark_energy  = e_mark;
    if (out_space_energy) *out_space_energy = e_space;

    /* Ratio of 2.0 (3 dB) required for confident decision */
    if (e_mark > e_space * 2.0f)
        return 1;
    else if (e_space > e_mark * 2.0f)
        return 0;
    else
        return -1; /* Uncertain */
}

/* =========================================================================
 * FSK Modulator
 * ========================================================================= */

void fsk_mod_init(fsk_modulator_t *mod, float f_mark, float f_space,
                  uint32_t baud, float amp, uint32_t fs)
{
    if (!mod) return;

    mod->freq_mark        = f_mark;
    mod->freq_space       = f_space;
    mod->baud             = baud;
    mod->fs_tx            = fs;
    mod->amplitude        = (amp < 0.0f) ? 0.0f : (amp > 1.0f) ? 1.0f : amp;
    mod->phase            = 0.0f;
    mod->phase_delta_mark = 2.0f * (float)M_PI * f_mark / (float)fs;
    mod->phase_delta_space= 2.0f * (float)M_PI * f_space / (float)fs;
    mod->samples_per_bit  = fs / baud;
    mod->bit_count        = 0;
    mod->bit_index        = 0;
    mod->sample_index     = 0;
    mod->active           = 0;

    /* Clear bit buffer */
    memset(mod->bit_buffer, 0, sizeof(mod->bit_buffer));
}

int fsk_mod_load_message(fsk_modulator_t *mod, const uint8_t *data, uint16_t len)
{
    if (!mod || !data || len == 0 || len > 64)
        return ERR_INVALID_PARAM;

    uint16_t bit_pos = 0;
    uint8_t  bit;

    /* --- Preamble: 10 cycles of 22 kHz tone --- */
    /* Each cycle = 2 bits (Manchester: 01 for '0', which gives alternating tone) */
    /* Actually, preamble is analog — we handle it by transmitting a fixed tone
     * before the data bits. For now, signal it with 16 alternating bits. */
    for (int p = 0; p < 16; p++) {
        if (bit_pos >= sizeof(mod->bit_buffer) * 8) break;
        /* Preamble marker — these bits won't be Manchester encoded;
         * they just keep the carrier on at ~22 kHz (same as Space freq)
         * to allow the receiver to detect signal presence. */
        bit = 1; /* Will produce carrier at mark freq — close to 22k */
        mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
        bit_pos++;
    }

    /* --- Sync: 4-bit Barker code 1010, Manchester encoded twice --- */
    /* Each Barker bit → Manchester pair → 2 bits */
    for (int b = 0; b < 4; b++) {
        if (bit_pos >= sizeof(mod->bit_buffer) * 8 - 2) break;
        bit = (BARKER_CODE >> (3 - b)) & 0x01;
        uint8_t enc = manchester_encode(bit);
        for (int m = 1; m >= 0; m--) {
            bit = (enc >> m) & 0x01;
            mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
            bit_pos++;
        }
    }

    /* --- Length byte (payload length), Manchester encoded --- */
    if (bit_pos >= sizeof(mod->bit_buffer) * 8 - 16) return ERR_BUF_FULL;
    for (int byte = 0; byte < 2; byte++) {
        /* First byte = payload_len, second = 0 (reserved) */
        uint8_t lb = (byte == 0) ? len : 0;
        for (int b = 7; b >= 0; b--) {
            bit = (lb >> b) & 0x01;
            uint8_t enc = manchester_encode(bit);
            for (int m = 1; m >= 0; m--) {
                uint8_t mb = (enc >> m) & 0x01;
                if (bit_pos >= sizeof(mod->bit_buffer) * 8) return ERR_BUF_FULL;
                mod->bit_buffer[bit_pos / 8] |= (mb << (7 - (bit_pos % 8)));
                bit_pos++;
            }
        }
    }

    /* --- Payload bytes, Manchester encoded --- */
    for (uint16_t i = 0; i < len; i++) {
        if (bit_pos >= sizeof(mod->bit_buffer) * 8 - 16) return ERR_BUF_FULL;
        for (int b = 7; b >= 0; b--) {
            bit = (data[i] >> b) & 0x01;
            uint8_t enc = manchester_encode(bit);
            for (int m = 1; m >= 0; m--) {
                uint8_t mb = (enc >> m) & 0x01;
                if (bit_pos >= sizeof(mod->bit_buffer) * 8) return ERR_BUF_FULL;
                mod->bit_buffer[bit_pos / 8] |= (mb << (7 - (bit_pos % 8)));
                bit_pos++;
            }
        }
    }

    /* --- CRC-16 appended (already inside payload... we'd need to recompute.
     * For now, assume CRC is appended by frame_build_bits() and included
     * in the payload data passed to this function. */

    mod->bit_count  = bit_pos;
    mod->bit_index  = 0;
    mod->sample_index = 0;
    mod->active     = 1;

    return ERR_OK;
}

float fsk_mod_tick(fsk_modulator_t *mod)
{
    if (!mod || !mod->active)
        return 0.0f;

    /* Determine current bit value */
    uint8_t bit = (mod->bit_buffer[mod->bit_index / 8] >> (7 - (mod->bit_index % 8))) & 0x01;
    float freq = bit ? mod->freq_mark : mod->freq_space;

    /* Generate sine sample */
    float sample = mod->amplitude * sinf(mod->phase);

    /* Advance DDS phase */
    float delta = bit ? mod->phase_delta_mark : mod->phase_delta_space;
    mod->phase += delta;
    if (mod->phase >= 2.0f * (float)M_PI)
        mod->phase -= 2.0f * (float)M_PI;

    /* Advance sample counter */
    mod->sample_index++;
    if (mod->sample_index >= mod->samples_per_bit) {
        mod->sample_index = 0;
        mod->bit_index++;
        if (mod->bit_index >= mod->bit_count) {
            mod->active = 0; /* Transmission complete */
        }
    }

    return sample;
}

int fsk_mod_is_done(const fsk_modulator_t *mod)
{
    return mod ? !mod->active : 1;
}

/* =========================================================================
 * FSK Demodulator
 * ========================================================================= */

void fsk_demod_init(fsk_demodulator_t *dem, float f_mark, float f_space,
                    uint32_t baud, float fs)
{
    if (!dem) return;

    dem->freq_mark         = f_mark;
    dem->freq_space        = f_space;
    dem->baud              = baud;
    dem->fs_rx             = fs;
    dem->samples_per_bit   = fs / baud;

    /* Initialize Goertzel filters for mark and space frequencies */
    goertzel_init(&dem->goertzel_mark,  f_mark,  GOERTZEL_N, fs);
    goertzel_init(&dem->goertzel_space, f_space, GOERTZEL_N, fs);

    dem->snr_estimate      = -100.0f; /* Very poor until we get signal */
    dem->prev_bit          = 0;
    dem->manchester_state  = 0;
    dem->manchester_half   = 0;
    dem->preamble_count    = 0;
    dem->synced            = 0;
    dem->sync_shift_reg    = 0;
    dem->rx_byte_index     = 0;
    dem->rx_bit_pos        = 0;
    dem->rx_frame_length   = 0;
    dem->rx_bytes_received = 0;
    dem->frame_ready       = 0;

    memset(dem->rx_buffer, 0, sizeof(dem->rx_buffer));
}

void fsk_demod_process(fsk_demodulator_t *dem, float sample)
{
    if (!dem)
        return;

    /* Feed sample into both Goertzel filters */
    goertzel_process(&dem->goertzel_mark, sample);
    goertzel_process(&dem->goertzel_space, sample);

    /* Every GOERTZEL_N samples, make a tone decision */
    if (dem->goertzel_mark.count >= dem->goertzel_mark.n) {
        float e_mark  = goertzel_get_energy(&dem->goertzel_mark);
        float e_space = goertzel_get_energy(&dem->goertzel_space);

        /* Re-initialize for next block */
        goertzel_init(&dem->goertzel_mark,  dem->freq_mark,  GOERTZEL_N, dem->fs_rx);
        goertzel_init(&dem->goertzel_space, dem->freq_space, GOERTZEL_N, dem->fs_rx);

        /* Compute SNR estimate from ratio */
        float total = e_mark + e_space;
        if (total > 1e-10f) {
            float ratio = (e_mark > e_space) ? (e_mark / e_space) : (e_space / e_mark);
            dem->snr_estimate = 10.0f * log10f(ratio);
        }

        /* --- Preamble detection --- */
        /* Before sync, look for strong energy at either frequency */
        if (!dem->synced) {
            /* Check for signal presence (either tone above threshold) */
            if (e_mark > 0.0001f || e_space > 0.0001f) {
                dem->preamble_count++;
                if (dem->preamble_count >= 3) {
                    /* Signal detected — look for Barker sync word */
                    dem->synced = 1;
                    dem->preamble_count = 0;
                }
            } else {
                dem->preamble_count = 0;
            }
            return;
        }

        /* --- Synchronized: decode bit --- */
        int bit = -1;
        if (e_mark > e_space * 2.0f)
            bit = 1;
        else if (e_space > e_mark * 2.0f)
            bit = 0;

        if (bit < 0)
            return; /* Uncertain — skip */

        /* --- Manchester decode: need two consecutive half-bit decisions --- */
        if (dem->manchester_state == 0) {
            /* First half-bit */
            dem->manchester_half = (uint8_t)bit;
            dem->manchester_state = 1;
        } else {
            /* Second half-bit — combine */
            int8_t decoded_bit = manchester_decode(dem->manchester_half, (uint8_t)bit);
            dem->manchester_state = 0;

            if (decoded_bit < 0)
                return; /* Invalid Manchester transition — reset */

            /* --- Assemble frame --- */
            if (dem->rx_bit_pos == 0) {
                /* First bit of a new byte */
                if (dem->rx_byte_index == 0 && decoded_bit == 1) {
                    /* Start of sync word marker — ignore the very first bit
                     * preamble may push through. Keep going. */
                }
            }

            dem->rx_buffer[dem->rx_byte_index] |= ((uint8_t)decoded_bit << (7 - dem->rx_bit_pos));
            dem->rx_bit_pos++;

            if (dem->rx_bit_pos >= 8) {
                dem->rx_bit_pos = 0;
                dem->rx_bytes_received = dem->rx_byte_index + 1;

                /* Check if we've received the length byte + payload + CRC */
                if (dem->rx_byte_index == 0) {
                    /* First byte = frame length */
                    dem->rx_frame_length = dem->rx_buffer[0] + 3; /* length + CRC-16 + sync byte? */
                    if (dem->rx_frame_length > 128)
                        dem->rx_frame_length = 128;
                }

                dem->rx_byte_index++;

                /* Check for frame completion */
                if (dem->rx_byte_index >= dem->rx_frame_length && dem->rx_frame_length > 0) {
                    dem->frame_ready = 1;
                    /* Reset for next frame */
                    dem->rx_byte_index = 0;
                    dem->rx_bytes_received = 0;
                    dem->synced = 0; /* Wait for next preamble */
                    dem->preamble_count = 0;
                }
            }
        }
    }
}

int fsk_demod_frame_ready(const fsk_demodulator_t *dem)
{
    return dem ? dem->frame_ready : 0;
}

int fsk_demod_get_frame(fsk_demodulator_t *dem, uint8_t *out_data, uint16_t *out_len)
{
    if (!dem || !out_data || !out_len)
        return ERR_INVALID_PARAM;
    if (!dem->frame_ready)
        return ERR_NO_DATA;

    *out_len = dem->rx_frame_length;
    memcpy(out_data, dem->rx_buffer, *out_len);

    return ERR_OK;
}

void fsk_demod_clear_frame(fsk_demodulator_t *dem)
{
    if (dem) {
        dem->frame_ready = 0;
        dem->rx_byte_index = 0;
        dem->rx_bit_pos = 0;
        dem->rx_frame_length = 0;
        memset(dem->rx_buffer, 0, sizeof(dem->rx_buffer));
    }
}

float fsk_demod_get_snr(const fsk_demodulator_t *dem)
{
    return dem ? dem->snr_estimate : -100.0f;
}

void fsk_demod_set_baud(fsk_demodulator_t *dem, uint32_t new_baud)
{
    if (dem && new_baud > 0) {
        dem->baud = new_baud;
        dem->samples_per_bit = dem->fs_rx / new_baud;
    }
}

/* =========================================================================
 * OOK Modulator
 * ========================================================================= */

void ook_mod_init(ook_modulator_t *mod, float carrier_freq,
                  uint32_t bit_duration_us, float amp, uint32_t fs)
{
    if (!mod) return;

    mod->carrier_freq     = carrier_freq;
    mod->bit_duration_us  = bit_duration_us;
    mod->amplitude        = (amp < 0.0f) ? 0.0f : (amp > 1.0f) ? 1.0f : amp;
    mod->phase            = 0.0f;
    mod->phase_delta      = 2.0f * (float)M_PI * carrier_freq / (float)fs;
    mod->fs_tx            = fs;
    mod->samples_per_bit  = (uint32_t)((float)fs * (float)bit_duration_us / 1000000.0f);
    mod->bit_count        = 0;
    mod->bit_index        = 0;
    mod->sample_index     = 0;
    mod->active           = 0;

    memset(mod->bit_buffer, 0, sizeof(mod->bit_buffer));
}

int ook_mod_load_message(ook_modulator_t *mod, const uint8_t *data, uint16_t len)
{
    if (!mod || !data || len == 0 || len > 64)
        return ERR_INVALID_PARAM;

    uint16_t bit_pos = 0;

    /* OOK preamble: 8 carrier-on bits for AGC settling */
    for (int i = 0; i < 8; i++) {
        if (bit_pos >= sizeof(mod->bit_buffer) * 8) return ERR_BUF_FULL;
        mod->bit_buffer[bit_pos / 8] |= (1 << (7 - (bit_pos % 8))); /* All ones = carrier on */
        bit_pos++;
    }

    /* Sync word: 8-bit pattern 0xA5 (10100101) */
    for (int b = 7; b >= 0; b--) {
        if (bit_pos >= sizeof(mod->bit_buffer) * 8) return ERR_BUF_FULL;
        uint8_t bit = (0xA5 >> b) & 0x01;
        mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
        bit_pos++;
    }

    /* Length byte */
    for (int b = 7; b >= 0; b--) {
        if (bit_pos >= sizeof(mod->bit_buffer) * 8) return ERR_BUF_FULL;
        uint8_t bit = (len >> b) & 0x01;
        mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
        bit_pos++;
    }

    /* Payload bytes */
    for (uint16_t i = 0; i < len; i++) {
        for (int b = 7; b >= 0; b--) {
            if (bit_pos >= sizeof(mod->bit_buffer) * 8) return ERR_BUF_FULL;
            uint8_t bit = (data[i] >> b) & 0x01;
            mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
            bit_pos++;
        }
    }

    mod->bit_count   = bit_pos;
    mod->bit_index   = 0;
    mod->sample_index = 0;
    mod->active      = 1;

    return ERR_OK;
}

float ook_mod_tick(ook_modulator_t *mod)
{
    if (!mod || !mod->active)
        return 0.0f;

    uint8_t bit = (mod->bit_buffer[mod->bit_index / 8] >> (7 - (mod->bit_index % 8))) & 0x01;

    float sample;
    if (bit) {
        /* Carrier ON */
        sample = mod->amplitude * sinf(mod->phase);
        mod->phase += mod->phase_delta;
        if (mod->phase >= 2.0f * (float)M_PI)
            mod->phase -= 2.0f * (float)M_PI;
    } else {
        /* Carrier OFF — silence */
        sample = 0.0f;
    }

    mod->sample_index++;
    if (mod->sample_index >= mod->samples_per_bit) {
        mod->sample_index = 0;
        mod->bit_index++;
        if (mod->bit_index >= mod->bit_count) {
            mod->active = 0;
        }
    }

    return sample;
}

int ook_mod_is_done(const ook_modulator_t *mod)
{
    return mod ? !mod->active : 1;
}

/* =========================================================================
 * OOK Demodulator
 * ========================================================================= */

void ook_demod_init(ook_demodulator_t *dem, float carrier_freq,
                    uint32_t bit_duration_us, float fs)
{
    if (!dem) return;

    dem->carrier_freq       = carrier_freq;
    dem->bit_duration_us    = bit_duration_us;
    dem->fs_rx              = fs;
    dem->envelope           = 0.0f;
    dem->noise_floor        = 0.0f;
    dem->threshold          = 0.1f;
    dem->env_avg_count      = 0;
    dem->sample_count       = 0;
    dem->samples_per_half_bit = (uint32_t)((float)fs * (float)bit_duration_us / 2000000.0f);
    dem->bit_value          = 0;
    dem->bit_ready          = 0;
    dem->synced             = 0;
    dem->sync_timer         = 0;
    dem->rx_byte_index      = 0;
    dem->rx_bit_pos         = 0;
    dem->frame_ready        = 0;

    memset(dem->rx_buffer, 0, sizeof(dem->rx_buffer));
}

void ook_demod_process(ook_demodulator_t *dem, float sample)
{
    if (!dem) return;

    /* Compute instantaneous envelope magnitude (rectify + LPF) */
    float abs_sample = (sample < 0.0f) ? -sample : sample;
    /* Simple one-pole IIR LPF with fast attack, slow decay */
    if (abs_sample > dem->envelope) {
        dem->envelope = dem->envelope * 0.5f + abs_sample * 0.5f; /* Fast attack */
    } else {
        dem->envelope = dem->envelope * 0.9f + abs_sample * 0.1f; /* Slow decay */
    }

    /* Update noise floor (track minimum envelope) */
    if (dem->env_avg_count < 1000) {
        /* Initial noise floor estimation */
        dem->noise_floor = dem->noise_floor * 0.99f + abs_sample * 0.01f;
        dem->env_avg_count++;
    } else {
        /* Track slowly in operation */
        if (dem->envelope < dem->noise_floor * 1.5f) {
            dem->noise_floor = dem->noise_floor * 0.995f + dem->envelope * 0.005f;
        }
    }

    /* Adaptive threshold: noise floor + 6 dB */
    dem->threshold = dem->noise_floor * 2.0f;
    if (dem->threshold < 0.005f) /* Minimum threshold floor */
        dem->threshold = 0.005f;

    /* Bit timing: sample at half-bit intervals */
    dem->sample_count++;
    if (dem->sample_count >= dem->samples_per_half_bit) {
        dem->sample_count = 0;

        /* Decision: carrier ON/OFF */
        uint8_t bit = (dem->envelope > dem->threshold) ? 1 : 0;

        if (!dem->synced) {
            /* Look for 4 consecutive ON bits (preamble) */
            if (bit == 1) {
                dem->sync_timer++;
                if (dem->sync_timer >= 4) {
                    dem->synced = 1;
                    dem->sync_timer = 0;
                }
            } else {
                dem->sync_timer = 0;
            }
            return;
        }

        /* Decode bit */
        dem->bit_value = bit;

        /* Build frame */
        if (dem->rx_bit_pos == 0) {
            dem->rx_buffer[dem->rx_byte_index] = 0;
        }

        dem->rx_buffer[dem->rx_byte_index] |= (bit << (7 - dem->rx_bit_pos));
        dem->rx_bit_pos++;

        if (dem->rx_bit_pos >= 8) {
            dem->rx_bit_pos = 0;
            dem->rx_byte_index++;

            /* Check for frame length */
            if (dem->rx_byte_index >= 2) {
                /* First byte was sync (0xA5), second is length */
                /* After length, we need length + CRC bytes */
                if (dem->rx_byte_index >= (dem->rx_buffer[1] + 4)) {
                    dem->frame_ready = 1;
                    dem->rx_byte_index = 0;
                    dem->synced = 0;
                    dem->sync_timer = 0;
                }
            }
        }
    }
}

int ook_demod_frame_ready(const ook_demodulator_t *dem)
{
    return dem ? dem->frame_ready : 0;
}

int ook_demod_get_frame(ook_demodulator_t *dem, uint8_t *out_data, uint16_t *out_len)
{
    if (!dem || !out_data || !out_len)
        return ERR_INVALID_PARAM;
    if (!dem->frame_ready)
        return ERR_NO_DATA;

    *out_len = dem->rx_byte_index;
    memcpy(out_data, dem->rx_buffer, *out_len);
    return ERR_OK;
}

void ook_demod_clear_frame(ook_demodulator_t *dem)
{
    if (dem) {
        dem->frame_ready = 0;
        dem->rx_byte_index = 0;
        dem->rx_bit_pos = 0;
        memset(dem->rx_buffer, 0, sizeof(dem->rx_buffer));
    }
}

/* =========================================================================
 * Whisper Mode Modulator
 * ========================================================================= */

void whisper_mod_init(whisper_modulator_t *mod, float amp, float fs, float bps)
{
    if (!mod) return;

    mod->active         = 0;
    mod->fs_tx          = fs;
    mod->amplitude      = (amp < 0.0f) ? 0.0f : (amp > 1.0f) ? 1.0f : amp;
    mod->hop_index      = 0;
    mod->chip_index     = 0;
    mod->samples_per_chip = (uint32_t)(fs / (bps * WHISPER_BARKER_CHIPS * WHISPER_HOP_SEQUENCE_LEN));
    if (mod->samples_per_chip < 10)
        mod->samples_per_chip = 10; /* Minimum */
    mod->sample_index   = 0;
    mod->phase          = 0.0f;
    mod->bit_count      = 0;
    mod->bit_index      = 0;

    memset(mod->bit_buffer, 0, sizeof(mod->bit_buffer));
}

int whisper_mod_load_message(whisper_modulator_t *mod, const uint8_t *data, uint16_t len)
{
    if (!mod || !data || len == 0 || len > 64)
        return ERR_INVALID_PARAM;

    /* Whisper mode adds preamble and sync similar to FSK but at lower rate */
    uint16_t bit_pos = 0;

    /* Preamble: 4 bits of all 1s for AGC */
    for (int i = 0; i < 4; i++) {
        mod->bit_buffer[bit_pos / 8] |= (1 << (7 - (bit_pos % 8)));
        bit_pos++;
    }

    /* Sync word: 4-bit Barker, raw (not Manchester, since we're using spread) */
    for (int b = 3; b >= 0; b--) {
        uint8_t bit = (BARKER_CODE >> b) & 0x01;
        mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
        bit_pos++;
    }

    /* Length byte */
    for (int b = 7; b >= 0; b--) {
        uint8_t bit = (len >> b) & 0x01;
        mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
        bit_pos++;
    }

    /* Payload */
    for (uint16_t i = 0; i < len; i++) {
        for (int b = 7; b >= 0; b--) {
            uint8_t bit = (data[i] >> b) & 0x01;
            mod->bit_buffer[bit_pos / 8] |= (bit << (7 - (bit_pos % 8)));
            bit_pos++;
        }
    }

    mod->bit_count  = bit_pos;
    mod->bit_index  = 0;
    mod->hop_index  = 0;
    mod->chip_index = 0;
    mod->sample_index = 0;
    mod->phase      = 0.0f;
    mod->active     = 1;

    return ERR_OK;
}

float whisper_mod_tick(whisper_modulator_t *mod)
{
    if (!mod || !mod->active)
        return 0.0f;

    /* Get current bit value */
    uint8_t bit = (mod->bit_buffer[mod->bit_index / 8] >> (7 - (mod->bit_index % 8))) & 0x01;

    /* Get Barker chip value for current chip (multiply bit by +1/-1 chip) */
    int8_t chip = whisper_barker[mod->chip_index];
    float amplitude_mod = bit ? (chip > 0 ? 1.0f : 0.3f) : (chip < 0 ? 1.0f : 0.3f);

    /* Get hop frequency */
    float freq = whisper_hop_freqs[mod->hop_index];

    /* Generate sample */
    float sample = mod->amplitude * amplitude_mod * sinf(mod->phase);

    /* Advance DDS phase for current frequency */
    float delta = 2.0f * (float)M_PI * freq / (float)mod->fs_tx;
    mod->phase += delta;
    if (mod->phase >= 2.0f * (float)M_PI)
        mod->phase -= 2.0f * (float)M_PI;

    /* Advance through chips and hops */
    mod->sample_index++;
    if (mod->sample_index >= mod->samples_per_chip) {
        mod->sample_index = 0;
        mod->chip_index++;
        if (mod->chip_index >= WHISPER_BARKER_CHIPS) {
            mod->chip_index = 0;
            mod->hop_index++;
            if (mod->hop_index >= WHISPER_HOP_SEQUENCE_LEN) {
                mod->hop_index = 0;
                mod->bit_index++;
                if (mod->bit_index >= mod->bit_count) {
                    mod->active = 0;
                }
            }
        }
    }

    return sample;
}

int whisper_mod_is_done(const whisper_modulator_t *mod)
{
    return mod ? !mod->active : 1;
}

/* =========================================================================
 * CRC-16-IBM
 * ========================================================================= */

uint16_t crc16_ibm(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0x0000; /* CRC-16-IBM init value */

    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ CRC16_POLY;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

int crc16_ibm_verify(const uint8_t *data, uint16_t len)
{
    if (!data || len < 2)
        return ERR_INVALID_PARAM;

    uint16_t expected_crc = crc16_ibm(data, len - 2);
    uint16_t actual_crc = (uint16_t)data[len - 2] | ((uint16_t)data[len - 1] << 8);

    return (expected_crc == actual_crc) ? ERR_OK : ERR_CRC;
}

uint16_t crc16_ibm_append(uint8_t *data, uint16_t len)
{
    uint16_t crc = crc16_ibm(data, len);
    data[len]     = crc & 0xFF;
    data[len + 1] = (crc >> 8) & 0xFF;
    return len + 2;
}

/* =========================================================================
 * Frame Builder
 * ========================================================================= */

int frame_build_bits(const uint8_t *payload, uint16_t payload_len,
                     uint8_t *bit_buf, uint16_t bit_buf_size,
                     uint16_t *bit_count)
{
    if (!payload || !bit_buf || !bit_count || payload_len > MAX_MESSAGE_LEN)
        return ERR_INVALID_PARAM;

    uint16_t bp = 0; /* Bit position */

    /* Clear buffer */
    memset(bit_buf, 0, bit_buf_size);

    /* Preamble: 16 bits of alternating 1-0 for carrier detection */
    for (int i = 0; i < 16; i++) {
        if (bp >= bit_buf_size * 8) return ERR_BUF_FULL;
        bit_buf[bp / 8] |= ((i % 2 == 0) ? 1 : 0) << (7 - (bp % 8));
        bp++;
    }

    /* Sync: 4-bit Barker code, raw */
    for (int b = 3; b >= 0; b--) {
        if (bp >= bit_buf_size * 8) return ERR_BUF_FULL;
        uint8_t bit = (BARKER_CODE >> b) & 0x01;
        bit_buf[bp / 8] |= (bit << (7 - (bp % 8)));
        bp++;
    }

    /* Length byte */
    for (int b = 7; b >= 0; b--) {
        if (bp >= bit_buf_size * 8) return ERR_BUF_FULL;
        uint8_t bit = (payload_len >> b) & 0x01;
        bit_buf[bp / 8] |= (bit << (7 - (bp % 8)));
        bp++;
    }

    /* Payload bytes */
    for (uint16_t i = 0; i < payload_len; i++) {
        for (int b = 7; b >= 0; b--) {
            if (bp >= bit_buf_size * 8) return ERR_BUF_FULL;
            uint8_t bit = (payload[i] >> b) & 0x01;
            bit_buf[bp / 8] |= (bit << (7 - (bp % 8)));
            bp++;
        }
    }

    /* CRC-16 appended as 2 bytes */
    uint16_t crc = crc16_ibm(payload, payload_len);
    for (int b = 7; b >= 0; b--) {
        if (bp >= bit_buf_size * 8) return ERR_BUF_FULL;
        uint8_t bit = ((uint8_t)(crc & 0xFF) >> b) & 0x01;
        bit_buf[bp / 8] |= (bit << (7 - (bp % 8)));
        bp++;
    }
    for (int b = 7; b >= 0; b--) {
        if (bp >= bit_buf_size * 8) return ERR_BUF_FULL;
        uint8_t bit = ((uint8_t)(crc >> 8) >> b) & 0x01;
        bit_buf[bp / 8] |= (bit << (7 - (bp % 8)));
        bp++;
    }

    *bit_count = bp;
    return ERR_OK;
}