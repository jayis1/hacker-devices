# Type-C Specter

**Author:** jayis1  
**Copyright:** Copyright (c) 2026 jayis1  
**Category:** USB Type-C / Power Delivery security research platform

## Legal and Ethical Disclaimer

Type-C Specter is designed strictly for **authorized security research, defensive validation, red-team simulation, and hardware testing**. It is not intended for unauthorized intrusion, destructive misuse, or deployment against systems, chargers, docks, phones, laptops, kiosks, vehicles, embedded products, or industrial equipment without explicit written permission from the owner. USB Type-C combines data, power, sideband signaling, and policy negotiation; abusing those capabilities on unauthorized targets may cause safety incidents, hardware damage, data loss, policy violations, and legal exposure. Use this design only in controlled environments or under signed rules of engagement.

## Device Purpose and Overview

Type-C Specter is a novel inline security-research instrument for studying trust decisions made at the **USB Type-C and USB Power Delivery policy layer**. Most USB attack tools focus on payload delivery after enumeration, cable implants, or generic protocol fuzzing against visible USB buses. Type-C Specter instead targets the earlier trust boundary: the moment a host, device, charger, dock, kiosk, or embedded target decides what is connected, who supplies power, who consumes power, whether a cable is electronically marked, whether VCONN should be enabled, whether a data role or power role swap is acceptable, and whether alternate-mode or debug-like behavior should be permitted.

That early decision point is security relevant because many systems assume the cable and PD environment are benign. A laptop may trust a dock enough to load additional drivers. A mobile device may expose a maintenance interface depending on attach state. An embedded target may choose a risky fallback path when brownout, current limit, or role confusion occurs. A kiosk may enter recovery or service behavior when a malformed accessory appears during boot. Those are all policy-layer attack surfaces that traditional pentest kits often do not exercise well.

Type-C Specter is designed as an **adversarial policy proxy**. The device sits inline between a target and a peer accessory, charger, cable emulator, or operator-controlled virtual profile. It observes and can selectively manipulate:

- CC1 / CC2 attach semantics
- USB-PD capability exchange and identity traffic
- cable identity and electronically marked cable claims
- VCONN-related transitions
- USB2 D+/D− path availability
- SBU sideband connectivity
- power-path conditions such as measured current, voltage, and bounded starvation
- operator-scripted timing windows for race and rollback testing

The platform therefore acts as a hybrid of inline tap, PD policy manipulator, power safety logger, cable identity emulator, and repeatable red-team scenario runner.

## Why the Device Is Original

Type-C Specter is not a generic USB analyzer with a new name. Its novelty is that it treats Type-C trust primitives as first-class adversarial controls for security operations. Many legitimate engineering tools can decode PD messages, but far fewer can deliberately model realistic security scenarios such as:

- transparent proxying a legitimate dock and then changing identity after host trust has been established
- advertising a cable profile inconsistent with measured electrical behavior
- triggering tightly timed role-swap requests around natural policy transitions
- asserting debug-accessory-like conditions for short windows to identify hidden service behavior
- reducing available current in a bounded, logged, safety-limited way to study maintenance or recovery paths
- isolating USB2 or SBU paths while preserving partial policy behavior to observe fallback logic

The concept is practical for red teams because USB-C has become the default interface for enterprise laptops, tablets, field gear, rugged embedded devices, AV systems, conference-room docks, charging carts, and service ports. Defenders need a way to simulate malicious chargers, malicious docks, malicious active cables, and compromised accessories without immediately jumping into full custom hardware development for each assessment.

## Threat Model and Attack Surface

### Security Questions Under Test

Type-C Specter is built to explore security questions such as:

1. Does the target validate accessory identity only once, or continuously?
2. Can the target be coerced into a different power role or data role under race conditions?
3. Does the target expose a debug, service, or recovery path if attach semantics resemble a factory or accessory mode?
4. Do cable e-marker claims cause a host to trust paths it should not trust?
5. How does the target react if current availability, VCONN assumptions, or sideband paths change mid-session?
6. Are pre-OS USB-C policy engines more permissive than the operating system that later takes over?
7. Are there instability or reboot loops that could become persistence or denial-of-service opportunities?

### Targeted Attack Surface Areas

Type-C Specter exercises multiple layers:

- **CC attach logic:** Rp/Rd/Ra interpretation, orientation, accessory detection, source/sink assumptions.
- **PD policy engine:** hard reset, soft reset, capabilities exchange, identity responses, structured and vendor-defined messaging.
- **Cable trust:** active cable claims, current rating assumptions, SOP’ and SOP’’ identity handling.
- **Power path:** VBUS source/sink transitions, current limiting, brownout recovery, over-voltage protection interactions.
- **Alternate modes and sidebands:** SBU behavior, entry negotiation dependencies, mode fallback.
- **Host/firmware glue:** BIOS or embedded controller policy, UCSI/TCPC firmware, operating-system dock handling, charger management daemons.

### Adversaries Simulated

The device is designed to model several real adversary classes:

- a malicious charger that alters capabilities or timing
- a malicious dock that changes identity or role behavior after trust is granted
- a malicious electronically marked cable that lies about capabilities
- a compromised accessory in a supply-chain scenario
- a field operator performing an authorized hardware red-team simulation

## Hardware Specifications

### Core Processing

- **Primary MCU:** STM32H743VI, Cortex-M7 at up to 480 MHz
- **Timing Coprocessor:** Lattice iCE40UP5K FPGA for deterministic CC-state timing, lane gating, and packet assist
- **External storage:** 32 MB QSPI NOR flash for scenarios, captures, and signed profile bundles
- **Secure element:** ATECC608B for operator profile protection and evidence-tag signing

### Type-C / Power Components

- dual USB Type-C receptacles: target side and peer side
- PD front-end controllers on both sides compatible with BMC signaling capture and injection
- USB2 high-speed analog mux for pass-through, isolation, and mirrored monitoring modes
- SBU analog switch matrix for pass-through and observe/isolate experimentation
- VBUS ideal-diode / power-path controller with discharge network and current limit control
- dual rail power monitor ICs for target-side and peer-side VBUS telemetry
- temperature sensing near the hot path for thermal shutdown logging

### Connectivity and I/O

- Wi-Fi 6 / BLE 5.3 companion radio module
- service USB-C port for management, charging, and offline updates
- microSD slot for trace export and scenario libraries
- SWD/JTAG and FPGA programming pads
- 1.54-inch e-paper display for low-power field status
- three buttons: Arm, Trigger, Safe
- RGB LED status bar and piezo buzzer for lab-safe alerts

### Power

- 3.7 V 2200 mAh Li-ion internal battery
- USB-C PD sink charging on service port
- 5 V, 3.3 V, and auxiliary low-noise rails
- hardware fuse and firmware-enforced safe fallback
- boot default: transparent passive monitor mode

### Form Factor

- approximately 88 mm x 54 mm x 14 mm
- inline pocketable enclosure with opposing USB-C ports
- internal metal frame with shield segmentation for radio and signal integrity
- ruggedized enough for bench work and mobile assessments

## Architecture

Type-C Specter separates safety-sensitive orchestration from timing-critical line control.

```text
[Target USB-C] -- CC/PD Front End A --+
                                       +--> [iCE40UP5K timing fabric] --> [USB2 mux]
[Peer USB-C]   -- CC/PD Front End B --+                                --> [SBU mux]
                                                       |
                                                       v
                                                [STM32H743 control MCU]
                                               /    |      |       \
                                        [QSPI] [ATECC] [ESP32-C6] [power telemetry]
```

### Functional Blocks

1. **CC/PD Interception Layer**  
   Front ends expose attach state and PD traffic from both cable sides. The FPGA timestamps critical edges and helps enforce deterministic pass-through or mutation windows.

2. **Policy Engine Layer**  
   The MCU selects a scenario, decides when mutation is permitted, applies operator limits, and records evidence. It can proxy a baseline profile or pivot to a scripted identity.

3. **Signal Steering Layer**  
   USB2 and SBU switching let the device keep power and policy conditions stable while changing data availability or sideband continuity.

4. **Power Safety Layer**  
   Current, voltage, and thermal telemetry are sampled continuously. Unsafe conditions abort all mutation logic and force transparent mode.

5. **Operator Interface Layer**  
   The app exposes scenario selection, cable profile browsing, evidence export, and an ethics acknowledgement gate.

## Firmware Design

The firmware in this repository is a compile-ready C reference implementation that models the control plane and scenario engine. It is structured as a host-buildable simulation so that logic is reviewable and testable before binding to silicon SDKs.

### Design Goals

- default to safe transparent operation
- make all active manipulations explicit and auditable
- support repeatable scenario execution rather than ad hoc operator poking
- keep power safety independent from scenario logic
- produce structured logs useful for after-action review
- expose a clean path for eventual porting to real MCUs and PD front ends

### Firmware Modules

- `main.c` – runtime loop, status updates, event logging, demo workflow
- `board.h` – shared types, limits, runtime structures
- `registers.h` – simulated register map and bitfields
- `drivers/pd_phy.*` – attach state model, identity profile handling, simulated PD packets
- `drivers/cc_mux.*` – USB2 and SBU signal-path mode control
- `drivers/vbus_meter.*` – current limit model and power sampling
- `drivers/script_engine.*` – scenario definitions and timed action sequencing
- `drivers/radio.*` – operator command queue abstraction and status publication

### Key Firmware Behaviors

- **Transparent startup:** mutation disabled until explicitly armed.
- **Scenario packs:** named offensive/validation workflows such as `dock_identity_flip`, `late_vconn_claim`, `role_swap_race`, `debug_accessory_probe`, and `power_starve_then_recover`.
- **Time-bounded actions:** mutation windows are temporary and scenario-driven.
- **Safety interlock:** over-current, out-of-range voltage, or excessive temperature immediately aborts the scenario and restores pass-through behavior.
- **Operator command channel:** status, scenario arming, profile loading, and current-limit updates are modeled through the radio layer.

## Companion Application

The app is implemented as a static web console in `app/`. That choice is deliberate: it can run from a field laptop, tablet browser, or a thin local web view without requiring a heavyweight mobile build step for concept validation.

### Included Screens

- **Dashboard:** attachment and telemetry summary, plus safety status
- **Scenarios:** cards for scenario packs with arm actions
- **Cable Profiles:** saved profile table for transparent and adversarial identities
- **Logs:** timeline and export controls
- **Ethics:** acknowledgement gate that prevents scenario arming until the operator confirms authorized use

### Operator Workflow

1. Connect to the device.
2. Review live attachment and safety state.
3. Acknowledge the authorized-use gate.
4. Select a scenario.
5. Run and monitor a bounded mutation sequence.
6. Export JSON evidence for reporting.

## Practical Use Cases

### Red Team Operations

- test enterprise laptops against hostile charger and dock trust decisions
- simulate compromised conference-room docking infrastructure
- assess rugged tablets and field laptops with USB-C service and charging ports
- validate mobile-device behavior around identity changes and role-swap requests

### Security Research

- characterize vendor-specific PD policy differences
- test cable identity parsing and trust assumptions
- reproduce boot-time or pre-OS Type-C edge cases consistently
- discover hidden service behaviors triggered by unusual accessory states
- compare firmware revisions for safe fallback quality

### Penetration Testing

- assess kiosks, thin clients, and embedded maintenance ports with authorization
- measure how endpoints react to bounded power starvation and recovery
- verify whether device hardening covers policy-layer attach manipulation

### Defensive Engineering

- create regression scenarios for BIOS / EC / PD controller updates
- verify that identity changes trigger revalidation or safe reset
- confirm that current limiting and sideband faults do not produce unsafe behavior
- produce evidence-backed lab test cases for product security teams

## Example Scenario Packs

### `dock_identity_flip`

The device passively observes a normal laptop-to-dock connection, then enables mutation and swaps to a forged dock identity while preserving otherwise plausible electrical behavior. Optional USB2 isolation briefly perturbs the path to check revalidation and driver reload behavior.

### `late_vconn_claim`

After a target stabilizes, the platform changes sideband assumptions and profile identity timing associated with VCONN-related expectations. Researchers watch for instability, mode confusion, or fallback regressions.

### `role_swap_race`

The device requests role swaps near natural transition windows, then injects a reset and retries to identify race conditions and poor rollback handling.

### `debug_accessory_probe`

A short debug-accessory-like assertion is introduced during attach timing to identify systems that expose hidden recovery or service states.

### `power_starve_then_recover`

Available current is intentionally reduced inside a strict, logged safety envelope, then restored to test maintenance behavior, reboot logic, or fail-open communication paths.

## Evidence and Telemetry Model

A professional security tool should preserve evidence, not just trigger effects. Type-C Specter is designed to capture:

- scenario name and parameters
- attach timeline with millisecond timestamps
- profile changes and mutation windows
- current-limit changes
- PD packet summaries
- thermal and current safety events
- operator acknowledgement state
- exportable machine-readable session records

The secure element concept allows future versions to sign session summaries so product teams and clients can trust that a specific scenario was used.

## KiCad Design Notes

The KiCad files in `kicad/` provide a realistic project skeleton with actual components, net names, and board footprints representing the design’s core architecture:

- STM32H743 MCU
- iCE40UP5K FPGA
- dual USB-C receptacles
- radio module
- power monitor and battery management support blocks
- QSPI flash and secure element

Revision A intentionally focuses on policy-layer and USB2/SBU experimentation rather than full USB4 packet termination. That keeps routing feasible while still providing substantial research value.

## Manufacturing and Prototype Strategy

A sensible prototype path is:

1. validate power path and dual Type-C attach behavior
2. validate PD interception and logging
3. add identity mutation and scenario logic
4. confirm safe fallback behavior under forced trips
5. test the companion console and export path
6. iterate on enclosure and shielding only after electrical behavior is stable

This helps avoid the common mistake of overbuilding high-speed features before the policy-layer research value is proven.

## Limitations

- reference firmware is a host simulation, not final silicon-bound production firmware
- no full USB4 or Thunderbolt packet termination in revision A
- real lab validation would be required before connecting expensive target hardware
- any field use demands authorization and a written safety process

## Conclusion

Type-C Specter is an original, practically useful device concept for security researchers working at the increasingly important USB Type-C trust boundary. By combining inline observation, policy-layer deception, bounded power manipulation, scenario automation, evidence capture, and strong safety defaults, it gives red teams and defenders a repeatable way to study how real systems behave before normal USB enumeration even begins. That makes it valuable for enterprise hardware assurance, embedded service-port assessment, mobile accessory testing, and product security regression work.

All files in this design set credit **jayis1** as the author and creator.
