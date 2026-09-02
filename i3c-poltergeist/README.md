# I3C Poltergeist

**Author:** jayis1  
**Copyright:** © jayis1  
**Status:** Conceptual hardware design with compile-ready firmware simulator, companion operator console, and KiCad design files  
**Use restriction:** Authorized security research, defensive validation, hardware assurance, and red-team operations only

## Legal and ethical notice

I3C Poltergeist is designed for lawful, authorized use only. The device is intended for hardware security labs, product security teams, platform validation engineers, and red teams operating with explicit written permission from the asset owner. MIPI I3C and mixed I3C/I²C buses often sit directly between application processors, embedded controllers, PMICs, secure elements, sensor hubs, touch controllers, haptics ICs, and baseband-adjacent subsystems. Interfering with these buses on systems you do not own or administer can break devices, corrupt sensor behavior, disable power rails, interfere with safety functions, or violate policy and law. Do not deploy this design against production devices, consumer electronics, vehicles, industrial systems, medical devices, or regulated equipment without written authorization and a rollback plan.

---

## 1. Device overview

**I3C Poltergeist** is a compact inline MIPI I3C and legacy I²C interposer built for modern hardware security research. Its core purpose is to observe, classify, shape, and selectively perturb dynamic control traffic on mixed-signal internal buses that traditional test clips and generic logic analyzers handle poorly. The device sits physically between a host controller and one or more downstream peripherals, preserving normal traffic by default while providing controlled capabilities for protocol downgrade detection, Common Command Code observation, hot-join event synthesis, In-Band Interrupt replay, dynamic address remapping analysis, and targeted timing perturbation.

The novel part of the design is not “another bus sniffer.” The unique value is that I3C Poltergeist treats **dynamic enumeration** itself as the security boundary. Many modern laptops, tablets, phones, AR headsets, drones, embedded AI modules, and industrial edge systems now rely on I3C because it offers higher performance than I²C while retaining backwards compatibility. That compatibility creates a rich attack surface: devices may begin in legacy open-drain discovery states, accept dynamic addresses, expose manufacturer IDs, advertise optional features, and respond differently when the controller silently falls back to I²C compatibility mode. Few field tools help a security researcher reproduce those edge cases inline without building a custom FPGA rig.

I3C Poltergeist fills that gap. The platform uses a deterministic bridge fabric to proxy both push-pull SDR signaling and open-drain compatibility phases, while a supervisory MCU tracks policy, safety gating, capture buffers, and operator actions. A secondary wireless management coprocessor provides out-of-band control so the operator can stage captures, arm triggers, and review telemetry without tethering a laptop directly to the target. The device is intended for bench work, implant development research, firmware trust-boundary mapping, and pre-silicon or post-silicon validation exercises where the team needs more than passive observation but less risk than a full custom interposer board.

In practical terms, I3C Poltergeist lets a researcher answer questions such as:

- Which sensors or coprocessors still accept legacy I²C transactions after an I3C-capable controller is present?
- Does a target trust dynamic address assignment too broadly?
- Are hot-join or In-Band Interrupt events filtered, authenticated, rate-limited, or blindly trusted?
- Can timing jitter during ENTDAA or CCC dispatch change peripheral state in a way that exposes fault behavior?
- Does a secure coprocessor reveal inventory, revision, or debug state on a bus thought to be internal-only?
- Can a defender reliably detect inline bus manipulation using telemetry and policy controls?

Those questions matter because internal peripheral buses increasingly mediate trust decisions. Sensor data informs authentication and attestation. Embedded controllers decide keyboard routing, lid state, battery behavior, and wake policy. Touch and haptics buses influence user intent. PMIC side channels expose power state transitions. A device that can instrument and perturb I3C safely, repeatably, and with evidence logging has real utility for red teams and defenders alike.

---

## 2. Purpose and mission profile

I3C Poltergeist is designed for five mission classes:

1. **Inline bus reconnaissance** – discover which downstream devices enumerate via static versus dynamic addressing, which CCCs appear during boot, and how quickly the controller transitions from compatibility mode into SDR transfers.
2. **Downgrade-path validation** – determine whether security-sensitive peripherals remain reachable with I²C-style reads and writes even after I3C negotiation, exposing compatibility pathways invisible to higher-layer software.
3. **Protocol event injection** – test how a host stack handles synthetic hot-join, deferred interrupt, and management-event conditions under policy-controlled timing constraints.
4. **Targeted perturbation and fault observation** – introduce bounded jitter, stretch compatibility phases, mirror or suppress selected CCCs, and observe fault-handling behavior without requiring an invasive rework of the whole target board.
5. **Defensive replay and regression testing** – replay captured sequences in a lab to verify that mitigations truly close observed trust gaps.

The device is deliberately optimized for **research repeatability** rather than covert exfiltration. It logs events, enforces a policy engine, supports fail-safe bypass, and preserves a strong authorized-use posture. That makes it suitable not only for offensive assessment but also for vendor security engineering and design assurance.

---

## 3. Attack surface and threat model

### 3.1 Primary target environments

I3C Poltergeist is most useful where internal peripheral buses connect moderately trusted devices to highly trusted controllers. Common targets include:

- Laptop sensor hubs and embedded controllers
- Tablet and smartphone IMU, touch, haptic, or camera-control buses
- Wearables with low-power sensor aggregation fabrics
- Edge AI appliances with board management controllers and environmental sensors
- Industrial HMIs and robotics controllers with mixed I3C/I²C sensor stacks
- Automotive development benches for non-safety validation labs
- Secure elements, TPM-adjacent controllers, PMICs, and trusted auxiliary processors using internal low-pin-count buses

### 3.2 Threat assumptions

The design assumes the operator has temporary physical access to a target PCB, cable harness, mezzanine, or breakout point and can place an inline interposer or clip harness between controller and peripheral bus nodes. It does **not** assume arbitrary code execution on the target. The attacker or researcher may have only a short access window and may need out-of-band control to avoid attaching a visible workstation directly to the asset.

### 3.3 What the device can observe or influence

- Open-drain compatibility transactions compatible with legacy I²C-style discovery and fallback traffic
- I3C Common Command Codes during enumeration and runtime maintenance
- Dynamic address assignment flow and address remap events
- Hot-Join and In-Band Interrupt signaling patterns
- Bus timing, retries, NACK behavior, and controller fallback logic
- Rail current and local board telemetry correlated to bus events
- Operator-triggered perturbation profiles with bounded safety policy

### 3.4 Threat model categories

**A. Confidentiality risk:** Internal peripherals may expose vendor IDs, calibration constants, sensor data, firmware revisions, or privileged state that software assumes is private. The device helps show when that assumption fails.

**B. Integrity risk:** If a host trusts downstream event signaling or configuration words without strong validation, a manipulated bus may alter behavior, spoof interrupts, or influence boot-time decisions.

**C. Availability risk:** Even benign research can destabilize a target if dynamic addressing, timing, or compatibility fallbacks are mishandled. For that reason, the design includes a hard bypass relay path and write-budget logic.

**D. Assurance risk:** Vendors may believe migration from I²C to I3C inherently improved security. In reality, mixed-mode support can widen the state space. The device is built to test exactly that assumption.

### 3.5 Defensive value

For blue teams and product security groups, I3C Poltergeist can be used to validate mitigations such as:

- strict controller filtering of unexpected CCCs
- per-device attestation before sensitive traffic
- hot-join suppression or rate limiting
- legacy I²C access disablement after secure boot
- anomaly logging for dynamic address churn
- segmentation between trust-critical and commodity peripherals

---

## 4. Hardware specification

### 4.1 Processing architecture

- **Primary MCU:** STM32H735RGV6
  - Supervises capture policy, telemetry, operator command parsing, power management, and fail-safe controls
  - Chosen for deterministic timing support, rich peripherals, and strong ecosystem support
- **Bridge / timing fabric:** Lattice CrossLink-NX LIFCL-17
  - Terminates host-side and target-side bus domains independently
  - Enforces bounded timing perturbation and dynamic address remap tables
  - Handles SDR forwarding, open-drain compatibility proxying, and event injection arbitration
- **Out-of-band control coprocessor:** ESP32-C6-MINI-1
  - Provides BLE and Wi-Fi operator link
  - Isolated from inline bus path so wireless faults cannot directly drive the target bus

### 4.2 Bus-facing and monitoring elements

- Dual inline board-to-board mezzanine connectors for host-side and target-side bus harnessing
- Configurable pull-up network for 1.2 V / 1.8 V / 3.3 V domains
- Low-capacitance analog switches for hard fail-safe bypass
- TI INA238 current and voltage monitor for target rail correlation
- Temperature sensor near bridge fabric for thermal derating
- Optional differential sideband headers for logic-analyzer correlation
- MicroSD slot for long captures and offline export
- USB-C for power, firmware update, and wired operator control

### 4.3 Radio and connectivity

- BLE 5.x for short-range operator control in lab or staging environment
- 2.4 GHz Wi-Fi for console access, log export, and remote triggering on isolated bench networks
- USB CDC ACM serial console for deterministic control when radio silence is preferred

### 4.4 Power system

- USB-C bus-powered, 5 V input
- Onboard 3.3 V digital rail via synchronous buck
- Selectable target-bus reference voltage buffers for low-voltage I/O domains
- Polyfuse and ideal-diode input protection
- Measured rail telemetry for safety cutoffs and evidence correlation

### 4.5 Form factor

- 72 mm × 38 mm four-layer PCB
- Rigid board with mounting holes and flex-harness attachment points
- Bench clip, mezzanine, or in-line ribbon harness use
- Designed to live inside a host shell during authorized hardware assessments, but primarily a bench tool

---

## 5. System architecture

### 5.1 High-level architecture

The device uses a **three-plane model**:

1. **Inline signaling plane** – host-side bus enters the timing fabric, which proxies transactions to the target-side bus and can apply policy-bound timing or protocol transforms.
2. **Supervision plane** – the STM32H7 gathers captures, enforces write budgets, stores profiles, and operates the fail-safe relay.
3. **Operator plane** – the ESP32-C6 exposes the management API and companion console, but only communicates with the MCU over a constrained UART protocol.

### 5.2 Text block diagram

```text
 Host SoC / EC
      │
      │ I3C / I²C mixed-mode
      ▼
+-------------------------+
| Host Connector / TVS    |
+-------------------------+
      │
      ▼
+-------------------------+      +---------------------------+
| CrossLink-NX Bridge     |<---->| STM32H735 Supervisor MCU |
| - SDR forwarding        | UART | - policy engine          |
| - CCC mirror/filter     |      | - capture ring           |
| - ENTDAA observation    |      | - fail-safe control      |
| - hot-join/IBI inject   |      | - telemetry/log export   |
| - timing perturbation   |      +---------------------------+
+-------------------------+                   │
      │                                       │ UART / command framing
      ▼                                       ▼
+-------------------------+      +---------------------------+
| Target Connector / TVS  |      | ESP32-C6 OOB Control     |
+-------------------------+      | - BLE/Wi-Fi console      |
      │                          | - operator auth/session   |
      ▼                          +---------------------------+
 Target Sensor / PMIC / EC bus               │
                                              ▼
                                     Web/phone console app
```

### 5.3 Safety architecture

The board is intentionally designed so the default power-up state is **pass-through observe mode**. Any action that changes target-visible behavior requires a loaded profile, explicit arming, and remaining write budget. If thermal, current, or voltage thresholds are exceeded, the MCU forces bypass mode and records the event. The bypass relay path allows rapid disengagement when a target becomes unstable.

---

## 6. Firmware design and engineering decisions

The supplied firmware is a **compile-ready simulator-oriented control stack** written in C. It models how the real device firmware would behave and compiles on a standard host toolchain. That lets the repo include executable logic, profile handling, event capture, and console framing without pretending a full vendor SDK environment exists inside the repository.

### 6.1 Firmware modules

- `main.c` – boot flow, scenario execution, event logging, capture orchestration, and operator summaries
- `drivers/i3c_bus.c` – bus inventory, dynamic address state, CCC capture, downgrade-path simulation, and injection primitives
- `drivers/timing_fabric.c` – perturbation profile staging, jitter windows, trigger matching, and fabric health status
- `drivers/policy.c` – safety policy, arming logic, write budget enforcement, and rollback conditions
- `drivers/telemetry.c` – readable status summaries and JSON export
- `drivers/radio.c` – operator heartbeat and export frame rendering for the out-of-band link
- `board.h` / `registers.h` – shared types, constants, and command definitions

### 6.2 Key design choices

**Simulation-first repo deliverable:** Instead of dropping placeholder firmware with “TODO” sections, the design includes working C that builds and runs on the host. This keeps the repository verifiable and gives future hardware bring-up a clean behavioral reference.

**Policy-gated active behavior:** Inline manipulation is separated from passive capture by a policy layer. That reduces accidental target disruption and mirrors what a real research lab would want when multiple operators share a tool.

**Capture evidence over silent action:** Every staged perturbation, CCC observation, downgrade event, or rollback condition creates structured events. The operator can export those records to justify findings.

**Bounded perturbation model:** The timing fabric does not expose arbitrary scripting of bus waveforms in the default design. Instead it accepts bounded presets, such as small jitter windows, CCC mirror suppression, or one-shot hot-join emission. That is enough for meaningful testing while reducing operator mistakes.

**Mixed-mode awareness:** The most important security insight on many targets is the transition between legacy open-drain compatibility transactions and faster I3C SDR behavior. The firmware tracks both explicitly.

### 6.3 Operational profiles included

The sample firmware models several realistic assessment profiles:

- **inventory-passive** – observe boot-time enumeration without changing bus behavior
- **downgrade-probe** – test whether selected peripherals still respond to legacy-style access after dynamic address assignment
- **hotjoin-ghost** – inject a synthetic hot-join after a bounded delay to test controller admission policy
- **ibi-shadow** – emulate or replay an In-Band Interrupt from a selected target profile
- **ccc-eclipse** – suppress or mirror a selected CCC once to test host retry behavior

---

## 7. Companion application and software interface

The repository includes a lightweight browser-based companion application. A web app is appropriate here because operators often need a quick console from a laptop, tablet, or phone without installing a heavy desktop client. The app is structured as a static interface that can later be hosted directly by the ESP32-C6 or served by a bench laptop.

### 7.1 App capabilities

- Device status cards with mode, write budget, rail telemetry, and operator link state
- Profile browser with mission presets and risk labels
- Capture timeline showing CCCs, dynamic address events, and policy outcomes
- Command staging pane for safe one-shot injections
- Ethics / authorized-use reminder embedded in the UI
- Simulated state transitions so the app is demonstrably functional even without physical hardware

### 7.2 Control protocol concept

The app assumes a JSON command channel over BLE GATT or WebSocket-to-serial bridge, for example:

```json
{
  "command": "stage_profile",
  "profile": "downgrade-probe",
  "arm": true,
  "write_budget": 2
}
```

Readbacks are modeled as compact JSON frames carrying status, events, captures, and export metadata. The firmware simulator generates equivalent JSON summaries for future integration work.

---

## 8. Red-team and research use cases

### 8.1 Laptop platform validation

A red team assessing a modern ultrabook may suspect that a secure sensor or embedded controller is reachable over an internal bus during early boot. I3C Poltergeist can sit inline between the SoC and a downstream device to capture CCC traffic, determine when dynamic address assignment occurs, and test whether the peripheral still accepts legacy transactions that were assumed disabled.

### 8.2 Mobile device trust mapping

On a tablet or phone development board, a researcher may want to understand whether haptics, touch, or IMU devices can generate event patterns that influence trusted UI state. The tool can characterize interrupt cadence, hot-join acceptance, and fallback behavior without reworking the processor board.

### 8.3 PMIC and sensor bus hardening

A defensive engineering team can use the tool to replay known-bad event sequences in regression tests after patching firmware. If a host once accepted spurious joins or failed to log downgrade attempts, the device becomes a reusable validation instrument.

### 8.4 Embedded product assurance

For industrial or edge devices, internal low-speed buses are often invisible to software-only security reviews. I3C Poltergeist provides a way to validate that trust assumptions hold across accessory modules, daughtercards, or vendor-supplied sensors.

### 8.5 Implant and interdiction research

In an authorized lab environment, teams studying hardware interdiction or inline implants can use the design as a starting point for understanding how a minimally invasive bus manipulator would behave, what telemetry it should expose, and what detection opportunities defenders might have.

---

## 9. Bring-up and lab workflow

1. Power the board over USB-C with the target disconnected.
2. Load the passive inventory profile and verify the board remains in observe mode.
3. Attach host-side and target-side harnesses with correct logic-voltage selection.
4. Collect a baseline capture through boot and dynamic address assignment.
5. Review the capture for CCC order, target IDs, address churn, and legacy access windows.
6. Only then arm a bounded active profile, such as downgrade-probe or one-shot hotjoin-ghost.
7. Export telemetry, event log, and capture timeline for evidence.
8. If any anomaly threshold triggers, the board forces bypass and records the reason.

---

## 10. Repository contents

- `README.md` – this document
- `firmware/` – compile-ready C firmware simulator and drivers
- `kicad/` – KiCad project, schematic, and PCB layout files
- `app/` – browser-based companion console with functional simulated workflows

---

## 11. Future expansion ideas

- HDR mode experimentation support with stronger timing granularity
- Secure log signing for chain-of-custody workflows
- Per-target bus adapters for phone and tablet mezzanine ecosystems
- Optional differential side-channel probes for synchronized power and protocol analysis
- Offline rule packs for vendor-specific CCC behavior

---

## 12. Why this device is novel

Plenty of tools can sniff I²C. Some high-end lab gear can decode I3C. Very few portable, operator-friendly devices are purpose-built to **inline proxy dynamic address assignment, hot-join behavior, CCC filtering, and mixed-mode downgrade testing** in one package. That combination is what makes I3C Poltergeist interesting. It focuses on a transition point in modern hardware architecture that is under-instrumented from a security perspective. It is practical because the buses are real and increasingly common. It is novel because it treats protocol state transitions as the primary object of study rather than only measuring signal edges.

For red teams, that means a new hardware foothold for platform trust validation. For defenders, it means a repeatable method to test whether a migration to newer peripheral buses truly improved security. For hardware engineers, it offers a concrete architecture for building a safer, policy-aware interposer rather than an opaque glitch box.

---

## 13. Authorized-use reminder

Use I3C Poltergeist only with explicit authorization, documented scope, and a rollback plan. The goal of this design is to improve hardware security research and defensive assurance, not to encourage misuse.