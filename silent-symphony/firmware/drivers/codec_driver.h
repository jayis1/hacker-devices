/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * WM8731S I²S Audio Codec Driver
 *
 * Initializes and controls the WM8731S stereo audio codec
 * via I²C control interface. Manages sample rate, power state,
 * and analog path routing.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef CODEC_DRIVER_H
#define CODEC_DRIVER_H

#include <stdint.h>
#include "../board.h"

/* =========================================================================
 * WM8731S I²C Address
 * ========================================================================= */
/* CSB pin low = 0x1A, CSB high = 0x1B. We hardwire low → 0x1A (7-bit). */
#define WM8731_I2C_ADDR         0x1A

/* =========================================================================
 * WM8731 Register Map (7-bit register addresses)
 * ========================================================================= */
#define WM8731_REG_LEFT_IN      0x00    /* Left line input volume */
#define WM8731_REG_RIGHT_IN     0x01    /* Right line input volume */
#define WM8731_REG_LEFT_HP      0x02    /* Left headphone volume */
#define WM8731_REG_RIGHT_HP     0x03    /* Right headphone volume */
#define WM8731_REG_ANALOG       0x04    /* Analog audio path control */
#define WM8731_REG_DIGITAL      0x05    /* Digital audio path control */
#define WM8731_REG_POWER        0x06    /* Power down control */
#define WM8731_REG_FORMAT       0x07    /* Digital audio interface format */
#define WM8731_REG_SAMPLING     0x08    /* Sampling control */
#define WM8731_REG_ACTIVE       0x09    /* Active control */
#define WM8731_REG_RESET        0x0F    /* Reset register */

/* =========================================================================
 * Register Bit Definitions
 * ========================================================================= */

/* WM8731_REG_LEFT_IN / RIGHT_IN (0x00, 0x01) */
#define WM8731_IN_MUTE          (1U << 7)    /* Mute */
#define WM8731_IN_VOL_MASK      0x3F          /* Volume: 0=0dB, 0x2F=+6dB, step=0.375dB */
#define WM8731_IN_VOL_DEFAULT   0x17          /* ~0 dB */

/* WM8731_REG_LEFT_HP / RIGHT_HP (0x02, 0x03) */
#define WM8731_HP_MUTE          (1U << 7)    /* Mute */
#define WM8731_HP_VOL_MASK      0x7F          /* Volume: 0x30=0dB, 0x7F=+6dB */
#define WM8731_HP_VOL_DEFAULT   0x30          /* 0 dB */
#define WM8731_HP_ZC            (1U << 6)    /* Zero-cross detection */

/* WM8731_REG_ANALOG (0x04) */
#define WM8731_ANALOG_BYPASS    (1U << 7)    /* Mic to HP bypass */
#define WM8731_ANALOG_DAC_SEL   (1U << 6)    /* DAC select */
#define WM8731_ANALOG_SIDETONE  (1U << 5)    /* Sidetone enable */
#define WM8731_ANALOG_MICMUTE   (1U << 1)    /* Microphone mute */
#define WM8731_ANALOG_MICBOOST  (1U << 0)    /* Microphone 20 dB boost */

/* WM8731_REG_DIGITAL (0x05) */
#define WM8731_DIGITAL_DEEMP    (3U << 1)    /* De-emphasis: 0=none, 1=48k, 2=44.1k, 3=32k */
#define WM8731_DIGITAL_SOFTMUTE (1U << 0)    /* Soft mute */
#define WM8731_DIGITAL_FILTER   (1U << 3)    /* ADC high-pass filter */

/* WM8731_REG_POWER (0x06) — each bit powers down a block */
#define WM8731_POWER_LINEIN     (1U << 0)    /* Line input */
#define WM8731_POWER_MIC        (1U << 1)    /* Microphone input */
#define WM8731_POWER_ADC        (1U << 2)    /* ADC */
#define WM8731_POWER_DAC        (1U << 3)    /* DAC */
#define WM8731_POWER_OUT        (1U << 4)    /* Output */
#define WM8731_POWER_OSC        (1U << 5)    /* Oscillator */
#define WM8731_POWER_CLKOUT     (1U << 6)    /* Clock output */
#define WM8731_POWER_ALL_ON     0x00          /* All blocks powered on */
#define WM8731_POWER_ALL_OFF    0x7F          /* Everything off */

/* WM8731_REG_FORMAT (0x07) */
#define WM8731_FORMAT_FORMAT_MASK  (3U << 2)      /* Format */
#define WM8731_FORMAT_I2S          (2U << 2)      /* I²S format (Philips) */
#define WM8731_FORMAT_MSB_FIRST    (0U << 2)      /* MSB first (left-justified) */
#define WM8731_FORMAT_LSB_FIRST    (1U << 2)      /* LSB first (right-justified) */
#define WM8731_FORMAT_PCM          (3U << 2)      /* DSP/PCM mode */
#define WM8731_FORMAT_LRSWAP     (1U << 5)    /* Left/right swap */
#define WM8731_FORMAT_IWL_16     (0U << 0)    /* 16-bit */
#define WM8731_FORMAT_IWL_20     (1U << 0)    /* 20-bit */
#define WM8731_FORMAT_IWL_24     (2U << 0)    /* 24-bit */
#define WM8731_FORMAT_IWL_32     (3U << 0)    /* 32-bit */
#define WM8731_FORMAT_MASTER     (1U << 6)    /* Master mode */
#define WM8731_FORMAT_LRP        (1U << 7)    /* LRCLK polarity (0=left low) */

/* WM8731_REG_SAMPLING (0x08) */
#define WM8731_SAMPLING_CLK_MASK     (3U << 0)   /* Clock divider */
#define WM8731_SAMPLING_CLK_384FS    (0U << 0)   /* MCLK = 384 * fs */
#define WM8731_SAMPLING_CLK_256FS    (1U << 0)   /* MCLK = 256 * fs */
#define WM8731_SAMPLING_CLK_128FS    (2U << 0)   /* MCLK = 128 * fs */
#define WM8731_SAMPLING_MCLKDIV2     (1U << 7)   /* Divide MCLK by 2 before use */
#define WM8731_SAMPLING_SR_MASK      (0x0FUL << 2) /* Sample rate select */
#define WM8731_SAMPLING_SR_48K       (0x0UL << 2)  /* 48 kHz */
#define WM8731_SAMPLING_SR_44K1      (0x1UL << 2)  /* 44.1 kHz */
#define WM8731_SAMPLING_SR_32K       (0x2UL << 2)  /* 32 kHz */
#define WM8731_SAMPLING_SR_96K       (0x7UL << 2)  /* 96 kHz (USB mode) */
#define WM8731_SAMPLING_USB          (1U << 6)    /* USB mode (250/272 fs instead of 384) */
#define WM8731_SAMPLING_BOSR         (1U << 1)    /* Base oversample rate */

/* WM8731_REG_ACTIVE (0x09) */
#define WM8731_ACTIVE_ENABLE    (1U << 0)    /* Activate interface */

/* =========================================================================
 * Default configuration: I²S 16-bit, 96 kHz, DAC+ADC active
 * ========================================================================= */
#define WM8731_CONFIG_INIT { \
    .regs[WM8731_REG_RESET]        = 0x000, \
    .regs[WM8731_REG_LEFT_IN]      = WM8731_IN_VOL_DEFAULT, \
    .regs[WM8731_REG_RIGHT_IN]     = WM8731_IN_VOL_DEFAULT, \
    .regs[WM8731_REG_LEFT_HP]      = WM8731_HP_VOL_DEFAULT | WM8731_HP_ZC, \
    .regs[WM8731_REG_RIGHT_HP]     = WM8731_HP_VOL_DEFAULT | WM8731_HP_ZC, \
    .regs[WM8731_REG_ANALOG]       = WM8731_ANALOG_DAC_SEL, \
    .regs[WM8731_REG_DIGITAL]      = WM8731_DIGITAL_FILTER, \
    .regs[WM8731_REG_POWER]        = WM8731_POWER_ALL_ON, \
    .regs[WM8731_REG_FORMAT]       = WM8731_FORMAT_I2S | WM8731_FORMAT_IWL_16, \
    .regs[WM8731_REG_SAMPLING]     = WM8731_SAMPLING_CLK_384FS | WM8731_SAMPLING_SR_48K, \
    .regs[WM8731_REG_ACTIVE]       = WM8731_ACTIVE_ENABLE, \
}

/* =========================================================================
 * Codec Driver State
 * ========================================================================= */

struct codec_config {
    uint16_t regs[16];   /* Shadow register map */
    uint8_t  initialized;
    uint32_t sample_rate;
};

/**
 * Initialize the WM8731S codec.
 * Configures GPIO for reset, initializes I²C4, sends register
 * configuration, and activates the codec.
 * 
 * @param config  Codec configuration (may be NULL for defaults)
 * @param sr      Sample rate (Hz): 8000, 16000, 32000, 44100, 48000, 96000
 * @return        0 on success, negative on error
 */
int codec_init(struct codec_config *config, uint32_t sr);

/**
 * Set codec sample rate.
 * Adjusts WM8731_SAMPLING register.
 * 
 * @param sr  New sample rate (Hz)
 * @return    0 on success, negative on error
 */
int codec_set_sample_rate(uint32_t sr);

/**
 * Set codec output volume.
 * 
 * @param left   Left channel volume (0–127, 0x30=0dB)
 * @param right  Right channel volume
 * @return       0 on success
 */
int codec_set_volume(uint8_t left, uint8_t right);

/**
 * Mute/unmute codec output.
 * 
 * @param mute  1 = mute, 0 = unmute
 * @return      0 on success
 */
int codec_set_mute(uint8_t mute);

/**
 * Power down codec.
 * 
 * @return 0 on success
 */
int codec_power_down(void);

/**
 * Power up codec from power-down state.
 * 
 * @return 0 on success
 */
int codec_power_up(void);

/**
 * Write a single register to WM8731 via I²C.
 * 
 * @param reg   Register address (7-bit)
 * @param val   Value (9-bit, bits 8:0)
 * @return      0 on success, negative on error
 */
int codec_reg_write(uint8_t reg, uint16_t val);

/**
 * I²C4 master write helper (2 bytes: reg_addr | val_hi | val_lo).
 * 
 * @param dev_addr  7-bit device address
 * @param data      Data to write (first byte = register, next = value MSB/LSB)
 * @param len       Number of bytes (must be 2 + val_width)
 * @return          0 on success, negative on error
 */
int i2c4_write(uint8_t dev_addr, const uint8_t *data, uint8_t len);

/**
 * Initialize I²C4 peripheral as master at 400 kHz.
 * 
 * @return 0 on success
 */
int i2c4_init(void);

/**
 * Initialize I²S1 (Tx) via SPI1 in I²S master mode.
 * 
 * @param sample_rate  Target sample rate (Hz)
 * @param bit_depth    Bit depth (16, 24, 32)
 * @param master_clock Enable master clock output
 * @return             0 on success
 */
int i2s_tx_init(uint32_t sample_rate, uint8_t bit_depth, uint8_t master_clock);

/**
 * Initialize I²S3 (Rx) via SPI3 in I²S slave mode.
 * Configured as slave so the Tx-side codec provides the clock.
 * 
 * @param sample_rate  Target sample rate (Hz)
 * @param bit_depth    Bit depth (16, 24, 32)
 * @return             0 on success
 */
int i2s_rx_init(uint32_t sample_rate, uint8_t bit_depth);

/**
 * Start I²S Tx DMA transfer (double-buffered, circular).
 * 
 * @param buf0      First buffer (must be I2S_TX_BUFFER_SIZE samples)
 * @param buf1      Second buffer (ping-pong)
 * @param n_samples Samples per buffer
 * @return          0 on success
 */
int i2s_tx_dma_start(int16_t *buf0, int16_t *buf1, uint16_t n_samples);

/**
 * Start I²S Rx DMA transfer (double-buffered, circular).
 * 
 * @param buf0      First buffer (must be I2S_RX_BUFFER_SIZE samples)
 * @param buf1      Second buffer (ping-pong)
 * @param n_samples Samples per buffer
 * @return          0 on success
 */
int i2s_rx_dma_start(int16_t *buf0, int16_t *buf1, uint16_t n_samples);

/**
 * Stop I²S Tx DMA transfer.
 */
void i2s_tx_dma_stop(void);

/**
 * Stop I²S Rx DMA transfer.
 */
void i2s_rx_dma_stop(void);

#endif /* CODEC_DRIVER_H */