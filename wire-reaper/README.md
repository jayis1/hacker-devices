# WireReaper — Multi-Bus Embedded Peripheral Infiltrator

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Unauthorized interception, modification, or injection of data on embedded buses in devices you do not own may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), and data protection regulations in your jurisdiction. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## Overview

**WireReaper** is a pocket-sized, battery-operated hardware tool for **multi-bus embedded peripheral security research**. It simultaneously or selectively monitors, captures, decodes, and injects traffic on the four most common embedded serial buses found in IoT devices, industrial sensors, automotive ECUs, access controllers, and consumer electronics:

| Bus | Role | Typical Targets |
|-----|------|-----------------|
| **I²C** | Sniff + master + slave emulation | EEPROMs, secure elements, sensors, PMICs |
| **SPI** | Sniff + master + slave emulation | Flash chips, displays, crypto accelerators |
| **UART** | Sniff + inject | Debug consoles, inter-MCU comms, GPS modules |
| **1-Wire** | Sniff + master emulation | iButton, DS24xx EEPROMs, temperature sensors |

The device sits physically in-line between a target's peripheral and its host controller (or clips onto exposed test pads), captures all bus traffic at wire speed into a circular DMA-backed capture buffer, decodes it into human-readable transactions in real time, and can autonomously inject crafted packets, replay captured sequences, or emulate a missing peripheral to extract secrets from the host.

What makes WireReaper novel compared to logic analyzers or bus pirates:

1. **Live multi-bus concurrency** — it can sniff I²C and SPI simultaneously on independent channels while also bridging a UART console, something no $30 bus pirate or Saleae clone can do.
2. **Protocol-aware decoding on-device** — transactions are decoded to address/register/value triples on the MCU, not just raw samples, so the operator app sees structured data immediately.
3. **Autonomous injection & emulation** — scripted attack payloads (e.g., "dump EEPROM 0x50", "fuzz SPI register 0x00", "emulate absent sensor") run on-device without a tethered PC.
4. **Encrypted BLE + USB CDC backhaul** — the operator controls the device from a phone over encrypted BLE or from a laptop over USB CDC, with captured data streamed in real time.

---

## Attack Surface and Threat Model

### What WireReaper Exposes

Embedded devices routinely expose their internal peripheral buses on unpopulated headers, test pads, or even populated connectors. Security researchers and red teams can use WireReaper to:

- **Extract firmware & secrets from SPI flash** — clip onto an external SPI NOR/NAND flash chip, emulate the host controller, and issue read commands to dump the full contents, including bootloader, config, and embedded keys.
- **Dump I²C EEPROMs and secure elements** — read out configuration EEPROMs, clone them, or fuzz the I²C address space to discover hidden devices.
- **Sniff debug UART consoles** — passively monitor a target's UART console to capture boot logs, credentials, and command shells without interfering with signal integrity.
- **Inject into UART consoles** — send commands to an exposed UART shell to escalate privileges, drop payloads, or configure the target.
- **1-Wire key/sensor cloning** — read iButton or DS24xx ROM IDs and EEPROM pages, enabling physical access-control bypass research.
- **Fuzz peripheral interfaces** — autonomously generate malformed I²C/SPI transactions to find parsing bugs in host drivers (peripheral fuzzing) or peripheral firmware (host fuzzing).
- **Replay attacks** — capture a legitimate transaction sequence and replay it to trigger guarded functionality (e.g., unlock commands, relay toggles).

### Threat Model

| Asset | Adversary Capability | WireReaper Role |
|-------|---------------------|-----------------|
| SPI flash contents | Physical access to PCB / test pads | Read-out via master emulation |
| I²C EEPROM config | Physical access to bus lines | Read/write/fuzz |
| UART console creds | Physical access to TX/RX/GND | Passive sniff + active inject |
| 1-Wire device ROM | Physical contact with bus | ROM read + clone |
| Host peripheral driver | In-line bus tap | Injection / fuzzing |

**Preconditions:** WireReaper requires **physical access** to the target's bus lines. It is a hardware-level tool, not a remote network exploit. It does not bypass encryption on buses that implement link-layer crypto (though it can still capture the ciphertext for offline analysis).

**Out of scope:** WireReaper does not attack JTAG/SWD (see GHOSTBUS), CAN (see CAN-Creeper), or high-speed buses like MIPI/LVDS. It focuses on the low/medium-speed embedded control buses that are the workhorses of IoT and embedded systems.

---

## Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32H743VIT6 (Cortex-M7, 480 MHz, 1 MB SRAM, 2 MB flash) |
| **FPGA co-processor** | Lattice iCE40-UP5K (5K LUTs) — high-speed bus sampling & trigger logic |
| **I²C channels** | 2× (up to 1 MHz Fast-mode Plus) |
| **SPI channels** | 2× (up to 50 MHz via FPGA, up to 25 MHz via MCU) |
| **UART channels** | 4× (up to 6 MHz, independent baud) |
| **1-Wire channel** | 1× (standard & overdrive timing) |
| **Capture buffer** | 512 MB external SDRAM (IS42S32800G-6BLI) via FMC, ring-buffered |
| **BLE** | nRF52840 module (Feather-style uBlox NINA-B302) — encrypted link |
| **USB** | USB 2.0 Hi-Speed (USB3300 ULPI PHY) — CDC virtual serial |
| **SD card** | microSD slot (SDHC, FAT32) — capture logging & offline replay |
| **Power** | 3.7V LiPo (1200 mAh) + USB bus power; MCP73831 charger, fuel gauge via I²C MAX17048 |
| **Form factor** | 95 × 55 × 12 mm PCB in 3D-printed enclosure; weight ~70 g |
| **Display** | 0.96" OLED (SSD1306, 128×64) — status & active channel |
| **Input** | 4 tactile buttons (menu navigation / trigger) |
| **Voltage tolerance** | Target bus 1.2V–5.5V via bi-directional level shifters (TXS0108E) |
| **BOM cost** | ~$85 at qty 1 |

### Block Diagram

```
 ┌─────────────────────────────────────────────────────────────┐
 │                        WireReaper                            │
 │                                                              │
 │   ┌──────────┐    FMC    ┌──────────────────────────────┐    │
 │   │ STM32H743 │◄────────►│  512 MB SDRAM (capture buf)  │    │
 │   │  Cortex-M7│           └──────────────────────────────┘    │
 │   │  480 MHz  │                │                              │
 │   │           │  SPI/I2C/UART  │                              │
 │   │           │◄───────────────┤                              │
 │   │           │   GPIO IRQ      │                              │
 │   └─────┬─────┘                 │                              │
 │         │  ULPI    ┌────────────┴──────────┐                   │
 │         │◄────────►│  USB3300 HS PHY       │── USB-C           │
 │         │          └───────────────────────┘                   │
 │         │                                                    │
 │         │  USART ┌───────────────┐                           │
 │         │◄──────►│  nRF52840 BLE │── PCB antenna              │
 │         │        └───────────────┘                           │
 │         │                                                    │
 │         │  I2C  ┌───────────────┐                           │
 │         │◄─────►│  SSD1306 OLED │                           │
 │         │       │  MAX17048 FG  │                           │
 │         │       └───────────────┘                           │
 │         │                                                    │
 │         │  SPI  ┌────────────────────────────┐               │
 │         │◄─────►│  Lattice iCE40-UP5K FPGA   │               │
 │         │       │  (hi-speed sampling +      │               │
 │         │       │   trigger logic)           │               │
 │         │       └──────────┬─────────────────┘               │
 │         │                  │ parallel / SPI                   │
 │         │       ┌──────────▼─────────────────┐               │
 │         │       │  TXS0108E level shifters   │               │
 │         │       │  (1.2V–5.5V bidirectional)  │               │
 │         │       └──────────┬─────────────────┘               │
 │         │                  │                                  │
 │  microSD│                  ▼                                  │
 │  (SDIO) │         ┌────────────────┐                          │
 └─────────┤         │  Probe header  │──► I2C×2 / SPI×2 /       │
           │         │  (14-pin, 0.1")│    UART×4 / 1-Wire /      │
           │         └────────────────┘    3.3V / GND            │
           └────────────────────────────────────────────────────┘
```

---

## Architecture

### Firmware Architecture

The firmware is a bare-metal C application on the STM32H743 with a cooperative scheduler, no full RTOS (to keep latency determinism for bus timing). It is organized as:

```
firmware/
├── Makefile          — GCC ARM toolchain, -O2, Cortex-M7 FPU
├── board.h           — pin map, peripheral assignments, clock tree
├── registers.h       — STM32H7 register definitions (MMIO structs)
├── main.c            — scheduler, command dispatch, capture loop
└── drivers/
    ├── i2c.c         — I2C sniffer/master/slave (4KB)
    ├── spi.c         — SPI sniffer/master/slave (4KB)
    ├── uart.c        — UART sniffer/inject (4KB)
    ├── onewire.c     — 1-Wire master/sniffer (4KB)
    ├── fpga.c        — iCE40 SPI config + high-speed sampler (4KB)
    ├── sdram.c       — FMC SDRAM init + ring buffer (4KB)
    ├── ble_uart.c    — nRF52840 control over USART + framing (4KB)
    ├── usb_cdc.c     — USB CDC descriptors + serial endpoint (4KB)
    └── sdcard.c      — SDIO block driver + FAT32 logging (4KB)
```

### Command Protocol

The device speaks a binary command protocol over both USB CDC and BLE UART. Commands include:

| Opcode | Name | Description |
|--------|------|-------------|
| `0x01` | CAPTURE_START | Start capturing on channel(s) with trigger config |
| `0x02` | CAPTURE_STOP | Stop capture, flush buffer |
| `0x03` | CAPTURE_STREAM | Stream decoded transactions in real time |
| `0x10` | I2C_SCAN | Scan I²C bus for active addresses (0x00–0x7F) |
| `0x11` | I2C_READ | Read N bytes from addr/register |
| `0x12` | I2C_WRITE | Write data to addr/register |
| `0x13` | I2C_EMULATE | Act as I²C slave at given address |
| `0x20` | SPI_READ | Read N bytes from SPI (with command prefix) |
| `0x21` | SPI_WRITE | Write data to SPI |
| `0x22` | SPI_EMULATE | Act as SPI slave, respond to reads with buffer |
| `0x30` | UART_SNIFF | Set baud, start passive sniff |
| `0x31` | UART_INJECT | Send bytes on UART TX |
| `0x40` | ONEWIRE_SCAN | Discover 1-Wire devices (ROM IDs) |
| `0x41` | ONEWIRE_READ | Read memory/ROM page |
| `0x50` | REPLAY_LOAD | Load a captured transaction sequence |
| `0x51` | REPLAY_RUN | Replay loaded sequence (with loop count) |
| `0x60` | FUZZ_START | Start fuzzing a bus with mutation engine |
| `0x70` | STATUS | Return device status (channels active, buffer level) |
| `0x71` | VERSION | Firmware version + capabilities bitmask |
| `0xFF` | RESET | Soft reset |

### Capture Pipeline

1. **Sampling** — I²C/UART sampled by MCU peripherals (DMA into SRAM); SPI sampled by FPGA at up to 100 MS/s for high-speed modes.
2. **Decoding** — Raw samples are decoded in-line by the protocol drivers into structured transactions (addr, reg, data, RW flag, ACK/NACK).
3. **Buffering** — Decoded transactions are packed into a compact binary record format and written to the 512 MB SDRAM ring buffer.
4. **Streaming** — When a host is connected, transactions are streamed live over USB/BLE. When offline, they log to the microSD card.
5. **Triggering** — Complex triggers (address match, data value match, byte pattern) are configured via the FPGA for glitch-free capture.

---

## Firmware Design Decisions

- **Bare-metal over RTOS:** Bus-timing determinism for 1-Wire overdrive and I²C slave emulation requires sub-microsecond ISR latency. A preemptive RTOS adds jitter; a cooperative scheduler with prioritized ISRs keeps the critical path lean.
- **FPGA for high-speed SPI:** The STM32H7 SPI peripherals cap at ~25 MHz in slave mode. For 50 MHz+ flash dumping, the iCE40 samples SCK/MOSI/MISO/CS on its 48 MHz clock (with DDR sampling for effective 96 MS/s) and streams packed samples to the MCU over a parallel FIFO.
- **External SDRAM over internal SRAM:** The H743 has 1 MB SRAM, which fills in ~0.5 s of SPI traffic at 50 MHz. 512 MB of SDRAM via the FMC bus gives ~15 minutes of continuous multi-bus capture.
- **Level shifters on all channels:** IoT targets range from 1.2V sensors to 5V legacy EEPROMs. TXS0108E auto-direction-sensing shifters cover the full range without configuration.
- **Dual radio for redundancy:** BLE (nRF52840) for wireless phone control, USB CDC for laptop control. Both can be active simultaneously.
- **Encrypted BLE link:** The nRF52840 runs a minimal encrypted transport (AES-CCM with pre-shared key) so captured data isn't exposed over the air during wireless operation.

---

## Application / Software Interface

The companion app is a React Native application for Android/iOS. It provides:

- **Connection screen** — scan for BLE devices, connect to WireReaper, enter encryption key
- **Dashboard** — live status of all bus channels, buffer fill, battery
- **Capture view** — real-time decoded transaction stream (color-coded by bus)
- **I²C console** — scan, read, write, emulate
- **SPI console** — read flash, write, emulate slave
- **UART terminal** — full bidirectional serial console
- **1-Wire tools** — scan ROMs, read pages
- **Replay manager** — load saved captures, run replays
- **Fuzzer** — configure mutation parameters, start/stop, monitor crashes
- **Settings** — device config, firmware update, export captures

The app communicates with the device using the same binary command protocol over a BLE characteristic (write commands, read notifications) or a USB OTG serial connection.

---

## Use Cases

### Red Team Operations

1. **Physical access assessment:** During an on-site engagement, clip WireReaper onto a target device's exposed SPI flash header and dump the firmware in seconds for offline analysis — no laptop required, data logs to SD card.
2. **Access control bypass:** Read 1-Wire iButton ROM IDs from a door reader's bus and clone them onto a writable iButton for physical access testing.
3. **Credential extraction from UART:** Sniff a device's debug UART during boot to capture plaintext credentials, SSH keys, or admin passwords printed in boot logs.
4. **Sensor spoofing:** Emulate a temperature or pressure sensor over I²C to inject false readings into an industrial controller, testing its safety response.

### Security Researchers

1. **Peripheral driver fuzzing:** Use the autonomous fuzzer to generate malformed I²C/SPI transactions against a host controller's driver, discovering parsing vulnerabilities that could lead to code execution.
2. **Secure element analysis:** Monitor I²C traffic between a host and a secure element (e.g., ATECC508A) to study the challenge-response protocol and assess key extraction resistance.
3. **Boot flow analysis:** Capture the SPI flash read sequence during boot to understand the bootloader's loading order and identify manipulation points.
4. **Protocol reverse engineering:** Sniff an unknown I²C/SPI peripheral's traffic under normal operation and use the on-device decoder to reconstruct the command set.

### Penetration Testers

1. **Embedded device assessment:** WireReaper is the standard tool for the "chip-off without chip-off" technique — read flash in-circuit without desoldering, using the FPGA's level shifting and the MCU's SPI master mode.
2. **Configuration dump & clone:** Read an I²C config EEPROM, modify it (e.g., enable debug mode, change boot args), and write it back to alter device behavior.
3. **Live UART shell interaction:** Connect to a device's UART shell, enumerate commands, and attempt privilege escalation — all from the phone app over BLE.
4. **Replay-based unlock:** Capture the I²C sequence sent when an authorized user unlocks a device, then replay it to trigger the unlock without credentials.

---

## Repository Layout

```
wire-reaper/
├── README.md                              (this file)
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── i2c.c
│       ├── spi.c
│       ├── uart.c
│       ├── onewire.c
│       ├── fpga.c
│       ├── sdram.c
│       ├── ble_uart.c
│       ├── usb_cdc.c
│       └── sdcard.c
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── App.js
    ├── package.json
    ├── screens/
    │   ├── ConnectionScreen.js
    │   ├── DashboardScreen.js
    │   ├── CaptureViewScreen.js
    │   ├── I2cConsoleScreen.js
    │   ├── SpiConsoleScreen.js
    │   ├── UartTerminalScreen.js
    │   ├── OneWireScreen.js
    │   ├── ReplayManagerScreen.js
    │   └── FuzzerScreen.js
    ├── components/
    │   └── DeviceContext.js
    └── utils/
        └── protocol.js
```

---

## License

| Component | License |
|-----------|---------|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| React Native companion app | MIT |

**Author:** jayis1
**Copyright:** © 2026 jayis1. All rights reserved.

---

## Acknowledgments

WireReaper was designed as an original contribution to the open-source security hardware community. It fills the gap between simple bus pirates (slow, single-bus, no decode) and expensive logic analyzers (passive only, no injection or emulation). Every line of firmware, every schematic symbol, and every app screen was authored by **jayis1**.