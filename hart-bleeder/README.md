# HART-Bleeder — HART Fieldbus Covert In-Line Attack Implant

```
   ╔═══════════════════════════════════════════════════════════════════════╗
   ║   ██╗  ██╗██████╗ ███████╗████████╗   ██████╗ ██╗  ██╗███████╗██████╗  ║
   ║   ██║  ██║██╔══██╗██╔════╝╚══██╔══╝   ██╔══██╗██║  ██║██╔════╝██╔══██╗ ║
   ║   ███████║██████╔╝███████╗   ██║      ██████╔╝███████║█████╗  ██████╔╝ ║
   ║   ██╔══██║██╔══██╗╚════██║   ██║      ██╔══██╗██╔══██║██╔══╝  ██╔══██╗ ║
   ║   ██║  ██║██║  ██║███████║   ██║      ██║  ██║██║  ██║███████╗██║  ██║ ║
   ║   ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝   ╚═╝      ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝ ║
   ║         HART Fieldbus Covert In-Line Attack Implant                     ║
   ╚═══════════════════════════════════════════════════════════════════════╝
```

**Author:** jayis1  
**SPDX-License-Identifier:** GPL-2.0  
**Version:** 1.0.0  
**Target Hardware:** STM32G474CEU6 + MAX13440E HART modem + nRF52832 BLE

---

## ⚠ Legal & Ethical Disclaimer

HART-Bleeder is a professional security research instrument designed **exclusively** for
authorized penetration testing, red team engagements, and defensive research on
industrial control systems (ICS), SCADA networks, and process control field
instrumentation that you own or have **explicit written authorization** to test.

Misuse — including attacking process control loops, field instruments, valves,
transmitters, or any industrial equipment you do not own or have permission to
evaluate — may violate computer fraud and abuse statutes (e.g., 18 U.S.C. § 1030
CFAA), critical infrastructure protection laws (e.g., the U.S. Critical
Infrastructure Protection Act, NERC CIP standards), industrial safety
regulations, and physical-tampering statutes in your jurisdiction. HART
fieldbus operates on live 4–20 mA current loops that may control chemical
processes, pressure valves, actuators, and safety instrumented systems (SIS);
deliberate manipulation of these loops can cause process upsets, equipment
damage, environmental releases, injury, or death. The author (**jayis1**) and
contributors disclaim all liability for damage, injury, or legal consequences
arising from use of this design. **Use only on equipment you own or are
authorized to test, and only with appropriate industrial safety precautions
including loop isolation, lockout/tagout, and safety observer presence.**

---

## 1. Purpose & Overview

HART-Bleeder is a pocket-sized, covert, in-line attack device targeting the
**HART (Highway Addressable Remote Transducer) protocol** — the dominant digital
communication overlay on 4–20 mA analog current loops used across industrial
process control, SCADA, and operational technology (OT) environments
worldwide. HART is deployed on virtually every modern pressure, temperature,
flow, and level transmitter, as well as on smart control valves and final
control elements in oil & gas, chemical, pharmaceutical, water/wastewater, and
power generation facilities.

The key insight that makes HART-Bleeder genuinely novel for red teamers: **the
HART fieldbus is the most ubiquitous yet least-examined attack surface in
industrial control.** Every process plant runs thousands of 4–20 mA loops,
and the HART digital overlay on those loops carries device configuration,
calibration, diagnostics, and — critically — **process setpoint commands**.
Despite this, HART is almost never treated as a trust boundary. Plant security
teams focus on the Ethernet/IP network and ignore the 2-wire fieldbus; OT
engineers treat HART as a maintenance/diagnostic channel, not a control
channel. HART-Bleeder exploits this blind spot.

The device inserts itself **in-line on a 4–20 mA current loop** between a
field instrument (transmitter or valve) and the control system (DCS, PLC, or
loop controller). Physically it appears as a small inline coupler; electrically
it is a **transparent man-in-the-middle** with the ability to:

- **Passively sniff** all HART traffic between the control system and field devices
- **Enumerate** every device on the loop (Command 0 sweep, poll addresses 0–63)
- **Inject spoofed HART frames** — issue arbitrary commands to field devices
- **Clamp the loop current** to an arbitrary value (physical-layer PV spoofing)
- **Write process setpoints** (Command 35) to control valves and final elements
- **Deny service** by opening the loop (breaking continuity) or inducing voltage sags
- **Exfiltrate data covertly** via a ±0.5 mA FSK covert channel on the loop itself
- **Fuzz** the HART parser of field devices with malformed frames to probe robustness
- **Replay** captured HART frames
- **Burst-capture** all frames for offline analysis and forensic reconstruction

All captured traffic, attack logs, and device profiles are stored on an
on-board microSD card and 16 MB QSPI flash, and can be streamed over an
**encrypted BLE C2 link** (XTEA-128 CBC) to an operator's mobile device running
the companion React Native app.

### 1.1 Why HART-Bleeder?

Modern ICS/SCADA red teams have excellent tools for the Ethernet/IP OT network:
industrial protocol analyzers, Modbus/TCP fuzzers, PLC exploitation frameworks.
But the **field level** — the 2-wire 4–20 mA loops that actually measure and
control the physical process — has been a blind spot. Existing HART tools
(USB-HART modems, Field Communicators like the Emerson 475) are designed for
maintenance engineers, not attackers: they are bulky, expensive ($2000–$5000),
require a display and keypad interaction, take 30+ seconds per command, and
are conspicuous. None of them can clamp loop current, induce sags, open the
loop, run covert channels, or operate covertly inline.

HART-Bleeder fills this gap with a $50 BOM, key-fob-sized device that:

- **Demonstrates the exposure** of HART fieldbus as an attack surface in OT assessments
- **Tests field device security** — parser robustness, write-protection, authentication
- **Assesses SIS isolation** — verifies safety loops are not reachable via HART
- **Enables covert exfiltration** from air-gapped process networks via the loop itself
- **Provides forensic capture** of process control traffic for incident response
- **Simulates field instrument compromise** for red-team / purple-team exercises

### 1.2 Key Capabilities

| Capability | Description |
|---|---|
| **Transparent In-Line MITM** | Inserts in series with a 4–20 mA loop; < 2 Ω insertion impedance in bypass mode |
| **HART Frame Sniffer** | Decodes all HART short & long frames (Command + Response) at 1200 baud FSK |
| **Bus Enumeration** | Command 0 sweep across poll addresses 0–63; extracts manufacturer, device type, 38-bit unique ID |
| **Command Injection** | Issue arbitrary HART commands to any addressed device; full command set 0–255 |
| **PV Spoofing (Digital)** | Send false Primary Variable readings via HART Command 1 responses |
| **PV Spoofing (Analog)** | Clamp the 4–20 mA loop current to any value 3.8–22 mA via DAC-driven resistance modulation |
| **Setpoint Writes** | Write process setpoints (Command 35) to control valves and final control elements |
| **Voltage Sag Injection** | Induce programmable loop voltage sags to crash/reset field devices |
| **Open-Loop DoS** | Break loop continuity via relay to deny all analog + digital control |
| **Covert Channel (Exfil)** | ±0.5 mA FSK modulation on the loop: ~1200 bit/s covert data channel |
| **Covert Channel (Receive)** | Demodulate ±0.5 mA FSK from a second HART-Bleeder on the same loop |
| **Frame Fuzzing** | Send malformed HART frames to probe field device parser robustness |
| **Frame Replay** | Replay previously captured HART frames byte-for-byte |
| **Burst Capture** | Record all HART frames for a duration to microSD + QSPI for offline analysis |
| **Encrypted BLE C2** | XTEA-128 CBC encrypted command/control over nRF52832 BLE to operator app |
| **USB-CDC Console** | Full serial console with command-line interface for direct operator control |
| **OLED Status Display** | 128×64 SSD1306 showing mode, battery, device count, loop current |
| **Battery Powered** | LiPo + USB-C charging; harvests no power from the loop (transparent) |
| **microSD Logging** | All frames, attack logs, and device profiles persisted to microSD |
| **Covert Form Factor** | 45 × 25 mm inline coupler; no LEDs in stealth mode |

---

## 2. Attack Surface & Threat Model

### 2.1 Threat Model

HART-Bleeder assumes the following threat scenario:

- **Attacker has physical access** to a 4–20 mA current loop — typically at a
  field junction box, marshalling cabinet, terminal block, or by clipping onto
  the loop wires at a transmitter/valve
- **Attacker can insert the device in-line** in under 10 seconds (inline
  coupler form factor, screw terminals)
- **The loop may be live** — carrying 4–20 mA at up to 42 V DC loop supply
- **The control system may be monitoring** the loop current and HART traffic
- **Field devices may or may not have write-protection** enabled (many do not)
- **The device must be electrically transparent** when passive — no detectable
  change to loop current, voltage, or HART bit error rate

### 2.2 Attack Surface Vectors

**Vector 1 — Passive Traffic Analysis (Reconnaissance)**
By inserting transparently and sniffing all HART frames, the attacker learns:
- Which devices are on the loop (manufacturers, device types, unique IDs)
- What commands the DCS/PLC issues and at what frequency
- Process variable values (temperatures, pressures, flows, levels)
- Setpoint values and tuning parameters
- Device configuration (tags, descriptors, calibration, ranges)

This alone constitutes a significant information disclosure — process recipes,
production rates, and equipment identities are often confidential.

**Vector 2 — Command Injection (Manipulation)**
The attacker issues arbitrary HART commands to field devices. Critical commands:
- **Command 35 (Write PV / Setpoint):** On final control elements (valves),
  this directly sets the process variable. A spoofed setpoint can open/close
  valves, alter flow rates, or drive processes to unsafe states.
- **Command 17 (Write Tag):** Overwrites device identification, enabling
  confusion or misdirection during incident response.
- **Command 8 (Write Loop Mode):** Can disable analog output, forcing the loop
  to a fixed current and hiding the true process value from the DCS.

**Vector 3 — Analog PV Spoofing (Physical-Layer Deception)**
By clamping the loop current via DAC-driven resistance modulation, the attacker
can make the DCS read any desired process value (e.g., "pressure is normal"
when it is not). This defeats HART-level monitoring because the analog signal
itself is forged — the DCS sees a plausible 4–20 mA signal that does not
reflect reality. This is the most dangerous attack: it can mask an over-pressure
condition from a safety system while the attacker manipulates the process.

**Vector 4 — Denial of Service**
Two DoS primitives:
- **Open-loop:** Breaking loop continuity causes the DCS to read 0 mA, tripping
  alarms and potentially initiating emergency shutdown. In a SIS loop, this can
  trigger a spurious trip.
- **Voltage sag:** Inducing a brief loop voltage drop can crash or reset field
  devices, causing loss of measurement or control for the sag duration.

**Vector 5 — Covert Channel (Air-Gap Bridging)**
The ±0.5 mA FSK covert channel allows two HART-Bleeders (or a HART-Bleeder and
a compromised field device) to communicate **over the current loop itself**,
bridging air gaps. Data modulated onto the loop can be received by a second
device elsewhere on the same loop — potentially crossing a security zone
boundary (e.g., from a process network to a corporate network segment that
shares a loop for signal conditioning).

**Vector 6 — Fuzzing (Robustness Testing)**
Malformed HART frames probe the parser of field devices for vulnerabilities:
buffer overflows, state machine confusion, command injection via byte-count
mismatches, and crashes. Field devices often run legacy firmware with minimal
input validation; fuzzing can reveal exploitable bugs.

### 2.3 What HART-Bleeder Does NOT Do

- It does **not** attack the analog 4–20 mA signal without being inline (it
  requires series insertion, not just proximity)
- It does **not** bypass HART write-protection if the field device has it
  enabled and enforces it correctly (it will detect and report MODE_REJECT)
- It does **not** survive loop isolation / galvanic isolation without the
  optional isolation transformer bypass engaged
- It does **not** harvest power from the loop — it runs on its own battery to
  remain electrically transparent

---

## 3. Hardware Specifications

### 3.1 MCU — STM32G474CEU6

| Parameter | Value |
|---|---|
| Core | ARM Cortex-M4F @ 170 MHz |
| Flash | 512 KB |
| SRAM | 128 KB |
| FPU | Single-precision (FPv4-SP) |
| Math Accelerators | CORDIC + FMAC (Filter Math Accelerator) |
| ADC | 12-bit, 5 MSPS, up to 43 channels |
| DAC | 2-channel 12-bit |
| UART | 6 × (USART1/2/3, UART4/5, LPUART1) |
| SPI | 3 × (SPI1/2/3) |
| I2C | 4 × |
| QSPI | 1 × (memory-mapped, for external flash) |
| USB | USB 2.0 FS (internal PHY) |
| Package | QFN-48 (7×7 mm) |

The STM32G474 is chosen for its high clock speed (fast enough for real-time
HART frame processing + covert channel demodulation), rich analog peripherals
(ADC + DAC for loop sensing and modulation), CORDIC/FMAC accelerators (useful
for any future DSP-based signal analysis), and low cost (~$4).

### 3.2 HART Modem — MAX13440EETD+

| Parameter | Value |
|---|---|
| Modulation | Bell 202 FSK (1200 Hz mark / 2200 Hz space) |
| Baud Rate | 1200 baud |
| Duplex | Half-duplex |
| Interface | UART 1200 baud, 8-O-1 |
| Control | /RTS (TX enable), /CD (carrier detect), /SHDN |
| Supply | 3.3 V |
| Package | TSSOP-14 |

The MAX13440E is a dedicated HART modem IC that handles the FSK modulation/
demodulation, bandpass filtering, and carrier detection. It interfaces to the
MCU via a standard 1200 baud UART with odd parity (the HART standard). The MCU
gates transmission via /RTS and reads carrier detect via /CD. An alternative
modem (TH8032 or AD5700-1) is footprint-compatible.

### 3.3 Current-Loop Interface

| Component | Purpose | Part |
|---|---|---|
| **Sense Resistor** | 250 Ω 0.1% precision — converts 4–20 mA to 1–5 V for ADC | Susumu RG2012P-2500-B-T5 |
| **Bypass Relay** | Shorts the sense resistor in passive mode (zero insertion impedance) | Omron G6K-2F-Y-DC3 |
| **Digital Pot** | MCP4131-503E — DAC-controlled wiper adds/subtracts loop resistance for current clamping | MCP4131-503E/MS |
| **Isolation Transformer** | Optional galvanic isolation for FSK coupling to high-voltage loops | Pulse HX1188NL |
| **Coupling Cap** | 0.22 µF 630V C0G — AC-couples HART FSK onto the DC loop | C1812C0G series |
| **Loop Terminals** | Screw terminals for inline insertion into 4–20 mA loop | 2-pin 5.08mm pitch |

### 3.4 Connectivity & Storage

| Function | Part | Interface |
|---|---|---|
| BLE C2 | Nordic nRF52832-QFAA | UART4 @ 115200 |
| microSD | MicroSDHC socket (Hirose DM3AT) | SPI1 @ 25 MHz |
| External Flash | Winbond W25Q128JVSIQ (16 MB) | QSPI memory-mapped |
| OLED | SSD1306 128×64 I2C | I2C2 (bit-banged) |
| USB-CDC | USB-C 2.0 receptacle + USBLC6-2SC6 ESD | USB FS internal PHY |
| Console | ST-Link VCP via USB-C | USART1 @ 115200 |

### 3.5 Power

| Parameter | Value |
|---|---|
| Battery | 500 mAh LiPo (single cell) |
| Charger | MCP73831T-2ACI/OT (USB-C charging, 500 mA) |
| Regulator | TPS63020DSJR buck-boost (3.3 V rail, 2 A) |
| Battery Life | ~8 h passive monitor, ~4 h active transmit |
| Loop Power | **None** — device is battery-powered, does not load the loop |
| Charging | USB-C 5V, ~2 h full charge |

### 3.6 Form Factor

| Parameter | Value |
|---|---|
| Dimensions | 45 × 25 × 8 mm (inline coupler) |
| PCB | 4-layer, 1.6 mm, ENIG finish |
| Weight | ~15 g (with battery) |
| Enclosure | Heat-shrink tubing (covert) or 3D-printed ABS sleeve |
| Connectors | 2× screw terminals (loop in/out), USB-C, microSD slot |

---

## 4. Architecture & Block Diagram

```
 ┌──────────────────────────────────────────────────────────────────────┐
 │                        HART-BLEEDER BLOCK DIAGRAM                    │
 │                         Author: jayis1 · GPL-2.0                     │
 └──────────────────────────────────────────────────────────────────────┘

  LOOP IN ─┬──[Bypass Relay K1]──┬──[Sense R5 250Ω]──┬── LOOP OUT
           │                      │                    │
           │                      │                    │
           ▼                      ▼                    ▼
     ┌──────────┐          ┌──────────┐         ┌──────────┐
     │Isolation │          │ ADC1 PA0 │         │ DAC1 PA4 │
     │Transformer│         │ (I_sense)│         │  (clamp) │
     │  T1      │          └────┬─────┘         └────┬─────┘
     └─────┬────┘               │                     │
           │                    │                     │
           │              ┌─────▼─────┐         ┌─────▼─────┐
           │              │ ADC1 PA1  │         │ Digital Pot│
           │              │ (V_sense) │         │ MCP4131   │
           │              └─────┬─────┘         └───────────┘
           │                    │
     ┌─────▼────┐         ┌─────▼──────────────────────────────┐
     │Coupling C│         │          STM32G474CEU6              │
     │ 0.22µF   │         │  (Cortex-M4F @ 170 MHz)            │
     └─────┬────┘         │                                      │
           │              │  ┌──────────┐  ┌──────────┐         │
           ▼              │  │ HART Stack│  │Attack Ops│         │
     ┌──────────┐  UART2  │  │ (framing, │  │ (spoof,  │         │
     │MAX13440E │◄───────►│  │ checksum, │  │ DoS, fuz,│         │
     │ HART Modem│ /RTS   │  │ poll, cmd)│  │ covert)  │         │
     │  U2      │  /CD    │  └──────────┘  └──────────┘         │
     └──────────┘  /SHDN  │                                      │
                          │  ┌──────────┐  ┌──────────┐         │
                          │  │ Console  │  │ BLE C2   │         │
                          │  │ (USB-CDC)│  │ (XTEA)   │         │
                          │  └────┬─────┘  └────┬─────┘         │
                          └───────┼────────────┼───────────────┘
                                  │            │ UART4
                          ┌───────▼──┐   ┌─────▼─────┐
                          │ USART1   │   │ nRF52832  │
                          │ USB-CDC  │   │ BLE Module│──► Operator App
                          └──────────┘   └───────────┘
                          ┌──────────┐   ┌───────────┐
                          │  microSD │   │ W25Q128   │
                          │  (SPI1)  │   │ QSPI Flash│
                          └──────────┘   └───────────┘
                          ┌──────────┐   ┌───────────┐
                          │ SSD1306  │   │  LiPo +   │
                          │  OLED    │   │ TPS63020  │
                          └──────────┘   └───────────┘
```

### 4.1 Signal Flow — Passive Monitor

1. Loop current flows through the sense resistor (R5, 250 Ω) → 1–5 V signal
2. ADC samples loop current (PA0) and voltage (PA1) at 1 kHz
3. HART modem (U2) demodulates FSK from the AC-coupled loop signal
4. MCU USART2 receives demodulated HART bytes at 1200 baud
5. HART stack parses frames, logs to microSD, pushes to BLE C2

### 4.2 Signal Flow — Command Injection

1. Operator issues command via BLE app or USB console
2. Attack module builds HART frame (checksum, preamble, addressing)
3. MCU USART2 sends bytes to modem at 1200 baud
4. /RTS asserted → modem modulates FSK onto loop via coupling cap
5. Target field device receives and responds; response captured by modem

### 4.3 Signal Flow — Analog PV Spoofing

1. Operator sets target current (e.g., 12 mA)
2. DAC1 outputs a voltage proportional to the target
3. Digital pot (MCP4131) wiper adjusts series resistance
4. Total loop resistance changes → loop current shifts to target
5. Bypass relay opens (removes the sense resistor short) to allow modulation
6. DCS reads the spoofed 4–20 mA value, unaware of manipulation

---

## 5. Firmware Details & Design Decisions

### 5.1 Firmware Architecture

The firmware is written in C11 and targets bare-metal STM32G474 with a custom
startup and linker script (no HAL/LL dependencies, no RTOS). It is organized
into the following modules:

| File | Lines | Responsibility |
|---|---|---|
| `main.c` | ~250 | State machine, main loop, task scheduling |
| `board_init.c` | ~230 | Clock tree, GPIO, ADC, DAC, SysTick, EXTI |
| `hart_stack.c` | ~300 | HART framing, checksum, parsing, transactions, enumeration |
| `modem_drv.c` | ~170 | MAX13440E UART driver, RTS/CD control, timing |
| `loop_drv.c` | ~230 | ADC sampling, DAC clamping, sag injection, covert modulation |
| `attacks.c` | ~290 | High-level attack operations (enumeration, spoof, DoS, fuzz, covert) |
| `console.c` | ~320 | USB-CDC CLI, command registration, dispatch, printf |
| `c2link.c` | ~310 | BLE UART protocol, XTEA-128 CBC, command dispatch |
| `storage.c` | ~280 | microSD SPI driver, QSPI flash, profile persistence |
| `oled_drv.c` | ~220 | SSD1306 I2C bit-bang driver, font, status display |
| `registers.h` | ~280 | STM32G474 register definitions and helpers |
| `board.h` | ~210 | Pin map, constants, electrical limits |
| `hart_stack.h` | ~190 | HART protocol definitions, frame structures, API |
| **Total** | **~3500** | |

### 5.2 State Machine

```
  BOOT → IDLE → (short press) → PASSIVE_MONITOR → IDLE
              → (long press)  → ARMED → IDLE
              → (cmd)         → ACTIVE → IDLE
              → (loop fault)  → FAULT → IDLE (auto-recover)
```

- **IDLE:** Device is connected but not monitoring. Loop is in passive mode
  (bypass relay closed, zero insertion impedance). LEDs off.
- **PASSIVE_MONITOR:** Continuously sniffs HART frames, logs to SD, pushes to
  BLE. Blue LED on. No transmission.
- **ARMED:** Enables destructive operations (inject, spoof, DoS, fuzz). Red
  LED on. Requires long-press of user button or explicit `mode` command.
- **ACTIVE:** An attack operation is in progress. Red LED on.
- **FAULT:** Loop electrical fault detected (under/over current, over voltage).
  All operations suspended. Auto-recovers when fault clears.

### 5.3 Design Decisions

**Why bare-metal, no RTOS?** The HART stack is inherently single-threaded
(half-duplex, low baud, slow transactions). An RTOS adds complexity and RAM
overhead with no benefit. The main loop + WFI is simpler and more predictable.

**Why a dedicated HART modem IC rather than software FSK?** The MAX13440E
handles Bell 202 FSK modulation/demodulation with built-in bandpass filtering
and carrier detection in hardware. Software FSK on a 170 MHz M4 is possible but
requires precise timing, continuous DMA, and significant CPU — the modem IC
offloads this for $2 and frees the MCU for protocol logic and attack operations.

**Why 8-O-1 (odd parity) on the modem UART?** The HART physical layer
specification (HCF_SPEC-127) mandates 8 data bits, 1 stop bit, odd parity at
1200 baud. This provides a basic integrity check on each byte.

**Why XTEA-128 for BLE C2 encryption?** XTEA is a compact, public-domain block
cipher that fits in ~30 lines of C and runs in software on a Cortex-M4 with no
hardware crypto. AES would require either a hardware accelerator (not all G474
variants have it) or a larger software implementation. For a short-range BLE
C2 link, XTEA-128 in CBC mode provides adequate confidentiality. The PSK is
provisioned via the app and never stored in plaintext.

**Why a 250 Ω sense resistor?** This is the HART standard value: 250 Ω ×
4–20 mA = 1–5 V, which conveniently matches the HART FSK signal amplitude
(±0.5 mA × 250 Ω = ±125 mV). It also fits within the ADC's 0–3.3 V range via
a /2 divider (0.5–2.5 V at the ADC pin).

**Why battery-powered, not loop-powered?** A loop-powered device would draw
current from the 4–20 mA loop, altering the process variable reading and
making the device detectable. By running on its own battery, HART-Bleeder
remains electrically invisible — the loop current and voltage are unaffected
by its presence (in bypass mode, insertion impedance < 2 Ω).

**Why a bypass relay?** In passive monitor mode, the sense resistor adds 250 Ω
to the loop, which changes the current-to-voltage relationship the DCS expects.
The bypass relay shorts the sense resistor so the device is electrically
transparent. The relay opens only when the operator actively clamps or modulates
the loop current.

### 5.4 Build & Flash

```bash
cd firmware
make            # builds hart-bleeder.elf, .bin, .hex
make flash      # flashes via OpenOCD + ST-Link
make size       # prints memory usage
make dump       # disassembly to .lst
```

Toolchain: `arm-none-eabi-gcc` (GCC 12+), OpenOCD, ST-Link V3.

---

## 6. Application / Software Interface

### 6.1 Companion App (React Native)

The companion app (`app/`) is a React Native application providing a mobile
operator interface over encrypted BLE. Screens:

| Screen | Function |
|---|---|
| **Dashboard** | Connection status, device state, battery, loop current, arm/disarm |
| **DeviceScan** | HART bus enumeration (Command 0 sweep), device list with mfr/type/ID |
| **LiveCapture** | Real-time captured HART frame feed with decode and hex dump |
| **LoopControl** | Mode selection, loop current readout, current clamp slider, sag injection |
| **AttackConsole** | Read PV, write setpoint, spoof, DoS, fuzz, covert exfil/receive |
| **SessionLog** | Encrypted log viewer with severity filters and share/export |
| **Settings** | BLE PSK authentication, inter-frame timing, master role, device info |

The app uses `@react-native-ble-plx/react-native-ble-plx` for BLE GATT
communication. The BLE protocol uses a custom framing layer
(`[0xAA][0x55][seq][len][op][payload][crc8]`) over GATT write-characteristic
commands and notification-characteristic responses (status, frames, logs).

### 6.2 USB-CDC Console

For direct operator control without a phone, the USB-CDC console provides a
full command-line interface. Connect via USB-C (ST-Link VCP or USB FS) at
115200 baud:

```
> help
Commands:
  help       show commands
  status     loop + stack status
  scan       enumerate HART bus
  readpv     read PV <addr>
  spoof      clamp loop <mA>
  setpt      write setpoint <a> <%>
  dos        open-loop DoS <ms>
  sag        voltage sag <ms> <%>
  capture    burst capture <ms>
  fuzz       fuzz <addr> <count>
  covert     covert exfil <msg>
  passive    return to passive
  mode       set loop mode <n>
  reboot     reset MCU
```

Example session:
```
> scan
Enumerating HART bus (cmd 0 sweep)...
Found 3 HART device(s):
  [0] addr=0 mfr=0x27 type=0x40 id=00112233AABB
  [1] addr=15 mfr=0x1B type=0x02 id=0123456789AB
  [2] addr=23 mfr=0x27 type=0x40 id=FFAABBCCDDEE
> readpv 0
PV=72.40%  I=15.584 mA
> spoof 12.0
spoofing loop current to 12.000 mA
> capture 5000
captured 12 frames
```

---

## 7. Use Cases

### 7.1 Red Team: OT/ICS Process Control Assessment

During a physical red team engagement at a chemical plant, the operator inserts
HART-Bleeder inline at a field junction box on a reactor temperature loop.
Passive monitoring reveals the DCS polls the transmitter every 2 seconds with
Command 1. The operator then writes a false setpoint (Command 35) to a
downstream control valve, demonstrating that the HART bus has no
authentication. Finding: the plant's SIS loops share the same HART
infrastructure as the BPCS, violating IEC 61511 separation requirements.

### 7.2 Security Research: Field Device Parser Fuzzing

A researcher uses the `fuzz` command to send 500 malformed HART frames to a
vintage pressure transmitter. Several frames with truncated byte counts cause
the device to stop responding for 30 seconds, indicating a watchdog timeout.
Further analysis reveals a buffer overflow in the device's HART command
dispatcher that could enable code execution on the transmitter's MCU.

### 7.3 Penetration Test: Air-Gap Bridging via Current Loop

An air-gapped process control network is physically isolated from the
corporate network. However, a 4–20 mA loop from a process transmitter runs
through a shared marshalling cabinet that also serves a corporate-network
data acquisition module. The operator places one HART-Bleeder on the process
side and another on the corporate side. Data exfiltrated from the process
network modulates the loop current by ±0.5 mA (within the HART noise floor),
crossing the air gap via the shared physical loop.

### 7.4 Defensive Research: HART Write-Protection Validation

A security team uses HART-Bleeder to validate that field device write-protection
is correctly enforced. The device attempts Command 35 (write setpoint) on each
transmitter; devices that accept the write without a MODE_REJECT status are
flagged for remediation. This provides an automated, auditable test of a
critical OT security control.

### 7.5 Incident Response: Forensic Loop Capture

During an incident involving unexplained process upsets, an investigator places
HART-Bleeder inline on the suspected loop and runs a 24-hour burst capture. The
captured HART frames reveal an unauthorized Command 35 setpoint write
originating from an unexpected poll address, indicating a rogue master device
on the loop. Frame timestamps correlate the writes with the process upsets.

---

## 8. Repository Structure

```
hart-bleeder/
├── README.md                  (this file)
├── firmware/
│   ├── Makefile               build system
│   ├── linker.ld              STM32G474 linker script
│   ├── startup_stm32g474.s    Cortex-M4 startup + vector table
│   ├── board.h                pin map, constants, electrical limits
│   ├── registers.h            STM32G474 register definitions
│   ├── main.c                 state machine + main loop
│   ├── board_init.c           clock, GPIO, ADC, DAC, timers
│   ├── hart_stack.h           HART protocol definitions + API
│   ├── hart_stack.c           framing, checksum, parsing, enumeration
│   ├── modem_drv.h            HART modem driver interface
│   ├── modem_drv.c            MAX13440E UART + RTS/CD driver
│   ├── loop_drv.h             current-loop interface API
│   ├── loop_drv.c             ADC sampling, DAC clamping, covert mod
│   ├── attacks.h              attack operations API
│   ├── attacks.c              enumeration, spoof, DoS, fuzz, covert
│   ├── console.h              console interface
│   ├── console.c              USB-CDC CLI + command dispatch
│   ├── c2link.h               BLE C2 link definitions
│   ├── c2link.c               XTEA-128 CBC + BLE protocol
│   ├── storage.h              storage interface
│   ├── storage.c              microSD SPI + QSPI flash + profiles
│   ├── oled_drv.h             OLED driver interface
│   └── oled_drv.c             SSD1306 I2C bit-bang + font
├── kicad/
│   ├── device.kicad_sch       schematic (11 sheets, full netlist)
│   ├── device.kicad_pcb       PCB layout (45×25 mm, 4-layer)
│   └── device.kicad_pro       KiCad project file
└── app/
    ├── App.js                 React Native entry + navigation
    ├── package.json           dependencies
    ├── components/
    │   └── BLEManager.js       BLE GATT client + C2 protocol
    └── screens/
        ├── DashboardScreen.js
        ├── DeviceScanScreen.js
        ├── LiveCaptureScreen.js
        ├── LoopControlScreen.js
        ├── AttackConsoleScreen.js
        ├── SessionLogScreen.js
        └── SettingsScreen.js
```

---

## 9. Bill of Materials (Estimate)

| Part | Qty | Unit Price | Source |
|---|---|---|---|
| STM32G474CEU6 | 1 | $4.20 | Mouser/DigiKey |
| MAX13440EETD+ | 1 | $5.30 | Mouser |
| nRF52832-QFAA | 1 | $4.50 | Mouser |
| W25Q128JVSIQ | 1 | $1.20 | DigiKey |
| MCP4131-503E | 1 | $0.90 | Mouser |
| TPS63020DSJR | 1 | $3.40 | TI |
| MCP73831T-2ACI/OT | 1 | $0.60 | Microchip |
| Omron G6K-2F-Y-DC3 | 1 | $3.20 | DigiKey |
| SSD1306 128×64 OLED | 1 | $2.50 | AliExpress |
| microSD socket | 1 | $0.80 | LCSC |
| USB-C receptacle | 1 | $0.70 | LCSC |
| 250Ω 0.1% resistor | 1 | $0.30 | DigiKey |
| Passives (R, C, L) | ~30 | $2.00 | — |
| PCB (4-layer) | 1 | $3.00 | JLCPCB |
| 500 mAh LiPo | 1 | $2.50 | — |
| **Total** | | **~$32** | |

---

## 10. Acknowledgments

HART-Bleeder was designed and authored by **jayis1** as an original security
research tool. The HART protocol is defined by the FieldComm Group
(formerly HART Communication Foundation); this device is an independent
implementation for authorized security research and is not affiliated with or
endorsed by the FieldComm Group. All firmware, hardware, and software in this
repository are original works by jayis1, licensed under GPL-2.0.

---

*Copyright (c) 2026 jayis1. Licensed under GPL-2.0.*
*Designed for authorized security research and red team operations only.*