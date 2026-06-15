/**
 * emmc.h — eMMC 5.1 Flash Driver Interface
 *
 * JESD84-B51 compliant eMMC driver for STM32H743 SDMMC2 peripheral.
 * Supports HS400 mode (200 MHz DDR, 8-bit bus) with DMA-based
 * block read for forensic acquisition.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#ifndef EMMC_H
#define EMMC_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * eMMC Command Definitions
 *===========================================================================*/

#define EMMC_CMD0_GO_IDLE_STATE        0
#define EMMC_CMD1_SEND_OP_COND         1
#define EMMC_CMD2_ALL_SEND_CID         2
#define EMMC_CMD3_SET_RELATIVE_ADDR    3
#define EMMC_CMD4_SET_DSR              4
#define EMMC_CMD5_SLEEP_AWAKE          5
#define EMMC_CMD6_SWITCH               6
#define EMMC_CMD7_SELECT_DESELECT      7
#define EMMC_CMD8_SEND_EXT_CSD         8
#define EMMC_CMD9_SEND_CSD             9
#define EMMC_CMD10_SEND_CID            10
#define EMMC_CMD12_STOP_TRANSMISSION   12
#define EMMC_CMD13_SEND_STATUS         13
#define EMMC_CMD15_GO_INACTIVE_STATE   15
#define EMMC_CMD16_SET_BLOCKLEN        16
#define EMMC_CMD17_READ_SINGLE_BLOCK   17
#define EMMC_CMD18_READ_MULTIPLE_BLOCK 18
#define EMMC_CMD23_SET_BLOCK_COUNT     23
#define EMMC_CMD24_WRITE_SINGLE_BLOCK  24
#define EMMC_CMD25_WRITE_MULTIPLE_BLOCK 25
#define EMMC_CMD35_ERASE_GROUP_START   35
#define EMMC_CMD36_ERASE_GROUP_END     36
#define EMMC_CMD38_ERASE               38

/*===========================================================================
 * eMMC Response Types
 *===========================================================================*/

#define EMMC_RESP_NONE      0
#define EMMC_RESP_R1        1   /* 48-bit, normal */
#define EMMC_RESP_R1b       2   /* 48-bit, busy */
#define EMMC_RESP_R2        3   /* 136-bit CID/CSD */
#define EMMC_RESP_R3        4   /* 48-bit OCR */
#define EMMC_RESP_R4        5   /* 48-bit, fast I/O */
#define EMMC_RESP_R5        6   /* 48-bit, interrupt */

/*===========================================================================
 * eMMC EXT_CSD Register Offsets (key fields)
 *===========================================================================*/

#define EXT_CSD_CMD_SET_REV            196
#define EXT_CSD_CMD_SET                191
#define EXT_CSD_HS_TIMING              185
#define EXT_CSD_BUS_WIDTH              183
#define EXT_CSD_ERASE_GROUP_DEF        175
#define EXT_CSD_BOOT_BUS_COND          177
#define EXT_CSD_BOOT_CONFIG_PROT       178
#define EXT_CSD_PARTITION_CONFIG       179
#define EXT_CSD_BOOT_SIZE_MULT         226
#define EXT_CSD_RPMB_SIZE_MULT         168
#define EXT_CSD_SEC_COUNT              212
#define EXT_CSD_CACHE_SIZE             249
#define EXT_CSD_PWR_CL_26_360          203
#define EXT_CSD_PWR_CL_52_360          202
#define EXT_CSD_PWR_CL_200_360         201
#define EXT_CSD_PWR_CL_DDR_52_360      200
#define EXT_CSD_PWR_CL_DDR_200_360     199
#define EXT_CSD_CARD_TYPE              196
#define EXT_CSD_REV                    192
#define EXT_CSD_PRE_EOL_INFO           267
#define EXT_CSD_DEVICE_LIFE_TIME_EST_A 268
#define EXT_CSD_DEVICE_LIFE_TIME_EST_B 269
#define EXT_CSD_SUPPORTED_CMD_SET      504

/* EXT_CSD_HS_TIMING values */
#define HS_TIMING_LEGACY       0x00
#define HS_TIMING_HIGH_SPEED   0x01
#define HS_TIMING_HS200        0x02
#define HS_TIMING_HS400        0x03

/* EXT_CSD_BUS_WIDTH values */
#define BUS_WIDTH_1BIT         0x00
#define BUS_WIDTH_4BIT         0x01
#define BUS_WIDTH_8BIT         0x02
#define BUS_WIDTH_4BIT_DDR     0x05
#define BUS_WIDTH_8BIT_DDR     0x06

/* EXT_CSD_PARTITION_CONFIG access */
#define PART_ACCESS_USER       0x00
#define PART_ACCESS_BOOT0      0x01
#define PART_ACCESS_BOOT1      0x02
#define PART_ACCESS_RPMB       0x03

/*===========================================================================
 * eMMC OCR Register Bits
 *===========================================================================*/

#define OCR_VDD_170_195        BIT(7)
#define OCR_VDD_200_260        (0x7FUL << 8)
#define OCR_VDD_270_360        (0x1FFUL << 15)
#define OCR_CARD_CAPACITY      BIT(30)
#define OCR_CARD_POWER_UP      BIT(31)
#define OCR_ACCESS_MODE_MASK   (3UL << 29)
#define OCR_ACCESS_MODE_BYTE   (0UL << 29)
#define OCR_ACCESS_MODE_SECTOR (2UL << 29)

/*===========================================================================
 * eMMC Card State Enumeration
 *===========================================================================*/

typedef enum {
    EMMC_STATE_IDLE     = 0,
    EMMC_STATE_READY    = 1,
    EMMC_STATE_IDENT    = 2,
    EMMC_STATE_STBY     = 3,
    EMMC_STATE_TRAN     = 4,
    EMMC_STATE_DATA     = 5,
    EMMC_STATE_RCV      = 6,
    EMMC_STATE_PRG      = 7,
    EMMC_STATE_DIS      = 8,
    EMMC_STATE_BTST     = 9,
    EMMC_STATE_SLP      = 10,
} emmc_state_t;

/*===========================================================================
 * eMMC Device Info Structure
 *===========================================================================*/

typedef struct {
    uint32_t cid[4];            /* 128-bit Card Identification */
    uint32_t csd[4];            /* 128-bit Card Specific Data */
    uint8_t  ext_csd[512];      /* Extended CSD register */
    uint16_t rca;               /* Relative Card Address */
    uint32_t capacity_blocks;   /* Total user area blocks (512B) */
    uint32_t capacity_bytes;    /* Total user area bytes */
    uint32_t erase_group_size;  /* Erase group in blocks */
    uint32_t boot0_size;        /* Boot partition 0 size in blocks */
    uint32_t boot1_size;        /* Boot partition 1 size in blocks */
    uint32_t rpmb_size;         /* RPMB partition size in blocks */
    uint8_t  hs_timing;         /* Current HS_TIMING */
    uint8_t  bus_width;         /* Current BUS_WIDTH */
    uint8_t  card_type;         /* EXT_CSD CARD_TYPE */
    uint8_t  cmd_set_rev;       /* EXT_CSD CMD_SET_REV */
    uint8_t  rev;               /* EXT_CSD REV (eMMC standard version) */
    uint8_t  pre_eol;           /* Pre-EOL info */
    uint8_t  life_est_a;        /* Device life time estimate type A */
    uint8_t  life_est_b;        /* Device life time estimate type B */
    bool     hs400_supported;   /* Card supports HS400 */
    bool     enhanced_strobe;   /* Card supports enhanced strobe */
} emmc_device_info_t;

/*===========================================================================
 * Acquisition State
 *===========================================================================*/

typedef struct {
    uint32_t start_block;       /* First block to read */
    uint32_t total_blocks;      /* Total blocks to acquire */
    uint32_t blocks_read;       /* Blocks read so far */
    uint32_t blocks_remaining;  /* Blocks remaining */
    uint32_t block_size;        /* Block size (512) */
    uint32_t dma_blocks;        /* Blocks per DMA transfer */
    uint8_t *sdram_buffer;      /* Current SDRAM buffer pointer */
    uint32_t sdram_offset;      /* Offset in ring buffer */
    uint32_t errors;            /* CRC/timeout error count */
    uint32_t retries;           /* Retry count */
    bool     dma_complete;      /* DMA transfer complete flag */
    bool     acquisition_active;/* Acquisition in progress */
    bool     abort_requested;   /* Abort requested */
} emmc_acq_state_t;

/*===========================================================================
 * Public API
 *===========================================================================*/

/**
 * Initialize the SDMMC2 peripheral for eMMC communication.
 * Configures GPIO pins, clocks, and DMA.
 * Returns 0 on success, negative on error.
 */
int emmc_init(void);

/**
 * Detect and initialize an eMMC device.
 * Performs full init sequence: CMD0→CMD1→CMD2→CMD3→CMD7→CMD8→CMD6(HS400).
 * Populates device_info with card parameters.
 * Returns 0 on success, negative error code on failure.
 */
int emmc_detect(emmc_device_info_t *device_info);

/**
 * Read a single 512-byte block from the eMMC device.
 * Uses polled I/O (no DMA). For debug and small reads.
 * Returns 0 on success.
 */
int emmc_read_single_block(uint32_t block_addr, uint8_t *buffer);

/**
 * Start a DMA-based multi-block read acquisition.
 * Reads total_blocks starting from start_block into the SDRAM ring buffer.
 * Uses CMD18 (READ_MULTIPLE_BLOCK) with CMD23 (SET_BLOCK_COUNT).
 * Returns 0 on success, negative on error.
 */
int emmc_acquire_start(emmc_acq_state_t *acq, uint32_t start_block,
                       uint32_t total_blocks);

/**
 * Continue acquisition after a DMA completion interrupt.
 * Called from ISR context. Advances ring buffer and triggers next transfer.
 * Returns 0 if more blocks remain, 1 if acquisition complete.
 */
int emmc_acquire_continue(emmc_acq_state_t *acq);

/**
 * Abort an in-progress acquisition.
 * Sends CMD12 (STOP_TRANSMISSION) and cleans up DMA.
 */
int emmc_acquire_abort(emmc_acq_state_t *acq);

/**
 * Switch the active eMMC partition.
 * partition: PART_ACCESS_USER, PART_ACCESS_BOOT0, PART_ACCESS_BOOT1, PART_ACCESS_RPMB
 * Returns 0 on success.
 */
int emmc_switch_partition(uint8_t partition);

/**
 * Send a SWITCH command (CMD6) to change EXT_CSD register.
 * index: EXT_CSD byte offset
 * value: new value
 * Returns 0 on success.
 */
int emmc_switch(uint8_t index, uint8_t value);

/**
 * Read the 512-byte EXT_CSD register into buffer.
 * Returns 0 on success.
 */
int emmc_read_ext_csd(uint8_t *buffer);

/**
 * Get the current card status (CMD13).
 * Returns the 32-bit status word.
 */
uint32_t emmc_get_status(void);

/**
 * Wait for the card to become ready (not busy).
 * Returns 0 when ready, negative on timeout.
 */
int emmc_wait_ready(uint32_t timeout_ms);

/**
 * Set the SDMMC2 clock frequency.
 * freq_hz: target frequency in Hz
 * Returns actual frequency set.
 */
uint32_t emmc_set_clock(uint32_t freq_hz);

/**
 * Configure the bus width.
 * width: 1, 4, or 8
 */
void emmc_set_bus_width(uint8_t width);

/**
 * Enable DDR mode on the bus.
 */
void emmc_set_ddr_mode(bool enable);

/**
 * Enable hardware flow control (for HS400).
 */
void emmc_set_hw_flow_control(bool enable);

/**
 * Parse EXT_CSD data and populate device_info fields.
 */
void emmc_parse_ext_csd(const uint8_t *ext_csd, emmc_device_info_t *info);

/**
 * Compute eMMC capacity from CSD or EXT_CSD.
 */
uint32_t emmc_compute_capacity(const emmc_device_info_t *info);

/**
 * Get human-readable error string for an error code.
 */
const char *emmc_error_string(int error_code);

/*===========================================================================
 * Error Codes
 *===========================================================================*/

#define EMMC_OK                 0
#define EMMC_ERR_NO_CARD       -1
#define EMMC_ERR_TIMEOUT       -2
#define EMMC_ERR_CRC           -3
#define EMMC_ERR_INIT          -4
#define EMMC_ERR_HS400         -5
#define EMMC_ERR_SWITCH        -6
#define EMMC_ERR_BUSY          -7
#define EMMC_ERR_PARAM         -8
#define EMMC_ERR_DMA           -9
#define EMMC_ERR_ABORTED       -10
#define EMMC_ERR_VOLTAGE       -11
#define EMMC_ERR_UNSUPPORTED   -12

#endif /* EMMC_H */
