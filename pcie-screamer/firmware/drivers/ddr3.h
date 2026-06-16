/**
 * drivers/ddr3.h — DDR3 SDRAM Driver Interface
 *
 * PCIe Screamer — NVMe Interposer
 * Author: jayis1
 * Date: 2026-06-16
 */

#ifndef DDR3_H
#define DDR3_H

#include <stdint.h>
#include <stddef.h>

/*============================================================================
 * DDR3 ORGANIZATION
 *============================================================================*/

#define DDR3_SIZE_BYTES         0x20000000UL  /* 512 MB */
#define DDR3_DATA_WIDTH         16            /* x16 organization */
#define DDR3_COL_BITS           10            /* 1024 columns */
#define DDR3_ROW_BITS           15            /* 32768 rows */
#define DDR3_BANK_BITS          3             /* 8 banks */
#define DDR3_BURST_LENGTH       8             /* BL8 */

/*============================================================================
 * DDR3 TIMING PARAMETERS (for MT41K256M16TW-107, tCK=2.5ns @ 800 MHz)
 *============================================================================*/

#define DDR3_TRC                24    /* tRC = 60ns */
#define DDR3_TRAS               14    /* tRAS = 35ns */
#define DDR3_TRP                6     /* tRP = 15ns */
#define DDR3_TRCD               6     /* tRCD = 15ns */
#define DDR3_TWR                6     /* tWR = 15ns */
#define DDR3_TWTR               4     /* tWTR = 7.5ns → 4 clocks */
#define DDR3_TRTP               4     /* tRTP = 7.5ns → 4 clocks */
#define DDR3_TFAW               16    /* tFAW = 40ns */
#define DDR3_TRRD               4     /* tRRD = 10ns */
#define DDR3_TCCD               4     /* tCCD = 4 clocks (BL8) */
#define DDR3_TREFI              3120  /* tREFI = 7.8µs */
#define DDR3_TRFC               104   /* tRFC = 260ns */
#define DDR3_CAS_LATENCY        6     /* CL = 6 */
#define DDR3_CAS_WRITE_LATENCY  5     /* CWL = 5 */
#define DDR3_WRITE_RECOVERY     8     /* WR = 8 */

/*============================================================================
 * DDR3 MODE REGISTER VALUES
 *============================================================================*/

/* MRS0: Burst Length=8, CAS Latency=6, DLL Reset, Write Recovery=8, Sequential */
#define DDR3_MRS0_VAL           0x0130

/* MRS1: DLL Enable, ODS=RZQ/6(34Ω), RTT_NOM=RZQ/4(40Ω), TDQS disabled, DIC=RZQ/6(34Ω) */
#define DDR3_MRS1_VAL           0x0044

/* MRS2: CWL=5, PASR=Full, ASR=Normal, SRT=Extended */
#define DDR3_MRS2_VAL           0x0000

/* MRS3: MPR=Normal */
#define DDR3_MRS3_VAL           0x0000

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * ddr3_init — Initialize DDR3 SDRAM
 *
 * Performs the full JEDEC DDR3 initialization sequence:
 *   1. Assert RESET#, wait 200 µs
 *   2. Release RESET#, wait 500 µs
 *   3. Assert CKE, wait for stabilization
 *   4. Issue ZQ Calibration Long (ZQCL)
 *   5. Program Mode Registers (MRS0-MRS3)
 *   6. Wait for DLL lock
 *   7. Issue ZQ Calibration Short (ZQCS)
 *
 * Returns: 0 on success, negative on error
 */
int ddr3_init(void);

/**
 * ddr3_calibrate — Perform DDR3 read/write leveling
 *
 * Runs the ECP5 DDR3 hard IP's automatic calibration:
 *   - Write leveling: adjusts DQS-to-CK phase per byte lane
 *   - Read leveling: adjusts DQS gate timing per byte lane
 *
 * Returns: 0 on success, negative on error
 */
int ddr3_calibrate(void);

/**
 * ddr3_write_dma — Write data to DDR3 circular buffer
 *
 * Writes a TLP record to the DDR3 circular buffer with a
 * 4-byte length prefix. The buffer wraps automatically when
 * CTRL_DDR3_WRAP_ENABLE is set.
 *
 * @param data       Record data to write
 * @param len_bytes  Record length in bytes
 * @return           0 on success, -1 if buffer full
 */
int ddr3_write_dma(const uint8_t *data, uint16_t len_bytes);

/**
 * ddr3_read_dma — Read data from DDR3 circular buffer
 *
 * Reads the next TLP record from the DDR3 circular buffer.
 * Returns 0 if the buffer is empty.
 *
 * @param buf      Output buffer
 * @param max_len  Maximum bytes to read
 * @return         Number of bytes read, 0 if empty, negative on error
 */
int ddr3_read_dma(uint8_t *buf, uint16_t max_len);

/**
 * ddr3_get_buffer_usage — Get circular buffer fill percentage
 *
 * Returns: 0-100 (percentage full)
 */
uint8_t ddr3_get_buffer_usage(void);

/**
 * ddr3_get_available — Get available bytes in circular buffer
 *
 * Returns: Number of bytes available to read
 */
uint32_t ddr3_get_available(void);

/**
 * ddr3_get_free_space — Get free bytes in circular buffer
 *
 * Returns: Number of bytes available to write
 */
uint32_t ddr3_get_free_space(void);

/**
 * ddr3_reset_pointers — Reset read/write pointers to zero
 *
 * Effectively clears the circular buffer.
 */
void ddr3_reset_pointers(void);

/**
 * ddr3_self_test — Run a basic DDR3 memory test
 *
 * Writes and reads back test patterns to verify memory integrity.
 *
 * Returns: 0 on success, negative on error
 */
int ddr3_self_test(void);

#endif /* DDR3_H */
