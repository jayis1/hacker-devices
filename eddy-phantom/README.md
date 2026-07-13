# EDDY-PHANTOM — Electromagnetic Emanation Keyboard Reconnaissance Device

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Intercepting electromagnetic emanations from equipment you do not own or have not been authorized to assess may violate wiretapping statutes (e.g., 18 U.S.C. § 2511 ECPA), computer-fraud and abuse laws (18 U.S.C. § 1030 CFAA), data-protection regulations (GDPR, CCPA), and TEMPEST/COMSEC regulations in your jurisdiction. Captured keystrokes can expose credentials, personal data, and confidential communications belonging to third parties. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**Eddy-Phantom** is a pocket-sized, battery-powered **electromagnetic emanation keylogger** that reconstructs keystrokes from a target keyboard *without any physical connection to the target device and without any software implant on the host*. It exploits the fact that every keystroke on a USB or PS/2 keyboard produces a distinctive burst of electromagnetic emanation as the key matrix is scanned and the USB differential pair (D+/D−) transitions carry the HID report. These emanations propagate through the keyboard's unshielded cable and can be captured by a sensitive near-field magnetic probe placed in proximity — typically within 30–60 cm — to the cable or keyboard enclosure.

The device builds on published TEMPEST research (notably Martin Vuagnoux and Sylvain Pasini's 2008–2009 "Compromising Electromagnetic Emanations of Wired and Wireless Keyboards" work at École Polytechnique Fédérale de Lausanne) and advances the concept from a lab bench full of SDRs and Faraday cages into a self-contained, field-deployable unit with on-board DSP, a trained classifier, and a BLE C2 channel for real-time exfiltration.

| Capability | What it does | Why it matters |
|------------|-------------|----------------|
| **EM emanation capture** | A 4-element near-field H-field probe array feeds a dual-channel 16-bit ADC (2 MSPS) through a variable-gain low-noise amplifier. The STM32H743 captures triggered bursts and stores them in external SDRAM. | Replaces expensive lab SDR setups with a pocket device; works against unmodified keyboards with no software on the target. |
| **Keystroke classification** | An on-device lightweight CNN/MFCC classifier (running on the Cortex-M7 with CMSIS-NN) maps each captured emanation burst to a predicted scancode. Per-key models are trained during a calibration phase on identical-keyboard units. | Real-time key recovery without needing a full SDR pipeline or GPU inference; the operator gets decoded text on the phone app within seconds. |
| **Profile library** | A microSD card holds pre-trained emanation profiles for common keyboard controllers (Holtek HT82K629A, Cypress CY7C63743, NXP LPC11U6x, ASIX AX88772 USB hub passthrough, etc.). | Allows the operator to walk into a target environment, identify the keyboard controller from a quick fingerprint scan, and load the matching profile for immediate decoding. |
| **BLE C2 & exfiltration** | Decoded keystrokes are streamed over an encrypted BLE 5 link to a companion phone app, which displays the reconstructed text buffer, session logs, and confidence scores. | Real-time situational awareness for the red-team operator; no need to physically retrieve the device mid-operation. |
| **Raw burst capture** | Full-resolution raw ADC samples of every detected burst can be logged to microSD for later offline analysis and profile refinement. | Enables post-engagement forensic analysis and improvement of classifier models; supports research into new keyboard controller signatures. |

### What Eddy-Phantom is *not*

It is **not** a USB implant, BadUSB injector, or in-line tap. It makes **no electrical contact** with the target cable and requires **no software** on the target host. It is a purely passive, radiative/inductive sensor. It does not transmit any signal toward the target (the BLE C2 operates on a completely different band and is directed away from the target). This makes it invisible to all software-based detection — EDR, kernel lockdown, USB device policy enforcement, and host-side monitoring are all irrelevant because the device never appears on any host bus.

### Threat model & attack surface

| Trust boundary | Eddy-Phantom's position |
|-----------------|--------------------------|
| OS / kernel / EDR | **Bypassed.** No software runs on the target host; the device is physically separate. |
| USB device policy / port lockdown | **Bypassed.** The device is not a USB device of the target; it does not connect to any target port. |
| Cable shielding / USB HID encryption | **The attack surface.** Most consumer and office keyboards use unshielded or poorly-shielded cables. USB HID reports are sent in plaintext on the D+/D− pair. No standard keyboard encrypts its HID reports at the link layer. |
| Physical security perimeter | **The primary defense.** The device must be placed within near-field range (~30–60 cm) of the target cable. Physical access to the target's workspace (even briefly) is required to position the probe. |
| Keyboard controller diversity | **Secondary defense.** Different controller chips produce different emanation signatures. The device's classifier must be trained per-controller-family. Novel controllers may produce unclassifiable bursts until profiles are built. |
| Wireless detection (BLE) | **Mitigatable.** The BLE C2 link uses advertising on a randomized MAC with a non-standard GATT service UUID. A WIDS could detect an unknown BLE device in the vicinity, though attributing it to a keyboard emanation attack requires significant analysis. |

---

## 2. Hardware Specifications

### Core processing
| Component | Part | Role |
|-----------|------|------|
| MCU | STM32H743VIT6 (Cortex-M7 @ 480 MHz, 2 MB flash, 1 MB SRAM) | Main processor: ADC management, DSP pipeline, classifier inference, BLE stack, SD I/O |
| External SDRAM | IS42S32800G-6BLI 32 MB SDRAM | Raw burst capture buffer (each burst ≈ 4 KB samples at 16-bit) |
| External QSPI Flash | W25Q128JVSIQ 16 MB | Classifier model weights, profile library, firmware assets |
| BLE Module | NINA-B306 (Nordic nRF5340, BLE 5.2) | Encrypted C2 channel to operator phone app |
| MicroSD socket | Hirose DM3AT | Profile library, raw burst logging, session logs |

### Analog front-end
| Component | Part | Role |
|-----------|------|------|
| Probe array | 4× custom shielded loop H-field probes (10 mm, 20 mm, 40 mm, 80 mm diameter) | Multi-band near-field magnetic coupling; different loop diameters resonate at different frequency bands covering 1–500 MHz |
| VGA / LNA | AD8331 variable gain amplifier (0.5–48 dB, 1–500 MHz BW) | Gain is software-controlled; auto-ranging based on burst amplitude |
| Anti-alias filter | LTC1564-7 (8th-order elliptic lowpass, programmable cutoff) | Limits ADC input bandwidth to prevent aliasing; cutoff set by MCU |
| ADC | AD7606C-6 (6-channel, 16-bit, simultaneous sampling, up to 1 MSPS/ch) | Captures 4 probe channels + 2 aux channels simultaneously; SPI interface to MCU |
| Trigger comparator | TLV3201 ultra-fast comparator | Hardware trigger on burst onset; reduces MCU polling overhead |

### Power
| Component | Specification |
|-----------|---------------|
| Battery | 2× RCR123A (16340) Li-ion, 3.7 V, 750 mAh each (parallel) |
| Power management | TPS62743 buck converter (3.3 V rail, 95% efficiency), TPS7A4700 LDO (1.8 V analog rail, ultra-low noise) |
| Battery management | MCP73831 Li-ion charger (USB-C charging), MAX17048 fuel gauge |
| Battery life | ~4–6 hours continuous capture; ~12 hours in triggered/standby mode |
| Charging | USB-C (5 V, power-only — no data connection to target) |

### Form factor
| Attribute | Value |
|-----------|-------|
| Dimensions | 118 × 68 × 24 mm (approx. large keyfob / small powerbank) |
| Weight | ~180 g (including battery) |
| Enclosure | CNC-milled aluminum with non-conductive polymer front panel (aluminum provides partial Faraday shielding for the digital section, isolating it from the analog probe inputs) |
| Probe mounting | Detachable flex-PCB probe head with 4 loops on a hinged arm; folds flat against the body for carry, extends to position near target cable |
| Indicators | 2× LEDs (power/status, capture-active), 1× tactile button (arm/disarm), 1× OLED 0.96" 128×64 (status display) |
| Connectivity | USB-C (charging + firmware update), BLE 5.2 (C2), microSD (profiles/logs) |

### Block diagram

```
  ┌──────────────────────────────────────────────────────────────────┐
  │                        EDDY-PHANTOM BOARD                        │
  │                                                                  │
  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐                         │
  │  │Loop  │  │Loop  │  │Loop  │  │Loop  │   4-element H-field     │
  │  │10mm  │  │20mm  │  │40mm  │  │80mm  │   probe array           │
  │  └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘                         │
  │     │         │         │         │                              │
  │     ▼         ▼         ▼         ▼                              │
  │  ┌─────────────────────────────────────┐                        │
  │  │          4-channel VGA (AD8331)     │  ← gain control from MCU│
  │  └─────────────────┬───────────────────┘                        │
  │                    │                                             │
  │  ┌─────────────────▼───────────────────┐                        │
  │  │   Anti-alias filter (LTC1564-7)     │  ← cutoff from MCU      │
  │  └─────────────────┬───────────────────┘                        │
  │                    │                                             │
  │  ┌─────────────────▼───────────────────┐  ┌──────────────┐      │
  │  │     ADC AD7606C-6 (16-bit, 1 MSPS)  │─→│ TLV3201 trig │      │
  │  └─────────────────┬───────────────────┘  └──────┬───────┘      │
  │                    │ SPI                         │ trigger IRQ   │
  │  ┌─────────────────▼──────────────────────────────▼───────┐     │
  │  │              STM32H743VIT6 (Cortex-M7 @ 480 MHz)       │     │
  │  │  ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐ │     │
  │  │  │ ADC DMA │→ │ Pre-proc │→ │ MFCC +   │→ │Classify│ │     │
  │  │  │ capture │  │ + filter │  │ feature  │  │(CMSIS- │ │     │
  │  │  │         │  │          │  │ extract  │  │ NN)    │ │     │
  │  │  └─────────┘  └──────────┘  └──────────┘  └───┬────┘ │     │
  │  │                                                │      │     │
  │  │  ┌──────────┐  ┌──────────┐  ┌──────┐  ┌──────┴────┐ │     │
  │  │  │ SD card  │  │ QSPI     │  │ OLED │  │BLE (NINA) │ │     │
  │  │  │ (logs)   │  │ (models) │  │ disp │  │ C2 link   │ │     │
  │  │  └──────────┘  └──────────┘  └──────┘  └───────────┘ │     │
  │  └──────────────────────────────────────────────────────┘     │
  │         │ SPI              │ FMC                               │
  │  ┌──────▼──────┐    ┌──────▼──────┐                           │
  │  │ 32 MB SDRAM │    │  16 MB QSPI │                           │
  │  │ (burst buf) │    │  Flash      │                           │
  │  └─────────────┘    └─────────────┘                           │
  │                                                                  │
  │  ┌─────────┐    ┌───────────┐    ┌──────────┐                   │
  │  │ MCP73831│    │ MAX17048  │    │ TPS62743 │                   │
  │  │ charger │    │ fuel gauge│    │ buck 3V3 │                   │
  │  └────┬────┘    └───────────┘    └──────────┘                   │
  │       │ USB-C                                                    │
  └───────┼──────────────────────────────────────────────────────────┘
          ▼
      USB-C (charge only)
```

---

## 3. Firmware Architecture & Design Decisions

### Boot sequence
1. **Board init** — clock tree to 480 MHz, enable FMC for SDRAM, configure QSPI memory-mapped mode, initialize all GPIO, enable cache (I+D), set up NVIC priorities.
2. **Analog front-end init** — power on AD8331 VGAs, set LTC1564 filter cutoff to default 500 kHz, configure AD7606C SPI at 2 MSPS with 4 channels enabled, set TLV3201 comparator threshold.
3. **Storage init** — mount microSD via FatFS, load active profile from QSPI (or SD fallback), initialize SDRAM burst ring buffer.
4. **BLE init** — boot NINA-B306 via UART, establish encrypted link with pre-shared key, advertise with randomized MAC.
5. **Classifier init** — load CNN weights from QSPI into SRAM, initialize CMSIS-NN tensors, run a self-test inference on a stored reference burst.
6. **Arm** — enter triggered-capture mode: ADC DMA runs continuously in a circular buffer, comparator monitors the analog trigger line. When a burst is detected (amplitude exceeds threshold for >8 µs), the DMA engine captures a fixed window (2048 samples × 4 channels = 16 KB) into SDRAM, then raises an interrupt.

### DSP pipeline
Each captured burst passes through:
1. **DC removal** — subtract per-channel mean.
2. **Bandpass filter** — software FIR bandpass (50–350 kHz for USB D+/D− emanations), implemented with CMSIS-DSP `arm_fir_f32`.
3. **Envelope detection** — Hilbert transform via CMSIS-DSP FFT to get instantaneous amplitude envelope.
4. **Feature extraction** — 13 MFCC coefficients computed from a 256-point power spectrum of the envelope, plus 4 probe-channel amplitude ratios (spatial diversity features), plus burst duration and inter-burst timing → 19-dimensional feature vector.
5. **Classification** — a 3-layer fully-connected neural network (19 → 64 → 32 → N where N = number of scancodes in the active profile) implemented with CMSIS-NN `arm_fully_connected_s16`. Softmax output gives per-scancode probabilities; the argmax is the predicted key if confidence > threshold (default 0.7), otherwise logged as "unknown."
6. **Sequence assembly** — consecutive scancodes are assembled into a text buffer with shift-state tracking (modifier keys detected from their distinct emanation signatures). Backspace, enter, and special keys are handled.

### Design decisions
- **Why 4 probes of different diameters?** Near-field loop probes have a frequency response that depends on loop diameter — smaller loops are sensitive to higher frequencies, larger loops to lower frequencies. By using 4 diameters simultaneously, the device captures a broad spectral picture of each burst in a single snapshot, which provides richer features for classification than a single probe.
- **Why an ADC instead of an SDR?** The emanations of interest are baseband/low-IF (the USB full-speed 12 Mbps signal produces significant energy in the 1–20 MHz harmonic range, and the key matrix scanning produces bursts in the 50–200 kHz range). A 16-bit ADC at 1–2 MSPS with good dynamic range captures these directly without the complexity, power consumption, and cost of an SDR front-end.
- **Why CMSIS-NN instead of a K210/Accelerator?** The classifier is small (19 → 64 → 32 → ~110), totaling ~6K parameters. CMSIS-NN on the H7 at 480 MHz inference this in under 1 ms. Adding a separate AI accelerator would increase power consumption, board complexity, and cost for negligible benefit.
- **Why SDRAM for burst buffering?** Each burst is 16 KB; in a fast-typing scenario (120 WPM ≈ 10 keystrokes/sec), bursts arrive every ~100 ms. SDRAM provides a large ring buffer so that no bursts are lost even if the classifier or SD write temporarily stalls. 32 MB holds ~2000 bursts of raw data.
- **Why BLE instead of WiFi?** BLE 5.2 supports encrypted, low-power, low-latency communication that is sufficient for text-rate data (decoded keystrokes are tiny — a few bytes each). WiFi would consume more power, generate more RF noise (potentially interfering with the analog front-end), and be more detectable. BLE's randomized MAC and short advertising bursts also make it less conspicuous on wireless monitoring.

---

## 4. Companion Application (React Native)

The **Eddy-Phantom** companion app runs on iOS/Android and communicates with the device over BLE 5.2. Key screens:

| Screen | Function |
|--------|----------|
| **Dashboard** | Connection status, battery level, current profile, capture state (armed/disarmed), live keystroke feed |
| **Live Feed** | Real-time reconstructed text buffer with confidence scores per character; color-coded by confidence (green > 0.9, yellow > 0.7, red < 0.7) |
| **Session Log** | Timestamped history of all captured keystrokes; exportable as text/JSON; filter by time range and confidence |
| **Profiles** | Browse and select keyboard controller profiles; shows supported keyboard models; initiate on-device calibration |
| **Calibration** | Guided workflow to train a new profile: operator presses known keys on an identical-keyboard unit, device records reference bursts, trains per-key feature templates |
| **Raw Capture** | View recent raw burst waveforms (4-channel); download to phone for offline analysis |
| **Settings** | BLE pairing key, confidence threshold, capture window length, filter parameters, SD card management, firmware update |

### BLE GATT service
| UUID | Type | Purpose |
|------|------|---------|
| `0xF001` | Service | Eddy-Phantom C2 |
| `0xF002` | Char (Notify) | Live keystroke stream (scancode + confidence + timestamp) |
| `0xF003` | Char (Write) | Command interface (arm, disarm, set profile, set threshold) |
| `0xF004` | Char (Notify) | Status updates (battery, capture state, error codes) |
| `0xF005` | Char (Read/Write) | Raw burst request (request burst by index, receive in chunks) |
| `0xF006` | Char (Write) | Authentication (HMAC-SHA256 challenge-response with PSK) |

---

## 5. Use Cases

### Red team: credential capture in physical engagement
During an on-site red team engagement, the operator briefly places Eddy-Phantom on or near the target's keyboard cable (e.g., under a desk cable tray, behind a monitor stand, taped to the cable itself using the flexible probe arm). The device arms and begins capturing. As the target user logs in, types emails, enters credentials into internal systems, the device classifies each keystroke and streams decoded text to the operator's phone via BLE. The operator can be positioned up to ~30 m away (BLE range with line-of-sight). No software implant, no USB connection, no network access to the target is required.

### Security research: keyboard controller emanation profiling
A security researcher uses Eddy-Phantom to systematically profile the EM emanations of different keyboard controller families. The calibration mode records reference bursts for every key on a given keyboard, and the researcher studies which controller families produce the most classifiable signatures, which are the most resistant, and what shielding or PCB design changes reduce emanation strength. This produces actionable guidance for keyboard manufacturers to harden their products.

### TEMPEST / COMSEC assessment
A COMSEC assessor uses Eddy-Phantom to demonstrate to stakeholders that emanations from standard office keyboards are exploitable at practical distances. The device provides a tangible, repeatable demonstration: "we placed this device 40 cm from your keyboard cable and recovered 87% of typed characters with this profile." This drives investment in shielded cables, filtered connectors, and TEMPEST zoning.

### Penetration testing: password recovery from locked workstation
In a scenario where the target workstation is locked but a user is present and typing their password, Eddy-Phantom can capture the password keystrokes without needing to compromise the host. Combined with a profile matching the target keyboard's controller, the device can recover the password in real time. This is particularly relevant in environments with strong host-based security (full-disk encryption, EDR, secure boot) where traditional software keyloggers are impractical.

### Supply chain / procurement validation
An organization procuring keyboards for sensitive environments (SCIFs, trading floors, executive offices) can use Eddy-Phantom as part of a procurement validation process: test candidate keyboards for emanation strength and classifiability, and select models that are resistant to this attack class.

---

## 6. Limitations & Countermeasures

| Limitation | Detail |
|------------|--------|
| **Range** | Near-field coupling requires close proximity (~30–60 cm). Far-field emanation capture at multiple meters would require a much larger antenna array and sensitive receiver, which is beyond the scope of this device. |
| **Profile dependency** | The classifier must be trained per keyboard controller family. Encountering an unprofiled controller results in low-confidence or "unknown" classifications. |
| **Mechanical keyboard variety**** | Mechanical keyboards with switch-level controllers (e.g., QMK on ARM) produce different emanation patterns than matrix-scan controllers. Separate profiles are needed. |
| **Typing speed / overlap** | Very fast typists can produce overlapping bursts that the trigger logic may merge. The DSP pipeline includes burst-segmentation logic to handle moderate overlap, but extreme speeds reduce accuracy. |
| **Shielded cables** | Keyboards with properly shielded USB cables (foil + braid, properly terminated) produce emanations 20–40 dB lower, potentially below the noise floor at practical distances. |
| **Wireless keyboards** | Bluetooth/proprietary wireless keyboards are a different attack surface (RF protocol exploitation, not baseband emanation) and are not directly targeted by this device, though the probe array can capture 2.4 GHz leakage with an appropriate profile. |

### Defensive countermeasures
1. **Use shielded USB cables** with properly grounded shielding (foil + braid).
2. **Select keyboards with low-emanation controller designs** (spread-spectrum clocking, on-chip decoupling, minimal trace loop areas).
3. **Maintain physical security** — do not allow unauthorized devices within 1 m of keyboard cables.
4. **TEMPEST zoning** — position sensitive workstations away from areas accessible to visitors.
5. **RF emission monitoring** — deploy WIDS/RF monitoring to detect unexpected BLE or analog devices in secure areas.

---

## 7. File Structure

```
eddy-phantom/
├── README.md                  ← This file
├── firmware/
│   ├── Makefile               ← ARM GCC build system
│   ├── main.c                 ← Main application logic & state machine
│   ├── board.h                ← Board pin definitions & hardware constants
│   ├── registers.h            ← MCU register definitions
│   ├── link.ld                ← Linker script for STM32H743
│   ├── startup_stm32h743.s    ← Startup assembly
│   └── drivers/
│       ├── board_init.c       ← Clock, GPIO, FMC, QSPI, cache initialization
│       ├── adc_capture.c      ← AD7606C SPI driver, DMA burst capture
│       ├── dsp_pipeline.c     ← FIR bandpass, Hilbert envelope, MFCC extraction
│       ├── classifier.c       ← CMSIS-NN inference, scancode mapping
│       ├── probe_array.c      ← VGA gain control, auto-ranging, probe switching
│       ├── sdcard.c           ← FatFS microSD driver, burst logging
│       ├── sdram.c            ← SDRAM burst ring buffer management
│       ├── qspi_flash.c       ← QSPI memory-mapped flash for model weights
│       ├── ble_c2.c           ← NINA-B306 BLE UART protocol, GATT emulation
│       ├── oled.c             ← SSD1306 OLED status display
│       └── profile_mgr.c      ← Keyboard profile loading, calibration, storage
├── kicad/
│   ├── device.kicad_sch       ← Schematic
│   ├── device.kicad_pcb       ← PCB layout
│   └── device.kicad_pro       ← KiCad project file
└── app/
    ├── App.js                 ← React Native app entry
    ├── package.json           ← Dependencies
    ├── components/
    │   └── BLEManager.js      ← BLE connection & protocol management
    └── screens/
        ├── DashboardScreen.js ← Status & live feed overview
        ├── LiveFeedScreen.js  ← Real-time reconstructed text
        ├── SessionLogScreen.js← Timestamped capture history
        ├── ProfileScreen.js   ← Keyboard profile selection & calibration
        ├── RawCaptureScreen.js← Raw burst waveform viewer
        └── SettingsScreen.js  ← Device configuration
```

---

## 8. Ethical & Legal Notes

Eddy-Phantom is a **dual-use security research tool**. Its passive, non-contact nature makes it particularly powerful and correspondingly dangerous. The author (**jayis1**) publishes this design to advance the state of defensive knowledge: defenders cannot protect against an attack class they cannot demonstrate, and TEMPEST/emanation threats have historically received insufficient attention outside classified environments.

**By using or building this device, you affirm that:**
- You will only deploy it against equipment you own or have explicit written authorization to assess.
- You will not use it to capture credentials, communications, or data belonging to third parties without their consent.
- You understand that unauthorized interception of electromagnetic emanations may be a criminal offense in your jurisdiction.
- You will use the knowledge gained to improve defensive posture, not to cause harm.

---

*Author: jayis1 — Eddy-Phantom — Electromagnetic Emanation Keyboard Reconnaissance Device*
*License: GPL-2.0 — This design is free for research and educational use.*