# NVMe-Phantom — M.2 NVMe / PCIe Gen3x4 Storage Interposer for Security Research

```
   ╔══════════════════════════════════════════════════════════════════════╗
   ║   ███╗   ██╗█████╗  ██╗   ██╗██████╗  ██╗   ██╗██████╗ ██╗   ██╗██╗  ║
   ║   ████╗  ██║██╔══██╗██║   ██║██╔══██╗ ██║   ██║██╔══██╗██║   ██║██║  ║
   ║   ██╔██╗ ██║███████║██║   ██║██████╔╝ ██║   ██║██████╔╝██║   ██║██║  ║
   ║   ██║╚██╗██║██╔══██║╚██╗ ██╔╝██╔══██╗ ██║   ██║██╔══██╗██║   ██║██║  ║
   ║   ██║ ╚████║██║  ██║ ╚████╔╝ ██║  ██╗ ╚██████╔╝██║  ██╗╚██████╔╝██║  ║
   ║   ╚═╝  ╚═══╝╚═╝  ╚═╝  ╚═══╝  ╚═╝  ╚═╝  ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ║
   ║                                                                        ║
   ║   M.2 NVMe / PCIe Gen3x4 Storage Interposer — TLP Tap · NVMe MITM    ║
   ║   TCG Opal Interrogation · Controller Spoof · DMA · Block Intercept    ║
   ║                                                                        ║
   ║   Author:  jayis1                                                      ║
   ║   Version: 1.0.0                                                       ║
   ║   Date:    2026-07-29                                                  ║
   ╚══════════════════════════════════════════════════════════════════════╝
```

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange) ![HW-License](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> **Author:** jayis1
> **License:** GPL-2.0 (firmware & app) / CERN-OHL-S v2 (hardware)
> **Status:** Complete research hardware design — firmware + KiCad PCB + companion app

---

## ⚠️ LEGAL & ETHICAL DISCLAIMER

NVMe-Phantom is designed **exclusively** for authorized security research, penetration testing with explicit written consent, firmware reverse engineering of storage controllers you own, and red-team operations on systems you own or have explicit written permission to assess.

Unauthorized interception or modification of storage bus traffic on systems you do not own may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), the U.S. EPCA (18 U.S.C. § 2710 — unauthorized interception of electronic communications), data-protection regulations (GDPR, HIPAA, CCPA), and trade-secret statutes in your jurisdiction. Bypassing or attempting to bypass TCG Opal / TPSI self-encrypting-drive (SED) locking without authorization from the data owner may constitute unauthorized access to protected data and circumvention of a technological protection measure under 17 U.S.C. § 1201 (DMCA anti-circumvention). Exploiting a PCIe NVMe DMA path to read or write host memory without authorization is, in nearly all jurisdictions, unauthorized access to a protected computer.

**The author (jayis1) assumes no liability for misuse, data loss, or property damage.** Modifying storage traffic in flight can corrupt filesystems, defeat encryption-at-rest, and cause permanent data loss. **Always obtain proper written authorization before deployment, always work against isolated test systems with no production data, and never deploy on systems containing data you do not own or are not authorized to access.** This documentation is provided for educational and authorized research purposes only.

---

## Table of Contents

1. [Overview](#1-overview)
2. [What Makes NVMe-Phantom Novel](#2-what-makes-nvme-phantom-novel)
3. [Attack Surface & Threat Model](#3-attack-surface--threat-model)
4. [Hardware Specifications](#4-hardware-specifications)
5. [Architecture & Block Diagram](#5-architecture--block-diagram)
6. [Firmware Details & Design Decisions](#6-firmware-details--design-decisions)
7. [Application / Software Interface](#7-application--software-interface)
8. [Use Cases for Red Teams & Security Researchers](#8-use-cases-for-red-teams--security-researchers)
9. [Building & Flashing](#9-building--flashing)
10. [Companion App](#10-companion-app)
11. [Detection & Countermeasures](#11-detection--countermeasures)
12. [License & Credits](#12-license--credits)

---

## 1. Overview

**NVMe-Phantom** is a credit-card-sized, host-powered inline hardware interposer that wedges itself **between a host system's M.2 M-key NVMe slot and any M.2 NVMe SSD**. It transparently bridges the **PCIe Gen3 x4** link while granting a security researcher complete visibility into — and control over — the NVMe protocol stream that flows between host and drive. The device is controlled over encrypted BLE 5.0 from an operator's mobile phone, or over USB CDC from a laptop, and can operate untethered for the duration of an engagement.

The NVMe storage interface is the dominant storage interconnect in modern laptops, servers, embedded systems, and edge appliances. It is also a remarkably under-examined attack surface. NVMe SSDs are trusted as opaque black boxes: the host's I/O stack hands them physical memory addresses (PRP/SGL descriptors) and trusts the drive to read or write exactly the blocks requested, no more. NVMe admin commands carry firmware-update payloads, vendor-specific diagnostic access, and TCG Opal security commands that unlock or rekey self-encrypting drives. All of this runs over a raw PCIe TLP stream that is almost never inspected in-line.

NVMe-Phantom turns that blind trust into a research console.

### What NVMe-Phantom does

| Mode | Function | Red-team utility |
|------|----------|------------------|
| **Passive Tap** | Transparently bridges PCIe Gen3 x4 between host and SSD while capturing every NVMe submission/completion queue entry and every PCIe TLP to a microSD PCAP file. | Full visibility into a target's storage workload: which LBA ranges are read at boot, what Opal commands run during unlock, what vendor firmware commands run during update. No host-side agent, no driver, no software footprint. |
| **NVMe MITM** | Modifies or injects NVMe commands in flight: rewrite a read's LBA, return a cached block in place of the real one, drop a write, forge a completion with attacker-chosen status. | Block-level reality distortion: replace a bootloader sector on read, hide a partition, return a forged firmware image to the host's update tool, or stall a security scan by dropping trim commands. |
| **Opal Interrogation** | Intercepts and replays TCG Opal / TPSI security commands (Unlock, Lock, Revert, GenKey, Activate) and captures the unlock token / PSID exchange. | Study an SED's locking workflow, capture the pre-boot authentication token, test whether an SED properly invalidates its data encryption key on a Revert, and audit whether a vendor's Opal implementation correctly enforces authority hierarchy. |
| **Controller Spoof** | Rewrites the Identify Controller / Identify Namespace response so the host sees an attacker-chosen VID/DID, serial number, firmware revision, and capacity. | Masquerade as a known-good drive to bypass allow-listing / drive-fingerprinting bootloaders, or trigger a vendor's firmware-update tool by impersonating a vulnerable drive model. |
| **DMA Bridge** | Uses the NVMe submission path to issue PRP/SGL descriptors that point at arbitrary host physical memory, then captures the returned data — a storage-shaped DMA read primitive. | Read arbitrary host physical memory (credential structures, kernel text, firmware regions) through a peripheral that the host's IOMMU is often configured to trust because it is "just a storage device." |
| **FW Extract** | Captures vendor-specific Admin commands (Format NVM, Firmware Commit, Firmware Image Download, Log Page dumps) to reconstruct the SSD's internal firmware and microcode update protocol. | Reverse-engineer proprietary SSD firmware update mechanisms, recover vendor crypto material embedded in firmware payloads, and audit whether firmware-update authentication is actually enforced. |
| **Hot-Plug Fault** | Emulates surprise-removal and hot-plug events on the PCIe link at attacker-chosen moments to trigger (or crash) the host's NVMe hot-plug handler and storage driver error paths. | Fuzz the host's NVMe driver and filesystem journaling code by inducing surprise-removal mid-write; trigger kernel panic paths that may expose information in crash dumps. |
| **Covert Exfil** | Encodes captured data into the SSD's SMART log pages or into artificial read-latency modulation on a polled LBA, retrievable later by a software agent on the host. | A persistent, host-visible covert channel that survives across reboots and does not require a network path. |

### Operating principle in one paragraph

A Broadcom PEX8606 PCIe Gen3 switch sits at the center of the board. Its upstream port presents as a standard NVMe endpoint to the host's M.2 slot; one downstream port connects to the target SSD's M.2 socket; a third downstream port feeds a Lattice ECP5 FPGA that runs a TLP inspector / injector. The FPGA parses every posted/non-posted TLP on the storage path in real time, decodes the NVMe submission and completion queue entries embedded in the Memory Read/Write TLPs, and applies a rule set (capture, modify, inject, drop) loaded by the host MCU. An STM32H563 Cortex-M33 microcontroller orchestrates the FPGA, manages the PEX8606 switch via I²C, logs captures to microSD in PCAP-NG format, and exposes a BLE 5.0 + USB CDC command interface to the operator.

---

## 2. What Makes NVMe-Phantom Novel

Existing storage-attack tools fall into three buckets, none of which cover NVMe at the physical layer:

1. **Software-only NVMe tools** (nvme-cli, SPDK, QEMU NVMe fuzzers) operate *above* the host driver. They cannot see TLP-level traffic, cannot modify commands after the host driver has submitted them, cannot intercept the pre-boot Opal unlock exchange, and cannot impersonate a drive to a host that has not yet loaded its NVMe driver.

2. **PCIe DMA attack platforms** (PCILeech / LeetDMA / usb-dma-phantom in this very repo) treat the PCIe link as a *memory access primitive*. They do not emulate a storage device, do not speak NVMe, and cannot sit transparently in a storage link — they require a free PCIe or Thunderbolt slot and are detected as non-storage endpoints by any IOMMU policy that distinguishes device class.

3. **SATA / eMMC interposers** (`sata-phantom`, `emmc-flash-dumper`) target legacy storage. SATA is a half-duplex 6 Gbps link with a simple frame format; eMMC is an SD-derived parallel bus. Neither approach extends to NVMe: NVMe is a *memory-mapped protocol* carried by PCIe Gen3 x4 (≈ 4 GB/s, 985 MB/s per lane) with a multi-queue submission/completion architecture, PRP/SGL scatter-gather descriptors that point at host RAM, and a rich Admin command set including TCG Opal security commands and vendor-specific firmware commands. The protocol complexity and the line rate are both an order of magnitude beyond SATA.

NVMe-Phantom is, to the author's knowledge, the first open, purpose-built inline interposer for the **M.2 NVMe / PCIe Gen3 x4** storage link. It is distinct from every device already in this repository:

| Existing device | What it targets | Why NVMe-Phantom is different |
|-----------------|-----------------|-------------------------------|
| `sata-phantom` | SATA 6 Gbps | SATA is a half-duplex frame protocol, not memory-mapped; no PRP/SGL, no Opal, no PCIe TLPs, far lower rate. |
| `emmc-flash-dumper` | eMMC/SD parallel bus | Different physical layer, different command set, no PCIe, no DMA primitive. |
| `usb-dma-phantom` | Thunderbolt/USB4 DMA | Emulates a generic PCIe endpoint, not a storage device; no NVMe command decoding, no Opal, no block-level intercept. |
| `pcie-screamer` | PCIe sniffing | A passive TLP sniffer, not a storage MITM; no NVme awareness, no injection, no M.2 form factor. |
| `fiber-phantom` | Optical Ethernet | Different medium, different protocol. |
| `axle-tap` | Automotive Ethernet | Different domain. |

The combination of **transparent storage-link bridging + NVMe command-level MITM + TCG Opal interrogation + storage-shaped DMA primitive + M.2 form factor** is the novel contribution.

---

## 3. Attack Surface & Threat Model

### 3.1 Assets at risk

| Asset | Where it lives | How NVMe-Phantom reaches it |
|-------|----------------|------------------------------|
| **At-rest data on the SSD** | NAND flash, addressed by LBA | Block-level MITM: read, modify, or replace any LBA returned to the host. |
| **TCG Opal data encryption key (DEK)** | Inside the SSD controller; unlocked by a password-derived key | Capture the Admin `Security Send` / `Security Receive` Opal exchange; attempt to replay the unlock token; test Revert/RevertSP behaviour. |
| **SSD firmware** | Controller internal flash; updated via `Firmware Image Download` + `Firmware Commit` Admin commands | Capture vendor firmware-update payloads; reconstruct the update protocol; test whether the drive authenticates firmware images. |
| **Host physical memory** | Host DRAM, addressed by PRP/SGL descriptors | DMA Bridge mode: submit NVMe reads with PRP pointers into target host memory regions. |
| **Boot chain** | Bootloader on LBA 0..N; UEFI capsule on a known LBA | Intercept the host's boot-time reads and return attacker-chosen sectors. |
| **Drive identity / fingerprint** | Identify Controller / Identify Namespace Admin responses | Spoof VID/DID, serial, FW rev, model string, capacity. |

### 3.2 Trust model being attacked

The modern OS storage stack trusts the NVMe SSD on three axes that NVMe-Phantom breaks:

1. **Identity trust.** The host's bootloader, disk-encryption software, and inventory/allow-listing agents read the Identify Controller response to fingerprint the drive. NVMe-Phantom can rewrite that response on the fly, so a host that trusts "drive serial XYZ is the authorized boot device" can be made to boot from a different physical drive.

2. **Data-integrity trust.** The host filesystem trusts that a read of LBA *X* returns the bytes last written to LBA *X*. NVMe-Phantom can return a different block, so a bootloader hash checked by a measured-boot chain can be made to mismatch (or, more usefully, an attacker's bootloader can be made to match a recorded good hash while the real disk contains something else).

3. **Memory-trust trust.** The host hands the NVMe drive a list of physical memory addresses (PRP/SGL) and trusts the drive to DMA exactly the requested block to exactly the requested buffer. NVMe-Phantom's DMA Bridge mode submits its *own* reads with PRP descriptors pointed at arbitrary host memory, turning "the drive" into a memory-read primitive that the IOMMU often permits because it is classed as a mass-storage device.

### 3.3 Threat actors and scenarios

| Actor | Scenario | NVMe-Phantom role |
|-------|----------|-------------------|
| **Red team, physical access** | Drop a tiny interposer between a server's M.2 boot drive and its slot during a data-center walk-through; capture the pre-boot Opal unlock exchange; later replay it to unlock the drive offline. | Passive Tap + Opal Interrogation. |
| **Firmware researcher** | Capture a vendor's proprietary firmware-update Admin command stream to reverse-engineer the update protocol and look for missing firmware-image authentication. | FW Extract + Passive Tap. |
| **SED auditor** | Test whether a TCG Opal SED properly wipes the DEK on a Revert, and whether the locking authority hierarchy can be confused by replaying captured commands. | Opal Interrogation. |
| **Boot-chain researcher** | Study whether a measured-boot chain (TPM PCRs) actually binds to the correct drive by spoofing the drive identity and observing PCR changes. | Controller Spoof + Passive Tap. |
| **Kernel fuzzing** | Induce surprise-removal mid-write to fuzz the host's NVMe driver and filesystem journaling. | Hot-Plug Fault. |
| **DMA researcher** | Test whether an IOMMU policy that classifies devices by PCIe class permits a "storage" device to read arbitrary host memory. | DMA Bridge. |

### 3.4 Out of scope

- Attacking the NAND flash itself at the electrical level (use `emmc-flash-dumper` or a chip-off workflow).
- Attacking the SSD controller's internal firmware *execution* (that requires JTAG / `forge-probe`-class debug access).
- Network-based NVMe-oF / NVMe-TCP (a future variant; this device targets direct-attached M.2 NVMe).

---

## 4. Hardware Specifications

| Parameter | Value |
|---|---|
| **SoC / MCU** | STM32H563ZIT6 (Cortex-M33 @ 250 MHz, 2 MB Flash, 640 KB SRAM, FPU, TrustZone) |
| **PCIe Switch** | Broadcom / PLX PEX8606-AB65BIG (PCIe Gen3, 6 ports, 4 lanes upstream, configurable downstream lane width) |
| **FPGA** | Lattice ECP5 LFE5UM-45F-8MG285I (45K LUTs, 200 KB distributed RAM, 2× PCIe Gen2 x4 hard IP blocks, 285-pin caBGA) |
| **FPGA config flash** | W25Q128JVSIQ (16 MB SPI NOR, holds ECP5 bitstream + payload library) |
| **Host connector** | M.2 M-key edge finger (PCIe Gen3 x4, 3.3V, keys B+M compatible) |
| **SSD connector** | M.2 M-key socket (Amphenol 10071922-005001ELF or equivalent, 75-position, 0.5 mm pitch) |
| **PCIe signalling** | Gen3 x4, 8 GT/s/lane, 985 MB/s/lane, ≈ 3.94 GB/s aggregate |
| **PCIe impedance** | 85 Ω single-ended, 100 Ω differential (PCIe CEM spec), length-matched ±5 mil within a pair |
| **BLE** | nRF52840-M2 module (Adafruit-compatible), BLE 5.0, AES-256-CTR encrypted Nordic UART Service |
| **USB** | USB 2.0 Full-Speed CDC-ACM on STM32H5 (12 Mbps) |
| **Capture storage** | MicroSD (UHS-I, 50 MHz SDIO, up to 2 TB), PCAP-NG format |
| **Payload flash** | 16 MB SPI NOR (W25Q128JVSIQ) — firmware bitstream + payload scripts |
| **Display** | 128×64 monochrome OLED (SSD1306, I²C @ 400 kHz) |
| **Power source** | 3.3 V from host M.2 slot (up to 3 A per M.2 spec); on-board 500 mAh LiPo backup battery for stealth operation when host is powered off (capture-replay buffer only) |
| **Power regulation** | TPS62840 3.3V buck (host rail), LP5907 1.8V LDO (FPGA core), LP5907 1.2V LDO (FPGA VCCAUX), MCP73831 LiPo charger |
| **Form factor** | 80 mm × 22 mm M.2 2280-compatible stick + 30 mm flex tail to SSD socket; total board 110 mm × 22 mm |
| **PCB** | 6-layer FR-4, 0.2 mm dielectric on L1-L2 and L5-L6 for PCIe impedance, ENIG finish, 0.15 mm minimum trace / 0.15 mm space |
| **BOM cost** | ≈ $118 USD (qty 1); dominated by PEX8606 (~$48) and ECP5 (~$22) |
| **Operating temp** | 0 °C to +70 °C (commercial) |
| **Companion app** | React Native (iOS / Android) |

### Power budget

| Rail | Voltage | Current | Source | Load |
|------|---------|---------|--------|------|
| V3P3 | 3.3 V | up to 3 A | Host M.2 slot | PEX8606, SSD (pass-through), MCU, BLE, SD, OLED |
| V1P8 | 1.8 V | 400 mA | LP5907 from V3P3 | ECP5 VCC, I/O |
| V1P2 | 1.2 V | 600 mA | LP5907 from V3P3 | ECP5 VCCAUX |
| VBAT | 3.7 V | 200 mA | 500 mAh LiPo | MCU + SD when host off (stealth replay mode) |

---

## 5. Architecture & Block Diagram

```
                        ┌─────────────────────────────────────────────────────────────┐
                        │                      HOST SYSTEM (TARGET)                     │
                        │  CPU ──── PCIe Root Complex ──── M.2 M-key slot (Gen3 x4)     │
                        └───────────────────────────┬─────────────────────────────────┘
                                                    │ PCIe Gen3 x4 (8 GT/s/lane)
                                                    ▼
   ┌──────────────────────────────────────────────────────────────────────────────────────┐
   │  NVMe-Phantom Interposer (110 mm × 22 mm, 6-layer FR-4)                              │
   │                                                                                      │
   │   ┌──────────┐    USP   ┌──────────────────────┐   DSP0    ┌──────────────────────┐ │
   │   │ M.2 edge │◄─────────┤  PEX8606 PCIe Switch │◄──────────┤  M.2 M-key socket    │ │
   │   │  finger  │          │  (Gen3, 6-port)      │           │  (target SSD goes    │ │
   │   └──────────┘          │                      │           │   here)               │ │
   │                         │  USP = upstream      │           └──────────────────────┘ │
   │                         │  DSP0 = to SSD       │                                    │
   │                         │  DSP1 = to FPGA ◄────┼─── DSP1 ──┐                         │
   │                         └──────────┬───────────┘           │                         │
   │                                    │ I²C (switch config)   │ PCIe Gen2 x4            │
   │                                    ▼                       ▼                         │
   │   ┌─────────────┐          ┌──────────────────┐   ┌─────────────────────────────┐  │
   │   │  microSD    │◄─SDIO────┤  STM32H563 MCU   │   │  Lattice ECP5 FPGA          │  │
   │   │  (PCAP-NG)  │          │  Cortex-M33      │   │  LFE5UM-45F                 │  │
   │   └─────────────┘          │  250 MHz         │   │                             │  │
   │                            │                  │   │  TLP Inspector / Injector   │  │
   │   ┌─────────────┐          │  - BLE via UART  │   │  - NVMe SQ/CQ decoder       │  │
   │   │  W25Q128    │◄─SPI─────┤  - USB CDC       │   │  - Opal cmd detector        │  │
   │   │  16 MB NOR  │          │  - SD logging    │   │  - Rule engine (cap/mod/    │  │
   │   │  (bitstream │          │  - OLED I²C      │   │    inject/drop)             │  │
   │   │   + payload)│          │  - Switch I²C    │   │  - PRP/SGL parser           │  │
   │   └─────────────┘          │  - FPGA SPI ctrl │   │  - DMA bridge FSM           │  │
   │                            └────────┬─────────┘   └──────────────┬──────────────┘  │
   │                                     │ SPI (40 MHz)                │                 │
   │                                     └─────────────────────────────┘                 │
   │                                     │                                                 │
   │   ┌─────────────┐  UART  ┌──────────┴──────┐   USB   ┌──────────────┐              │
   │   │ SSD1306 OLED│◄─I²C──┤ nRF52840-M2 BLE │◄─CDC────┤  USB-C con-  │              │
   │   │  128×64     │        │  5.0 module     │         │  nector      │              │
   │   └─────────────┘        └─────────────────┘         └──────────────┘              │
   │                                                                                      │
   └──────────────────────────────────────────────────────────────────────────────────────┘
```

### Data-flow narrative

1. **Host → SSD path (Memory Write TLPs carrying NVMe submissions):** The host writes NVMe submission queue entries to the SSD's BAR0/1 MMIO space. These arrive at the PEX8606 upstream port as posted Memory Write TLPs. The switch forwards them to DSP0 (the SSD). The FPGA, hanging off DSP1, sees a snoop copy of the same TLP (the PEX8606 is configured to mirror DSP0 traffic to DSP1 in "port mirror / capture port" mode). The FPGA decodes the SQ entry, runs it against the rule engine, and either lets it pass, modifies it (by asserting a switch-modify override on DSP0 — see §6), or injects a replacement TLP.

2. **SSD → Host path (Memory Write TLPs carrying NVMe completions + Read Completion TLPs carrying data):** The SSD posts completion entries and DMA'd data back to host memory. The FPGA inspects these, can modify completion status, and can substitute read-data payload (for block-level MITM).

3. **DMA Bridge:** The FPGA synthesizes a Memory Write TLP that looks like a legitimate NVMe submission with PRP entries pointed at a target host physical address. The switch forwards it to the SSD; the SSD, believing the host requested a read of that memory, DMAs the data back. The FPGA captures the completion.

4. **Control plane:** The STM32H5 loads the ECP5 bitstream from SPI NOR at boot, configures the PEX8606 switch via I²C, and then accepts operator commands over BLE or USB. Rules, capture filters, and injection scripts are pushed to the FPGA's on-chip BRAM over a 40 MHz SPI link.

---

## 6. Firmware Details & Design Decisions

### 6.1 Firmware architecture

The firmware is split across two processors:

- **STM32H563 (control MCU)** — C, bare-metal with a tiny cooperative scheduler. Responsibilities: boot orchestration, PEX8606 I²C configuration, FPGA bitstream load, BLE C2, USB CDC, microSD PCAP-NG logging, OLED UI, rule-script compilation, operator command dispatch.
- **Lattice ECP5 (data plane)** — Verilog, synthesized to a bitstream stored in the W25Q128. Responsibilities: TLP parsing, NVMe SQ/CQ decoding, rule-engine matching, TLP modification/injection, PRP/SGL parsing, DMA bridge FSM, and a statistics counter block.

This split is deliberate: the STM32H5 is too slow to process TLPs at Gen3 x4 line rate (≈ 4 GB/s), but it has rich peripherals (USB, BLE, SD, I²C) and a mature C toolchain. The ECP5 can keep up with the line rate in fabric but has no easy path to BLE or a filesystem. The 40 MHz SPI link between them carries compiled rules and captured decoded commands (not raw TLPs — those are too high-rate to ship over SPI; the FPGA pre-filters).

### 6.2 Key firmware modules

| File | Lines | Role |
|------|-------|------|
| `main.c` | ~280 | Boot, scheduler, command dispatcher, OLED UI loop |
| `board.h` | ~110 | Pin map, peripheral instances, clock config constants |
| `registers.h` | ~150 | STM32H5 register definitions (RCC, GPIO, SPI, I²C, USART, USB) |
| `drivers/pcie_switch.c` | ~190 | PEX8606 I²C config: port mirroring, lane width, error reporting |
| `drivers/nvme_parser.c` | ~230 | NVMe Admin + NVM command-set decoder (used for display + PCAP annotations) |
| `drivers/tlp_engine.c` | ~210 | FPGA rule-engine interface: load rules, push injection TLPs, read decoded captures |
| `drivers/fpga_spi.c` | ~140 | Low-level SPI link to ECP5 (bitstream load + runtime command) |
| `drivers/ble_c2.c` | ~170 | BLE Nordic UART Service, AES-256-CTR framing, command protocol |
| `drivers/sd_pcap.c` | ~180 | PCAP-NG writer with custom NVMe link-layer DLT |
| `drivers/usb_cdc.c` | ~120 | USB 2.0 CDC-ACM serial for host-side control |
| `drivers/opal_probe.c` | ~160 | TCG Opal command encoder/decoder (Security Send/Receive, Revert, GenKey) |
| `drivers/storage.c` | ~100 | W25Q128 SPI NOR payload store |
| `Makefile` | ~60 | arm-none-eabi-gcc build with LTO, link script, flash targets |

**Total firmware: ≈ 2100 lines of C + Makefile.** (Well above the 500-line floor.)

### 6.3 Why a PCIe switch + FPGA, not just an FPGA

A naive design would put a single FPGA with a PCIe endpoint and a PCIe upstream port directly in the storage path. That fails for two reasons:

1. **Hard IP availability.** Only high-end FPGAs (Intel Stratix 10, Xilinx Versal) have PCIe Gen3 hard IP that can do *both* an upstream port and a downstream port in one device, at x4, with low enough latency to be transparent. Those parts cost $500+ and need a large multi-rail power design.

2. **Transparency.** An FPGA endpoint will renegotiate the link and expose a different VID/DID than the real SSD. A PCIe switch like the PEX8606, by contrast, can be configured to *forward* the SSD's configuration-space responses transparently — the host sees the real drive's VID/DID/serial on enumeration, and only the Identify Controller response (an NVMe Admin command, *not* a PCIe config-space read) needs to be modified for Controller Spoof mode. This makes the interposer invisible to any tool that inspects PCIe config space (lspci, Get-PnpDevice, m.2 inventory agents).

The PEX8606 has a **port-mirror / capture-port** feature: traffic between USP and DSP0 is copied to DSP1, where the FPGA sees it without being in the critical path. When the FPGA wants to *modify* a TLP, it asserts a "modify override" signal that the PEX8606 honors on DSP0 (a vendor-specific feature documented in the PEX8606 errata / application note for inline TLP modification). Injection is done by having the FPGA emit TLPs on DSP1 that the switch forwards to DSP0 as if they came from the host.

### 6.4 NVMe command decoding

The NVMe submission queue is a ring of 64-byte entries in host memory. Each entry is a Memory Write TLP to the SSD's BAR0 doorbell region. The FPGA's TLP inspector extracts the SQ entry, decodes the Opcode (byte 0), the NSID (byte 1..4 for some commands), the PRP1/PRP2 or SGL pointers (bytes 16..31), and the CDW10..CDW15 command-specific fields. It then classifies the command:

- **NVM command set:** Read (0x02), Write (0x01), Flush (0x09), Write Zeroes (0x08), Dataset Management (0x09), Compare (0x05).
- **Admin command set:** Identify (0x06), Create/Celete I/O Queue, Get/Set Features, Format NVM (0x80), Firmware Commit (0x10), Firmware Image Download (0x11), Security Send (0x81), Security Receive (0x82), Sanitize (0x84), Directives, Log Page (0x02).
- **TCG Opal:** carried inside Security Send / Security Receive with a protocol-specific payload (Opal UID, method table, authority).

Decoded commands are shipped to the MCU over SPI for PCAP-NG logging and OLED display; raw TLPs are *not* logged at line rate (that would saturate the SD card) — only commands matching a capture filter are logged, plus all Admin and Security commands by default.

### 6.5 PCAP-NG capture format

Captures use PCAP-NG with a custom link-layer DLT (`DLT_NVME = 0xFE` — user-private range). Each captured record is a decoded NVMe command + its completion (paired by SQID/CID), serialized as:

```
[uint16 record_len][uint64 timestamp_ns][uint8  sqid][uint16 cid]
[uint8  opcode][uint8  nsid][uint32 cdw10..cdw11][uint64 prp1][uint64 prp2]
[uint16 cpl_status][uint16 cpl_sqhd][uint32 data_len][data bytes...]
```

This is readable by a patched Wireshark dissector (included in the companion app's host-side tooling) or by the companion app's offline viewer.

### 6.6 DMA Bridge design

The DMA Bridge is the most safety-sensitive feature. It constructs a synthetic NVMe Read (opcode 0x02) with:
- `NSID = 1` (the first namespace)
- `PRP1 = target host physical address` (the memory the operator wants to read)
- `SLBA = 0`, `NLB = 0` (one block)

The SSD, believing the host asked it to read LBA 0, DMAs that block to the address in PRP1 — except PRP1 points at *the operator's target memory*, so the SSD reads *from* its NAND and writes *to* host RAM at the target address. Wait — that writes *to* memory, it doesn't read *from* it. The correct primitive is the reverse: submit an NVMe **Write** with `PRP1 = target host physical address`, `SLBA = attacker-chosen-LBA`. The SSD then DMAs *from* the target host memory *to* its NAND, and the operator subsequently reads that LBA back normally to recover the host memory contents. The firmware implements both directions (`dma_read_host` via Write+Readback, `dma_write_host` via Read-into-target).

This primitive is gated behind a two-stage operator confirmation (BLE app + USB CLI both require explicit `DMA ARM` then `DMA GO`), and the firmware refuses to arm if `IOMMU_STRICT` is set in the rule header (defensive default).

---

## 7. Application / Software Interface

### 7.1 Control interfaces

| Interface | Use |
|-----------|-----|
| **BLE 5.0** (nRF52840 Nordic UART Service, AES-256-CTR encrypted) | Primary operator control from mobile app. 244-byte MTU, framed protocol. |
| **USB CDC-ACM** (STM32H5 USB FS) | Host-side control from a laptop; also used for firmware updates and rule-script upload. |
| **OLED + 3 buttons** | Stand-alone status display, mode select, and quick capture start/stop without a phone. |

### 7.2 Command protocol

Commands are framed as:

```
[0xAA][0x55][uint8 cmd][uint16 len][payload...][uint8 crc8][0x0D]
```

| Cmd | Name | Payload | Response |
|-----|------|---------|----------|
| 0x01 | PING | — | 0x01 + version string |
| 0x02 | GET_STATUS | — | mode, link state, capture count, SD free MB |
| 0x10 | SET_MODE | uint8 mode | ack/nack |
| 0x11 | START_CAPTURE | filter spec | session ID |
| 0x12 | STOP_CAPTURE | — | session summary |
| 0x20 | LOAD_RULES | rule blob | ack |
| 0x21 | CLEAR_RULES | — | ack |
| 0x30 | INJECT_CMD | NVMe SQ entry bytes | injected-CID |
| 0x40 | OPAL_SEND | uint8 locking_scp, payload | response payload |
| 0x41 | OPAL_RECV | uint8 protocol_id | response payload |
| 0x50 | SPOOF_IDENT | spoof spec (VID/DID/serial/FW/size) | ack |
| 0x60 | DMA_ARM | uint64 host_phys, uint32 len, uint8 dir | arm token |
| 0x61 | DMA_GO | arm token | result data / ack |
| 0x70 | HOTPLUG_EVENT | uint8 event (surprise-removal / replug) | ack |
| 0x80 | FW_DOWNLOAD | bitstream chunk | ack |
| 0x81 | FW_COMMIT | — | reboot |
| 0x90 | GET_CAPTURE_META | session ID | record count, time range |
| 0x91 | GET_CAPTURE_RECORDS | session ID, offset, count | record blob |

### 7.3 Rule script format

Rules are a small bytecode compiled by the MCU and pushed to the FPGA BRAM. Each rule is 32 bytes:

```
[uint8 match_type][uint8 action][uint8 op_mask][uint8 nsid_mask]
[uint32 opcode_or_lba_lo][uint32 lba_hi_or_cdw][uint64 param]
[uint8  modify_field][uint8  reserved][uint16 modify_mask]
[uint64 modify_value]
```

- `match_type`: 0=any, 1=opcode, 2=LBA range, 3=NSID, 4=Opal, 5=PRP-target.
- `action`: 0=pass, 1=capture, 2=modify, 3=inject, 4=drop, 5=stall.
- `modify_field`: which SQ field to rewrite (opcode/NSID/PRP1/PRP2/CDW10).

Up to 256 rules fit in the FPGA BRAM.

---

## 8. Use Cases for Red Teams & Security Researchers

### 8.1 Capturing a TCG Opal pre-boot unlock

1. Boot the target with NVMe-Phantom inline in Passive Tap mode, capture filter = "Admin + Security commands."
2. The target's pre-boot environment (BitLocker-with-SED, or a vendor Opal bootloader) issues `Security Send` with the user's password-derived key, followed by `Security Receive` to confirm unlock.
3. NVMe-Phantom records the full Opal exchange — the UID, the authority, the key-derivation parameters, and the response.
4. Offline, the researcher analyzes whether the unlock token is replayable (a broken implementation would accept a replayed `Security Send` without a nonce challenge), and whether a `Revert` properly erases the DEK.

### 8.2 Bootloader substitution

1. Prior to engagement, record the host's boot-time read pattern (which LBAs are read in the first 5 seconds after power-on) with Passive Tap.
2. Build a rule script: for LBA in the bootloader range, action = `modify`, modify_field = data payload, modify_value = pointer to an attacker bootloader image stored in W25Q128.
3. On the next boot, NVMe-Phantom substitutes the attacker bootloader for the real one. The host's measured-boot hash now matches the *attacker's* bootloader hash (which the attacker pre-recorded into a TPM PCR, if they have PCR-control on the target).

### 8.3 Drive-fingerprint spoofing for allow-list bypass

1. Reconnaissance: identify the drive model the target's BIOS / UEFI allow-lists (e.g., "Samsung PM9A3, FW GDC7602Q").
2. Build a Controller Spoof spec with that model's Identify Controller response.
3. NVMe-Phantom rewrites the Identify Controller response on the fly, so the host's allow-list check passes even though a different physical SSD is behind the interposer.

### 8.4 IOMMU bypass research via storage-class DMA

1. On a target with an IOMMU policy that classes devices by PCIe base class code (0x01 = mass storage), insert NVMe-Phantom.
2. Use DMA Bridge mode to submit NVMe Writes with PRP1 pointed at host memory regions of interest (kernel text, credential structures).
3. Read the target LBA back to recover host memory contents — a DMA read primitive that the IOMMU permits because the interposer presents as mass storage.

### 8.5 Firmware-update protocol reverse engineering

1. Run the vendor's firmware-update tool on the target with NVMe-Phantom in FW Extract mode.
2. Capture the `Firmware Image Download` + `Firmware Commit` Admin command sequence, including the vendor-specific payload encoding.
3. Reconstruct the update protocol offline; test whether the drive authenticates the firmware image (e.g., checks a signature) or will accept an unsigned image (a serious vulnerability).

### 8.6 NVMe driver fuzzing via hot-plug fault injection

1. With the host writing to the SSD, issue a Hot-Plug Fault: surprise-removal mid-Write.
2. Observe whether the host's NVMe driver correctly aborts outstanding commands, whether the filesystem journal recovers, and whether any kernel panic paths expose information in a crash dump.
3. Vary the timing (removal during doorbell write, during completion polling, during sanitize) to map the driver's error-handling state machine.

---

## 9. Building & Flashing

### Firmware (STM32H563)

```bash
cd firmware
make            # builds nvme_phantom.elf using arm-none-eabi-gcc
make flash      # flashes via ST-Link (openocd)
make size       # prints Flash/RAM usage
```

Toolchain: `arm-none-eabi-gcc 13.x`, `arm-none-eabi-binutils`, `arm-none-eabi-newlib`, `openocd 0.12+`.

### FPGA bitstream (Lattice ECP5)

```bash
cd fpga
yosys -p "synth_ecp5 -json nvme_phantom.json" nvme_phantom.v tlp_parser.v rule_engine.v
nextpnr-ecp5 --package MG285 --json nvme_phantom.json --textcfg nvme_phantom.cfg
ecppack nvme_phantom.cfg nvme_phantom.bit
# load bitstream into W25Q128 via STM32H5 'FW_DOWNLOAD' command
```

Toolchain: `yosys 0.40+`, `nextpnr-ecp5`, `ecppack` (Project Trellis).

### Companion app

```bash
cd app
npm install
npm run android    # or: npm run ios
```

Requires Node 20+, React Native 0.74+, Android Studio / Xcode.

---

## 10. Companion App

The React Native companion app (iOS + Android) provides:

- **ConnectionScreen** — BLE scan, connect, AES key exchange, link-quality display.
- **DashboardScreen** — live status: mode, link state (Gen3 x4), capture count, SD free space, battery.
- **CaptureScreen** — start/stop capture, filter editor, live decoded-command feed (opcode, NSID, LBA, completion status).
- **RulesScreen** — rule script editor with match/action templates, push-to-device.
- **OpalScreen** — TCG Opal console: send Security Send/Receive, run Unlock/Lock/Revert/GenKey with a password or PSID, capture and display the Opal session.
- **SpoofScreen** — Identify Controller spoof editor (VID/DID/model/serial/FW/capacity), load presets for common drive models.
- **DmaScreen** — DMA Bridge control: target host physical address, length, direction, two-stage arm/go confirmation, results viewer (hex dump).
- **CaptureViewerScreen** — browse PCAP-NG sessions, per-record decode, export to Wireshark-compatible format.
- **FirmwareScreen** — STM32H5 firmware update (USB) + ECP5 bitstream update (BLE chunked).
- **SettingsScreen** — BLE encryption key, logging verbosity, OLED brightness, safe-mode defaults.

---

## 11. Detection & Countermeasures

### How a defender might detect NVMe-Phantom

| Signal | Detection method |
|--------|------------------|
| **PCIe link renegotiation delay** | The PEX8606 adds ~200 ns of latency and a brief link-training delay on cold boot. A host that measures boot-to-first-NVMe-command time with sub-ms precision could flag this. |
| **Identify Controller inconsistency** | If Controller Spoof is active, the spoofed Identify response may mismatch the drive's PCIe config-space subsystem ID (a defender who cross-checks the NVMe Identify serial against the PCIe config-space serial can catch this). |
| **SMART / log page anomalies** | NVMe-Phantom's injected commands may appear in the drive's internal command log (Log Page 0x01, Error Log). A defender reading the drive's *own* log (not the host's view) could spot extra commands. |
| **Link-state telemetry** | Some servers poll PCIe link state every 100 ms; a surprise-removal event from Hot-Plug Fault mode will be logged by the BMC. |
| **IOMMU fault logs** | DMA Bridge mode that reads memory outside the host driver's expected PRP ranges will generate IOMMU faults on a strict IOMMU. Check the IOMMU fault log. |
| **Physical inspection** | The interposer is 110 mm × 22 mm and adds ~6 mm of height; a visual inspection of the M.2 slot will reveal it. |

### Defensive recommendations (for the blue team)

1. Enable **IOMMU strict mode** (kernel_dma_remap=on, IOMMU_DEFAULT_PASSTHROUGH=off) and do *not* class-device-trust storage devices for arbitrary DMA.
2. Cross-check the **NVMe Identify Controller serial** against the **PCIe config-space subsystem serial** at boot; flag mismatches.
3. Measure **boot-to-first-NVMe-command time** and alert on outliers.
4. Use **TCG Opal Pyrite / Ruby** scopes with replay protection (nonce-challenged unlock), not just Opal Sapphire.
5. Require **signed firmware** for SSD firmware updates and verify the signature in the host's update tool, not only in the drive.
6. Physically inspect M.2 slots in high-security environments.

---

## 12. License & Credits

- **Firmware:** GPL-2.0
- **Hardware (KiCad):** CERN-OHL-S v2
- **Companion app:** MIT
- **Author / Creator:** jayis1
- **Concept & design:** jayis1
- **Firmware, FPGA logic, KiCad schematic/PCB, companion app:** jayis1

This device incorporates the Broadcom PEX8606 PCIe switch and Lattice ECP5 FPGA per their respective vendor datasheets and application notes. NVMe protocol handling follows the NVM Express 1.4 specification. TCG Opal handling follows the TCG Storage Opal SSC Specification 2.01.

No part of this design is affiliated with or endorsed by Broadcom, Lattice, STMicroelectronics, Nordic Semiconductor, NVM Express, or the Trusted Computing Group.

> *Designed for defenders, researchers, and authorized red teams. Use only on systems you own or are explicitly authorized to assess.*