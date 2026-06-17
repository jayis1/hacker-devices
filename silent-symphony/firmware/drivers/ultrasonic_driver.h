/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Ultrasonic Tx/Rx Driver — API Header
 *
 * Provides FSK, OOK, and whisper-mode modulation/demodulation
 * for the ultrasonic transducers.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ULTRASONIC_DRIVER_H
#define ULTRASONIC_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include "../board.h"

/* =========================================================================
 * Goertzel Algorithm — Tone Energy Detector
 * ========================================================================= */

/**
 * GoertzelState — persistent filter state for one frequency bin
 * 
 * The Goertzel algorithm computes the energy at a specific frequency
 * using a 2-pole IIR filter. It is far more efficient than FFT when
 * only a few frequency bins are needed.
 */
typedef struct {
    float s_prev;       /* Q1[n-1] — first delayed sample */
    float s_prev2;      /* Q2[n-2] — second delayed sample */
    float coeff;        /* 2 * cos(2*pi*k/N) — precomputed coefficient */
    uint16_t n;         /* Block size */
    uint16_t count;     /* Samples processed so far */
} goertzel_state_t;

/**
 * Initialize a Goertzel state for detecting a target frequency.
 * 
 * @param state    Uninitialized Goertzel state
 * @param freq     Target frequency in Hz (e.g., 21000 for 21.0 kHz)
 * @param n        Analysis block size (samples)
 * @param fs       Sample rate in Hz (e.g., 192000)
 */
void goertzel_init(goertzel_state_t *state, float freq, uint16_t n, float fs);

/**
 * Feed one sample into the Goertzel filter.
 * 
 * @param state   Goertzel filter state
 * @param sample  Input sample (float, -1.0 to 1.0)
 */
void goertzel_process(goertzel_state_t *state, float sample);

/**
 * Get the energy at the target frequency from the current block.
 * Call this after processing N samples via goertzel_process().
 * 
 * @param  state  Goertzel filter state (will be reset for next block)
 * @return        Energy magnitude (arbitrary units, compare between bins)
 */
float goertzel_get_energy(goertzel_state_t *state);

/**
 * Process a full buffer of samples for two frequencies and return
 * the energy ratio (mark_energy / space_energy).
 * 
 * @param  samples       Input buffer of float samples
 * @param  n_samples     Number of samples
 * @param  freq_mark     Mark frequency (Hz)
 * @param  freq_space    Space frequency (Hz)
 * @param  fs            Sample rate (Hz)
 * @param  out_mark_energy  Output: energy at mark frequency
 * @param  out_space_energy Output: energy at space frequency
 * @return               bit decision: 1=mark, 0=space, -1=uncertain
 */
int goertzel_bit_decision(const float *samples, uint16_t n_samples,
                          float freq_mark, float freq_space, float fs,
                          float *out_mark_energy, float *out_space_energy);

/* =========================================================================
 * FSK Modulator — Generate Ultrasonic Tones
 * ========================================================================= */

/**
 * FSK modulator state.
 */
typedef struct {
    float    freq_mark;             /* Frequency for '1' (Hz) */
    float    freq_space;            /* Frequency for '0' (Hz) */
    uint32_t baud;                  /* Symbols per second */
    uint32_t samples_per_bit;       /* Calculated: fs_tx / baud */
    float    amplitude;             /* Output amplitude (0.0 to 1.0) */
    float    phase;                 /* Current DDS phase accumulator */
    float    phase_delta_mark;      /* Phase increment per sample for mark freq */
    float    phase_delta_space;     /* Phase increment per sample for space freq */
    uint32_t fs_tx;                 /* Tx sample rate */
    uint8_t  bit_buffer[256];       /* Bits to transmit */
    uint16_t bit_count;             /* Number of bits in buffer */
    uint16_t bit_index;             /* Current bit position */
    uint32_t sample_index;          /* Sample within current bit */
    uint8_t  active;                /* Non-zero if modulation is running */
} fsk_modulator_t;

/**
 * Initialize FSK modulator.
 *
 * @param mod     Uninitialized FSK modulator
 * @param f_mark  Mark frequency (Hz)
 * @param f_space Space frequency (Hz)
 * @param baud    Baud rate (symbols/second)
 * @param amp     Output amplitude (0.0–1.0)
 * @param fs      Tx sample rate (Hz)
 */
void fsk_mod_init(fsk_modulator_t *mod, float f_mark, float f_space,
                  uint32_t baud, float amp, uint32_t fs);

/**
 * Load a message for FSK transmission.
 * Converts bytes to bits (MSB first, then Manchester encodes).
 *
 * @param  mod     FSK modulator state
 * @param  data    Message bytes
 * @param  len     Message length (bytes, max 64)
 * @return         0 on success, negative on error
 */
int fsk_mod_load_message(fsk_modulator_t *mod, const uint8_t *data, uint16_t len);

/**
 * Generate the next sample for FSK transmission.
 * Call this at the Tx sample rate to fill output buffers.
 *
 * @param  mod  FSK modulator state
 * @return      Audio sample (float, -1.0 to 1.0). Returns 0.0 if idle.
 */
float fsk_mod_tick(fsk_modulator_t *mod);

/**
 * Check if FSK transmission is complete.
 *
 * @param  mod  FSK modulator state
 * @return      1 if done, 0 if still transmitting
 */
int fsk_mod_is_done(const fsk_modulator_t *mod);

/* =========================================================================
 * FSK Demodulator — Decode Ultrasonic Tones
 * ========================================================================= */

/**
 * FSK demodulator state.
 */
typedef struct {
    float    freq_mark;             /* Mark frequency (Hz) */
    float    freq_space;            /* Space frequency (Hz) */
    uint32_t baud;                  /* Expected baud rate */
    uint32_t samples_per_bit;       /* Samples per bit at current rate */
    float    fs_rx;                 /* Rx sample rate */
    
    /* Goertzel-based tone detection */
    goertzel_state_t goertzel_mark;
    goertzel_state_t goertzel_space;
    float    snr_estimate;          /* Current estimated SNR (dB) */
    
    /* Manchester decoder state */
    uint8_t  prev_bit;              /* Previous bit decision */
    uint8_t  manchester_state;      /* 0=waiting for half-bit, 1=have half-bit */
    uint8_t  manchester_half;       /* Stored half-bit */
    
    /* Frame synchronization */
    uint32_t preamble_count;        /* Consecutive 22 kHz preamble detections */
    uint8_t  synced;                /* 1 if frame sync acquired */
    uint16_t sync_shift_reg;        /* Barker correlator shift register */
    
    /* Output */
    uint8_t  rx_buffer[128];        /* Received data buffer */
    uint16_t rx_byte_index;         /* Current byte being assembled */
    uint8_t  rx_bit_pos;            /* Bit position in current byte */
    uint16_t rx_frame_length;       /* Expected frame length (bytes) */
    uint16_t rx_bytes_received;     /* Bytes received so far */
    uint8_t  frame_ready;           /* 1 when complete frame is ready */
} fsk_demodulator_t;

/**
 * Initialize FSK demodulator.
 *
 * @param dem      Uninitialized FSK demodulator
 * @param f_mark   Mark frequency (Hz)
 * @param f_space  Space frequency (Hz)
 * @param baud     Expected baud rate (symbols/second)
 * @param fs       Rx sample rate (Hz)
 */
void fsk_demod_init(fsk_demodulator_t *dem, float f_mark, float f_space,
                    uint32_t baud, float fs);

/**
 * Process one sample from the I²S Rx buffer.
 * 
 * @param dem     FSK demodulator state
 * @param sample  Input sample (float, -1.0 to 1.0)
 */
void fsk_demod_process(fsk_demodulator_t *dem, float sample);

/**
 * Check if a complete frame has been received.
 *
 * @param  dem  FSK demodulator state
 * @return      1 if frame ready, 0 otherwise
 */
int fsk_demod_frame_ready(const fsk_demodulator_t *dem);

/**
 * Get the received frame data.
 * Caller must copy data before calling fsk_demod_clear_frame().
 *
 * @param  dem       FSK demodulator state
 * @param  out_data  Output buffer (up to 128 bytes)
 * @param  out_len   Output: received data length
 * @return           0 on success, negative on error
 */
int fsk_demod_get_frame(fsk_demodulator_t *dem, uint8_t *out_data, uint16_t *out_len);

/**
 * Clear the received frame flag (call after retrieving data).
 *
 * @param dem  FSK demodulator state
 */
void fsk_demod_clear_frame(fsk_demodulator_t *dem);

/**
 * Get the current SNR estimate.
 *
 * @param  dem  FSK demodulator state
 * @return      SNR estimate in dB, negative if unreliable
 */
float fsk_demod_get_snr(const fsk_demodulator_t *dem);

/**
 * Adjust baud rate based on measured timing.
 * Implements adaptive rate fallback.
 *
 * @param dem    FSK demodulator state
 * @param new_baud New baud rate
 */
void fsk_demod_set_baud(fsk_demodulator_t *dem, uint32_t new_baud);

/* =========================================================================
 * OOK Modulator — On-Off Keying
 * ========================================================================= */

typedef struct {
    float    carrier_freq;          /* Carrier frequency (Hz) */
    uint32_t bit_duration_us;       /* Time per bit (microseconds) */
    float    amplitude;             /* Carrier amplitude */
    float    phase;                 /* DDS phase */
    float    phase_delta;           /* Phase increment per sample */
    uint32_t fs_tx;                 /* Tx sample rate */
    uint8_t  bit_buffer[256];       /* Bits to transmit */
    uint16_t bit_count;
    uint16_t bit_index;
    uint32_t samples_per_bit;
    uint32_t sample_index;
    uint8_t  active;
} ook_modulator_t;

void ook_mod_init(ook_modulator_t *mod, float carrier_freq,
                  uint32_t bit_duration_us, float amp, uint32_t fs);
int  ook_mod_load_message(ook_modulator_t *mod, const uint8_t *data, uint16_t len);
float ook_mod_tick(ook_modulator_t *mod);
int  ook_mod_is_done(const ook_modulator_t *mod);

/* =========================================================================
 * OOK Demodulator
 * ========================================================================= */

typedef struct {
    float    carrier_freq;
    uint32_t bit_duration_us;
    float    fs_rx;
    
    /* Envelope detection */
    float    envelope;              /* Current envelope estimate */
    float    noise_floor;           /* Adaptive noise floor */
    float    threshold;             /* Decision threshold */
    uint32_t env_avg_count;         /* Samples in moving average */
    
    /* Bit timing */
    uint32_t sample_count;
    uint32_t samples_per_half_bit;
    uint8_t  bit_value;
    uint8_t  bit_ready;
    
    /* Frame sync */
    uint8_t  synced;
    uint32_t sync_timer;
    
    /* Output */
    uint8_t  rx_buffer[128];
    uint16_t rx_byte_index;
    uint8_t  rx_bit_pos;
    uint8_t  frame_ready;
} ook_demodulator_t;

void ook_demod_init(ook_demodulator_t *dem, float carrier_freq,
                    uint32_t bit_duration_us, float fs);
void ook_demod_process(ook_demodulator_t *dem, float sample);
int   ook_demod_frame_ready(const ook_demodulator_t *dem);
int   ook_demod_get_frame(ook_demodulator_t *dem, uint8_t *out_data, uint16_t *out_len);
void  ook_demod_clear_frame(ook_demodulator_t *dem);

/* =========================================================================
 * Whisper Mode — Spread Spectrum (DSSS-like)
 * ========================================================================= */

/**
 * Whisper mode spreads each data bit across 19–23 kHz
 * using a pseudo-random frequency hopping pattern, modulated
 * with a 7-chip Barker sequence for processing gain.
 * 
 * This provides approximately 8.5 dB of SNR improvement
 * at the cost of drastically reduced data rate (1–5 bps).
 */

#define WHISPER_HOP_SEQUENCE_LEN  16
#define WHISPER_BARKER_CHIPS      7

/* 16-hop sequence covering 19–23 kHz */
static const float whisper_hop_freqs[WHISPER_HOP_SEQUENCE_LEN] = {
    19000.0f, 19500.0f, 20000.0f, 20500.0f,
    21000.0f, 21500.0f, 22000.0f, 22500.0f,
    19250.0f, 19750.0f, 20250.0f, 20750.0f,
    21250.0f, 21750.0f, 22250.0f, 22750.0f
};

/* 7-chip Barker sequence: +1 +1 +1 -1 -1 +1 -1 */
static const int8_t whisper_barker[WHISPER_BARKER_CHIPS] = {1, 1, 1, -1, -1, 1, -1};

typedef struct {
    uint8_t  active;
    float    fs_tx;
    float    amplitude;
    
    /* Hop state */
    uint8_t  hop_index;
    uint8_t  chip_index;
    uint32_t samples_per_chip;
    uint32_t sample_index;
    
    /* DDS */
    float    phase;
    
    /* Bit buffer */
    uint8_t  bit_buffer[256];
    uint16_t bit_count;
    uint16_t bit_index;
} whisper_modulator_t;

void whisper_mod_init(whisper_modulator_t *mod, float amp, float fs, float bps);
int  whisper_mod_load_message(whisper_modulator_t *mod, const uint8_t *data, uint16_t len);
float whisper_mod_tick(whisper_modulator_t *mod);
int  whisper_mod_is_done(const whisper_modulator_t *mod);

/* =========================================================================
 * Utility: CRC-16-IBM
 * ========================================================================= */

/**
 * Compute CRC-16-IBM over a data buffer.
 * 
 * @param  data  Input buffer
 * @param  len   Buffer length
 * @return       16-bit CRC
 */
uint16_t crc16_ibm(const uint8_t *data, uint16_t len);

/**
 * Verify CRC-16-IBM and return 0 on success.
 * 
 * @param  data   Buffer containing payload + 2-byte CRC (LE)
 * @param  len    Total length (including CRC)
 * @return        0 if CRC matches, negative on mismatch
 */
int crc16_ibm_verify(const uint8_t *data, uint16_t len);

/**
 * Append CRC-16-IBM to a buffer.
 * 
 * @param  data  Buffer with payload; CRC appended at data[len], data[len+1]
 * @param  len   Payload length
 * @return       len + 2
 */
uint16_t crc16_ibm_append(uint8_t *data, uint16_t len);

/* =========================================================================
 * Frame Builder — Assemble Protocol Messages
 * ========================================================================= */

/**
 * Build a complete transmission frame with preamble, sync,
 * length, payload, and CRC.
 * 
 * Frame format:
 *   [Preamble: 10 cycles at 22 kHz tone]
 *   [Sync: 4-bit Barker code (1010) encoded as 4 bits]
 *   [Length: 1 byte]
 *   [Payload: 0-64 bytes]
 *   [CRC-16: 2 bytes, LE]
 * 
 * The output *bits* are written to bit_buf (for modulator).
 * 
 * @param  payload   Payload bytes to send
 * @param  payload_len  Payload length (0–64)
 * @param  bit_buf   Output buffer for bits
 * @param  bit_buf_size  Size of bit buffer
 * @param  bit_count Output: number of bits written
 * @return           0 on success, negative on error
 */
int frame_build_bits(const uint8_t *payload, uint16_t payload_len,
                     uint8_t *bit_buf, uint16_t bit_buf_size,
                     uint16_t *bit_count);

#endif /* ULTRASONIC_DRIVER_H */