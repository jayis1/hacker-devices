# eSPI Revenant — Inline eSPI Host / Embedded-Controller Interposer for Platform Trust-Boundary Research

![status](https://img.shields.io/badge/status-design-green) ![author](https://img.shields.io/badge/author-jayis1-orange) ![license](https://img.shields.io/badge/license-GPL--2.0-blue) ![hardware](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> Author: **jayis1**  
> Hardware License: **CERN-OHL-S v2**  
> Firmware/App License: **GPL-2.0-only**

---

## Legal and Ethical Notice

**eSPI Revenant is for authorized use only.** This design is intended strictly for defensive hardware security research, authorized red-team exercises, platform validation, embedded firmware assessment, secure-boot analysis, and laboratory study of host-to-embedded-controller trust boundaries. Using an interposer like this against computers, appliances, industrial HMIs, POS terminals, thin clients, servers, laptops, kiosks, or any other hardware that you do not own or have explicit written permission to assess may violate criminal law, civil law, tampering statutes, export restrictions, service agreements, workplace policy, and privacy obligations. The author, **jayis1**, provides this design for educational and authorized assessment only and assumes no liability for misuse, outages, damaged systems, corrupted firmware, unsafe modifications, or consequential loss.

Inline manipulation of eSPI traffic can affect reset sequencing, sleep states, keyboard controller behavior, flash transactions, thermal policy, watchdog events, and battery charging policy. In a real target, those controls may be intertwined with safety and data integrity. Use only within approved maintenance windows, with backups, rollback plans, spare hardware, and stakeholders who understand that platform instability can result from even carefully bounded experiments.

---

## 1. Device Purpose and Overview

**eSPI Revenant** is a novel hardware interposer built for a security problem that is both widespread and under-instrumented: the trust boundary between a modern x86/ARM host platform controller and the always-on embedded controller, Super I/O complex, or security-adjacent service logic that speaks **Enhanced Serial Peripheral Interface (eSPI)**.

The eSPI bus replaced LPC in many contemporary laptops, mini PCs, networking appliances, industrial computers, secure terminals, kiosks, and edge servers. While defenders often focus on UEFI images, TPM attestations, secure boot chains, or operating-system hardening, the host platform still depends on a lower-level conversation with sideband logic that decides whether a key press happened, whether the lid is closed, whether thermal events are credible, whether a battery is authentic, when the machine should enter S3/S0ix, and when flash accesses or out-of-band notifications should be trusted. That conversation is not abstract. It rides on wires: **CS#, CLK, IO0..IO3, ALERT#, RESET#**, and a set of virtual-wire semantics layered over a deceptively compact electrical interface.

The practical consequence is that systems with excellent software controls may still rely on a peripheral-management plane that few blue teams monitor and few red teams can instrument with timing precision. eSPI Revenant exists to make that invisible plane measurable, reproducible, and testable.

At a hardware level, eSPI Revenant is a **thin flex-mezzanine inline interposer** that sits between a host SoC/PCH and an embedded controller or eSPI-attached peripheral root. It preserves the original bus topology by default, but introduces deterministic capture, timestamping, trigger logic, and bounded channel manipulation under operator control. Unlike a generic logic analyzer, it is purpose-built to understand eSPI concepts:

- virtual-wire state transitions,
- peripheral-channel I/O and memory cycles,
- out-of-band (OOB) messages,
- flash-channel request and response timing,
- reset and alert choreography,
- and the policy implications of those events during boot, resume, firmware update, and degraded thermal or battery conditions.

The novel aspect of eSPI Revenant is not merely that it “sniffs” the bus. The device treats the host/EC link as a **live security control plane** and allows controlled experiments at that plane with hardware-enforced safety limits. It can, for example:

- capture the precise sequence of virtual wires during boot and sleep transitions,
- emulate small timing drifts in flash-channel completions to identify fragile host assumptions,
- inject bounded virtual-wire events such as wake or SCI/SMI-like stimuli to exercise firmware handlers,
- replay peripheral-channel keyboard-controller style transactions during tightly scoped windows,
- test whether platform firmware detects contradictory power-state assertions,
- correlate bus behavior with EC current draw and rail changes,
- and assess whether watchdog, secure-update, or battery-authentication logic depends on unauthenticated sideband assertions.

That is useful across multiple disciplines. For red teams, it creates a realistic lab instrument for validating whether a field device’s hardware trust anchors survive physical interposer attacks. For incident responders and secure-platform engineers, it offers a way to reproduce suspicious host/EC race conditions without reworking a motherboard. For firmware researchers, it fills the gap between protocol documents and actual stateful platform behavior.

In short, **eSPI Revenant turns the hidden motherboard conversation into a first-class assessment surface**.

---

## 2. Why This Device Matters

Modern systems increasingly split authority across silicon domains. A host CPU or PCH may enforce measured boot while an embedded controller handles power sequencing, button events, thermal emergencies, and housekeeping. Those two domains meet over eSPI. The protocol was designed for practical platform integration, not for adversarial transparency. In many products, if the EC says the lid is closed, the platform believes it. If the EC signals a thermal excursion, firmware may throttle or shut down. If the EC brokers flash ownership or update state, a narrow race can become a foothold.

From a defensive standpoint, organizations need tooling to answer questions such as:

1. Does the host verify impossible or contradictory virtual-wire state transitions?
2. Can the platform be induced into alternate boot paths through brief, bounded eSPI disturbances?
3. Are firmware-update modes gated by authenticated policy, or only by bus-visible conditions?
4. Does the EC expose hidden manufacturing, debugging, or recovery pathways when specific peripheral-channel sequences appear at resume time?
5. How often does security review stop at SPI flash contents and ignore the message fabric that mediates access to that flash?

Traditional analyzers can observe waveforms, but they usually do not understand platform semantics and they rarely provide a clean, repeatable control surface for red-team or product-security workflows. Generic FPGA dev boards can be adapted, but the setup burden is high and the safety characteristics are poor. eSPI Revenant is intentionally designed as a **portable, protocol-aware, safe-by-default motherboard interposer**.

The device is particularly relevant for:

- enterprise laptops and docking endpoints,
- kiosk and retail POS mainboards,
- industrial x86 controllers,
- fanless edge gateways,
- whitebox appliances,
- rugged tablets,
- automotive infotainment compute modules during bench analysis,
- and secure terminals where the EC/PCH boundary is central to trust.

Because those systems often experience field servicing, depot repair, supply-chain inspection, or after-hours maintenance access, the physical interposer attack model is realistic enough to merit serious validation.

---

## 3. Attack Surface and Threat Model

### 3.1 Primary Attack Surface

The device targets several tightly coupled surfaces at once:

1. **eSPI Virtual Wire Channel** — reset acknowledgment, sleep-state signaling, wake events, host warnings, error indicators, SUS signals, and generalized sideband truth claims.
2. **eSPI Peripheral Channel** — keyboard controller style cycles, ACPI/EC register access windows, legacy I/O semantics carried over eSPI.
3. **eSPI OOB Channel** — management or asynchronous sideband messages that may influence firmware or service processors.
4. **eSPI Flash Channel** — host/EC arbitration, response timing, suspend/resume behaviors, and assumptions during secure update or recovery.
5. **Reset and Alert Pins** — orchestration opportunities around `ALERT#`, `RESET#`, host suspend rails, and platform boot phases.
6. **Correlated Power / Thermal Context** — whether bus events line up with EC rail consumption, regulator sequencing, battery-auth states, or platform watchdog behavior.

### 3.2 Adversary / Operator Model

The assumed operator is an authorized researcher or red-team member with **bench access or temporary physical access** to a motherboard, daughterboard, or internal flex connector. They can insert an interposer between the host and target eSPI endpoint using board-specific adapter shims. No host credentials are required for passive capture. Active features are gated by a hardware arming switch and profile selection.

### 3.3 Security Questions eSPI Revenant Helps Answer

- Can virtual-wire assertions trigger privileged host behavior without authenticated provenance?
- Does the platform recover safely if flash-channel responses jitter during boot?
- Are battery, charger, lid, tamper, or keyboard events trusted too early in boot?
- Can an attacker force or prolong a recovery state by manipulating sideband timing?
- Does the EC expose undocumented maintenance commands through peripheral cycles?
- Are secure-boot indicators meaningful if the EC-side policy plane can still be confused?

### 3.4 Safety Boundaries

The design deliberately avoids “full arbitrary chaos” primitives. Active manipulation is bounded by:

- explicit profile arming,
- relay-style bypass and analog switch passthrough fallback,
- watchdog-driven reversion to passive mode,
- flash-channel delay ceilings,
- single-shot or rate-limited virtual-wire injections,
- and thermal/rail brownout protection for the interposer itself.

This matters because the goal is **measurement and repeatable validation**, not indiscriminate motherboard damage.

---

## 4. Hardware Specifications

| Category | Specification |
|---|---|
| Main timing fabric | **Lattice CrossLink-NX LIFCL-17** FPGA for bus capture, skew control, trigger timing, and safe channel arbitration |
| Control MCU | **STM32H723ZG** Cortex-M7 @ 550 MHz for policy engine, logging, USB, profiles, and safety interlocks |
| Wireless / auxiliary control | **ESP32-C6-MINI-1** for Wi-Fi 6 / BLE out-of-band control when a wired service lead is undesirable |
| Level translation | Dual auto-direction-aware but timing-bounded shifters plus discrete eSPI-tuned translators for 1.8 V / 3.3 V platforms |
| Inline switching | Low-capacitance analog switch matrix and fail-open/fail-through mux chain preserving host-to-EC path by default |
| Instrumentation | INA232 dual current/voltage monitor, comparator timestamp inputs, 2× fast ADC lanes for rail and edge-correlation capture |
| Storage | 256 Mbit QSPI NOR for local capture ring buffer, microSD footprint for extended traces |
| Connectors | Board-specific mezzanine interposer footprint, 40-pin flex adapter header, USB-C service port, Tag-Connect SWD, UART pads |
| Power input | USB-C 5 V, bench 3.3 V header, or target-rail parasitic monitor mode |
| Safety controls | Hardware arm switch, passive-only boot jumper, watchdog bypass, thermal sensor, TVS and series damping network |
| Form factor | 76 mm × 34 mm rigid-flex control board with short matched-length interposer tails |
| Indicators | Low-visibility RGB status LED, arming LED, buzzer-disable solder jumper |
| Attribution | All silkscreen, docs, metadata, and firmware identify **jayis1** as author/creator |

### 4.1 Bus-Physical Considerations

eSPI is high enough speed and timing-sensitive enough that the interposer cannot behave like a loose tangle of jumper wires. eSPI Revenant therefore uses:

- matched-length inline traces for `CLK`, `CS#`, `ALERT#`, `RESET#`, and `IO0..IO3`,
- configurable source damping resistors near the switch matrix,
- short flex-tail adapters for platform-specific insertion,
- and FPGA-side oversampling / retiming only when explicitly enabled.

This means the device can remain electrically transparent in passive mode, while still supporting deterministic active perturbations within bounded skew budgets.

### 4.2 Sensors and Correlation Inputs

The added instrumentation is a major differentiator. Beyond bus captures, eSPI Revenant samples:

- EC rail current draw,
- always-on rail presence,
- host suspend rail transition timing,
- interposer temperature,
- and optional lid-switch / button breakouts.

Those signals let a researcher correlate protocol events with physical platform state, which is often the only way to understand firmware race conditions.

---

## 5. Architecture and Block Diagram

### 5.1 System Architecture

```text
                 ┌──────────────────────────────────────────────────────────┐
 Host PCH / SoC  │                    eSPI Revenant                        │  EC / SuperIO /
 eSPI Master     │                                                          │  eSPI Peripheral
┌──────────────┐ │  ┌──────────────┐   ┌────────────────┐   ┌────────────┐ │ ┌──────────────┐
│ CS#, CLK,    ├─┼──┤ Trace Buffer ├───┤ CrossLink-NX   ├───┤ Switch /   ├─┼─┤ CS#, CLK,    │
│ IO0..IO3     │ │  │ + Trigger    │   │ Timing Fabric  │   │ Mux Matrix │ │ │ IO0..IO3     │
│ ALERT#,RST#  ├─┼──┤ Edge Capture ├───┤ Channel Parser │   └─────┬──────┘ │ ├──────────────┤
└──────────────┘ │  └──────────────┘   └───────┬────────┘         │        │ └──────────────┘
                 │                              │                  │        │
                 │                      ┌───────▼────────┐   ┌────▼──────┐ │
                 │                      │ STM32H723 MCU  │   │ Rail /    │ │
                 │                      │ Policy Engine  │   │ Current   │ │
                 │                      │ Logging / USB  │   │ Monitors  │ │
                 │                      └───────┬────────┘   └────┬──────┘ │
                 │                              │                 │         │
                 │                      ┌───────▼────────┐   ┌────▼──────┐ │
                 │                      │ ESP32-C6       │   │ QSPI NOR  │ │
                 │                      │ BLE/Wi-Fi App  │   │ + microSD │ │
                 │                      └───────┬────────┘   └───────────┘ │
                 │                           USB-C / UART / SWD            │
                 └──────────────────────────────────────────────────────────┘
```

### 5.2 Functional Partitioning

**FPGA fabric** handles edge-level timing, channel framing, trigger recognition, and active skew insertion. This keeps critical bus behavior deterministic even if the MCU is busy writing logs.

**MCU firmware** owns policy, safety, profile application, local storage management, app telemetry, and operator workflows.

**ESP32-C6** provides a non-invasive control surface for field use. In many bench scenarios, the device is used via USB-C only; the wireless layer exists so researchers are not forced to tether another cable to a partially disassembled target.

**Monitoring front-end** measures rails and side signals to enrich traces with real-world context.

### 5.3 Passive vs Active Modes

1. **Passive Mirror** — all traffic is observed, timestamped, and decoded; no bus modification occurs.
2. **Guarded Trigger Mode** — capture plus a single bounded action when a trigger condition matches, such as a specific virtual-wire sequence.
3. **Profiled Active Mode** — a formally described scenario with rate limits, timing ceilings, and automatic rollback.
4. **Fail-Safe Bypass** — direct passthrough after watchdog timeout, thermal event, or arm-switch release.

---

## 6. Firmware Details and Design Decisions

The firmware in `firmware/` is written in real, compile-ready C and authored by **jayis1**. It is intentionally structured for host compilation first, so the policy engine can be simulated and reviewed on Linux using GCC before anyone ports the abstractions to a specific board support package.

### 6.1 Firmware Modules

- `espi_bus` — models channel traffic, virtual wires, flash timing, and profile application.
- `interposer` — owns passthrough, skew windows, injection eligibility, watchdog fallback, and channel safety.
- `trace` — ring buffer capture, anomaly scoring, decode summaries, and export-friendly event formatting.
- `power` — rail, current, and temperature policy with rollback conditions.
- `radio` — compact frame protocol for the companion app and local command ingestion.
- `main.c` — orchestrates scenarios, prints deterministic transcripts, and demonstrates profile handling.

### 6.2 Design Principles

**Host-buildable logic first**: hardware-research tools become more trustworthy when their core logic can be compiled and tested without vendor IDE lock-in.

**Bounded active behavior**: the firmware does not offer unbounded arbitrary fuzzing by default. Instead, it encodes *profiles* that document what will happen, when, and under what safety limits.

**Stateful traceability**: every meaningful action is logged with timestamps, source, risk level, and a short reason string so lab notebooks and engagement reports stay reproducible.

**Clear separation between timing and policy**: bus-critical decisions are abstracted as if the FPGA executes them, while the host simulation reflects the same policy semantics. This keeps future hardware implementation aligned with the design package.

**Passive-safe boot**: the simulated and intended hardware both start in passive observation mode until explicitly armed.

### 6.3 Example Active Research Profiles

- `resume-glitch-window` — delays a narrow set of flash completions during resume to test EC/host error handling.
- `vw-contradiction-lab` — injects one contradictory virtual-wire pulse after a valid state transition to see whether host firmware sanity-checks it.
- `ec-maintenance-probe` — replays bounded peripheral-channel sequences that mimic authorized service interactions.
- `battery-policy-race` — correlates virtual-wire power events with rail consumption to examine charger/EC assumptions.
- `tamper-ack-loop` — rate-limited ALERT#/virtual-wire pattern testing for devices with tamper-detect EC logic.

### 6.4 Output and Telemetry

The firmware prints a human-readable simulation transcript but internally uses structured records that map cleanly to USB CDC, BLE GATT, or websocket transport. This makes the app and the firmware conceptually aligned.

---

## 7. Application / Software Interface

The companion application in `app/` is a self-contained browser-based control panel authored by **jayis1**. It avoids external dependencies so operators can open it directly from a forensic workstation or evidence laptop without pulling packages from the internet. While a production build could become a native mobile client or Electron app, the included interface is already useful and functional as a design artifact.

### 7.1 App Capabilities

- live status overview of link state, arming state, and active profile,
- trace feed showing decoded virtual-wire, peripheral, OOB, and flash events,
- scenario selection with risk descriptions,
- safety checklist and rollback indicators,
- rail telemetry and temperature margin display,
- and export-ready operator notes.

### 7.2 Interface Model

The app is structured as multiple sections rather than a single static dashboard:

1. **Overview** — current mode, trigger counts, last anomaly score, rail telemetry.
2. **Profiles** — selectable engagement presets with explicit effect summaries.
3. **Trace Feed** — decoded event timeline emphasizing what changed and why it matters.
4. **Control & Safety** — arm/disarm, passive-only mode, watchdog state, rollback conditions.
5. **Field Notes** — operator checklist items and assessment reminders.

### 7.3 Protocol Assumptions

The provided `app.js` consumes deterministic local mock telemetry that mirrors the firmware’s model. In a real deployment, the same object schema would arrive over BLE notifications or a USB serial bridge.

---

## 8. Use Cases for Red Teams, Security Researchers, and Penetration Testers

### 8.1 Red-Team Lab Validation

A red team with access to a target laptop model can insert eSPI Revenant on a spare board and ask whether physical interposition enables meaningful privilege influence without touching the SPI flash first. The goal is not necessarily exploitation in the field, but evidence-based understanding of platform resilience.

### 8.2 Secure-Boot and Recovery-State Analysis

Researchers can study whether the host enters special maintenance states when eSPI-side assumptions change briefly during boot or resume. This is especially valuable when secure boot is present but undocumented recovery pathways still exist.

### 8.3 EC Firmware Reverse Engineering Support

Because the tool captures the conversational context surrounding EC behavior, it helps reverse engineers map command semantics, side effects, and state transitions that would otherwise be hard to infer from firmware blobs alone.

### 8.4 Industrial / Appliance Mainboard Assessment

Thin clients, kiosks, edge gateways, and industrial HMIs often use embedded controllers for watchdog, thermal, or service logic. eSPI Revenant lets testers validate whether those devices can be confused into unsafe or maintenance-like states.

### 8.5 Incident Response Reproduction

When a vendor suspects intermittent host/EC race conditions, the device can reproduce boundary cases in a disciplined manner instead of relying on “it only fails once every fifty resumes” debugging.

### 8.6 Supply-Chain Security Research

Interposer tooling is useful for validating whether a malicious or substituted EC could influence platform behavior through documented but weakly validated sideband interactions. That makes the device relevant to depot-repair and refurbishment threat modeling.

---

## 9. Detailed Hardware Notes

### 9.1 Main Processing Elements

The **CrossLink-NX** was chosen because it balances low power, adequate I/O flexibility, and timing determinism in a compact package. It can sit close to the interposer traces and handle trigger-level operations without requiring a large, power-hungry FPGA.

The **STM32H723** provides enough headroom for capture management, local decode, structured logging, USB transport, and safety policy. It also has mature development tooling and enough peripherals to bridge the FPGA, QSPI storage, and wireless coprocessor cleanly.

The **ESP32-C6** is intentionally separated from timing-critical bus behavior. Its role is operator convenience, not protocol authority.

### 9.2 Adapter Philosophy

No single motherboard connector is universal. The baseboard therefore exposes a short, impedance-aware adapter interface. Device packs for specific laptop, appliance, or motherboard families would break out that interface into insertion shims, flex extenders, or test-clip harnesses.

### 9.3 Power Strategy

The interposer can run from USB-C in most bench scenarios. A passive parasitic mode exists for limited telemetry-only use, but active injection is disabled unless stable external power is present. This prevents the tool from becoming a new source of platform instability.

### 9.4 Storage Strategy

Short traces live in QSPI NOR for robustness and immediate retrieval. Longer bench sessions can stream to microSD. The local firmware model preserves both because field work often benefits from resilient ring-buffer storage even when removable media is absent.

---

## 10. Operational Workflow

A typical authorized workflow looks like this:

1. Identify the target board and appropriate adapter shim.
2. Insert eSPI Revenant inline between host and EC/peripheral endpoint.
3. Power the interposer from USB-C while leaving it in passive-only mode.
4. Capture a known-good boot and resume sequence.
5. Review decoded virtual wires, flash timing, and peripheral cycles.
6. Select a single bounded research profile.
7. Arm active mode physically and in software.
8. Execute the scenario, allowing the watchdog and rollback logic to protect the platform.
9. Compare traces between baseline and experiment.
10. Document whether the target validated, ignored, or catastrophically trusted the manipulated conditions.

This workflow is important because the tool is built for *methodical* security work, not novelty demonstrations.

---

## 11. Threat-Model Summary by Persona

| Persona | Access Level | What eSPI Revenant Helps Validate |
|---|---|---|
| Red-team hardware operator | Temporary authorized physical access | Whether the host/EC trust boundary can influence secure state or recovery behavior |
| Product security engineer | Bench access to development boards | Whether firmware robustly validates sideband events and timing edge cases |
| Incident responder | Failed or suspicious units | Whether anomalies align with host/EC communication faults |
| Embedded researcher | Full lab control | Protocol semantics, race conditions, and undocumented service flows |
| Penetration tester | Authorized appliance tear-down | Whether field-relevant physical access creates leverage beyond software attack surface |

---

## 12. Repository Layout

```text
espi-revenant/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── espi_bus.h / espi_bus.c
│       ├── interposer.h / interposer.c
│       ├── trace.h / trace.c
│       ├── power.h / power.c
│       └── radio.h / radio.c
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── index.html
    ├── styles.css
    └── app.js
```

---

## 13. Future Extensions

Potential future work includes:

- board-specific adapter packs for common laptop families,
- direct protocol export into Saleae / Sigrok compatible formats,
- TPM locality-correlation triggers when eSPI and other sideband domains interact,
- machine-learning clustering of resume/boot path anomalies,
- and secure evidence signing for captured traces.

These are extensions, not prerequisites. The core design already stands on its own as a novel and practical research device.

---

## 14. Conclusion

**eSPI Revenant** is a purpose-built inline interposer for one of the least visible but most security-relevant hardware trust boundaries in modern systems. It is original in concept, practical in deployment, and disciplined in safety. By combining deterministic timing hardware, a substantial host-buildable firmware model, rail-aware instrumentation, and an operator-friendly control surface, it gives authorized security teams a way to study how real platforms behave when the host/EC conversation is no longer taken on faith.

That matters because many compromises, persistence mechanisms, and resilience failures do not begin in the operating system. They begin in the assumptions below it.

**Created by jayis1.**
