# MoCA Phantom — Inline Coax Lateral Movement Mapper and MoCA Security Research Bridge

**Author:** jayis1  
**License:** Hardware — CERN-OHL-S v2 · Firmware — GPL-2.0-only · App — MIT  
**Version:** 1.0.0

> **Legal / Ethical Notice:** MoCA Phantom is designed strictly for authorized security research, defensive validation, red-team exercises, and penetration testing conducted with explicit written permission from the owner or operator of the target coax plant, premises network, or broadband infrastructure. Do not deploy, connect, test, bridge, capture, or manipulate traffic on shared coax, subscriber networks, hospitality infrastructure, MDUs, or residential environments without authorization.

---

## 1. Purpose and Overview

MoCA Phantom is a **portable inline MoCA assessment platform** built for a problem that remains surprisingly underexplored in offensive and defensive security work: the security boundary created, assumed, or misunderstood around **Ethernet-over-coax infrastructure**. Many organizations think of coax as “legacy cabling,” “television wiring,” or “provider-owned physical layer,” but in hotels, apartments, mixed-use buildings, managed residences, campuses, yachts, and distributed media systems, coax often becomes an **unexamined lateral movement fabric**. The result is a hidden transport plane where segmentation assumptions, privacy settings, node enrollment rules, and physical leakage are rarely tested with the same rigor as Wi-Fi or Ethernet.

MoCA Phantom addresses that gap. It is a **battery-powered, dual-F-connector inline bridge and survey instrument** that can be inserted into a coax run, observe MoCA beacons and management exchanges, build a node map, identify privacy weaknesses, detect leakage into adjacent risers or neighboring units, and perform carefully bounded manipulation or emulation during authorized engagements. Rather than being a generic SDR, generic Ethernet tap, or broadband modem replacement, it is purpose-built around **MoCA trust validation**, **coax path attribution**, and **practical red-team utility**.

The novel contribution is not just “another RF tool.” MoCA Phantom combines several functions that are normally fragmented across lab gear, provider diagnostics, and ad hoc coax experimentation:

- Inline pass-through on a live coax segment.
- Active and passive discovery of MoCA nodes and network identifiers.
- Detection of privacy-disabled, miskeyed, or leaky MoCA domains.
- Controlled manipulation of selected management and probe traffic for resilience testing.
- Spectrum-energy correlation to identify suspicious splitter paths or adjacent-unit bleed.
- Mobile-first operator control with safety acknowledgment and engagement profiles.
- Battery-powered field use for hospitality, MDU, and facility red-team operations.

In practical terms, MoCA Phantom helps answer questions such as:

- Can a guest-room coax drop reach another room’s network via a shared riser?
- Are provider-managed set-top boxes and customer devices on the same MoCA domain?
- Is MoCA privacy enabled consistently, or are nodes falling back to insecure enrollment behavior?
- Can an assessor discover hidden nodes or leakage paths from a closet, riser, or demarc extension?
- Can defenders detect unauthorized node appearance, node-ID reuse, or suspicious management traffic on the coax plane?

For red teams and security researchers, MoCA Phantom creates a new class of field-deployable infrastructure assessment device: small enough to carry, specific enough to be useful, and engineered around the realities of coax-based lateral movement.

---

## 2. Why This Device Matters

Enterprise security tooling has historically focused on routed networks, Wi-Fi, USB, Bluetooth, industrial fieldbuses, and removable media. Coax rarely receives equivalent treatment outside telecommunications engineering. That blind spot matters because MoCA is often present in environments where defenders assume strong isolation for reasons that do not actually hold under physical inspection.

Examples include:

- **Hotels and resorts:** guest rooms, IPTV deployments, and back-of-house media distribution frequently share coax infrastructure in ways that are poorly documented.
- **Apartment buildings and student housing:** risers and splitters may inadvertently expose neighboring units to the same RF domain or permit signal bleed across branch legs.
- **Luxury homes and estates:** managed AV systems may bridge security cameras, media appliances, gateways, and control systems through MoCA extenders.
- **Yachts, trains, and remote facilities:** legacy coax may remain the easiest physical path across compartments where rewiring is expensive.
- **Retail and branch locations:** set-top boxes, signage players, and gateway extenders can create hybrid coax/IP trust boundaries that no one is actively monitoring.

Traditional assessment methods are awkward here. A generic Ethernet tap may never see the transport if the sensitive segment is inside the MoCA domain. A lab SDR may see energy but not provide operational workflow. Provider analyzers are expensive, closed, and not designed for red-team tradecraft. MoCA Phantom fills that gap with a device architecture that treats coax as a legitimate attack surface deserving first-class tooling.

---

## 3. Attack Surface and Threat Model

### 3.1 Primary Attack Surface

MoCA Phantom is designed to evaluate the following security-relevant surfaces:

1. **MoCA network formation beacons** used to advertise domain presence and channel use.
2. **Node enrollment and privacy behavior**, including inconsistent keying and fallback operation.
3. **Shared coax plant topology**, especially splitters, amplifiers, and riser leakage paths.
4. **Management-plane traffic** between customer premises equipment, gateways, set-top boxes, and media extenders.
5. **Lateral movement opportunities** where devices trust node membership rather than explicit segmentation.
6. **Physical access points** such as room drops, maintenance panels, telecom closets, and service loops.

### 3.2 Threat Model

MoCA Phantom assumes an operator with authorized short-duration physical access to a coax segment. The operator may be a red teamer, penetration tester, facility security engineer, broadband provider security team, or hardware researcher. The adversary model under test is a capable actor who:

- Can temporarily connect or inline a small device on a live coax run.
- Understands that the coax domain may expose infrastructure beyond the visible room or cabinet.
- Wants to observe or validate trust boundaries without immediately disrupting service.
- May seek to discover hidden neighbors, unmanaged nodes, weak privacy, or bridging faults.

### 3.3 Defender Assumptions Challenged

MoCA Phantom exists to test assumptions defenders frequently make without evidence:

- “Coax isn’t part of the network in a security sense.”
- “If the Ethernet switch is segmented, the media network is segmented too.”
- “Neighboring rooms or units can’t see each other because the provider installed filters.”
- “Set-top infrastructure is operational technology, not an attack path.”
- “Physical access to a coax jack is harmless unless someone brings a cable modem.”

### 3.4 Safety Boundary

MoCA Phantom is intentionally designed with **safe-mode**, **bypass defaults**, and **bounded manipulation rules**. Its firmware philosophy is to prefer observation first and intervention second. The device is not intended to be used to degrade service, violate regulations, or interfere with provider-owned infrastructure outside controlled assessments.

---

## 4. Hardware Specifications

### 4.1 Core Compute

- **Primary MCU:** NXP i.MX RT1176
  - Dual-core Cortex-M7/M4 architecture
  - High performance for capture processing, UI transport, and profile management
  - Rich peripheral set for SPI, I2C, UART, USB, SDIO, ADC, and high-speed memory interfaces
- **Inline fabric / timing engine:** Lattice CrossLink-NX FPGA
  - Handles deterministic pass-through control, capture triggers, timing guards, and relay orchestration
  - Allows hard separation of real-time coax/PHY supervision from higher-level application logic

### 4.2 MoCA / Coax Front End

- **2 × F-type inline coax connectors** for insertion into a live run
- **MoCA 2.5 front-end module** with low-noise receive path and controlled transmit staging for authorized probe/injection modes
- **Digitally switchable attenuator bank** for coax path characterization and leakage testing
- **Directional coupler and RF detector path** for relative energy measurement
- **Integrated point-of-entry filter bay** for testing isolation and misconfiguration scenarios in lab or field-approved workflows

### 4.3 Operator Connectivity

- **ESP32-C6 module** for BLE 5 and Wi-Fi 6 control plane connectivity
- BLE mode for low-visibility nearby operation from a phone or tablet
- Wi-Fi mode for bench use, data export, firmware staging, and richer dashboard sessions
- **USB-C service port** for wired management and charging

### 4.4 Storage and Logging

- **256 Mbit QSPI flash** for engagement profiles, signed update bundles, capture snapshots, node maps, and operator notes
- Optional microSD footprint for extended lab capture export

### 4.5 Power System

- Single-cell LiPo battery
- USB-C charge and power-path management
- Fuel-gauge telemetry exposed to firmware
- High-efficiency buck rails for 3.3 V logic and 1.2 V fabric
- Hardware bypass path so passive continuity remains available when the device is off or forced safe

### 4.6 Sensors and Indicators

- RF energy detector for channel-strength trend observation
- Battery and rail telemetry
- Tri-color status LED for mode/risk state
- Link/activity indicators for coax front-end states
- Side-mounted guarded bypass button

### 4.7 Form Factor

- Handheld field instrument
- Approximate size: **132 mm × 76 mm × 24 mm**
- Aluminum or glass-filled polymer enclosure with recessed connectors
- Magnet plate and hook-loop mounting options for telecom closets or structured cabling panels

---

## 5. Architecture and Block Diagram

MoCA Phantom uses a **three-plane architecture**:

1. **RF / inline plane** for safe insertion and signal observation.
2. **Embedded control plane** for node discovery, rules, telemetry, logging, and safety policy.
3. **Operator application plane** for guided workflows and evidence capture.

```text
┌────────────────────────────────────────────────────────────────────┐
│                         MoCA Phantom                              │
│                         Author: jayis1                            │
├────────────────────────────────────────────────────────────────────┤
│  F-IN ─ Directional Coupler ─ MoCA Front End ─ Relay Bypass ─ F-OUT│
│           │                       │                │               │
│           │                       ▼                │               │
│           │                Lattice CrossLink-NX    │               │
│           │           timing guard / trigger path  │               │
│           │                       ▲                │               │
│           ▼                       │                ▼               │
│      RF detector        SPI / IRQ / GPIO       Attenuator bank    │
│           │                       │                                │
│           └───────────────┬───────┘                                │
│                           ▼                                        │
│                    NXP i.MX RT1176                                 │
│            node mapping / policy / logging / BLE API              │
│                  ▲                    ▲                            │
│                  │                    │                            │
│              QSPI Flash          ESP32-C6 BLE/Wi-Fi               │
│                  │                    │                            │
│              Profiles            Mobile companion app              │
│                                                            USB-C   │
│                                  LiPo / charger / gauges          │
└────────────────────────────────────────────────────────────────────┘
```

### 5.1 Design Rationale

A pure SDR-style approach would make the device too fragile, too power-hungry, and too workflow-heavy for practical field use. A pure MCU-only bridge would not provide adequate isolation between timing-sensitive pass-through logic and operator-layer complexity. The FPGA-backed split preserves deterministic behavior while the MCU handles node modeling, policies, safe-mode logic, storage, and remote operation.

The architecture also acknowledges a practical truth: operators often need evidence more than they need raw packet captures. MoCA Phantom therefore prioritizes **node identity, privacy state, leakage indicators, channel behavior, and engagement profiles** rather than pretending the user always wants a giant undifferentiated RF dump.

---

## 6. Firmware Details and Design Decisions

The `firmware/` directory contains a **substantial compile-ready C control-plane simulation**. It is not a stub. It models the intended embedded stack and demonstrates:

- node discovery across a target MoCA domain,
- beacon, management, data, and probe frame handling,
- ruleset matching and bounded frame transformation,
- capture ring buffering,
- simulated storage flushes,
- radio telemetry uplink,
- power-state drift and safe-mode assertion,
- coax spectrum snapshot updates.

### 6.1 Firmware Modules

- `main.c` — boot flow, stream loop, discovery, summary reporting
- `board.h` — shared enums, metrics, node/frame/rule structures
- `registers.h` — modeled register map and control bits
- `drivers/moca.c` — demo stream generation, node discovery, frame encoding, capture ring
- `drivers/rule_engine.c` — policy matching and bounded manipulations
- `drivers/storage.c` — capture persistence model
- `drivers/radio.c` — BLE/Wi-Fi style status export
- `drivers/power.c` — battery and safe-mode trigger simulation
- `drivers/coax_monitor.c` — energy/leakage trend model
- `Makefile` — host-buildable simulation target

### 6.2 Core Firmware Philosophy

**Observation-first default:** the device should spend most of its life discovering and characterizing the MoCA plane before any active behavior occurs.

**Narrow actions, not broad chaos:** rules match on source, destination, traffic class, and channel. That is far safer than “rewrite everything from node X.”

**Safe-mode means actually safe:** low battery, operator assertion, or future timing faults should force the device away from aggressive manipulation modes.

**Useful evidence beats maximum complexity:** the firmware emphasizes clear events, node maps, and privacy findings. In real assessments, that produces more defensible reporting than a mountain of uncategorized captures.

**Battery-aware field operation:** the power model exists because this device is meant to be used in risers, closets, and rooms where wall power may be inconvenient or conspicuous.

### 6.3 Example Rules Included

The sample firmware demonstrates several realistic test behaviors:

- Tagging suspicious unencrypted bridge traffic for alerting.
- Rerouting a probe frame to another test endpoint during an authorized lab scenario.
- Forcing a fresh privacy key identifier to validate key-change handling.
- Rate-capping a target flow to observe resilience or anomaly detection.
- Adding bounded delay to a probe path to study path attribution and monitoring fidelity.

These are intentionally conservative examples. The design encourages **small, measurable manipulations** rather than broad traffic disruption.

---

## 7. Application / Software Interface

The companion app in `app/` is a lightweight React Native interface designed for field use. It avoids heavy navigation dependencies and instead uses a simple tabbed layout driven by React state. This keeps the app easy to audit and portable.

### 7.1 App Screens

- **Dashboard** — high-level device status, engagement summary, and current mode.
- **Node Map** — discovered MoCA nodes, privacy state, RF quality, and role labeling.
- **Profiles** — operator-selectable engagement profiles such as survey-only, riser audit, or adjacent-unit leakage investigation.
- **Capture Feed** — recent alerts, management observations, and notable events.
- **Safety** — legal reminder, safe-mode toggle state, and operator checklist.

### 7.2 App Workflow

A typical operator flow looks like this:

1. Connect to the device over BLE.
2. Review the current profile and legal warning.
3. Run a survey to discover nodes and identify privacy-disabled participants.
4. Open the Node Map to see controller/endpoint roles and suspected leakage.
5. If authorized, enable a more active profile with bounded manipulations.
6. Observe captures and export findings into the engagement record.

### 7.3 Protocol Model

The sample app includes a utility that converts profile names into a simple mock command bundle. In production, this layer would be replaced by a signed command protocol with role-based unlocks, explicit safe-mode acknowledgments, and event export to encrypted storage.

---

## 8. Use Cases for Red Teams, Security Researchers, and Penetration Testers

### 8.1 Hospitality Lateral Movement Validation

A red team assessing a hotel can use MoCA Phantom to determine whether guest-room coax drops expose shared media infrastructure. If multiple rooms participate in the same MoCA domain, that may create an unexpected path to set-top boxes, streaming appliances, or management gateways.

### 8.2 MDU Neighbor Isolation Testing

In an apartment or student housing engagement, the device can help determine whether splitters, filters, or riser design permit adjacent-unit leakage. This is especially valuable when defenders claim unit isolation but lack current coax documentation.

### 8.3 Broadband Equipment Hardening

A provider security team can use MoCA Phantom to validate whether field devices consistently enforce privacy, reject stale membership behavior, and generate meaningful alerts when an unauthorized node appears.

### 8.4 Executive Residence or Estate AV Review

High-end homes frequently contain hybrid AV/security ecosystems with media extenders, gateways, cameras, and automation components. MoCA Phantom helps determine whether the “TV wiring” has quietly become a trusted lateral path.

### 8.5 Lab Reproduction of Coax Threat Scenarios

Researchers can use the platform to build repeatable scenarios around privacy disablement, node impersonation indicators, weak isolation, and path leakage without depending entirely on closed provider test gear.

### 8.6 Blue-Team Detection Engineering

Defenders can use MoCA Phantom to create realistic training data for monitoring systems. If they want to know whether their environment notices a new MoCA node, fluctuating privacy state, or suspicious probe behavior, this device gives them a practical way to find out.

---

## 9. KiCad Design Package

The `kicad/` folder contains a project-level KiCad design skeleton with real component references, symbols, net naming, and representative connectivity for the device concept:

- `device.kicad_pro`
- `device.kicad_sch`
- `device.kicad_pcb`

The design files model the major subsystems: the MCU, FPGA, radio module, flash, charger, rails, and coax front-end connectors. The schematic is not presented as a manufacturing release, but it is structured as a realistic starting point for board development and review.

---

## 10. Threat-Informed Operational Concept

MoCA Phantom is not trying to be the most powerful RF platform on paper. It is trying to be the most useful **coax-specific security assessment instrument** in the field. That leads to a different set of priorities:

- Fast situational understanding over raw protocol trivia.
- Field survivability over lab-only complexity.
- Bounded, accountable actions over unlimited destructive potential.
- Explicit operator safety workflows over “trust the expert user.”

That matters because real assessments are constrained by time, access, visibility, and reporting burden. If an operator has ten minutes in a telecom closet or one guest-room maintenance window, they need something that can be inserted quickly, explain what it sees, and preserve evidence without demanding a rack of lab gear.

---

## 11. Security Controls and Defensive Design Features

Even though this is an offensive security research tool, the design includes built-in controls:

- **Safe-mode default** for low-power or guarded workflows.
- **Bypass path** to reduce the chance of accidental prolonged disruption.
- **Explicit profile model** so the operator knows whether the device is surveying or actively manipulating.
- **Battery and rail telemetry** to avoid undefined behavior during field use.
- **Operator-facing safety screen** in the companion app.
- **Narrow ruleset semantics** for more accountable testing.

These controls are critical if the platform is to be used professionally and ethically rather than as an uncontrolled experiment.

---

## 12. Future Extensions

Potential future revisions could add:

- richer MoCA topology fingerprinting,
- signed evidence bundles,
- optional wired Ethernet uplink for chained management,
- multi-profile rule packs for hospitality vs. residential vs. lab use,
- enclosure variants with hidden service filters,
- hardware attestation for sensitive consultancy workflows.

---

## 13. Build and Usage Notes

### Firmware

From `firmware/`:

```bash
make
./build/moca_phantom_sim
```

### Companion App

From `app/`:

```bash
npm install
npm start
```

The app is structured for Expo / React Native workflows and focuses on simple, auditable UI state.

---

## 14. Final Notes

MoCA Phantom is meant to push security work into a neglected but operationally significant corner of real infrastructure. Coax is not just legacy media wiring; in many environments it is a hidden layer-2 substrate with weak visibility and strong implicit trust. A device that can map, validate, and carefully test that substrate gives red teams, researchers, and defenders a way to answer questions that current tooling often ignores.

Used responsibly and only with authorization, MoCA Phantom provides a novel platform for discovering whether the organization’s “out-of-band media network” is actually an unmonitored attack path hiding in plain sight.
