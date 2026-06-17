# Forge-Probe — Silicon Backdoor Debug Probe

```
   ╔══════════════════════════════════════════════════════════════╗
   ║   ███████╗ ██████╗ ██████╗  ██████╗ ███████╗              ║
   ║   ██╔════╝██╔═══██╗██╔══██╗██╔════╝ ██╔════╝              ║
   ║   █████╗  ██║   ██║██████╔╝██║  ███╗█████╗                ║
   ║   ██╔══╝  ██║   ██║██╔══██╗██║   ██║██╔══╝                ║
   ║   ██║     ╚██████╔╝██║  ██║╚██████╔╝███████╗              ║
   ║   ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝              ║
   ║   ██████╗ ██████╗  ██████╗ ██████╗ ███████╗               ║
   ║   ██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝               ║
   ║   ██████╔╝██████╔╝██║   ██║██████╔╝█████╗                 ║
   ║   ██╔═══╝ ██╔══██╗██║   ██║██╔══██╗██╔══╝                 ║
   ║   ██║     ██║  ██║╚██████╔╝██████╔╝███████╗               ║
   ║   ╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝               ║
   ║                                                              ║
   ║   Silicon Backdoor Debug Probe v1.0                          ║
   ║   Author: jayis1                                             ║
   ╚══════════════════════════════════════════════════════════════╝
```

> **⚠️ Legal & Ethical Notice:** Forge-Probe is designed exclusively for **authorized security research, penetration testing, hardware reverse engineering, and vulnerability assessment**. Unauthorized access to computer systems, extraction of proprietary firmware, or bypassing security protections without explicit written permission is illegal in most jurisdictions. The author (jayis1) assumes no liability for misuse. **Always obtain proper written authorization** before testing any system you do not own. This tool is intended solely for defensive security research, red team engagements with written authorization, hardware CTF competitions, educational purposes, and legitimate device debugging.

---

## Table of Contents

1. [Overview](#overview)
2. [What Makes Forge-Probe Unique](#what-makes-forge-probe-unique)
3. [Attack Surface & Threat Model](#attack-surface--threat-model)
4. [Hardware Specifications](#hardware-specifications)
5. [Architecture & Block Diagram](#architecture--block-diagram)
6. [Firmware Details & Design Decisions](#firmware-details--design-decisions)
7. [Protocol Support](#protocol-support)
8. [Application/Software Interface](#applicationsoftware-interface)
9. [Use Cases for Red Teams & Security Researchers](#use-cases-for-red-teams--security-researchers)
10. [Building & Flashing](#building--flashing)
11. [Companion App](#companion-app)
12. [License & Credits](#license--credits)

---

## Overview

**Forge-Probe** is a pocket-sized, FPGA-accelerated JTAG/SWD/cJTAG debug interface platform purpose-built for hardware security research. It is a **universal silicon backdoor probe** — designed to discover, enumerate, and interact with embedded debug interfaces on virtually any microcontroller, CPU, or SoC that follows industry-standard IEEE 1149.1 (JTAG), ARM Serial Wire Debug (SWD), or IEEE 1149.7 (cJTAG/Compact JTAG) protocols.

Unlike general-purpose debug probes (J-Link, ST-Link, OpenOCD-based adapters) that assume you already know the target and want to program/debug it, Forge-Probe is designed for the **security researcher's workflow**: plug in blindly to an unknown board, auto-detect the protocol, discover the architecture, map the memory space, identify security protections (RDP, flash lock, secure debug), dump flash contents, perform boundary scan analysis, fuzz MEM-AP interfaces for hidden memory regions, and log everything — all without any prior knowledge of the target.

At its core, Forge-Probe pairs an **STM32H743IIK6** Cortex-M7 microcontroller running at 480 MHz with a **Lattice iCE40UP5K** FPGA for hardware-accelerated debug protocol timing. This hybrid architecture allows software-defined protocol flexibility (the MCU handles the protocol logic) while the FPGA provides cycle-accurate TCK/SWCLK generation up to 120 MHz, hardware bit-banging for bit-bang mode, and large buffer management for boundary scan chains up to 32K cells.

---

## What Makes Forge-Probe Unique

| Feature | Typical Debug Probe | Forge-Probe |
|---------|-------------------|-------------|
| **Protocol auto-detection** | Manual selection | Automatic SWD → JTAG → cJTAG fallback |
| **Target discovery** | Requires config | Blind plug-and-play enumeration |
| **RDP/flash lock detection** | Not available | Integrated option byte analysis |
| **Boundary scan** | Requires separate tool | Built-in BS chain capture + interpretation |
| **MEM-AP fuzzing** | Not available | Automated hidden region discovery |
| **Security state fingerprinting** | Manual analysis | Automated RDP level, SPIDEN, debug level |
| **FPGA acceleration** | Rare (only high-end probes) | iCE40UP5K for 120 MHz TCK |
| **Voltage level agility** | Fixed 3.3V or 5V | Configurable 1.8V / 2.5V / 3.3V / 5.0V |
| **Form factor** | Dongle/box | 55×30 mm pocketable PCB |
| **Open source** | Often proprietary | **Fully open** (CERN-OHL-S hardware, GPL firmware, MIT app) |
| **Standalone operation** | Requires PC | Headless + SD card logging, smartphone app via USB/BLE |

---

## Attack Surface & Threat Model

### Attack Surface

Forge-Probe targets the **debug and test infrastructure** embedded in virtually all modern silicon:

1. **JTAG (IEEE 1149.1) TAP interface** — Present on >95% of MCUs, CPUs, FPGAs, and SoCs. Provides direct access to boundary scan, IDCODE, and (on ARM devices) the Debug Port.

2. **Serial Wire Debug (SWD)** — ARM-standard 2-wire debug interface. Present on Cortex-M0/M3/M4/M7/M23/M33/M55 and many Cortex-R and Cortex-A implementations. Provides full read/write access to memory and core registers when unlocked.

3. **Compact JTAG (IEEE 1149.7/cJTAG)** — 2-wire JTAG variant used in space-constrained applications (wearables, IoT, mobile SoCs). Uses JLOC protocol layer for advanced power management.

4. **CoreSight Debug Infrastructure** — ARM's debug and trace architecture. Includes ROM tables (component discovery), ETM/ITM trace, DWT data watchpoints, FPB flash patching/breakpoints, and cross-triggering (CTI).

5. **Flash Readout Protection (RDP)** — MCU-level access control that gates debug port access. Forge-Probe detects RDP Level 0 (open), Level 1 (partial lockdown), and Level 2 (permanent brick) — and can attempt bypass on Level 1 targets.

6. **Boundary Scan Chain** — Digital "MRI" of the chip's I/O pins. Forge-Probe can capture and interpret the entire pin state of a running system through the JTAG boundary scan register, revealing signal activity, pin configurations, and board-level connectivity.

### Threat Model

| Threat Actor | Capability | Surface Area |
|---|---|---|
| **Red Team** | Physical access during engagement | JTAG/SWD headers, test points, exposed vias |
| **Penetration Tester** | Authorized hardware assessment | OBD-II, debug UART, JTAG/SWD on evaluation boards |
| **Firmware Analyst** | Reverse engineering | Flash dump, ROM extraction, register state capture |
| **Hardware CTF Competitor** | Challenge board interaction | JTAG/SWD, boundary scan for embedded puzzles |
| **Security Researcher** | Vulnerability discovery | MEM-AP fuzzing, debug register inspection, trace capture |

**Blind spots** that Forge-Probe addresses:
- Unknown/first-look hardware where no debug config exists
- Locked-down targets where someone has attempted (but failed) full RDP Level 2
- Legacy/industrial gear with JTAG headers but unknown pinout
- SoCs with multiple TAPs in daisy-chain configurations
- Devices with cJTAG where no standard tool offers support

---

## Hardware Specifications

### Core Components

| Component | Part Number | Purpose |
|---|---|---|
| **MCU** | STM32H743IIK6 (BGA176) | Main processor: protocol logic, USB stack, SD card, LUA scripting |
| **FPGA** | Lattice iCE40UP5K-SG48 (QFN48) | Hardware acceleration: TCK gen, bit-bang, buffer mgmt, BS capture |
| **Regulator (3.3V)** | LTPS7A47 | Low-noise main rail power |
| **Regulator (1.2V)** | LTPS7A47 | FPGA core voltage |
| **Level Shifter** | TXB0108 (×2) | 8-channel bidirectional for MPIO 1.8V–5V translation |
| **Current Sensor** | INA219 | I²C target current/voltage monitoring |
| **Crystal** | 25 MHz | HSE for MCU PLL (480 MHz core) |
| **Flash** | W25Q256JV (on FPGA SPI) | FPGA configuration storage + bulk data buffer |

### Debug Interfaces

| Interface | Connector | Pins | Features |
|---|---|---|---|
| **JTAG** | 20-pin ARM standard header (2.54mm) | TCK, TMS, TDI, TDO, nTRST, nSRST, RTCK, VTREF | Up to 120 MHz TCK (FPGA accel), daisy-chain up to 4 TAPs |
| **SWD** | 10-pin Cortex standard header (2.54mm) | SWCLK, SWDIO, SWO, nSRST, VTREF | Up to 60 MHz SWCLK, SWO trace capture |
| **cJTAG** | 2-pin (shared with SWD header) | TCKC, TMSC | IEEE 1149.7 JLOC Layer 0/1 support |
| **MPIO** | 16-pin 2.54mm header | 16 configurable IO channels | UART, SPI, I2C, 1-Wire, PWM, GPIO |

### Connectivity

| Interface | Type | Speed | Details |
|---|---|---|---|
| **USB** | USB-C (OTG HS) | 480 Mbps | ULPI external PHY, vendor-specific bulk endpoints |
| **BLE** | Optional nRF52840 add-on module | 1 Mbps | Not fitted by default, header provided |
| **MicroSD** | SDMMC 4-bit | 48 MHz | Flash dump storage up to 32 GB |
| **Console UART** | 3.3V TTL | 115200 baud | Debug console (via CP2102 or direct) |

### Power

| Rail | Voltage | Max Current | Source |
|---|---|---|---|
| **USB VBUS** | 5.0V | 1.5A | USB-C (1.5 A @ 3.0V advertised) |
| **Main Logic** | 3.3V | 500 mA | LTPS7A47 from VBUS |
| **FPGA Core** | 1.2V | 200 mA | LTPS7A47 from 3.3V |
| **Target Supply** | 1.8V / 2.5V / 3.3V / 5.0V | 100 mA (each) | Selectable via GPIO-controlled LDOs |
| **Sleep** | 3.3V | ~5 mA | USB suspend + FPGA shutdown |

### Physical

| Parameter | Value |
|---|---|
| **PCB Dimensions** | 55 × 30 mm |
| **PCB Layers** | 6-layer (signal-signal-plane-plane-signal-signal) |
| **PCB Thickness** | 0.8 mm (thin for pocket carry) |
| **Copper Finish** | ENIG (gold) for durability |
| **Weight** | ~15 g (with headers) |
| **Mounting** | 4× M3 mounting holes |
| **Enclosure** | Custom 3D-printed ABS (STL provided) |

---

## Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Forge-Probe Architecture                          │
│                                                                             │
│  ┌─────────┐   SPI 60MHz    ┌──────────────┐    JTAG/SWD/cJTAG Pins        │
│  │ iCE40UP5K│◄──────────────►│ STM32H743IIK6│◄──────────────────────────────►│
│  │  FPGA   │                │   Cortex-M7  │    ┌──────────────────┐       │
│  │         │                │   480 MHz    │    │ 20-pin JTAG      │       │
│  │• TCK gen│                │              │    │ 10-pin SWD       │       │
│  │• Bitbang│                │• Protocol    │    │ 2-pin cJTAG      │       │
│  │• BS buf │                │  state mach. │    └──────────────────┘       │
│  │• DR buf │                │• USB stack   │                               │
│  │• PLL    │                │• SD card FS  │    ┌──────────────────┐       │
│  │         │                │• Lua engine  │    │ 16-pin MPIO      │       │
│  │ W25Q256 │                │• Command proc│    │ (UART/SPI/I2C/1W)│       │
│  │ Config  │                │              │    └──────────────────┘       │
│  └─────────┘                └─────┬────────┘                               │
│                                    │                                        │
│          ┌─────────────────────────┼─────────────────────────┐              │
│          │                         │                         │              │
│          ▼                         ▼                         ▼              │
│  ┌──────────────┐       ┌──────────────────┐      ┌──────────────────┐     │
│  │ USB-C (ULPI) │       │ MicroSD (48 MHz) │      │ INA219 Current   │     │
│  │ 480 Mbps     │       │ Flash dump store │      │ Sensor (I²C)     │     │
│  └──────────────┘       └──────────────────┘      └──────────────────┘     │
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                     Voltage Rail Matrix                              │  │
│  │  1.8V ──► Selectable ──► Target VIO   (for level-shifted debug)    │  │
│  │  2.5V ──► Selectable ──► Target VIO                                  │  │
│  │  3.3V ──► Default ──► Target VIO                                    │  │
│  │  5.0V ──► Legacy ──► Target VIO                                     │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Data Flow

1. **Detection Phase**: MCU sweeps protocols (SWD → JTAG → cJTAG) through the MPIO multiplexer, reading IDCODE/DPIDR to identify the target.
2. **Enumeration Phase**: For SWD targets, MCU powers up the DAP, discovers APs via SELECT/IDR reads, walks the CoreSight ROM table, and reads MEM-AP BASE to locate system memory.
3. **Resource Phase**: Based on discovered architecture, the MCU selects appropriate clock speeds, FPGA acceleration profiles, and voltage levels.
4. **Operation Phase**: Commands (read/write/dump/fuzz/scan) flow from USB host → MCU command processor → protocol engine → FPGA timing generator → target. Responses flow back through the same chain.
5. **Storage Phase**: Flash dumps and boundary scan captures write to microSD card in FAT32 format, organized by target IDCODE and timestamp.

---

## Firmware Details & Design Decisions

### Architecture Overview

The firmware is written in **C11** targeting the STM32H743's Cortex-M7 core (with FPv5-D16 FPU). The build system uses GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`) with link-time optimization (`-flto`) and aggressive size/speed optimization (`-O2` or `-Os`).

**Key design decisions:**

#### 1. FPGA vs Bit-Bang Tradeoff
While the STM32H743 can bit-bang TCK at up to ~50 MHz using tight DWT-timed loops, the FPGA handles:
- **>50 MHz TCK** — essential for modern Cortex-M7 and Cortex-A targets that support high-speed debugging
- **Boundary scan chain buffers** — chains can be 32K+ cells; shifting this many bits through GPIO bit-bang would stall the MCU for seconds. The FPGA manages shift-DR entirely in hardware.
- **cJTAG JLOC layer** — cJTAG's 2-wire protocol requires precise timing windows that are difficult to guarantee under interrupt latency.

#### 2. Protocol Detection Strategy
The firmware implements a **probabilistic waterfall** detection:
- Try SWD first (2 pins, lowest bar to entry) by sending a line reset and reading DPIDR
- Fall back to JTAG (4 pins, most common) via IDCODE read
- Try cJTAG last (least common, requires specialized JLOC handshake)
- If nothing responds, try SWJ (SWD-over-JTAG transition sequence)

This ensures maximum compatibility without user configuration.

#### 3. MEM-AP Fuzzing Engine
The MEM-AP fuzzer (in `automation_scan_devices()`) reads well-known memory addresses and reports non-trivial values. It specifically targets:
- Flash base regions (0x08000000, 0x00000000)
- SRAM (0x20000000, 0x10000000, 0x30000000)
- Peripheral buses (0x40000000, 0x50000000)
- System control space (0xE0000000)
- Option byte regions (0x1FFF7000, 0x1FFFC800)
- FPGA/SDRAM windows (0x60000000, 0x80000000, 0xC0000000)

Non-0x00000000 and non-0xFFFFFFFF reads indicate live memory regions. Combined with auto-increment reads, this maps the target's accessible address space.

#### 4. Flash Security Bypass
Option byte probing identifies RDP level by reading well-known vendor locations:
- STM32F0: 0x1FFF7000
- STM32L0: 0x1FFFC800
- STM32F4/H7: 0x40022000/0x40023C10

If RDP Level 1 is detected (payload 0xAA or 0x5A), Forge-Probe attempts key injection via FLASH_OPTKEYR with the standard unlock sequence (0x08192A3B, 0x4C5D6E7F). This works on targets where the RDP was set but the debug port wasn't fully disabled (common misconfiguration).

#### 5. Logging & Audit Trail
Every transaction is logged to an internal ring buffer and optionally to the SD card in CSV format:
- Timestamp + protocol + command + response + error status
- Full hexdumps of IDCODE, DPIDR, AP IDR, ROM table entries
- Flash dump integrity checksums (CRC32 per sector)

This provides a complete audit trail for penetration testing reports.

#### 6. Safety Protections
- **Overcurrent protection**: INA219 monitors target VIO. If current exceeds 100 mA, rail is disconnected.
- **Voltage qualification**: Forge-Probe measures VTREF before driving any signals. If VTREF is absent (< 500 mV), all outputs remain high-impedance.
- **Watchdog**: 30-second IWDG prevents lockups during long boundary scans or flash dumps.

---

## Protocol Support

### JTAG (IEEE 1149.1)
- **TAP states**: Full 16-state TAP controller state machine
- **Instructions**: BYPASS, IDCODE, EXTEST, SAMPLE/PRELOAD, INTEST, CLAMP, HIGHZ, ARM DPACC/APACC
- **Daisy chains**: Up to 4 TAPs auto-detected with IR length probing
- **Clock**: 4 kHz – 120 MHz (FPGA), 4 kHz – 50 MHz (MCU bitbang)
- **RTCK support**: Adaptive clocking for targets with variable TCK tolerance

### SWD (ARM Serial Wire Debug)
- **Line reset**: 50 + 8 cycle sequence
- **DP registers**: DPIDR, CTRL/STAT, SELECT, RDBUFF, TARGETID (DPv3+)
- **AP registers**: CSW, TAR, DRW, BD0-3, CFG, BASE, IDR
- **DPv2/v3**: Supports DPv3 extended SELECT for 256 APs
- **SWO trace**: UART-style serial wire output capture (optional)

### cJTAG (IEEE 1149.7)
- **JLOC Layer 0**: Basic 2-wire scan
- **JLOC Layer 1**: Advanced power management, scan frequency negotiation
- **Operations**: Shift, Halt, TLRS, Control, Transfer, Resume

---

## Application/Software Interface

### USB Protocol (Vendor-Specific Bulk Endpoints)

| EP | Direction | Type | Max Packet | Purpose |
|---|---|---|---|---|
| EP0 | Control | Standard | 64 | Device enumeration |
| EP1 | OUT | Bulk | 64 | Command input (USB → Device) |
| EP2 | IN | Bulk | 64 | Response output (Device → USB) |
| EP3 | OUT | Bulk | 512 | Data write (USB → Device, flash programming) |
| EP4 | IN | Bulk | 512 | Data read (Device → USB, flash dump streaming) |
| EP5 | IN | Bulk | 512 | High-speed stream (Flash dump, boundary scan) |

### Command Set (Byte 0 = Opcode)

| Opcode | Command | Payload | Response | Description |
|---|---|---|---|---|
| `0x01` | IDENTIFY | None | 64-byte descriptor | Get device + target info |
| `0x02` | READ_MEMORY | addr(4) + len(4) | data(len) | Read target memory |
| `0x03` | WRITE_MEMORY | addr(4) + len(4) + data(len) | ACK | Write target memory |
| `0x04` | SCAN | None | Target descriptor | Auto-detect and enumerate |
| `0x05` | DUMP_FLASH | start(4) + len(4) | Stream(EP5) | Dump flash to SD + stream |
| `0x06` | FUZZ_MEM_AP | None | Text log | Fuzz MEM-AP for hidden regions |
| `0x07` | RESET_TARGET | None | ACK | Pulse nSRST |
| `0x08` | HALT_TARGET | None | ACK | Halt Cortex-M core |
| `0x09` | RESUME_TARGET | None | ACK | Resume Cortex-M core |
| `0x0A` | READ_REGISTER | reg_num(1) | reg_num(1) + value(4) | Read core register |
| `0x0B` | BOUNDARY_SCAN | None | chain_length(4) | Capture BS chain |
| `0x0C` | SET_CLOCK | freq_hz(4) | ACK | Set TCK/SWCLK speed |
| `0x0D` | SET_PROTOCOL | protocol(1) | ACK | Force protocol selection |

---

## Use Cases for Red Teams & Security Researchers

### 1. First-Look Hardware Assessment
**Scenario**: Red team gains brief physical access to an embedded device during a site engagement.

**Workflow**:
1. Connect Forge-Probe's SWD header (or probe test points)
2. Device auto-detects protocol, enumerates the target
3. Within 5 seconds: target architecture, flash protection status, memory map known
4. If unlocked: full flash dump to SD card in 60 seconds (2 MB @ 48 MHz SWCLK)
5. Pocket the probe and SD card, leave no trace

**Intel gained**: Complete firmware image, core register state at time of connection, boundary scan pin state, ROM table component inventory.

### 2. IP Theft & Firmware Extraction Countermeasure Assessment
**Scenario**: Manufacturer wants to verify their RDP/secure debug implementation is effective.

**Workflow**:
1. Connect to production board via JTAG/SWD
2. Forge-Probe reports "RDP Level 1 — flash locked"
3. Attempt key injection bypass
4. If bypass succeeds: manufacturer knows their implementation is flawed
5. If bypass fails: manufacturer can validate protection effectiveness

### 3. Automotive ECU Reverse Engineering
**Scenario**: Researcher analyzing an engine control unit's firmware for vulnerabilities.

**Workflow**:
1. Locate JTAG or SWD pads on ECU PCB (often accessible with the case open)
2. Forge-Probe's 1.8V/3.3V/5V rail selection matches legacy/vintage ECU voltage domains
3. Detect MPC5xxx/TMS570/Infineon architecture via IDCODE readback
4. Boundary scan reveals external flash interface pin states
5. MEM-AP fuzzing discovers shadow memory regions (often used for calibration data)

### 4. Hardware CTF Exploitation
**Scenario**: CTF challenge presents a blinking LED board with a debug header and a "locked" flag stored in flash.

**Workflow**:
1. Auto-detect: SWD protocol, Cortex-M4 architecture
2. MEM-AP fuzzing reveals hidden memory region at non-standard address
3. Boundary scan shows one output pin driving high — the LED, but another pin is tied to a tamper GPIO
4. Write tamper GPIO low via MEM-AP to disable tamper logic
5. Full flash dump reveals flag at offset 0x00010000

### 5. IoT Botnet Forensics
**Scenario**: Security researcher analyzing a compromised IoT camera.

**Workflow**:
1. Connect to UART console via MPIO (115200 baud) — get boot log
2. Switch to SWD while target is running (no reset needed)
3. Read current PC, stack pointer, and XPSR to determine execution state
4. Dump flash to capture modified bootloader + injected malware
5. Boundary scan captures GPIO states to identify which peripherals are active

### 6. FIPS 140-3 / Common Criteria Evaluation
**Scenario**: Lab evaluating a cryptographic module's tamper response.

**Workflow**:
1. Boundary scan captures full pin state of the module under normal operation
2. Trigger tamper event (thermal, voltage, or optical glitch)
3. Boundary scan captures pin state during tamper response
4. Compare captures to verify that sensitive key material pins are properly zeroized

---

## Building & Flashing

### Prerequisites

```bash
# Install ARM GCC toolchain (Ubuntu/Debian)
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi openocd

# Or from ARM's official site
curl -Lo gcc-arm-none-eabi.tar.bz2 \
  https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
tar xjf gcc-arm-none-eabi.tar.bz2
export PATH=$PATH:$(pwd)/gcc-arm-none-eabi-10.3-2021.10/bin
```

### Build Firmware

```bash
cd forge-probe/firmware
make all              # Build ELF + HEX + BIN
make size             # Show code/data/bss sizes
make debug            # Build with debug symbols
```

### Flash via ST-Link

```bash
make flash            # Via OpenOCD + ST-Link
# Or manually:
openocd -f interface/stlink-v3.cfg -f target/stm32h7x.cfg \
  -c "program build/forge-probe.elf verify reset exit"
```

### Companion App

```bash
cd forge-probe/app
npm install
npx react-native run-android  # For Android
npx react-native run-ios      # For iOS (requires Xcode)
```

---

## Companion App Features

The React Native companion app provides:
- **Dashboard**: Connection management (USB/BLE), target status, quick actions
- **Memory Explorer**: Hex dump viewer, address/length input, byte/half/word access, write support, export to share
- **Flash Dump**: Progress-indicated dump to device storage with export capability
- **Register Viewer**: Full Cortex-M register dump (R0-R12, SP, LR, PC, XPSR)
- **Boundary Scan**: Chain length display, simplified I/O state visualization
- **Settings**: Protocol selection, clock speed, voltage rail selection, auto-scan toggle

---

## License & Credits

| Component | License | Author |
|---|---|---|
| **Hardware (KiCad design)** | CERN-OHL-S-2.0 | jayis1 |
| **Firmware** | GPL-2.0-only | jayis1 |
| **Companion App** | MIT | jayis1 |
| **Documentation** | CC-BY-4.0 | jayis1 |

**Author**: jayis1

**Contributions**: This device was originally conceived and designed by jayis1. All firmware, hardware schematics, PCB layout, companion application, and documentation are original works.

### Third-Party Components Used
- **STM32H7 HAL/LL** — STMicroelectronics (BSD-3-Clause)
- **Newlib Nano** — Red Hat (BSD-like)
- **Lattice iCE40** — Lattice Semiconductor
- **iCE40UP5K** — Lattice Semiconductor

### Disclaimer
FORGE-PROBE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. THE AUTHOR DISCLAIMS ALL LIABILITY FOR ANY DAMAGES ARISING FROM USE OF THIS DEVICE. USE RESPONSIBLY AND LEGALLY.

---

*"The best way to secure silicon is to understand how it can be compromised." — jayis1*