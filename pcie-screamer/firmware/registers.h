/**
 * registers.h — PCIe Screamer MMIO Register Map
 *
 * Complete memory-mapped I/O register definitions for the PCIe Screamer
 * FPGA gateware. All registers are 32-bit, little-endian, accessible
 * from the RISC-V soft CPU at base address 0x80000000.
 *
 * Author: jayis1
 * Date: 2026-06-16
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/*============================================================================
 * BASE ADDRESSES
 *============================================================================*/

#define MMIO_BASE               0x80000000UL

#define SCR_BASE                (MMIO_BASE + 0x00000000)  /* System Control Registers */
#define PCIE_BASE               (MMIO_BASE + 0x10000000)  /* PCIe Hard IP Registers */
#define DDR3_BASE               (MMIO_BASE + 0x20000000)  /* DDR3 Controller Registers */
#define USB_BASE                (MMIO_BASE + 0x30000000)  /* USB/FT601 Registers */
#define SPI_BASE                (MMIO_BASE + 0x40000000)  /* SPI Flash Registers */
#define GPIO_BASE               (MMIO_BASE + 0x50000000)  /* GPIO Registers */
#define TLP_SNIFFER_BASE        (MMIO_BASE + 0x60000000)  /* TLP Sniffer Registers */
#define TLP_INJECTOR_BASE       (MMIO_BASE + 0x70000000)  /* TLP Injector Registers */

/*============================================================================
 * SYSTEM CONTROL REGISTERS (SCR_BASE + offset)
 *============================================================================*/

#define SCR_VERSION             (SCR_BASE + 0x0000)  /* RO: Firmware version */
#define SCR_STATUS              (SCR_BASE + 0x0004)  /* RO: Device status flags */
#define SCR_CONTROL             (SCR_BASE + 0x0008)  /* RW: Global control */
#define SCR_ERROR               (SCR_BASE + 0x000C)  /* RO: Error status (W1C) */
#define SCR_ERROR_MASK          (SCR_BASE + 0x0010)  /* RW: Error interrupt mask */
#define SCR_TIMESTAMP_LO        (SCR_BASE + 0x0014)  /* RO: Timestamp low 32 bits */
#define SCR_TIMESTAMP_HI        (SCR_BASE + 0x0018)  /* RO: Timestamp high 16 bits */
#define SCR_CAPTURE_COUNT       (SCR_BASE + 0x001C)  /* RO: Total TLPs captured */
#define SCR_DDR3_WRITE_PTR      (SCR_BASE + 0x0020)  /* RO: DDR3 write pointer */
#define SCR_DDR3_READ_PTR       (SCR_BASE + 0x0024)  /* RW: DDR3 read pointer */
#define SCR_DDR3_OVERFLOW_COUNT (SCR_BASE + 0x0028)  /* RO: Buffer overflow count */
#define SCR_USB_TX_BYTES        (SCR_BASE + 0x002C)  /* RO: USB bytes transmitted */
#define SCR_USB_RX_BYTES        (SCR_BASE + 0x0030)  /* RO: USB bytes received */
#define SCR_PCIE_LTSSM_STATE    (SCR_BASE + 0x0034)  /* RO: PCIe LTSSM state */
#define SCR_PCIE_LINK_SPEED     (SCR_BASE + 0x0038)  /* RO: Negotiated link speed */
#define SCR_PCIE_LINK_WIDTH     (SCR_BASE + 0x003C)  /* RO: Negotiated link width */
#define SCR_PCIE_RX_TLP_COUNT   (SCR_BASE + 0x0040)  /* RO: TLPs from host */
#define SCR_PCIE_TX_TLP_COUNT   (SCR_BASE + 0x0044)  /* RO: TLPs to host */
#define SCR_INJECT_COUNT        (SCR_BASE + 0x0048)  /* RO: Injected TLPs sent */
#define SCR_INJECT_ERROR_COUNT  (SCR_BASE + 0x004C)  /* RO: Injection errors */
#define SCR_TEMP_SENSOR         (SCR_BASE + 0x0050)  /* RO: Temperature sensor */
#define SCR_VOLT_MON_3V3        (SCR_BASE + 0x0054)  /* RO: 3.3V rail monitor */
#define SCR_VOLT_MON_1V1        (SCR_BASE + 0x0058)  /* RO: 1.1V rail monitor */
#define SCR_VOLT_MON_1V35       (SCR_BASE + 0x005C)  /* RO: 1.35V rail monitor */
#define SCR_VOLT_MON_1V2        (SCR_BASE + 0x0060)  /* RO: 1.2V rail monitor */
#define SCR_VOLT_MON_1V8        (SCR_BASE + 0x0064)  /* RO: 1.8V rail monitor */
#define SCR_CAPTURE_FILTER_MASK (SCR_BASE + 0x0068)  /* RW: TLP type filter mask */
#define SCR_CAPTURE_FILTER_ADDR_LO (SCR_BASE + 0x006C) /* RW: Address filter low */
#define SCR_CAPTURE_FILTER_ADDR_HI (SCR_BASE + 0x0070) /* RW: Address filter high */
#define SCR_CAPTURE_FILTER_REQID (SCR_BASE + 0x0074)  /* RW: Requester ID filter */
#define SCR_INJECT_CONTROL      (SCR_BASE + 0x0078)  /* RW: Injection control */
#define SCR_INJECT_ADDR_LO      (SCR_BASE + 0x007C)  /* RW: Injection address low */
#define SCR_INJECT_ADDR_HI      (SCR_BASE + 0x0080)  /* RW: Injection address high */
#define SCR_INJECT_TLP_TYPE     (SCR_BASE + 0x0084)  /* RW: Injection TLP type */
#define SCR_INJECT_LENGTH       (SCR_BASE + 0x0088)  /* RW: Injection length (DW) */
#define SCR_INJECT_TAG          (SCR_BASE + 0x008C)  /* RW: Injection tag */
#define SCR_INJECT_REQID        (SCR_BASE + 0x0090)  /* RW: Injection requester ID */
#define SCR_INJECT_DATA_FIFO    (SCR_BASE + 0x0094)  /* WO: Injection data FIFO */
#define SCR_INJECT_TRIGGER      (SCR_BASE + 0x0098)  /* WO: Trigger injection */
#define SCR_INJECT_RESULT       (SCR_BASE + 0x009C)  /* RO: Injection result */
#define SCR_FW_UPDATE_CTRL      (SCR_BASE + 0x00A0)  /* RW: FW update control */
#define SCR_FW_UPDATE_ADDR      (SCR_BASE + 0x00A4)  /* RW: SPI flash address */
#define SCR_FW_UPDATE_DATA      (SCR_BASE + 0x00A8)  /* WO: SPI flash write data */
#define SCR_FW_UPDATE_CRC       (SCR_BASE + 0x00AC)  /* RO: Update image CRC */
#define SCR_REDRIVER_CTRL       (SCR_BASE + 0x00B0)  /* RW: Redriver I2C control */
#define SCR_CLOCK_CTRL          (SCR_BASE + 0x00B4)  /* RW: Si5332 clock control */
#define SCR_GPIO_OUT            (SCR_BASE + 0x00B8)  /* RW: GPIO output values */
#define SCR_GPIO_IN             (SCR_BASE + 0x00BC)  /* RO: GPIO input values */
#define SCR_GPIO_OE             (SCR_BASE + 0x00C0)  /* RW: GPIO output enable */
#define SCR_DDR3_CAL_STATUS     (SCR_BASE + 0x00C4)  /* RO: DDR3 calibration */
#define SCR_DDR3_READ_LATENCY   (SCR_BASE + 0x00C8)  /* RW: DDR3 read latency */
#define SCR_DDR3_WRITE_LATENCY  (SCR_BASE + 0x00CC)  /* RW: DDR3 write latency */
#define SCR_DDR3_REFRESH_COUNT  (SCR_BASE + 0x00D0)  /* RO: DDR3 refresh count */
#define SCR_DDR3_ECC_ERROR_COUNT (SCR_BASE + 0x00D4) /* RO: DDR3 ECC errors */
#define SCR_SELF_TEST_RESULT    (SCR_BASE + 0x00D8)  /* RO: Self-test result */
#define SCR_SELF_TEST_CTRL      (SCR_BASE + 0x00DC)  /* RW: Self-test control */
#define SCR_WATCHDOG_KICK       (SCR_BASE + 0x00E0)  /* WO: Watchdog pet */
#define SCR_INJECT_DATA_BUFFER  (SCR_BASE + 0x1000)  /* WO: 4KB injection buffer */

/*============================================================================
 * SCR_STATUS Bit Definitions
 *============================================================================*/

#define STATUS_PCIE_LINK_UP         (1 << 0)
#define STATUS_USB_ENUMERATED       (1 << 1)
#define STATUS_CAPTURE_ACTIVE       (1 << 2)
#define STATUS_DDR3_READY           (1 << 3)
#define STATUS_DDR3_OVERFLOW        (1 << 4)
#define STATUS_INJECT_BUSY          (1 << 5)
#define STATUS_FW_UPDATE_BUSY       (1 << 6)
#define STATUS_SELF_TEST_BUSY       (1 << 7)
#define STATUS_TEMP_WARNING         (1 << 8)
#define STATUS_TEMP_CRITICAL        (1 << 9)
#define STATUS_VOLT_3V3_FAULT       (1 << 10)
#define STATUS_VOLT_1V1_FAULT       (1 << 11)
#define STATUS_VOLT_1V35_FAULT      (1 << 12)
#define STATUS_VOLT_1V2_FAULT       (1 << 13)
#define STATUS_VOLT_1V8_FAULT       (1 << 14)
#define STATUS_GOLDEN_IMAGE         (1 << 15)
#define STATUS_BITSTREAM_ENCRYPTED  (1 << 16)

/*============================================================================
 * SCR_CONTROL Bit Definitions
 *============================================================================*/

#define CTRL_CAPTURE_ENABLE         (1 << 0)
#define CTRL_CAPTURE_HOST2DEV       (1 << 1)
#define CTRL_CAPTURE_DEV2HOST       (1 << 2)
#define CTRL_CAPTURE_CFG_TLP        (1 << 3)
#define CTRL_CAPTURE_MSG_TLP        (1 << 4)
#define CTRL_CAPTURE_CPL_TLP        (1 << 5)
#define CTRL_CAPTURE_MEM_TLP        (1 << 6)
#define CTRL_CAPTURE_IO_TLP         (1 << 7)
#define CTRL_FILTER_ENABLE          (1 << 8)
#define CTRL_TIMESTAMP_ENABLE       (1 << 9)
#define CTRL_DDR3_WRAP_ENABLE       (1 << 10)
#define CTRL_USB_STREAM_ENABLE      (1 << 11)
#define CTRL_INJECT_ENABLE          (1 << 12)
#define CTRL_INJECT_ARMED           (1 << 13)
#define CTRL_LED_OVERRIDE           (1 << 14)
#define CTRL_SOFT_RESET             (1 << 15)

/*============================================================================
 * SCR_CAPTURE_FILTER_MASK Bit Definitions
 *============================================================================*/

#define FILTER_MRD                  (1 << 0)   /* Memory Read Request */
#define FILTER_MWR                  (1 << 1)   /* Memory Write Request */
#define FILTER_MRDLK                (1 << 2)   /* Memory Read Lock */
#define FILTER_IORD                 (1 << 3)   /* I/O Read */
#define FILTER_IOWR                 (1 << 4)   /* I/O Write */
#define FILTER_CFGRD0               (1 << 5)   /* Config Read Type 0 */
#define FILTER_CFGWR0               (1 << 6)   /* Config Write Type 0 */
#define FILTER_CFGRD1               (1 << 7)   /* Config Read Type 1 */
#define FILTER_CFGWR1               (1 << 8)   /* Config Write Type 1 */
#define FILTER_MSG                  (1 << 9)   /* Message */
#define FILTER_MSGD                 (1 << 10)  /* Message with Data */
#define FILTER_CPL                  (1 << 11)  /* Completion */
#define FILTER_CPLD                 (1 << 12)  /* Completion with Data */
#define FILTER_CPLLK                (1 << 13)  /* Completion Locked */
#define FILTER_CPLDLK               (1 << 14)  /* Completion Locked with Data */
#define FILTER_FETCHADD             (1 << 15)  /* Fetch and Add AtomicOp */
#define FILTER_SWAP                 (1 << 16)  /* Unconditional Swap */
#define FILTER_CAS                  (1 << 17)  /* Compare and Swap */

/*============================================================================
 * TLP Type Definitions (for injection)
 *============================================================================*/

#define TLP_TYPE_MRD                0x00
#define TLP_TYPE_MWR                0x01
#define TLP_TYPE_MRDLK              0x02
#define TLP_TYPE_IORD               0x03
#define TLP_TYPE_IOWR               0x04
#define TLP_TYPE_CFGRD0             0x05
#define TLP_TYPE_CFGWR0             0x06
#define TLP_TYPE_CFGRD1             0x07
#define TLP_TYPE_CFGWR1             0x08
#define TLP_TYPE_MSG                0x09
#define TLP_TYPE_MSGD               0x0A
#define TLP_TYPE_CPL                0x0B
#define TLP_TYPE_CPLD               0x0C

/*============================================================================
 * Injection Result Codes
 *============================================================================*/

#define INJECT_RESULT_SUCCESS       0
#define INJECT_RESULT_TIMEOUT       1
#define INJECT_RESULT_UR            2   /* Unsupported Request */
#define INJECT_RESULT_CA            3   /* Completer Abort */
#define INJECT_RESULT_CRS           4   /* Configuration Retry Status */

/*============================================================================
 * PCIe LTSSM States
 *============================================================================*/

#define LTSSM_DETECT_QUIET          0x00
#define LTSSM_DETECT_ACTIVE         0x01
#define LTSSM_POLLING_ACTIVE        0x02
#define LTSSM_POLLING_COMPLIANCE    0x03
#define LTSSM_POLLING_CONFIG        0x04
#define LTSSM_CONFIG_LINKWIDTH_START 0x05
#define LTSSM_CONFIG_LINKWIDTH_ACCEPT 0x06
#define LTSSM_CONFIG_LANENUM_WAIT   0x07
#define LTSSM_CONFIG_LANENUM_ACCEPT 0x08
#define LTSSM_CONFIG_COMPLETE       0x09
#define LTSSM_CONFIG_IDLE           0x0A
#define LTSSM_RECOVERY_RCVR_LOCK    0x0B
#define LTSSM_RECOVERY_SPEED        0x0C
#define LTSSM_RECOVERY_RCVR_CFG     0x0D
#define LTSSM_RECOVERY_IDLE         0x0E
#define LTSSM_L0                    0x0F
#define LTSSM_L0S                   0x10
#define LTSSM_L1                    0x11
#define LTSSM_L2                    0x12
#define LTSSM_DISABLED              0x13
#define LTSSM_LOOPBACK              0x14
#define LTSSM_HOT_RESET             0x15

/*============================================================================
 * PCIe Link Speed Codes
 *============================================================================*/

#define PCIE_SPEED_GEN1             1    /* 2.5 GT/s */
#define PCIE_SPEED_GEN2             2    /* 5.0 GT/s */
#define PCIE_SPEED_GEN3             3    /* 8.0 GT/s */

/*============================================================================
 * PCIe Link Width Codes
 *============================================================================*/

#define PCIE_WIDTH_X1               1
#define PCIE_WIDTH_X2               2
#define PCIE_WIDTH_X4               4

/*============================================================================
 * PCIe Hard IP Register Offsets (PCIE_BASE + offset)
 *============================================================================*/

#define PCIE_CTRL_REG               (PCIE_BASE + 0x0000)
#define PCIE_STATUS_REG             (PCIE_BASE + 0x0004)
#define PCIE_LTSSM_STATE_REG        (PCIE_BASE + 0x0008)
#define PCIE_LINK_SPEED_REG         (PCIE_BASE + 0x000C)
#define PCIE_LINK_WIDTH_REG         (PCIE_BASE + 0x0010)
#define PCIE_MAX_SPEED_REG          (PCIE_BASE + 0x0014)
#define PCIE_LINK_WIDTH_CFG_REG     (PCIE_BASE + 0x0018)
#define PCIE_MAX_PAYLOAD_REG        (PCIE_BASE + 0x001C)
#define PCIE_VENDOR_ID_REG          (PCIE_BASE + 0x0020)
#define PCIE_DEVICE_ID_REG          (PCIE_BASE + 0x0024)
#define PCIE_BAR0_REG               (PCIE_BASE + 0x0028)
#define PCIE_BAR0_MASK_REG          (PCIE_BASE + 0x002C)
#define PCIE_RX_EQ_REG              (PCIE_BASE + 0x0030)
#define PCIE_TX_DEEMPH_REG          (PCIE_BASE + 0x0034)
#define PCIE_RX_TLP_ENABLE_REG      (PCIE_BASE + 0x0038)
#define PCIE_TX_TLP_ENABLE_REG      (PCIE_BASE + 0x003C)
#define PCIE_RX_FIFO_STATUS_REG     (PCIE_BASE + 0x0040)
#define PCIE_RX_FIFO_DATA_REG       (PCIE_BASE + 0x0044)
#define PCIE_TX_FIFO_STATUS_REG     (PCIE_BASE + 0x0048)
#define PCIE_TX_FIFO_DATA_REG       (PCIE_BASE + 0x004C)

/* PCIE_CTRL_REG bits */
#define PCIE_CTRL_RESET             (1 << 0)
#define PCIE_CTRL_LINK_TRAIN_EN     (1 << 1)

/* PCIE_STATUS_REG bits */
#define PCIE_STATUS_PIPE_RDY        (1 << 0)
#define PCIE_STATUS_LINK_UP         (1 << 1)
#define PCIE_STATUS_TLP_RX_RDY      (1 << 2)
#define PCIE_STATUS_TLP_TX_RDY      (1 << 3)

/* PCIE_RX_FIFO_STATUS_REG bits */
#define PCIE_RX_FIFO_NOT_EMPTY      (1 << 0)
#define PCIE_RX_FIFO_OVERFLOW       (1 << 1)

/* PCIE_TX_FIFO_STATUS_REG bits */
#define PCIE_TX_FIFO_NOT_FULL       (1 << 0)
#define PCIE_TX_FIFO_UNDERFLOW      (1 << 1)

/*============================================================================
 * DDR3 Controller Register Offsets (DDR3_BASE + offset)
 *============================================================================*/

#define DDR3_CTRL_REG               (DDR3_BASE + 0x0000)
#define DDR3_STATUS_REG             (DDR3_BASE + 0x0004)
#define DDR3_MRS0_REG               (DDR3_BASE + 0x0008)
#define DDR3_MRS1_REG               (DDR3_BASE + 0x000C)
#define DDR3_MRS2_REG               (DDR3_BASE + 0x0010)
#define DDR3_MRS3_REG               (DDR3_BASE + 0x0014)
#define DDR3_TIMING_TRC_REG         (DDR3_BASE + 0x0018)
#define DDR3_TIMING_TRAS_REG        (DDR3_BASE + 0x001C)
#define DDR3_TIMING_TRP_REG         (DDR3_BASE + 0x0020)
#define DDR3_TIMING_TRCD_REG        (DDR3_BASE + 0x0024)
#define DDR3_TIMING_TWR_REG         (DDR3_BASE + 0x0028)
#define DDR3_TIMING_TWTR_REG        (DDR3_BASE + 0x002C)
#define DDR3_TIMING_TRTP_REG        (DDR3_BASE + 0x0030)
#define DDR3_TIMING_TFAW_REG        (DDR3_BASE + 0x0034)
#define DDR3_TIMING_TRRD_REG        (DDR3_BASE + 0x0038)
#define DDR3_TIMING_TCCD_REG        (DDR3_BASE + 0x003C)
#define DDR3_TIMING_TREFI_REG       (DDR3_BASE + 0x0040)
#define DDR3_TIMING_TRFC_REG        (DDR3_BASE + 0x0044)
#define DDR3_WRITE_PTR_REG          (DDR3_BASE + 0x0048)
#define DDR3_READ_PTR_REG           (DDR3_BASE + 0x004C)
#define DDR3_BUFFER_SIZE_REG        (DDR3_BASE + 0x0050)
#define DDR3_WRITE_DATA_REG         (DDR3_BASE + 0x0054)
#define DDR3_READ_DATA_REG          (DDR3_BASE + 0x0058)
#define DDR3_OVERFLOW_COUNT_REG     (DDR3_BASE + 0x005C)
#define DDR3_CAL_CTRL_REG           (DDR3_BASE + 0x0060)
#define DDR3_CAL_STATUS_REG         (DDR3_BASE + 0x0064)
#define DDR3_CAL_RESULT0_REG        (DDR3_BASE + 0x0068)
#define DDR3_CAL_RESULT1_REG        (DDR3_BASE + 0x006C)

/* DDR3_CTRL_REG bits */
#define DDR3_CTRL_RESET             (1 << 0)
#define DDR3_CTRL_INIT_START        (1 << 1)

/* DDR3_STATUS_REG bits */
#define DDR3_STATUS_READY           (1 << 0)
#define DDR3_STATUS_CAL_DONE        (1 << 1)
#define DDR3_STATUS_REFRESHING      (1 << 2)

/* DDR3_CAL_CTRL_REG bits */
#define DDR3_CAL_WRITE_LEVELING     (1 << 0)
#define DDR3_CAL_READ_LEVELING      (1 << 1)
#define DDR3_CAL_DONE               (1 << 31)

/* DDR3 MRS0 fields */
#define DDR3_MRS0_CL6               (6 << 4)
#define DDR3_MRS0_WR8               (8 << 9)
#define DDR3_MRS0_DLL_RESET         (1 << 8)
#define DDR3_MRS0_BT_SEQ            (0 << 3)
#define DDR3_MRS0_BL8               (0 << 0)

/* DDR3 MRS1 fields */
#define DDR3_MRS1_DIC_34            (0 << 1)
#define DDR3_MRS1_RTT_40            (2 << 9)
#define DDR3_MRS1_ODS_34            (0 << 5)
#define DDR3_MRS1_DLL_EN            (0 << 0)

/* DDR3 MRS2 fields */
#define DDR3_MRS2_CWL5              (0 << 3)
#define DDR3_MRS2_PASR_FULL         (0 << 0)
#define DDR3_MRS2_ASR_NORMAL        (0 << 6)
#define DDR3_MRS2_SRT_EXTENDED      (0 << 7)

/* DDR3 MRS3 fields */
#define DDR3_MRS3_MPR_NORMAL        (0 << 2)

/*============================================================================
 * USB/FT601 Register Offsets (USB_BASE + offset)
 *============================================================================*/

#define USB_CTRL_REG                (USB_BASE + 0x0000)
#define USB_STATUS_REG              (USB_BASE + 0x0004)
#define USB_TX_FIFO_DATA_REG        (USB_BASE + 0x0008)
#define USB_TX_FIFO_COUNT_REG       (USB_BASE + 0x000C)
#define USB_RX_FIFO_DATA_REG        (USB_BASE + 0x0010)
#define USB_RX_FIFO_COUNT_REG       (USB_BASE + 0x0014)
#define USB_TX_BYTE_COUNT_REG       (USB_BASE + 0x0018)
#define USB_RX_BYTE_COUNT_REG       (USB_BASE + 0x001C)

/*============================================================================
 * GPIO Register Offsets (GPIO_BASE + offset)
 *============================================================================*/

#define GPIO_OUT_REG                (GPIO_BASE + 0x0000)
#define GPIO_IN_REG                 (GPIO_BASE + 0x0004)
#define GPIO_OE_REG                 (GPIO_BASE + 0x0008)
#define GPIO_INT_EN_REG             (GPIO_BASE + 0x000C)
#define GPIO_INT_STATUS_REG         (GPIO_BASE + 0x0010)

/*============================================================================
 * TLP Sniffer Register Offsets (TLP_SNIFFER_BASE + offset)
 *============================================================================*/

#define SNIFFER_CTRL_REG            (TLP_SNIFFER_BASE + 0x0000)
#define SNIFFER_STATUS_REG          (TLP_SNIFFER_BASE + 0x0004)
#define SNIFFER_FILTER_MASK_REG     (TLP_SNIFFER_BASE + 0x0008)
#define SNIFFER_FILTER_ADDR_LO_REG  (TLP_SNIFFER_BASE + 0x000C)
#define SNIFFER_FILTER_ADDR_HI_REG  (TLP_SNIFFER_BASE + 0x0010)
#define SNIFFER_FILTER_REQID_REG    (TLP_SNIFFER_BASE + 0x0014)
#define SNIFFER_TLP_COUNT_REG       (TLP_SNIFFER_BASE + 0x0018)
#define SNIFFER_TLP_DROPPED_REG     (TLP_SNIFFER_BASE + 0x001C)

/*============================================================================
 * TLP Injector Register Offsets (TLP_INJECTOR_BASE + offset)
 *============================================================================*/

#define INJECTOR_CTRL_REG           (TLP_INJECTOR_BASE + 0x0000)
#define INJECTOR_STATUS_REG         (TLP_INJECTOR_BASE + 0x0004)
#define INJECTOR_ADDR_LO_REG        (TLP_INJECTOR_BASE + 0x0008)
#define INJECTOR_ADDR_HI_REG        (TLP_INJECTOR_BASE + 0x000C)
#define INJECTOR_TLP_TYPE_REG       (TLP_INJECTOR_BASE + 0x0010)
#define INJECTOR_LENGTH_REG         (TLP_INJECTOR_BASE + 0x0014)
#define INJECTOR_TAG_REG            (TLP_INJECTOR_BASE + 0x0018)
#define INJECTOR_REQID_REG          (TLP_INJECTOR_BASE + 0x001C)
#define INJECTOR_DATA_FIFO_REG      (TLP_INJECTOR_BASE + 0x0020)
#define INJECTOR_TRIGGER_REG        (TLP_INJECTOR_BASE + 0x0024)
#define INJECTOR_RESULT_REG         (TLP_INJECTOR_BASE + 0x0028)
#define INJECTOR_COUNT_REG          (TLP_INJECTOR_BASE + 0x002C)
#define INJECTOR_ERROR_COUNT_REG    (TLP_INJECTOR_BASE + 0x0030)

/*============================================================================
 * REGISTER ACCESS MACROS
 *============================================================================*/

/* Volatile register read/write */
#define REG32(addr)                 (*(volatile uint32_t *)(addr))

#define REG_READ(addr)              REG32(addr)
#define REG_WRITE(addr, val)        (REG32(addr) = (val))
#define REG_SET_BITS(addr, mask)    (REG32(addr) |= (mask))
#define REG_CLR_BITS(addr, mask)    (REG32(addr) &= ~(mask))
#define REG_WRITE_BITS(addr, mask, val) \
    (REG32(addr) = (REG32(addr) & ~(mask)) | ((val) & (mask)))

/* Polling helper */
#define REG_POLL_UNTIL(addr, mask, val, timeout_us) \
    ({ \
        int __ret = -1; \
        for (uint32_t __t = 0; __t < (timeout_us); __t++) { \
            if ((REG_READ(addr) & (mask)) == (val)) { \
                __ret = 0; \
                break; \
            } \
            delay_us(1); \
        } \
        __ret; \
    })

#endif /* REGISTERS_H */
