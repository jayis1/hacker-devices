# Harmonic Reaper — Non-Linear Junction Detector (NLJD) for TSCM

> **Author:** jayis1
> **License:** GPL-2.0
> **Status:** ✅ Complete — docs, KiCad, firmware, companion app

---

## ⚖️ Legal & Ethical Disclaimer

The Harmonic Reaper is an open hardware **Technical Surveillance Counter-Measures (TSCM)** instrument
designed exclusively for **authorized** security surveys — sweeping a facility you own or have written
permission to inspect for hidden electronic surveillance devices (bugs), covert cameras, GPS trackers,
and similar threats. It transmits low-power RF and must be operated in accordance with local radio-
communications regulations (FCC §15.209 in the US, ETSI EN 300 440 in the EU, and equivalents elsewhere).

**Do NOT** use this device:
- Against targets you do not own or control without explicit written authorization.
- To locate, tamper with, or exfiltrate legitimate equipment you do not own.
- In any manner that violates national spectrum or surveillance laws.

The author (jayis1) and contributors disclaim all liability for misuse. You are responsible for
obtaining proper legal authorization before operating the device.

---

## 1. What It Is

**Harmonic Reaper** is a portable, battery-operated **Non-Linear Junction Detector (NLJD)** — the
gold-standard tool in professional bug-sweeping. When a radio-frequency carrier illuminates a
semiconductor junction (any PN or Schottky junction in a transistor, diode, IC, or even a corroded
metal contact), the junction's non-linear I-V curve **rectifies** the incident wave and re-radiates
harmonics of the fundamental. The 2nd harmonic (2f₀) is characteristic of semiconductors; the 3rd
harmonic (3f₀) is characteristic of dissimilar-metal / corroded contacts. By comparing the relative
amplitude of 2f₀ vs 3f₀ returns, an operator can distinguish a hidden bug from a rusty nail.

Unlike a frequency-counter or spectrum-sweeper that only detects *active* transmitters, the NLJD
finds **dormant** electronics — devices that are powered off, in standby, air-gapped, or using burst
transmission. This makes it indispensable for:

- Pre-meeting room sweeps (corporate boardrooms, SCIFs, hotel rooms)
- Vehicle anti-track sweeps (GPS trackers hidden in wheel wells, undercarriages)
- Gift / mail screening (covert devices inside presents or parcels)
- Supply-chain integrity inspection (implanted hardware trojans in returned equipment)
- Red-team post-engagement device recovery (locate your own dropped beacons)

---

## 2. Attack Surface & Threat Model

| Threat Class | What NLJD Detects | Limitation |
|---|---|---|
| **Covert RF transmitter (bug)** | Strong 2f₀ from oscillator/mixer ICs even when in standby | Can't classify modulation — follow up with SDR |
| **Hidden camera (wired or wireless)** | 2f₀ from CMOS sensor + lens coating scatter | Optical confirmation still needed |
| **GPS tracker (magnetic mount)** | 2f₀ from GPS SoC + GSM modem, strong return near metal | Metallic underbody raises 3f₀ noise |
| **Audio recorder (flash-based)** | 2f₀ from ADC + flash controller | Small devices harder at range |
| **Hardware implant (USB/ETH cable)** | 2f₀ from embedded MCU, even unpowered | Must isolate cable from host |
| **False positives (rusty metal, dissimilar-metal contacts)** | Dominant 3f₀ with weak 2f₀ | Ratio-based classifier rejects these |

**Threat model the device *cannot* address:**
- Purely passive optical bugs (no electronics — mirror, fiber tap).
- Devices heavily shielded by conductive enclosures (Faraday) — returns attenuated.
- Biological surveillance (human informants).

The operator's threat model is the classic TSCM one: an adversary has planted one or more covert
electronic devices in the target space and may have done so days or weeks in advance. The devices may
be dormant, scheduled, or voice-activated. The NLJD provides a *detection* primitive; localization to
<10 cm is achieved via the directional antenna and a dual-channel (2f₀/3f₀) amplitude readout.

---

## 3. Hardware Specifications

### 3.1 Mechanical & Power

| Parameter | Value |
|---|---|
| Form factor | Handheld wand, 280 mm × 56 mm × 34 mm, 480 g |
| Wand head | Directional log-spiral antenna, 90 mm aperture, 35° 3 dB beamwidth |
| Battery | 2× 21700 Li-ion, 5000 mAh each (10 Ah total), replaceable |
| Runtime | ~4.5 h continuous sweep, >24 h standby |
| Charging | USB-C PD 20 V, 2 A; full charge in 90 min |
| Display | 0.96" OLED (128×64 SSD1306) on wand handle + haptic motor |
| Connectivity | BLE 5.2 (nRF52840) to companion app, USB-C CDC for firmware/log |
| Operating temp | −10 °C to +50 °C |
| Ingress | IP54 (sweep environment dust/splash) |

### 3.2 RF Architecture

| Parameter | Value |
|---|---|
| Fundamental frequency f₀ | 2.4 GHz (ISM, switchable to 2.36 GHz DFS band) |
| TX power | −10 to +15 dBm, software-settable in 1 dB steps (30 dB AGC range) |
| RX bands | 2f₀ = 4.80 GHz, 3f₀ = 7.20 GHz (simultaneous dual-channel) |
| RX architecture | Direct-conversion, I/Q, 12-bit, 20 MSPS per channel |
| Receiver NF | 3.2 dB (2f₀ channel), 3.8 dB (3f₀ channel) |
| Dynamic range | >75 dB per channel, log-detector + ADC combined |
| Harmonic rejection | 2f₀/3f₀ isolation >55 dB via diplexer + cavity filter |
| Pulse mode | 1–10 kHz PRF, 50 ns–10 µs pulse width (reduces average power, raises peak) |
| Sensitivity (2f₀) | −125 dBm in 1 Hz RBW (CW), −105 dBm in 1 kHz RBW |
| Spatial resolution | <8 cm at 30 cm standoff (3 dB spot size) |

### 3.3 Core Components (BOM highlights)

| Function | Part | Notes |
|---|---|---|
| MCU / BLE | **Nordic nRF52840** (Cortex-M4F, 64 MHz) | Wand control, BLE, AGC loop, OLED, battery |
| Signal processor | **Xilinx Spartan-7 XC7S6** FPGA | Harmonic FFT, ratio classifier, pulse timing |
| TX synthesizer | **Analog Devices ADF4159** | 2.4 GHz fractional-N, <1 Hz resolution |
| TX PA + AGC | **Qorvo QPL9547** | 30 dB gain range, 0.5 dB steps |
| 2f₀ receiver | **Analog Devices AD9361** (agile, subset used) | Direct-conversion I/Q, 12-bit |
| 3f₀ receiver | **Analog Devices ADL5802** mixer + **AD9226** ADC | Heterodyne to 200 MHz IF |
| Antenna | Custom log-spiral on 4-layer Rogers RO4350B | Dual-band feed, 35° beamwidth |
| Diplexer | Cavity + SAW combined (2f₀/3f₀ split) | >55 dB isolation |
| Power management | **TI TPS6523550** PMIC + 4× **TPS61088** boost | 3.3 V/1.8 V rails, 9 V PA rail |
| OLED | **Solomon Systech SSD1306** | 128×64, SPI |
| IMU | **TDK InvenSense ICM-42688** | Sweep-angle logging, motion tagging |
| Storage | **Winbond W25Q128** 16 MB NOR + microSD | Sweep logs, geo-tagged hits |
| USB-C PD | **TI TPS25750** | 20 V charging, CDC data |

### 3.4 Block Diagram

```
        ┌─────────────────────────────────────────────────────────────────┐
        │  Battery 2×21700 ── PMIC (TPS6523550) ── 3.3V/1.8V/9V rails     │
        │  USB-C PD (TPS25750) ─────────────────── / charger                │
        └──────────────┬──────────────────────────────────────────────────┘
                       │ power
        ┌──────────────▼──────────────────────────────────────────────────┐
        │  nRF52840 (Cortex-M4F) — wand controller                         │
        │   • BLE 5.2 stack (Nordic SoftDevice)                            │
        │   • AGC loop, pulse timing, OLED UI, IMU logging                 │
        │   • USB-C CDC log/firmware                                       │
        └──┬───────────┬──────────────┬──────────────┬────────────────────┘
           │ SPI       │ SPI          │ UART          │ GPIO
           │           │              │               │
   ┌───────▼───┐  ┌────▼────┐  ┌──────▼──────┐  ┌────▼─────┐
   │ Spartan-7 │  │ SSD1306  │  │ ICM-42688   │  │ Haptic   │
   │  FPGA      │  │  OLED    │  │  IMU (SPI)  │  │  DRV2605 │
   │            │  └──────────┘  └─────────────┘  └──────────┘
   │  • 2× FFT  │
   │  • ratio    │
   │  • peak     │
   └──┬─────────┬─┘
      │ SPI     │ SPI
   ┌──▼────┐ ┌──▼────────────┐
   │ AD9361│ │ ADL5802 +     │
   │ (2f₀) │ │ AD9226 (3f₀)  │
   └──┬────┘ └──┬───────────┘
      │ I/Q     │ IF
   ┌──▼─────────▼───────────┐      ┌───────────────────┐
   │   Diplexer / 4.8/7.2   │◄─────│  TX chain         │
   │   cavity filter        │      │  ADF4159 → QPL9547│
   └───────────┬────────────┘      │  (2.4 GHz PA+AGC) │
               │                   └─────────┬─────────┘
        ┌──────▼──────┐                       │
        │ Log-spiral  │◄──────────────────────┘
        │ antenna     │
        │ (TX + RX)   │
        └─────────────┘
```

---

## 4. Firmware Architecture & Design Decisions

### 4.1 Why two processors?

The nRF52840 handles user-facing control — BLE, OLED, IMU, haptic — because it has a mature
SoftDevice and excellent BLE power management. The Spartan-7 FPGA handles the **deterministic**
real-time work that a 64 MHz MCU cannot: two simultaneous 1024-point FFTs at a 20 kSPS sweep rate,
ratio classification, and pulse-timing with sub-50 ns jitter. Splitting the work this way keeps
the BLE stack responsive (no long IRQ-disabled FFT critical sections) and lets the FPGA pipeline
run at its own clock without contending for CPU cycles.

### 4.2 Detection algorithm

1. **Transmit**: ADF4159 synthesizes 2.4 GHz CW (or 1–10 kHz pulse train for peak-power mode).
   TX power is set by AGC so the *received* 2f₀ level stays below ADC saturation.
2. **Receive**: The antenna's reflected return is split by the diplexer into 2f₀ (4.8 GHz) and
   3f₀ (7.2 GHz) channels, each down-converted to baseband/IF and digitized.
3. **Spectral estimate**: FPGA computes a 1024-point radix-2 FFT on each channel, integrates
   N=16 spectra (coherent), and reports (P2, P3) — the power in the bins around 2f₀ and 3f₀.
4. **Classifier**: `ratio_db = 10·log10(P2 / P3)`.
   - `ratio > +6 dB`  → **SEMICONDUCTOR** (likely bug / hidden electronics)
   - `ratio < −6 dB` → **DISSIMILAR METAL** (corroded joint, rusty nail — false positive)
   - `|ratio| < 6 dB` → **AMBIGUOUS** (investigate, reduce standoff)
5. **Spatial scan**: IMU logs wand heading; operator sweeps the wand across the surface.
   A "hit" is declared when P2 crosses a threshold AND the ratio favors semiconductor for ≥3
   consecutive IMU-tagged samples. The haptic fires and the hit is geo/angle-tagged to storage.
6. **Quiet mode**: TX pulses are gated to 50 ns–1 µs with randomized PRF to defeat detection
   by an adversary's own RF activity monitor.

### 4.3 Firmware layout

| File | Role | LOC |
|---|---|---|
| `firmware/main.c` | Boot, scheduler, mode state machine, BLE C2 | ~260 |
| `firmware/board.h` | Pin map, peripheral assignments | ~90 |
| `firmware/registers.h` | nRF52840 peripheral register definitions | ~150 |
| `firmware/drivers/adf4159.c` | TX synthesizer driver (SPI, fractional-N) | ~170 |
| `firmware/drivers/qpl9547.c` | PA + AGC driver, power table | ~110 |
| `firmware/drivers/ad9361.c` | 2f₀ receiver init, I/Q streaming | ~200 |
| `firmware/drivers/ad9226_if.c` | 3f₀ IF channel ADC + mixer control | ~140 |
| `firmware/drivers/fpga_spi.c` | Spartan-7 command/status over SPI | ~130 |
| `firmware/drivers/oled_ssd1306.c` | OLED UI, bar graph, hit display | ~180 |
| `firmware/drivers/imu_icm42688.c` | IMU readout, sweep angle tagging | ~120 |
| `firmware/drivers/ble_c2.c` | BLE GATT command/status, Nordic UART Service | ~220 |
| `firmware/drivers/power_mgmt.c` | Battery gauge, PMIC rails, charging | ~110 |
| `firmware/drivers/storage.c` | Hit log to NOR flash + microSD, FAT glue | ~150 |
| `firmware/drivers/usb_cdc.c` | USB CDC log streaming + DFU trigger | ~100 |
| `firmware/Makefile` | arm-none-eabi build, link, size, flash | ~80 |

Total firmware: **~2100 lines** of compile-ready, well-commented C.

### 4.4 Key design decisions

- **2.4 GHz fundamental** rather than the traditional 800 MHz / 1.5 GHz: gives a smaller antenna
  (critical for a handheld wand) and lands 2f₀/3f₀ in bands where low-cost silicon receivers
  (AD9361 family, integrated mixers) are abundant, keeping BOM under the $100 target.
- **Pulsed TX** is the default mode. Average power stays well under FCC §15.209 limits while peak
  power (and thus detection range) is preserved. The MCU randomizes PRF so the device doesn't
  present a stable spectral signature to an adversary.
- **AGC on TX, not RX**: keeping the *received* 2f₀ level in the ADC's sweet spot is more robust
  than per-channel RX AGC because the 2f₀/3f₀ ratio — the actual classifier — must be measured with
  identical receiver gains, which fixed-gain RX guarantees.
- **IMU sweep-angle tagging**: lets the companion app reconstruct the spatial sweep path and
  plot hits on a polar map of the room, rather than relying on the operator's memory.

---

## 5. Companion Application

A React Native app (iOS + Android) pairs over BLE 5.2 and provides:

- **DashboardScreen** — live P2/P3 bar graph, ratio, classification verdict, battery
- **SweepScreen** — polar map of the room with hits plotted by IMU heading + range, sweep replay
- **ThresholdScreen** — TX power, ratio thresholds, pulse mode, quiet-mode toggle
- **HitLogScreen** — scrollable list of hits with timestamp, angle, power, ratio, geo-tag
- **SettingsScreen** — BLE pairing, firmware update (DFU over BLE), data export, calibration
- **CalibrationScreen** — walk-through wizard: baseline noise floor, reference diode check

The app uses Nordic's UART Service (NUS) for command/status and a custom binary frame protocol
(`utils/protocol.js`) that mirrors the firmware's `ble_c2.c` packet format.

---

## 6. Use Cases

### Red Team / Physical Pentest
- **Pre-engagement sweep of your own gear** — confirm no implant was placed in your hotel room
  or rental vehicle before the operation starts.
- **Post-engagement recovery** — locate beacons/dropboxes you deployed but couldn't retrieve.
- **Clean-room inspection** — verify a client facility is free of planted devices before a
  sensitive meeting or product launch.

### Security Researcher
- **Hardware-implant hunting** — screen returned / refurbished equipment for supply-chain
  implants (the 2f₀ return from an added MCU is unmistakable against a clean PCB).
- **Covert-camera discovery** — sweep AirBnBs, short-term rentals, dressing rooms.
- **Anti-stalker countermeasure** — locate GPS trackers magnetically attached to vehicles.

### Penetration Tester
- **Physical access recon** — before a physical pentest, sweep the target's reception / meeting
  areas to confirm whether the *target* has its own surveillance that could expose your team.
- **Device triage** — rapidly localize any electronic device in a wall, ceiling, or fixture
  before opening it, reducing damage and time.

---

## 7. File Layout

```
harmonic-reaper/
├── README.md                      (this file)
├── phase1_conceptual_architecture.md
├── phase2_component_selection_schematics.md
├── phase3_pcb_blueprints_layout.md
├── phase4_software_stack.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   ├── linker.ld
│   ├── drivers/
│   │   ├── adf4159.c        # TX synthesizer
│   │   ├── qpl9547.c        # PA + AGC
│   │   ├── ad9361.c         # 2f₀ receiver
│   │   ├── ad9226_if.c      # 3f₀ IF channel
│   │   ├── fpga_spi.c       # Spartan-7 control
│   │   ├── oled_ssd1306.c   # OLED UI
│   │   ├── imu_icm42688.c   # IMU sweep tagging
│   │   ├── ble_c2.c         # BLE command/control
│   │   ├── power_mgmt.c     # battery + PMIC
│   │   ├── storage.c        # hit log + microSD
│   │   └── usb_cdc.c        # USB CDC + DFU
│   └── ld/
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── App.js
    ├── package.json
    ├── screens/
    │   ├── DashboardScreen.js
    │   ├── SweepScreen.js
    │   ├── ThresholdScreen.js
    │   ├── HitLogScreen.js
    │   ├── SettingsScreen.js
    │   └── CalibrationScreen.js
    ├── components/
    │   └── BLEManager.js
    └── utils/
        └── protocol.js
```

---

## 8. Calibration & Operation

1. **Baseline**: Hold the wand 1 m from any wall in "QUIET RX" mode (TX off). Record the noise
   floor in both channels. The firmware stores this as `baseline_2f` / `baseline_3f`.
2. **Reference diode**: Hold the included 1N4148 reference target at 30 cm. Confirm a strong 2f₀
   return with `ratio > +6 dB`. This validates the TX/RX chain end-to-end.
3. **Sweep**: Set TX power so baseline + 30 dB lands mid-ADC. Move the wand in a 30 cm raster
   pattern across the surface at ~10 cm/s. A hit triggers haptic + OLED peak + a logged entry.
4. **Triage**: For each hit, reduce standoff to 10 cm and re-measure. A true semiconductor hit
   will *increase* in P2 while keeping `ratio > +6 dB`; a dissimilar-metal false positive will
   shift toward `ratio < −6 dB` as the spot size tightens.

---

## 9. Regulatory Notes

The 2.4 GHz ISM band is license-free in most jurisdictions for low-power intentional radiators.
Pulsed mode keeps average EIRP under 41.25 dBm/1 MHz (FCC §15.247 equivalent) and the device is
intended for indoor TSCM use. Operators in the EU should verify ETSI EN 300 440 compliance for
their specific deployment. **The operator is solely responsible for lawful operation.**

---

*Designed and authored by **jayis1**. Open hardware under GPL-2.0.*