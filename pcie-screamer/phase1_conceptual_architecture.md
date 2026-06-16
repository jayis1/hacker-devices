# Phase 1: Conceptual Architecture — PCIe Screamer

## 1. Device Identity & Mission

**Device Name:** PCIe Screamer  
**Codename:** SCREAMER-M2  
**Classification:** PCI Express Gen2/Gen3 x4 NVMe Interposer & Traffic Analyzer  
**Primary Use Case:** Passive, transparent interception of all PCIe Transaction Layer Packets (TLPs) flowing between a host motherboard and an NVMe M.2 SSD, with optional active DMA injection capability for security research, vulnerability assessment, and red-team operations.

**Mission Statement:** To provide security researchers with a completely transparent, undetectable PCIe bus-level man-in-the-middle platform capable of full-duplex traffic capture at line rate (up to 20 GT/s aggregate across 4 Gen2 lanes), real-time TLP decoding, and controlled injection of crafted Memory Read/Write TLPs for DMA attack simulation — all in a self-contained M.2 2280 interposer form factor that requires no host software installation.

## 2. Attack Surface & Threat Model

### 2.1 Passive Capture Surface
- **NVMe Admin Submission Queues (ASQ):** Captures all admin commands including Identify, Get Features, Set Features, Format NVM, Firmware Commit/Download, Sanitize, and Namespace Management.
- **NVMe I/O Submission Queues (IOSQ):** Captures all read/write commands, their PRP/SGL scatter-gather lists, and associated metadata.
- **NVMe Completion Queues (CQ):** Captures command completion status, phase tag toggles, and SQ head pointer updates.
- **PCIe Configuration Space:** Captures all Type 0/Type 1 configuration read/write TLPs, BAR programming, MSI/MSI-X capability writes, AER (Advanced Error Reporting) registers, and PCIe capability structure enumeration.
- **PCIe Message TLPs:** Captures INTx emulation messages, PME messages, error messages, vendor-defined messages, and power management state transitions.
- **Memory-Mapped I/O (MMIO):** Captures all host-to-device MMIO reads/writes targeting the NVMe controller's BAR0/1 registers — doorbell writes, SQ/CQ tail/head pointer updates, interrupt mask sets/clears, and controller configuration registers.
- **DMA Transactions:** Captures all device-to-host memory read/write completions, revealing exactly which host physical memory pages the NVMe controller accesses during data transfers.

### 2.2 Active Injection Surface
- **DMA Read Injection:** Craft and inject Memory Read Request TLPs (MRd) targeting arbitrary host physical addresses. The host PCIe root complex will respond with Completion with Data (CplD) TLPs containing the contents of the targeted memory — enabling arbitrary physical memory read without host cooperation.
- **DMA Write Injection:** Craft and inject Memory Write Request TLPs (MWr) with arbitrary data payloads to arbitrary host physical addresses — enabling kernel memory corruption, page table manipulation, and code injection.
- **Configuration Space Injection:** Craft and inject Configuration Read/Write TLPs targeting downstream devices or the root complex itself.
- **I/O Space Injection:** Craft and inject I/O Read/Write TLPs for legacy device interaction.
- **Message Injection:** Craft and inject Message TLPs to trigger interrupts, power management events, or error conditions on the host.

### 2.3 Threat Actor Profiles
- **Red Team Operator:** Uses passive capture to map NVMe driver behavior, identify DMA patterns, and locate sensitive data in transit. Uses active injection to demonstrate DMA attack feasibility during authorized engagements.
- **Vulnerability Researcher:** Uses passive capture to reverse-engineer NVMe firmware behavior, identify IOMMU bypass opportunities, and discover DMA remapping gaps. Uses active injection to validate proof-of-concept exploits.
- **Forensic Analyst:** Uses passive capture to record all storage I/O during incident response, capturing data that never touches the filesystem layer (encrypted volumes, temporary buffers, swap).
- **Hardware Security Auditor:** Uses passive capture to verify that IOMMU/DMA remapping is correctly configured, that ACS (Access Control Services) is enforced, and that ATS (Address Translation Services) are properly validated.

## 3. Performance Targets & Constraints

### 3.1 Throughput Requirements
| Parameter | Target | Rationale |
|-----------|--------|-----------|
| PCIe Link Width | x4 lanes | Standard NVMe M.2 M-key configuration |
| PCIe Link Speed | Gen2 (5.0 GT/s) native; Gen3 (8.0 GT/s) with retimer | Gen2 covers most embedded/older targets; Gen3 for modern NVMe |
| Aggregate Raw Bandwidth | 20 GT/s (Gen2 x4) = ~2.0 GB/s per direction | 5 GT/s × 4 lanes × (128b/130b encoding for Gen3, 8b/10b for Gen2) |
| Effective Data Bandwidth (Gen2) | ~1.6 GB/s per direction (after 8b/10b) | 20 GT/s × 0.8 encoding efficiency |
| USB 3.0 Egress Bandwidth | 5 Gbps (~450 MB/s effective) | FT601 SuperSpeed FIFO bridge |
| DDR3 Buffer Depth | 4 Gb (512 MB) | MT41K256M16TW-107, ~260 ms of Gen2 x4 full-rate buffering |
| TLP Capture Rate | All TLPs, zero loss at Gen2 x4 line rate | FPGA parallel pipeline with DDR3 write buffering |
| Injection Latency | < 500 ns from trigger to TLP on wire | FPGA direct TLP generator bypassing DDR3 buffer |

### 3.2 Physical Constraints
| Parameter | Target |
|-----------|--------|
| Form Factor | M.2 2280 (22 mm × 80 mm) interposer |
| Thickness | < 4.0 mm total (including components on both sides) |
| M.2 Connector (Host Side) | M.2 M-key edge connector (card edge, gold fingers) |
| M.2 Connector (Device Side) | M.2 M-key socket (Amphenol 10132797-0011LF or similar, top-mount) |
| Power Source | 3.3V from M.2 slot (pass-through to device) + supplemental from USB |
| Power Budget | < 6W total (FPGA ~2W, DDR3 ~1W, USB bridge ~1W, redrivers ~1W, misc ~1W) |
| Operating Temperature | 0°C to 70°C (commercial) |
| Weight | < 25g |

### 3.3 Operational Constraints
- **Transparency:** The interposer must be electrically transparent to both host and device during passive capture mode. PCIe link training, equalization, and power management must function normally. The host must enumerate the NVMe device identically with or without the interposer present.
- **Non-Volatility:** No persistent storage on the interposer. All captured data is streamed over USB or held in volatile DDR3. Power-off = complete data loss.
- **No Host Software:** The interposer requires zero software installation on the target host. All control and data egress is via the USB 3.0 side-channel to a separate analysis machine.
- **Hot-Plug Survivability:** The interposer must survive M.2 hot-plug events, including surprise removal and insertion with inrush current limiting.

## 4. System Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        PCIe SCREAMER INTERPOSER                         │
│                                                                         │
│  ┌──────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────┐  │
│  │          │    │              │    │              │    │          │  │
│  │  HOST    │◄──►│  PCIe Gen2/3 │◄──►│    FPGA      │◄──►│  NVMe    │  │
│  │  M.2     │    │  Redriver    │    │  Lattice     │    │  SSD     │  │
│  │  SLOT    │    │  PI3EQX12908 │    │  ECP5-45F    │    │  M.2     │  │
│  │          │    │              │    │              │    │  SOCKET  │  │
│  └──────────┘    └──────────────┘    └──────┬───────┘    └──────────┘  │
│                                             │                          │
│                    ┌────────────────────────┼──────────────────┐       │
│                    │                        │                  │       │
│               ┌────▼────┐            ┌──────▼──────┐    ┌─────▼────┐  │
│               │  DDR3   │            │   USB 3.0   │    │  Config  │  │
│               │  4 Gb   │            │   FT601Q    │    │  SPI     │  │
│               │  x16    │            │   FIFO Br.  │    │  Flash   │  │
│               └─────────┘            └──────┬──────┘    └──────────┘  │
│                                             │                          │
│                                    ┌────────▼────────┐                 │
│                                    │   USB 3.0       │                 │
│                                    │   Type-C Conn   │                 │
│                                    │   (Analysis PC) │                 │
│                                    └─────────────────┘                 │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                        POWER TREE                                │  │
│  │  M.2 3.3V ──┬── 3.3V Rail (Redrivers, FT601 IO, Flash, LEDs)   │  │
│  │             ├── DC-DC Buck ── 1.35V (DDR3 VDD/VDDQ)             │  │
│  │             ├── DC-DC Buck ── 1.2V (FPGA VCCIO, FT601 Core)     │  │
│  │             ├── DC-DC Buck ── 1.1V (FPGA VCC Core)              │  │
│  │             ├── DC-DC Buck ── 1.8V (FPGA VCCAUX, PLL)           │  │
│  │             └── USB VBUS ── 5V ──┬── 3.3V LDO (backup)         │  │
│  │                                  └── 1.1V LDO (backup)          │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

## 5. Data Flow Architecture

### 5.1 Passive Capture Pipeline

```
PCIe Lanes (Host Side)
    │
    ▼
PI3EQX12908 Redriver (RX equalization, TX de-emphasis)
    │
    ▼
FPGA ECP5-45F PCIe Hard IP (PCS + PMA)
    │  ┌─ Lane 0: RX0/TX0 ──┐
    │  ├─ Lane 1: RX1/TX1 ──┤  8b/10b decode → Symbol alignment → Ordered sets
    │  ├─ Lane 2: RX2/TX2 ──┤  → DLLP extraction → TLP reassembly
    │  └─ Lane 3: RX3/TX3 ──┘
    │
    ▼
TLP Filter & Classifier (FPGA fabric)
    │  ┌─ TLP Type (MRd/MWr/CfgRd/CfgWr/Cpl/CplD/Msg) ──┐
    │  ├─ Requester ID / Completer ID                     │
    │  ├─ Tag field                                       │
    │  ├─ Address [63:2] for memory TLPs                  │
    │  ├─ Length (10-bit DW count)                        │
    │  └─ Traffic Class (TC) / Attributes                 │
    │
    ▼
TLP Timestamp Engine (48-bit timestamp @ 125 MHz = 8 ns resolution)
    │
    ▼
DDR3 Write DMA Engine
    │  ┌─ 128-bit wide write port @ 400 MHz (DDR3-800) ──┐
    │  ├─ Circular buffer, 512 MB total                   │
    │  ├─ Write pointer (head)                            │
    │  └─ Read pointer (tail) for USB egress             │
    │
    ▼
USB Egress DMA Engine
    │  ┌─ 32-bit FT601 FIFO interface @ 100 MHz ──┐
    │  ├─ Packet framing with sync markers         │
    │  ├─ CRC-32 per packet                        │
    │  └─ Flow control (FT601 TXE# handshake)      │
    │
    ▼
FT601Q USB 3.0 FIFO Bridge
    │
    ▼
USB Type-C → Analysis PC (Companion App)
```

### 5.2 Active Injection Pipeline

```
Companion App (USB)
    │
    ▼
FT601Q (RX channel)
    │
    ▼
Command Parser (FPGA fabric)
    │  ┌─ INJECT_MRd: {address[63:2], length[9:0], tag[7:0], requester_id[15:0]} ──┐
    │  ├─ INJECT_MWr: {address[63:2], length[9:0], data[0..N]}                     │
    │  ├─ INJECT_CfgRd: {bus[7:0], dev[4:0], func[2:0], reg[11:2]}                 │
    │  └─ INJECT_CfgWr: {bus[7:0], dev[4:0], func[2:0], reg[11:2], data[31:0]}    │
    │
    ▼
TLP Builder (FPGA fabric)
    │  ┌─ Fmt/Type encoding ──┐
    │  ├─ TC/Attr/TH/TD/EP    │
    │  ├─ Length / Tag        │
    │  ├─ Requester ID        │
    │  ├─ Address routing     │
    │  ├─ CRC (LCRC) calc    │
    │  └─ Framing (STP/SDP/END) │
    │
    ▼
PCIe Hard IP TX Pipeline
    │
    ▼
PI3EQX12908 Redriver (TX direction)
    │
    ▼
Host M.2 Slot (injected TLP appears as if from NVMe device)
```

## 6. Bus Topology & Interconnect

### 6.1 PCIe Lane Mapping

```
Host M.2 Edge Connector (Gold Fingers, Top Side)
  Pin 41: PETp0  ──── PI3EQX12908 Ch0 A+ ──── FPGA SERDES RX0+ ──── PI3EQX12908 Ch0 B+ ──── Device M.2 Socket PETp0
  Pin 43: PETn0  ──── PI3EQX12908 Ch0 A- ──── FPGA SERDES RX0- ──── PI3EQX12908 Ch0 B- ──── Device M.2 Socket PETn0
  Pin 47: PERp0  ──── PI3EQX12908 Ch0 B+ ──── FPGA SERDES TX0+ ──── PI3EQX12908 Ch0 A+ ──── Device M.2 Socket PERp0
  Pin 49: PERn0  ──── PI3EQX12908 Ch0 B- ──── FPGA SERDES TX0- ──── PI3EQX12908 Ch0 A- ──── Device M.2 Socket PERn0

  Pin 53: PETp1  ──── PI3EQX12908 Ch1 A+ ──── FPGA SERDES RX1+ ──── PI3EQX12908 Ch1 B+ ──── Device M.2 Socket PETp1
  Pin 55: PETn1  ──── PI3EQX12908 Ch1 A- ──── FPGA SERDES RX1- ──── PI3EQX12908 Ch1 B- ──── Device M.2 Socket PETn1
  Pin 59: PERp1  ──── PI3EQX12908 Ch1 B+ ──── FPGA SERDES TX1+ ──── PI3EQX12908 Ch1 A+ ──── Device M.2 Socket PERp1
  Pin 61: PERn1  ──── PI3EQX12908 Ch1 B- ──── FPGA SERDES TX1- ──── PI3EQX12908 Ch1 A- ──── Device M.2 Socket PERn1

  Pin 65: PETp2  ──── PI3EQX12908 Ch2 A+ ──── FPGA SERDES RX2+ ──── PI3EQX12908 Ch2 B+ ──── Device M.2 Socket PETp2
  Pin 67: PETn2  ──── PI3EQX12908 Ch2 A- ──── FPGA SERDES RX2- ──── PI3EQX12908 Ch2 B- ──── Device M.2 Socket PETn2
  Pin 71: PERp2  ──── PI3EQX12908 Ch2 B+ ──── FPGA SERDES TX2+ ──── PI3EQX12908 Ch2 A+ ──── Device M.2 Socket PERp2
  Pin 73: PERn2  ──── PI3EQX12908 Ch2 B- ──── FPGA SERDES TX2- ──── PI3EQX12908 Ch2 A- ──── Device M.2 Socket PERn2

  Pin 77: PETp3  ──── PI3EQX12908 Ch3 A+ ──── FPGA SERDES RX3+ ──── PI3EQX12908 Ch3 B+ ──── Device M.2 Socket PETp3
  Pin 79: PETn3  ──── PI3EQX12908 Ch3 A- ──── FPGA SERDES RX3- ──── PI3EQX12908 Ch3 B- ──── Device M.2 Socket PETn3
  Pin 83: PERp3  ──── PI3EQX12908 Ch3 B+ ──── FPGA SERDES TX3+ ──── PI3EQX12908 Ch3 A+ ──── Device M.2 Socket PERp3
  Pin 85: PERn3  ──── PI3EQX12908 Ch3 B- ──── FPGA SERDES TX3- ──── PI3EQX12908 Ch3 A- ──── Device M.2 Socket PERn3
```

### 6.2 Reference Clock Distribution

```
Host M.2 Pin 51: REFCLKp ───┬── PI3EQX12908 REFCLK_IN+
Host M.2 Pin 53: REFCLKn ───┼── PI3EQX12908 REFCLK_IN-
                             │
                             ├── Si5332 Clock Buffer IN+
                             │   └── OUT0+/- ── FPGA PCIE_REFCLK_P/N (Bank 7)
                             │   └── OUT1+/- ── Device M.2 Socket REFCLKp/n
                             │
                             └── Device M.2 Socket REFCLKp/n (pass-through)
```

### 6.3 Sideband Signal Pass-Through

```
Host M.2 Pin 69: PEDET (NC on interposer — always pulled low for M.2 detect)
Host M.2 Pin 50: PERST# ──── Pass-through to Device M.2 Socket PERST#
                            └── Also to FPGA GPIO for reset detection
Host M.2 Pin 56: CLKREQ# ─── Pass-through to Device M.2 Socket CLKREQ#
Host M.2 Pin 58: PEWAKE# ─── Pass-through to Device M.2 Socket PEWAKE#
Host M.2 Pin 75: SUSCLK ──── Pass-through to Device M.2 Socket SUSCLK (32.768 kHz)
```

### 6.4 DDR3 Interface

```
FPGA Bank 6 (DDR3 Interface, 1.35V SSTL135):
  A[14:0]    ── DDR3 Address bus
  BA[2:0]    ── DDR3 Bank Address
  DQ[15:0]   ── DDR3 Data bus (x16)
  DQS[1:0]   ── DDR3 Data Strobe (differential)
  DM[1:0]    ── DDR3 Data Mask
  CK/CK#     ── DDR3 Differential Clock
  CKE        ── DDR3 Clock Enable
  CS#        ── DDR3 Chip Select
  RAS#       ── DDR3 Row Address Strobe
  CAS#       ── DDR3 Column Address Strobe
  WE#        ── DDR3 Write Enable
  ODT        ── DDR3 On-Die Termination
  RESET#     ── DDR3 Reset
```

### 6.5 USB 3.0 FIFO Interface (FT601Q)

```
FPGA Bank 5 (3.3V LVCMOS):
  DATA[31:0]  ── FT601Q D[31:0] (bidirectional 32-bit FIFO data)
  BE[3:0]     ── FT601Q BE[3:0] (byte enables)
  TXE#        ── FT601Q TXE# (transmit FIFO empty, active low)
  RXF#        ── FT601Q RXF# (receive FIFO full, active low)
  WR#         ── FT601Q WR# (write strobe)
  RD#         ── FT601Q RD# (read strobe)
  OE#         ── FT601Q OE# (output enable)
  SIWU#       ── FT601Q SIWU# (send immediate / wake up)
  CLK         ── FT601Q CLK (100 MHz from FPGA PLL)
  RESET#      ── FT601Q RESET#
```

### 6.6 Configuration & Debug

```
FPGA Bank 0 (3.3V):
  SPI Flash: W25Q128JVSIQ (128 Mb)
    ┌─ CS#   ── FPGA CSPI_CSN
    ├─ CLK   ── FPGA CSPI_CLK
    ├─ MOSI  ── FPGA CSPI_MOSI
    └─ MISO  ── FPGA CSPI_MISO

  Status LEDs:
    ┌─ LED0 (Green) ── FPGA GPIO: PCIe Link Up
    ├─ LED1 (Blue)  ── FPGA GPIO: USB Enumeration
    ├─ LED2 (Amber) ── FPGA GPIO: Capture Active
    └─ LED3 (Red)   ── FPGA GPIO: Error/Fault

  Debug UART (optional, test points):
    ┌─ TX ── FPGA GPIO
    └─ RX ── FPGA GPIO
```

## 7. FPGA Internal Architecture

### 7.1 Top-Level Module Hierarchy

```
top_scramer
├── pcie_hard_ip_wrapper        // ECP5 PCIe Hard IP instantiation
│   ├── pcie_pcs_pma            // Physical Coding Sublayer + PMA
│   ├── pcie_dlp_handler        // Data Link Layer Processor
│   └── pcie_tlp_interface      // TLP RX/TX FIFO interface
├── tlp_sniffer                 // Passive TLP capture engine
│   ├── tlp_rx_monitor          // Monitors RX TLPs (host→device)
│   ├── tlp_tx_monitor          // Monitors TX TLPs (device→host)
│   ├── tlp_classifier          // Decodes TLP header fields
│   ├── tlp_timestamp           // 48-bit timestamp counter
│   └── tlp_packer              // Packs TLP + metadata into 128-bit records
├── ddr3_controller             // DDR3 SDRAM controller
│   ├── ddr3_phy                // Physical interface (IODELAY, DQS gating)
│   ├── ddr3_init_fsm           // JEDEC initialization sequence
│   ├── ddr3_write_dma          // Write DMA from TLP packer
│   └── ddr3_read_dma           // Read DMA to USB egress
├── usb_egress_engine           // USB 3.0 streaming engine
│   ├── ft601_driver            // FT601 FIFO interface driver
│   ├── packet_framer           // Frame sync + CRC-32
│   └── flow_control            // TXE#/RXF# handshake management
├── tlp_injector                // Active TLP injection engine
│   ├── cmd_parser              // Parses injection commands from USB
│   ├── tlp_builder             // Constructs PCIe TLPs
│   ├── lcrc_generator          // LCRC calculation (32-bit parallel)
│   └── inject_arbiter          // Arbitration between pass-through and injection
├── pcie_config_space           // Interposer's own PCIe config space (optional)
│   └── type0_config            // Type 0 config space registers
├── clock_reset_manager         // Clock domain crossing and reset sequencing
│   ├── pll_125mhz              // 125 MHz system clock (PCIe core)
│   ├── pll_100mhz              // 100 MHz FT601 clock
│   ├── pll_400mhz              // 400 MHz DDR3 clock
│   └── reset_sequencer         // Power-on reset sequencing
└── debug_uart                  // Optional debug UART
```

### 7.2 Clock Domains

| Domain | Frequency | Source | Consumers |
|--------|----------|--------|-----------|
| PCIe Core | 125 MHz | PCIe Hard IP PLL (from REFCLK 100 MHz) | PCIe IP, TLP sniffer, TLP injector |
| DDR3 Controller | 400 MHz | FPGA PLL (from 125 MHz) | DDR3 PHY, write/read DMA |
| DDR3 User Interface | 200 MHz | FPGA PLL (÷2 from 400 MHz) | TLP packer, USB egress engine |
| USB FIFO | 100 MHz | FPGA PLL (from 125 MHz) | FT601 driver, packet framer |
| Timestamp | 125 MHz | PCIe Core domain | TLP timestamp counter |
| Debug UART | 125 MHz (÷13 ≈ 9.615 MHz) | PCIe Core domain | UART TX/RX |

### 7.3 Clock Domain Crossing (CDC) Strategy

All CDC boundaries use:
- **Async FIFOs** (ECP5 EBR-based) for data paths between PCIe Core (125 MHz) ↔ DDR3 User (200 MHz) ↔ USB FIFO (100 MHz)
- **Double-flop synchronizers** for single-bit control signals
- **Gray-coded pointers** for multi-bit FIFO level indicators
- **Handshake synchronizers** for configuration register writes

## 8. Power Architecture

### 8.1 Power Sources

| Source | Voltage | Max Current | Use |
|--------|---------|-------------|-----|
| M.2 Slot 3.3V | 3.3V ±9% | 2.5A (per M.2 spec) | Primary power for all rails |
| USB VBUS | 5V ±5% | 900 mA (USB 3.0) | Supplemental/backup power |

### 8.2 Power Rails

| Rail | Voltage | Max Current | Regulator | Loads |
|------|---------|-------------|-----------|-------|
| VCC3V3 | 3.3V | 1.5A | Direct from M.2 + LDO backup | Redrivers, FT601 IO, SPI Flash, LEDs, FPGA VCCIO banks |
| VCC1V35 | 1.35V | 0.8A | TPS62825 (3A buck) | DDR3 VDD/VDDQ, FPGA Bank 6 VCCIO |
| VCC1V2 | 1.2V | 0.5A | TPS62825 (3A buck) | FPGA VCCIO (Banks 0,1,2,3,4,5,7), FT601 Core |
| VCC1V1 | 1.1V | 1.2A | TPS62825 (3A buck) | FPGA VCC (core voltage) |
| VCC1V8 | 1.8V | 0.3A | TPS62825 (3A buck) | FPGA VCCAUX, PLL analog supply |
| VCC5V0 | 5.0V | 0.9A | USB VBUS direct | Backup LDO inputs |

### 8.3 Power Sequencing

```
Power-On:
  1. M.2 3.3V present → VCC3V3 rail active
  2. VCC3V3 stable → Enable VCC1V8 (FPGA AUX/PLL first)
  3. VCC1V8 stable → Enable VCC1V2 (FPGA I/O)
  4. VCC1V2 stable → Enable VCC1V1 (FPGA Core)
  5. VCC1V1 stable → Enable VCC1V35 (DDR3)
  6. All rails stable → Release FPGA POR reset
  7. FPGA config loaded → DDR3 init → PCIe link training

Power-Off:
  Reverse sequence with 1 ms delays between each rail disable
```

### 8.4 Decoupling Strategy

- **FPGA Core (VCC1V1):** 10× 100 nF X7R 0402 + 4× 10 µF X7R 0805 + 2× 100 µF tantalum
- **FPGA VCCIO Banks:** 4× 100 nF X7R 0402 + 1× 10 µF X7R 0805 per bank
- **FPGA VCCAUX/PLL:** 4× 100 nF X7R 0402 + 2× 10 µF X7R 0805 + ferrite bead isolation
- **DDR3 VDD/VDDQ:** 8× 100 nF X7R 0402 + 4× 10 µF X7R 0805 + 2× 100 µF tantalum
- **Redrivers:** 2× 100 nF X7R 0402 + 1× 10 µF X7R 0603 per device
- **FT601Q:** 6× 100 nF X7R 0402 + 2× 10 µF X7R 0805 + 1× 47 µF tantalum

## 9. Firmware Boot Strategy

### 9.1 FPGA Configuration

The ECP5-45F boots from external SPI flash (W25Q128JVSIQ, 128 Mb) in Master SPI mode. Configuration pins:
- **CFG[1:0] = 01** (Master SPI, x1 mode)
- **PROGRAMN** pulled high (10k to 3.3V), driven low by supervisor to re-init
- **INITN** pulled high (10k to 3.3V), open-drain status output
- **DONE** pulled high (10k to 3.3V), indicates configuration complete

Bitstream size: ~4.7 Mb for ECP5-45F (uncompressed), fits in 128 Mb flash with room for golden + update images.

### 9.2 Boot Sequence

```
1. Power rails sequence up
2. FPGA POR released → samples CFG pins → enters Master SPI mode
3. FPGA drives CSPI_CSN low, clocks CSPI_CLK, reads bitstream from flash
4. INITN goes high → DONE goes high → FPGA enters user mode
5. Firmware initializes:
   a. PLLs (125 MHz, 100 MHz, 400 MHz)
   b. DDR3 controller (JEDEC init sequence: reset, ZQ cal, MRS, training)
   c. PCIe Hard IP (link training, equalization)
   d. FT601 driver (reset, enumeration wait)
   e. TLP sniffer pipeline
   f. USB egress engine
6. Main loop: capture TLPs → buffer DDR3 → stream USB
```

## 10. Companion App Architecture

### 10.1 Technology Stack
- **Framework:** React Native (cross-platform iOS/Android/Desktop via React Native Windows/macOS)
- **USB Communication:** Node.js native module via `node-hid` or libusb for FT601 D3XX driver interface
- **State Management:** React Context + useReducer
- **Visualization:** Custom canvas-based TLP timeline and scatter plots
- **Export:** PCAPng format for Wireshark PCIe dissector compatibility

### 10.2 App Screens
1. **CaptureScreen:** Real-time TLP stream display, filter controls, start/stop capture, buffer status
2. **AnalysisScreen:** TLP decode view, NVMe command extraction, DMA address mapping, search/filter
3. **InjectionScreen:** TLP injection builder, address/type/length configuration, trigger controls
4. **SettingsScreen:** Device configuration, firmware update, calibration, export settings

### 10.3 Wire Protocol (USB)
- Binary framed protocol with sync markers (0xSCREAM = 0x53 0x43 0x52 0x45 0x41 0x4D)
- 32-bit CRC (CRC-32C Castagnoli) per frame
- Command IDs: CAPTURE_START (0x01), CAPTURE_STOP (0x02), INJECT_TLP (0x10), QUERY_STATUS (0x20), FW_UPDATE (0x30)
- TLP data frames: header (timestamp, type, length, direction) + raw TLP bytes

## 11. Security Considerations

### 11.1 Operational Security
- No non-volatile storage of captured data (DDR3 only, wiped on power loss)
- No host-detectable USB device (USB side-channel is physically separate)
- No modification to host PCIe configuration space
- Transparent to PCIe link training and power management

### 11.2 Anti-Forensic Features
- FPGA bitstream encrypted (ECP5 AES-128 bitstream encryption with user key in eFuse)
- No serial number or identifying information in USB descriptors
- USB VID/PID programmable (default: generic FTDI values, user-customizable)
- Tamper-evident: configuration CRC check on boot, LED indication if bitstream modified

### 11.3 Ethical Use
- Device intended for authorized security research only
- Active injection features require explicit user confirmation
- Documentation includes clear warnings about legal restrictions on DMA attacks

## 12. Development & Production Estimates

| Phase | Effort | Deliverables |
|-------|--------|--------------|
| Schematic Design | 3 weeks | Full schematic, BOM, netlist |
| PCB Layout | 4 weeks | 8-layer HDI PCB, impedance-controlled |
| FPGA RTL Development | 8 weeks | PCIe IP integration, TLP sniffer, DDR3 controller, USB engine |
| Firmware Integration | 2 weeks | Boot, initialization, main loop |
| Companion App | 4 weeks | React Native app, USB driver, protocol |
| Testing & Validation | 3 weeks | Signal integrity, throughput, compatibility |
| **Total** | **~24 weeks** | Production-ready design |

| Cost Item | Unit Cost | Quantity | Extended |
|-----------|-----------|----------|----------|
| Lattice ECP5-45F (LFE5U-45F-6BG381C) | $22.50 | 1 | $22.50 |
| PI3EQX12908 PCIe Redriver | $4.80 | 2 | $9.60 |
| FT601Q USB 3.0 Bridge | $8.50 | 1 | $8.50 |
| MT41K256M16TW-107 DDR3 | $4.20 | 1 | $4.20 |
| W25Q128JVSIQ SPI Flash | $0.85 | 1 | $0.85 |
| TPS62825 Buck Converters | $1.20 | 4 | $4.80 |
| Si5332 Clock Buffer | $3.50 | 1 | $3.50 |
| M.2 Socket (Amphenol) | $2.80 | 1 | $2.80 |
| USB Type-C Connector | $0.60 | 1 | $0.60 |
| 8-layer HDI PCB (prototype qty 5) | $12.00 | 1 | $12.00 |
| Passives (R, C, L, ferrite beads) | $8.00 | 1 | $8.00 |
| Assembly (prototype) | $15.00 | 1 | $15.00 |
| **Total BOM (qty 1 prototype)** | | | **$93.35** |
| **Total BOM (qty 100 production)** | | | **~$62.00** |

---

*End of Phase 1 — Conceptual Architecture. This document defines the complete system architecture, data flow, bus topology, power tree, clock domains, and operational parameters for the PCIe Screamer interposer.*
