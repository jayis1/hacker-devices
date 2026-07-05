# DRAM-PHANTOM — DDR4 SO-DIMM Interposer for Live Memory Acquisition, Rowhammer Injection & Memory Covert-Channel Research

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Tampering with memory buses, inducing Rowhammer bit-flips, or capturing live DRAM contents of systems you do not own or have not been authorized to assess may violate computer-fraud and abuse statutes (e.g., 18 U.S.C. § 1030 CFAA), data-protection regulations (GDPR, CCPA), and physical-tampering laws in your jurisdiction. Memory acquisition can expose cryptographic keys, credentials, and confidential data belonging to third parties. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**DRAM-Phantom** is a battery-backed **DDR4 SO-DIMM interposer** — a thin PCB that plugs into a target system's SO-DIMM slot while the real DRAM module plugs into a mating socket on top of the interposer. A Xilinx Artix-7 FPGA on the interposer taps the DDR4 command/address and data buses in real time, giving a security researcher four distinct capabilities on live, in-system DRAM without OS cooperation:

| Capability | What it does | Why it matters |
|------------|-------------|----------------|
| **Passive bus snoop** | Decodes every ACTIVATE / READ / WRITE / REFRESH command and every DQ burst, building a live row-access trace. | Reveals memory-access patterns of protected code, detects anomalous row-activation storms, profiles victim behaviour for Rowhammer targeting. |
| **Rowhammer injection** | Drives the CA bus as a bus master to issue rapid, repeated ACTIVATE commands on a chosen "aggressor" row, inducing bit-flips in adjacent "victim" rows. | Demonstrates Rowhammer privilege-escalation (PTE flips, `page_owner` flips) on systems without TRR / with weak TRR, for red-team privilege-escalation chains. |
| **Cold / warm boot memory imaging** | On host power loss or reset, the on-board battery holds DRAM refresh (the interposer's FPGA issues refresh commands autonomously) while the MCU streams the full DRAM contents out over USB / BLE / SD for forensic acquisition — no OS, no DMA, no kernel driver. | Recovers in-memory secrets (disk-encryption keys, session tokens, full process memory) from a seized or powered-down machine, complementing cold-boot attacks with a hardware aid that does not depend on chilling the chips. |
| **Memory covert channel** | Uses row-conflict timing (ACTIVATE-to-ACTIVATE on the same bank) as a high-bandwidth covert channel between two sandboxes/VMs that share the physical channel, even when they are logically isolated. | Demonstrates cross-tenant information leakage in public-cloud and multi-tenant embedded systems; a red-team "can we really isolate these workloads?" proof. |

### What DRAM-Phantom is *not*

It is **not** a DMA attack device. It does not impersonate a PCIe device, it does not speak to the host CPU over any standard bus, and it does not require any host-side driver, kernel module, or OS cooperation. It operates entirely at the **electrical layer of the DDR4 channel**, which makes it invisible to every software-based monitoring agent (EDR, kernel lockdown, IOMMU enforcement, secure-boot attestation) — those defenses protect software attack surfaces, not the physical DRAM channel. This is precisely what makes it a powerful red-team tool *and* a serious threat model that defenders must understand.

### Threat model & attack surface

| Trust boundary | DRAM-Phantom's position |
|-----------------|--------------------------|
| OS / kernel | **Bypassed.** No OS is involved; the device sits below the CPU's memory controller in the signal path. |
| Secure Boot / measured boot | **Bypassed.** Boot integrity verifies firmware *of the host*; it does not verify the wires between the memory controller and the DRAM. |
| IOMMU / DMA protections | **Irrelevant.** DRAM-Phantom is not a DMA initiator on any host-visible bus. |
| TPM / disk encryption | **Targeted indirectly.** Disk-encryption keys live in DRAM; DRAM-Phantom can image that DRAM on reset. |
| TRR (Target Row Refresh) | **Adversarial.** Rowhammer injection mode actively probes for and works around weak TRR by hammering patterns that defeat single-row refresh-on-activate schemes. |
| Physical tamper seals | **Defeated by form factor.** The interposer is 0.8 mm thick and fits inside the SO-DIMM bay of most laptops and mini-PCs with minimal mechanical clearance. |

### Why DRAM-Phantom is novel

Existing memory-acquisition and Rowhammer research is done with one of three approaches, each of which has a major limitation:

1. **Software Rowhammer (e.g., Hammernet, Rowhammer.js, TRRespass)** — runs from inside the OS and relies on cache-flush primitives (`CLFLUSH`, `DC IVAC`) or huge pages to hammer rows. Defeated by any OS that restricts cache flushing, by TRR-aware controllers, and by the memory controller's own command reordering, which the software attacker cannot see or control.
2. **PCIe DMA attack (e.g., PCILeech / FPGA DMA)** — requires a free PCIe slot or Thunderbolt port, is blocked by IOMMU when configured, and is loud (a new device appears on the host bus).
3. **Cold-boot with canned-air chilling** — requires physical removal of the DRAM and rapid chilling; unreliable on soldered LPDDR, and once you remove the module you cannot refresh it in-system.

DRAM-Phantom is the **first device to attack the DDR4 channel itself**, *in situ*, at the electrical layer:

- It sees the **exact command stream** the memory controller issues, so it can target the precise row the OS just touched (no guessing via huge pages).
- It can drive the CA bus **as a master** while the host memory controller is quiesced (reset asserted, CKE held low), giving deterministic Rowhammer control no software attacker has.
- It can **hold refresh** of the DRAM after host power-off using its own battery and an autonomous refresh state machine, then drain the contents over USB — a "warm boot" acquisition that does not need chilling and works on soldered DRAM where you cannot physically remove the module.
- The **covert channel** mode is unique: by measuring ACTIVATE-to-ACTIVATE latency on a shared bank, two colluding sandboxes (one with a sender bit, one with a receiver) can exfiltrate data with no software-visible network, file, or syscall activity.

No prior open hardware tool provides all four capabilities on a DDR4 channel.

---

## 2. Hardware Specifications

### 2.1 Form factor

- **DDR4 SO-DIMM interposer**, 260-pin mating connector on the bottom (plugs into the host slot) and a 260-pin SO-DIMM socket on top (the real module plugs in).
- Interposer PCB: **0.8 mm thick**, 6-layer, 69.6 mm × 30 mm (SO-DIMM PCB envelope, slightly extended on one end for the FPGA + MCU cluster).
- Passive passthrough of all 260 pins through a controlled-impedance fan-out; only CA bus + DQS + a subset of DQ are tapped to the FPGA via series resistors for non-intrusive snooping.
- A small **LiPo pouch cell** (3.7 V, 500 mAh) sits in the extended area and powers refresh during warm-boot acquisition; charged from the host VDDSPD / VPP rail when the host is on.

### 2.2 Compute

| Function | Part | Role |
|----------|------|------|
| Bus tap / snoop / Rowhammer engine | **Xilinx Artix-7 XC7A100T-2FGG484I** | 100K-logic-cell FPGA with 240 DSP slices and 16-transceiver-grade I/O banks; runs the DDR4 CA-bus monitor, the autonomous refresh state machine, the Rowhammer pattern generator, and the DQ burst-capture FIFO. |
| Companion MCU | **STM32G474CEU6** (Cortex-M4, 170 MHz) | Manages the app interface (USB CDC + BLE), the OLED, the SD card, command dispatch to the FPGA, and the warm-boot acquisition drain over USB Mass Storage. |
| Wireless link | **Nordic nRF52840** module (BLE 5 + USB CDC bridge) | Out-of-band command/control and status streaming to the companion app when USB cannot be wired (e.g., target is a sealed laptop). |
| Display | **0.96" OLED, SSD1306, 128×64, I²C** | Live status: row-access rate, hammer counter, acquisition progress, battery state. |
| Storage | **microSD socket** (UHS-I, SPI + 4-bit SDIO) | Buffered full-DRAM-image dump destination when USB host is not available. |
| Battery / power | **LiPo 3.7 V 500 mAh + MAX17048 fuel gauge + TPS63020 buck-boost** | Holds DRAM refresh for ~90 s after host power loss; charges from host VDD (2.5 V VDDSPD is lifted to 3.3 V via the boost) or USB. |

### 2.3 DDR4 electrical interface (tap topology)

The interposer taps the following DDR4 signals (all 1.2 V VDDQ, SSTL_12 / POD12):

- **Command / Address (CA):** `A[0:17]`, `BA[0:1]`, `BG[0:1]`, `CKE`, `CS#`, `ACT#`, `RAS#`/`A16`, `CAS#`/`A15`, `WE#`/`A14`, `ODT`, `PAR`, `RESET#`, `ZQ`. Tapped via **22 Ω series resistors** into the FPGA (the FPGA presents a high-impedance LVCMOS12 input; the series resistor isolates the FPGA's input capacitance from the DDR4 CA bus to keep stub reflections under the JEDEC budget).
- **Clock:** `CK_t`, `CK_c` (differential), tapped to an FPGA clock-capable pair via AC-coupling caps so the FPGA recovers the controller's exact clock domain.
- **Data strobes:** `DQS0_t/c`, `DQS1_t/c` (the two byte-lane strobes for an x16 module) tapped via 33 Ω series resistors.
- **Data (subset):** `DQ[0:7]` and `DQ[8:15]` tapped via 33 Ω series resistors — the FPGA captures the data byte on each DQS edge for burst logging. (For x8 modules, only one byte lane is populated.)
- **Drive capability (master mode):** In Rowhammer and warm-boot modes the FPGA asserts its own drivers on `CS#`, `ACT#`, `RAS#`, `CAS#`, `WE#`, `CKE`, `A[0:17]`, `BA`, `BG`, and `CK_t/c` to issue commands. The host memory controller is quiesced by the MCU asserting `RESET#` low (open-drain, gated through a series resistor) before the FPGA takes the bus. `ODT` is managed by the FPGA in master mode.

The tap is **non-intrusive** in snoop mode: the FPGA inputs are high impedance, the series resistors dominate the stub, and the host memory controller sees only a small capacitive bump (~2–3 pF), well within DDR4 margin budgets at 3200 MT/s. In master mode the host is held in reset, so there is no contention.

### 2.4 Connectivity

- **USB-C** (USB 2.0 Full-Speed via STM32 + a USB-C receptacle on the extended interposer tail): configuration, command, and warm-boot image drain.
- **BLE 5** (nRF52840): wireless control/status for sealed-target scenarios; paired to the companion app.
- **microSD**: up to 2 GB FAT16 image dumps (one per acquisition session), plus CSV row-access traces.
- **OLED**: 128×64 status display, always-on in snoop mode (drawn from host VDD when present, from battery otherwise).
- **Tactile button**: mode-cycle (Snoop → Rowhammer → Warm-Boot → Covert-Channel → Idle) for headless field operation.

### 2.5 Power budget

| Rail | Source | Load | Notes |
|------|--------|------|-------|
| 1.2 V VDDQ | Host (passthrough) | DRAM + FPGA I/O bank | FPGA VCCINT is local 1.0 V from buck. |
| 1.0 V FPGA core | TPS62110 from host VDD or battery | ~250 mA peak | Powers Artix-7 fabric + BRAM. |
| 3.3 V | TPS63020 buck-boost from battery or host | MCU, BLE, OLED, SD, level shift | Always-on rail. |
| Battery | 500 mAh LiPo | Refresh-only drain ~180 mA | ~90 s autonomous refresh budget. |

---

## 3. Architecture & Block Diagram

```
                ┌─────────────────────────────────────────────────────────────┐
                │                        TARGET SYSTEM                        │
                │   ┌─────────────┐        DDR4 channel        ┌────────────┐ │
                │   │ CPU / SoC   │───────────────────────────▶│   DDR4     │ │
                │   │ mem ctrl    │  CK_t/c, CA[0:25], DQ, DQS  │  SO-DIMM   │ │
                │   └─────────────┘                            └────────────┘ │
                └───────────────┬─────────────────────────────────┬──────────┘
                                │ (plugs into host slot)            │ (real module plugs here)
                ┌───────────────▼─────────────────────────────────▼──────────┐
                │                    DRAM-PHANTOM INTERPOSER                    │
                │  ┌──────────────────────────────────────────────────────┐  │
                │  │  DDR4 SO-DIMM edge (260-pin) ──── SO-DIMM socket (top)│  │
                │  │         │ series-R taps (CA, CK, DQS, DQ subset)      │  │
                │  │         ▼                                              │  │
                │  │  ┌─────────────────────────────┐   ┌────────────────┐ │  │
                │  │  │   Artix-7 XC7A100T FPGA      │   │  STM32G474 MCU │ │  │
                │  │  │  ┌───────────────────────┐  │   │  (Cortex-M4)   │ │  │
                │  │  │  │ DDR4 CA monitor /     │  │◀──│  - cmd dispatch │ │  │
                │  │  │  │ row-access trace FIFO │  │SPI│  - USB CDC      │ │  │
                │  │  │  ├───────────────────────┤  │   │  - BLE bridge   │ │  │
                │  │  │  │ Rowhammer pattern gen │  │   │  - SD card I/O  │ │  │
                │  │  │  ├───────────────────────┤  │   │  - OLED I²C     │ │  │
                │  │  │  │ Autonomous refresh SM │  │   │  - mode button │ │  │
                │  │  │  ├───────────────────────┤  │   └──────┬─────────┘ │  │
                │  │  │  │ DQ burst capture FIFO │  │          │           │  │
                │  │  │  ├───────────────────────┤  │   ┌──────▼─────────┐ │  │
                │  │  │  │ Covert-channel timing │  │   │  nRF52840 BLE  │ │  │
                │  │  │  └───────────────────────┘  │   └────────────────┘ │  │
                │  │  └─────────────────────────────┘                      │  │
                │  │  ┌───────────┐  ┌─────────────┐  ┌────────────────┐   │  │
                │  │  │  OLED     │  │ microSD     │  │ USB-C / LiPo / │   │  │
                │  │  │ SSD1306   │  │  socket     │  │ fuel gauge     │   │  │
                │  │  └───────────┘  └─────────────┘  └────────────────┘   │  │
                │  └──────────────────────────────────────────────────────┘  │
                └─────────────────────────────────────────────────────────────┘
                                              │ BLE / USB
                                              ▼
                            ┌────────────────────────────────┐
                            │   Companion App (React Native)  │
                            │  Snoop | Rowhammer | WarmBoot | │
                            │  CovertChannel | Settings        │
                            └────────────────────────────────┘
```

### Data flow

1. **Snoop mode:** The FPGA's CA monitor decodes every command in real time (clocked by the recovered host CK), classifies it (ACT/READ/WRITE/REF/PRE), and pushes a compact record (`{bank_group, bank, row, col, op, t}`) into a 36 Kb BRAM ring FIFO. The STM32 drains the FIFO over SPI at ~20 MHz and either streams it to USB CDC / BLE or writes CSV to SD. The DQ capture FIFO optionally snapshots the data byte for each READ/WRITE for full-content logging.

2. **Rowhammer mode:** The MCU asserts host `RESET#` (open-drain, gated by a series resistor so the host mem-controller releases the bus), the FPGA switches its CA I/O to output, and the Rowhammer pattern generator issues a deterministic `ACT(row=R) / PRE / ACT(row=R) / PRE ...` loop at the maximum the DRAM tRC allows (e.g., 48 ns at DDR4-3200). After N hammers the FPGA issues a `READ` on the victim row `R±1` and the DQ capture FIFO records the data; the MCU diffs against the pre-hammer snapshot to flag flipped bits.

3. **Warm-boot mode:** On host power-loss detection (VDD drops below 1.0 V), the MCU cuts the host VDD sense, the FPGA's autonomous refresh state machine takes over CA (`REF` every tREFI = 7.8 µs) from battery, and the MCU begins a `READ` sweep across all rows/banks, draining DQ bytes over SPI to a USB Mass Storage / SD image file. The full contents of an 8 GB module are drained in ~110 s.

4. **Covert-channel mode:** The FPGA measures the ACTIVATE-to-ACTIVATE latency on a chosen bank and exposes a 1-bit "channel busy" signal modulated by row conflicts; two colluding sandboxes (sender writes a row in bank K, receiver tries to activate a different row in bank K and times the conflict) exfiltrate data with no software-visible channel.

---

## 4. Firmware

### 4.1 Design decisions

- **FPGA-side logic (Artix-7):** implemented in Verilog (described in the README; the firmware tree here is the MCU side). The FPGA bitstream is generated from `rtl/` Verilog sources (out of scope for this repo's C tree, but the MCU side speaks to the FPGA over an SPI command interface described in `fpga.c`).
- **MCU firmware (STM32G474):** bare-metal C, no RTOS, a tiny cooperative scheduler in `main.c`. All drivers are register-level (no HAL dependency) so the build is reproducible with `arm-none-eabi-gcc` and the included `link.ld`.
- **Why bare-metal:** predictability of the warm-boot drain — we cannot afford jitter in the SPI drain that would starve the FPGA's refresh budget.
- **No DMA on the SPI-to-FPGA path** deliberately: we want the MCU to be in full control of every command the FPGA issues, so a bug never causes an unintended Rowhammer on a real machine. All hammer patterns are explicitly armed by a two-key sequence (physical button + app confirm).

### 4.2 File layout

```
firmware/
├── Makefile              # arm-none-eabi-gcc build for STM32G474
├── link.ld               # linker script (Cortex-M4, 512 KB flash, 128 KB SRAM)
├── startup_stm32g474.s   # vector table + reset handler
├── board.h               # pin map, peripheral assignments
├── registers.h           # STM32G4 register definitions
├── main.c                # scheduler, mode state machine, command dispatch
└── drivers/
    ├── board_init.c      # clocks, GPIO, NVIC, peripheral enable
    ├── fpga.c            # SPI command interface to Artix-7 (load patterns, drain FIFOs)
    ├── ddr4_snoop.c      # decodes snoop FIFO records, builds row-access trace
    ├── rowhammer.c       # arms + executes Rowhammer patterns, reads bit-flip results
    ├── mem_capture.c     # warm-boot READ sweep + image drain to SD / USB
    ├── covert_channel.c  # configures bank-conflict timing sensor
    ├── usb_cdc.c         # USB CDC virtual serial for app
    ├── ble_uart.c         # UART bridge to nRF52840
    ├── sdcard.c          # SDIO + FatFs-lite for image dumps and CSV traces
    └── oled.c            # SSD1306 status display
```

See the firmware files in this directory for the full, commented source. The MCU firmware is **530+ lines of real C** across the tree.

### 4.3 Safety interlocks

- **Two-key arm:** every destructive operation (Rowhammer, warm-boot takeover, covert-channel timing sensor enable) requires (a) the physical mode button to be held for ≥3 s and (b) a `CONFIRM <token>` command over USB/BLE with a token shown on the OLED. This prevents accidental hammering when the interposer is plugged into an unrelated machine.
- **Target-presence check:** before any master-mode operation the MCU reads the FPGA's `SPD_EEPROM` decoder to confirm a real DDR4 module is present and its density is known; if not, the operation aborts.
- **Refresh watchdog:** in warm-boot mode a hardware timer in the MCU re-arms the FPGA refresh command every tREFI regardless of MCU firmware state; if the MCU hangs, DRAM stays refreshed for the full battery budget.

---

## 5. Application / Software Interface

The companion app is a **React Native** application (see `app/`) that pairs with DRAM-Phantom over BLE or USB CDC. It provides four primary screens mirroring the device modes:

- **SnoopScreen** — live row-access trace viewer, configurable filters (bank, row, op type), export to CSV.
- **RowhammerScreen** — aggressor-row selector, hammer count, victim-row auto-sweep, bit-flip heatmap, side-by-side pre/post dump diff.
- **WarmBootScreen** — acquisition progress bar (rows × banks), estimated time, image download to phone storage, SHA-256 of dump.
- **CovertChannelScreen** — bank selector, timing histogram, bit-rate estimator, sender/receiver role toggle.
- **SettingsScreen** — battery state, firmware version, two-key arm token display, SPD info read-back, factory self-test.

The app uses a shared `DeviceContext` (BLE/USB transport abstraction) so the same screens work over either transport.

---

## 6. Use Cases

### 6.1 Red team — privilege escalation via Rowhammer

A red-team operator with brief physical access to a target laptop (e.g., during a "borrowed" coffee-break window) plugs DRAM-Phantom into a free SO-DIMM slot (or replaces a module if only one slot is populated), boots the target normally, and from the companion app arms a Rowhammer pattern targeting the row that the OS just allocated for the victim's page table. The FPGA, with deterministic command timing no software attacker can achieve, flips a PTE present bit, granting the attacker write access to a kernel page. The whole operation takes seconds of physical access plus a few minutes of in-app hammering, and leaves **no software artifact** on the host — no driver, no log, no new bus device.

### 6.2 Forensics — warm-boot memory acquisition of a powered-down machine

A seized laptop is powered off; its disk is encrypted. The examiner opens the SO-DIMM bay, swaps one module for DRAM-Phantom + the original module (the interposer holds refresh from battery), powers the machine briefly to let the OS load keys into DRAM, then triggers a reset. DRAM-Phantom's FPGA holds refresh and drains the full DRAM image to SD over the next two minutes. The examiner then analyses the image offline with Volatility/rekall to extract the disk-encryption key — no chilling, no module removal, works on soldered LPDDR4 where the interposer can be wire-tapped onto the board.

### 6.3 Security research — TRR effectiveness testing

A researcher evaluating a new DRAM part's Target Row Refresh (TRR) effectiveness uses DRAM-Phantom to issue known-defeating patterns (e.g., the "double-sided, many-sided" patterns from TRRespass) directly on the CA bus with precise tRC control, then sweeps victim rows for bit flips. Because the FPGA sees the host's own refresh schedule, it can time its hammers to land *between* host refreshes, maximising the chance of defeating weak TRR — something software-only hammerers cannot do because they don't see the controller's refresh schedule.

### 6.4 Cloud / multi-tenant — covert-channel demonstration

Two colluding workloads on a shared physical host (e.g., a cloud tenant that has rented two small VMs that happen to map to the same physical bank) use DRAM-Phantom's covert-channel timing sensor to exfiltrate data with no network, no syscall, no file activity — only row-conflict timing. This is a red-team proof that "logical isolation" of co-tenants is not "physical isolation," informing cloud-provider hardening decisions.

### 6.5 Defender — memory-resident implant detection

A defender who suspects a kernel-mode rootkit uses DRAM-Phantom in snoop mode to record every WRITE to a chosen physical-page range (e.g., the kernel's `modprobe_path` or a syscall table region). Any unscheduled WRITE to that region is a strong indicator of a memory-resident implant modifying kernel state from a non-CPU agent — something host-side EDR fundamentally cannot see because the EDR itself runs on the CPU whose writes it would need to monitor.

---

## 7. Bill of Materials (key parts)

| Ref | Part | Footprint | Qty |
|-----|------|-----------|-----|
| U1 | Xilinx Artix-7 XC7A100T-2FGG484I | FGG484 | 1 |
| U2 | ST STM32G474CEU6 | UFQFPN-48 | 1 |
| U3 | Nordic nRF52840 module (Fanstel BT840F) | castellated | 1 |
| U4 | TI TPS63020DSJR | SON-14 | 1 |
| U5 | TI TPS62110 | VQFN-16 | 1 |
| U6 | Maxim MAX17048 fuel gauge | TDFN-8 | 1 |
| U7 | ISSI IS25LP032D 32 Mb QSPI flash (FPGA bitstream) | SOIC-8 | 1 |
| U8 | DDR4 SO-DIMM 260-pin socket (top) | SODIMM-260 | 1 |
| J1 | DDR4 SO-DIMM 260-pin edge (bottom, PCB edge fingers) | edge | 1 |
| J2 | USB-C receptacle | USB-C-16P | 1 |
| J3 | microSD socket (Hirose DM3D-SF) | SMD | 1 |
| DS1 | 0.96" OLED SSD1306 I²C | FPC-8 | 1 |
| BT1 | LiPo 3.7 V 500 mAh pouch | wire | 1 |
| SW1 | Tactile button (mode) | SMD | 1 |

---

## 8. Legal / Ethical Use

DRAM-Phantom is a research instrument. Use it only on systems you own or for which you have **explicit, written authorization** to assess. Memory acquisition, Rowhammer injection, and covert-channel timing on systems you do not own may constitute unauthorized access under computer-fraud statutes (CFAA, CMA, GDPR Art. 32) and may expose third-party data. The author (**jayis1**) provides this design for educational and authorized-research purposes and assumes no liability for misuse. When in doubt, do not deploy.

---

## 9. License & Author

- **Hardware (KiCad):** CERN-OHL-S v2
- **Firmware & app:** GPL-2.0
- **Author:** jayis1
- **Project:** DRAM-Phantom v1.0

---

*Designed by jayis1 for authorized security research.*