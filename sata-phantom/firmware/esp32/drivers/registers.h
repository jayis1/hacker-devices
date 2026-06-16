/**
 * @file registers.h
 * @author jayis1
 * @brief FPGA register map and memory-mapped I/O interface for SATA Phantom
 *
 * This header defines the complete register interface between the ESP32-S3
 * (SPI master) and the Gowin GW1N-LV1 FPGA (SPI slave). All communication
 * with the FPGA's SATA frame processing pipeline happens through these registers.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * SPI Protocol Overview
 * ===================================================================
 *
 * All register accesses use a 16-bit address followed by 32-bit data.
 *
 * Write transaction (32-bit data):
 *   [CS low] [16-bit ADDR] [32-bit DATA] [CS high]
 *
 * Read transaction (32-bit data read back):
 *   [CS low] [16-bit ADDR | 0x8000] [32-bit DATA from FPGA] [CS high]
 *
 * Multi-word transactions:
 *   Auto-incrementing address mode if ADDR bit 0x4000 is set.
 *   FPGA increments address by 4 each word.
 */

/* Protocol constants */
#define REG_ADDR_MASK           0x3FFF  /* Address field mask (14 bits) */
#define REG_READ_FLAG           0x8000  /* Bit to indicate read operation */
#define REG_AUTO_INC_FLAG       0x4000  /* Bit to enable auto-increment */
#define REG_DATA_WIDTH          32      /* Register data width in bits */
#define REG_ADDR_WIDTH          16      /* Register address width in bits */

/* ===================================================================
 * Core Register Map (16-bit address space)
 * ===================================================================
 *
 * Addresses are 16-bit values. Data is always 32-bit.
 * Reserved addresses read 0x00000000 and ignore writes.
 */

/* --- System & Identification Registers (0x00–0x0F) --- */

#define REG_SYS_VERSION         0x0000  /* R: FPGA firmware version [31:16]=major, [15:8]=minor, [7:0]=patch */
#define REG_SYS_BUILD_DATE      0x0001  /* R: Build date (Unix timestamp seconds since epoch) */
#define REG_SYS_SCRATCH         0x0002  /* R/W: Scratch register for testing */
#define REG_SYS_RESET           0x0003  /* W: Write 0xDEADBEEF to reset FPGA logic */
#define REG_SYS_SLEEP           0x0004  /* W: Write 1 to enter low-power standby */
#define REG_SYS_STATUS          0x0005  /* R: System status flags (see SYS_STATUS_*) */
#define REG_SYS_DIAG            0x0006  /* R: Diagnostic counter (increments each SPI access) */

/* --- Control Registers (0x10–0x1F) --- */

#define REG_CTRL_MAIN           0x0010  /* R/W: Main control register (see CTRL_*) */
#define REG_CTRL_MODE           0x0011  /* R/W: Operation mode: 0=transparent, 1=monitor, 2=active, 3=exfil */
#define REG_CTRL_SPEED_OVERRIDE 0x0012  /* R/W: Speed override: 0=auto, 1=1.5G, 2=3.0G, 3=6.0G */
#define REG_CTRL_ZERO_JITTER    0x0013  /* R/W: Zero-jitter constant delay in microseconds (0=disabled) */
#define REG_CTRL_LOOPBACK       0x0014  /* W: Write 1 to enable internal loopback (TX→RX on FPGA) */
#define REG_CTRL_PASSTHROUGH    0x0015  /* R/W: 1=force passthrough regardless of rules */

/* --- Status & Monitoring Registers (0x20–0x2F) --- */

#define REG_STAT_LINK           0x0020  /* R: Link status (see LINK_STAT_*) */
#define REG_STAT_NEGOTIATED     0x0021  /* R: Negotiated speed: 0=1.5G, 1=3.0G, 2=6.0G */
#define REG_STAT_COUNTER_READ   0x0022  /* R: Total frames from drive→host (48-bit, low 32) */
#define REG_STAT_COUNTER_READ_HI 0x0023 /* R: Total frames from drive→host (high 16 bits) */
#define REG_STAT_COUNTER_WRITE  0x0024  /* R: Total frames from host→drive (48-bit, low 32) */
#define REG_STAT_COUNTER_WRITE_HI 0x0025 /* R: Total frames from host→drive (high 16 bits) */
#define REG_STAT_COUNTER_DROPPED 0x0026 /* R: Frames dropped by rules */
#define REG_STAT_COUNTER_MODIFIED 0x0027 /* R: Frames modified by rules */
#define REG_STAT_COUNTER_ERRORS  0x0028 /* R: Frames with CRC errors */
#define REG_STAT_COUNTER_INJECTED 0x0029 /* R: Frames injected */
#define REG_STAT_COUNTER_OOB     0x002A /* R: OOB signal events (COMINIT/COMWAKE/COMRESET) */
#define REG_STAT_TEMP            0x002B /* R: FPGA die temperature (raw ADC, ~10mV/°C) */
#define REG_STAT_VCCINT          0x002C /* R: FPGA core voltage (1.2V rail ADC) */
#define REG_STAT_VCCIO           0x002D /* R: FPGA IO voltage (3.3V rail ADC) */
#define REG_STAT_POWER           0x002E /* R: Power source (see PWR_SOURCE_*) */
#define REG_STAT_BATTERY         0x002F /* R: Battery voltage in mV */

/* --- Frame Capture Registers (0x30–0x3F) --- */

#define REG_CAPTURE_CTRL        0x0030  /* R/W: Capture control (see CAPTURE_CTRL_*) */
                                        /*   Write 1 → start capture */
                                        /*   Write 0 → stop capture */
                                        /*   Read returns status */
#define REG_CAPTURE_STATUS      0x0031  /* R: Capture status (see CAPTURE_STAT_*) */
#define REG_CAPTURE_LEN         0x0032  /* R: Captured frame length in bytes */
#define REG_CAPTURE_FIS_TYPE    0x0033  /* R: FIS type of captured frame */
#define REG_CAPTURE_LBA_LO      0x0034  /* R: LBA of captured frame (low 32 bits) */
#define REG_CAPTURE_LBA_HI      0x0035  /* R: LBA of captured frame (high 32 bits) */
#define REG_CAPTURE_DIRECTION   0x0036  /* R: Direction (0=read/drive→host, 1=write/host→drive) */
#define REG_CAPTURE_DATA_PORT   0x0037  /* R: Read captured data byte-by-byte (auto-increment addr) */
#define REG_CAPTURE_TIMESTAMP   0x0038  /* R: Capture timestamp in μs since FPGA power-on */

/* --- Rule Engine Registers (0x40–0x5F) --- */

/* The FPGA has 16 identical rule slots. Each rule slot occupies 4 registers:
 *   BASE + 0: Rule LBA start (low 32 bits)
 *   BASE + 1: Rule LBA start (high 32 bits) + opcode
 *   BASE + 2: Rule mask/data pattern (low 32 bits)
 *   BASE + 3: Rule mask/data pattern (high 32 bits) + action + control
 *
 * Where BASE = REG_RULE_SLOT_0 + (slot × 4)
 */

#define REG_RULE_SLOT_0         0x0040  /* Rule slot 0 base address */
#define REG_RULE_SLOT_1         0x0044
#define REG_RULE_SLOT_2         0x0048
#define REG_RULE_SLOT_3         0x004C
#define REG_RULE_SLOT_4         0x0050
#define REG_RULE_SLOT_5         0x0054
#define REG_RULE_SLOT_6         0x0058
#define REG_RULE_SLOT_7         0x005C
#define REG_RULE_SLOT_8         0x0060
#define REG_RULE_SLOT_9         0x0064
#define REG_RULE_SLOT_10        0x0068
#define REG_RULE_SLOT_11        0x006C
#define REG_RULE_SLOT_12        0x0070
#define REG_RULE_SLOT_13        0x0074
#define REG_RULE_SLOT_14        0x0078
#define REG_RULE_SLOT_15        0x007C

/* Rule slot sub-register offsets (relative to slot base) */
#define RULE_OFFS_LBA_LO        0       /* [31:0]   = LBA range start bits [31:0] */
#define RULE_OFFS_LBA_HI_OP     1       /* [31:16]  = LBA range start bits [47:32] */
                                        /* [15:8]   = Opcode match */
                                        /* [7:0]    = Opcode mask */
#define RULE_OFFS_MASK_PAT      2       /* [31:0]   = LBA mask bits [31:0] / data pattern [31:0] */
#define RULE_OFFS_CTRL          3       /* [31:16]  = LBA mask bits [47:32] */
                                        /* [15:12]  = Direction: 0=read, 1=write, 2=both */
                                        /* [11:8]   = Action: 0=monitor, 1=drop, 2=corrupt, 3=inject */
                                        /* [7:4]    = Reserved */
                                        /* [3]      = Enabled */
                                        /* [2:0]    = Priority (0=highest) */

/* Rule engine global registers */
#define REG_RULE_GLOBAL_ENABLE  0x0080  /* R/W: Global rule enable (bitmask, bit N = slot N) */
#define REG_RULE_GLOBAL_HIT     0x0081  /* R: Global rule hit counter (32-bit, saturating) */
#define REG_RULE_LAST_MATCH     0x0082  /* R: Index of last matching rule */
#define REG_RULE_CLEAR_HITS     0x0083  /* W: Write 1 to clear all hit counters */

/* --- Injection FIFO Registers (0x90–0x9F) --- */

#define REG_FIFO_STATUS         0x0090  /* R: FIFO status (see FIFO_STAT_*) */
#define REG_FIFO_CTRL           0x0091  /* W: FIFO control (see FIFO_CTRL_*) */
#define REG_FIFO_PUSH           0x0092  /* W: Push 4 bytes to FIFO (auto-increment cycles through all FIFO words) */
#define REG_FIFO_FLUSH          0x0093  /* W: Write 1 to flush FIFO */
#define REG_FIFO_LEVEL          0x0094  /* R: Current fill level in bytes */
#define REG_FIFO_CAPACITY       0x0095  /* R: Total FIFO capacity in bytes (always reads 512) */

/* FIFO status bits */
#define FIFO_STAT_EMPTY         (1 << 0)
#define FIFO_STAT_FULL          (1 << 1)
#define FIFO_STAT_HALF_FULL     (1 << 2)
#define FIFO_STAT_OVERFLOW      (1 << 3)  /* Write when full — data lost */
#define FIFO_STAT_UNDERFLOW     (1 << 4)  /* Read when empty */
#define FIFO_STAT_READY         (1 << 5)  /* FIFO has data ready for injection */

/* FIFO control bits */
#define FIFO_CTRL_ENABLE        (1 << 0)  /* Enable injection (merge FIFO data into stream) */
#define FIFO_CTRL_FLUSH         (1 << 1)  /* Flush all data */
#define FIFO_CTRL_INSERT_IMM    (1 << 2)  /* Insert at next idle/ALIGN gap */
#define FIFO_CTRL_INSERT_DMA    (1 << 3)  /* Insert inside next DMA Setup frame */
#define FIFO_CTRL_LOOP          (1 << 4)  /* Loop the injected frame continuously (stress test) */

/* --- Scratch Buffer Registers (0xA0–0xAF) --- */

#define REG_SCRATCH_ADDR        0x00A0  /* R/W: Scratch buffer read/write address (0–8191) */
#define REG_SCRATCH_DATA        0x00A1  /* R/W: Scratch buffer data at current address (auto-increment) */
#define REG_SCRATCH_SIZE        0x00A2  /* R: Scratch buffer size in bytes (always reads 8192) */

/* --- OOB Signal Registers (0xB0–0xBF) --- */

#define REG_OOB_STATUS          0x00B0  /* R: OOB signal status (see OOB_STAT_*) */
#define REG_OOB_COUNTER         0x00B1  /* R: OOB event counter */
#define REG_OOB_LAST_TYPE       0x00B2  /* R: Last OOB type: 0=COMINIT, 1=COMWAKE, 2=COMRESET */
#define REG_OOB_LAST_TIMESTAMP  0x00B3  /* R: Timestamp of last OOB event in μs */
#define REG_OOB_TRIGGER         0x00B4  /* W: Write OOB type to generate (0=COMINIT, 1=COMWAKE, 2=COMRESET) */

/* OOB status bits */
#define OOB_STAT_COMINIT_RECV   (1 << 0)
#define OOB_STAT_COMWAKE_RECV   (1 << 1)
#define OOB_STAT_COMRESET_RECV  (1 << 2)
#define OOB_STAT_COMINIT_SENT   (1 << 8)
#define OOB_STAT_COMWAKE_SENT   (1 << 9)
#define OOB_STAT_COMRESET_SENT  (1 << 10)

/* ===================================================================
 * Status & System Register Bit Definitions
 * =================================================================== */

/* REG_SYS_STATUS bits */
#define SYS_STATUS_CONFIG_DONE  (1 << 0)  /* FPGA configuration loaded successfully */
#define SYS_STATUS_PLL_LOCKED   (1 << 1)  /* PLL locked to reference clock */
#define SYS_STATUS_LINK_UP      (1 << 2)  /* SATA link is established */
#define SYS_STATUS_RX_ACTIVE    (1 << 3)  /* Data being received on SATA RX */
#define SYS_STATUS_TX_ACTIVE    (1 << 4)  /* Data being transmitted on SATA TX */
#define SYS_STATUS_CAPTURE_BUSY (1 << 5)  /* Frame capture in progress */
#define SYS_STATUS_OVER_TEMP    (1 << 6)  /* Temperature warning ( > 85°C ) */
#define SYS_STATUS_BROWNOUT     (1 << 7)  /* Brownout condition detected */
#define SYS_STATUS_SPI_ERROR    (1 << 8)  /* SPI protocol error in last transaction */
#define SYS_STATUS_FATAL        (1 << 15) /* Fatal error — FPGA needs reset */

/* REG_STAT_LINK bits */
#define LINK_STAT_ESTABLISHED   (1 << 0)
#define LINK_STAT_SPEED_MASK    (3 << 4)   /* Speed: 0=1.5G, 1=3.0G, 2=6.0G */
#define LINK_STAT_STATE_MASK    (0xF << 8) /* Link state FSM (debug) */
#define LINK_STAT_ALIGN_COUNT   (0xFF << 16) /* ALIGN primitive count */

/* REG_STAT_POWER values */
#define PWR_SOURCE_SATA_5V     0
#define PWR_SOURCE_SATA_12V    1
#define PWR_SOURCE_USB_C       2
#define PWR_SOURCE_BATTERY     3
#define PWR_SOURCE_NONE        4

/* REG_CTRL_MAIN bits */
#define CTRL_MAIN_RESET         (1 << 0)
#define CTRL_MAIN_MONITOR_EN    (1 << 1)
#define CTRL_MAIN_MODIFY_EN     (1 << 2)
#define CTRL_MAIN_CAPTURE_EN    (1 << 3)
#define CTRL_MAIN_ZERO_JITTER   (1 << 4)
#define CTRL_MAIN_LOOPBACK      (1 << 5)

/* REG_CAPTURE_CTRL */
#define CAPTURE_CTRL_START      (1 << 0)  /* Start continuous capture */
#define CAPTURE_CTRL_SINGLE     (1 << 1)  /* Capture one frame then stop */
#define CAPTURE_CTRL_TRIGGER    (1 << 2)  /* Capture on rule match only */
#define CAPTURE_CTRL_FILTER_DIR (1 << 3)  /* Filter by direction (use CAPTURE_CTRL_DIR mask) */
#define CAPTURE_CTRL_DIR_READ   (1 << 4)  /* If FILTER_DIR set: capture reads */
#define CAPTURE_CTRL_DIR_WRITE  (1 << 5)  /* If FILTER_DIR set: capture writes */

/* REG_CAPTURE_STATUS */
#define CAPTURE_STAT_IDLE       (1 << 0)
#define CAPTURE_STAT_CAPTURING  (1 << 1)
#define CAPTURE_STAT_AVAILABLE  (1 << 2)  /* Captured frame ready to read */
#define CAPTURE_STAT_OVERFLOW   (1 << 3)  /* Frame was too large for scratch buffer */
#define CAPTURE_STAT_TRIGGERED  (1 << 4)  /* Capture was trigger-activated */

/* ===================================================================
 * SATA ATA Command Opcodes (subset relevant for filtering)
 * =================================================================== */

/* Read commands */
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_READ_DMA_QUEUED     0xC7
#define ATA_CMD_READ_DMA_QUEUED_EXT 0x26
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_READ_SECTORS_EXT    0x24
#define ATA_CMD_READ_MULTIPLE       0xC4
#define ATA_CMD_READ_MULTIPLE_EXT   0x29
#define ATA_CMD_READ_VERIFY         0x40
#define ATA_CMD_READ_VERIFY_EXT     0x42
#define ATA_CMD_READ_LOG_DMA_EXT    0x47
#define ATA_CMD_READ_LOG_EXT        0x2F
#define ATA_CMD_READ_FPDMA_QUEUED   0x60  /* NCQ read */

/* Write commands */
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_WRITE_DMA_QUEUED    0xCC
#define ATA_CMD_WRITE_DMA_QUEUED_EXT 0x36
#define ATA_CMD_WRITE_SECTORS       0x30
#define ATA_CMD_WRITE_SECTORS_EXT   0x34
#define ATA_CMD_WRITE_MULTIPLE      0xC5
#define ATA_CMD_WRITE_MULTIPLE_EXT  0x39
#define ATA_CMD_WRITE_FPDMA_QUEUED  0x61  /* NCQ write */
#define ATA_CMD_WRITE_LOG_DMA_EXT   0x57
#define ATA_CMD_WRITE_LOG_EXT       0x3F

/* Security commands */
#define ATA_CMD_SECURITY_SET_PASS   0xF1
#define ATA_CMD_SECURITY_UNLOCK     0xF2
#define ATA_CMD_SECURITY_ERASE_PREP 0xF3
#define ATA_CMD_SECURITY_ERASE_UNIT 0xF4
#define ATA_CMD_SECURITY_FREEZE     0xF5
#define ATA_CMD_SECURITY_DISABLE    0xF6

/* Feature commands */
#define ATA_CMD_SET_FEATURES        0xEF
#define ATA_CMD_IDENTIFY_DEVICE     0xEC
#define ATA_CMD_IDENTIFY_PACKET     0xA1
#define ATA_CMD_READ_NATIVE_MAX_EXT 0x27
#define ATA_CMD_SET_MAX_ADDRESS_EXT 0x37
#define ATA_CMD_TRUSTED_RECEIVE     0x5C
#define ATA_CMD_TRUSTED_SEND        0x5E
#define ATA_CMD_TRUSTED_NON_DATA    0x5B
#define ATA_CMD_DEVICE_CONFIGURATION 0xB1

/* Write data patterns of interest */
#define ATA_CMD_DOWNLOAD_MICROCODE  0x92
#define ATA_CMD_STANDBY_IMMEDIATE   0xE0
#define ATA_CMD_IDLE_IMMEDIATE      0xE1
#define ATA_CMD_CHECK_POWER_MODE    0xE5
#define ATA_CMD_SMART_READ_DATA     0xB0  /* SMART subcommand in feature register */

/* ===================================================================
 * SATA FIS Frame Sizes (in bytes, excluding SOF/EOF/CRC)
 * =================================================================== */
#define FIS_SIZE_REG_H2D        20
#define FIS_SIZE_REG_D2H        20
#define FIS_SIZE_DMA_SETUP      28
#define FIS_SIZE_DATA_MIN       1       /* Minimum Data FIS payload */
#define FIS_SIZE_DATA_MAX       8192    /* Maximum Data FIS payload (Gen3) */
#define FIS_SIZE_PIO_SETUP      20
#define FIS_SIZE_DMA_ACTIVATE   4
#define FIS_SIZE_BIST           12
#define FIS_SIZE_DEV_BITS       8

/* Generic frame constants */
#define SATA_DWORD_SIZE         4
#define SATA_CRC_SIZE           4
#define SATA_SOF_DWORD          0x36363636  /* Start of Frame primitive (simplified) */
#define SATA_EOF_DWORD          0xB5B5B5B5  /* End of Frame primitive (simplified) */
#define SATA_ALIGN_DWORD        0x4A4A4A4A  /* ALIGN primitive */

/* ===================================================================
 * Error codes returned by SPI transactions
 * =================================================================== */
#define SPI_OK                  0
#define SPI_ERR_TIMEOUT         -1
#define SPI_ERR_CRC             -2
#define SPI_ERR_BUSY            -3
#define SPI_ERR_INVALID_ADDR    -4
#define SPI_ERR_UNKNOWN         -5

/* ===================================================================
 * Helper macros for register operations
 * =================================================================== */

/* Build a rule slot's sub-register address */
#define RULE_ADDR(slot, offset) (REG_RULE_SLOT_0 + ((slot) * 4) + (offset))

/* Encode a rule slot's LBA_HI_OP register */
#define RULE_LBA_HI_OP(lba_hi, opcode, opcode_mask) \
    (((uint32_t)(lba_hi) << 16) | ((uint32_t)(opcode) << 8) | (uint32_t)(opcode_mask))

/* Encode a rule slot's CTRL register */
#define RULE_CTRL(mask_hi, dir, action, enabled, priority) \
    (((uint32_t)(mask_hi) << 16) | ((uint32_t)(dir) << 12) | \
     ((uint32_t)(action) << 8) | ((uint32_t)(enabled) << 3) | \
     (uint32_t)(priority))

/* Check if a bit in a multi-bit status register matches */
#define REG_MATCH(value, mask)  (((value) & (mask)) == (mask))

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */
