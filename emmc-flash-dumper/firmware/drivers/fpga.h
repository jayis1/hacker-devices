/**
 * fpga.h — iCE40UP5K FPGA Interface Driver
 *
 * Manages FPGA bitstream loading, runtime SPI communication,
 * NAND timing configuration, and interrupt handling.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#ifndef FPGA_H
#define FPGA_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * FPGA Register Map
 *===========================================================================*/

#define FPGA_REG_ID             0x00
#define FPGA_REG_CTRL           0x04
#define FPGA_REG_STATUS         0x08
#define FPGA_REG_NAND_TIMING_0  0x0C
#define FPGA_REG_NAND_TIMING_1  0x10
#define FPGA_REG_NAND_TIMING_2  0x14
#define FPGA_REG_NAND_DATA_IN   0x18
#define FPGA_REG_NAND_DATA_OUT  0x1C
#define FPGA_REG_NAND_CMD       0x20
#define FPGA_REG_NAND_ADDR      0x24
#define FPGA_REG_FIFO_COUNT     0x28
#define FPGA_REG_FIFO_DATA      0x2C
#define FPGA_REG_INTR_MASK      0x30
#define FPGA_REG_INTR_STATUS    0x34

#define FPGA_EXPECTED_ID        0x464E4144UL  /* "FNAD" */

/*===========================================================================
 * FPGA Control Register Bits
 *===========================================================================*/

#define FPGA_CTRL_NAND_ENABLE       BIT(0)
#define FPGA_CTRL_NAND_RESET        BIT(1)
#define FPGA_CTRL_SPI_PASSTHRU      BIT(2)
#define FPGA_CTRL_FIFO_RESET        BIT(3)
#define FPGA_CTRL_ECC_BYPASS        BIT(4)
#define FPGA_CTRL_AUTO_READ         BIT(5)

/*===========================================================================
 * FPGA Status Register Bits
 *===========================================================================*/

#define FPGA_STATUS_READY           BIT(0)
#define FPGA_STATUS_NAND_BUSY       BIT(1)
#define FPGA_STATUS_FIFO_EMPTY      BIT(2)
#define FPGA_STATUS_FIFO_FULL       BIT(3)
#define FPGA_STATUS_FIFO_HALF       BIT(4)
#define FPGA_STATUS_CONFIG_DONE     BIT(7)

/*===========================================================================
 * FPGA Interrupt Flags
 *===========================================================================*/

#define FPGA_INTR_RB_RISE           BIT(0)
#define FPGA_INTR_RB_FALL           BIT(1)
#define FPGA_INTR_FIFO_THRESHOLD    BIT(2)
#define FPGA_INTR_FIFO_OVERFLOW     BIT(3)
#define FPGA_INTR_CMD_DONE          BIT(4)
#define FPGA_INTR_TIMEOUT           BIT(5)

/*===========================================================================
 * NAND Command Definitions
 *===========================================================================*/

#define NAND_CMD_READ_PAGE          0x00
#define NAND_CMD_READ_PAGE_CACHE    0x31
#define NAND_CMD_READ_ID            0x90
#define NAND_CMD_READ_PARAM_PAGE    0xEC
#define NAND_CMD_RESET              0xFF
#define NAND_CMD_STATUS             0x70
#define NAND_CMD_ERASE_BLOCK        0x60
#define NAND_CMD_ERASE_CONFIRM      0xD0
#define NAND_CMD_PROGRAM_PAGE       0x80
#define NAND_CMD_PROGRAM_CONFIRM    0x10
#define NAND_CMD_READ_PAGE_CONFIRM  0x30

/*===========================================================================
 * ONFI Parameter Page Structure
 *===========================================================================*/

typedef struct {
    uint32_t signature;             /* "ONFI" */
    uint16_t revision;
    uint16_t features;
    uint16_t opt_commands;
    uint8_t  manufacturer[12];
    uint8_t  model[20];
    uint8_t  jedec_id;
    uint16_t date_code;
    uint8_t  reserved1[13];
    uint32_t data_bytes_per_page;
    uint16_t spare_bytes_per_page;
    uint32_t pages_per_block;
    uint32_t blocks_per_lun;
    uint8_t  luns_per_target;
    uint8_t  addr_cycles;
    uint8_t  bits_per_cell;
    uint16_t max_bad_blocks_per_lun;
    uint16_t block_endurance;
    uint8_t  guaranteed_valid_blocks;
    uint16_t block_endurance_guaranteed;
    uint8_t  programs_per_page;
    uint8_t  partial_programming;
    uint8_t  bits_ecc_correctability;
    uint8_t  interleaved_addr;
    uint8_t  interleaved_oper;
    uint8_t  reserved2[13];
    uint8_t  io_pin_capacitance;
    uint16_t timing_mode;
    uint16_t tPROG_max;
    uint16_t tBERS_max;
    uint16_t tR_max;
    uint16_t tCCS_min;
    uint8_t  reserved3[23];
    uint16_t vendor_revision;
    uint8_t  vendor_specific[88];
    uint16_t crc;
} onfi_params_t;

/*===========================================================================
 * NAND Timing Configuration
 *===========================================================================*/

typedef struct {
    uint8_t  tWC_ns;        /* Write cycle time */
    uint8_t  tWP_ns;        /* Write pulse width */
    uint8_t  tWH_ns;        /* Write hold time */
    uint8_t  tRC_ns;        /* Read cycle time */
    uint8_t  tRP_ns;        /* Read pulse width */
    uint8_t  tREH_ns;       /* Read enable hold time */
    uint8_t  tWB_ns;        /* WE# high to R/B# low */
    uint8_t  tADL_ns;       /* ALE to data loading */
    uint8_t  tWHR_ns;       /* WE# high to RE# low */
    uint8_t  tR_max_us;     /* Max page read time */
    uint8_t  tBERS_max_ms;  /* Max block erase time */
} nand_timing_t;

/*===========================================================================
 * NAND Device Info
 *===========================================================================*/

typedef struct {
    onfi_params_t onfi;
    nand_timing_t timing;
    uint32_t total_pages;
    uint32_t total_blocks;
    uint32_t page_size;         /* Data bytes per page */
    uint32_t oob_size;          /* Spare bytes per page */
    uint32_t pages_per_block;
    uint32_t blocks_per_lun;
    uint32_t total_size_bytes;
    uint8_t  addr_cycles;
    uint8_t  manufacturer_id;
    uint8_t  device_id;
    bool     onfi_compliant;
    bool     detected;
} nand_device_info_t;

/*===========================================================================
 * Public API
 *===========================================================================*/

/**
 * Initialize the FPGA interface.
 * Configures SPI1, GPIO control pins, and EXTI for interrupt.
 * Returns 0 on success.
 */
int fpga_init(void);

/**
 * Load FPGA bitstream from flash memory via SPI slave configuration.
 * bitstream: pointer to bitstream data
 * length: bitstream length in bytes
 * Returns 0 on success, negative on error.
 */
int fpga_load_bitstream(const uint8_t *bitstream, uint32_t length);

/**
 * Verify FPGA is operational by reading ID register.
 * Returns 0 if ID matches FPGA_EXPECTED_ID.
 */
int fpga_verify(void);

/**
 * Read a 32-bit register from the FPGA.
 * addr: register address (0x00-0x34)
 * Returns register value.
 */
uint32_t fpga_read_reg(uint8_t addr);

/**
 * Write a 32-bit value to an FPGA register.
 * addr: register address
 * value: value to write
 */
void fpga_write_reg(uint8_t addr, uint32_t value);

/**
 * Reset the FPGA (assert CRESET, then release).
 */
void fpga_reset(void);

/**
 * Configure NAND timing parameters in FPGA registers.
 * timing: timing parameters from ONFI detection
 */
void fpga_config_nand_timing(const nand_timing_t *timing);

/**
 * Send a NAND command byte via FPGA.
 * cmd: command byte
 */
void fpga_nand_send_cmd(uint8_t cmd);

/**
 * Send a NAND address sequence via FPGA.
 * addr: 5-byte address (column low, column high, row low, row mid, row high)
 * cycles: number of address cycles (4 or 5)
 */
void fpga_nand_send_addr(const uint8_t *addr, uint8_t cycles);

/**
 * Read captured NAND data from FPGA FIFO.
 * buffer: destination buffer
 * words: number of 32-bit words to read
 */
void fpga_nand_read_fifo(uint32_t *buffer, uint32_t words);

/**
 * Get number of words available in FPGA capture FIFO.
 */
uint32_t fpga_nand_fifo_count(void);

/**
 * Reset the FPGA NAND capture FIFO.
 */
void fpga_nand_fifo_reset(void);

/**
 * Enable/disable NAND controller in FPGA.
 */
void fpga_nand_enable(bool enable);

/**
 * Enable/disable SPI passthrough mode (for SPI NOR access).
 */
void fpga_spi_passthrough(bool enable);

/**
 * Set FPGA interrupt mask.
 * mask: OR of FPGA_INTR_* flags
 */
void fpga_set_intr_mask(uint32_t mask);

/**
 * Read and clear FPGA interrupt status.
 * Returns interrupt flags.
 */
uint32_t fpga_get_intr_status(void);

/**
 * Detect NAND flash via FPGA.
 * Reads ONFI parameter page and populates device info.
 * Returns 0 on success.
 */
int fpga_nand_detect(nand_device_info_t *info);

/**
 * Read a raw NAND page (data + OOB) via FPGA.
 * page_addr: page number
 * buffer: destination buffer (must be page_size + oob_size bytes)
 * Returns 0 on success.
 */
int fpga_nand_read_page(uint32_t page_addr, uint8_t *buffer,
                        const nand_device_info_t *info);

/**
 * Read multiple NAND pages with DMA.
 * start_page: first page
 * count: number of pages
 * sdram_buffer: SDRAM destination
 * Returns 0 on success.
 */
int fpga_nand_read_pages_dma(uint32_t start_page, uint32_t count,
                             uint8_t *sdram_buffer,
                             const nand_device_info_t *info);

/**
 * Get human-readable error string.
 */
const char *fpga_error_string(int error_code);

/*===========================================================================
 * Error Codes
 *===========================================================================*/

#define FPGA_OK                 0
#define FPGA_ERR_LOAD          -1
#define FPGA_ERR_ID            -2
#define FPGA_ERR_TIMEOUT       -3
#define FPGA_ERR_SPI           -4
#define FPGA_ERR_NO_CHIP       -5
#define FPGA_ERR_ONFI          -6
#define FPGA_ERR_READ          -7
#define FPGA_ERR_FIFO_OVF      -8
#define FPGA_ERR_NOT_READY     -9

#endif /* FPGA_H */
