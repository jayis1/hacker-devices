/*
 * registers.h — JTAG/SWD/cJTAG Protocol Register Definitions
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Complete register map for ARM Debug Interface v5/v6, CoreSight,
 * DAP, DP, and AP registers. Used by Forge-Probe firmware for
 * low-level debug access to target silicon.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  JTAG State Machine                                                       *
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    JTAG_STATE_TEST_LOGIC_RESET    = 0x0F,
    JTAG_STATE_RUN_TEST_IDLE       = 0x0C,
    JTAG_STATE_SELECT_DR_SCAN      = 0x0E,
    JTAG_STATE_CAPTURE_DR          = 0x06,
    JTAG_STATE_SHIFT_DR            = 0x02,
    JTAG_STATE_EXIT1_DR            = 0x01,
    JTAG_STATE_PAUSE_DR            = 0x03,
    JTAG_STATE_EXIT2_DR            = 0x00,
    JTAG_STATE_UPDATE_DR           = 0x05,
    JTAG_STATE_SELECT_IR_SCAN      = 0x0D,
    JTAG_STATE_CAPTURE_IR          = 0x07,
    JTAG_STATE_SHIFT_IR            = 0x04,
    JTAG_STATE_EXIT1_IR            = 0x00,
    JTAG_STATE_PAUSE_IR            = 0x02,
    JTAG_STATE_EXIT2_IR            = 0x00,
    JTAG_STATE_UPDATE_IR           = 0x06
} jtag_state_t;

/* JTAG TAP instruction codes (ARM standard) */
#define JTAG_IR_BYPASS             0xFF      /* All 1s: bypass */
#define JTAG_IR_IDCODE             0x0E
#define JTAG_IR_EXTEST             0x0C
#define JTAG_IR_SAMPLE_PRELOAD     0x0D
#define JTAG_IR_INTEST             0x07
#define JTAG_IR_CLAMP              0x0A
#define JTAG_IR_HIGHZ              0x0B
#define JTAG_IR_ARM_DP_ACC         0x0F      /* ARM Debug Port access */

/* Boundary-scan cell types */
#define BS_CELL_BYPASS            0
#define BS_CELL_INPUT             1
#define BS_CELL_OUTPUT            2
#define BS_CELL_BIDIR             3
#define BS_CELL_CONTROL           4
#define BS_CELL_INTERNAL          5
#define BS_CELL_CLOCK             6
#define BS_CELL_OBSERVE_ONLY      7

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  ARM Debug Interface v5 (ADIv5) — Debug Port Register Map                  *
 * ═══════════════════════════════════════════════════════════════════════════ */

/* APnDAP = 0 for DP access, 1 for AP access */
#define DP_SELECT_APnDAP_SHIFT    24
#define DP_SELECT_APSEL_SHIFT     4
#define DP_SELECT_APBANK_SHIFT    0

/* SWD request header bits */
#define SWD_REQUEST_START         0        /* Always 1 */
#define SWD_REQUEST_APnDP        1         /* 0=DP, 1=AP */
#define SWD_REQUEST_RnW          2         /* 0=write, 1=read */
#define SWD_REQUEST_A2           3         /* Address bit 2 */
#define SWD_REQUEST_A3           4         /* Address bit 3 */
#define SWD_REQUEST_PARITY       5         /* Even parity of [4:1] */
#define SWD_REQUEST_STOP         6         /* Always 0 */
#define SWD_REQUEST_PARK         7         /* Drive high/Tr1-state */

/* SWD SELECT register fields */
#define DP_SELECT_APSEL_MASK     (0xFFUL << 24)
#define DP_SELECT_APBANKSEL_MASK (0x0FUL << 0)
#define DP_SELECT_APSEL(x)       ((x & 0xFFUL) << 24)
#define DP_SELECT_APBANKSEL(x)   ((x & 0x0FUL) << 0)

/* DP register addresses (SWD A[3:2]) */
#define DP_REG_DPIDR             0x00      /* Debug Port ID Register (RO) */
#define DP_REG_CTRL_STAT         0x04      /* Control/Status (RW) */
#define DP_REG_SELECT            0x08      /* AP Select (RW) */
#define DP_REG_RDBUFF            0x0C      /* Read Buffer (RO) */
#define DP_REG_TARGETID          0x04      /* Same addr, different SELECT */
#define DP_REG_DLPIDR            0x08      /* Data Link Protocol ID */

/* CTRL_STAT bit definitions */
#define DP_CTRL_STAT_ORUNERR      (1UL << 0)
#define DP_CTRL_STAT_WDATAERR     (1UL << 1)
#define DP_CTRL_STAT_READOK       (1UL << 2)
#define DP_CTRL_STAT_STICKYERR    (1UL << 5)
#define DP_CTRL_STAT_STICKYCMP    (1UL << 8)
#define DP_CTRL_STAT_TRNMODE_BITS 12
#define DP_CTRL_STAT_TRNMODE_MASK (3UL << 12)
#define DP_CTRL_STAT_CDBGRSTREQ   (1UL << 26)
#define DP_CTRL_STAT_CDBGRSTACK   (1UL << 27)
#define DP_CTRL_STAT_CSYSPWRUPREQ (1UL << 28)
#define DP_CTRL_STAT_CSYSPWRUPACK (1UL << 29)
#define DP_CTRL_STAT_CDBGPWRUPREQ (1UL << 30)
#define DP_CTRL_STAT_CDBGPWRUPACK (1UL << 31)

/* Transfer mode */
#define DP_TRNMODE_NORMAL         (0UL << 12)
#define DP_TRNMODE_VERIFY         (1UL << 12)
#define DP_TRNMODE_COMPARE        (2UL << 12)

/* DPIDR fields */
#define DP_DPIDR_DESIGNER_SHIFT   1
#define DP_DPIDR_DESIGNER_MASK    (0x3FFUL << 1)
#define DP_DPIDR_PARTNO_SHIFT     12
#define DP_DPIDR_PARTNO_MASK      (0xFFFFUL << 12)
#define DP_DPIDR_VERSION_SHIFT    28
#define DP_DPIDR_VERSION_MASK     (0xFUL << 28)
#define DP_DPIDR_MIN_SHIFT        24
#define DP_DPIDR_MIN_MASK         (0xFUL << 24)

/* Known DP part numbers */
#define DP_PARTNO_ARM_CS10        0x0A01    /* Cortex-M0+ */
#define DP_PARTNO_ARM_CS11        0x0A02    /* Cortex-M3 */
#define DP_PARTNO_ARM_CS12        0x0A03    /* Cortex-M4 */
#define DP_PARTNO_ARM_CS13        0x0A04    /* Cortex-M7 */
#define DP_PARTNO_ARM_CS14        0x0A05    /* Cortex-M23 */
#define DP_PARTNO_ARM_CS15        0x0A06    /* Cortex-M33 */
#define DP_PARTNO_ARM_CS20        0x0B01    /* Cortex-R5 */
#define DP_PARTNO_ARM_CS30        0x0C01    /* Cortex-A5 */
#define DP_PARTNO_ARM_CS31        0x0C02    /* Cortex-A7 */
#define DP_PARTNO_ARM_CS32        0x0C03    /* Cortex-A9 */
#define DP_PARTNO_ARM_CS33        0x0C04    /* Cortex-A15 */
#define DP_PARTNO_ARM_CS34        0x0C05    /* Cortex-A53 */
#define DP_PARTNO_ARM_CS35        0x0C06    /* Cortex-A72 */

/* DPv3+ TARGETID register */
#define DP_TARGETID_AP_BITS       8, 0     /* Bits 8:0 */
#define DP_TARGETID_TDESIGN       25, 9    /* Bits 25:9 */
#define DP_TARGETID_REVISION      31, 28   /* Bits 31:28 */

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Access Port (AP) Registers — 4-bit APBANK, 4-bit address                  *
 * ═══════════════════════════════════════════════════════════════════════════ */

/* MEM-AP register offsets (within APBANK) */
#define MEMAP_REG_CSW             0x00      /* Control/Status Word */
#define MEMAP_REG_TAR             0x04      /* Transfer Address */
#define MEMAP_REG_DRW             0x0C      /* Data Read/Write */
#define MEMAP_REG_BD0             0x10      /* Banked Data 0 */
#define MEMAP_REG_BD1             0x14      /* Banked Data 1 */
#define MEMAP_REG_BD2             0x18      /* Banked Data 2 */
#define MEMAP_REG_BD3             0x1C      /* Banked Data 3 */
#define MEMAP_REG_CFG             0xF4      /* AP Configuration (RO) */
#define MEMAP_REG_BASE            0xF8      /* Debug Base Address (RO) */
#define MEMAP_REG_IDR             0xFC      /* AP Identification (RO) */

/* CSW bit definitions */
#define MEMAP_CSW_SIZE_BYTE       (0UL << 0)
#define MEMAP_CSW_SIZE_HALF       (1UL << 0)
#define MEMAP_CSW_SIZE_WORD       (2UL << 0)
#define MEMAP_CSW_SIZE_DWORD      (3UL << 0)
#define MEMAP_CSW_ADDRINC_MASK    (3UL << 4)
#define MEMAP_CSW_ADDRINC_OFF     (0UL << 4)     /* Fixed address */
#define MEMAP_CSW_ADDRINC_SINGLE  (1UL << 4)     /* Increment single */
#define MEMAP_CSW_ADDRINC_PACKED  (2UL << 4)     /* 8/16-bit packed */
#define MEMAP_CSW_DEVICE_EN       (1UL << 6)
#define MEMAP_CSW_TRPROBE         (1UL << 7)
#define MEMAP_CSW_SPIDEN          (1UL << 23)    /* Secure debugging */
#define MEMAP_CSW_PROT(n)         ((n & 0x1FUL) << 24)
#define MEMAP_CSW_MODE_MASK       (3UL << 29)
#define MEMAP_CSW_MODE_NORMAL     (0UL << 29)
#define MEMAP_CSW_MODE_DBG        (1UL << 29)
#define MEMAP_CSW_HPROT_SHIFT     24
#define MEMAP_CSW_HPROT_MASK      (0x3FUL << 24)

/* AP IDR fields */
#define AP_IDR_REVISION_MASK      (0xFUL << 0)
#define AP_IDR_JEDEC_SHIFT        1
#define AP_IDR_JEDEC_MASK         (1UL << 1)
#define AP_IDR_CLASS_MASK         (0xFUL << 8)
#define AP_IDR_VARIANT_MASK       (0xFUL << 12)
#define AP_IDR_TYPE_MASK          (0xFFUL << 16)
#define AP_IDR_DESIGNER_SHIFT     24
#define AP_IDR_DESIGNER_MASK      (0xFFUL << 24)

/* AP class values */
#define AP_CLASS_NONE             0x0
#define AP_CLASS_MEM_AP           0x1
#define AP_CLASS_AHB_AP           0x2
#define AP_CLASS_APB_AP           0x3
#define AP_CLASS_AXI_AP           0x4
#define AP_CLASS_AHB5_AP          0x5
#define AP_CLASS_APB5_AP          0x6

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  CoreSight ROM Table / Component Discovery                                 *
 * ═══════════════════════════════════════════════════════════════════════════ */

/* CoreSight component class */
#define CS_CLASS_0X00             0         /* Generic / miscellaneous */
#define CS_CLASS_0X01             1         /* ROM table */
#define CS_CLASS_0X09             9         /* Debug component */
#define CS_CLASS_0X0A             10        /* Trace component */
#define CS_CLASS_0X0B             11        /* PMU component */
#define CS_CLASS_0X0C             12        /* CTI component */

/* ROM table entry format */
#define ROM_ENTRY_PRESENT         (1UL << 0)
#define ROM_ENTRY_FORMAT_MASK     (3UL << 1)
#define ROM_ENTRY_OFFSET_SHIFT    12
#define ROM_ENTRY_OFFSET_MASK     (0xFFFFFUL << 12)

/* Known CoreSight PIDs */
#define PID_CORTEX_M7_ROM         0x00000
#define PID_CORTEX_M4_ROM         0x00001
#define PID_CORTEX_M33_ROM        0x00002
#define PID_ETM_M7                0x00101
#define PID_ITM                   0x00102
#define PID_DWT                   0x00103
#define PID_FPB                   0x00104
#define PID_TPIU                  0x00105
#define PID_CTI                   0x00106
#define PID_ETB                   0x00107
#define PID_STM                   0x00108
#define PID_GENERIC_DAP           0x01000

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  cJTAG (IEEE 1149.7) — Compact JTAG Protocol                               *
 * ═══════════════════════════════════════════════════════════════════════════ */

/* cJTAG operation codes (3-bit) */
#define CJTAG_OP_SHIFT           0x0       /* Enter Shift-DR on next TCKC */
#define CJTAG_OP_HALT            0x1       /* Halt (TMSC held low for 8+ clocks) */
#define CJTAG_OP_TLRS            0x2       /* Go to Test-Logic-Reset */
#define CJTAG_OP_STDCTRL         0x3       /* Standard control sequence */
#define CJTAG_OP_TLR_CNT         0x4       /* TLRS with counter */
#define CJTAG_OP_XFER            0x5       /* Data transfer */
#define CJTAG_OP_RSUP            0x6       /* Resume / supply config */
#define CJTAG_OP_EXT             0x7       /* Extended opcode */

/* cJTAG JLOC layer commands */
#define CJTAG_JLOC_IDLE          0x00
#define CJTAG_JLOC_START_SCAN    0x01
#define CJTAG_JLOC_SCAN          0x02
#define CJTAG_JLOC_END_SCAN      0x03
#define CJTAG_JLOC_SET_CLOCK     0x04
#define CJTAG_JLOC_GET_CLOCK     0x05
#define CJTAG_JLOC_POWER_CTRL    0x06

/* cJTAG scan format (3-bit codes in two transport clocks) */
#define CJTAG_SCAN_4BIT          0x0
#define CJTAG_SCAN_8BIT          0x1
#define CJTAG_SCAN_16BIT         0x2
#define CJTAG_SCAN_32BIT         0x3
#define CJTAG_SCAN_LEADING       0x4
#define CJTAG_SCAN_TRAILING      0x5

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Forge-Probe Hardware Register Map (MCU Memory-Mapped, 32-bit)             *
 * ═══════════════════════════════════════════════════════════════════════════ */

#define FP_BASE                   0x60000000UL    /* FPGA BRAM window */
#define FP_CTRL                   0x60000000UL    /* Control register */
#define FP_STATUS                 0x60000004UL    /* Status register */
#define FP_CLOCK_DIV              0x60000008UL    /* TCK/SWCLK clock divider */
#define FP_TAP_CTRL               0x6000000CUL    /* TAP state control */
#define FP_DR_BUFFER              0x60000100UL    /* Data register buffer (4 KB) */
#define FP_IR_BUFFER              0x60001100UL    /* Instruction register buffer */
#define FP_BS_BUFFER              0x60002000UL    /* Boundary scan buffer (32 KB) */
#define FP_BS_CHAIN_LEN           0x6000A000UL    /* Boundary scan chain length */
#define FP_SWD_REQ                0x6000A004UL    /* SWD request packet */
#define FP_SWD_RESP               0x6000A008UL    /* SWD response packet */
#define FP_SWD_ACK                0x6000A00CUL    /* SWD ACK status (3-bit) */
#define FP_CJTAG_CTRL             0x6000A010UL    /* cJTAG controller */

/* FP_CTRL bit definitions */
#define FP_CTRL_ENABLE            (1UL << 0)
#define FP_CTRL_SOFT_RESET        (1UL << 1)
#define FP_CTRL_PROTOCOL_MASK     (3UL << 2)
#define FP_CTRL_PROTOCOL_JTAG     (0UL << 2)
#define FP_CTRL_PROTOCOL_SWD      (1UL << 2)
#define FP_CTRL_PROTOCOL_CJTAG    (2UL << 2)
#define FP_CTRL_TARGET_PWR        (1UL << 4)
#define FP_CTRL_TARGET_VIO_SHIFT  5
#define FP_CTRL_TARGET_VIO_MASK   (3UL << 5)
#define FP_CTRL_LOOPBACK          (1UL << 7)
#define FP_CTRL_BITBANG           (1UL << 8)
#define FP_CTRL_INT_EN            (1UL << 16)
#define FP_CTRL_DMA_EN            (1UL << 17)
#define FP_CTRL_STREAM_MODE       (1UL << 18)

/* FP_STATUS bit definitions */
#define FP_STATUS_BSY             (1UL << 0)
#define FP_STATUS_DR_DONE         (1UL << 1)
#define FP_STATUS_IR_DONE         (1UL << 2)
#define FP_STATUS_TAP_ERROR       (1UL << 3)
#define FP_STATUS_SWD_PARITY_ERR  (1UL << 4)
#define FP_STATUS_SWD_PROTO_ERR   (1UL << 5)
#define FP_STATUS_OVERCURRENT     (1UL << 6)
#define FP_STATUS_TARGET_PRESENT  (1UL << 7)
#define FP_STATUS_VREF_READY      (1UL << 8)
#define FP_STATUS_BS_DONE         (1UL << 9)
#define FP_STATUS_DMA_DONE        (1UL << 16)

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Common ARM Memory-Mapped Peripheral Bases                                 *
 * ═══════════════════════════════════════════════════════════════════════════ */

#define ARM_SCS_BASE              0xE000E000UL    /* System Control Space */
#define ARM_ITM_BASE              0xE0000000UL    /* Instrumentation Trace */
#define ARM_DWT_BASE              0xE0001000UL    /* Data Watchpoint & Trace */
#define ARM_FPB_BASE              0xE0002000UL    /* Flash Patch & Breakpoint */
#define ARM_TPIU_BASE             0xE0040000UL    /* Trace Port Interface */
#define ARM_ETM_BASE              0xE0041000UL    /* Embedded Trace Macrocell */
#define ARM_CTI_BASE              0xE0042000UL    /* Cross-Trigger Interface */
#define ARM_STM_BASE              0xE0044000UL    /* System Trace Macrocell */
#define ARM_ETB_BASE              0xE0045000UL    /* Embedded Trace Buffer */
#define ARM_ROM_TABLE             0xE00FF000UL    /* Cortex-M ROM Table */

/* SCS registers */
#define SCS_CPUID                 0xE000ED00UL
#define SCS_ICSR                  0xE000ED04UL
#define SCS_AIRCR                 0xE000ED0CUL
#define SCS_SCR                   0xE000ED10UL
#define SCS_CCR                   0xE000ED14UL
#define SCS_SHPR1                 0xE000ED18UL
#define SCS_SHPR2                 0xE000ED1CUL
#define SCS_SHPR3                 0xE000ED20UL
#define SCS_SHCSR                 0xE000ED24UL
#define SCS_CFSR                  0xE000ED28UL
#define SCS_HFSR                  0xE000ED2CUL
#define SCS_DFSR                  0xE000ED30UL
#define SCS_MMFAR                 0xE000ED34UL
#define SCS_BFAR                  0xE000ED38UL
#define SCS_DHCSR                 0xE000EDF0UL
#define SCS_DCRSR                 0xE000EDF4UL
#define SCS_DCRDR                 0xE000EDF8UL
#define SCS_DEMCR                 0xE000EDFCUL

/* DHCSR (Debug Halting Control & Status Register) */
#define DHCSR_DEBUGEN             (1UL << 0)
#define DHCSR_HALT                (1UL << 1)
#define DHCSR_STEP                (1UL << 2)
#define DHCSR_REGRDBK             (1UL << 16)
#define DHCSR_REGWRBK             (1UL << 17)
#define DHCSR_S_RESET_ST          (1UL << 25)
#define DHCSR_S_RETIRE_ST         (1UL << 26)
#define DHCSR_S_LOCKUP            (1UL << 19)
#define DHCSR_S_SLEEP             (1UL << 18)
#define DHCSR_S_HALT              (1UL << 17)
#define DHCSR_S_REGRDY            (1UL << 16)

/* DCRSR (Debug Core Register Selector) fields */
#define DCRSR_REGSEL_SHIFT        0
#define DCRSR_REGSEL_MASK         (0x1FUL << 0)
#define DCRSR_WNR_BIT             16
#define DCRSR_REGWNR              (1UL << 16)

/* Core register numbers for DCRSR */
#define DCRSR_R0                  0x00
#define DCRSR_R1                  0x01
#define DCRSR_R2                  0x02
#define DCRSR_R3                  0x03
#define DCRSR_R4                  0x04
#define DCRSR_R5                  0x05
#define DCRSR_R6                  0x06
#define DCRSR_R7                  0x07
#define DCRSR_R8                  0x08
#define DCRSR_R9                  0x09
#define DCRSR_R10                 0x0A
#define DCRSR_R11                 0x0B
#define DCRSR_R12                 0x0C
#define DCRSR_R13                 0x0D          /* MSP/PSP based on current */
#define DCRSR_R13_MSP             0x0D
#define DCRSR_R13_PSP             0x0E
#define DCRSR_R14                 0x0F          /* LR */
#define DCRSR_R15                 0x10          /* PC */
#define DCRSR_XPSR                0x11
#define DCRSR_MSP                 0x12          /* Main SP */
#define DCRSR_PSP                 0x13          /* Process SP */
#define DCRSR_PRIMASK             0x14
#define DCRSR_BASEPRI             0x15
#define DCRSR_FAULTMASK           0x16
#define DCRSR_CONTROL             0x17
#define DCRSR_FPSCR               0x21         /* Float status */
#define DCRSR_FPSYS               0x22         /* FP system register */

/* DEMCR */
#define DEMCR_VC_CORERESET        (1UL << 0)
#define DEMCR_VC_MMERR            (1UL << 4)
#define DEMCR_VC_NOCPERR          (1UL << 5)
#define DEMCR_VC_STATERR          (1UL << 6)
#define DEMCR_VC_CHKERR           (1UL << 7)
#define DEMCR_VC_INTERR           (1UL << 8)
#define DEMCR_VC_BUSERR           (1UL << 9)
#define DEMCR_SDONE               (1UL << 16)
#define DEMCR_MON_EN              (1UL << 16)
#define DEMCR_MON_PEND            (1UL << 17)
#define DEMCR_MON_STEP            (1UL << 18)
#define DEMCR_MON_REQ             (1UL << 19)
#define DEMCR_TRCENA              (1UL << 24)
#define DEMCR_VC_HARDERR          (1UL << 10)

/* AIRCR */
#define AIRCR_VECTKEY             (0x05FAUL << 16)
#define AIRCR_SYSRESETREQ         (1UL << 2)
#define AIRCR_VECTCLRACTIVE       (1UL << 1)
#define AIRCR_ENDIANNESS          (1UL << 15)

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Forge-Probe Discovery Protocol — Target Description Structure             *
 * ═══════════════════════════════════════════════════════════════════════════ */

#define FP_MAX_APS                8
#define FP_MAX_ROMS               16
#define FP_MAX_TAPS               4          /* JTAG daisy-chain taps */
#define FP_DESC_STRING_LEN        48

typedef enum {
    FP_PROTO_JTAG = 0,
    FP_PROTO_SWD  = 1,
    FP_PROTO_CJTAG = 2,
    FP_PROTO_SWJ  = 3          /* Auto-detect SWD vs JTAG */
} fp_protocol_t;

typedef enum {
    FP_ARCH_UNKNOWN = 0,
    FP_ARCH_CORTEX_M0,
    FP_ARCH_CORTEX_M3,
    FP_ARCH_CORTEX_M4,
    FP_ARCH_CORTEX_M7,
    FP_ARCH_CORTEX_M23,
    FP_ARCH_CORTEX_M33,
    FP_ARCH_CORTEX_R4,
    FP_ARCH_CORTEX_R5,
    FP_ARCH_CORTEX_A5,
    FP_ARCH_CORTEX_A7,
    FP_ARCH_CORTEX_A9,
    FP_ARCH_CORTEX_A15,
    FP_ARCH_CORTEX_A53,
    FP_ARCH_CORTEX_A72,
    FP_ARCH_RISCV,
    FP_ARCH_CUSTOM
} fp_target_arch_t;

typedef struct {
    uint32_t        idcode;             /* 32-bit IDCODE register */
    uint32_t        dpidr;              /* Debug Port IDR */
    fp_protocol_t   protocol;
    fp_target_arch_t architecture;
    uint32_t        target_id;          /* TARGETID (DPv3+) */
    uint32_t        mem_ap_base;        /* MEM-AP BASE address */
    uint8_t         num_aps;            /* Available APs */
    uint32_t        ap_bases[FP_MAX_APS];
    uint32_t        rom_table_base;
    uint32_t        rom_entries[FP_MAX_ROMS];
    uint8_t         num_rom_entries;
    uint8_t         ir_length;          /* JTAG IR length (4–32 bits) */
    uint8_t         tap_count;          /* Daisy-chain TAP count */
    char            description[FP_DESC_STRING_LEN];
    uint32_t        boundary_chain_len; /* Boundary scan cells */
    uint16_t        manufacturer_id;    /* JEDEC manufacturer code */
    bool            secure_debug;       /* SPIDEN status */
    bool            flash_locked;       /* Flash readout protection */
    uint8_t         debug_level;        /* 0=locked, 1=restricted, 2=full */
} fp_target_desc_t;

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Forge-Probe Flash Programming / Readout Commands                          *
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Common flash controller offsets (vendor-dependent, used by flash driver) */
#define FLASH_CMD_PROGRAM         0x01
#define FLASH_CMD_ERASE_SECTOR    0x02
#define FLASH_CMD_ERASE_MASS      0x03
#define FLASH_CMD_READ            0x04
#define FLASH_CMD_READ_OPTION     0x05
#define FLASH_CMD_WRITE_OPTION    0x06
#define FLASH_CMD_UNLOCK          0x07
#define FLASH_CMD_LOCK            0x08
#define FLASH_CMD_OB_LOAD         0x09

/* Flash status */
#define FLASH_STATUS_BUSY         (1UL << 0)
#define FLASH_STATUS_WRITE_ERR    (1UL << 1)
#define FLASH_STATUS_READY        (1UL << 2)
#define FLASH_STATUS_PROTECT      (1UL << 3)
#define FLASH_STATUS_ECC_ERR      (1UL << 4)

#endif /* REGISTERS_H */