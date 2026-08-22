# EtherCAT Phantom — Deterministic Inline EtherCAT Frame Surgeon

**Author:** jayis1  
**License:** Hardware — CERN-OHL-S v2 · Firmware — GPL-2.0-only · App — MIT  
**Version:** 1.0.0

> **Legal / Ethical Notice:** EtherCAT Phantom is designed strictly for authorized security research, defensive validation, red-team exercises, and industrial control system assessment performed with explicit written permission from the system owner. Do not deploy, test, connect, or operate this device against production equipment, safety systems, or industrial networks without authorization and a validated engagement plan.

---

## 1. Device Overview

EtherCAT Phantom is a **portable inline EtherCAT interception, manipulation, and telemetry platform** built for industrial control system security research. Its purpose is not to passively observe Ethernet traffic in a generic way, but to sit **directly in the process-data path of EtherCAT fieldbus segments** and perform **deterministic, cycle-aware frame inspection and bounded process-value surgery** without breaking the real-time assumptions that make EtherCAT operationally useful.

That distinction matters. Traditional network taps, switch SPAN ports, and generic Ethernet tools are useful for broad network monitoring, but they usually fail when the engagement objective is narrowly focused on **industrial fieldbus integrity**, **drive/setpoint tampering**, **controller-to-actuator trust boundaries**, or **real-time process value abuse**. EtherCAT traffic is highly timing-sensitive and often used in manufacturing lines, packaging machines, robotics cells, motion-control racks, and safety-adjacent automation systems. An assessment platform that adds excessive latency, breaks link continuity, or mishandles process datagrams is worse than useless; it changes the test environment and may collapse the very system being evaluated.

EtherCAT Phantom is designed around that operational reality. The device is an **inline dual-port 100BASE-TX bridge with FPGA-timed fast path**, backed by an MCU control plane and a wireless operator interface. It allows an operator to:

- Observe cyclic EtherCAT process data in real time.
- Identify specific slaves, object indexes, and subindexes of interest.
- Inject bounded manipulations into chosen process values.
- Enforce safe-mode and timing budgets so risky operations degrade back to bypass.
- Log frame modifications, timing margins, and resulting process data for later analysis.
- Operate physically near a cabinet, machine, or remote IO island while controlling the device through a tablet or phone.

The concept is novel because it combines **inline EtherCAT frame surgery**, **deterministic timing enforcement**, **failsafe relay bypass**, **portable battery operation**, and **mobile-first operator control** in one purpose-built research instrument. Most existing tools in real industrial assessments fall into one of three categories: passive Wireshark-only visibility, general-purpose Ethernet man-in-the-middle gear, or expensive lab-grade protocol analyzers that are not optimized for covert or field deployment. EtherCAT Phantom fills the gap between protocol comprehension and practical red-team utility.

---

## 2. Purpose and Practical Security Value

EtherCAT is widely trusted once the physical network segment is established. In many deployments, operators assume that if a frame came from the legitimate controller and the slaves acknowledge it, the process values can be trusted. That assumption is fragile. Inline access to the fieldbus lets an adversary, researcher, or red team target the narrowest part of the operational trust chain: the exact bytes that carry torque, position, state, enable, latch, or setpoint semantics.

EtherCAT Phantom is useful because it enables realistic testing of questions defenders frequently avoid asking:

- What happens if a servo setpoint is clamped below commanded value during a critical motion sequence?
- Can a heating element setpoint be quietly derated while HMI displays remain plausible?
- Can safety-adjacent but non-safety-certified process variables be manipulated to induce drift, nuisance faults, or degraded output?
- Does the PLC or motion controller verify that returned process data actually reflects physical state?
- Can an engineer detect sub-cycle timing anomalies introduced by a malicious inline node?
- Are EtherCAT segments physically exposed inside panels, machine bases, or field junction boxes?

These are not hypothetical. OT red teams and industrial security researchers increasingly need tooling that can validate **control-loop trust assumptions** rather than just perimeter segmentation. EtherCAT Phantom provides a bounded way to do exactly that.

---

## 3. Attack Surface and Threat Model

### 3.1 Targeted Attack Surface

EtherCAT Phantom targets the **field-level operational data plane** of industrial control systems. Specific attack surfaces include:

1. **Cyclic process data** between EtherCAT master and slaves.
2. **Mailbox exchanges** carrying configuration, diagnostics, or object dictionary access.
3. **Physical cabinet access** to exposed fieldbus patching or service loops.
4. **Trust in actuator commands** such as target position, velocity, torque, and enable words.
5. **Trust in sensor return values** used for HMI visualization, process tuning, or supervisory logic.
6. **Timing assumptions** inside closed-loop motion and process control networks.

### 3.2 Assets at Risk from a Defender Perspective

- Motion-control setpoints.
- Remote IO state.
- Analog actuator outputs.
- Thermal and pressure process values.
- Machine production quality and throughput.
- Diagnostics integrity.
- Safety-adjacent sequencing logic.
- Incident response clarity if logs diverge from actual on-wire values.

### 3.3 Adversary Model

EtherCAT Phantom assumes an operator with:

- Short-duration physical access to insert an inline bridge.
- Ability to identify the correct EtherCAT segment.
- Knowledge of at least some target object dictionary semantics or willingness to infer them by observation.
- Desire to preserve link continuity and avoid obvious process collapse.

### 3.4 Defensive Assumptions the Device Tests

- “Fieldbus access requires too much expertise to be practical.”
- “If traffic is not IP-routed, it is not realistically attackable.”
- “Operational values are trusted because the controller generates them.”
- “Timing-sensitive buses are naturally resistant to manipulation.”
- “Physical access is equivalent to full compromise anyway, so protocol-specific defenses are irrelevant.”

EtherCAT Phantom explicitly challenges those assumptions.

### 3.5 Safety Boundary

The device is **not** intended for use against certified safety protocols, life-safety systems, or uncontrolled production machinery. The firmware architecture includes a safe-mode policy and hard bypass relay because one of the core design goals is to let a researcher back out of a risky condition faster than the process can drift into instability.

---

## 4. Hardware Specifications

### 4.1 Core Compute

- **Primary MCU:** STM32H753VIT6
  - Cortex-M7 @ 480 MHz
  - High RAM capacity for logging, command parsing, capture buffering, and management tasks
  - USB 2.0 FS, SPI, UART, I2C, ADC, timers
- **Timing / Fast-Path Fabric:** Lattice ECP5 LFE5U-25F FPGA
  - Deterministic frame parsing and rewrite staging
  - Nanosecond-budget enforcement for inline modifications
  - Dual-port path arbitration and failover state tracking

### 4.2 Network / Fieldbus Front End

- **2 × 100BASE-TX Ethernet PHY:** Microchip KSZ8081RNBCA
- **2 × integrated-magnetics RJ45 ports** for EtherCAT line insertion
- **Bypass relay path** to preserve cable continuity on power loss or hard safe-mode assertion
- **Link-status sensing** for both ingress and egress ports

### 4.3 Wireless / Operator Connectivity

- **ESP32-C6-WROOM-1** used as combined BLE 5 and Wi-Fi control module
- BLE for quiet nearby control from mobile devices
- Wi-Fi for bench use, logging export, and firmware staging

### 4.4 Storage

- **128 Mbit QSPI flash** for capture buffers, rulesets, profiles, and signed update images

### 4.5 Power Subsystem

- **Single-cell LiPo input**
- **USB-C service/power port**
- **BQ24074** Li-ion charger / power-path management
- **TPS62172** buck regulator for efficient 3.3 V rail generation
- 1.2 V FPGA core rail via dedicated regulator stage
- Battery telemetry exposed to firmware

### 4.6 Physical Form Factor

- Approximate enclosure: **126 mm × 72 mm × 23 mm**
- Rugged handheld anodized aluminum shell or field-service polymer enclosure
- Two inline EtherCAT ports on one edge, USB-C and status LEDs on opposite edge
- Magnetic rear mounting plate option for cabinet walls

### 4.7 Indicators and Service Access

- RGB status LED for mode / risk state
- Link LEDs per port
- Hidden service SWD header
- Side button for emergency bypass / operator acknowledgment

---

## 5. High-Level Architecture

EtherCAT Phantom uses a **split-plane architecture**:

1. **Fast path (FPGA):** parse EtherCAT frames, track timing budget, stage process-data rewrite, maintain deterministic bridge behavior.
2. **Control plane (MCU):** rule management, capture aggregation, profile persistence, power management, user command parsing, and radio coordination.
3. **Operator plane (mobile app):** ruleset authoring, target selection, live captures, safety acknowledgment, and telemetry review.

### 5.1 Block Diagram

```text
                 ┌─────────────────────────────────────────┐
                 │          EtherCAT Phantom              │
                 │           Author: jayis1               │
                 ├─────────────────────────────────────────┤
  Port A RJ45 ───┤ KSZ8081 PHY A ─┐                  ┌───┤ KSZ8081 PHY B ├── Port B RJ45
                 │                │                  │   │
                 │          ┌─────▼──────────────────▼──┐│
                 │          │   Lattice ECP5 FPGA       ││
                 │          │ Inline parser / rewriter  ││
                 │          │ timing guard / bypass ctl ││
                 │          └─────▲──────────────────┬──┘│
                 │                │ SPI / IRQ        │   │
                 │          ┌─────┴──────────────────▼──┐│
                 │          │      STM32H753 MCU        ││
                 │          │ rules, logging, storage,  ││
                 │          │ radio control, safety     ││
                 │          └─────▲─────────────┬───────┘│
                 │                │ UART/USB    │ I2C/SPI │
                 │      ┌─────────┘             └──────┐  │
                 │  ESP32-C6 BLE/Wi-Fi           QSPI Flash│
                 │                                         │
                 │  LiPo + Charger + Buck + Relay Bypass   │
                 └─────────────────────────────────────────┘
```

### 5.2 Why This Architecture

An MCU alone is insufficient for reliable inline real-time frame manipulation at fieldbus timing margins. An FPGA alone is awkward for operator UX, storage, wireless policy, and safe engagement workflows. The split-plane design allows the FPGA to own the deterministic path while the MCU owns human complexity, logging, and policy.

---

## 6. Firmware Details and Design Decisions

The included firmware folder implements the **control-plane simulation** and serves as a compile-ready reference for the embedded stack. In a production build, the MCU would interact with an FPGA bitstream, PHY management interfaces, power telemetry, and radio transport. The simulation included in this repository is intentionally substantial rather than a stub: it demonstrates configuration, rule matching, frame parsing, capture buffering, storage logging, radio status emission, power-state tracking, and mailbox interaction.

### 6.1 Firmware Modules

- `main.c` — boot sequence, stream processing loop, summary reporting
- `board.h` — platform constants, enums, shared runtime structs
- `registers.h` — hardware register map placeholders and bitfields
- `drivers/ethercat.c` — frame encode/decode, capture ring, demo stream
- `drivers/fpga_mailbox.c` — command channel between MCU and fast path
- `drivers/rule_engine.c` — rule matching and bounded process-value transforms
- `drivers/storage.c` — profile persistence and capture log storage
- `drivers/radio.c` — BLE/Wi-Fi telemetry abstraction
- `drivers/power.c` — battery and power-state modeling
- `Makefile` — host-buildable simulation target

### 6.2 Design Decisions

**Deterministic safety first:** if timing exceeds budget, the simulated FPGA sets bypass and safe-mode. That models the real hardware intention: manipulation is a privilege granted only while the timing envelope remains healthy.

**Rules are explicit and bounded:** a rule matches on slave address, object index, and subindex. This reduces accidental cross-target tampering.

**Safe mode blocks sensitive changes:** certain control objects are treated as sensitive and become monitor-only when safe mode is asserted.

**Capture logging is ring-buffered:** the firmware continuously accumulates recent events and periodically flushes them to simulated storage and radio output.

**Radio is optional, not assumed:** the device can continue inline operation even if wireless connectivity drops.

**Power state participates in safety:** low battery can force the system back toward safe bypass behavior instead of gambling on unstable operation.

### 6.3 Example Built-In Rules

The default rules illustrate realistic assessment patterns:

- Clamp a conveyor speed object to a bounded range.
- Offset a heater setpoint downward by a fixed amount.
- Force a servo target position to a spoofed value.
- Toggle a safety-adjacent latch bit in a bounded demo scenario.
- Monitor a valve position without modification.

### 6.4 Firmware Build and Validation

The Makefile is designed to compile on a host system with GCC for deterministic simulation and traceability during repository validation. This is intentional: it lets contributors and reviewers execute the control logic without requiring a full cross-toolchain or physical hardware.

---

## 7. Application / Software Interface

The `app/` directory contains a React Native / Expo-oriented companion interface authored by **jayis1**. The application is designed around a field operator workflow rather than a generic dashboard.

### 7.1 Main Screens

1. **Overview**
   - Link state
   - timing budget
   - timing margin
   - battery state
   - safe-mode status

2. **Rules**
   - Ruleset visibility
   - object-targeted manipulations
   - operator comprehension of active transforms

3. **Captures**
   - process-data delta review
   - modified vs observed distinction
   - quick field triage of what changed

4. **Safety**
   - engagement checklist
   - authorization reminders
   - bypass and change-control awareness

### 7.2 Control Model

A production version of the app would speak to the device using a compact binary or CBOR-based application protocol over BLE GATT or Wi-Fi WebSocket/TLS. The included `protocol.js` provides sample device state, rules, and capture records to ground the UI.

### 7.3 Why Mobile-First

OT assessments often happen standing in front of cabinets, under conveyors, next to robot cells, or inside cramped service corridors. A laptop is not always physically convenient or discreet. Mobile-first control makes inline engagement far more practical.

---

## 8. Hardware Operation Concept

1. Researcher identifies an EtherCAT segment and confirms authorization.
2. Device is inserted inline between master-side cable and downstream slaves.
3. Relay bypass preserves continuity during power transitions.
4. FPGA fast path learns line state and begins transparent forwarding.
5. MCU receives metadata about object activity and timing margin.
6. Operator selects observe-only mode first.
7. Rules are armed only after target mapping and risk review.
8. Selected process values are modified if and only if timing and policy allow.
9. All modifications are logged with before/after state.
10. Safe mode or emergency bypass can be asserted instantly.

---

## 9. Use Cases for Red Teams, Security Researchers, and Pentesters

### 9.1 Motion-System Trust Validation

A red team can validate whether a robot controller detects that a downstream drive target position has been changed mid-cycle. This directly tests controller assumptions, HMI truthfulness, and physical-vs-reported state divergence.

### 9.2 Quality Degradation Simulation

Instead of causing loud failures, the device can apply subtle bounded manipulations to process values such as conveyor speed or thermal output. This models adversaries who prefer degraded quality, scrap generation, or hidden throughput loss rather than outages.

### 9.3 Cabinet Exposure Assessment

A pentester can document whether fieldbus segments are physically exposed in accessible panels or unmanaged machine spaces. EtherCAT Phantom turns “physical exposure exists” into a tangible demonstration of what that exposure means operationally.

### 9.4 Detection Engineering Exercises

Blue teams can monitor whether inline tampering produces alerts in industrial IDS platforms, controller diagnostics, or engineering workstations. The device becomes a repeatable test harness for defensive analytics.

### 9.5 Protocol Reverse Engineering

Researchers can observe object activity patterns, infer setpoint semantics, and correlate process behavior with object dictionary changes over time.

### 9.6 Resilience and Recovery Drills

Teams can test how quickly a site detects and recovers from field-level value distortion, including whether operations personnel know how to isolate suspect EtherCAT segments.

---

## 10. Threat Model Boundaries and Research Ethics

EtherCAT Phantom is powerful precisely because it lives near real machinery behavior. That means misuse could be unsafe. The design therefore emphasizes:

- explicit authorization,
- controlled environments,
- bounded rule design,
- emergency bypass,
- deterministic timing guardrails,
- and transparent operator logging.

Researchers should treat inline fieldbus manipulation with the same seriousness they would apply to electrical safety or lockout/tagout procedures. In many environments, process variables ultimately govern motion, heat, pressure, or material handling. A “security test” that ignores operational safety is not a professional assessment.

---

## 11. Why EtherCAT Phantom Is Distinct from Existing Tools

EtherCAT Phantom is not just another network tap, not a generic PLC exploit box, and not a broad-spectrum packet injector. Its distinctives are:

- **EtherCAT-specific mission focus** rather than generic Ethernet posture.
- **Deterministic inline manipulation** rather than offline replay.
- **FPGA-timed fast path** rather than best-effort software bridging.
- **Built-in safety downgrade and bypass logic** rather than operator hope.
- **Portable field form factor** rather than rack or bench dependence.
- **Mobile operator workflow** designed for real cabinet-side operations.

Those qualities make it practically useful for modern OT red teaming and industrial security validation.

---

## 12. Repository Contents

```text
ethercat-phantom/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── ethercat.c / ethercat.h
│       ├── fpga_mailbox.c / fpga_mailbox.h
│       ├── rule_engine.c / rule_engine.h
│       ├── storage.c / storage.h
│       ├── radio.c / radio.h
│       └── power.c / power.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── App.js
    ├── package.json
    ├── components/
    ├── screens/
    └── utils/
```

---

## 13. Future Enhancements

Potential future work for EtherCAT Phantom includes:

- automated slave fingerprinting and PDO inference,
- signed ruleset packages with operator attestations,
- line-rate capture export to removable media,
- deterministic attack sequencing tied to observed machine states,
- hardware timestamp correlation with external sensors,
- and optional optical or isolated variants for harsher industrial environments.

---

## 14. Final Disclaimer

**EtherCAT Phantom was created by jayis1 for authorized use only.** It is a security research and validation instrument intended to help defenders, assessors, and engineers understand real industrial trust boundaries. Unauthorized deployment may violate law, policy, contract, and safety obligations. Always obtain explicit written permission, define test boundaries in advance, coordinate with operations and safety stakeholders, and prefer non-production or tightly controlled windows whenever possible.
