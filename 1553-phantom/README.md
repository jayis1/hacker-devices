# 1553-Phantom — MIL-STD-1553B Avionics Bus Implant & Bus-Controller Simulator

> **Author:** jayis1
> **License:** Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT
> **Status:** ✅ Complete (concept → schematic → PCB → firmware → app)

---

## ⚖️ Legal & Ethical Disclaimer

1553-Phantom is a **defensive and authorized-research tool only**. It is intended
for security researchers, red teams operating under signed Rules of Engagement,
avionics OEMs hardening their LRUs, and maintenance/airworthiness engineers
performing ground-test bench validation.

**You MUST NOT** use 1553-Phantom against any airborne vehicle, weapon system, or
ground station without explicit written authorization from the asset owner and,
where applicable, the relevant civil/military airworthiness authority.
MIL-STD-1553B carries real flight-control, navigation, and weapons data.
Unauthorized injection of commands onto a live 1553 bus can cause loss of vehicle
and loss of life, and is a serious federal / international crime under aviation
sabotage, critical-infrastructure, and computer-fraud statutes.

By building or operating 1553-Phantom you accept **full and exclusive**
responsibility for any damage, injury, or legal consequence. The author
(jayis1) provides this design **as-is, without warranty**, and disclaims all
liability for misuse.

**If in doubt: keep it on the test bench. Never connect to a live aircraft bus.**

---

## 1. What 1553-Phantom Is

1553-Phantom is a pocket-sized **inline implant and bus simulator** for the
**MIL-STD-1553B** avionics data bus — the dual-redundant, transformer-coupled,
Manchester-biphase-L encoded serial bus that has carried flight-control,
navigation, and weapons-bus data on virtually every military aircraft since the
late 1960s, and on a great many commercial airliners, rotorcraft, and
launch-vehicle stages as well.

Physically, the device is a small ruggedized box (≈ 70 × 45 × 18 mm,
≈ 55 g) with two **triax / twinax** MIL-1553 connectors (primary + redundant
channels), a 1.3″ monochrome OLED, a USB-C CDC port for host control, and a
single user button. Internally it is built around a **Lattice iCE40-UP5K
FPGA** that implements the hard-real-time 1 Mbps Manchester codec, and an
**STM32G474** Cortex-M4F host that runs the protocol stack (BC / RT / BM role
machines), the command scheduler, the CLI, and the display.

### Why an FPGA?

MIL-1553 is brutal on a generic MCU. Each bit cell is 1 µs; Manchester
biphase-L coding means a transition at every bit-center plus a mid-bit
violation to delimit sync heads; the standard mandates a tight ±150 ns
zero-cross symmetry. Polling an interrupt from a Cortex-M at 1 MHz with
jitter is unreliable. The proven approach — used by every commercial 1553 IP
core — is to put the serializer/deserializer, sync detection, parity/gap
checking, and word-timer in an FPGA, then hand framed 20-bit words up to a
host MCU over a parallel or SPI slave interface. 1553-Phantom follows that
architecture with a tiny low-cost iCE40.

### Three roles, one device

MIL-1553 traffic is strictly **command/response**: exactly one *Bus
Controller* (BC) issues commands, up to 31 *Remote Terminals* (RT) respond,
and an optional *Bus Monitor* (BM, "black box") records. 1553-Phantom can be
configured into any of the three roles, **or run several at once** — for
example BM-sniffing primary channel A while acting as a rogue RT-7 injecting
spoofed attitude data on channel B.

| Role | Code path | What it does |
|------|-----------|--------------|
| **BM** (Bus Monitor) | `bm_role.c` | Passively decodes and timestamps every word on both channels; reports status, detects RT timeouts / parity errors / parity fail / sync-loss / protocol violations. |
| **RT** (Remote Terminal) | `rt_role.c` | Emulates up to 8 RT addresses simultaneously. Per-RT, per-subaddress transmit/receive substore buffers; supports mode codes (vector word, synchronize, transmit status etc.); injects selectable fault classes into its own status word (busy, subsystem flag, terminal flag, service request). |
| **BC** (Bus Controller) | `bc_role.c` | Drives the bus with a fully programmable minor/major frame schedule. Can issue every command type (BC→RT, RT→BC, RT→RT, mode-code, broadcast). Targets real RTs to fuzz their protocol parsers (illegal subaddress, oversized block length, wraparound SA wrap attacks, double-BC contention). |
| **MITM** | `mitm_role.c` | Active inline relay: forwards Channel A↔B transparently while selectively rewriting, dropping, or injecting selected word patterns based on a user-defined match/replace table. |

---

## 2. Attack Surface and Threat Model

### What the standard gets wrong (and a red team can abuse)

MIL-1553B is a 1973 spec. The threat model it assumed was *physical* — a
broken wire, a single-point short, or a lightning transient. It deliberately
contains **no cryptographic authentication, no integrity codes, and no
sequence numbering**. Authentication is purely *implied by physical address*:
if a word arrives from RT address 6, the BC trusts that RT-6 sent it, because
RT-6 is the only thing wired to that drop on that twisted-pair. Any device
physically on the bus that can speak 1553 *is* the bus, for all intents and
purposes.

Concrete attack classes 1553-Phantom enables a researcher to demonstrate:

1. **Rogue-RT injection** — impersonate an RT and push spoofed navigation,
   fuel, or hydraulic data to the BC and to other RTs via RT→RT transfers. The
   legitimate RT may also be replying, producing a collision / protocol fault —
   or, if the attacker is faster, its word may simply win (there is no
   arbitration in 1553, only timing).
2. **Rogue-BC takeover** — become the active BC and command real LRUs. This is
   the equivalent of a CAN-bus "frame injection" attack but with higher
   stakes. The original BC will fault on the resulting protocol violations
   and may go quiet, ceding the bus.
3. **Status-word fault injection** — as a rogue RT, deliberately set/clear
   `BUSY`, `SUBSYSTEM FLAG`, `TERMINAL FLAG`, or `SERVICE REQUEST` in the
   status word to force the real BC into its fault-handling branches.
4. **Mode-code fuzzing** — issue reserved/illegal mode codes, oversized
   `Transmit Vector Word`, illegal `Transmit Last Word`, etc., to probe LRU
   parser robustness.
5. **Block-length & subaddress wrap** — issue BC→RT/RT→BC commands with
   illegal word counts (0 → 32 are valid; 31 is special-cased as 32) and
   subaddress wrap patterns (SA 31 is a mode-code selector).
6. **Cross-channel injection** — inject on the redundant channel B while the
   live bus is healthy on channel A, observing whether the BC's redundancy
   management actually cuts over to the corrupt data or to the good copy.
7. **MITM rewrite** — inline between a real LRU and the stub coupler,
   transparently rewrite a single sensor word (e.g. angle-of-attack) without
   disturbing the rest of the bus.
8. **Gap-timing fuzzing** — vary inter-message gaps below the 4 µs minimum
   and above the 14 µs (RT response) timeout to probe BC/RT edge cases.
9. **Parity / sync corruption** — inject single-bit parity faults and
   malformed sync heads to measure receiver fault counters and isolation
   behavior.
10. **Black-box traffic export** — passive BM capture with full per-word
    timestamps (12.5 ns resolution from the FPGA), exported as a `.pcapng`
    (LINKTYPE_MIL1553 synthetic) or CSV for post-hoc analysis.

### Threat model, stated plainly

| Asset | Adversary | Channel | Impact |
|-------|-----------|---------|--------|
| RT sensor data (AoA, fuel, IMU) | Rogue RT on the stub | Wired 1553 | Bad data → wrong decisions |
| BC command schedule | Rogue BC takeover | Wired 1553 | Unauthorized actuator commands |
| Redundancy isolation | Cross-channel inject | Wired 1553 ch. B | Defeat redundancy mgmt |
| LRU parser robustness | Mode-code/length fuzz | Wired 1553 | LRU crash / lockup / DoS |
| BM "black box" integrity | MITM rewrite | Inline at LRU drop | Falsified recorded data |

1553-Phantom lets the **defender** reproduce all of the above on a bench, on a
hangar test rig, or on a decommissioned LRU, to qualify LRUs and bus-network
design against them. It is **not** for use on live aircraft.

---

## 3. Hardware Specifications

### Block summary

| Subsystem | Part | Notes |
|-----------|------|-------|
| 1553 physical layer | 2 × **HOLT HI-1573PSF** dual-channel transceiver ICs (or single HI-1567), 3.3 V, integrated transformer drivers, ISO7816/UART-style interface to FPGA | 4 channels total = primary + redundant + 2 spare/drop-tap. Transformer isolation per AIA, per-pin 6.2 kV. |
| Couplers / stubs | External MIL-1553 triax (twinax) via **MIL-DTL-38999** mini-circular connectors, stub-terminated 78 Ω, isolated coupler on host side | Device presents as an inline coupler or a stub drop, jumper-selectable. |
| Codec / PHY | **Lattice iCE40-UP5K** (SG48) — 5.3k LUT4s, 16 DSP, 1 Mb SPRAM, 128 kb BRAM | Implements `manchester_tx`, `manchester_rx`, `sync_detect`, `word_timer`, dual-channel FIFO, SPI-slave to host. Open-source `yosys/nextpnr` toolchain — no vendor lock-in. |
| Host MCU | **STM32G474CBT6** — Cortex-M4F @ 170 MHz, 128 kB Flash, 128 kB SRAM, USB FS + CAN-FD, AES-256, CORDIC, FMAC | Runs the role state machines, BC frame scheduler, BM capture ring, RT substore, CLI, display. |
| USB | **USB 2.0 FS CDC**, USB-C receptacle, on-board LDOs | 12 Mbps CDC shell; bit-banged or USART2-tied — see firmware notes. |
| Power | 3.7 V **Li-ion 14500** cell (replaceable) + **TP4056** charger + **MAX17048** fuel-gauge; USB-C 5 V charging; external 28 VDC barrel jack for hangar-test | ~5–6 h passive sniffer, ~3 h active BC. Hot-swap battery without dropping the bus via USB power. |
| Display | **SSD1306** 128×64 OLED, SPI @ 10 MHz | BC frame status, BM packet count, RT subs, role switcher, capture/export progress. |
| User input | 1 × tactile button (long-press = role cycle, short = mode) + capacitive sense on case for "panic" | One-button UX; full control via CLI. |
| Debug | SWD 5-pin header + iCE40 6-pin SPI flash header | OpenOCD / iceprog compatible. |
| Indicators | 3 LEDs (PWR / ACT-A / ACT-B) + green/red combo LED for "armed/armed-inject" safety state | 1553 traffic on each channel blinks ACT-A/B. |
| Form factor | 70 × 45 × 18 mm PCB sandwich, 1.6 mm 4-layer FR-4, machined Al enclosure (Faraday cage, RF-tight) with gasket, MIL-DTL-38999 connectors on a short pigtail | Rugged; < 60 g. |

### Power budget

| Rail | Source | Load | Notes |
|------|--------|------|------|
| +3.3 V | USB-C 5 V via LDO + battery boost | MCU + FPGA + transceivers + OLED | ~120 mA peak |
| +1.2 V | On-board LDO for iCE40 core | FPGA core | ~30 mA |
| +5.0 V | USB-C only (or 28 V buck) | Charger | 1 A USB |

### Clocking

- MCU HSE 16 MHz crystal → PLL 170 MHz (3.3 V).
- FPGA on-board 24 MHz crystal → internal PLL → 48 MHz codec clock (96-sample-per-bit Manchester sampling).
- No free-running crystal between FPGA and MCU: all word timestamps come from the FPGA's 96-MHz counter, latched into a register the MCU reads over SPI; MCU wraps a 32-bit µs counter for export consistency.

### Why not a single-chip "1553 transceiver + MCU"?

There are integrated 1553 MACs (e.g. DDC BU-61580, Holt HI-6130) that expose
an MCU bus interface directly. We deliberately chose the **FPGA + transceiver**
split for three reasons:

1. **Openness.** The HI-6130 / BU-61580 are ITAR/EAR-controlled parts and their
   MACs are closed cores. The iCE40 codec we ship is open Verilog.
2. **Flexibility.** Fuzzing requires pathological Manchester waveforms (e.g.
   half-bit-width glitches, invalid parity, mid-word sync heads). A fixed MAC
   will *correct* those for you; the FPGA codec lets the researcher deliberately
   inject them.
3. **Cost & availability.** An iCE40-UP5K + HI-1573 costs ~$20 in single units;
   a controlled integrated MAC is far more expensive and gated.

---

## 4. Architecture and Block Diagram

```
            ┌────────────── MIL-STD-1553B bus (ch. A primary) ─────────────┐
            │                                                              │
   ┌────────┴────────┐   ┌───────────────┐   ┌─────────────────────┐  ┌────┴────────┐
   │ 38999 Ch-A conn │───│ HI-1573 #1    │───│ iCE40-UP5K          │  │ 38999 Ch-B │
   │ (triax)         │   │ 1553 xceiver  │   │  manchester_rx0     │  │ conn        │
   │ + ISO-coupler   │   │ + xfmr drv    │   │  manchester_tx0     │  │ (redundant) │
   └─────────────────┘   └───────────────┘   │  manchester_rx1     │  └─────────────┘
                                             │  manchester_tx1     │
   ┌─────────────────┐   ┌───────────────┐   │  sync_detect  ×2    │
   │ 38999 Ch-B conn │───│ HI-1573 #2    │───│  parity_check ×2    │
   │                 │   │ (spare / tap) │   │  word_timer        │
   └─────────────────┘   └───────────────┘   │  gap_watchdog      │
                                             │  dual 1 KiB FIFO    │
                                             │  SPI-slave → MCU   │
                                             │  96 MHz timestamp  │
                                             └─────────┬──────────┘
                                                       │ SPI (host master)
                                            ┌──────────┴───────────┐
                                            │  STM32G474           │
                                            │  role FSM (BC/RT/BM/MITM)│
                                            │  frame scheduler     │
                                            │  substore buffers    │
                                            │  capture ring (32 KiB)│
                                            │  CLI / display / btn │
                                            └────────┬─────────────┘
                                                     │
              ┌───────────────┬──────────────┬──────┴┐
              │ SSD1306 OLED   │ USB-C CDC    │ LEDs   │ TP4056/MAX17048 battery
              └───────────────┴──────────────┴────────┘
```

### Data flow (BC role, example)

1. Host CLI parses a schedule from CSV into `bc_schedule[]`.
2. Scheduler tick fires at T = T0 + Δt. MCU writes the BC command word
   (`0x1C07` — RT 7, SA 3, RX, 4 words) plus up to 4 data words into the
   FPGA TX FIFO over SPI, then asserts `TX_GO`.
3. FPGA Manchester-encodes, drives the selected channel's HI-1573 tx.
4. On the channel, real RT-7 responds with a status word + 4 words.
5. FPGA `manchester_rx` syncs, decodes, parity-checks, drops each 20-bit
   word into the RX FIFO with a 32-bit timestamp.
6. MCU drains the RX FIFO, validates the status word's RT address, logs the
   response to the capture ring, advances the schedule, and renders the
   OLED.

### Data flow (BM / sniffer)

Same path but the MCU never writes the TX FIFO; it only drains RX from both
channels continuously into a ring buffer. On host "export", the MCU streams
the ring over USB CDC in a custom `1553cap` binary format (header + word
records), which the companion app converts to `.pcapng` or CSV.

---

## 5. Firmware

### Layout

```
firmware/
├── Makefile          arm-none-eabi-gcc build for STM32G474 + icepack for FPGA bitstream
├── linker.ld         memory layout: 128K flash, 128K SRAM
├── board.h           pin map, role enum, public config
├── registers.h       STM32G474 register definitions + bit helpers
├── main.c            clock tree, GPIO, scheduler, CLI dispatch, OLED render
└── drivers/
    ├── fpga_link.c/.h   SPI master to iCE40, TX FIFO load, RX FIFO drain, status read
    ├── codec_reg.h      mirror of the FPGA register map (sync to verilog)
    ├── bc_role.c/.h     Bus Controller: schedule parser, frame runner, response check
    ├── rt_role.c/.h     Remote Terminal: up to 8 RT addresses, substores, mode codes
    ├── bm_role.c/.h     Bus Monitor: dual-channel capture ring, protocol-fault detector
    ├── mitm_role.c/.h   Inline rewrite: match/replace table, forward/drop/inject
    ├── capture.c/.h     32 KiB ring buffer, 1553cap binary export over USB CDC
    ├── display.c/.h     SSD1306 driver (SPI), text + small bar renderer
    ├── buttons.c/.h     debounce + long-press + panic handler
    └── usb_cdc.c/.h     STM32 USB FS CDC, 12 Mbps, line-coding, ring RX
```

### Design decisions

- **Register-level, no HAL.** `registers.h` defines the few STM32G474 blocks
  we touch (RCC, GPIO, SPI1, TIM6/7, USB, PWR) with named bit fields. This
  keeps the firmware small and self-contained, and lets the researcher read
  exactly what the silicon is doing.
- **Single main loop, no RTOS.** The workload is bounded and reactive. A 1 ms
  `TIM6` tick drives the schedule deadline check; everything else is event
  driven (SPI completion, USB RX, button EXTI). Predictable worst-case
  latency beats a generic scheduler here.
- **FPGA as the hard-RT layer.** The MCU never bit-bangs Manchester. It
  hands framed 20-bit words to the FPGA; the FPGA does the
  microsecond-accurate encode/decode and timestamp. The MCU's job is
  protocol and policy.
- **Capture ring is lock-free single-writer/single-reader.** `bm_role`
  writes head; USB export reads tail. No locking needed because the same CPU
  does both and never preempts itself.
- **Fuzz primitives live in `bc_role` / `rt_role`.** Each exposes a
  `fuzz_*_enable(mask)` API: bit 0 = parity fault, bit 1 = bad sync, bit 2 =
  illegal word count, bit 3 = gap violation, bit 4 = status-word fault
  injection, bit 5 = reserved-mode-code. The CLI turns these on/off at runtime.
- **Safe-by-default boot.** On power-up the device is in **BM (passive sniff)
  role** with all TX paths gated. Switching to BC or RT requires an explicit
  `arm` command over USB CDC and a 3 s long-press on the case button, which
  raises the red "ARMED" LED. Power loss returns to BM. There is no way to
  inject 1553 traffic without this two-step arm.

### Approximate line counts (target 500+)

| File | Lines |
|------|-------|
| `main.c` | ~220 |
| `registers.h` | ~150 |
| `board.h` | ~90 |
| `drivers/fpga_link.c` | ~190 |
| `drivers/bc_role.c` | ~260 |
| `drivers/rt_role.c` | ~240 |
| `drivers/bm_role.c` | ~180 |
| `drivers/mitm_role.c` | ~170 |
| `drivers/capture.c` | ~140 |
| `drivers/display.c` | ~150 |
| `drivers/usb_cdc.c` | ~200 |
| `drivers/buttons.c` | ~90 |
| **Total C** | **~2080** |

Plus the open Verilog codec (`codec/manchester.v`, etc.) is referenced but
shipped separately; the firmware treats the codec as a register-level
black box.

---

## 6. Application / Software Interface

### USB CDC CLI

The firmware presents a single USB CDC ACM device at 12 Mbps. A line-oriented
shell accepts:

```
help
role bm|rt|bc|mitm            # switch role (RT/BC/MITM need arm first)
arm                          # 3 s button also required to finalize
disarm
status                       # print role, channel health, capture count

rt  set <addr>               # configure which RT address(es) to emulate (0-30)
rt  tx  <sa> <hex...>         # pre-load RT transmit substore
rt  fault <mask>             # inject status-word faults (busy/flag/svc)
rt  mode <code> [data]       # respond to a mode code with data

bc  load csv <schedule>      # parse a minor/major frame CSV
bc  run                      # start frame runner
bc  stop
bc  gap <us>                 # override inter-message gap (fuzz)
bc  fuzz <mask>              # enable fuzz primitives

mitm add <chan> <match> <repl>  # match/replace rule
mitm drop <chan> <match>
mitm inject <chan> <at_us> <word>
mitm clear

bm  start
bm  stop
bm  flush                    # drain capture ring to host
bm  export csv|1553cap|pcapng

log level debug|info|warn
reboot
```

### Companion app (React Native)

`app/` contains a React Native (Expo) companion that connects over a
USB-OTG CDC bridge on Android and (with `react-native-usb-serialport`) on
desktop. It provides four primary screens:

1. **Dashboard** — role selector, channel-A/B activity bars, capture count,
   armed/disarmed state, battery %.
2. **Bus Monitor** — live decoded word stream (cmd/data/status color-coded),
   per-RT response time histogram, fault counters, one-tap `Pause`,
   one-tap `Export` (saves `.pcapng` to the device download folder).
3. **BC Schedule Editor** — a table editor for minor/major frame CSV,
   with templates (`reset poll`, `normal poll`, `mode-code sweep`,
   `fuzz campaign`). Sends `bc load csv` over CDC.
4. **RT / MITM Console** — per-RT substore editor + mode-code response
   picker; MITM match/replace rule list with enable toggles.

### Export formats

- **CSV** — `ts_ns,ch,word,role,rt,sa,wc,type,parity_ok`.
- **1553cap** — compact binary (magic `15 53 CAP 1`), record stream;
  smallest on-wire size, used for long captures.
- **pcapng** — synthetic DLT 211 (LINKTYPE_MIL1553 via a private
  pseudo-linktype in the app). Wireshark with the `1553` dissector
  plugin (shipped in `app/utils/`) decodes it.

---

## 7. Use Cases

### Red team (authorized test rig only)

- **Rogue-RT data injection** — present as RT-12 (AHRS) and push a
  hard-over attitude value to the BC, observe downstream behavior on the
  bench.
- **BC takeover** — arm BC role, run a schedule that polls every real RT,
  observe whether the original BC isolates correctly or cedes the bus.
- **Redundancy defeat** — inject corrupt data on channel B while channel A
  is healthy; measure BC crossover vs. hold-last vs. fault behavior.
- **Mode-code fuzz sweep** — `bc fuzz 0x20` runs every mode code against
  every RT, logging which LRUs respond, fault, or hang.

### Security researcher

- **LRU parser robustness** — connect 1553-Phantom to a single LRU on a
  bench, run the fuzz suite, capture crashes / fault counters / bus
  lockups.
- **Capture diffing** — sniff a known-good schedule, then sniff a modified
  one, diff the two `.pcapng` files in Wireshark to characterize an LRU's
  response to malformed inputs.
- **Timing / gap fuzz** — vary the inter-message gap to map a receiver's
  gap-watchdog window precisely.
- **Status-word fault injection study** — systematically assert each
  status-word bit to enumerate which BC fault-handling branches are
  reachable.

### Penetration tester (air-gapped bench, decommissioned gear)

- **Side-channel on redundancy** — observe whether the BC's channel-A→B
  switchover is sticky after a single bad frame or requires N consecutive
  bad frames (a real attack-relevant parameter).
- **MITM at the LRU drop** — inline at a single LRU's stub coupler,
  rewrite one sensor word to study fault-propagation without disturbing
  the rest of the bus.
- **"Black box" integrity test** — inject on the BM channel to verify the
  recorded data can in fact be tampered with from a physical drop
  (motivating the case for cryptographic / authenticated-1553 extensions).

### Maintenance / airworthiness

- **Ground-test bus simulation** — replace a missing LRU with an RT
  emulator so the rest of the system can be powered and exercised on the
  bench without the real unit.
- **Cable / coupler continuity** — passive BM confirms both channels and
  every drop are electrically healthy before power-on.

---

## 8. Bill of Materials (excerpt)

| Ref | Part | Pkg | Notes |
|-----|------|-----|-------|
| U1 | STM32G474CBT6 | LQFP48 | host MCU |
| U2 | iCE40-UP5K-SG48 | QFN-48 | codec FPGA |
| U3,U4 | HOLT HI-1573PSF | PLCC-44 | 1553 dual transceiver (or HI-1567 single) |
| U5 | SSD1306 OLED 128×64 | module | SPI display |
| U6 | TP4056 | SOP-8 | Li-ion charger |
| U7 | MAX17048G+T10 | TDFN-8 | fuel gauge |
| U8 | AP2112-3.3 | SOT-23-5 | 3.3 V LDO |
| U9 | MIC5365-1.2 | SC-70 | FPGA core LDO |
| Y1 | 16 MHz crystal | HC-49 | MCU HSE |
| Y2 | 24 MHz crystal | SMD-3225 | FPGA |
| J1,J2 | MIL-DTL-38999 Series III twinax | panel | 1553 ch-A / ch-B |
| J3 | USB-C 16-pin | SMD | CDC + charge |
| J4 | 2.1 mm barrel | panel | 28 VDC hangar input |
| BT1 | 14500 Li-ion 600 mAh | holder | removable |
| LED1-3 | 0805 SMD | | PWR / ACT-A / ACT-B |
| LED4 | 0805 dual green/red | | armed / armed-inject |
| SW1 | tactile 6×6 mm | | user button |
| F1 | mini-blade 1 A | | 28 V input fuse |

Full reference designators and netlist are in `kicad/`.

---

## 9. Safety Interlocks

1553-Phantom is a device whose entire purpose is to *inject* traffic onto a
bus that may, in a misuse scenario, carry safety-critical data. We therefore
include hard interlocks that a pure sniffer would not need:

1. **Boot-to-passive.** Power-on role is always **BM**. No TX path is
   electrically enabled until `arm`.
2. **Two-step arm.** `arm` over USB CDC raises a software request; the
   device also requires a ≥3 s long-press of the user button. If either
   is missing within 5 s, the request lapses.
3. **Armed LED.** A red LED is lit whenever any TX-capable role is active.
   A second green LED additionally lights when *injecting* (BC/RT/MITM with
   live traffic). Two independent GPIOs drive the two LEDs; a single
   firmware bug cannot silently light the wrong one.
4. **Power-loss default.** The arm latch is held only in SRAM. Loss of
   power, brown-out, or watchdog reset returns the device to BM and the
   arm latch clears. There is no flash-persistent arm.
5. **Channel gate.** A dedicated FPGA output enables the HI-1573 tx
   drivers. The MCU cannot assert it without going through the arm latch,
   and the arm latch also gates it. Two domains must agree.
6. **Panic.** A capacitive sense on the case (or a second short-press on
   the button) immediately disarms and returns to BM, drains the TX FIFO,
   and holds the tx drivers low.

---

## 10. Limitations and Honest Caveats

- **Not a 1553 MAC.** The open iCE40 codec is sufficient for research, fuzz,
  and emulation; it is **not** a DO-254 DAL-A qualifiable MAC. Do not fly it.
- **Channel count.** 4 transceiver channels (2 primary, 2 spare/tap). For
  more, add a second HI-1573; the FPGA has the LUTs.
- **Sample rate.** 96 MHz codec sampling gives ~10 ns timestamp granularity
  (after the 96× decimation), well within the 150 ns zero-cross tolerance.
- **Open-source toolchain caveat.** iCE40 uses `yosys + nextpnr-ice40`
  (open). It is slower to build than vendor tools but has no license gate.
- **Legal.** See the disclaimer at the top. This is a bench tool.

---

## 11. Acknowledgements

The 1553-Phantom design, firmware, schematic, PCB, and app are the work of
**jayis1**. MIL-STD-1553B is a U.S. Department of Defense standard (SAE
AS15531 / MIL-STD-1553B Notice 4). Reference for protocol behavior only;
this project is not affiliated with or endorsed by the DoD, SAE, or any
aircraft OEM.

*File a build log / patch via the repo. Pull requests welcome.*