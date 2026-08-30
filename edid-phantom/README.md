# EDID Phantom

**Author:** jayis1  
**Copyright:** Copyright (c) 2026 jayis1  
**Status:** Conceptual hardware platform with compile-ready firmware simulator, companion app, and KiCad starting design  
**Use restriction:** Authorized security research, defensive validation, and sanctioned red-team work only

## Legal and Ethical Disclaimer

EDID Phantom is designed exclusively for lawful, authorized assessment of systems that you own or have explicit written permission to test. It is intended for display trust-boundary research, AV security validation, kiosk hardening, conference-room attack-surface mapping, and controlled red-team exercises. It must **not** be used against third-party environments, consumer devices, production meeting spaces, protected content workflows, or any infrastructure where you lack authorization. Manipulating display buses can cause denial of service, loss of video, failed training, undesirable UI navigation through CEC, and operational disruption. Use slowly, document every change, preserve rollback capability, and keep live activity within the scope approved by the system owner.

---

## 1. Purpose and Overview

EDID Phantom is a novel inline display-security research appliance built to expose an under-examined trust boundary: the signaling relationship between a video source and the display it believes it is talking to. Most organizations defend networks, identities, and endpoints, but the source-to-display path is often treated as an inert cable. In reality, that path contains a meaningful control plane: DDC/I2C transactions, EDID discovery, hot-plug detect behavior, +5V signaling, optional CEC messaging, repeater semantics, timing quirks, and policy assumptions embedded in display enumeration logic.

EDID Phantom sits between a source device and a monitor, capture card, projector, matrix switch, conference-room appliance, digital signage controller, kiosk computer, or secure workstation docking path. It can passively observe the negotiation, proxy it with timing control, or emulate a sink while reshaping capability advertisements. Its mission is not to break cryptography or capture protected content. Instead, it helps researchers understand how systems behave when the display identity changes, when capability descriptors become inconsistent, when hot-plug timing is perturbed, when CEC frames appear in unexpected sequences, or when display-dependent policy engines make unsafe assumptions.

The practical security value is broader than it first appears. Many enterprise workflows trust that the attached display is benign and static. Kiosk stacks may unlock hidden menus when a “maintenance display” is detected. Thin clients may enable alternative rendering paths when HDR or audio-capable sinks are present. Conference-room appliances may expose pairing prompts, screen-sharing state transitions, or power management logic through CEC. Developer workstations may leak workflow metadata through repeated EDID probes or screen topology changes. Security teams need a way to explore those behaviors in a controlled, instrumented, rate-limited manner.

EDID Phantom addresses that need with a hybrid architecture. A microcontroller manages scenario logic, safety policy, telemetry, and app integration. A small FPGA handles deterministic bus observation and precise edge timing for HPD and DDC mediation. An external EEPROM enables baseline sink cloning and rollback. An ESP32-C6 sideband module provides local BLE/Wi-Fi control without requiring the assessment target to trust or install anything. The result is a pocketable board that behaves like an inline display interposer, protocol microscope, and controlled perturbation engine.

What makes EDID Phantom novel is its focus on the **display trust boundary as a security control surface** rather than merely a compatibility or AV debugging problem. Existing tools often concentrate on content capture, video conversion, or cable testing. EDID Phantom instead treats the sink identity and control-plane choreography as a research domain in its own right.

---

## 2. Device Concept Summary

At a high level, EDID Phantom provides four operating styles:

1. **Monitor mode** – passive observation only, used to capture DDC and CEC behavior with no intentional intervention.
2. **Inline proxy mode** – forwards the path while selectively altering timing, sink descriptors, and event pacing under policy control.
3. **Ghost sink mode** – emulates a display for source-side behavior validation without requiring a real monitor.
4. **Hardened lab mode** – allows aggressive but bounded testing in training environments with thermal, timing, and policy guardrails.

The device is meant for:

- Red teams evaluating conference-room, signage, kiosk, and executive AV assumptions.
- Hardware security researchers studying source parsing behavior for EDID and DDC edge cases.
- Embedded developers validating how industrial HMIs, thin clients, or GPU firmware react to malformed-but-checksummed capability blocks.
- Defensive teams building detection and hardening guidance for display-dependent workflows.

It is intentionally **not** positioned as a covert surveillance recorder, DRM bypass platform, or generalized video capture product.

---

## 3. Attack Surface and Threat Model

### 3.1 Primary attack surface under study

EDID Phantom explores the following surfaces:

- **EDID parsing logic** in operating systems, GPUs, BIOS/UEFI, kiosk launchers, meeting-room endpoints, and signage players.
- **DDC timing behavior**, including retry handling, offset assumptions, bus timeout sensitivity, and fallback logic.
- **HPD edge handling**, such as re-enumeration storms, display disconnect/reconnect logic, and profile switching tied to hot-plug events.
- **CEC control flows**, including power state, source selection, menu navigation, and display wake behavior in shared AV environments.
- **Sink identity trust**, where source-side software enables extra features or hidden modes because a display claims certain capabilities.
- **Operational leakage**, where repeated bus activity reveals when screen-sharing, lock-screen transitions, BIOS screens, or GPU mode changes occur.

### 3.2 Threat scenarios

EDID Phantom is designed to help model and test scenarios such as:

- A digital-signage controller that enters maintenance mode when it sees a specific vendor block or audio profile.
- A conference-room appliance that reacts to CEC traffic in ways that disclose source priority or wake state.
- A kiosk stack that briefly exposes admin tooling after unexpected display re-training.
- A workstation that changes rendering path, privacy-filter mode, or audio routing depending on sink capabilities.
- An embedded Linux HMI that crashes or misconfigures itself when faced with unusual but checksum-valid EDID fields.
- A secure space where operators want to know whether display-side metadata or control flows could be abused to trigger state transitions.

### 3.3 Threat model assumptions

- The operator has physical access to the cable path or a sanctioned inline insertion point.
- The target trusts standard HDMI/DP negotiation and does not authenticate sink identity.
- The goal is controlled behavior study, not content decryption or content theft.
- The operator wants deterministic rollback and audit logging.
- Disturbance must be minimized outside approved lab or exercise windows.

### 3.4 Safety boundaries

EDID Phantom includes deliberate guardrails:

- Pulse-count and spacing limits for HPD manipulation.
- Profile-level restrictions on whether CEC or EDID mutation is allowed.
- Current/thermal monitoring to disable more aggressive behaviors when margins narrow.
- Baseline EDID preservation for quick recovery.
- App-visible risk labels to prevent accidental escalation.

---

## 4. Hardware Specifications

### 4.1 Core processing

- **Primary MCU:** Raspberry Pi RP2350B
  - Chosen for dual-core flexibility, strong GPIO handling, deterministic low-latency control, and excellent developer accessibility.
  - Handles policy engine, logging, scenario management, BLE/Wi-Fi bridge coordination, and safe state transitions.

- **Timing / bus fabric:** Lattice iCE40UP5K FPGA
  - Chosen for low power, fast bring-up, open tooling friendliness, and suitability for bus observation and timing-sensitive forwarding.
  - Monitors DDC activity, timestamps events, supports precise HPD shaping, and allows deterministic sideband measurements.

### 4.2 Radio and control plane

- **ESP32-C6-WROOM-1**
  - BLE for local operator tablet or phone control.
  - Wi-Fi for lab integration, telemetry relay, and optional secured control dashboard.
  - Kept logically separate from the target bus to avoid introducing host-side trust requirements.

### 4.3 Memory and storage

- **24LC256 EEPROM** for baseline EDID image caching and rollback experiments.
- QSPI flash on the MCU module for profile storage, event ring buffer snapshots, and signed firmware bundles.

### 4.4 Display-side interfaces

- **HDMI source-side connector** and **HDMI sink-side connector** as the primary reference implementation.
- Optional future board spin with **USB-C DisplayPort Alt Mode pass-through mezzanine**.
- ESD / level-protection stage using a device in the **TPD12S016** class.

### 4.5 Sensors and instrumentation

- Board temperature sensing via MCU ADC path.
- Current-sense amplifier on the 3V3 rail.
- HPD and +5V presence monitoring.
- DDC bus activity counters.
- Optional TMDS clock detect for future rev validation of training transitions without attempting content capture.

### 4.6 Power

- USB-C debug/power input for bench use.
- Source-side +5V sense only; not relied on as the board’s sole power source.
- Onboard 3V3 and 1V2 regulation for MCU, radio, and FPGA domains.
- Optional single-cell LiPo support in a future revision for untethered field characterization.

### 4.7 Form factor

- Approximate inline board size: **150 mm x 125 mm** in the current conceptual layout.
- Ruggedized future target: small billet enclosure with short pigtails or panel-mounted HDMI connectors.
- Designed to live in a conference-room kit, kiosk validation bag, or lab instrumentation drawer.

---

## 5. Architecture and Block Diagram

```text
     HDMI Source
         │
         │   DDC / HPD / CEC / +5V sense
         ▼
   [ Protection + Level Conditioning ]
         │
         ├──────────────► [ RP2350B MCU ] ───────► Policy Engine / Logging / Profiles
         │                         │
         │                         ├────────────► BLE / Wi-Fi bridge control
         │                         │               via ESP32-C6
         │                         │
         │                         └────────────► EEPROM baseline EDID store
         │
         └──────────────► [ iCE40UP5K FPGA ] ───► Deterministic DDC observation
                                   │              HPD timing, trigger windows
                                   │
                                   ▼
                              HDMI Sink
```

### 5.1 Functional partitioning

The RP2350B owns high-level decision making. It evaluates profile policy, rates actions, logs events, prepares mutated EDID images, and synchronizes state to the companion app. The FPGA sits lower in the stack and deals with timing-sensitive path observation. This split keeps the architecture realistic: flexible firmware for scenario logic, deterministic hardware for protocol edges.

### 5.2 Why the split matters

Display negotiation edge cases often happen at uncomfortable timing boundaries. A pure MCU approach can work for many tasks, but inline bus shaping becomes easier to reason about if tight path observation and edge generation are handled in programmable logic. Conversely, building everything in FPGA would make app-driven experimentation slower and less operator-friendly. EDID Phantom combines both.

---

## 6. Firmware Details and Design Decisions

The included firmware is a compile-ready simulator written in C and organized like a portable embedded project. It intentionally mirrors how production firmware would be structured:

- `main.c` – boot, register emulation, simulation loop, reporting.
- `drivers/log.*` – bounded event ring buffer.
- `drivers/profile.*` – scenario catalog and profile lookup.
- `drivers/ddc.*` – EDID image creation, mutation, transaction capture.
- `drivers/cec.*` – CEC observation and guarded transmit logic.
- `drivers/radio.*` – BLE/Wi-Fi-side telemetry transcript generation.
- `drivers/safety.*` – thermal/current derating and HPD policy enforcement.
- `board.h` / `registers.h` – shared definitions and simulated hardware map.

### 6.1 Firmware design philosophy

The firmware is designed around **bounded experimentation**. Every active behavior is tied to a profile. The profile determines whether EDID mutation is even allowed, whether HPD pulses can be emitted, how far CEC automation may go, and what timing spacing is acceptable. This is deliberate: the device should make unsafe behavior harder to trigger casually.

### 6.2 EDID mutation strategy

EDID Phantom mutates selected fields while preserving a valid checksum. This allows researchers to study source behavior under realistic malformed or shifted capability claims without sending arbitrary garbage. The mutation engine changes a subset of bytes associated with capabilities, luminance hints, or profile-linked characteristics. In a hardware build, these mutations would be constrained by per-profile policies and optionally diffed against a captured baseline.

### 6.3 DDC capture model

The simulator logs transactions with address, offset, length, checksum accumulation, and timestamps. In real hardware, the FPGA would observe the DDC bus with precise timing while the MCU classifies patterns. This separation allows later expansion into parser fingerprinting, retry analysis, and per-source behavioral clustering.

### 6.4 CEC model

The included CEC logic reflects a core design belief: CEC is often overlooked in security assessment, yet it can influence source selection, power state, and UI navigation in conference rooms and kiosks. EDID Phantom therefore supports passive capture and carefully rate-limited guarded transmissions for lab scenarios.

### 6.5 Safety and rollback

The firmware enforces thermal/current checks and hard pulse limits. If a live board revision experiences rising current or thermal load, it can derate active features rather than continuing aggressively. That matters in field research, where inline devices must not become the failure point in an AV path.

---

## 7. Companion Application / Software Interface

The `app/` directory contains a real React Native-style companion application layout authored by jayis1. It includes:

- **Overview screen** for profile selection and mutation preview.
- **Capture screen** for bus and policy event feed review.
- **Mutation Studio** for toggling preserved vendor blocks, timing jitter, HDR spoofing posture, and CEC guard rails.
- **CEC Console** for controlled operator actions and scenario explanations.
- **Safety screen** for ethics reminders, handling guidance, and profile warnings.

### 7.1 App-to-device interface concept

In a physical implementation, the app would talk to the ESP32-C6 sideband over BLE GATT or a local Wi-Fi HTTPS API. Core endpoints would include:

- `GET /status`
- `GET /events`
- `POST /profiles/select`
- `POST /edid/mutate`
- `POST /hpd/pulse`
- `POST /cec/transmit`
- `GET /rollback/baseline`

The app does not need host drivers on the target system. This is intentional: the target should remain unaware of the control plane, because the device is evaluating trust assumptions at the cable boundary rather than through a software agent.

---

## 8. Use Cases

### 8.1 Red team use cases

- Assess whether conference-room appliances behave differently when display identity changes.
- Test whether signage or kiosk systems expose maintenance workflows after display negotiation disruptions.
- Validate whether executive AV rooms accept unsafe CEC commands or wake events that expand attack surface.
- Observe timing of screen-sharing and source-selection behavior without touching the target network.

### 8.2 Security researcher use cases

- Fuzz source-side EDID parsing with checksum-valid capability shifts.
- Build a library of DDC retry patterns across GPU vendors and operating systems.
- Study how embedded systems react to sink removal, retrain events, or contradictory descriptor fields.
- Develop signatures for fragile display negotiation stacks.

### 8.3 Penetration tester use cases

- Demonstrate to clients that physical cable paths may carry control-plane risk, not just pixels.
- Safely emulate a maintenance display in a lab copy of a kiosk or signage stack.
- Capture evidence of insecure source fallback logic and recovery behavior.
- Create remediation guidance around locking down CEC, validating sink assumptions, and handling re-enumeration sanely.

### 8.4 Defensive engineering use cases

- Regression-test BIOS, GPU firmware, kiosk launchers, or embedded display stacks against descriptor anomalies.
- Build safer conference-room standards that disable unnecessary CEC or impose source-side policy checks.
- Validate AV installations before deployment in high-assurance or executive environments.

---

## 9. KiCad Design Notes

The `kicad/` directory contains a starting KiCad project, schematic, and PCB concept files. The schematic explicitly includes real component classes and named nets for:

- RP2350B MCU (`U1`)
- iCE40UP5K FPGA (`U2`)
- 24LC256 EEPROM (`U3`)
- ESP32-C6-WROOM-1 radio module (`U4`)
- TPD12S016-class HDMI protection / control stage (`U5`)
- Source and sink HDMI connectors (`J1`, `J2`)

The PCB file defines named nets, footprints, pad connectivity, and an inline mechanical outline appropriate for a first-pass concept board. It should be treated as a foundation for a real engineering iteration, not a finished manufacturing release. A future revision would refine impedance control, connector mechanicals, ESD placement, and possibly shift to a stack-up better suited for high-speed routing constraints.

---

## 10. Operational Workflow

A typical authorized workflow would look like this:

1. Capture a baseline EDID from the known-good sink.
2. Select a conservative observation or proxy profile.
3. Insert EDID Phantom inline and confirm stable video.
4. Record DDC/CEC telemetry and source behavior.
5. Introduce one narrow mutation at a time.
6. Observe whether the target re-enumerates, changes mode, exposes menus, or crashes.
7. If approved, test guarded HPD pulses or limited CEC actions.
8. Roll back to the baseline image and confirm recovery.
9. Export logs for reporting and client remediation guidance.

This disciplined sequencing matters. The device is most useful when used like a surgical instrument, not a chaos button.

---

## 11. Future Extensions

Potential next steps for EDID Phantom include:

- DisplayPort AUX observation mezzanine.
- Optional optical isolation for specific lab conditions.
- Signed scenario bundles for repeatable client demonstrations.
- Per-source fingerprint database for DDC parser behavior.
- Tamper-evident casework for controlled red-team logistics.
- Better passive timing characterization of TMDS/FRL state without capturing protected payloads.

---

## 12. Why EDID Phantom Matters

Security teams increasingly operate in spaces where physical infrastructure, user experience, and embedded control planes intersect. Display paths are everywhere: boardrooms, kiosks, SOC walls, hospitals, industrial control HMIs, secure workstations, hoteling desks, labs, and mobile command setups. Yet the tooling for studying those paths from a security perspective is sparse. EDID Phantom fills that gap by treating display negotiation as a first-class research surface.

It is compact enough to be plausible, structured enough to be buildable, and constrained enough to be responsibly used. It gives defenders and red teams a way to ask questions that are hard to ask with commodity tools: What does the source believe about the display? How much trust is embedded in that belief? What breaks when the display identity changes? Which control flows remain active even when nobody thinks of the cable as part of the attack surface?

Those are valuable questions. EDID Phantom is designed to help answer them.
