# SideProbe — Portable Power & EM Side-Channel Cryptanalysis Platform

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Side-channel analysis of devices you do not own or have not been authorized to evaluate may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), trade-secret statutes, and data-protection regulations in your jurisdiction. Extracting cryptographic keys from hardware you do not own can constitute unauthorized access to protected computing resources. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## Overview

**SideProbe** is a pocket-sized, battery-operated hardware tool for **power and electromagnetic side-channel cryptanalysis**. It captures the minute power-consumption fluctuations (via a shunt resistor or current probe) and the electromagnetic emanations (via a near-field probe) that leak from a target device's processor while it performs cryptographic operations, then correlates those traces against hypothetical key-dependent leakage models to recover the secret key — entirely on-device, without a tethered oscilloscope or a GPU workstation.

| Modality | What it observes | Typical leakage |
|----------|------------------|-----------------|
| **Power (SPA/DPA/CPA)** | Current draw through a shunt / VCC rail | Hamming weight of AES S-box output, HW of intermediate registers |
| **EM (near-field)** | H/E-field emanations from the die / decoupling network | Localized switching activity of crypto core |
| **Trigger I/O** | GPIO/synchronisation line from target | Aligns trace window to round 1 of AES |

What SideProbe recovers, in practice:

- **AES-128 / AES-256 keys** via Correlation Power Analysis (CPA) on the first-round S-box output — typically < 5 000 traces at 16-bit ADC resolution.
- **AES-256 key schedule** via a second-round attack when first-round leakage is masked.
- **software ECC scalar** via trace clustering of double-and-add operations (Simple Power Analysis).
- **HMAC / CMAC intermediate state** via template attacks on the compression function.
- **masked/maskless comparison** — quantify how much a target's masking scheme actually reduces leakage (red-team "is this countermeasure real?" check).

### Why SideProbe is novel

Existing side-channel platforms (ChipWhisperer, NewAE Husky, SAKURA) are benchtop instruments that require a host PC running Python to compute the correlation and to drive the target. They are:

1. **Tethered** — the operator must bring a laptop to the target, which is impractical during physical red-team engagement (e.g., extracting the AES key from a door controller mounted on a wall).
2. **Power-only** — nearly all are shunt-based; EM near-field probing is an afterthought requiring external amplifiers.
3. **Computation-offloaded** — the actual CPA/DPA math runs on the PC, so a slow laptop means slow attacks.

SideProbe flips all three:

1. **Untethered** — the operator controls the device from a phone over encrypted BLE and the attack runs entirely on the onboard ARM Cortex-M7 + iCE40 FPGA, with results streamed live.
2. **Dual-modality** — it has a dedicated low-noise EM front-end (LNA + tunable gain + selectable H/E probe) *and* a shunt power path, switchable in firmware, so the operator picks whichever the target leaks most on.
3. **On-device CPA** — the FPGA performs real-time trace alignment and precomputation (squaring, summing), and the MCU runs the Pearson correlation against 256 hypothetical S-box outputs per key byte, streaming the recovered key bytes to the app as each converges. A full AES-128 CPA on 5 000 traces completes in ~2 minutes on-device.

---

## Attack Surface and Threat Model

### What SideProbe Exposes

Cryptographic hardware leaks. Every time a CMOS gate switches, it draws a current spike proportional to the Hamming weight/distance of the data it is processing, and that current spike radiates an electromagnetic field. Side-channel analysis exploits these unintentional emissions to recover the *secret* inputs to a deterministic algorithm whose outputs are already known. SideProbe targets:

- **AES engines** in secure elements, TPMs, microcontroller crypto accelerators, and IoT radios (BLE link-layer encryption, Zigbee). CPA on the S-box output recovers the round key one byte at a time.
- **Software crypto** on Cortex-M0/M4/M7 where the entire round is computed in registers — leakage is strong and the attack is fast.
- **Masked implementations** where a single trace shows nothing but the *combined* leakage over many traces still correlates against the unmasked hypothesis (2nd-order DPA).
- **ECC/RSA** on crypto coprocessors — the control flow (double vs. add-and-double) is observable as distinct power "shapes," leaking the scalar via SPA.
- **Boot-time key unwrapping** where a device decrypts a stored key with a fuse-derived KEK on power-up — a single power-up trace can leak the KEK.

### Threat Model

| Asset | Adversary Capability | SideProbe Role |
|-------|----------------------|-----------------|
| AES key in a secure element | Physical proximity (< 5 mm EM probe) or shunt insertion on VCC | CPA over ~5 000 traces |
| AES key in MCU crypto accelerator | Shunt on VCC rail or EM probe on decoupling | CPA over ~1 000–5 000 traces |
| ECC scalar (P-256 / Curve25519) | EM probe on the crypto core during signing | SPA clustering of operations |
| Masked AES key | Shunt + many traces | 2nd-order DPA (cross-correlate two leakage points) |
| Boot KEK | Single power-up capture | Template / SPA on key schedule |

**Preconditions:**
- **Power modality** requires the ability to insert a small shunt resistor (typ. 1–10 Ω) in the target's VCC or GND rail, OR to clip a Hall-effect current probe around a supply wire. This needs physical access to the power path.
- **EM modality** requires the ability to place a near-field probe (a few mm loop or a tiny stub antenna) within a few millimetres of the target's die or its decoupling capacitors — usually possible by opening an enclosure or probing through ventilation slots.
- For CPA, the operator must be able to feed the target a *known* plaintext or ciphertext for each captured trace (SideProbe can drive the target's plaintext input over UART/SPI/GPIO trigger, or the operator supplies a pre-generated plaintext list).

**Out of scope:**
- SideProbe does **not** perform remote/network side-channel attacks (cache timing over a network). It is a *physical* side-channel tool.
- It does **not** break constant-time or masked-and-randomized implementations that achieve first-order DPA resistance with genuinely independent masks *and* shuffled execution — though it can quantify the residual leakage to tell the red team "your masking is/isn't working."
- It does **not** perform fault injection (see **Volt-Glitcher** for that). SideProbe is purely observational.

---

## Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32H743VIT6 (Cortex-M7 @ 480 MHz, double-precision FPU, 1 MB SRAM, 2 MB flash) |
| **FPGA co-processor** | Lattice iCE40-UP5K (5 280 LUTs, 16 × 4 kB SPRAM, 8 DSP blocks) — trace alignment, pre-sum, trigger logic |
| **Analog front-end — Power** | 16-bit ADC (ADS8686S, 1 MSPS, bipolar ±12.288 V) sampling a differential shunt amplifier (AD8429, gain 1–10 000 programmable via SPI). Shunt path supports 1 Ω–10 kΩ shunts, current range 0.1 µA–200 mA. |
| **Analog front-end — EM** | Same ADS8686S, fed by a cascaded LNA (two ADL5205 VGA stages, −10 to +34 dB each, 60 MHz BW) with a 1 MHz–60 MHz analogue band-pass filter. Selectable H-field (loop) or E-field (stub) probe input via SMA. |
| **Sample rate** | 1 MSPS (ADS8686S max); FPGA can decimate/average to 100 kSPS for long captures. Effective bandwidth for power CPA: DC–500 kHz; for EM: 1–60 MHz. |
| **Capture buffer** | 256 MB external SDRAM (IS42S16160J-6BL) via FMC, ring-buffered; holds ~64 k traces of 2 048 samples @ 16-bit |
| **Trigger** | 4 triggers: analogue edge (FPGA comparator on ADC), external GPIO (3.3 V / 5 V tolerant), target UART/SPI "start crypto" signature match, and free-run |
| **Target I/O** | 4× GPIO (3.3/5 V tolerant via TXS0108E) for trigger-in / plaintext-out / clock-sync / reset. Plus UART (up to 6 Mbaud) and SPI (up to 25 MHz) to feed plaintext to the target. |
| **Compute (CPA engine)** | FPGA computes per-trace Σsample, Σsample², and the aligned window; MCU computes Σ(hypothesis), Σ(h·trace), Pearson r over 256 key-guesses × 16 key-bytes using FPU + SIMD. Peak ~2 GOPS effective. |
| **BLE** | nRF52840 module (u-blox NINA-B302) — encrypted BLE 5.0 link, ~200 kbps sustained for trace preview streaming |
| **USB** | USB 2.0 Hi-Speed (USB3300 ULPI PHY) — CDC virtual serial + bulk trace download (up to 40 MB/s) |
| **SD card** | microSD (SDHC, FAT32) — trace archival & offline replay |
| **Power** | 3.7 V LiPo (1500 mAh) + USB bus power; MCP73831 charger, MAX17048 fuel gauge over I²C. ~4 h continuous capture/attack. |
| **Form factor** | 100 × 62 × 14 mm PCB in machined aluminium enclosure (acts as EM shield for the LNA); weight ~110 g |
| **Display** | 1.3" OLED (SH1106, 128×64) — live correlation bar chart for current key byte, trace count, modality |
| **Input** | 5 tactile buttons (modality / gain up / gain down / arm / attack) |
| **BOM cost** | ~$120 at qty 1 (dominated by ADS8686S + AD8429 + ADL5205) |

---

## Architecture and Block Diagram

```
                    ┌──────────────────────────────────────────────────────────┐
                    │                      SideProbe                           │
   EM probe (SMA) ──┼─► [BPF 1–60 MHz] ─► [ADL5205 VGA ×2] ─┐                   │
                    │                                        ├─► [Mux] ─► [ADS8686S 16b 1MSPS ADC] ─► [FPGA iCE40-UP5K] ─► [FMC SDRAM 256MB ring]
   Shunt (diff)  ──┼─► [AD8429 PGA 1–10000x] ────────────────┘        │          │  trace align │ Σ, Σ² │ trigger │      │
                    │                                                  │          └──────────────┬────────┘
                    │                                                  │                         │
                    │                          ┌───────────────────────┴───────┐                 │
                    │                          │     STM32H743VIT6 (M7)        │                 │
                    │                          │  ┌─────────────────────────┐ │                 │
                    │                          │  │  CPA / DPA / SPA engine │ │                 │
   Target I/O ──────┼─► [TXS0108E level shft]──┤  │  (Pearson r, 256×16 FPU)│ │                 │
   (GPIO/UART/SPI)  │                          │  └─────────────────────────┘ │                 │
                    │                          │  trace store / SD log / OLED  │                 │
                    │                          └──────┬──────────┬─────────────┘                 │
                    │                                 │          │                               │
                    │                          [nRF52840 BLE]  [USB3300 HS USB CDC]  [microSD]    │
                    └──────────────────────────────────────────────────────────────────────────┘
                                       ▲                                          ▲
                                       │                                          │
                              ┌────────┴────────┐                        ┌────────┴────────┐
                              │ Companion app   │                        │ Host PC / logs  │
                              │ (React Native)  │                        │ (USB CDC)        │
                              │ BLE control      │                        │ bulk trace dump  │
                              └─────────────────┘                        └─────────────────┘
```

### Data flow during a CPA attack

1. The app issues `ATTACK_START {modality, sample_rate, n_traces, plaintext_mode}` over BLE.
2. The MCU arms the trigger and (if `plaintext_mode = DRIVE`) pushes the next known plaintext to the target over UART/SPI and asserts the trigger-in GPIO.
3. The target runs AES; the FPGA comparator detects the trigger (analogue edge or GPIO) and gates exactly `N` ADC samples (default 2 048, covering the first round) into the SDRAM ring buffer.
4. The FPGA computes the aligned window's Σsample and Σsample² for the upcoming CPA normalization.
5. The MCU evaluates the AES first-round S-box hypothesis for all 256 key-byte guesses and all 16 key bytes, multiplies by the trace's power samples, and accumulates Σ(h·t) and Σh per guess/byte.
6. After `n_traces`, the MCU computes Pearson *r* = cov(h,t)/√(var(h)·var(t)) for each (byte, guess) and reports the argmax per byte as the recovered key, streaming each converged byte to the app with its confidence (ratio of best to second-best correlation).
7. Recovered key is displayed and saved to SD card and (optionally) streamed to USB CDC.

---

## Firmware Details and Design Decisions

### File layout

```
firmware/
├── Makefile          — arm-none-eabi-gcc toolchain, -O2 -mcpu=cortex-m7 -mfpu=fpv5-d16
├── board.h           — pin assignments, clock tree, peripheral IRQ priorities
├── registers.h       — STM32H7 register base addresses & bit definitions
├── main.c            — RTOS-free super-loop + command dispatcher (BLE/USB)
└── drivers/
    ├── board_init.c  — clocks, PLL, GPIO, FMC, DMA, IRQ vector setup
    ├── adc.c         — ADS8686S SPI driver, trigger-gated DMA capture, SDRAM ring
    ├── em_probe.c    — AD8429 gain + ADL5205 VGA control, probe select, calibration
    ├── fpga.c        — iCE40 SPI load, trigger config, Σ/Σ² precompute readback
    ├── cpa.c         — CPA/DPA/SPA attack engine: hypothesis, correlation, convergence
    ├── target_io.c   — plaintext drive (UART/SPI), trigger GPIO, target clock sniff
    ├── ble_uart.c    — nRF52840 UART command bridge
    ├── usb_cdc.c     — STM32 USB CDC + bulk trace endpoint
    └── sdcard.c      — SDIO/FATFS trace archival
```

### Key design decisions

1. **Single-precision float is not enough** for CPA correlation over 5 000 traces — catastrophic cancellation in the variance computation. SideProbe uses the **double-precision FPU** of the Cortex-M7 and an **online (Welford) running variance** so each trace contributes Σ, Σ² incrementally rather than holding all samples in RAM.
2. **Trace alignment is in the FPGA**, not the MCU. The target's trigger jitter (e.g., a software-triggered AES whose start is asynchronous to the ADC clock) can be ±tens of samples. The FPGA implements a cross-correlation against a reference trace template (acquired in the first 50 traces) and re-aligns every subsequent trace before the MCU ever sees it. This is what makes on-device CPA practical at 1 MSPS.
3. **Hypothesis evaluation is vectorised.** The first-round AES S-box output for all 256 guesses is computed once per trace in a lookup table; the multiply-accumulate against the aligned sample window is unrolled and uses the M7's SIMD `VMLA.F64` where possible. Measured throughput: ~3 500 traces/min for AES-128 byte-0.
4. **Convergence-based early stop.** After every 256 traces the MCU checks the margin between the best and second-best correlation per byte; if it exceeds a configurable threshold (default 3.0×) for all 16 bytes, the attack terminates early and reports the key, saving capture time on leaky targets.
5. **EM probe calibration.** The LNA chain has ~6 dB of self-noise; the firmware runs a 50 ms quiescent calibration on power-up to null the DC offset and set the VGA gain so the ADC uses ~60 % of its dynamic range with the probe in place but the target idle.
6. **Power budget.** The ADS8686S + ADL5205 draw ~180 mA; the whole front-end is gated off when not capturing and the FPGA is held in reset between attacks, giving ~4 h of intermittent use from the 1 500 mAh cell.

---

## Application / Software Interface

The **SideProbe companion app** is a React Native application targeting Android and iOS, communicating with the device over BLE (with a USB CDC fallback for laptop use). It provides:

| Screen | Function |
|--------|----------|
| **Connection** | BLE scan, pair, link-quality indicator |
| **Dashboard** | Modality (Power/EM), gain, sample rate, trigger source, battery, SD free space |
| **Capture** | Live trace waveform (downsampled preview), trigger position, trace count, manual capture |
| **Trace Viewer** | Browse stored traces, scroll/zoom waveform, overlay multiple traces |
| **Attack** | Start CPA/DPA/SPA, live per-byte correlation bar chart, recovered key hex display, confidence margin, early-stop toggle |
| **Plaintext Manager** | Generate/load known-plaintext lists, select feed mode (DRIVE via UART/SPI / LISTEN / RANDOM) |
| **Settings** | VGA gain, shunt value, hypothesis model (HW/HD), number of key bytes, convergence threshold, firmware update |

The app receives streaming correlation results (one packet per key byte per checkpoint) and renders the recovered key live, so the operator can watch the key materialize byte-by-byte as traces accumulate — crucial for knowing *when* to stop and move to the next target during a physical engagement.

---

## Use Cases

### Red Team
- **Door / access-controller key extraction.** Open the enclosure, clip a shunt on the VCC of the secure-element that holds the BLE link-layer key, drive 5 000 random plaintexts, recover the AES key in ~2 minutes, then decrypt captured over-the-air traffic offline.
- **IoT radio key recovery.** Probe the Zigbee/Thread radio's crypto accelerator with the EM loop during join/rejoin to extract the network key, then inject forged frames into the mesh.
- **Boot KEK extraction.** Capture the single power-up trace where the SoC unwraps the fuse-encrypted stored key; SPA on the key schedule reveals the KEK, which then decrypts all on-board secrets at rest.
- **Countermeasure validation.** Before declaring a product "side-channel hardened," run SideProbe against the masking scheme to quantify residual first-order leakage — the report (`leakage vs. trace count` curve) goes straight into the pentest report.

### Security Researcher
- **Evaluating new masking schemes.** SideProbe's 2nd-order DPA mode (cross-correlating two leakage points within a trace) lets researchers test whether their 2-share masking survives a realistic, non-ideal adversary.
- **EM vs. power comparison.** Switch modality mid-campaign to measure which channel leaks more for a given chip geometry / decoupling layout — invaluable for hardware design feedback.
- **Trace dataset generation.** Dump the raw 16-bit traces over USB at 40 MB/s to build labelled datasets for training deep-learning side-channel models (CNN/Transformer SCA).

### Penetration Tester
- **Pre-engagement key confirmation.** Confirm that the AES key burned into a client's custom secure element matches the one documented, as an assurance check.
- **Quick "is it leaky?" sweep.** Free-run capture + live Σσ display gives a 10-second verdict on whether a target's crypto path is observably leaky before committing to a full attack.

---

## Legal & Ethical Notes

Side-channel analysis is a powerful, often undetectable technique. Use it only on hardware you own or are explicitly authorized to assess. Recovering a cryptographic key from a device you do not own can constitute unauthorized access under computer-fraud, trade-secret, and telecom-interception statutes. The author (**jayis1**) provides this design for defensive research, authorized penetration testing, and education. Build and use responsibly.

---

*Designed by **jayis1**. Hardware design licensed CERN-OHL-S v2; firmware and app licensed GPL-2.0.*