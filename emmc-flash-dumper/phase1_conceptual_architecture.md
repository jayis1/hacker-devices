# Phase 1: Conceptual Architecture — eMMC Flash Dumper

## 1.0 Executive Summary

The **eMMC Flash Dumper** is a portable, self-contained forensic acquisition platform designed for direct, hardware-level extraction of raw flash memory from embedded devices, IoT endpoints, smartphones, automotive ECUs, and industrial controllers. Unlike software-based imaging tools that rely on a cooperative host OS, the Flash Dumper connects directly to the flash memory ICs — eMMC, raw NAND, or SPI NOR — and performs a bit-for-bit readout at the electrical level. This makes it immune to OS-level anti-forensics, locked bootloaders, and filesystem encryption that operates above the block layer.

The device targets three primary flash interfaces:
1. **eMMC 5.1** (JESD84-B51) via the 8-bit HS400 SD/MMC bus at up to 400 MB/s
2. **Parallel NAND Flash** (ONFI 4.0 / Toggle 2.0) via an 8-bit asynchronous interface with FPGA-assisted timing
3. **SPI NOR Flash** (single/dual/quad SPI) via a dedicated OCTOSPI peripheral at up to 100 MHz SDR

Acquired data streams over USB 3.0 SuperSpeed (5 Gbps) to a host computer running the companion analysis app, or is written directly to an onboard microSD card in standalone mode. A 512 MB DDR3L SDRAM buffer decouples flash readout speed from USB transfer speed, preventing FIFO underruns during high-speed eMMC HS400 acquisition.

## 1.1 System Purpose & Mission Profile

### Primary Mission
Extract a complete, verifiable forensic image of flash memory from a target device without requiring the target's CPU to be operational, cooperative, or even present. The operator desolders or clips onto the flash IC, connects the Dumper, and initiates acquisition.

### Secondary Missions
- **Chip-off forensics**: Read flash chips removed from damaged devices
- **In-system acquisition**: Clip onto flash ICs still soldered to a PCB (ISP — In-System Programming)
- **Data recovery**: Recover data from devices with corrupted firmware or failed boot sequences
- **Security research**: Extract and analyze firmware, bootloaders, and secure elements from IoT/embedded devices
- **Verification**: Compute SHA-256 hashes of acquired images for chain-of-custody

### Operational Constraints
- **Portable**: Credit-card sized PCB (85 mm × 54 mm), battery-powered via LiPo
- **Field-ready**: Operates standalone (no host PC required) with OLED UI
- **Under $100 BOM**: Aggressive cost target using commodity components
- **Open source**: Full KiCad design files, firmware source, and companion app

## 1.2 Attack Surface & Threat Model

### What the Device Attacks
The Flash Dumper attacks the **physical layer** of flash memory storage. It bypasses all software security controls by reading raw NAND pages (including spare/OOB areas), eMMC partitions (including boot0/boot1/RPMB), and SPI NOR sectors directly from the silicon.

### Security Mechanisms Bypassed
| Mechanism | How Bypassed |
|-----------|-------------|
| Full-Disk Encryption (FDE) | Raw image contains encrypted blocks; analysis performed offline |
| Secure Boot / Verified Boot | Flash read at hardware level; boot ROM not involved |
| Locked Bootloader | Direct flash access; no bootloader interaction |
| Filesystem Permissions | Raw block device image; no OS mediation |
| eMMC RPMB (Replay Protected Memory Block) | RPMB read via authenticated access if key known; otherwise, raw NAND dump includes RPMB area |
| JTAG/SWD Lock | Not needed; flash accessed directly |
| USB Debugging Disabled | Not needed; flash accessed directly |

### Limitations
- **Encrypted data**: The Dumper extracts raw ciphertext; decryption requires key material obtained separately
- **BGA packages**: eMMC chips in 153/169-ball BGA require soldering skill or specialized adapters
- **eMMC RPMB**: The RPMB partition requires HMAC authentication; raw read may be possible depending on controller implementation
- **Monolithic eMCP/ePOP**: Combined DRAM+flash packages require careful pin isolation
- **Wear-leveling metadata**: Raw NAND dumps include FTL metadata; reconstruction requires filesystem-specific analysis

## 1.3 Performance Targets

| Parameter | Target | Notes |
|-----------|--------|-------|
| eMMC Read Speed | 200-400 MB/s | HS400 mode, 8-bit DDR @ 200 MHz |
| NAND Page Read | 40-80 MB/s | ONFI 4.0, async mode, FPGA-timed |
| SPI NOR Read | 12-25 MB/s | Quad SPI @ 100 MHz SDR |
| USB Transfer | 300+ MB/s | USB 3.0 SuperSpeed bulk |
| microSD Write | 30-60 MB/s | UHS-I SDR104 |
| Image Verification | SHA-256 @ 50+ MB/s | Hardware crypto accelerator |
| Battery Life | 2+ hours active | 2000 mAh LiPo |
| Boot Time | < 3 seconds | From power-on to ready |

## 1.4 Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        eMMC FLASH DUMPER SYSTEM                          │
│                                                                          │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────────────────┐   │
│  │  LiPo Battery │    │  USB 3.0     │    │  microSD Card Slot       │   │
│  │  3.7V 2000mAh │    │  Type-C Conn │    │  UHS-I SDR104            │   │
│  └──────┬───────┘    └──────┬───────┘    └───────────┬──────────────┘   │
│         │                  │                         │                   │
│  ┌──────┴──────────────────┴─────────────────────────┴──────────────┐   │
│  │                     POWER MANAGEMENT UNIT                         │   │
│  │  TPS6521815 PMIC: 3.3V, 1.8V, 1.2V, 1.35V, 1.0V rails          │   │
│  │  BQ25896 Charger: LiPo charge + OTG boost (5V @ 2A)              │   │
│  └──────┬──────────────────┬──────────────────┬─────────────────────┘   │
│         │                  │                  │                          │
│  ┌──────┴──────┐   ┌───────┴───────┐  ┌───────┴──────────┐             │
│  │ STM32H743VI │   │ Lattice       │  │ DDR3L SDRAM      │             │
│  │ Cortex-M7   │   │ iCE40UP5K     │  │ MT41K256M16TW    │             │
│  │ @ 480 MHz   │◄──┤ FPGA          │  │ 512 MB (4 Gb)    │             │
│  │             │   │ 48-QFN        │  │ 96-BGA           │             │
│  │ ┌─────────┐ │   │               │  │                  │             │
│  │ │SDMMC2   ├─┼───┼───────────────┼──┤                  │             │
│  │ │(eMMC)   │ │   │               │  │                  │             │
│  │ └─────────┘ │   │ ┌───────────┐ │  │                  │             │
│  │ ┌─────────┐ │   │ │ NAND Ctrl │ │  │                  │             │
│  │ │FMC      ├─┼───┼─┤ Timing +  │ │  │                  │             │
│  │ │(NAND)   │ │   │ │ Capture   │ │  │                  │             │
│  │ └─────────┘ │   │ └───────────┘ │  │                  │             │
│  │ ┌─────────┐ │   │               │  │                  │             │
│  │ │OCTOSPI1 │ │   │               │  │                  │             │
│  │ │(SPI NOR)│ │   │               │  │                  │             │
│  │ └─────────┘ │   │               │  │                  │             │
│  │ ┌─────────┐ │   │               │  │                  │             │
│  │ │FMC SDRAM├─┼───┼───────────────┼──┤                  │             │
│  │ │Ctrl     │ │   │               │  │                  │             │
│  │ └─────────┘ │   │               │  │                  │             │
│  │ ┌─────────┐ │   │               │  │                  │             │
│  │ │USB OTG_HS│ │   │               │  │                  │             │
│  │ │(ULPI)   │ │   │               │  │                  │             │
│  │ └─────────┘ │   │               │  │                  │             │
│  │ ┌─────────┐ │   │               │  │                  │             │
│  │ │HASH     │ │   │               │  │                  │             │
│  │ │(SHA-256)│ │   │               │  │                  │             │
│  │ └─────────┘ │   │               │  │                  │             │
│  └──────┬──────┘   └───────────────┘  └──────────────────┘             │
│         │                                                               │
│  ┌──────┴────────────────────────────────────────────┐                  │
│  │              USER INTERFACE                        │                  │
│  │  SSD1306 128×64 OLED (I2C)                        │                  │
│  │  4× Tactile Buttons (GPIO)                        │                  │
│  │  2× RGB Status LEDs (GPIO PWM)                    │                  │
│  │  Buzzer (GPIO PWM)                                │                  │
│  └───────────────────────────────────────────────────┘                  │
│                                                                          │
│  ┌────────────────────────────────────────────────────┐                 │
│  │              TARGET INTERFACES                     │                 │
│  │                                                    │                 │
│  │  ┌──────────────────┐  ┌──────────────────┐        │                 │
│  │  │ eMMC Socket      │  │ NAND TSOP-48     │        │                 │
│  │  │ 153-ball BGA     │  │ ZIF Socket       │        │                 │
│  │  │ adapter board    │  │ (or ISP header)  │        │                 │
│  │  └──────────────────┘  └──────────────────┘        │                 │
│  │  ┌──────────────────┐  ┌──────────────────┐        │                 │
│  │  │ SPI NOR SOIC-8   │  │ ISP Probe Header │        │                 │
│  │  │ Test Clip        │  │ 20-pin 0.1"      │        │                 │
│  │  └──────────────────┘  └──────────────────┘        │                 │
│  └────────────────────────────────────────────────────┘                 │
└─────────────────────────────────────────────────────────────────────────┘
```

## 1.5 Data Flow Architecture

### Acquisition Pipeline

```
Target Flash IC
      │
      ▼
┌─────────────────┐
│ Physical Layer   │  eMMC: CLK/CMD/DAT0-7 (1.8V or 3.3V signaling)
│ Interface        │  NAND: DQ0-7, CLE, ALE, WE#, RE#, CE#, R/B#
│ (Level Shifters) │  SPI: SCK, MOSI, MISO, CS#, WP#, HOLD#
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Host Controller  │  SDMMC2: eMMC 5.1 HS400, DMA to SDRAM
│ (STM32H743)      │  FMC: NAND async, FPGA-assisted timing
│                  │  OCTOSPI1: SPI NOR quad mode, memory-mapped
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ DMA Engine       │  Dual-port MDMA + BDMA
│                  │  Stream 0: Flash → SDRAM (acquisition)
│                  │  Stream 1: SDRAM → USB FIFO (transfer)
│                  │  Stream 2: SDRAM → SDMMC1 (microSD write)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 512 MB DDR3L     │  Ring buffer, 64 KB blocks
│ SDRAM Buffer     │  Write pointer (acquisition), Read pointer (transfer)
└────────┬────────┘
         │
    ┌────┴────┐
    ▼         ▼
┌────────┐ ┌────────┐
│ USB 3.0│ │microSD │
│ Host   │ │ Card   │
└────────┘ └────────┘
```

### Data Paths

| Path | Source | Destination | Protocol | Max Throughput |
|------|--------|-------------|----------|----------------|
| eMMC → SDRAM | SDMMC2 | FMC SDRAM Bank 1 | Internal DMA | 400 MB/s |
| NAND → SDRAM | FMC NAND | FMC SDRAM Bank 1 | FPGA-timed DMA | 80 MB/s |
| SPI NOR → SDRAM | OCTOSPI1 | FMC SDRAM Bank 1 | Memory-mapped DMA | 25 MB/s |
| SDRAM → USB | FMC SDRAM | USB OTG_HS (ULPI) | DMA Stream 1 | 300+ MB/s |
| SDRAM → microSD | FMC SDRAM | SDMMC1 | DMA Stream 2 | 60 MB/s |
| SDRAM → HASH | FMC SDRAM | HASH (SHA-256) | DMA Stream 3 | 50+ MB/s |

## 1.6 Bus Topology & Interconnect

### Internal Buses

```
                    ┌─────────────────────────────────────┐
                    │         AXI Bus Matrix (64-bit)      │
                    │         240 MHz, 8 masters/8 slaves  │
                    └─────────────────────────────────────┘
                                    │
        ┌───────────┬───────────────┼───────────────┬───────────┐
        ▼           ▼               ▼               ▼           ▼
   ┌─────────┐ ┌─────────┐   ┌───────────┐   ┌─────────┐ ┌─────────┐
   │Cortex-M7│ │ MDMA    │   │ SDMMC2    │   │ OCTOSPI1│ │ USB     │
   │I-Cache  │ │ 2×Stream│   │ AHB Slave │   │ AHB Slave│ │ OTG_HS  │
   │D-Cache  │ │         │   │           │   │          │ │ ULPI    │
   └─────────┘ └─────────┘   └───────────┘   └─────────┘ └─────────┘
                                    │
                           ┌────────┴────────┐
                           ▼                 ▼
                    ┌───────────┐     ┌───────────┐
                    │ FMC       │     │ SDMMC1    │
                    │ (SDRAM +  │     │ (microSD) │
                    │  NAND)    │     │           │
                    └─────┬─────┘     └───────────┘
                          │
              ┌───────────┴───────────┐
              ▼                       ▼
        ┌───────────┐          ┌───────────┐
        │ DDR3L     │          │ NAND      │
        │ SDRAM     │          │ Target    │
        │ 512 MB    │          │ (via FPGA)│
        └───────────┘          └───────────┘
```

### External Buses

| Bus | Pins | Voltage | Speed | Termination |
|-----|------|---------|-------|-------------|
| eMMC HS400 | CLK, CMD, DAT0-7, RST_n | 1.8V | 200 MHz DDR | 50Ω series at source |
| NAND Async | DQ0-7, CLE, ALE, WE#, RE#, CE#, R/B#, WP# | 1.8V/3.3V | 50 MHz max | 50Ω series |
| SPI NOR Quad | SCK, IO0-3, CS# | 1.8V/3.3V | 100 MHz SDR | 33Ω series |
| DDR3L SDRAM | A0-15, BA0-2, DQ0-15, DQS, DM, CK, CKE, CS#, RAS#, CAS#, WE#, ODT | 1.35V | 400 MHz DDR | ODT 40Ω |
| USB 3.0 ULPI | CLK, DIR, STP, NXT, D0-7 | 1.8V | 60 MHz | Internal |
| microSD | CLK, CMD, DAT0-3, CD | 3.3V/1.8V | 208 MHz SDR | 50Ω series |
| I2C (OLED) | SDA, SCL | 3.3V | 400 kHz | 4.7kΩ pull-up |
| FPGA SPI Config | SCK, MOSI, MISO, CS#, CRESET, CDONE | 3.3V | 20 MHz | None |

## 1.7 Clock Tree

```
┌─────────────────────────────────────────────────────────────┐
│ 25 MHz HSE Crystal (STM32H743)                               │
│   │                                                          │
│   ├──► PLL1: 25 MHz × 192 / 10 = 480 MHz → SYSCLK (CPU)     │
│   │     ├──► AHB: /1 = 240 MHz                               │
│   │     ├──► APB1: /4 = 60 MHz                               │
│   │     ├──► APB2: /2 = 120 MHz                              │
│   │     ├──► FMC: /2 = 120 MHz (SDRAM)                       │
│   │     └──► SDMMC2: /2 = 120 MHz → /3 = 40 MHz base        │
│   │                                                          │
│   ├──► PLL2: 25 MHz × 160 / 10 = 400 MHz                     │
│   │     ├──► SDMMC1: /2 = 200 MHz (microSD SDR104)           │
│   │     └──► OCTOSPI: /4 = 100 MHz                           │
│   │                                                          │
│   └──► PLL3: 25 MHz × 120 / 10 = 300 MHz                     │
│         └──► USB OTG_HS ULPI: 60 MHz (external PHY)          │
│                                                              │
│ 32.768 kHz LSE Crystal → RTC (timestamping)                  │
│                                                              │
│ 12 MHz FPGA Oscillator → iCE40UP5K internal PLL              │
│   └──► 48 MHz sysclk, 24 MHz NAND timing                     │
└─────────────────────────────────────────────────────────────┘
```

## 1.8 Memory Map

### STM32H743VI Memory Map (Relevant Regions)

| Region | Start Address | Size | Peripheral |
|--------|--------------|------|------------|
| ITCM RAM | 0x00000000 | 64 KB | Instruction TCM |
| DTCM RAM | 0x20000000 | 128 KB | Data TCM |
| AXI SRAM | 0x24000000 | 512 KB | System RAM |
| SRAM1 | 0x30000000 | 128 KB | AHB SRAM |
| SRAM2 | 0x30020000 | 128 KB | AHB SRAM |
| SRAM3 | 0x30040000 | 32 KB | AHB SRAM |
| SRAM4 | 0x38000000 | 64 KB | Backup SRAM |
| FMC SDRAM Bank 1 | 0xC0000000 | 256 MB | SDRAM (first half) |
| FMC SDRAM Bank 2 | 0xD0000000 | 256 MB | SDRAM (second half) |
| OCTOSPI1 (MMAP) | 0x90000000 | 256 MB | SPI NOR memory-mapped |
| FMC NAND | 0x80000000 | 256 MB | NAND common space |
| FMC NAND Attribute | 0x88000000 | 256 MB | NAND attribute space |
| SDMMC2 FIFO | 0x52013480 | 128 words | eMMC data FIFO |
| USB OTG_HS FIFO | 0x50040000 | 4 KB | USB endpoint FIFO |
| HASH Digest | 0x50060400 | 32 bytes | SHA-256 digest registers |

### FPGA Register Map (via SPI)

| Register | Address | Width | Description |
|----------|---------|-------|-------------|
| FPGA_ID | 0x00 | 32 | Version/ID (0x464E4144 "FNAD") |
| FPGA_CTRL | 0x04 | 32 | Control register |
| FPGA_STATUS | 0x08 | 32 | Status register |
| NAND_TIMING_0 | 0x0C | 32 | tWC, tWP, tWH timing |
| NAND_TIMING_1 | 0x10 | 32 | tRC, tRP, tREH timing |
| NAND_TIMING_2 | 0x14 | 32 | tWB, tADL, tWHR timing |
| NAND_DATA_IN | 0x18 | 32 | Captured NAND data (read) |
| NAND_DATA_OUT | 0x1C | 32 | NAND data to drive (write) |
| NAND_CMD | 0x20 | 32 | NAND command latch |
| NAND_ADDR | 0x24 | 32 | NAND address latch (5 cycles) |
| FPGA_FIFO_COUNT | 0x28 | 32 | Words in capture FIFO |
| FPGA_FIFO_DATA | 0x2C | 32 | FIFO read port |
| FPGA_INTR_MASK | 0x30 | 32 | Interrupt mask |
| FPGA_INTR_STATUS | 0x34 | 32 | Interrupt status (WC) |

## 1.9 Power Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ USB 5V (Type-C) ──► BQ25896 Charger                          │
│   │                │  ├─ LiPo Charge (4.2V, 2A max)          │
│   │                │  ├─ OTG Boost (5V, 2A from battery)     │
│   │                │  └─ Power Path: VSYS = max(VBUS, VBAT)  │
│   │                │                                         │
│   └────────────────┤                                         │
│                    ▼                                         │
│              VSYS (3.5-5V)                                   │
│                    │                                         │
│                    ▼                                         │
│            TPS6521815 PMIC                                   │
│              │                                               │
│    ┌─────────┼─────────┬─────────┬─────────┬─────────┐      │
│    ▼         ▼         ▼         ▼         ▼         ▼      │
│  DCDC1    DCDC2    DCDC3    DCDC4    LDO1     LDO2          │
│  1.2V     3.3V     1.35V    1.8V     1.0V     1.8V          │
│  VDDCORE  VDDIO   DDR3L    eMMC/    VDDUSB   FPGA VCCIO     │
│  1.5A      2A      1.5A     NAND     200mA    300mA          │
│                             1.5A                             │
│                                                              │
│  Power Budget:                                               │
│  ┌──────────────┬────────┬────────┬──────────┐              │
│  │ Rail         │ Voltage│ Current│ Power    │              │
│  ├──────────────┼────────┼────────┼──────────┤              │
│  │ VDDCORE      │ 1.2V   │ 400mA  │ 0.48W    │              │
│  │ VDDIO (3.3V) │ 3.3V   │ 300mA  │ 0.99W    │              │
│  │ DDR3L        │ 1.35V  │ 350mA  │ 0.47W    │              │
│  │ VDDIO (1.8V) │ 1.8V   │ 200mA  │ 0.36W    │              │
│  │ VDDUSB       │ 1.0V   │ 50mA   │ 0.05W    │              │
│  │ FPGA VCCIO   │ 1.8V   │ 100mA  │ 0.18W    │              │
│  │ FPGA VCC     │ 1.2V   │ 50mA   │ 0.06W    │              │
│  │ OLED + LEDs  │ 3.3V   │ 30mA   │ 0.10W    │              │
│  │ Target Flash │ 1.8/3.3│ 100mA  │ 0.33W    │              │
│  ├──────────────┼────────┼────────┼──────────┤              │
│  │ TOTAL        │        │        │ ~3.0W    │              │
│  └──────────────┴────────┴────────┴──────────┘              │
│                                                              │
│  Battery: 2000 mAh × 3.7V = 7.4 Wh → ~2.5 hours active       │
└─────────────────────────────────────────────────────────────┘
```

## 1.10 FPGA Role & Justification

The Lattice iCE40UP5K FPGA (48-QFN, 5280 LUTs, 120 kbit block RAM, 2× SPI hard IP) serves three critical functions:

### 1. NAND Flash Timing Controller
The STM32H743's FMC peripheral supports NAND flash but with fixed timing parameters that may not match all ONFI/Toggle-mode devices. The FPGA sits between the FMC and the NAND target, providing:
- Programmable timing generation (tWC, tWP, tWH, tRC, tRP, tREH) with 5 ns resolution
- Automatic ECC calculation bypass (raw data capture including OOB)
- Command/address latch sequencing
- R/B# monitoring with programmable timeout
- 4 KB page capture FIFO (dual-port block RAM)

### 2. Voltage-Level Translation
The FPGA's I/O banks can be configured for 1.8V, 2.5V, or 3.3V signaling, allowing direct connection to NAND targets at their native voltage without external level shifters.

### 3. Signal Integrity & Glitch Protection
The FPGA provides Schmitt-trigger inputs on all NAND data lines, filtering out ringing and ground-bounce artifacts common in ISP (in-system programming) scenarios where long probe wires are used.

### FPGA-to-MCU Interface
Communication between the STM32H743 and the iCE40UP5K uses SPI (OCTOSPI1 in regular SPI mode) at 20 MHz, plus dedicated GPIO lines for interrupt (FPGA→MCU) and reset (MCU→FPGA). The FPGA configuration bitstream is stored in the STM32's internal flash and loaded via SPI slave configuration mode on boot.

## 1.11 Form Factor & Mechanical

| Parameter | Value |
|-----------|-------|
| PCB Dimensions | 85 mm × 54 mm (credit card size) |
| PCB Thickness | 1.6 mm, 4-layer |
| Enclosure | 3D-printed ABS, snap-fit |
| Weight | ~45g (PCB + battery + enclosure) |
| Connectors | USB-C (data + power), microSD slot, 20-pin ISP header, eMMC BGA adapter socket |
| Display | 0.96" 128×64 OLED, top face |
| Buttons | 4× tactile, side edge |
| Target Adapters | Separate adapter boards for TSOP-48, BGA-153, SOIC-8 test clip |

## 1.12 Software Architecture Overview

### Firmware (STM32H743, bare-metal C)
```
┌─────────────────────────────────────────┐
│              main.c                      │
│  System init, CLI parser, task dispatch  │
├─────────────────────────────────────────┤
│  board.h          registers.h           │
│  Pin definitions   MMIO register map     │
├─────────────────────────────────────────┤
│  drivers/                               │
│  ├── emmc.c/h      eMMC 5.1 driver      │
│  ├── nand.c/h      NAND ONFI driver     │
│  ├── spinor.c/h    SPI NOR driver       │
│  ├── fpga.c/h      FPGA interface       │
│  ├── sdram.c/h     DDR3L init + mgmt    │
│  ├── usb_device.c/h USB 3.0 bulk device │
│  ├── sdcard.c/h    microSD FatFS        │
│  ├── oled.c/h      SSD1306 display      │
│  └── hash.c/h      SHA-256 accelerator  │
├─────────────────────────────────────────┤
│  usb_descriptors.h                      │
│  USB device + BOS descriptors           │
└─────────────────────────────────────────┘
```

### Companion App (React Native)
```
┌─────────────────────────────────────────┐
│  App.js — Navigation container          │
├─────────────────────────────────────────┤
│  screens/                               │
│  ├── AcquisitionScreen.js               │
│  │   Device selection, progress, control │
│  └── AnalysisScreen.js                   │
│      Hex viewer, partition table, hash  │
├─────────────────────────────────────────┤
│  components/                            │
│  └── HexViewer.js                        │
│      Virtualized hex dump with search   │
├─────────────────────────────────────────┤
│  utils/                                 │
│  └── protocol.js                         │
│      Binary wire protocol, CRC, framing │
└─────────────────────────────────────────┘
```

## 1.13 Development & Build Toolchain

| Component | Toolchain |
|-----------|-----------|
| MCU Firmware | arm-none-eabi-gcc 12.3, STM32CubeH7 HAL, Make |
| FPGA Bitstream | Yosys + nextpnr-ice40 + IceStorm (open-source) |
| KiCad PCB | KiCad 8.0, 4-layer, 0.1mm trace/space |
| Companion App | React Native 0.76, react-native-usb, Metro |
| Mechanical | FreeCAD / OpenSCAD for enclosure |

## 1.14 Risk Analysis & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| eMMC HS400 signal integrity | Medium | High | 4-layer PCB, controlled impedance, length-matched traces |
| NAND timing incompatibility | Medium | Medium | FPGA programmable timing, ONFI parameter page detection |
| DDR3L layout failure | Low | High | Fly-by topology, length-matched within 5 mil, reference layout from STM AN5122 |
| USB 3.0 enumeration failure | Low | Medium | Follow STM USB HS reference design, ULPI PHY USB3320 |
| BGA soldering (eMMC adapter) | High | Low | Adapter board outsourced to PCB fab with assembly |
| Battery over-discharge | Low | Medium | BQ25896 protection, firmware low-battery shutdown at 3.3V |
| Target flash damage from overvoltage | Medium | High | FPGA I/O voltage detection, auto-ranging level shifters, current limiting |
| FPGA bitstream corruption | Low | Medium | CRC check before load, golden image in protected flash sector |

## 1.15 Compliance & Legal

This device is designed for **legitimate security research, forensic investigation, data recovery, and educational purposes**. Users are responsible for complying with all applicable laws regarding device tampering, data access, and privacy. The design includes no features specifically intended to circumvent DRM, access control, or other technological protection measures in violation of DMCA or similar laws.

The open-source release includes full schematics, PCB layout, firmware source, and FPGA gateware under permissive licenses (MIT for software, CERN OHL-S for hardware).

---

*End of Phase 1 — Conceptual Architecture*
