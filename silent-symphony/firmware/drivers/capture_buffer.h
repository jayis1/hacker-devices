/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Capture Buffer Manager — QSPI Flash + PSRAM
 *
 * Manages the 16 MB QSPI flash and 8 MB PSRAM for
 * storing captured ultrasonic audio and demodulated data.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef CAPTURE_BUFFER_H
#define CAPTURE_BUFFER_H

#include <stdint.h>
#include "../board.h"

/* =========================================================================
 * Capture Storage Layout
 * =========================================================================
 *
 * PSRAM (8 MB, FMC Bank3 @ 0x68000000):
 *   - Used as a fast circular buffer for live I²S capture
 *   - 8 MB = ~21 seconds of 192 kHz 16-bit mono
 *   - Double-buffered: CPU processes one half while DMA fills other
 *
 * QSPI Flash (16 MB, QuadSPI @ 0x90000000):
 *   - Persistent storage for captured data and configuration
 *   - Partition layout:
 *     | 0x000000 - 0x000FFF |  4 KB  | Bootloader config / magic
 *     | 0x001000 - 0x00FFFF | 60 KB  | Device configuration
 *     | 0x010000 - 0x01FFFF | 64 KB  | Capture metadata table
 *     | 0x020000 - 0xFFFFFF | ~15 MB | Capture data blocks
 * ========================================================================= */

#define CAPTURE_PARTITION_BOOTCFG   0x000000UL
#define CAPTURE_PARTITION_CONFIG    0x001000UL
#define CAPTURE_PARTITION_METADATA  0x010000UL
#define CAPTURE_PARTITION_DATA      0x020000UL
#define CAPTURE_PARTITION_DATA_SIZE (QSPI_FLASH_SIZE - CAPTURE_PARTITION_DATA)

/* Capture metadata entry (32 bytes per slot) */
#define CAPTURE_METADATA_ENTRY_SIZE  32
#define CAPTURE_MAX_SLOTS            MAX_CAPTURE_SLOTS

typedef struct __attribute__((packed)) {
    uint32_t timestamp;              /* Unix timestamp (or seconds since boot) */
    uint32_t offset;                 /* Byte offset in data partition */
    uint32_t length;                 /* Data length in bytes */
    uint16_t sample_rate;            /* Sample rate (Hz) */
    uint8_t  bit_depth;              /* Bits per sample (16) */
    uint8_t  modulation_type;        /* 0=raw, 1=FSK, 2=OOK, 3=whisper */
    uint8_t  power_level;            /* Tx power or Rx gain at capture time */
    uint8_t  signal_quality;         /* 0-100 */
    uint8_t  reserved[16];           /* Future use */
} capture_metadata_t;

/* =========================================================================
 * Ring Buffer for Live Capture (PSRAM)
 * ========================================================================= */

#define PSRAM_CAPTURE_BASE      ((int16_t *)FMC_BANK3_BASE)
#define PSRAM_CAPTURE_SIZE      (PSRAM_SIZE / 2)  /* 4 million 16-bit samples */
/* 4M samples @ 192 kHz = ~20.8 seconds */

typedef struct {
    volatile uint32_t write_index;   /* DMA write position (incremented by interrupt) */
    volatile uint32_t read_index;    /* Application read position */
    volatile uint32_t bytes_filled;  /* Total bytes written this session */
    volatile uint8_t  overflow;      /* 1 if buffer wrapped */
    volatile uint8_t  active;        /* 1 if capture in progress */
} capture_ring_t;

extern capture_ring_t g_capture_ring;

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * Initialize QSPI flash.
 * Sets up quad SPI in memory-mapped mode for fast reads,
 * and indirect mode for writes (page program).
 * 
 * @return 0 on success
 */
int qspi_init(void);

/**
 * Read a block from QSPI flash (memory-mapped).
 * 
 * @param addr   Byte address (absolute, in QSPI address space)
 * @param data   Output buffer
 * @param len    Number of bytes to read
 */
void qspi_read(uint32_t addr, uint8_t *data, uint32_t len);

/**
 * Write a block to QSPI flash (page program, 1-256 bytes).
 * Target sector must be erased first.
 * 
 * @param addr  Byte address (absolute)
 * @param data  Data to write
 * @param len   Length (1-256)
 * @return      0 on success, negative on error
 */
int qspi_write(uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * Erase a sector (4 KB) in QSPI flash.
 * 
 * @param addr  Any address within the sector
 * @return      0 on success
 */
int qspi_erase_sector(uint32_t addr);

/**
 * Erase a 64 KB block in QSPI flash.
 * 
 * @param addr  Any address within the block
 * @return      0 on success
 */
int qspi_erase_block_64k(uint32_t addr);

/**
 * Erase the entire QSPI flash chip.
 * 
 * @return 0 on success
 */
int qspi_erase_chip(void);

/**
 * Initialize PSRAM via FMC.
 * 
 * @return 0 on success
 */
int psram_init(void);

/**
 * Start a live capture session to PSRAM.
 */
void capture_ring_start(void);

/**
 * Stop capture session.
 */
void capture_ring_stop(void);

/**
 * Get current capture ring state.
 * 
 * @return  Number of bytes captured so far
 */
uint32_t capture_ring_get_size(void);

/**
 * Save current PSRAM capture to QSPI flash as a permanent capture.
 * 
 * @param mod_type   Modulation type
 * @param quality    Signal quality (0-100)
 * @return           Capture slot index (0-based), or negative on error
 */
int capture_save_to_flash(uint8_t mod_type, uint8_t quality);

/**
 * Load a capture from QSPI flash.
 * 
 * @param slot       Capture slot index
 * @param out_data   Output buffer (must be large enough)
 * @param out_len    Output: actual data length
 * @return           0 on success, negative on error
 */
int capture_load_from_flash(uint8_t slot, uint8_t *out_data, uint32_t *out_len);

/**
 * Get metadata for a capture slot.
 * 
 * @param slot  Slot index
 * @param meta  Output: metadata structure
 * @return      0 on success
 */
int capture_get_metadata(uint8_t slot, capture_metadata_t *meta);

/**
 * Delete a capture slot (erase its data blocks + clear metadata).
 * 
 * @param slot  Slot index
 * @return      0 on success
 */
int capture_delete(uint8_t slot);

/**
 * Get number of used capture slots.
 * 
 * @return Number of slots with valid data
 */
uint8_t capture_get_slot_count(void);

#endif /* CAPTURE_BUFFER_H */