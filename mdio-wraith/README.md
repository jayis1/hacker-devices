# MDIO Wraith — Inline Ethernet PHY Management-Plane Interposer for Clause 22 / Clause 45 Security Research

![status](https://img.shields.io/badge/status-design-green) ![author](https://img.shields.io/badge/author-jayis1-orange) ![license](https://img.shields.io/badge/license-GPL--2.0-blue) ![hardware](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> Author: **jayis1**  
> Hardware License: **CERN-OHL-S v2**  
> Firmware/App License: **GPL-2.0-only**

---

## Legal and Ethical Notice

**MDIO Wraith is for authorized use only.** This device is intended strictly for defensive hardware security research, authorized penetration testing, sanctioned red-team operations, network equipment validation, and laboratory protocol analysis. Do not deploy, inline, or test this design against switches, routers, industrial Ethernet controllers, IP cameras, power systems, access-control appliances, building automation gear, medical devices, or any other equipment without explicit written authorization from the owner and stakeholders responsible for safety and availability. The author, **jayis1**, provides this design for educational and authorized assessment only and assumes no liability for misuse, outages, damaged equipment, broken network links, service interruptions, corrupted non-volatile PHY settings, safety incidents, compliance violations, or consequential loss.

MDIO is often treated as a harmless service bus, but in deployed hardware it can control PHY resets, loopback state, port isolation, energy-saving behavior, strap shadow registers, link advertisement, cable diagnostics, and vendor-specific debug pages. Those functions can change how field devices behave even when the operating system and forwarding plane remain unchanged. Use only on bench targets, spares, or engagement systems whose owners understand the possible impact.

---

## 1. Purpose and Overview

**MDIO Wraith** is a portable inline interposer and management-plane analysis platform designed to expose one of the least monitored but most security-relevant surfaces inside network-connected embedded hardware: the **Ethernet PHY control plane**. A large amount of red-team and product-security work concentrates on higher layers such as web interfaces, SSH services, routing daemons, containerized control planes, industrial protocols, bootloaders, or secure-storage chips. Those layers matter, but they all depend on lower-level assumptions about whether a network link exists, what speed the link negotiated, whether loopback is active, which PHY strap personality was latched, and which vendor debug pages are accessible. That lower layer is commonly governed through **MDIO/MDC** using IEEE Clause 22 or Clause 45 register transactions.

In practical terms, network devices trust their PHYs. A switch ASIC, router SoC, BMC, IP camera processor, or industrial controller often reads status from a PHY and accepts it as ground truth. Management software may log a link-down event, change spanning-tree behavior, reinitialize a MAC, switch to a failover circuit, or expose diagnostics to an administrator based on values coming from that tiny management bus. In many designs, the same bus also reaches hidden vendor registers that alter LED behavior, downshift logic, loopback, cable test engines, energy-efficient Ethernet policy, SGMII bridging settings, or alternate strap banks. If a security team cannot observe or safely manipulate that path, it cannot fully validate trust assumptions at the physical network boundary.

MDIO Wraith solves that problem by acting as a **protocol-aware man-in-the-middle for Ethernet PHY management traffic**. It sits inline between the host side and the PHY side of an MDIO link, preserving direct pass-through by default while adding four capabilities that ordinary analyzers usually lack:

1. **Deterministic MDIO capture** with timestamps and register-aware decoding.
2. **Guarded inline modification** of selected writes under a hardware-enforced safety policy.
3. **Correlated power and rail telemetry** so a researcher can see whether management-plane activity lines up with target current draw or reset behavior.
4. **Profile-driven experiments** that make repeatable red-team or validation workflows possible without turning the interposer into a reckless fuzzing box.

The novel part of MDIO Wraith is not just that it watches MDIO. The device treats the PHY control plane as a **security boundary worth instrumenting like a bus protocol, not just a maintenance interface**. That matters because many field devices embed assumptions such as:

- “link state is trustworthy,”
- “loopback is only used in factory test,”
- “vendor page changes require trusted firmware,”
- “port isolation or strap overrides cannot happen in service,”
- “the PHY register image reflects physical reality,”
- and “management software will notice if PHY state becomes contradictory.”

Those assumptions are not always true.

MDIO Wraith is aimed at systems where Ethernet is central to function and the PHY boundary is near security-critical logic, including:

- branch routers and firewalls,
- managed switches,
- industrial gateways and PLC-adjacent controllers,
- access-control panels,
- PoE appliances,
- cameras and NVR boards,
- BMC-equipped servers,
- kiosk networking backplanes,
- automotive Ethernet lab targets,
- and custom embedded Linux products that use discrete PHYs.

It gives red teams and defenders a way to ask a deeper class of question: **what happens if the physical link truth itself becomes adversarial, or simply inconsistent?**

---

## 2. Security Problem This Device Addresses

The Ethernet PHY is often invisible in architecture diagrams even though it is the first active silicon element behind an RJ45, magnetics module, or copper side of an SFP cage. That PHY negotiates speed and duplex, exposes cable state, reports link failures, surfaces energy-saving changes, and often hosts vendor-specific features that a system integrator barely documents. Software above it may assume the PHY is honest because it lives on the same board. However, that trust is fragile in several realistic situations:

- depot repair or supply-chain handling,
- contract manufacturing debug fixtures left populated,
- field servicing by third parties,
- authorized red-team hardware validation,
- product-security lab analysis,
- or incident response where an operator suspects hardware-assisted tampering.

MDIO is particularly attractive as a research target because it tends to be **low pin-count, widespread, semantically rich, and lightly defended**. Compared with attacking an entire switch ASIC data plane, influencing or studying the management bus is far more accessible. A small inline interposer can observe all register transactions and reproduce conditions such as:

- whether a host periodically polls link status or only reads it at boot,
- whether a device reacts safely when link and autonegotiation states contradict each other,
- whether vendor page selection is exposed too broadly,
- whether loopback or isolate bits have operational consequences beyond diagnostics,
- whether a backup path, HA member, or radio uplink is trusted too quickly after a PHY event,
- and whether changes to PoE-adjacent PHYs affect broader device behavior.

This is not theoretical. Real products use PHY register bits to drive LEDs, watchdog assumptions, redundancy logic, QoS profile changes, traffic-shaping presets, and fault annunciation. On an industrial endpoint or a camera platform, that can alter operator belief about connectivity or state even before packet-level evidence is considered.

MDIO Wraith is therefore a device for **trust-boundary validation at Layer 1.5**: not raw copper waveform injection, and not high-level packet tampering, but the management layer where physical-link meaning becomes software policy.

---

## 3. Attack Surface and Threat Model

### 3.1 Primary Attack Surface

MDIO Wraith targets several interrelated surfaces:

1. **Clause 22 register transactions** — classic BMCR/BMSR/PHYID/ANER/vendor-page access paths.
2. **Clause 45 MMD access** — extended device addressing used in more advanced PHYs for PMA/PCS/autoneg/vendor blocks.
3. **PHY strap shadowing** — systems where latched boot straps can also be reflected or altered through vendor pages.
4. **Host polling logic** — software that assumes register reads correspond directly to physical state.
5. **PHY reset and power sequencing** — whether resets, power anomalies, or page changes alter system trust.
6. **Correlated telemetry surfaces** — current draw, target rail stability, and timing around state changes.

### 3.2 Assumed Operator Model

The assumed operator is an **authorized researcher with bench or maintenance access** to the target PCB, test header, interposer flex point, or PHY breakout path. They may be reverse-engineering a switch mainboard, validating a secure router, testing a PoE access panel, or reproducing an observed fault. Passive capture requires only inline connection. Active modification requires both profile selection and a hardware arm condition.

### 3.3 Questions MDIO Wraith Helps Answer

- Does the host notice impossible link-status transitions?
- Are vendor pages polled or trusted in production mode?
- Can one-time loopback or isolate writes force a security-relevant state change?
- Does software distinguish physical cable failure from management-plane deception?
- Are safety systems, camera failover, ring redundancy, or industrial alarms driven by unauthenticated PHY state?
- Can a discrete PHY expose hidden manufacturing paths through MDIO pages that field software never audits?

### 3.4 Safety Boundaries

MDIO Wraith is deliberately not an unrestricted destructive fuzzing platform. Safety controls include:

- passive boot by default,
- write budgets per profile,
- thermal and current limits,
- block rules for dangerous writes such as power-down,
- watchdog-driven bypass fallback,
- rate-limited one-shot triggers,
- explicit operator arming for active profiles,
- and a defined fail-safe path to direct host-to-PHY pass-through.

The result is a tool for **repeatable research**, not arbitrary disruption.

---

## 4. Hardware Specifications

| Category | Specification |
|---|---|
| Timing / interposer fabric | **Lattice CrossLink-NX class FPGA** for bidirectional MDIO state capture, clock-domain isolation, edge filtering, and deterministic gating |
| Control MCU | **STM32H723ZG** Cortex-M7 for policy engine, USB service console, profile management, logging, and safety logic |
| Auxiliary wireless control | **ESP32-C6-MINI-1** for optional Wi-Fi/BLE out-of-band telemetry and local tablet control |
| Inline buses | **MDIO/MDC** primary, with optional GPIO strap sense lines and reset observation |
| Instrumentation | **INA232** current/voltage monitor on target-side rail, comparator timestamp inputs, temperature sensor |
| Storage | QSPI NOR for trace ring buffer and profile persistence; optional microSD footprint for long captures |
| Connectors | Host-side MDIO clip/header, PHY-side MDIO clip/header, USB-C service port, SWD pads, UART pads, test loops for reset and strap lines |
| Power | USB-C 5 V input with local 3.3 V regulation, or target-powered monitor mode with isolation policy |
| Protection | TVS array, series damping on MDIO/MDC, current-shunt monitor, arm switch, watchdog bypass control |
| Form factor | Small rigid board with dual short cable/clip leads suitable for bench insertion into target test headers or interposer harnesses |
| Intended attribution | All silkscreen, metadata, docs, firmware, and application content identify **jayis1** as the author |

### 4.1 Physical Design Considerations

MDIO is lower speed than many serial buses, but inline integrity still matters. Long jumpers, high-capacitance probes, or poorly chosen pull-ups can change waveforms enough to create false conclusions. MDIO Wraith therefore assumes:

- short matched service leads,
- selectable pull-up domains,
- controlled series resistors near the switch path,
- and FPGA-side sampling that mirrors edges without becoming the default active driver.

### 4.2 Why the Mixed FPGA + MCU Architecture

An MCU alone can log a lot of useful data, but when a researcher wants deterministic inline behavior—especially around turnaround timing, bus arbitration, or one-shot guarded writes—timing should not depend on firmware interrupt latency. The FPGA is responsible for near-wire behavior; the MCU handles policy, UX, logging, and guardrails. The ESP32-C6 exists so a companion app can control the device without adding another service cable to an already crowded bench setup.

---

## 5. Architecture and Block Diagram

```text
Host SoC / Switch ASIC / BMC                        Target PHY / PHY Chain
MDIO master side                                     MDIO peripheral side
┌────────────────────┐                           ┌────────────────────────┐
│ MDC, MDIO, RESET#  │                           │ MDC, MDIO, RESET#, LED │
└─────────┬──────────┘                           └──────────┬─────────────┘
          │                                                 │
          │      ┌──────────────────────────────────────┐   │
          └──────┤     MDIO Wraith inline interposer    ├───┘
                 │                                      │
                 │  Host clip ──> analog switch/mux ──> PHY clip
                 │                    │
                 │              CrossLink-NX FPGA
                 │          decode / trigger / gate / mirror
                 │                    │
                 │               STM32H723 MCU
                 │          policy / logging / USB / safety
                 │                    │
                 │               ESP32-C6 module
                 │             BLE / Wi-Fi console
                 │                    │
                 │          INA232 + temp + rail monitor
                 │                    │
                 │         QSPI NOR / optional microSD
                 └──────────────────────────────────────┘
```

### Functional Partitioning

- **Inline switch path:** preserves direct host-to-PHY communication in passive mode and fail-safe bypass.
- **FPGA timing fabric:** decodes MDIO frames, watches address/register/value patterns, and enforces deterministic trigger windows.
- **MCU policy engine:** decides whether a planned write is allowed under the selected profile and current safety conditions.
- **Telemetry plane:** correlates register events with current draw, voltage drift, and temperature.
- **Companion app:** provides profile loading, capture review, safety acknowledgement, and register staging.

### Operating Modes

1. **Passive Mirror** — capture and decode only.
2. **Guarded Trigger Mode** — one bounded action after a selected condition.
3. **Profiled Active Mode** — explicit write budget and experiment flow.
4. **Fail-Safe Bypass** — hard rollback to direct pass-through.

---

## 6. Firmware Design and Decisions

The firmware in `firmware/` is authored by **jayis1** and intentionally written as **compile-ready host-simulation C** first. That means the policy engine, capture model, telemetry logic, and profile behavior can be built and reviewed with GCC on Linux before adaptation to a board support package. This improves auditability and avoids burying the interesting logic inside vendor IDE metadata.

### 6.1 Module Overview

- `main.c` — orchestrates end-to-end simulation, profile execution, status rendering, and ethics banner output.
- `mdio_bus.c` — models PHY inventory, Clause 22 and Clause 45 reads, guarded writes, capture storage, trigger detection, and vendor-page snapshots.
- `profile.c` — defines reusable research profiles such as passive fingerprinting, isolate-bit testing, loopback diversion, and strap shadowing.
- `telemetry.c` — tracks board temperature, target current, target voltage, and JSON-friendly status export.
- `radio.c` — formats compact frames for the companion application or wireless service channel.
- `safety.c` — enforces arming rules, write budgets, thermal/current/voltage policy, and fail-safe rollback.

### 6.2 Core Firmware Principles

**Policy before power.** The firmware will not perform any active write simply because the transport can. It evaluates profile requirements, arming state, write budget, and register-specific block rules first.

**Host-buildable logic.** Researchers can run `make` locally and inspect deterministic output before touching a real target.

**Stateful logging.** Each event captures a timestamp, code, channel, risk, and reason string so a test can be reproduced later.

**Vendor-page awareness.** Many practical PHY questions involve non-standard registers. The simulation therefore includes snapshot support and profile-driven page actions.

**Separation of timing and policy.** The code assumes FPGA-side timing enforcement for real hardware, while the simulation mirrors the same semantics at a higher level.

### 6.3 Example Research Profiles

#### `phy-fingerprint`
Passive inventory profile. Scans all discovered PHYs, reads key identity and status registers, and snapshots vendor registers without performing any writes.

#### `isolated-link-drop`
A guarded experiment that waits for a selected link-up condition and then performs a single isolate-bit write to test how the host handles a sudden management-plane-induced disconnect.

#### `loopback-diversion`
Performs one bounded loopback plus restart-autoneg write to study whether management software or failover logic trusts link indications too broadly.

#### `strap-shadow`
Targets vendor-page behavior and strap-bank shadowing. Useful for lab work on PHYs that reflect boot personality or debugging state through MDIO-accessible registers.

### 6.4 Safety in Firmware

The safety layer blocks power-down writes, enforces profile budgets, disables active behavior on thermal or rail anomalies, and can force a bypass rollback. Those controls matter because the PHY control plane often touches systems whose availability is operationally significant.

---

## 7. Companion Application / Software Interface

The `app/` directory contains a real static companion console authored by **jayis1**. It uses HTML, CSS, and JavaScript so the interface can be opened immediately in a browser without a framework install. That was an intentional choice for a field-friendly research tool: many users want a simple service laptop or tablet interface, not a heavy dependency chain.

### App Capabilities

- Dashboard cards for live mode, voltage, current, temperature, anomalies, and attribution.
- Profile browser with load/select behavior.
- Capture feed showing decoded events and operator actions.
- Register staging panel for simulated writes or safety-denied actions.
- Ethics screen with authorized-use reminders.
- Guarded arm / fail-safe bypass actions.

### Intended Protocol Behavior

The firmware’s `radio.c` module emits compact heartbeat and capture-export frames. In real hardware, the app would subscribe over BLE, Wi-Fi, or USB serial to those frames and render them into the same UI structure already represented in the static demo.

### Why a Static App Instead of a Large Framework

A static browser app is easier to inspect, easier to archive in an engagement package, and easier to adapt into a future React Native or Electron front end if the user later wants mobile packaging. The current implementation still includes real screens and real interactions rather than placeholder mockups.

---

## 8. KiCad Design Files

The `kicad/` directory includes:

- `device.kicad_sch` — a schematic describing the host clip, PHY clip, FPGA timing fabric, STM32H723 controller, ESP32-C6 auxiliary module, and INA232 telemetry path with named nets.
- `device.kicad_pcb` — a board file with placed footprints, net assignments, routed core connections, and a defined board outline.
- `device.kicad_pro` — project metadata and net class settings.

This is a design package, not a manufacturing release. A final fabrication-ready revision would add decoupling density, impedance review, DRC cleanup, connector strain-relief considerations, and target-specific adapter details.

---

## 9. Practical Red-Team and Research Use Cases

### 9.1 Network Appliance Validation
Use MDIO Wraith to verify whether a firewall, SD-WAN appliance, or router reacts safely when a PHY suddenly reports contradictory link/autoneg state without corresponding packet evidence.

### 9.2 Managed Switch Testing
Study whether management software trusts PHY pages that influence diagnostics, loopback, cable tests, or fault LEDs. Validate whether alerting distinguishes maintenance activity from malicious or corrupted register state.

### 9.3 PoE and Access Infrastructure
Assess embedded devices where PHY status affects camera availability, access-panel connectivity, or edge controller redundancy. Correlate current draw with state changes to spot deeper hardware reactions.

### 9.4 Industrial Ethernet Research
Examine whether a controller or gateway changes safety or service behavior when the PHY control plane becomes inconsistent. This is especially useful for bench-only validation of failover assumptions and engineering-station diagnostics.

### 9.5 Supply-Chain and Depot Inspection
A defender can place MDIO Wraith inline during acceptance testing to compare an expected PHY fingerprint and vendor-register baseline against the actual board under evaluation.

### 9.6 Incident Response
When a fielded device shows unexplained link flapping, abnormal failover, or contradictory PHY telemetry, MDIO Wraith can reproduce or capture the sequence without immediately swapping the entire mainboard.

---

## 10. Build and Simulation Workflow

### Firmware

```bash
cd firmware
make
./build/mdio_wraith_sim
```

The included build produces a host-executable simulation that:

- prints the device banner,
- inventories PHYs,
- executes each profile,
- records capture data,
- emits heartbeat/export frames,
- and demonstrates bypass rollback at the end.

### App

Open `app/index.html` in a browser. No bundler is required for the current console.

### Hardware

Open the KiCad project in KiCad 8 or later, review named nets and footprint assignments, and extend the adapter harness for the target platform you intend to assess.

---

## 11. Threats, Limitations, and Future Work

MDIO Wraith is intentionally focused on the management plane, not packet forwarding or full copper-layer signal injection. That means it will not replace a TAP, TDR, oscilloscope, SGMII analyzer, or high-speed SerDes interposer. It also assumes the host actually uses MDIO in a visible way; some integrated systems hide or multiplex management access behind switch silicon or internal buses.

Other limitations:

- Vendor pages differ sharply between PHY families.
- Some targets use strap-only behaviors that cannot be changed safely after boot.
- Invasive inline access may still require target-specific fixtures.
- Production-safe use in critical infrastructure demands strict change windows and rollback plans.

Future work that would fit this design:

- board-specific flex interposers for common PHY packages,
- cable-side link pulse correlation,
- SGMII/USXGMII sideband awareness,
- signed experiment profiles,
- and richer diffing of vendor-register baselines across device fleets.

---

## 12. Summary

**MDIO Wraith** is a new hardware design for a real gap in security research: **trust validation of the Ethernet PHY management plane**. It combines an inline interposer path, protocol-aware FPGA timing, MCU-enforced safety policy, rail telemetry, a browser-based companion console, and a host-buildable firmware simulation. That makes it useful to red teams, product-security engineers, defenders, and incident responders who need to understand whether a device’s idea of network reality can be observed, challenged, or safely tested below the packet layer.

Everything in this design package—documentation, firmware, app, and hardware metadata—credits **jayis1** as the author.
