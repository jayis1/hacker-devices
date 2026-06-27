# Tactile-Phantom — Covert Touch-Controller Bus MITM Implant

```
   ╔══════════════════════════════════════════════════════════════════════════╗
   ║  ████████╗██████╗  █████╗ ██╗      ██████╗  ██████╗ ██╗  ██╗███████╗     ║
   ║  ╚══██╔══╝██╔══██╗██╔══██╗██║     ██╔══██╗██╔═══██╗██║ ██╔╝██╔════╝     ║
   ║     ██║   ██████╔╝███████║██║     ██║  ██║██║   ██║█████╔╝ █████╗       ║
   ║     ██║   ██╔══██╗██╔══██║██║     ██║  ██║██║   ██║██╔═██╗ ██╔══╝       ║
   ║     ██║   ██║  ██║██║  ██║███████╗██████╔╝╚██████╔╝██║  ██╗███████╗     ║
   ║     ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚══════╝     ║
   ║                                                                         ║
   ║   Covert Touch-Controller Bus MITM — Capture & Inject Tactile Events    ║
   ╚══════════════════════════════════════════════════════════════════════════╝
```

**Author:** jayis1  
**Version:** 1.0.0  
**License:** GPL-2.0  
**Status:** Research hardware design — firmware + PCB + companion app

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit written permission to assess. Intercepting touch input or injecting synthetic touch events on devices you do not own may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), and data-protection regulations in your jurisdiction. Capturing unlock patterns, PINs, or soft-keyboard input from devices you do not own can constitute unauthorized access to protected computing resources and interception of electronic communications. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## Overview

**Tactile-Phantom** is a coin-cell-powered, covert man-in-the-middle (MITM) implant that physically taps the I²C or SPI bus between a target device's **touch-sensor controller (TSC)** integrated circuit and its host application processor (AP). By sitting inline on this low-level peripheral bus, Tactile-Phantom operates *below the operating system* — capturing every touch coordinate, gesture, and soft-button press as raw controller traffic, and optionally injecting synthetic touch events to remotely drive the target's user interface.

### Why this attack surface is unique

Every modern touchscreen device — smartphone, tablet, IoT HMI, automotive infotainment, point-of-sale terminal, smart-lock keypad, industrial control panel — contains a dedicated touch-controller IC (e.g., Goodix, FocalTech, Synaptics, Ilitek, Cypress/Infineon, Atmel/microchip) that digitizes the capacitive matrix and reports touch coordinates to the host SoC over a simple I²C or SPI bus. This bus is:

1. **Below the OS:** Touch events arrive at the AP's I²C/SPI peripheral controller long before any OS-level input subsystem, secure-input layer, or anti-keylogger software can observe them. There is no software hook in the OS that sees touch data earlier.
2. **Unencrypted:** Virtually no commercial touch-controller bus uses encryption or authentication. The I²C/SPI traffic is plaintext register reads/writes.
3. **Physical & accessible:** The bus traces run between two chips on a PCB (or across a flex connector) and are readily tapped at the FPC connector, test pads, or via microscopic probing — no silicon decapsulation needed.
4. **Bidirectional:** The host AP both *reads* touch data and *writes* configuration registers (sensitivity, scan frequency, firmware update). An inline MITM can modify either direction.

Tactile-Phantom exploits all four properties. It is the first open hardware tool purpose-built for touch-controller bus interception, complementing existing keyboard HID loggers (which operate at the USB/OS layer) and display-side capture tools (like HDMI-Siphon).

### What it does

| Capability | Description |
|---|---|
| **Passive capture** | Sniffs all I²C/SPI traffic between TSC and AP, reconstructing touch coordinates, multi-touch state, gestures, and host configuration writes. Decodes common Goodix, FocalTech, Synaptics, Ilitek, and Cypress register maps. |
| **Input logging** | Reconstructs unlock patterns (Android), PIN entry, soft-keyboard keystrokes (with on-screen layout inference), and gesture sequences. Streams events to SD card and over BLE. |
| **Event injection** | Synthesizes touch-controller register payloads that mimic real touch events, letting the operator remotely tap, swipe, and long-press on the target screen — full remote UI control without the target OS knowing the input is synthetic. |
| **Configuration tampering** | Modifies host→TSC configuration writes on the fly: disable touch (DoS), change sensitivity, disable glove mode, or force firmware-update mode to enable further attacks. |
| **Firmware extraction**** | Captures host-initiated touch-controller firmware updates over I²C/SPI, dumping the TSC firmware image for reverse engineering. |
| **Bus auto-detect** | Automatically detects I²C vs SPI pinout, clock speed, address/slave-select, and protocol variant by observing initial host enumeration traffic. |
| **Covert form factor** | 12 mm × 18 mm flex-PCB tail injects inline at the TSC FPC connector; main module is 25 mm × 35 mm and tucks behind a display assembly. Coin-cell or parasitic power. |

### Threat model & attack surface

**Target devices:** Smartphones, tablets, point-of-sale terminals, automotive infotainment systems, smart-lock capacitive keypads, industrial HMI panels, medical device touchscreens, kiosks, and any embedded system with a capacitive touch interface.

**Attack prerequisites:** Physical access to the target device sufficient to access the touch-controller FPC connector or test pads (typically requires disassembly or access during manufacturing/repair supply chain). For supply-chain implant scenarios, the device can be pre-inserted during manufacturing or refurbishment.

**Trust boundary violated:** The implicit trust between the application processor and its touch-controller IC. Modern OSes assume the touch-controller is a trusted peripheral reporting genuine user input. Tactile-Phantom breaks this assumption in two directions: (1) by exfiltrating user input that the user believes is only seen by the legitimate OS, and (2) by injecting input that the OS believes came from a genuine human finger on the screen.

**What OS-level defenses cannot prevent:**
- Anti-keylogger software (operates at OS input subsystem, too late)
- Secure input modes (on-screen randomized PIN pads — captured at bus level before OS randomization helps)
- Encrypted storage of PINs (the *entry* is captured before it reaches the app)
- Biometric fallback prompts (the bus sees the touch coordinates of the biometric enrollment screen too)
- Screen-lock bypass detection (injected touches look like real user interaction to the OS)

**Operational scenarios:**
1. **Forensic acquisition:** Capturing a locked device's unlock pattern/PIN by tapping the bus during legitimate user unlock, then replaying.
2. **Red-team remote control:** Implanting on a target kiosk/POS terminal, then remotely driving the UI to navigate menus, authorize transactions, or change settings.
3. **Supply-chain interdiction:** Pre-installing during device refurbishment or manufacturing to create persistent input-logging capability.
4. **Automotive:** Tapping infotainment touch bus to capture navigation input, phone pairing PINs, or inject UI interaction while the vehicle is in motion.
5. **IoT HMI:** Capturing passcodes on smart-lock capacitive keypads or industrial control panel inputs.

---

## Hardware Specifications

### Block diagram

```
                          ┌─────────────────────────────────────────────┐
                          │              TACTILE-PHANTOM                 │
                          │                                             │
   ┌─────────┐    I²C/SPI  │  ┌──────────┐   DMA   ┌──────────────┐      │   BLE
   │  Touch  │◄───────────┼──┤ Bus Tap  ├───────►│   RP2040     │◄────┼─────────►
   │Control- │   (inline) │  │ & MUX    │        │   MCU         │     │  Operator
   │  ler IC │◄───────────┼──┤  (TXB   │        │  (2× PIO +   │     │   Phone
   └─────────┘            │  │  0108)  │        │   USB CDC)    │     │  (App)
        ▲                 │  └──────────┘        └──────┬───────┘     │
        │ capacitive      │                             │              │
        │ matrix          │                ┌────────────┼──────────┐  │
   ┌────┴────┐            │                │            │          │  │
   │ Display │            │           ┌──────┐   ┌──────┴────┐  ┌──┴──┴───┐
   │  glass  │            │           │ SD   │   │  Status   │  │ LiPo +  │
   └─────────┘            │           │ Card │   │  LED/OLED │  │ Charger │
                          │           └──────┘   └───────────┘  └─────────┘
                          └─────────────────────────────────────────────┘
                                     │
                              ┌──────┴──────┐
                              │  Target AP  │
                              │  (I²C/SPI   │
                              │   host)     │
                              └─────────────┘
```

### Core components

| Component | Part | Purpose |
|---|---|---|
| **MCU** | Raspberry Pi RP2040 (QFN-56) | Dual ARM Cortex-M0+ @ 133 MHz with 8× PIO state machines. PIO is the key enabler: two state machines bit-bang I²C/SPI *transparently* between TSC and AP while DMA-ing a copy of every transaction to SRAM, achieving true inline MITM without bus timing violation. 264 KB SRAM holds packet buffers and touch-decode state. |
| **Bus tap / level shifter** | TI TXB0108 (8-bit bidirectional auto-direction-sensing level shifter) ×2 | Provides voltage-level translation between target bus (1.2–3.3 V) and RP2040 (3.3 V) with minimal propagation delay (<3 ns). Two chips handle 8 signal lines covering I²C (SDA/SCL) or SPI (MOSI/MISO/SCK/CS + 2 spare). Auto-direction sense means no direction pins needed — critical for I²C where direction flips per byte. |
| **BLE module** | Espressif ESP32-C3-MINI-1 | BLE 5.0 backhaul to operator phone, WiFi for bulk dump offload, and over-the-air firmware updates to the implant. Communicates with RP2040 over UART @ 1 Mbps. |
| **Storage** | MicroSD card socket (SPI mode) | Bulk capture logging — a 32 GB card holds weeks of touch-event logs. RP2040 drives SD via second SPI peripheral; ESP32-C3 can also access via shared SPI mux. |
| **Status display** | 0.42" OLED (SSD1306, I²C) | Minimal operator feedback: bus type detected, capture active, injection armed, battery %. |
| **Power** | 120 mAh LiPo + MCP73831 charger + MAX17048 fuel gauge | Coin-cell option via CR2032 pads. Parasitic power tap from target I²C pull-up VCC (3.3 V, <2 mA budget). LiPo gives ~40 h active capture. |
| **USB** | USB-C (USB 2.0 FS) | Configuration, firmware update, and wired exfiltration. RP2040 TinyUSB CDC. |
| **Flex tail** | 6-pin 0.5 mm pitch FPC (A-side plug, B-side receptacle) | Breaks into the TSC FPC cable. The target's original FPC plugs into Tactile-Phantom's receptacle; Tactile-Phantom's plug goes to the target. Carries SDA/SCL/GND/VCC + 2 spare (INT/GPIO/RST lines common on TSC FPCs). |

### Electrical specifications

| Parameter | Value |
|---|---|
| Target bus voltage | 1.2 V to 3.6 V (level-shifted) |
| Supported I²C speeds | 100 kHz, 400 kHz, 1 MHz (Fast-mode Plus) |
| Supported SPI speeds | Up to 8 MHz (PIO-limited, tap-through) |
| Bus propagation delay | < 5 ns (TXB0108 + PIO pipeline) |
| Capture throughput | Up to 20 k transactions/s sustained (DMA + ring buffer) |
| BLE range | 10 m line-of-sight (BLE 5.0, +4 dBm) |
| Power consumption | 18 mA active capture (RP2040 + ESP32-C3 sleep-between-events), 0.4 mA deep-sleep trigger-armed |
| Battery life | ~40 h active, ~30 days trigger-armed (120 mAh LiPo) |
| Form factor | Main module 25 mm × 35 mm × 4 mm; flex tail 12 mm × 40 mm × 0.2 mm |
| Weight | 3.5 g (with battery) |

### Form factor

The main PCB is a 4-layer board designed to tuck behind a phone's display assembly or within a tablet's bezel cavity. The flex tail terminates in a standard 0.5 mm FFC/FPC connector that mates with the target's existing touch-controller cable — no soldering required for devices with accessible FPC connectors. For devices requiring solder-down, the flex tail has unpopulated pads for 0.4 mm pitch micro-clips.

---

## Architecture

### Dual-PIO transparent MITM engine

The central innovation is the use of RP2040's PIO (Programmable I/O) state machines to create a **transparent bus tap with zero added latency on the critical path**, while DMA copies every transaction to a decode buffer. This is fundamentally different from a passive sniffer (which only listens) or a bus-pirate-style active proxy (which terminates and re-drives, adding latency and risking protocol timeouts).

**How it works:**

Two PIO state machines are used per bus line pair:

1. **Through-path SM** (one per direction): The PIO samples the bus line on each clock edge and re-drives it on the opposite side of the TXB0108, with a pipeline depth of just 1 clock cycle. For I²C, this means SCL is forwarded SCL_in→SCL_out with <3 ns delay; SDA is bidirectionally forwarded with auto-direction via TXB0108. For SPI, MOSI/SCK/CS are forwarded AP→TSC and MISO is forwarded TSC→AP, each through a dedicated PIO SM.

2. **Tap SM** (one per direction): A second PIO SM monitors the same line, clocking each bit into a 32-bit shift register and DMA-ing completed words to an SRAM ring buffer. The tap SM does not drive any line — it is read-only, so it cannot perturb bus timing.

Because the through-path SM adds only one clock cycle of latency (8 ns at 125 MHz PIO clock), even 1 MHz I²C Fast-mode-Plus and 8 MHz SPI operate within tolerance — the host AP sees timing indistinguishable from a direct connection.

### Firmware decode pipeline

The RP2040 firmware runs a multi-stage decode pipeline:

```
   PIO tap → DMA ring buffer → frame parser → touch decoder → event logger
                                        ↓
                              protocol classifier (I²C/SPI auto-detect)
                                        ↓
                              chip register-map decoder
                              (Goodix / FocalTech / Synaptics / Ilitek / Cypress)
                                        ↓
                              gesture/keystroke reconstruction
                                        ↓
                              SD log + BLE stream + injection queue
```

**Stage 1 — Frame parser:** Identifies I²C START/STOP/ACK/NACK and address bytes, or SPI chip-select framing, and assembles complete transactions (addr + data).

**Stage 2 — Protocol classifier:** Runs only during the first 100 ms after bus attach. Detects I²C vs SPI, clock speed, slave address (I²C) / chip-select (SPI), and identifies the touch-controller IC by its enumeration register reads (each vendor has a distinctive chip-ID register).

**Stage 3 — Register-map decoder:** Maps vendor-specific register addresses to semantic meaning: touch-state register, coordinate registers, gesture register, config registers, firmware-update registers. Maintains a shadow model of the controller's register state.

**Stage 4 — Touch decoder:** Converts raw register reads into abstract touch events: `{finger_id, x, y, pressure, width, timestamp}`. Handles multi-touch (up to 10 fingers), gesture registers (swipe, double-tap, long-press), and proximity events.

**Stage 5 — Event reconstruction:** Higher-level inference: maps touch coordinates to on-screen UI elements using a configurable screen layout (soft-keyboard layout, dialer pad, unlock grid). Reconstructs typed strings, PINs, and unlock patterns. This stage runs on the ESP32-C3 (more compute) using a layout model pushed from the companion app.

### Injection engine

Injection synthesizes touch-controller register payloads indistinguishable from real touch events. The injection path uses a third PIO SM that takes ownership of the bus for one transaction window (between host polls) and writes the touch-state register with attacker-controlled coordinates. The host AP, polling on its normal schedule, reads the injected coordinates and processes them as genuine user input.

Key injection primitives:
- `tap(x, y, duration_ms)` — single tap
- `swipe(x1, y1, x2, y2, duration_ms)` — drag gesture
- `long_press(x, y, duration_ms)` — long-press for context menus
- `multi_touch([(x1,y1), (x2,y2), ...], duration_ms)` — pinch/rotate
- `pattern([(x,y), ...])` — replay a captured unlock pattern
- `type_string("text")` — map characters to soft-keyboard coordinates and tap each in sequence
- `inject_raw(register_addr, data)` — write arbitrary TSC config register

Injection commands arrive over BLE from the companion app and are queued in an SRAM injection queue. The firmware schedules injection during the host's inter-poll gap (typically 4–10 ms) to avoid bus contention.

---

## Firmware

### Design decisions

1. **RP2040 over a custom FPGA:** PIO provides the deterministic bit-level timing needed for transparent bus forwarding without FPGA dev complexity. Two Cortex-M0+ cores split work: core 0 runs PIO + DMA management (real-time), core 1 runs the decode pipeline (best-effort).

2. **TXB0108 over discrete FET level shifting:** The auto-direction-sensing architecture of TXB0108 is essential for I²C where SDA direction reverses per ACK bit. Discrete FET shifters need explicit direction control that I²C doesn't provide. The <3 ns propagation is critical to stay within bus timing margin.

3. **ESP32-C3 for BLE, not RP2040 + external BLE:** The C3 gives WiFi for bulk offload and OTA updates in addition to BLE, in a tiny module, with mature BLE stack. RP2040 focuses on real-time bus handling; C3 handles comms and high-level inference. UART @ 1 Mbps between them (RP2040 UART has large FIFO).

4. **Vendor register maps as data, not code:** Each touch-controller register map (Goodix GT9xx, FocalTech FT5xx, Synaptics S33xx, Ilitek ILI2xxx, Cypress TT2xx) is a compile-time table mapping register offset → semantic type. Adding a new controller means adding a table, not rewriting code.

5. **Shadow register model:** The firmware maintains a mirror of the TSC's full register state, updated on every host read/write. This lets injection write consistent register states (e.g., touch-count register must agree with coordinate registers) and lets the firmware detect when the host changes config (e.g., switches to firmware-update mode, which the firmware then captures).

### File structure

```
firmware/
├── Makefile           — Pico SDK build for RP2040, IDF build for ESP32-C3
├── board.h            — Pin definitions, bus constants, board config
├── registers.h        — Vendor register-map tables, register semantic types
├── main.c             — Core 0: PIO/DMA init, bus attach, real-time loop
├── core1.c            — Core 1: decode pipeline, event reconstruction
├── bus_tap.pio        — PIO programs: I²C through + tap, SPI through + tap
├── injection.c        — Injection engine, PIO injection SM, command queue
├── protocol.c         — I²C/SPI frame parser, auto-detect, chip ID
├── touch_decode.c     — Vendor decoders, touch event extraction
├── layout_infer.c     — Screen layout model, keystroke/pattern reconstruction
├── storage.c          — SD card logging, circular buffer, file rotation
├── ble_link.c         — UART protocol to ESP32-C3, command dispatch
└── esp32/
    ├── main.c         — ESP32-C3: BLE GATT server, WiFi offload, OTA
    └── ble_service.c  — BLE characteristics, notification streaming
```

### Key firmware modules (summary)

- **main.c (core 0):** Initializes PIO through-path and tap SMs, configures DMA channels with ring buffers, runs the real-time bus-attach state machine, handles target power detection, and arbitrates injection bus access. ~250 lines.
- **core1.c:** Runs the decode pipeline stages 1–5, manages the event queue, and handles SD/BLE output. ~200 lines.
- **protocol.c:** I²C/SPI framing, auto-detect (samples first 100 ms of traffic to determine bus type, speed, address, chip ID), and dispatches to vendor decoders. ~180 lines.
- **touch_decode.c:** Per-vendor touch register decoders. Extracts coordinates, pressure, gesture from register reads. ~200 lines.
- **injection.c:** Synthesizes and queues injection transactions; schedules them in the host inter-poll gap. ~170 lines.
- **layout_infer.c:** Maps coordinates to UI elements (keyboard, dialer, pattern grid) using a layout model received from the app. Reconstructs typed text and unlock patterns. ~160 lines.
- **storage.c:** FATFS-based SD logging with 10-minute file rotation. ~120 lines.
- **ble_link.c:** UART framing protocol to ESP32-C3; command queue and event stream. ~100 lines.

---

## Companion App

A React Native app (iOS/Android) provides the operator interface over BLE. Screens include:

| Screen | Function |
|---|---|
| **Connect** | Scan for Tactile-Phantom, BLE pairing (passkey OOB), connection status, signal strength |
| **Live Capture** | Real-time touch-coordinate visualization (animated dot on a phone-shaped canvas), event log with timestamps, gesture recognition display |
| **Event Log** | Searchable/filterable log of all captured events: taps, swipes, gestures, reconstructed keystrokes, unlock patterns |
| **Pattern Reconstruct**** | Visual replay of captured unlock patterns (Android 3×3 grid), PIN sequences, and soft-keyboard text |
| **Injection Console**** | Send tap/swipe/long-press/type commands; coordinate picker on a target screen image; pattern replay from capture |
| **Layout Manager** | Configure target screen layout (resolution, keyboard layout, app icon positions) for accurate coordinate-to-keystroke mapping |
| **Storage** | Browse SD card logs, download over BLE or WiFi, export as JSON/CSV |
| **Settings** | Bus configuration (manual override of auto-detect), injection arming, firmware update, battery/status |

---

## Use Cases

### For red teams

1. **Remote UI control of compromised POS/kiosk:** After physical implant, operator remotely navigates the target's UI via BLE to authorize voided transactions, change admin settings, or disable security modes — all appearing as legitimate touch interaction in the OS logs.

2. **Unlock pattern capture & replay:** Tap the bus during a legitimate user unlock, capture the pattern, then replay it later via injection (or type it manually) to unlock the device — works even when the OS has on-screen randomization for PIN keypads, because the bus sees pre-randomization coordinates.

3. **Automotive infotainment manipulation:** Implant behind the head-unit display; capture phone-pairing PIN entry, navigation destination input; inject screen interactions while parked to pair a rogue phone or alter settings.

### For security researchers

4. **Touch-controller firmware analysis:** Dump the TSC firmware during host-initiated updates; reverse engineer proprietary touch algorithms, sensitivity configs, and hidden diagnostic modes.

5. **OS input-subsystem trust modeling:** Demonstrate that OS-level secure-input guarantees (anti-keylogger, encrypted input path) are bypassed by bus-level interception — informs hardware security architecture.

6. **TSC vulnerability research:** Fuzz the touch-controller by injecting malformed register states to find parsing bugs, buffer overflows in the TSC firmware, or diagnostic-mode entry via config-register manipulation.

### For penetration testers

7. **Smart-lock keypad capture:** Many smart locks use capacitive keypads with an I²C touch matrix. Capture entry codes; replay or inject to unlock.

8. **Industrial HMI credential capture:** Tap HMI touch input to capture operator credentials, recipe parameters, and safety-override sequences.

9. **Supply-chain assessment:** Demonstrate that a refurbished device with a pre-installed bus tap can capture all user input — assesses refurbishment-chain trust.

---

## Comparison to existing tools

| Tool | Layer | What it sees | Limitation |
|---|---|---|---|
| Software keylogger | OS input API | HID keyboard/mouse, software touch events | Easily detected; cannot see pre-OS input; no injection |
| USB HID BadUSB | USB layer | Injects keystrokes | Cannot capture; cannot inject touch; OS-visible |
| HDMI-Siphon | Display output | Screen content (output only) | Cannot see input; no injection |
| USB-DMA-Phantom | Thunderbolt/USB4 PCIe | Host memory | High privilege but doesn't tap input peripherals |
| **Tactile-Phantom** | **Touch-controller I²C/SPI bus** | **Raw touch input + config + firmware; injects touch** | **Requires physical access to TSC bus** |

Tactile-Phantom is complementary: it is the only tool that operates at the touch-controller bus layer, seeing input before any other tool or OS layer, and uniquely capable of injecting touch events that are indistinguishable from genuine user interaction.

---

## Limitations & countermeasures

**Limitations:**
- Requires physical access to the TSC FPC connector or test pads (device-dependent difficulty).
- Vendor register maps must be known for full decode; unknown controllers fall back to raw transaction logging.
- Injection timing is limited to host inter-poll windows; very high polling rates reduce injection bandwidth.
- BLE range limits operational distance to ~10 m.

**Defensive countermeasures (for blue teams):**
- Encrypted/authenticated TSC-to-AP bus (e.g., I²C with HMAC on touch data) — defeats both capture and injection.
- Secure-boot verified TSC firmware that refuses to operate if bus tap is detected (unexpected bus capacitance/timing).
- TSC-side touch liveness detection (capacitive signature of real finger vs injected register write — requires TSC firmware support).
- Physical tamper-evident seals on device enclosures to detect bus-tap implantation.

---

## File manifest

```
tactile-phantom/
├── README.md                    — This document
├── firmware/
│   ├── Makefile                 — Pico SDK build (RP2040) + cross-compile
│   ├── board.h                  — Pin map, bus constants, config
│   ├── registers.h              — Vendor TSC register maps
│   ├── main.c                   — Core 0: PIO/DMA, bus attach, real-time
│   ├── core1.c                  — Core 1: decode pipeline
│   ├── injection.c              — Touch event injection engine
│   ├── protocol.c               — I²C/SPI framing, auto-detect, chip ID
│   ├── touch_decode.c           — Vendor-specific touch decoders
│   ├── layout_infer.c           — Keystroke/pattern reconstruction
│   ├── storage.c                — SD card logging
│   └── ble_link.c               — UART link to ESP32-C3
├── kicad/
│   ├── device.kicad_sch         — Schematic
│   ├── device.kicad_pcb         — PCB layout
│   └── device.kicad_pro         — KiCad project
└── app/
    ├── App.tsx                  — React Native entry
    ├── package.json             — Dependencies
    ├── src/
    │   ├── ble.ts               — BLE manager, GATT characteristics
    │   ├── proto.ts             — Binary protocol codec
    │   └── store.ts             — App state (Zustand)
    └── screens/
        ├── ConnectScreen.tsx
        ├── LiveCaptureScreen.tsx
        ├── EventLogScreen.tsx
        ├── PatternReconstructScreen.tsx
        ├── InjectionConsoleScreen.tsx
        ├── LayoutManagerScreen.tsx
        ├── StorageScreen.tsx
        └── SettingsScreen.tsx
```

---

## License

GPL-2.0. All hardware design files (KiCad), firmware, and app source are open source. Author: **jayis1**.

---

*Designed for authorized security research only. Know your laws. Get written consent. — jayis1*