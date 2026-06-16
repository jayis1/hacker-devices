# Phase 4: Software Stack — PCIe Screamer

## 1. Overview

The PCIe Screamer firmware stack spans FPGA gateware (Verilog RTL), embedded C firmware running on a soft CPU or state machines within the FPGA, and a React Native companion application. This document covers the complete software architecture: boot strategy, register maps, clock initialization, GPIO configuration, device drivers with DMA, the USB wire protocol, and build instructions.

### 1.1 Software Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                    PCIE SCREAMER SOFTWARE STACK                      │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  COMPANION APP (React Native)                                │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │  │
│  │  │ Capture  │ │ Analysis │ │Injection │ │ Settings │       │  │
│  │  │ Screen   │ │ Screen   │ │ Screen   │ │ Screen   │       │  │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘       │  │
│  │       └─────────────┴─────────────┴─────────────┘           │  │
│  │                         │                                    │  │
│  │  ┌──────────────────────┴──────────────────────────────┐    │  │
│  │  │  utils/protocol.js — Binary Wire Protocol           │    │  │
│  │  │  Frame builder/parser, CRC-32C, command dispatch    │    │  │
│  │  └──────────────────────┬──────────────────────────────┘    │  │
│  └─────────────────────────┼───────────────────────────────────┘  │
│                            │ USB 3.0 (FT601 D3XX)                 │
│  ┌─────────────────────────┼───────────────────────────────────┐  │
│  │  FPGA GATEWARE (Verilog RTL)                                │  │
│  │                         │                                    │  │
│  │  ┌──────────────────────┴──────────────────────────────┐    │  │
│  │  │  usb_egress_engine / tlp_injector (command handler)  │    │  │
│  │  └──────────────────────┬──────────────────────────────┘    │  │
│  │                         │                                    │  │
│  │  ┌──────────────────────┴──────────────────────────────┐    │  │
│  │  │  tlp_sniffer / ddr3_controller / pcie_hard_ip       │    │  │
│  │  └─────────────────────────────────────────────────────┘    │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  EMBEDDED C FIRMWARE (optional soft CPU or boot state FSM)  │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │  │
│  │  │ main.c   │ │ board.h  │ │registers │ │ drivers/ │       │  │
│  │  │ Init +   │ │ Pin defs │ │ .h MMIO  │ │ pcie.c   │       │  │
│  │  │ main loop│ │ AF mux   │ │ register │ │ ddr3.c   │       │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

## 2. Boot Strategy

### 2.1 FPGA Configuration Boot Flow

The ECP5-45F boots from external SPI flash in Master SPI mode. The boot sequence is:

```
Power-On Reset (POR) Asserted
    │
    ▼
VCC3V3, VCC1V8, VCC1V2, VCC1V1, VCC1V35 rails sequence up
    │
    ▼
FPGA POR released (PROGRAMN goes high, internal POR timer ~10 ms)
    │
    ▼
FPGA samples CFG[1:0] pins → 01 = Master SPI x1 mode
    │
    ▼
FPGA drives INITN low (clearing configuration SRAM)
    │
    ▼
FPGA releases INITN (goes high, pulled up externally)
    │
    ▼
FPGA drives CSPI_CSN low, begins clocking CSPI_CLK at ~2.5 MHz
    │
    ▼
FPGA reads bitstream header from SPI flash:
    │  Byte 0-3:   Sync word (0x7E 0xAA 0x99 0x7E)
    │  Byte 4-7:   Bitstream length (big-endian)
    │  Byte 8-11:  IDCODE / command opcodes
    │  Byte 12+:   Configuration frame data
    │
    ▼
FPGA loads configuration frames into internal SRAM (~4.7 Mb for 45F)
    │
    ▼
FPGA performs CRC check on loaded configuration
    │
    ▼
FPGA asserts DONE high → enters user mode
    │
    ▼
User logic starts: PLL lock → DDR3 init → PCIe link training → USB enum
```

### 2.2 Bitstream Format & Encryption

The ECP5 supports AES-128 encrypted bitstreams. The bitstream structure:

```
Offset  | Content
--------|----------------------------------
0x0000  | Sync word: 0x7EAA997E
0x0004  | Bitstream length (32-bit BE)
0x0008  | IDCODE command: 0xE2 0x00 0x00 0x00 <32-bit IDCODE>
0x000C  | ISC_ENABLE command: 0xC6 0x08 0x00 0x00
0x0010  | ISC_ERASE command: 0x0E 0x04 0x00 0x00
0x0014  | LSC_INIT_ADDRESS: 0x46 0x00 0x00 0x00
0x0018  | LSC_WRITE_COMP_DIC: 0x02 <DIC bytes>
0x001C+ | LSC_PROG_INCR_RTI: 0x82 <frame data> (repeated)
...     | ...
End-16  | ISC_PROGRAM_DONE: 0x5E 0x00 0x00 0x00
End-12  | ISC_DISABLE: 0x26 0x00 0x00 0x00
End-8   | LSC_REFRESH: 0x79 0x00 0x00 0x00
End-4   | NOOP: 0xFF 0xFF 0xFF 0xFF
```

For encrypted bitstreams, the `LSC_PROG_INCR_RTI` frames are AES-128 encrypted with a key stored in eFuse. The FPGA decrypts on-the-fly during configuration.

### 2.3 Multi-Boot / Golden Image Strategy

The 128 Mb SPI flash (W25Q128JV) can hold multiple bitstreams:

```
Flash Layout:
  0x000000 - 0x5FFFFF: Golden bitstream (6 MB, factory-programmed)
  0x600000 - 0xBFFFFF: Update bitstream (6 MB, field-upgradable)
  0xC00000 - 0xFFFFFF: User data / calibration / persistent config (4 MB)
```

The FPGA CFG pins can be strapped for multi-boot. If the update image fails CRC, the FPGA falls back to the golden image.

### 2.4 Firmware Initialization Sequence (Post-Configuration)

Once the FPGA enters user mode, the firmware state machine executes:

```
State 0: RESET
    ├── Assert all internal resets
    ├── Wait 100 µs for PLL power stabilization
    └── Transition to PLL_INIT

State 1: PLL_INIT
    ├── Configure PLL0: 125 MHz from PCIe REFCLK (100 MHz in, ×5/4 = 125 MHz)
    ├── Configure PLL1: 100 MHz from 125 MHz (×4/5 = 100 MHz)
    ├── Configure PLL2: 400 MHz from 125 MHz (×16/5 = 400 MHz)
    ├── Wait for all PLL lock signals
    └── Transition to DDR3_INIT

State 2: DDR3_INIT
    ├── Assert DDR3_RESET# low, wait 200 µs
    ├── Release DDR3_RESET#, wait 500 µs until CKE can be asserted
    ├── Assert CKE high
    ├── Wait 400 ns (tXPR = 5 × tCK = 5 × 2.5 ns = 12.5 ns, but spec says 400 ns min after reset)
    ├── Issue ZQ Calibration command (ZQCL)
    ├── Wait 512 clock cycles (tZQinit = 512 × 2.5 ns = 1.28 µs)
    ├── Issue MRS (Mode Register Set):
    │   MRS0: CL=6, WR=8, DLL Reset=1, AL=0, BT=Sequential, BL=8
    │   MRS1: DIC=34Ω, TDQS=disabled, RTT_NOM=40Ω, AL=0, DLL=enable, ODS=34Ω
    │   MRS2: CWL=5, PASR=full, ASR=normal, SRT=extended
    │   MRS3: MPR=normal, MPR_RF=normal
    ├── Wait for DLL lock (tDLLK = 512 clock cycles)
    ├── Issue ZQ Calibration command (ZQCS)
    ├── DDR3 ready for normal operation
    └── Transition to PCIE_INIT

State 3: PCIE_INIT
    ├── Assert PCIE_PERST# to device (hold NVMe in reset)
    ├── Configure PCIe Hard IP registers:
    │   PCIE_LINK_WIDTH = x4
    │   PCIE_MAX_LINK_SPEED = Gen2 (5.0 GT/s)
    │   PCIE_RX_EQ_MODE = adaptive
    │   PCIE_TX_DEEMPH = -3.5 dB
    ├── Enable PCIe Hard IP PCS and PMA
    ├── Wait for PIPE interface ready
    ├── Release PCIE_PERST# to device
    ├── Wait for link training:
    │   Poll LTSSM state until L0 (active link)
    │   Timeout: 100 ms (per PCIe spec)
    ├── If link fails to train:
    │   Retry with Gen1 (2.5 GT/s) fallback
    │   If still fails: set error LED, halt
    └── Transition to USB_INIT

State 4: USB_INIT
    ├── Assert FT_RESET# low, wait 1 ms
    ├── Release FT_RESET#, wait 10 ms for FT601 internal init
    ├── Start 100 MHz clock to FT601 (FT_CLK)
    ├── Wait for FT601 enumeration on USB bus:
    │   Monitor FT_SUSPEND# (low = enumerated and active)
    │   Timeout: 5 seconds
    ├── Configure FT601 FIFO mode:
    │   32-bit wide, 100 MHz, synchronous FIFO
    │   FT_WR# / FT_RD# / FT_OE# / FT_SIWU# control
    └── Transition to ACTIVE

State 5: ACTIVE
    ├── Enable TLP sniffer pipeline
    ├── Enable USB egress engine
    ├── Enable command parser (for injection commands)
    ├── Set LED0 (Green) = PCIe Link Up
    ├── Set LED1 (Blue) = USB Enumerated
    └── Enter main loop
```

## 3. MMIO Register Map

### 3.1 Register Address Space

The firmware exposes a memory-mapped register space accessible from the USB command interface. All registers are 32-bit wide, little-endian.

```
Base Address: 0x8000_0000 (within FPGA address space)

Offset    | Name                    | Access | Description
----------|-------------------------|--------|----------------------------
0x0000    | SCR_VERSION             | RO     | Firmware version (major.minor.patch.build)
0x0004    | SCR_STATUS              | RO     | Device status flags
0x0008    | SCR_CONTROL             | RW     | Global control register
0x000C    | SCR_ERROR               | RO     | Error status (latched, write-1-to-clear)
0x0010    | SCR_ERROR_MASK          | RW     | Error interrupt mask
0x0014    | SCR_TIMESTAMP_LO        | RO     | 48-bit timestamp, low 32 bits
0x0018    | SCR_TIMESTAMP_HI        | RO     | 48-bit timestamp, high 16 bits
0x001C    | SCR_CAPTURE_COUNT       | RO     | Total TLPs captured since reset
0x0020    | SCR_DDR3_WRITE_PTR      | RO     | DDR3 circular buffer write pointer
0x0024    | SCR_DDR3_READ_PTR       | RW     | DDR3 circular buffer read pointer
0x0028    | SCR_DDR3_OVERFLOW_COUNT | RO     | Number of buffer overflow events
0x002C    | SCR_USB_TX_BYTES        | RO     | Total bytes transmitted over USB
0x0030    | SCR_USB_RX_BYTES        | RO     | Total bytes received over USB
0x0034    | SCR_PCIE_LTSSM_STATE    | RO     | Current PCIe LTSSM state
0x0038    | SCR_PCIE_LINK_SPEED     | RO     | Negotiated link speed (1=Gen1, 2=Gen2, 3=Gen3)
0x003C    | SCR_PCIE_LINK_WIDTH     | RO     | Negotiated link width (1,2,4)
0x0040    | SCR_PCIE_RX_TLP_COUNT   | RO     | TLPs received from host
0x0044    | SCR_PCIE_TX_TLP_COUNT   | RO     | TLPs transmitted to host
0x0048    | SCR_INJECT_COUNT        | RO     | Injected TLPs sent
0x004C    | SCR_INJECT_ERROR_COUNT  | RO     | Injection errors (malformed, rejected)
0x0050    | SCR_TEMP_SENSOR         | RO     | Internal temperature sensor (FPGA diode)
0x0054    | SCR_VOLT_MON_3V3        | RO     | 3.3V rail monitor (ADC counts)
0x0058    | SCR_VOLT_MON_1V1        | RO     | 1.1V rail monitor
0x005C    | SCR_VOLT_MON_1V35       | RO     | 1.35V rail monitor
0x0060    | SCR_VOLT_MON_1V2        | RO     | 1.2V rail monitor
0x0064    | SCR_VOLT_MON_1V8        | RO     | 1.8V rail monitor
0x0068    | SCR_CAPTURE_FILTER_MASK | RW     | TLP type filter mask (see below)
0x006C    | SCR_CAPTURE_FILTER_ADDR_LO | RW  | Address filter low 32 bits
0x0070    | SCR_CAPTURE_FILTER_ADDR_HI | RW  | Address filter high 32 bits
0x0074    | SCR_CAPTURE_FILTER_REQID  | RW  | Requester ID filter
0x0078    | SCR_INJECT_CONTROL      | RW     | Injection control register
0x007C    | SCR_INJECT_ADDR_LO      | RW     | Injection target address low
0x0080    | SCR_INJECT_ADDR_HI      | RW     | Injection target address high
0x0084    | SCR_INJECT_TLP_TYPE     | RW     | Injection TLP type (MRd=0x00, MWr=0x01, CfgRd=0x04, CfgWr=0x05)
0x0088    | SCR_INJECT_LENGTH       | RW     | Injection TLP length (in DW, 1-1024)
0x008C    | SCR_INJECT_TAG          | RW     | Injection TLP tag
0x0090    | SCR_INJECT_REQID        | RW     | Injection requester ID
0x0094    | SCR_INJECT_DATA_FIFO    | WO     | Injection data FIFO (write data here for MWr)
0x0098    | SCR_INJECT_TRIGGER      | WO     | Write any value to trigger injection
0x009C    | SCR_INJECT_RESULT       | RO     | Injection result (0=success, 1=timeout, 2=UR, 3=CA, 4=CRS)
0x00A0    | SCR_FW_UPDATE_CTRL      | RW     | Firmware update control
0x00A4    | SCR_FW_UPDATE_ADDR      | RW     | SPI flash address for update
0x00A8    | SCR_FW_UPDATE_DATA      | WO     | SPI flash write data FIFO
0x00AC    | SCR_FW_UPDATE_CRC       | RO     | Computed CRC of update image
0x00B0    | SCR_REDRIVER_CTRL        | RW     | Redriver I2C control (EQ, de-emphasis settings)
0x00B4    | SCR_CLOCK_CTRL           | RW     | Si5332 clock generator control
0x00B8    | SCR_GPIO_OUT             | RW     | GPIO output values
0x00BC    | SCR_GPIO_IN              | RO     | GPIO input values
0x00C0    | SCR_GPIO_OE              | RW     | GPIO output enable
0x00C4    | SCR_DDR3_CAL_STATUS      | RO     | DDR3 calibration status
0x00C8    | SCR_DDR3_READ_LATENCY    | RW     | DDR3 read latency adjustment
0x00CC    | SCR_DDR3_WRITE_LATENCY   | RW     | DDR3 write latency adjustment
0x00D0    | SCR_DDR3_REFRESH_COUNT   | RO     | DDR3 refresh cycle count
0x00D4    | SCR_DDR3_ECC_ERROR_COUNT | RO     | DDR3 ECC error count (if ECC enabled)
0x00D8    | SCR_SELF_TEST_RESULT     | RO     | Built-in self-test result
0x00DC    | SCR_SELF_TEST_CTRL       | RW     | Self-test control (write 1 to start)
0x00E0-0FFC| (Reserved)             | —      | Reserved for future expansion
0x1000-1FFC| SCR_INJECT_DATA_BUFFER | WO     | 4 KB injection data buffer (for large MWr)
```

### 3.2 SCR_STATUS Bit Definitions (Offset 0x0004)

```
Bit  | Name              | Description
-----|-------------------|----------------------------------
0    | PCIE_LINK_UP      | PCIe link is in L0 state
1    | USB_ENUMERATED    | USB 3.0 enumeration complete
2    | CAPTURE_ACTIVE    | TLP capture pipeline running
3    | DDR3_READY        | DDR3 initialized and calibrated
4    | DDR3_OVERFLOW     | DDR3 buffer has overflowed (latched)
5    | INJECT_BUSY       | Injection engine is processing a request
6    | FW_UPDATE_BUSY    | Firmware update in progress
7    | SELF_TEST_BUSY    | Self-test in progress
8    | TEMP_WARNING      | Temperature above 70°C
9    | TEMP_CRITICAL     | Temperature above 85°C
10   | VOLT_3V3_FAULT    | 3.3V rail out of range
11   | VOLT_1V1_FAULT    | 1.1V rail out of range
12   | VOLT_1V35_FAULT   | 1.35V rail out of range
13   | VOLT_1V2_FAULT    | 1.2V rail out of range
14   | VOLT_1V8_FAULT    | 1.8V rail out of range
15   | GOLDEN_IMAGE      | Running from golden (fallback) image
16   | BITSTREAM_ENCRYPTED| Bitstream is AES-128 encrypted
17-31| (Reserved)        | Reserved
```

### 3.3 SCR_CONTROL Bit Definitions (Offset 0x0008)

```
Bit  | Name              | Description
-----|-------------------|----------------------------------
0    | CAPTURE_ENABLE    | Enable TLP capture (1=on, 0=off)
1    | CAPTURE_HOST2DEV  | Capture host-to-device TLPs
2    | CAPTURE_DEV2HOST  | Capture device-to-host TLPs
3    | CAPTURE_CFG_TLP   | Capture configuration TLPs
4    | CAPTURE_MSG_TLP   | Capture message TLPs
5    | CAPTURE_CPL_TLP   | Capture completion TLPs
6    | CAPTURE_MEM_TLP   | Capture memory read/write TLPs
7    | CAPTURE_IO_TLP    | Capture I/O TLPs
8    | FILTER_ENABLE     | Enable TLP filtering
9    | TIMESTAMP_ENABLE  | Enable 48-bit timestamping
10   | DDR3_WRAP_ENABLE  | Enable DDR3 circular buffer wrap
11   | USB_STREAM_ENABLE | Enable USB streaming
12   | INJECT_ENABLE     | Enable TLP injection capability
13   | INJECT_ARMED      | Arm injection trigger (set before INJECT_TRIGGER)
14   | LED_OVERRIDE      | Override automatic LED control
15   | SOFT_RESET        | Trigger soft reset of capture pipeline
16-31| (Reserved)        | Reserved
```

### 3.4 SCR_CAPTURE_FILTER_MASK Bit Definitions (Offset 0x0068)

```
Bit  | TLP Type Filtered
-----|-------------------
0    | Memory Read Request (MRd)
1    | Memory Write Request (MWr)
2    | Memory Read Lock (MRdLk)
3    | I/O Read Request (IORd)
4    | I/O Write Request (IOWr)
5    | Configuration Read Type 0 (CfgRd0)
6    | Configuration Write Type 0 (CfgWr0)
7    | Configuration Read Type 1 (CfgRd1)
8    | Configuration Write Type 1 (CfgWr1)
9    | Message Request (Msg)
10   | Message with Data (MsgD)
11   | Completion (Cpl)
12   | Completion with Data (CplD)
13   | Completion Locked (CplLk)
14   | Completion Locked with Data (CplDLk)
15   | Fetch and Add AtomicOp (FetchAdd)
16   | Unconditional Swap AtomicOp (Swap)
17   | Compare and Swap AtomicOp (CAS)
18-31| (Reserved)
```

Setting a bit to 1 **excludes** that TLP type from capture. Setting all bits to 0 captures everything.

## 4. Clock Initialization

### 4.1 PLL Configuration (ECP5 EHXPLLL)

The ECP5-45F has two PLLs (PLL0 and PLL1 in the sysCLOCK PLL blocks). We use both plus a third from DDR3 hard IP.

**PLL0: 125 MHz System Clock (from PCIe REFCLK 100 MHz)**

```
Input:  PCIE_REFCLK_P/N = 100 MHz differential HCSL
Output: CLKOP = 125 MHz (system clock)
        CLKOS = 125 MHz (PCIe user interface, phase-aligned)
        CLKOK = 25 MHz (auxiliary)

Configuration:
  CLKI_DIV   = 4   (100 MHz / 4 = 25 MHz PFD)
  CLKFB_DIV  = 5   (25 MHz × 5 = 125 MHz VCO)
  CLKOP_DIV  = 8   (1000 MHz VCO / 8 = 125 MHz)
  CLKOS_DIV  = 8   (125 MHz)
  CLKOK_DIV  = 40  (1000 MHz / 40 = 25 MHz)

  VCO = 25 MHz × 5 × 8 = 1000 MHz
  CLKOP = 1000 / 8 = 125 MHz ✓
```

**PLL1: 100 MHz FT601 Clock (from 125 MHz system clock)**

```
Input:  125 MHz (from PLL0 CLKOP, internally routed)
Output: CLKOP = 100 MHz (FT601 FIFO clock)
        CLKOS = 200 MHz (DDR3 user interface clock)

Configuration:
  CLKI_DIV   = 5   (125 MHz / 5 = 25 MHz PFD)
  CLKFB_DIV  = 4   (25 MHz × 4 = 100 MHz VCO)
  CLKOP_DIV  = 8   (800 MHz VCO / 8 = 100 MHz)
  CLKOS_DIV  = 4   (800 MHz / 4 = 200 MHz)

  VCO = 25 MHz × 4 × 8 = 800 MHz
  CLKOP = 800 / 8 = 100 MHz ✓
  CLKOS = 800 / 4 = 200 MHz ✓
```

**DDR3 PLL: 400 MHz DDR3 Controller Clock**

The ECP5 DDR3 hard IP includes its own PLL for the DDR3 PHY. Configured for:

```
Input:  125 MHz (from PLL0 CLKOP)
Output: 400 MHz (DDR3 PHY clock, 2× DDR3-800 data rate)
        DDR3 CK/CK# = 400 MHz (800 MT/s data rate)

The DDR3 hard IP internally multiplies by 16/5:
  125 MHz × 16 / 5 = 400 MHz
```

### 4.2 PLL Lock Sequencing

```
1. Assert PLL reset (PLL0_RST=1, PLL1_RST=1)
2. Wait 100 µs for power stabilization
3. Release PLL0 reset (PLL0_RST=0)
4. Poll PLL0_LOCK until high (timeout: 1 ms)
5. Release PLL1 reset (PLL1_RST=0) — PLL1 uses PLL0 output as reference
6. Poll PLL1_LOCK until high (timeout: 1 ms)
7. Enable DDR3 PLL (via DDR3 hard IP control register)
8. Poll DDR3_PLL_LOCK until high (timeout: 1 ms)
9. All clocks stable → proceed to DDR3 initialization
```

### 4.3 Si5332 Clock Generator Configuration

The Si5332 (U7) provides buffered copies of the PCIe REFCLK. Configuration via I2C (FPGA GPIO_9/GPIO_10):

```
I2C Address: 0x60 (7-bit)

Input:  IN1 (from Host M.2 REFCLKp/n, 100 MHz HCSL)
Outputs:
  OUT0: 100 MHz HCSL → FPGA PCIE_REFCLK_P/N
  OUT1: 100 MHz HCSL → Device M.2 Socket REFCLKp/n
  OUT2: Disabled
  OUT3: Disabled
  OUT4: Disabled
  OUT5: Disabled

Configuration registers (simplified):
  Reg 0x000E: OUT0_FORMAT = 0x01 (HCSL)
  Reg 0x0012: OUT0_DIV = 0x0001 (÷1, 100 MHz)
  Reg 0x002E: OUT1_FORMAT = 0x01 (HCSL)
  Reg 0x0032: OUT1_DIV = 0x0001 (÷1, 100 MHz)
  Reg 0x0040: PLL_M_NUM = 0x0000_0000 (integer mode)
  Reg 0x0044: PLL_M_DEN = 0x0000_0000
```

## 5. GPIO Configuration

### 5.1 GPIO Bank Assignments

The ECP5-45F GPIOs are organized into 8 banks. Each bank has its own VCCIO voltage and can be configured independently.

```
Bank 0 (VCCIO=3.3V): Configuration, LEDs, Debug, Sideband Monitors
Bank 1 (VCCIO=3.3V): FT601 Control Signals
Bank 2 (VCCIO=3.3V): FT601 Data[15:0]
Bank 3 (VCCIO=3.3V): FT601 Data[31:16] + BE[3:0]
Bank 4 (VCCIO=1.2V): PCIe SERDES Lanes 0-1
Bank 5 (VCCIO=1.2V): PCIe SERDES Lanes 2-3
Bank 6 (VCCIO=1.35V): DDR3 Interface (SSTL135)
Bank 7 (VCCIO=1.2V): PCIe REFCLK, Control Signals
```

### 5.2 GPIO Initialization Sequence

```
For each bank:
  1. Wait for VCCIO rail to be stable
  2. Configure I/O type:
     Bank 0,1,2,3: LVCMOS33
     Bank 4,5,7: LVCMOS12 (or HCSL for REFCLK)
     Bank 6: SSTL135
  3. Set drive strength:
     Bank 0,1,2,3: 8 mA (default)
     Bank 4,5,7: 4 mA
     Bank 6: 12 mA (DDR3 requires stronger drive)
  4. Set slew rate:
     Bank 0,1,2,3: Fast
     Bank 4,5,7: Fast
     Bank 6: Fast (DDR3 timing)
  5. Configure pull-up/pull-down:
     Bank 0: Pull-up on CSPI_CSN, INITN, DONE, PROGRAMN
     Bank 1: Pull-up on FT_TXE#, FT_RXF# (open-drain from FT601)
     Bank 6: SSTL135 termination (ODT controlled dynamically)
  6. Set initial output values:
     LED[3:0] = 0 (all off)
     FT_RESET# = 0 (hold FT601 in reset)
     FT_SIWU# = 1 (inactive)
     FT_WR# = 1 (inactive)
     FT_RD# = 1 (inactive)
     FT_OE# = 1 (inactive)
  7. Set output enables:
     LED[3:0] = output
     FT control signals = output
     FT_D[31:0] = bidirectional (controlled by OE#)
     DDR3 signals = per DDR3 controller
```

## 6. Device Drivers

### 6.1 PCIe Driver (drivers/pcie.c)

The PCIe driver manages the ECP5 PCIe Hard IP. Key responsibilities:

**6.1.1 PCIe Hard IP Initialization**

```c
// pcie_init() — Initialize PCIe Hard IP
// Returns: 0 on success, negative error code on failure

int pcie_init(void) {
    uint32_t reg;

    // 1. Assert PCIe Hard IP reset
    PCIE_CTRL_REG = PCIE_CTRL_RESET;

    // 2. Configure link parameters
    PCIE_LINK_WIDTH_REG = PCIE_LINK_X4;
    PCIE_MAX_SPEED_REG = PCIE_SPEED_GEN2;
    PCIE_MAX_PAYLOAD_REG = PCIE_MPS_256;  // 256 bytes max payload

    // 3. Configure vendor/device IDs for the interposer's own config space
    PCIE_VENDOR_ID_REG = 0x1D6B;  // Placeholder (user-configurable)
    PCIE_DEVICE_ID_REG = 0x0001;

    // 4. Configure BAR0 (optional — for interposer's own MMIO space)
    PCIE_BAR0_REG = 0x00000000;  // 4 KB MMIO space
    PCIE_BAR0_MASK_REG = 0xFFFFF000;

    // 5. Enable RX equalization
    PCIE_RX_EQ_REG = PCIE_RX_EQ_ADAPTIVE;

    // 6. Set TX de-emphasis
    PCIE_TX_DEEMPH_REG = PCIE_TX_DEEMPH_MINUS3_5DB;

    // 7. Release reset
    PCIE_CTRL_REG = 0;  // Clear reset

    // 8. Wait for PIPE interface ready
    for (int timeout = 0; timeout < 100000; timeout++) {
        if (PCIE_STATUS_REG & PCIE_STATUS_PIPE_RDY)
            break;
        udelay(1);
    }
    if (!(PCIE_STATUS_REG & PCIE_STATUS_PIPE_RDY))
        return -ETIMEDOUT;

    // 9. Release PERST# to downstream device
    gpio_set(PCIE_PERST_GPIO, 1);  // De-assert PERST#

    // 10. Wait for link training to L0
    for (int timeout = 0; timeout < 100000; timeout++) {
        reg = PCIE_LTSSM_STATE_REG;
        if ((reg & 0x3F) == PCIE_LTSSM_L0)
            break;
        udelay(1);
    }
    if ((PCIE_LTSSM_STATE_REG & 0x3F) != PCIE_LTSSM_L0)
        return -ETIMEDOUT;

    // 11. Read back negotiated parameters
    uint32_t speed = PCIE_LINK_SPEED_REG;
    uint32_t width = PCIE_LINK_WIDTH_REG;

    // 12. Enable TLP reception
    PCIE_RX_TLP_ENABLE_REG = 1;
    PCIE_TX_TLP_ENABLE_REG = 1;

    return 0;
}
```

**6.1.2 TLP Reception (Passive Capture)**

The PCIe Hard IP presents received TLPs on a FIFO interface. The TLP sniffer reads from this FIFO:

```c
// pcie_rx_tlp_read() — Read a received TLP from the RX FIFO
// buf: output buffer (must be at least 4096 bytes for max TLP)
// Returns: number of bytes read, or negative error

int pcie_rx_tlp_read(uint8_t *buf, uint16_t max_len) {
    uint32_t header[4];  // TLP header is 3 or 4 DW (12 or 16 bytes)
    uint16_t length_dw;
    uint16_t total_bytes;

    // Check if TLP available
    if (!(PCIE_RX_FIFO_STATUS_REG & PCIE_RX_FIFO_NOT_EMPTY))
        return 0;  // No TLP available

    // Read TLP header (first 4 DW)
    for (int i = 0; i < 4; i++) {
        header[i] = PCIE_RX_FIFO_DATA_REG;
    }

    // Parse header to determine length
    uint8_t fmt = (header[0] >> 29) & 0x3;
    uint8_t type = (header[0] >> 24) & 0x1F;
    length_dw = (header[0] >> 0) & 0x3FF;  // 10-bit length field

    // Calculate total TLP size
    uint8_t header_dw = (fmt == 0x3) ? 4 : 3;  // 4DW header if fmt=3
    total_bytes = (header_dw + length_dw) * 4;

    if (total_bytes > max_len)
        return -ENOSPC;

    // Copy header to output buffer
    memcpy(buf, header, header_dw * 4);

    // Read data payload (if any)
    for (int i = header_dw; i < header_dw + length_dw; i++) {
        uint32_t dw = PCIE_RX_FIFO_DATA_REG;
        memcpy(buf + i * 4, &dw, 4);
    }

    // Read LCRC (1 DW, not included in length)
    uint32_t lcrc = PCIE_RX_FIFO_DATA_REG;
    // Optionally verify LCRC here

    return total_bytes;
}
```

**6.1.3 TLP Transmission (Active Injection)**

```c
// pcie_tx_tlp_inject() — Inject a TLP onto the PCIe link
// tlp_type: MRd=0, MWr=1, CfgRd0=4, CfgWr0=5, CfgRd1=6, CfgWr1=7, Msg=10, MsgD=11
// requester_id: 16-bit bus/device/function
// tag: 8-bit tag for non-posted requests
// address: 64-bit target address (for memory/configuration TLPs)
// data: payload data (for MWr, MsgD, CfgWr)
// length_dw: data length in DW (1-1024)
// Returns: 0 on success, negative error

int pcie_tx_tlp_inject(uint8_t tlp_type, uint16_t requester_id,
                        uint8_t tag, uint64_t address,
                        const uint8_t *data, uint16_t length_dw) {
    uint32_t header[4];
    uint8_t fmt, type;

    // Determine Fmt and Type fields
    switch (tlp_type) {
    case TLP_MRD:  // Memory Read Request
        if (address > 0xFFFFFFFF) {
            fmt = 0x1;  // 4DW header (64-bit address)
            type = 0x00;
        } else {
            fmt = 0x0;  // 3DW header (32-bit address)
            type = 0x00;
        }
        break;
    case TLP_MWR:  // Memory Write Request
        if (address > 0xFFFFFFFF) {
            fmt = 0x3;  // 4DW header with data
            type = 0x01;
        } else {
            fmt = 0x2;  // 3DW header with data
            type = 0x01;
        }
        break;
    case TLP_CFGRD0:  // Configuration Read Type 0
        fmt = 0x0; type = 0x04;
        break;
    case TLP_CFGWR0:  // Configuration Write Type 0
        fmt = 0x2; type = 0x05;
        break;
    case TLP_CFGRD1:  // Configuration Read Type 1
        fmt = 0x0; type = 0x06;
        break;
    case TLP_CFGWR1:  // Configuration Write Type 1
        fmt = 0x2; type = 0x07;
        break;
    case TLP_MSG:     // Message (no data)
        fmt = 0x0; type = 0x10;
        break;
    case TLP_MSGD:    // Message with Data
        fmt = 0x2; type = 0x11;
        break;
    default:
        return -EINVAL;
    }

    // Build header DW0
    header[0] = (fmt << 29) | (type << 24) |
                (0 << 22) |  // TC = 0
                (0 << 18) |  // Attr = 0
                (0 << 16) |  // TH = 0
                (0 << 15) |  // TD = 0 (no TLP digest)
                (0 << 14) |  // EP = 0
                (0 << 12) |  // Attr2 = 0
                (length_dw & 0x3FF);

    // Build header DW1
    header[1] = (requester_id << 16) |
                (tag << 8) |
                (0 << 0);  // Last DW BE / First DW BE (for memory)

    // Build header DW2 (address low)
    header[2] = (uint32_t)(address & 0xFFFFFFFF);

    // Build header DW3 (address high, only for 4DW headers)
    if (fmt == 0x1 || fmt == 0x3) {
        header[3] = (uint32_t)((address >> 32) & 0xFFFFFFFF);
    }

    // Wait for TX FIFO space
    int timeout = 1000;
    while (!(PCIE_TX_FIFO_STATUS_REG & PCIE_TX_FIFO_NOT_FULL)) {
        if (--timeout <= 0) return -ETIMEDOUT;
        udelay(1);
    }

    // Write header to TX FIFO
    int header_dw = (fmt == 0x1 || fmt == 0x3) ? 4 : 3;
    for (int i = 0; i < header_dw; i++) {
        PCIE_TX_FIFO_DATA_REG = header[i];
    }

    // Write data payload (if any)
    if (fmt == 0x2 || fmt == 0x3) {
        for (int i = 0; i < length_dw; i++) {
            uint32_t dw;
            memcpy(&dw, data + i * 4, 4);
            PCIE_TX_FIFO_DATA_REG = dw;
        }
    }

    // LCRC is automatically appended by the PCIe Hard IP

    // For non-posted requests (MRd, CfgRd), wait for completion
    if (tlp_type == TLP_MRD || tlp_type == TLP_CFGRD0 || tlp_type == TLP_CFGRD1) {
        // Completion will arrive on RX FIFO — handled by sniffer or dedicated handler
    }

    return 0;
}
```

**6.1.4 LCRC Calculation (Parallel CRC-32)**

The PCIe LCRC uses the Ethernet CRC-32 polynomial (0x04C11DB7) but calculated over the TLP header and data only (not including framing symbols). The FPGA implements this in hardware using a parallel CRC-32 generator:

```c
// lcrc_calculate() — Calculate PCIe LCRC over TLP bytes
// data: TLP header + data payload (without framing)
// len_bytes: total length in bytes
// Returns: 32-bit LCRC value

uint32_t lcrc_calculate(const uint8_t *data, uint16_t len_bytes) {
    uint32_t crc = 0xFFFFFFFF;  // Initial value (inverted)

    for (uint16_t i = 0; i < len_bytes; i++) {
        crc ^= (uint32_t)data[i] << 24;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return ~crc;  // Final inversion
}
```

### 6.2 DDR3 Driver (drivers/ddr3.c)

The DDR3 driver manages the MT41K256M16TW-107 DDR3L SDRAM through the ECP5 DDR3 hard IP.

**6.2.1 DDR3 Initialization**

```c
// ddr3_init() — Initialize DDR3 SDRAM
// Returns: 0 on success, negative error

int ddr3_init(void) {
    uint32_t reg;

    // 1. Assert DDR3 reset
    DDR3_CTRL_REG = DDR3_CTRL_RESET;
    gpio_set(DDR3_RESET_GPIO, 0);  // Drive RESET# low
    udelay(200);

    // 2. Release DDR3 reset
    gpio_set(DDR3_RESET_GPIO, 1);  // Drive RESET# high
    udelay(500);  // Wait for stabilization

    // 3. Configure DDR3 controller parameters
    DDR3_MRS0_REG = DDR3_MRS0_CL6 | DDR3_MRS0_WR8 | DDR3_MRS0_DLL_RESET |
                    DDR3_MRS0_BT_SEQ | DDR3_MRS0_BL8;
    DDR3_MRS1_REG = DDR3_MRS1_DIC_34 | DDR3_MRS1_RTT_40 |
                    DDR3_MRS1_ODS_34 | DDR3_MRS1_DLL_EN;
    DDR3_MRS2_REG = DDR3_MRS2_CWL5 | DDR3_MRS2_PASR_FULL |
                    DDR3_MRS2_ASR_NORMAL | DDR3_MRS2_SRT_EXTENDED;
    DDR3_MRS3_REG = DDR3_MRS3_MPR_NORMAL;

    // 4. Configure timing parameters (for 800 MHz clock, tCK=2.5ns)
    DDR3_TIMING_TRC_REG   = 24;  // tRC = 60ns / 2.5ns = 24 clocks
    DDR3_TIMING_TRAS_REG  = 14;  // tRAS = 35ns / 2.5ns = 14 clocks
    DDR3_TIMING_TRP_REG   = 6;   // tRP = 15ns / 2.5ns = 6 clocks
    DDR3_TIMING_TRCD_REG  = 6;   // tRCD = 15ns / 2.5ns = 6 clocks
    DDR3_TIMING_TWR_REG   = 6;   // tWR = 15ns / 2.5ns = 6 clocks
    DDR3_TIMING_TWTR_REG  = 4;   // tWTR = 7.5ns / 2.5ns = 3 → round up to 4
    DDR3_TIMING_TRTP_REG  = 4;   // tRTP = 7.5ns / 2.5ns = 3 → round up to 4
    DDR3_TIMING_TFAW_REG  = 16;  // tFAW = 40ns / 2.5ns = 16 clocks
    DDR3_TIMING_TRRD_REG  = 4;   // tRRD = 10ns / 2.5ns = 4 clocks
    DDR3_TIMING_TCCD_REG  = 4;   // tCCD = 4 clocks (BL8)
    DDR3_TIMING_TREFI_REG = 3120; // tREFI = 7.8µs / 2.5ns = 3120 clocks
    DDR3_TIMING_TRFC_REG  = 104; // tRFC = 260ns / 2.5ns = 104 clocks

    // 5. Release DDR3 controller reset
    DDR3_CTRL_REG = 0;

    // 6. Wait for DDR3 controller ready
    for (int timeout = 0; timeout < 100000; timeout++) {
        if (DDR3_STATUS_REG & DDR3_STATUS_READY)
            break;
        udelay(1);
    }
    if (!(DDR3_STATUS_REG & DDR3_STATUS_READY))
        return -ETIMEDOUT;

    // 7. Perform read/write leveling calibration
    ddr3_calibrate();

    return 0;
}
```

**6.2.2 DDR3 Calibration**

```c
// ddr3_calibrate() — Perform DDR3 read/write leveling
// Returns: 0 on success

int ddr3_calibrate(void) {
    // Write leveling: adjust DQS-to-CK phase per byte lane
    DDR3_CAL_CTRL_REG = DDR3_CAL_WRITE_LEVELING;
    while (!(DDR3_CAL_STATUS_REG & DDR3_CAL_DONE))
        udelay(1);

    // Read leveling: adjust DQS gate timing per byte lane
    DDR3_CAL_CTRL_REG = DDR3_CAL_READ_LEVELING;
    while (!(DDR3_CAL_STATUS_REG & DDR3_CAL_DONE))
        udelay(1);

    // Read calibration results
    uint8_t dqs_delay_0 = DDR3_CAL_RESULT0_REG & 0xFF;
    uint8_t dqs_delay_1 = DDR3_CAL_RESULT1_REG & 0xFF;

    return 0;
}
```

**6.2.3 DDR3 Write DMA (from TLP Sniffer)**

```c
// ddr3_write_dma() — Write TLP data to DDR3 circular buffer
// data: TLP record (header + metadata + raw TLP)
// len_bytes: total record length
// Returns: 0 on success, -ENOSPC if buffer full

int ddr3_write_dma(const uint8_t *data, uint16_t len_bytes) {
    uint32_t write_ptr, read_ptr, buffer_size;
    uint32_t space_available;

    // Read current pointers
    write_ptr = DDR3_WRITE_PTR_REG;
    read_ptr = DDR3_READ_PTR_REG;
    buffer_size = DDR3_BUFFER_SIZE_REG;  // 512 MB = 0x20000000

    // Calculate available space (circular buffer)
    if (write_ptr >= read_ptr) {
        space_available = buffer_size - (write_ptr - read_ptr);
    } else {
        space_available = read_ptr - write_ptr;
    }

    // Need room for record + 4-byte length prefix
    uint32_t needed = len_bytes + 4;
    if (needed > space_available) {
        DDR3_OVERFLOW_COUNT_REG++;
        return -ENOSPC;
    }

    // Write length prefix
    DDR3_WRITE_DATA_REG = len_bytes;

    // Write data in 32-bit words
    uint16_t words = (len_bytes + 3) / 4;
    for (uint16_t i = 0; i < words; i++) {
        uint32_t word = 0;
        uint8_t bytes_this_word = (i == words - 1) ? (len_bytes % 4) : 4;
        if (bytes_this_word == 0) bytes_this_word = 4;
        memcpy(&word, data + i * 4, bytes_this_word);
        DDR3_WRITE_DATA_REG = word;
    }

    // Update write pointer
    write_ptr = (write_ptr + needed) % buffer_size;
    DDR3_WRITE_PTR_REG = write_ptr;

    return 0;
}
```

**6.2.4 DDR3 Read DMA (to USB Egress)**

```c
// ddr3_read_dma() — Read TLP data from DDR3 circular buffer
// buf: output buffer
// max_len: maximum bytes to read
// Returns: number of bytes read, 0 if buffer empty

int ddr3_read_dma(uint8_t *buf, uint16_t max_len) {
    uint32_t write_ptr, read_ptr, buffer_size;
    uint32_t available;

    // Read current pointers
    write_ptr = DDR3_WRITE_PTR_REG;
    read_ptr = DDR3_READ_PTR_REG;
    buffer_size = DDR3_BUFFER_SIZE_REG;

    // Calculate available data
    if (write_ptr >= read_ptr) {
        available = write_ptr - read_ptr;
    } else {
        available = buffer_size - read_ptr + write_ptr;
    }

    if (available < 4)  // Need at least length prefix
        return 0;

    // Read length prefix
    uint32_t record_len = DDR3_READ_DATA_REG;
    if (record_len > max_len)
        return -ENOSPC;

    // Read data in 32-bit words
    uint16_t words = (record_len + 3) / 4;
    for (uint16_t i = 0; i < words; i++) {
        uint32_t word = DDR3_READ_DATA_REG;
        uint8_t bytes_this_word = (i == words - 1) ? (record_len % 4) : 4;
        if (bytes_this_word == 0) bytes_this_word = 4;
        memcpy(buf + i * 4, &word, bytes_this_word);
    }

    // Update read pointer
    read_ptr = (read_ptr + record_len + 4) % buffer_size;
    DDR3_READ_PTR_REG = read_ptr;

    return record_len;
}
```

### 6.3 USB Egress Driver (drivers/usb_ft601.c)

**6.3.1 FT601 Initialization**

```c
// ft601_init() — Initialize FT601 USB 3.0 FIFO bridge
// Returns: 0 on success, negative error

int ft601_init(void) {
    // 1. Assert reset
    gpio_set(FT_RESET_GPIO, 0);
    udelay(1000);  // 1 ms

    // 2. Start 100 MHz clock
    ft601_clock_start();  // Enable PLL1 CLKOP output to FT_CLK pin

    // 3. Release reset
    gpio_set(FT_RESET_GPIO, 1);
    udelay(10000);  // 10 ms for FT601 internal init

    // 4. Wait for USB enumeration
    // FT601 drives SUSPEND# low when enumerated and active
    int timeout = 5000000;  // 5 seconds in µs
    while (gpio_get(FT_SUSPEND_GPIO) == 1) {
        if (--timeout <= 0)
            return -ETIMEDOUT;
        udelay(1);
    }

    // 5. Configure FIFO mode
    // FT601 is already in 32-bit synchronous FIFO mode by default
    // FT_WR#, FT_RD#, FT_OE#, FT_SIWU# are ready

    return 0;
}
```

**6.3.2 USB Data Transmission**

```c
// ft601_tx_packet() — Transmit a packet over USB
// data: packet data
// len_bytes: packet length (must be multiple of 4 for 32-bit FIFO)
// Returns: 0 on success, negative error

int ft601_tx_packet(const uint8_t *data, uint16_t len_bytes) {
    uint16_t words = len_bytes / 4;

    // Wait for TXE# low (FIFO not empty = space available)
    int timeout = 10000;
    while (gpio_get(FT_TXE_GPIO) == 1) {
        if (--timeout <= 0)
            return -ETIMEDOUT;
        udelay(1);
    }

    // Assert OE# low (enable FPGA outputs)
    gpio_set(FT_OE_GPIO, 0);

    // Write data words
    for (uint16_t i = 0; i < words; i++) {
        // Wait for TXE# low before each word
        timeout = 1000;
        while (gpio_get(FT_TXE_GPIO) == 1) {
            if (--timeout <= 0) {
                gpio_set(FT_OE_GPIO, 1);
                return -ETIMEDOUT;
            }
        }

        // Place data on bus
        uint32_t word;
        memcpy(&word, data + i * 4, 4);
        FT_DATA_OUT_REG = word;

        // Assert WR# low (write strobe)
        gpio_set(FT_WR_GPIO, 0);
        udelay(1);  // 10 ns min pulse width, 1 µs is safe
        gpio_set(FT_WR_GPIO, 1);
    }

    // De-assert OE#
    gpio_set(FT_OE_GPIO, 1);

    return 0;
}
```

**6.3.3 USB Data Reception**

```c
// ft601_rx_packet() — Receive a packet from USB
// buf: output buffer
// max_len: maximum bytes to receive
// Returns: number of bytes received, 0 if no data, negative error

int ft601_rx_packet(uint8_t *buf, uint16_t max_len) {
    uint16_t words_received = 0;

    // Check if data available (RXF# low = FIFO has data)
    if (gpio_get(FT_RXF_GPIO) == 1)
        return 0;  // No data

    // Assert OE# low (enable FPGA outputs — actually FPGA is reading, so OE# is for FT601)
    // For reads: OE# low, RD# pulsed
    gpio_set(FT_OE_GPIO, 0);

    while (gpio_get(FT_RXF_GPIO) == 0 && words_received < max_len / 4) {
        // Assert RD# low (read strobe)
        gpio_set(FT_RD_GPIO, 0);
        udelay(1);  // Wait for data to appear
        gpio_set(FT_RD_GPIO, 1);

        // Read data from bus
        uint32_t word = FT_DATA_IN_REG;
        memcpy(buf + words_received * 4, &word, 4);
        words_received++;
    }

    gpio_set(FT_OE_GPIO, 1);

    return words_received * 4;
}
```

## 7. USB Wire Protocol

### 7.1 Frame Format

All communication between the companion app and the PCIe Screamer uses a binary framed protocol over the FT601 USB 3.0 FIFO channel.

```
Frame Structure:
┌──────────────────────────────────────────────────────────────┐
│  SYNC (6 bytes)  │  CMD (1)  │  LEN (2)  │  PAYLOAD (0-4096) │  CRC32 (4)  │
│  0x53 0x43 0x52  │           │           │                    │             │
│  0x45 0x41 0x4D  │           │           │                    │             │
│  "SCREAM"         │           │           │                    │             │
└──────────────────────────────────────────────────────────────┘

Total frame: 13 + PAYLOAD_LEN bytes
Maximum frame: 4109 bytes (13 header + 4096 payload)
```

### 7.2 Command IDs

```
CMD    | Name                  | Direction        | Description
-------|-----------------------|------------------|----------------------------------
0x01   | CMD_CAPTURE_START     | Host → Device    | Start TLP capture
0x02   | CMD_CAPTURE_STOP      | Host → Device    | Stop TLP capture
0x03   | CMD_CAPTURE_DATA      | Device → Host    | Captured TLP data packet
0x04   | CMD_CAPTURE_OVERFLOW  | Device → Host    | Buffer overflow notification
0x10   | CMD_INJECT_TLP        | Host → Device    | Inject a TLP
0x11   | CMD_INJECT_RESULT     | Device → Host    | Injection result
0x20   | CMD_QUERY_STATUS      | Host → Device    | Query device status
0x21   | CMD_STATUS_RESPONSE   | Device → Host    | Status response
0x22   | CMD_READ_REG          | Host → Device    | Read MMIO register
0x23   | CMD_READ_REG_RESP     | Device → Host    | Register read response
0x24   | CMD_WRITE_REG         | Host → Device    | Write MMIO register
0x25   | CMD_WRITE_REG_ACK     | Device → Host    | Register write acknowledge
0x30   | CMD_FW_UPDATE_INIT    | Host → Device    | Initiate firmware update
0x31   | CMD_FW_UPDATE_DATA    | Host → Device    | Firmware update data block
0x32   | CMD_FW_UPDATE_CRC     | Device → Host    | Firmware update CRC response
0x33   | CMD_FW_UPDATE_COMMIT  | Host → Device    | Commit firmware update
0x34   | CMD_FW_UPDATE_RESULT  | Device → Host    | Firmware update result
0x40   | CMD_SELF_TEST_START   | Host → Device    | Start built-in self-test
0x41   | CMD_SELF_TEST_RESULT  | Device → Host    | Self-test result
0xF0   | CMD_ERROR             | Device → Host    | Error notification
0xF1   | CMD_ACK               | Either direction | Generic acknowledge
0xF2   | CMD_NACK              | Either direction | Generic negative acknowledge
0xFF   | CMD_RESET             | Host → Device    | Soft reset device
```

### 7.3 Payload Formats

**CMD_CAPTURE_START (0x01) — Host → Device**

```
Offset | Size | Field
-------|------|---------------------------
0      | 4    | Filter mask (see SCR_CAPTURE_FILTER_MASK)
4      | 8    | Address filter (64-bit, 0 = disabled)
12     | 2    | Requester ID filter (0 = disabled)
14     | 1    | Flags:
       |      |   Bit 0: Capture host→device
       |      |   Bit 1: Capture device→host
       |      |   Bit 2: Enable timestamping
       |      |   Bit 3: Enable DDR3 buffering
       |      |   Bit 4: Enable USB streaming
```

**CMD_CAPTURE_DATA (0x03) — Device → Host**

```
Offset | Size | Field
-------|------|---------------------------
0      | 6    | Timestamp (48-bit, 8 ns resolution)
6      | 1    | Direction (0=Host→Dev, 1=Dev→Host)
7      | 1    | TLP Type (Fmt[1:0] + Type[4:0])
8      | 2    | TLP Length (bytes, including header)
10     | 2    | Requester ID
12     | 1    | Tag
13     | 1    | Traffic Class
14     | N    | Raw TLP bytes (header + data + LCRC)
```

**CMD_INJECT_TLP (0x10) — Host → Device**

```
Offset | Size | Field
-------|------|---------------------------
0      | 1    | TLP Type (0=MRd, 1=MWr, 4=CfgRd0, 5=CfgWr0, 6=CfgRd1, 7=CfgWr1)
1      | 2    | Requester ID
3      | 1    | Tag
4      | 8    | Address (64-bit)
12     | 2    | Data length (DW, 0 for reads)
14     | 1    | Traffic Class
15     | N    | Data payload (only for writes, length × 4 bytes)
```

**CMD_INJECT_RESULT (0x11) — Device → Host**

```
Offset | Size | Field
-------|------|---------------------------
0      | 1    | Status (0=Success, 1=Timeout, 2=UR, 3=CA, 4=CRS)
1      | 2    | Completion length (bytes, 0 if error)
3      | N    | Completion data (if success and read request)
```

**CMD_QUERY_STATUS (0x20) — Host → Device**

```
No payload (empty frame, LEN=0)
```

**CMD_STATUS_RESPONSE (0x21) — Device → Host**

```
Offset | Size | Field
-------|------|---------------------------
0      | 4    | SCR_STATUS register
4      | 4    | SCR_CAPTURE_COUNT
8      | 4    | SCR_DDR3_WRITE_PTR
12     | 4    | SCR_DDR3_READ_PTR
16     | 4    | SCR_DDR3_OVERFLOW_COUNT
20     | 4    | SCR_USB_TX_BYTES
24     | 4    | SCR_USB_RX_BYTES
28     | 4    | SCR_PCIE_LTSSM_STATE
32     | 4    | SCR_PCIE_LINK_SPEED
36     | 4    | SCR_PCIE_LINK_WIDTH
40     | 4    | SCR_TEMP_SENSOR
44     | 4    | SCR_VOLT_MON_3V3
48     | 4    | SCR_VOLT_MON_1V1
52     | 4    | SCR_VOLT_MON_1V35
56     | 4    | SCR_VOLT_MON_1V2
60     | 4    | SCR_VOLT_MON_1V8
```

**CMD_READ_REG (0x22) — Host → Device**

```
Offset | Size | Field
-------|------|---------------------------
0      | 4    | Register address (32-bit offset from base)
```

**CMD_READ_REG_RESP (0x23) — Device → Host**

```
Offset | Size | Field
-------|------|---------------------------
0      | 4    | Register address
4      | 4    | Register value
```

**CMD_WRITE_REG (0x24) — Host → Device**

```
Offset | Size | Field
-------|------|---------------------------
0      | 4    | Register address
4      | 4    | Register value
```

**CMD_FW_UPDATE_INIT (0x30) — Host → Device**

```
Offset | Size | Field
-------|------|---------------------------
0      | 4    | Total image size (bytes)
4      | 4    | Expected CRC-32C
8      | 4    | Target flash address (0x600000 for update slot)
```

**CMD_FW_UPDATE_DATA (0x31) — Host → Device**

```
Offset | Size | Field
-------|------|---------------------------
0      | 4    | Block offset (bytes from start of image)
4      | 4    | Block length (bytes, max 4080)
8      | N    | Block data
```

### 7.4 CRC-32C (Castagnoli)

The wire protocol uses CRC-32C (Castagnoli) with polynomial 0x1EDC6F41. This is the same CRC used by iSCSI, SCTP, and Btrfs.

```c
// crc32c_calculate() — Calculate CRC-32C over data
// data: input bytes
// len: number of bytes
// Returns: 32-bit CRC value

uint32_t crc32c_calculate(const uint8_t *data, uint16_t len) {
    uint32_t crc = 0xFFFFFFFF;

    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0x82F63B78;  // Reflected polynomial
            } else {
                crc = (crc >> 1);
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}
```

## 8. Build Instructions

### 8.1 FPGA Gateware Build (Verilog → Bitstream)

**Toolchain:** Lattice Radiant Software (Version 3.2 or later)

```bash
# 1. Set up Radiant environment
source /opt/lattice/radiant/3.2/bin/lin64/radiant_env.sh

# 2. Create project
radiant create_project pcie_screamer \
    --family "Lattice ECP5" \
    --device "LFE5U-45F-6BG381C" \
    --synthesis "Synplify Pro" \
    --implementation "Lattice Radiant"

# 3. Add source files
radiant add_source top_screamer.v
radiant add_source pcie_hard_ip_wrapper.v
radiant add_source tlp_sniffer.v
radiant add_source tlp_injector.v
radiant add_source ddr3_controller.v
radiant add_source usb_egress_engine.v
radiant add_source clock_reset_manager.v

# 4. Add constraint files
radiant add_constraint pcie_screamer.ldc  # Lattice Design Constraints
radiant add_constraint pcie_screamer.pdc  # Physical Design Constraints

# 5. Run synthesis
radiant run_synthesis

# 6. Run implementation (place & route)
radiant run_implementation

# 7. Generate bitstream
radiant generate_bitstream \
    --output pcie_screamer.bit \
    --compress \
    --encrypt --key_file efuse_key.txt  # Optional AES-128 encryption

# 8. Generate SPI flash programming file
radiant generate_prom \
    --input pcie_screamer.bit \
    --output pcie_screamer.mcs \
    --format MCS \
    --spi_flash W25Q128JV
```

### 8.2 SPI Flash Programming

```bash
# Using Lattice Diamond Programmer (or open-source ecpprog)
# Option A: Lattice Programmer (with HW-USBN-2B cable)
pgrcmd -cable HW-USBN-2B -infile pcie_screamer.mcs -program

# Option B: ecpprog (open-source, via FTDI or compatible cable)
ecpprog -o 0x000000 pcie_screamer.bit

# Option C: Program via USB on the device itself (using FW_UPDATE commands)
# The companion app can send CMD_FW_UPDATE_INIT/DATA/COMMIT to program flash
```

### 8.3 Embedded C Firmware Build (if using soft CPU)

**Toolchain:** RISC-V GCC (for LatticeMico32 or VexRiscv soft CPU) or LM32 toolchain

```bash
# Using VexRiscv soft CPU on ECP5
cd firmware/

# Build
make CROSS_COMPILE=riscv32-unknown-elf- all

# Outputs:
#   build/firmware.elf   — ELF executable
#   build/firmware.bin   — Binary image
#   build/firmware.hex   — Intel HEX for FPGA BRAM initialization
```

**Makefile:**

```makefile
# Makefile for PCIe Screamer firmware
CROSS_COMPILE ?= riscv32-unknown-elf-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

CFLAGS = -march=rv32imc -mabi=ilp32 -Os -ffreestanding \
         -Wall -Wextra -Werror \
         -I. -Idrivers \
         -DF_CPU=125000000UL

LDFLAGS = -T linker.ld -nostdlib -nostartfiles

SRCS = main.c \
       drivers/pcie.c \
       drivers/ddr3.c \
       drivers/usb_ft601.c \
       drivers/tlp_sniffer.c \
       drivers/tlp_injector.c

OBJS = $(SRCS:.c=.o)
TARGET = build/firmware

.PHONY: all clean

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex
```

### 8.4 Companion App Build (React Native)

```bash
cd app/

# Install dependencies
npm install

# Run on development machine (with USB connected)
npx react-native start

# Build for Android
npx react-native run-android

# Build for iOS
npx react-native run-ios

# Build for desktop (React Native Windows/macOS)
npx react-native run-windows
npx react-native run-macos
```

## 9. Device Tree / Hardware Description

### 9.1 Conceptual Device Tree (for documentation)

Although the PCIe Screamer doesn't run Linux, this device tree describes the hardware topology for reference:

```dts
/dts-v1/;

/ {
    model = "jayis1 PCIe Screamer";
    compatible = "nous,pcie-screamer";
    #address-cells = <1>;
    #size-cells = <1>;

    fpga@0 {
        compatible = "lattice,ecp5-45f";
        reg = <0x00000000 0x08000000>;  // 128 MB address space

        clocks {
            clk_125mhz: system_clk {
                compatible = "fixed-clock";
                clock-frequency = <125000000>;
            };
            clk_100mhz: usb_clk {
                compatible = "fixed-clock";
                clock-frequency = <100000000>;
            };
            clk_400mhz: ddr3_clk {
                compatible = "fixed-clock";
                clock-frequency = <400000000>;
            };
        };

        pcie@0x10000000 {
            compatible = "lattice,ecp5-pcie";
            reg = <0x10000000 0x00010000>;
            link-width = <4>;
            max-link-speed = <2>;  // Gen2
        };

        ddr3@0x20000000 {
            compatible = "micron,mt41k256m16";
            reg = <0x20000000 0x20000000>;  // 512 MB
            data-width = <16>;
            clock-frequency = <800>;  // 800 MT/s
        };

        usb@0x30000000 {
            compatible = "ftdi,ft601";
            reg = <0x30000000 0x00001000>;
            fifo-width = <32>;
            fifo-clock = <100000000>;
        };

        spi@0x40000000 {
            compatible = "winbond,w25q128jv";
            reg = <0x40000000 0x01000000>;  // 16 MB
            spi-max-frequency = <104000000>;
        };

        gpio@0x50000000 {
            compatible = "lattice,ecp5-gpio";
            reg = <0x50000000 0x00001000>;
            gpio-controller;
            #gpio-cells = <2>;

            leds {
                led0: led-pcie-link {
                    gpios = <&gpio 0 0>;
                    label = "pcie_link";
                };
                led1: led-usb-enum {
                    gpios = <&gpio 1 0>;
                    label = "usb_enum";
                };
                led2: led-capture {
                    gpios = <&gpio 2 0>;
                    label = "capture_active";
                };
                led3: led-error {
                    gpios = <&gpio 3 0>;
                    label = "error";
                };
            };
        };
    };
};
```

## 10. Error Handling & Recovery

### 10.1 Error Categories

| Category | Examples | Recovery Strategy |
|----------|----------|-------------------|
| PCIe Link Errors | Link down, training failure, CRC errors | Retry training, fallback to Gen1, notify host |
| DDR3 Errors | Calibration failure, ECC errors, refresh timeout | Recalibrate, mark bad region, notify host |
| USB Errors | Enumeration failure, FIFO overflow/underflow | Reset FT601, re-enumerate |
| Buffer Overflow | DDR3 circular buffer full | Drop oldest data, increment overflow counter, notify host |
| Power Faults | Voltage rail out of range | Halt capture, set error LED, notify host |
| Thermal Faults | Temperature > 85°C | Throttle capture rate, notify host, emergency halt at 95°C |
| Configuration Errors | Bitstream CRC failure | Fall back to golden image, notify host |
| Injection Errors | UR/CA/CRS completion status | Report to host, do not retry automatically |

### 10.2 Watchdog Timer

A hardware watchdog timer in the FPGA resets the device if the main state machine hangs:

```
Watchdog period: 1 second
Pet mechanism: Write any value to SCR_WATCHDOG_KICK register
If watchdog expires: Assert global reset, reload from golden image
```

---

*End of Phase 4 — Software Stack. This document covers the complete boot strategy, MMIO register map, clock initialization, GPIO configuration, device drivers with DMA for PCIe/DDR3/USB, the USB wire protocol with CRC-32C, build instructions for FPGA gateware and firmware, and a hardware device tree for the PCIe Screamer.*
