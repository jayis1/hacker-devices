# tpm-phantom — Trusted Platform Module Bus Analyzer

> **A covert, pocket-sized hardware tool for sniffing, analyzing, and (optionally) injecting traffic on the Trusted Platform Module (TPM) bus — supporting both LPC and SPI TPM 2.0 interfaces.**

**Author:** jayis1
**License:** Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT
**Version:** 1.0
**BOM Cost:** ~$72 at 1K volume

---

## ⚖️ Legal & Ethical Disclaimer

**This device is for authorized security research, penetration testing, and red team operations ONLY.**

The tpm-phantom is designed to help security researchers understand TPM communication, audit TPM implementations, and test the security of systems that rely on TPM for measured boot, disk encryption key sealing, and platform integrity attestation.

**Unauthorized interception of TPM bus traffic on systems you do not own or do not have explicit written authorization to test may violate:**
- The Computer Fraud and Abuse Act (CFAA) in the United States
- The Computer Misuse Act in the United Kingdom
- Similar computer crime statutes in other jurisdictions
- DMCA anti-circumvention provisions (TPM communications may be technical protection measures)

TPM buses carry sensitive cryptographic material including sealed storage keys, PCR values, endorsement keys, and attestation data. Intercepting this traffic on production systems may expose keys that protect full-disk encryption, secure boot chains, and digital rights management systems.

By using this device, you accept full responsibility for ensuring you have proper authorization. The author (jayis1) disclaims all liability for misuse.

---

## 1. Purpose & Overview

The Trusted Platform Module (TPM) is a secure cryptoprocessor embedded in virtually every modern computer, server, and many embedded systems. It provides hardware-rooted trust for measured boot, sealed storage, platform attestation, and key protection. Two physical interfaces carry TPM 2.0 traffic in production hardware:

1. **LPC (Low Pin Count) bus** — the legacy Intel interface, still common in desktops, servers, and BMC subsystems. TPM communicates over IO cycles at addresses 0x4E/0x4F (legacy) or the TIS locality window at 0xFED40000+.

2. **SPI (Serial Peripheral Interface)** — the modern interface used on virtually all client platforms since ~2015 (Intel/AMD client SoCs, many ARM SoCs). TPM registers are accessed via 24-bit addressed SPI transactions with a simple command/addr/data framing.

The tpm-phantom is a **dual-interface TPM bus analyzer** that can:

- **Passively snoop** both LPC and SPI TPM traffic in real-time
- **Reconstruct** TPM register reads and writes into human-readable transactions
- **Decode** TPM 2.0 TIS register names (ACCESS, STS, DATA_FIFO, DID_VID, etc.)
- **Filter** captured traffic by address or TPM-only access
- **Log** all transactions to a MicroSD card for long-duration captures
- **Stream** transactions wirelessly over BLE to a companion mobile app
- **Inject** crafted IO writes on LPC or spoof read responses on SPI (experimental, for authorized research only)

### Why This Matters for Security Research

TPM is the root of trust for billions of devices, but its bus-level behavior is poorly documented and rarely audited. Security researchers need tools to:

- **Audit measured boot implementations** — verify PCR extend sequences
- **Test locality access control** — verify that locality 0-4 access restrictions are enforced
- **Observe key exchange timing** — analyze TPM command/response latency for side-channel research
- **Verify secure boot attestation** — observe the full attestation quote flow
- **Research bus-level attacks** — test whether TPM traffic can be manipulated or spoofed
- **Reverse-engineer proprietary TPM commands** — observe vendor-specific command sequences
- **Validate TCG spec compliance** — verify that platform firmware follows the TCG PC Client TPM Profile

No existing open-source tool provides this dual-bus TPM analysis capability. The tpm-phantom fills this gap.

---

## 2. Attack Surface & Threat Model

### What the tpm-phantom observes

| Data | LPC | SPI | Security implication |
|---|---|---|---|
| TPM_ACCESS register reads/writes | ✅ | ✅ | Locality request/release sequences |
| TPM_STS status polling | ✅ | ✅ | Command readiness timing |
| TPM_DATA_FIFO reads/writes | ✅ | ✅ | TPM command/response payload bytes |
| TPM_DID_VID reads | ✅ | ✅ | Device/vendor identification |
| TPM_INTERFACE_ID reads | ✅ | ✅ | Interface capability disclosure |
| SERIRQ transitions (LPC) | ✅ | — | TPM interrupt behavior |
| SPI wait states (0x00 padding) | — | ✅ | TPM processing latency |

### Threat model

The tpm-phantom operates as a **passive bus snooper** by default. It connects to the LPC or SPI probe headers (or via clip-on probes to the TPM chip pins) and listens without driving the bus. This is equivalent to an oscilloscope or logic analyzer but with TPM protocol-level decoding.

The **injection** capability (LPC IO write, SPI read response spoofing) is an active mode that requires careful authorization. On LPC, injection drives the LAD[3:0] lines as a bus master — this can cause contention if the host is also driving. The board includes 100Ω series resistors on LAD lines to limit current during contention. On SPI, injection pre-loads the MISO line with crafted data that the host reads instead of real TPM responses.

**What an attacker with a tpm-phantom could learn:**
- TPM command sequences during boot (PCR extends, startup sequences)
- Sealed data unsealing operations (disk encryption key release)
- Attestation quote responses (EK/AIK certificate chain)
- Locality access patterns (who can access TPM at what locality)
- Vendor-specific command extensions

**What the tpm-phantom does NOT do:**
- It does not extract the EK (Endorsement Key) — that's burned into the TPM at manufacturing
- It does not bypass TPM access control — it observes it
- It does not decrypt sealed data — it observes the unseal command flow
- It does not perform MITM on encrypted TPM sessions — the TPM 2.0 protocol uses parameter encryption for sensitive data

---

## 3. Hardware Specifications

### MCU & Processing

| Component | Part | Role |
|---|---|---|
| Main MCU | STM32H743VIT6 | Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM — runs capture engine, protocol stack, USB CDC |
| BLE Co-processor | nRF52840-QFAA | Cortex-M4F @ 64 MHz, handles BLE 5.0, Nordic UART Service transparent bridge |
| Crystal (HSE) | 25 MHz SMD 3225 | STM32H7 system clock source |
| Crystal (BLE) | 32.768 kHz SMD 3215 | nRF52840 RTC clock |
| Crystal (BLE RF) | 32 MHz (internal nRF RC or optional) | BLE radio calibration |

### Bus Interfaces

| Interface | Pins | Speed | Mode |
|---|---|---|---|
| LPC | LCLK, LFRAME#, LAD[3:0], LPCPD#, SERIRQ | 24/33 MHz | GPIO edge-triggered capture |
| SPI TPM | CS#, SCK, MOSI, MISO, IRQ# | Up to 50 MHz | SPI2 slave + DMA double-buffer |
| USB | USB-C 2.0 (DM, DP) | Full-Speed 12 Mbps | USB CDC virtual serial port |
| BLE | nRF52840 BLE 5.0 | 1-2 Mbps | Nordic UART Service (NUS) |
| MicroSD | SDMMC1 4-bit | 40 MHz | Block write capture log |

### Power

| Source | Path |
|---|---|
| USB VBUS (5V) | → AMS1117-3.3 → 3V3 rail |
| Battery (optional) | LiPo 3.7V → MCP1703-3.3 → 3V3 rail |
| Current draw | ~120 mA active capture, ~45 mA idle |

### Form Factor

- **Board dimensions:** 60 × 30 mm (USB stick form factor)
- **Layers:** 4-layer FR4, 1.6 mm thickness
- **Stackup:** F.Cu (signals) / In1.Cu (GND plane) / In2.Cu (3V3 plane) / B.Cu (routing)
- **Probe connectors:** 2×5 2.54mm header (LPC), 2×4 2.54mm header (SPI TPM)
- **LEDs:** 4× SMD 0603 (status, capture, error, LPC mode)
- **Weight:** ~15 g (without battery)

---

## 4. Architecture & Block Diagram

```
                    ┌─────────────────────────────────────────────┐
                    │              tpm-phantom board                │
                    │                                             │
  LPC probe ────────┤ LCLK  LFRAME#  LAD[3:0]  LPCPD#  SERIRQ      │
  (2×5 header)      │   │       │        │        │       │       │
                    │   ▼       ▼        ▼        ▼       ▼       │
                    │ ┌─────────────────────────────────────────┐ │
                    │ │         EXTI + GPIO edge capture          │ │
                    │ │         (LCLK rising, LFRAME edges)       │ │
                    │ │                 │                         │ │
  SPI TPM probe ────┤│  CS# SCK MOSI MISO IRQ#                    │ │
  (2×4 header)      ││   │   │   │   │   │                       │ │
                    ││   ▼   ▼   ▼   ▼   ▼                       │ │
                    ││  SPI2 slave + DMA double-buffer            │ │
                    ││  (CS# edge via EXTI, RX DMA stream)        │ │
                    ││         │           │                      │ │
                    ││         ▼           ▼                      │ │
                    ││ ┌────────────────────────────┐             │ │
                    ││ │   Transaction Reassembler   │             │ │
                    ││ │ (LPC state machine + SPI    │             │ │
                    ││ │  command/addr/data parser)  │             │ │
                    ││ └──────────┬─────────────────┘             │ │
                    ││            │                               │ │
                    ││            ▼                               │ │
                    ││ ┌────────────────────────────┐             │ │
                    ││ │   Wire Protocol Encoder     │             │ │
                    ││ │ (CRC-16, frame packer)      │             │ │
                    ││ └───┬──────────┬──────────┬──┘             │ │
                    ││     │          │          │                 │ │
                    ││     ▼          ▼          ▼                 │ │
                    ││  USB CDC    UART3      SDMMC1               │ │
                    ││  (host)   (→ nRF52840) (MicroSD log)        │ │
                    ││                │                           │ │
  USB-C ────────────┤│                ▼                           │ │
  (host PC)         ││     nRF52840 BLE 5.0                       │ │
                    ││     (Nordic UART Service)                   │ │
                    ││         │                                   │ │
                    └─────────┼───────────────────────────────────┘
                              │
                              ▼ BLE 5.0
                    ┌─────────────────────┐
                    │  Companion App      │
                    │  (React Native)     │
                    │  Dashboard / Capture│
                    │  Analyze / Inject   │
                    └─────────────────────┘
```

### Data flow

1. **LPC capture:** LCLK rising edges trigger EXTI interrupts. The ISR samples LAD[3:0] and feeds a state machine that decodes cycle type, address, and data nibbles. LFRAME# edges frame the start/end of each transaction. Completed transactions are pushed to a ring buffer.

2. **SPI capture:** SPI2 runs in slave mode with DMA double-buffering. CS# edges (EXTI) frame transactions. DMA RX chunks are processed to extract command (0x83=READ, 0x00=WRITE), 24-bit address, and data bytes.

3. **Reassembly:** The main loop drains both transaction ring buffers, applies address filters, and encodes each transaction as a wire-protocol frame.

4. **Distribution:** Frames are sent simultaneously over USB CDC (to host PC), BLE UART bridge (to mobile app), and MicroSD (persistent log).

---

## 5. Firmware Details

### Architecture

The firmware is written in portable C11 targeting the STM32H743 (Cortex-M7). It uses bare-metal register access (no HAL library) for maximum performance and minimum overhead. The codebase is organized as:

```
firmware/
├── main.c              — entry point, clock init, main loop, command dispatch
├── board.h             — pin assignments, clock config, GPIO helpers
├── registers.h         — MMIO register definitions for all peripherals
├── stm32h743_flash.ld  — linker script (Flash, DTCM, SRAM, ITCM regions)
├── Makefile            — arm-none-eabi-gcc build system
└── drivers/
    ├── lpc_driver.c/h     — LPC bus snoop + injection
    ├── spi_tpm_driver.c/h — SPI TPM capture + injection
    ├── usb_cdc.c/h        — USB CDC virtual serial port
    ├── ble_bridge.c/h     — UART bridge to nRF52840
    ├── sd_capture.c/h     — MicroSD block write storage
    └── wire_protocol.c/h  — binary frame encoder/decoder
```

### Key design decisions

**Direct register access over HAL:** The STM32H7 HAL adds significant overhead and is not suited for the tight timing requirements of LPC bus snooping (24 MHz clock = ~41 ns per edge). Direct MMIO register access gives deterministic ISR latency.

**EXTI-based LPC capture:** Rather than using TIM input capture (which is limited to 4 channels), the firmware uses EXTI edge interrupts on LCLK and LFRAME#. This allows capturing all 6 LPC signals with flexible edge selection. The ISR reads the GPIO IDR register in ~3 cycles.

**SPI2 slave + DMA:** SPI TPM traffic is captured by running SPI2 in slave mode with DMA double-buffering. The DMA RX stream uses circular mode with two 64-byte buffers — when one fills, the ISR switches to the other and processes the data. This achieves zero-loss capture at 50 MHz SPI clock.

**CRC-16/CCITT wire protocol:** Both the USB and BLE channels use the same frame format with CRC-16/CCITT (poly 0x1021) error detection. This ensures data integrity over the lossy BLE link.

**Standalone mode:** The user button (PC13) toggles capture without requiring a connected app — useful for field captures where the operator just needs to clip on and start logging to SD.

### Performance

| Metric | Value |
|---|---|
| LPC LCLK max capture rate | 33 MHz (30 ns per edge, ISR latency ~40 ns) |
| SPI max capture rate | 50 MHz (DMA sustained, zero loss) |
| Transaction throughput | ~500K transactions/sec (ring buffer limited) |
| SD write rate | ~5 MB/s (SDMMC1 4-bit @ 40 MHz) |
| BLE latency | ~15 ms end-to-end (NUS transparent) |
| USB CDC latency | ~1 ms (bulk IN) |

---

## 6. Application / Software Interface

### Companion app (React Native)

The companion app runs on Android/iOS and connects to the tpm-phantom over BLE. It provides four screens:

1. **Dashboard** — Device connection status, capture mode selection (LPC/SPI), start/stop controls, live statistics (total/TPM transactions, SPI reads/writes, SD blocks), firmware version.

2. **Capture** — Real-time scrolling transaction log with color-coded bus type (LPC=amber, SPI=cyan), direction (R/W), TPM register name decode, address, data, timestamp. Includes TPM-only filter and address filter.

3. **Analyze** — TPM register access frequency analysis (bar chart of reads vs writes per register), searchable transaction history, detailed transaction decode with TPM_ACCESS and TPM_STS bitfield interpretation.

4. **Inject** — LPC IO write injection (enter address + data byte) and SPI read response injection (enter hex byte sequence to preload on MISO). Includes injection log and safety warnings.

### Wire protocol

Both USB CDC and BLE use the same binary frame format:

```
[SOF0=0xA5][SOF1=0x5A][LEN u16-le][CMD u8][PAYLOAD...][CRC16 u16-le]
```

Commands (app → device): GET_STATUS, START_CAPTURE, STOP_CAPTURE, GET_STATS, SET_FILTER, INJECT_LPC, INJECT_SPI, SET_LED, RESET, SD_STATUS, FLUSH_SD, GET_VERSION.

Responses (device → app): STATUS, TRANSACTION, STATS, SD_INFO, VERSION, ERROR.

---

## 7. Use Cases

### Red Team Operations

- **Pre-engagement reconnaissance:** Clip onto a target system's TPM probe header during physical access to capture the full measured boot sequence. Analyze PCR values to understand the target's boot configuration.

- **Secure boot auditing:** Observe the full TPM startup, PCR extend, and locality request sequence during boot to verify the platform's measured boot implementation matches expected values.

- **Disk encryption key flow analysis:** Observe the BitLocker/Clevis unseal operation — when the OS requests the volume encryption key from the TPM, the tpm-phantom captures the full command/response sequence including locality and access register state.

### Security Research

- **TPM side-channel research:** Measure SPI wait-state counts (0x00 padding bytes returned while the TPM processes) to analyze processing-time variations across different commands. This can reveal timing side-channels in TPM implementations.

- **Locality access control testing:** Systematically test whether each locality (0-4) correctly enforces access restrictions by observing the ACCESS register state during locality transitions.

- **Vendor command reverse engineering:** Capture vendor-specific TPM commands (opcode 0x200000+) to understand proprietary extensions in server TPMs or custom embedded TPMs.

### Penetration Testing

- **Firmware supply chain verification:** Capture the boot-time TPM measurements and compare PCR values against known-good baselines to detect firmware tampering.

- **TPM spoofing (authorized test only):** Use SPI injection to return crafted TPM responses to test whether the host OS properly validates TPM attestation data. Can the OS be tricked into accepting a fake attestation quote?

- **Locality escalation testing:** Inject crafted ACCESS register writes on LPC to test whether the platform firmware properly validates locality requests — can locality 4 (firmware) be accessed from locality 0 (OS)?

---

## 8. Bill of Materials

| Ref | Part | Package | Qty | Est. Cost |
|---|---|---|---|---|
| U1 | STM32H743VIT6 | LQFP-100 | 1 | $18.50 |
| U2 | nRF52840-QFAA | QFN-48 | 1 | $7.20 |
| U3 | AMS1117-3.3 | SOT-223 | 1 | $0.15 |
| U4 | TLV75512PDBV | SOT-23-5 | 1 | $0.80 |
| U5 | USBLC6-2SC6 | SOT-23-6 | 1 | $0.35 |
| Y1 | 25 MHz crystal | SMD 3225 | 1 | $0.30 |
| Y2 | 32.768 kHz crystal | SMD 3215 | 1 | $0.25 |
| J1 | USB-C receptacle | HRO TYPE-C-31-M-12 | 1 | $0.80 |
| J2 | 2×5 pin header | 2.54mm THT | 1 | $0.15 |
| J3 | 2×4 pin header | 2.54mm THT | 1 | $0.12 |
| J4 | MicroSD socket | Hirose DM3D-SF | 1 | $1.20 |
| RN1 | 100Ω resistor network | 0603 x4 | 1 | $0.05 |
| D2-D5 | LED 0603 (G/B/R/G) | 0603 | 4 | $0.20 |
| SW1 | Tactile button | 6×6mm SMD | 1 | $0.10 |
| C1-C10 | 100nF X7R | 0402 | 10 | $0.50 |
| C11-C12 | 10µF X5R | 0805 | 2 | $0.20 |
| R1-R8 | 10kΩ 1% | 0402 | 8 | $0.08 |
| L1 | 3.9nH RF inductor | 0402 | 1 | $0.10 |
| PCB | 4-layer FR4 60×30mm | — | 1 | $3.50 |

**Total estimated BOM:** ~$72.50 at 1K volume.

---

## 9. Building & Flashing

### Firmware

```bash
cd tpm-phantom/firmware
# Install arm-none-eabi-gcc toolchain
make -j$(nproc)
# Flash via ST-Link
make flash
# Or flash the binary manually
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "program build/tpm-phantom.bin 0x08000000 verify reset exit"
```

### Companion App

```bash
cd tpm-phantom/app
npm install
# Android
npx react-native run-android
# iOS
npx react-native run-ios
```

### Operating the device

1. **Power:** Connect USB-C to a host or USB power bank. The green status LED illuminates.
2. **Probe connection:** Connect the LPC or SPI probe header to the target's TPM bus. Use a grabber clip or soldered pigtail.
3. **Standalone capture:** Press the user button (PC13) to toggle capture. The blue LED indicates active capture. Transactions log to MicroSD.
4. **App control:** Open the companion app, scan for BLE, connect, and use the Dashboard to select capture mode and start.
5. **Data retrieval:** Remove MicroSD and read the capture log, or stream live via USB CDC (e.g., `screen /dev/ttyACM0 115200`).

---

## 10. License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0-only |
| React Native companion app | MIT |

All components authored by **jayis1**.

---

## 11. Acknowledgments

- TCG (Trusted Computing Group) for the TPM 2.0 and PC Client Platform TPM Profile specifications
- Intel for the LPC Interface Specification
- ST Microelectronics for the STM32H7 reference manual (RM0433)
- Nordic Semiconductor for the nRF52840 Product Specification

---

*tpm-phantom v1.0 — © jayis1 — Built for authorized security research.*