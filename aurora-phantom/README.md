# Aurora-Phantom — Optical Compromising-Emanations (Optical TEMPEST) Reconstruction Device

```
   ╔══════════════════════════════════════════════════════════════════════════╗
   ║   █████╗ ██████╗  ██████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗     ║
   ║  ██╔══██╗██╔══██╗██╔═══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗    ║
   ║  ███████║██████╔╝██║   ██║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝    ║
   ║  ██╔══██║██╔══██╗██║   ██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗    ║
   ║  ██║  ██║██║  ██║╚██████╔╝██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║    ║
   ║  ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝    ║
   ║                                                                          ║
   ║   Optical Compromising Emanations — Display & LED Content Reconstruction ║
   ╚══════════════════════════════════════════════════════════════════════════╝
```

**Author:** jayis1
**Version:** 1.0.0
**License:** GPL-2.0
**Status:** Research hardware design — firmware + PCB + companion app

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, red-team operations on systems you own or are explicitly authorized to assess, and TEMPEST/EMC pre-compliance screening of your own products. Reconstructing display content, keystroke indicators, or LED-borne data from light emanations originating from equipment you do not own may violate computer-fraud and abuse statutes (e.g., 18 U.S.C. § 1030), wiretap laws (18 U.S.C. § 2511), and data-protection regulations in your jurisdiction. Even *passive* optical interception of emanations from systems you do not own can constitute unauthorized access to protected computing resources and interception of electronic communications. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**Aurora-Phantom** is a covert, battery-powered **optical compromising-emanations** reconnaissance device that reconstructs information-bearing light leaking from electronic displays, status LEDs, fiber-coupled indicators, and backlit panels — **without any physical or electrical contact with the target**. Where the existing `spectre-sniffer` platform targets the *radio-frequency* (EM) side channel and `hdmi-siphon` requires a *direct cable tap*, Aurora-Phantom targets a fundamentally different physics: the **modulated visible and near-infrared light** that every illuminated screen and indicator LED inevitably radiates into its environment.

Every pixel that switches state, every scanline that refreshes, and every status LED that blinks encodes information into the photons it emits. That light does not stop at the bezel of the monitor. It leaks through office windows into the parking lot, diffuses under door gaps into the corridor, reflects off ceiling tiles, whiteboards, and the operator's own face, and is scattered by atmospheric haze. With a sufficiently sensitive photon-counting receiver and a lock-in correlation engine synchronized to the target's refresh/pixel clock, the spatial and temporal structure of the original image can be statistically recovered — even from a single-pixel brightness stream sampled through a window across the street.

Aurora-Phantom is the dedicated sensor platform for that recovery. It pairs a **16×16 single-photon avalanche diode (SPAD) focal array** (sensitive to single photons, DC–80 MHz event rate, 400–1000 nm) with a **Lattice iCE40UP5K FPGA** that performs per-pixel time-tagged photon counting, digital lock-in demodulation, and refresh-rate estimation, plus a **dual-core ESP32-S3** that runs the reconstruction state machine, manages SD-card capture, and provides a Bluetooth Low Energy command/exfil channel to the operator's phone. The whole assembly fits behind a 30 mm optical aperture in a rugged, pocket-sized enclosure.

### 1.1 Key Capabilities

- **Optical TEMPEST / van-Eck-by-light**: Reconstruct coarse display content (text blocks, window layouts, scroll/motion, full-screen color flashes) from light emanations sampled through glass, under doors, or from diffuse reflections — at distances from a few centimeters (probe against a bezel) up to tens of meters (window-to-window across a street).
- **LED-Borne Data Exfiltration Capture**: Demodulate data covertly (or unintentionally) modulated onto status LEDs of air-gapped equipment, switches, routers, and IoT devices — including low-rate (≤1 kbit/s) on/off-keyed channels that are invisible to the eye but trivially resolved by photon counting.
- **Refresh-Rate / Timing Fingerprinting**: Estimate the target display's exact pixel clock, horizontal/vertical sync rates, and backlight PWM dimming frequency to within ppm, producing a hardware fingerprint usable for device identification and for synchronizing the lock-in correlator.
- **Backlight PWM Keystroke Side-Channel**: Detect subtle backlight-brightness modulation induced by keyboard-activity power-rail coupling on some laptop panels, enabling keystroke-timing inference from pure optical observation.
- **Long-Baseline Photon Recording**: Stream time-tagged photon events to SD card at up to 40 Mevents/s for minutes-to-hours of offline analysis with Python/MATLAB lock-in and compressed-sensing reconstruction toolchains.
- **Covert Operation**: Fully passive — no RF emissions of its own in capture mode (BLE disabled during silent runs; data is buffered to SD and exfiltrated later via a separate "rendezvous" mode).

### 1.2 Target Audience

- **Red Team Operators**: Physical security assessments, "across-the-street" reconnaissance of target offices, air-gap bridge reconnaissance.
- **TEMPEST / EMC Engineers**: Pre-compliance screening of your own products for optical emanation leakage before formal SDIP-27/NATO TEMPEST evaluation.
- **Hardware Security Researchers**: LED-covert-channel detection, display-side-channel characterization, anti-eavesdropping countermeasure validation.
- **Academic Side-Channel Labs**: Reproducible platform for optical emanation research (a historically under-served side channel compared to power and EM).

---

## 2. Attack Surface & Threat Model

### 2.1 Target Emanation Sources

| Source | Optical Leakage Mechanism | Information Recovered |
|---|---|---|
| LCD / OLED monitors | Per-pixel brightness modulation riding on refresh scan; backlight PWM envelope; panel self-emission (OLED) | Coarse screen content, window layout, motion, text-block presence |
| Laptop panels | Backlight brightness modulation coupled from CPU/keyboard power rails | Keystroke timing, CPU activity bursts |
| Status / indicator LEDs | Intentional or unintentional on/off-keyed or PWM modulation | Covert exfil channels, device state, crypto-operation indicators |
| Fiber-coupled indicator pigtails | Light coupled from internal data links onto front-panel fiber | Raw link activity, link-state signaling |
| LED room lighting on shared circuits | Subtle brightness ripple from data-bearing loads on the same phase | Network/load activity inference |
| KVM / KVM-over-IP status LEDs | Activity blink patterns | Operator presence, switching events |

### 2.2 Attacker Capabilities (Assumed)

- **Line of diffuse optical path** to the target: direct view, through glass, under a door, or via a diffuse reflecting surface (wall, ceiling). A direct line of sight is *not* required — diffuse reflected light still carries the temporal modulation.
- **Physical proximity**: 0.1 m (aperture pressed to bezel) to ~30 m (window-to-window). Sensitivity scales with collected photon flux; a 30 mm aperture with SPAD array is usable to ~10 m on a bright monitor in a darkened room, further with a larger auxiliary lens.
- **No physical or electrical contact** with the target. The receiver is entirely passive optically.
- **Concealment**: device fits in a jacket pocket, hollowed book, or ceiling-tile mount; aperture can be aimed through a 5 mm pinhole.
- **Recording duration**: minutes-to-hours of raw photon time-tags on SD card; real-time reconstruction for the most cooperative targets.

### 2.3 Threat Model

**Attacker Goal:** Recover information-bearing content from a target's optical emanations without contact, network presence, or any emanation from the attacker that the defender could detect.

**Attacker Position:** The attacker is in the same room, an adjacent room, outside a window, or along a reflective surface path. The device is concealed and passively receiving.

**Defender Capabilities (countermeasures the device must overcome or that bound its utility):**
- **Optical shielding**: bezel light seals, polarizing privacy filters (attenuate but do not eliminate diffuse leakage — the device targets residual + reflected light).
- **Refresh-rate randomization / spread-spectrum pixel clocks**: degrades lock-in SNR; the device includes a robust refresh-rate estimator and a spread-spectrum-tolerant correlator mode, but energy is spread into noise.
- **LED data whitening / constant-current drive**: defeats LED-borne channel recovery.
- **Physical access controls** preventing the attacker from placing a receiver along an optical path.
- **TEMPEST-certified displays** with low-leakage backlights and shielded panel emissions.

**Representative Attack Scenarios:**
1. **Across-the-street screen recon**: From a parked vehicle, aim the aperture through a window at a target office monitor after dark. Recover window-layout changes and large text-block presence over a multi-minute integration, exfil later via BLE rendezvous.
2. **Under-the-door laptop recon**: Slide a 3 mm fiber pigtail under a server-room door aimed at a rack KVM status LED cluster. Demodulate the activity-blink channel to map operator sessions.
3. **Air-gap LED exfil detection**: During an authorized assessment, position the device 2 m from an air-gapped workstation suspected of hosting malware that modulates the HDD LED. Recover the covert channel bitstream to confirm the implant and characterize its protocol.
4. **Backlight keystroke timing**: From a reflective whiteboard opposite a target laptop, sample backlight ripple and infer keystroke timing bursts for later password-entropy analysis.
5. **Pre-compliance self-test**: TEMPEST lab screens a new product's front-panel LEDs for unintended data modulation before sending the unit for formal SDIP-27 evaluation.

### 2.4 Legal & Ethical Disclaimer (restated)

> **WARNING**: Aurora-Phantom is provided **solely** for authorized security research and testing of systems you own or are explicitly authorized in writing to assess. Passive optical interception of emanations from equipment you do not own can constitute unlawful interception of electronic communications and unauthorized access under computer-fraud, wiretap, and data-protection laws. The author (**jayis1**) assumes no liability for any misuse. Obtain written authorization before every deployment.

---

## 3. Hardware Specifications

| Subsystem | Component | Key Parameters |
|---|---|---|
| Photon sensor | 16×16 SPAD focal array (Hamamatsu/MPCC-class, integrated TDC) | 400–1000 nm, <100 ps timing jitter, ≤80 MHz/pixel, 256 pixels, on-die TDC |
| Optics | 30 mm aspheric collector lens + tunable liquid-crystal bandpass (450–950 nm) + 5 mm fiber pigtail input option | f/1.4 collector; LC bandpass FWHM ~20 nm, tunable in ~10 ms |
| FPGA (signal core) | Lattice iCE40UP5K in iCE40 UltraPlus family | 5.3k LUTs, 128 kB SPRAM, 8× DSP, 2× PLL; runs per-pixel photon time-tagging + lock-in |
| MCU (application) | Espressif ESP32-S3 (dual-core Xtensa LX7, 240 MHz) | BLE 5.0, Wi-Fi 4, 512 kB SRAM, vector extensions for reconstruction math |
| Timing reference | Si5351C clock generator locked to a 0.5 ppm TCXO | Generates FPGA sample clock, SPAD gate, and reference LO for lock-in |
| Storage | microSD card socket (UHS-I, up to 512 GB) via ESP32-S3 SDMMC | Raw photon-event stream + reconstructed frames |
| Connectivity | BLE 5.0 (command/exfil), USB-C (power, config, raw dump) | BLE only enabled in *rendezvous* mode to preserve RF silence during capture |
| Power | 2× 18650 Li-ion (parallel, 3500 mAh) + nPM1300 PMIC | ~8 h continuous capture; hot-swap USB-C charging |
| User interface | Single tactile button (mode/rendezvous) + RGB status LED (operator cues) + 1.3" OLED (optional, debug only) | No UI in deployed/cover mode — all control via BLE or pre-programmed mission file |
| Form factor | 95 × 55 × 22 mm machined-Al enclosure, anodized matte black; 30 mm optical aperture on one face | Pocketable; M3 tripod thread on base; pinhole lens cap with 5 mm aperture |

### Power Budget (typical capture mission)

| Rail | Consumers | Draw |
|---|---|---|
| 3V3 analog | SPAD bias controller, LC bandpass | ~45 mA |
| 1V2 core | iCE40UP5K | ~90 mA |
| 3V3 digital | ESP32-S3 (active), Si5351, SD card (write) | ~110 mA |
| 5V boost (LED/OLED) | Status RGB, optional OLED | ~15 mA |
| **Total** | | **~260 mA → ~13 h on 3500 mAh** (capture-only, BLE off) |

---

## 4. Architecture & Block Diagram

```
                     ┌────────────────────────────────────────────────────────┐
                     │                   OPTICAL FRONT END                     │
                     │   30 mm aspheric  ┌──────────┐   16×16 SPAD            │
                     │   collector lens ─▶│ LC band  │─▶ focal array + TDC    │
                     │  (f/1.4, 450-950nm)│  pass    │  (256 px, 80 MHz/px)   │
                     │   optional 5 mm   │ (tunable)│  photon time-tags out  │
                     │   fiber pigtail   └──────────┘                        │
                     └────────────────────┬───────────────────────────────────┘
                                          │ per-pixel time-tag stream
                                          ▼
   ┌─────────────┐  ref    ┌──────────────────────────────────────────────┐
   │  TCXO 0.5ppm│────────▶│   iCE40UP5K FPGA  — SIGNAL CORE               │
   │             │         │  ┌────────────┐ ┌──────────────┐ ┌─────────┐ │
   │  Si5351C    │─ clocks ▶│ │ per-pixel  │ │  digital      │ │ refresh │ │
   │  PLL synth  │         │  │ photon     │ │  lock-in      │ │ rate    │ │
   │             │         │  │ time-tag   │ │  demodulator  │ │ estimator│ │
   └─────────────┘         │  │ buffer     │ │  (I/Q per px) │ │ (FFT)   │ │
                          │  └─────┬──────┘ └──────┬───────┘ └────┬────┘ │
                          │        │               │              │      │
                          │   ┌────▼───────────────▼──────────────▼────┐ │
                          │   │   SPRAM frame buffer / IQ accumulator  │ │
                          │   └────────────────┬───────────────────────┘ │
                          └────────────────────┬──────────────────────────┘
                                               │ IQ + histogram stream
                                               ▼
   ┌──────────────────────────────────────────────────────────────────────┐
   │   ESP32-S3  — APPLICATION CORE                                       │
   │   ┌──────────────┐  ┌──────────────────┐  ┌────────────────────┐    │
   │   │ reconstruction│  │ refresh-rate /   │  │ mission manager     │   │
   │   │ state machine │  │ fingerprint SVC  │  │ (pre-program file)  │   │
   │   │ (compressed-  │  │                  │  │                    │    │
   │   │  sensing)     │  │                  │  │                    │    │
   │   └──────┬───────┘  └──────────────────┘  └─────────┬──────────┘    │
   │          │                                            │               │
   │   ┌──────▼───────┐  ┌──────────────┐   ┌─────────────▼─────────┐    │
   │   │ SD-card raw  │  │ BLE 5.0 link │   │ USB-C config / dump    │    │
   │   │ event logger │  │ (rendezvous) │   │                        │    │
   │   └──────────────┘  └──────────────┘   └────────────────────────┘    │
   └──────────────────────────────────────────────────────────────────────┘
          │ BLE                                  │ USB-C
          ▼                                      ▼
   ┌──────────────┐                    ┌──────────────────┐
   │ Companion app│                    │ Host PC /  offline│
   │ (React Native)│                   │ analysis toolchain│
   └──────────────┘                    └──────────────────┘
```

### Data-flow summary

1. **Photons → events**: each of 256 SPAD pixels produces a digital pulse on every detected photon; the on-die TDC stamps each pulse against the FPGA-supplied reference clock (sub-ns). The FPGA buffers time-tags per pixel in a rolling SPRAM ring.
2. **Events → IQ**: the FPGA lock-in demodulator mixes the per-pixel event train with a complex LO (reference frequency = candidate pixel/refresh clock) and integrates I/Q over a programmable window. This lifts the weak modulation out of shot noise.
3. **IQ → frames**: the ESP32-S3 consumes the I/Q accumulator output, runs a refresh-rate estimator (FFT on the aggregate rate stream) to lock the LO, then performs compressed-sensing / sparse reconstruction to produce a coarse 2-D image or a 1-D brightness-time trace.
4. **Frames → storage/exfil**: reconstructed frames and raw event snippets are written to SD; on rendezvous, frames are pushed over BLE to the companion app for visualization and export.

---

## 5. Firmware Design & Rationale

The firmware is split across two processors with sharply bounded responsibilities:

- **FPGA (iCE40UP5K)** — the *signal core*. Written in portable C that maps to a soft-CPU + hand-RTL boundary (the repo firmware exposes the firmware interface and a hardware-abstraction `registers.h` so the same C control code is portable across an iCE40 with a soft RISC-V (``lr16``) or a host simulation). Responsibilities: SPAD gating & TDC setup, per-pixel event buffering, lock-in demodulation, refresh-rate FFT, frame-buffer management.
- **ESP32-S3** — the *application core*. Responsibilities: mission scheduling, reconstruction state machine, SD logging, BLE rendezvous protocol, USB CDC console, power management.

### 5.1 Key design decisions

1. **Photon counting, not integrating ADCs.** An integrating ADC (like a camera sensor) integrates charge over an exposure and saturates in bright ambient light, destroying the weak modulation. SPADs count *individual* photons and are limited only by dead time (~10–20 ns), so the dynamic range against ambient background is recovered by lock-in correlation rather than by optical filtering alone. This is the enabling choice for the whole device.
2. **Lock-in demodulation in the FPGA.** The information-bearing modulation (refresh scan, backlight PWM, LED data) is narrowband; the background is broadband shot noise. A digital lock-in (complex multiply + integrate) is the optimal linear filter and is cheap in FPGA logic. Doing it in the FPGA, before the ESP32 ever sees the data, cuts the inter-processor bandwidth by 100–1000×.
3. **Refresh-rate estimator on the aggregate rate stream.** Before locking the LO we don't know the target's pixel clock. We compute an FFT of the total photon-rate time series (summed across pixels) in the FPGA; the strongest low-frequency line is almost always the vertical refresh rate or the backlight PWM. This gives the LO seed in <100 ms, then we track it with a PLL.
4. **RF silence during capture.** BLE is hard-disabled (PMIC rail off) in capture mode. The mission is pre-programmed via USB-C or a one-shot BLE session *before* the run; results are buffered to SD and exfiltrated in a separate *rendezvous* mode initiated by the operator button. This is a deliberate operational choice for covert use — the device is a passive optical receiver, and any RF emission during a run would defeat the purpose.
5. **Compressed-sensing reconstruction on the ESP32.** With only 16×16 spatial pixels we cannot image a 1920×1080 screen directly. Instead we exploit sparsity: screen content is largely blank background with localized bright regions (text, windows). We collect a time series of 16×16 low-res "views" modulated by the scan, and solve a sparse inverse problem (basis-pursuit / orthogonal-matching-pursuit lite) to localize bright regions to sub-superpixel accuracy. This will not recover readable 8-pt text across the street, but it reliably recovers window layouts, motion, large text blocks, and full-screen events — which is the realistic and still very useful threat.
6. **Mission files.** A mission is a small JSON/binary file loaded onto SD or pushed over BLE/USB that specifies target refresh-rate hint, integration window, bandpass setting, recording duration, and exfil policy. This lets an operator arm the device from the app, drop it in position, and walk away.

### 5.2 Firmware file map

| File | Role |
|---|---|
| `firmware/main.c` | Application-core entry: boot, mission load, state machine, BLE/USB/SD dispatch |
| `firmware/board.h` | Pin map, peripheral instances, board-level constants (author: jayis1) |
| `firmware/registers.h` | Memory-mapped register definitions for the FPGA signal core (author: jayis1) |
| `firmware/drivers/spad_driver.c/h` | SPAD array init, bias control, gating, TDC configuration |
| `firmware/drivers/optics_driver.c/h` | LC bandpass tuning, lens-cap / fiber input select |
| `firmware/drivers/lockin.c/h` | Lock-in demodulator control, LO frequency set, integration window |
| `firmware/drivers/refresh_estimator.c/h` | Aggregate-rate FFT + PLL refresh-rate tracker |
| `firmware/drivers/reconstruct.c/h` | Compressed-sensing / sparse reconstruction state machine |
| `firmware/drivers/storage.c/h` | SD raw-event + frame logging, ring buffer |
| `firmware/drivers/ble_link.c/h` | BLE 5.0 GATT rendezvous protocol + command parser |
| `firmware/drivers/usb_cdc.c/h` | USB-C CDC console for config and raw dump |
| `firmware/drivers/power.c/h` | nPM1300 PMIC control, rail gating for RF silence |
| `firmware/Makefile` | idf-style build (ESP32-S3) + FPGA image placeholder |

---

## 6. Companion Application

A React Native app (`app/`) provides the operator console:

- **ConnectScreen** — BLE scan & pair, device identity, battery, SD free space, mode (capture / rendezvous / standby).
- **MissionScreen** — compose a mission file: target refresh-rate hint, integration window, bandpass wavelength, recording duration, exfil policy (buffer-and-rendezvous vs. live BLE frames).
- **LiveSpectrumScreen** — real-time FFT of the aggregate photon-rate stream; operator watches for the refresh-rate line to lock.
- **ReconstructionScreen** — live coarse 16×16 → upsampled reconstruction preview + brightness time trace.
- **EventLogScreen** — raw event capture log, export to file.
- **StorageScreen** — SD file browser, download raw event files / reconstructed frames over BLE.
- **SettingsScreen** — device name, calibration, TCXO trim, optical bandpass calibration, firmware version.

The app uses a TypeScript BLE service module (`src/ble.ts`) and a small Redux-like store (`src/store.ts`).

---

## 7. Use Cases

### 7.1 Red Team — "across-the-street" office recon
Operator parks with a line of view to a target office window after dark. A pre-loaded mission aims the 30 mm aperture at the target monitor, integrates for 5–15 min, and buffers reconstructed window-layout frames to SD. Operator returns at rendezvous time, presses the button, and pulls frames over BLE.

### 7.2 Security Researcher — air-gap LED covert-channel characterization
During an authorized assessment of a suspected air-gap bridge, position Aurora-Phantom 1–3 m from the target's HDD/network LEDs. The lock-in demodulates the on/off-keyed channel; the researcher exports the bitstream to analyze the covert protocol and confirm the implant.

### 7.3 TEMPEST/EMC Lab — optical pre-compliance self-screening
A product team screens each new front-panel LED and display for unintended data modulation *before* paying for formal SDIP-27 evaluation. Aurora-Phantom's spectrum view flags any LED whose brightness carries data-correlated spectral lines.

### 7.4 Penetration Tester — under-the-door rack KVM activity mapping
A fiber pigtail slipped under a data-center door samples the rack KVM LED cluster; the device maps operator-session timing for use in social-engineering and physical-access planning.

### 7.5 Academic — optical side-channel research
A reproducible, open photon-counting platform for studying display optical emanations, LED covert channels, and backlight keystroke side-channels — a historically under-equipped side channel.

---

## 8. Limitations & Countermeasures

- **Spatial resolution is fundamentally limited** by the 16×16 array. Aurora-Phantom will not recover readable small text across the street. It recovers *structure* (window layouts, large text blocks, motion, full-screen flashes). Readable-text recovery needs a much larger array or a scanning aperture and is out of scope for v1.
- **Bright ambient light** raises shot-noise floor and reduces SNR; the LC bandpass + lock-in together recover ~40 dB of dynamic range, but daylight on a sunlit window may exceed this. Best performance is after dark or indoors.
- **Refresh-rate randomization / spread-spectrum pixel clocks** are an effective defense; the spread-spectrum-tolerant correlator mode helps but cannot fully recover lost SNR.
- **Polarizing privacy filters** attenuate direct leakage but not diffuse reflection; they raise, but do not eliminate, the attack difficulty.
- **LED constant-current drive / whitening** fully defeats the LED-channel recovery use case.

---

## 9. License & Author

All firmware, schematics, PCB layout, application code, and this documentation are authored by **jayis1** and released under **GPL-2.0**. No component of this design is authored by or affiliated with Nous Research. All author fields, copyright headers, and metadata credit **jayis1**.

> This design is a research artifact. Build, test, and use only on systems you own or are explicitly authorized in writing to assess.