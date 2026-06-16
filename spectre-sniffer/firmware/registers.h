//==============================================================================
// registers.h — Spectre-Sniffer FPGA Register Definitions and Bitfields
// Author: jayis1
// Description: Low-level register access macros, bitfield definitions, and
//              inline accessor functions for the FPGA control interface.
//==============================================================================

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

//==============================================================================
// SPI Register Access Primitives
//==============================================================================

// Maximum retries for register operations
#define REG_MAX_RETRIES         5
#define REG_TIMEOUT_US          1000

// SPI transaction flags
#define REG_FLAG_READ           (1 << 7)    // Read operation flag
#define REG_FLAG_WRITE          (0 << 7)    // Write operation flag
#define REG_FLAG_BURST          (1 << 6)    // Burst mode flag

//==============================================================================
// Register Access Functions (implemented in fpga.c)
//==============================================================================

/**
 * @brief Read an 8-bit FPGA register via SPI.
 * @param reg Register address (0x00–0x0F)
 * @param value Pointer to store read value
 * @return SPECTRE_OK on success, error code on failure
 */
extern int fpga_reg_read(uint8_t reg, uint8_t *value);

/**
 * @brief Write an 8-bit FPGA register via SPI.
 * @param reg Register address (0x00–0x0F)
 * @param value Value to write
 * @return SPECTRE_OK on success, error code on failure
 */
extern int fpga_reg_write(uint8_t reg, uint8_t value);

/**
 * @brief Read a 16-bit FPGA register (two consecutive 8-bit reads).
 * @param reg Base register address
 * @param value Pointer to store 16-bit value
 * @return SPECTRE_OK on success, error code on failure
 */
extern int fpga_reg_read16(uint8_t reg, uint16_t *value);

/**
 * @brief Write a 16-bit FPGA register (two consecutive 8-bit writes).
 * @param reg Base register address
 * @param value 16-bit value to write
 * @return SPECTRE_OK on success, error code on failure
 */
extern int fpga_reg_write16(uint8_t reg, uint16_t value);

/**
 * @brief Read a 32-bit FPGA register (four consecutive 8-bit reads).
 * @param reg Base register address
 * @param value Pointer to store 32-bit value
 * @return SPECTRE_OK on success, error code on failure
 */
extern int fpga_reg_read32(uint8_t reg, uint32_t *value);

/**
 * @brief Write a 32-bit FPGA register (four consecutive 8-bit writes).
 * @param reg Base register address
 * @param value 32-bit value to write
 * @return SPECTRE_OK on success, error code on failure
 */
extern int fpga_reg_write32(uint8_t reg, uint32_t value);

/**
 * @brief Modify specific bits in an FPGA register (read-modify-write).
 * @param reg Register address
 * @param mask Bitmask of bits to modify
 * @param value New value for masked bits
 * @return SPECTRE_OK on success, error code on failure
 */
static inline int fpga_reg_modify(uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t current;
    int ret = fpga_reg_read(reg, &current);
    if (ret != SPECTRE_OK) return ret;
    current = (current & ~mask) | (value & mask);
    return fpga_reg_write(reg, current);
}

//==============================================================================
// ADC Register Helpers
//==============================================================================

/**
 * @brief Enable or disable the ADC capture engine.
 * @param enable true to enable, false to disable
 * @return SPECTRE_OK on success
 */
static inline int adc_set_enable(bool enable) {
    return fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_ADC_ENABLE,
                           enable ? FPGA_CTRL_ADC_ENABLE : 0);
}

/**
 * @brief Check if ADC PLL is locked.
 * @return true if locked, false otherwise
 */
static inline bool adc_is_locked(void) {
    uint8_t status;
    if (fpga_reg_read(FPGA_REG_STATUS, &status) != SPECTRE_OK) return false;
    return (status & FPGA_STAT_ADC_LOCKED) != 0;
}

/**
 * @brief Check if ADC overflow has occurred.
 * @return true if overflow detected
 */
static inline bool adc_overflow_detected(void) {
    uint8_t status;
    if (fpga_reg_read(FPGA_REG_STATUS, &status) != SPECTRE_OK) return false;
    return (status & FPGA_STAT_ADC_OVERFLOW) != 0;
}

/**
 * @brief Set the decimation factor for the FIR filter.
 * @param factor Decimation factor (1–256)
 * @return SPECTRE_OK on success
 */
static inline int adc_set_decimation(uint16_t factor) {
    if (factor < MIN_DECIMATION_FACTOR || factor > MAX_DECIMATION_FACTOR) {
        return SPECTRE_ERR_INVALID_PARAM;
    }
    return fpga_reg_write(FPGA_REG_DECIMATION, (uint8_t)factor);
}

//==============================================================================
// FFT Engine Helpers
//==============================================================================

/**
 * @brief Enable or disable the FFT engine.
 * @param enable true to enable, false to disable
 * @return SPECTRE_OK on success
 */
static inline int fft_set_enable(bool enable) {
    return fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_FFT_ENABLE,
                           enable ? FPGA_CTRL_FFT_ENABLE : 0);
}

/**
 * @brief Check if FFT computation is complete.
 * @return true if FFT done
 */
static inline bool fft_is_done(void) {
    uint8_t status;
    if (fpga_reg_read(FPGA_REG_STATUS, &status) != SPECTRE_OK) return false;
    return (status & FPGA_STAT_FFT_DONE) != 0;
}

/**
 * @brief Set the FFT window type.
 * @param window Window type (FFT_WINDOW_HANNING, _BLACKMAN, _FLATTOP)
 * @return SPECTRE_OK on success
 */
static inline int fft_set_window(uint8_t window) {
    if (window > FFT_WINDOW_FLATTOP) return SPECTRE_ERR_INVALID_PARAM;
    return fpga_reg_modify(FPGA_REG_FFT_CTRL, 0x03, window);
}

/**
 * @brief Read FFT bin data from FPGA.
 * @param bin_index Starting bin index (0–511)
 * @param data Pointer to store 16-bit magnitude values (max 64 bins per read)
 * @param num_bins Number of bins to read (max 64)
 * @return SPECTRE_OK on success
 */
extern int fft_read_bins(uint16_t bin_index, uint16_t *data, uint16_t num_bins);

//==============================================================================
// Trigger Unit Helpers
//==============================================================================

/**
 * @brief Enable or disable the trigger unit.
 * @param enable true to enable, false to disable
 * @return SPECTRE_OK on success
 */
static inline int trigger_set_enable(bool enable) {
    return fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_TRIG_ENABLE,
                           enable ? FPGA_CTRL_TRIG_ENABLE : 0);
}

/**
 * @brief Check if trigger event occurred.
 * @return true if triggered
 */
static inline bool trigger_has_fired(void) {
    uint8_t status;
    if (fpga_reg_read(FPGA_REG_STATUS, &status) != SPECTRE_OK) return false;
    return (status & FPGA_STAT_TRIGGERED) != 0;
}

/**
 * @brief Clear the trigger flag.
 * @return SPECTRE_OK on success
 */
static inline int trigger_clear(void) {
    return fpga_reg_modify(FPGA_REG_STATUS, FPGA_STAT_TRIGGERED, 0);
}

/**
 * @brief Set the trigger level (signed 16-bit value).
 * @param level Trigger threshold in ADC codes
 * @return SPECTRE_OK on success
 */
static inline int trigger_set_level(int16_t level) {
    return fpga_reg_write16(FPGA_REG_TRIG_LEVEL, (uint16_t)level);
}

/**
 * @brief Set trigger mode.
 * @param rising_edge true for rising edge, false for falling edge
 * @return SPECTRE_OK on success
 */
static inline int trigger_set_edge(bool rising_edge) {
    return fpga_reg_modify(FPGA_REG_TRIG_CTRL, 0x01, rising_edge ? 0x01 : 0x00);
}

//==============================================================================
// Capture Buffer Helpers
//==============================================================================

/**
 * @brief Start a single-shot capture.
 * @return SPECTRE_OK on success
 */
static inline int capture_start_single(void) {
    return fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_SINGLE_CAPTURE,
                           FPGA_CTRL_SINGLE_CAPTURE);
}

/**
 * @brief Start continuous capture mode.
 * @return SPECTRE_OK on success
 */
static inline int capture_start_continuous(void) {
    return fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_CONT_CAPTURE,
                           FPGA_CTRL_CONT_CAPTURE);
}

/**
 * @brief Stop all capture operations.
 * @return SPECTRE_OK on success
 */
static inline int capture_stop(void) {
    uint8_t mask = FPGA_CTRL_CONT_CAPTURE | FPGA_CTRL_SINGLE_CAPTURE;
    return fpga_reg_modify(FPGA_REG_CTRL, mask, 0);
}

/**
 * @brief Check if capture buffer is full.
 * @return true if buffer full
 */
static inline bool capture_buffer_full(void) {
    uint8_t status;
    if (fpga_reg_read(FPGA_REG_STATUS, &status) != SPECTRE_OK) return false;
    return (status & FPGA_STAT_BUFFER_FULL) != 0;
}

/**
 * @brief Check if capture buffer is half-full.
 * @return true if buffer half-full
 */
static inline bool capture_buffer_half(void) {
    uint8_t status;
    if (fpga_reg_read(FPGA_REG_STATUS, &status) != SPECTRE_OK) return false;
    return (status & FPGA_STAT_BUFFER_HALF) != 0;
}

/**
 * @brief Read captured samples from FPGA buffer.
 * @param buffer Destination buffer (16-bit samples)
 * @param num_samples Number of samples to read
 * @return Number of samples actually read, or negative error
 */
extern int capture_read_samples(int16_t *buffer, uint32_t num_samples);

/**
 * @brief Get total sample count since last reset.
 * @param count Pointer to store 64-bit sample count
 * @return SPECTRE_OK on success
 */
static inline int capture_get_sample_count(uint64_t *count) {
    uint32_t low, high;
    int ret = fpga_reg_read32(FPGA_REG_SAMPLE_COUNT_L, &low);
    if (ret != SPECTRE_OK) return ret;
    ret = fpga_reg_read32(FPGA_REG_SAMPLE_COUNT_H, &high);
    if (ret != SPECTRE_OK) return ret;
    *count = ((uint64_t)high << 32) | low;
    return SPECTRE_OK;
}

//==============================================================================
// FPGA Configuration Helpers
//==============================================================================

/**
 * @brief Reset the FPGA (soft reset).
 * @return SPECTRE_OK on success
 */
static inline int fpga_soft_reset(void) {
    int ret = fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_RESET, FPGA_CTRL_RESET);
    if (ret != SPECTRE_OK) return ret;
    // Hold reset for a brief period
    for (volatile int i = 0; i < 100; i++);
    return fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_RESET, 0);
}

/**
 * @brief Read FPGA firmware version.
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 * @return SPECTRE_OK on success
 */
static inline int fpga_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch) {
    uint8_t version;
    int ret = fpga_reg_read(FPGA_REG_FW_VERSION, &version);
    if (ret != SPECTRE_OK) return ret;
    *major = (version >> 4) & 0x0F;
    *minor = (version >> 2) & 0x03;
    *patch = version & 0x03;
    return SPECTRE_OK;
}

/**
 * @brief Read FPGA die temperature (approximate).
 * @param temp_celsius Pointer to store temperature in Celsius
 * @return SPECTRE_OK on success
 */
static inline int fpga_get_temperature(float *temp_celsius) {
    uint8_t raw;
    int ret = fpga_reg_read(FPGA_REG_TEMP_SENSOR, &raw);
    if (ret != SPECTRE_OK) return ret;
    // Temperature sensor: 0x00 = 0°C, 0xFF = 100°C (approximate)
    *temp_celsius = (float)raw * 100.0f / 255.0f;
    return SPECTRE_OK;
}

//==============================================================================
// RF Frontend Control Helpers
//==============================================================================

/**
 * @brief Set the tunable filter center frequency.
 * @param freq_hz Center frequency in Hz
 * @return SPECTRE_OK on success
 */
extern int rf_set_filter_freq(uint64_t freq_hz);

/**
 * @brief Set the downconverter LO frequency.
 * @param freq_hz LO frequency in Hz
 * @return SPECTRE_OK on success
 */
extern int rf_set_lo_freq(uint64_t freq_hz);

/**
 * @brief Enable or disable the LNA.
 * @param channel LNA channel (0 or 1)
 * @param enable true to enable
 * @return SPECTRE_OK on success
 */
extern int rf_set_lna_enable(uint8_t channel, bool enable);

/**
 * @brief Set LNA gain.
 * @param gain_db Gain in dB (10 or 20)
 * @return SPECTRE_OK on success
 */
extern int rf_set_lna_gain(uint8_t gain_db);

//==============================================================================
// Debug and Diagnostics
//==============================================================================

/**
 * @brief Enable FPGA loopback mode for self-test.
 * @param enable true to enable loopback
 * @return SPECTRE_OK on success
 */
static inline int fpga_set_loopback(bool enable) {
    return fpga_reg_modify(FPGA_REG_CTRL, FPGA_CTRL_LOOPBACK,
                           enable ? FPGA_CTRL_LOOPBACK : 0);
}

/**
 * @brief Run FPGA built-in self-test (BIST).
 * @return SPECTRE_OK if BIST passes, error code otherwise
 */
extern int fpga_run_bist(void);

/**
 * @brief Read FPGA debug register.
 * @param value Pointer to store debug value
 * @return SPECTRE_OK on success
 */
static inline int fpga_read_debug(uint8_t *value) {
    return fpga_reg_read(FPGA_REG_DEBUG, value);
}

/**
 * @brief Write FPGA debug register.
 * @param value Debug value to write
 * @return SPECTRE_OK on success
 */
static inline int fpga_write_debug(uint8_t value) {
    return fpga_reg_write(FPGA_REG_DEBUG, value);
}

#endif // REGISTERS_H
