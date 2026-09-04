# MCTP Wraith

**Author:** jayis1  
**Copyright:** Copyright (c) 2026 jayis1  
**Category:** Server sideband, platform management, MCTP/PLDM/SPDM research instrument

## Legal and Ethical Disclaimer

MCTP Wraith is designed strictly for **authorized** security research, defensive validation, red-team simulation, hardware assurance testing, lab use, and product security engineering. It must only be used on systems you own or are explicitly authorized to assess in writing. The platform can intercept, delay, mutate, replay, and impersonate management-plane traffic between platform components such as a host CPU, BMC, NIC, NVMe enclosure, retimer, hot-swap backplane, or security controller. Misuse against third-party systems, production systems, critical infrastructure, cloud hardware, medical devices, vehicles, or shared enterprise environments without explicit authorization may cause outages, failed firmware updates, integrity alerts, platform isolation events, or legal consequences. Use only in controlled environments with rollback procedures, maintenance windows, signed approvals, capture retention controls, and a hardware bypass plan.

## Device Purpose and Overview

MCTP Wraith is a portable inline sideband-management instrument built for one of the most under-observed attack surfaces in modern enterprise hardware: the low-bandwidth management traffic that flows between the BMC, host, retimers, add-in cards, storage backplanes, and service processors. Large parts of server trust are negotiated over channels that are rarely visible to defenders in real time. Even when the high-speed dataplane is encrypted or segmented, the management plane often remains reachable through SMBus, I3C, MCTP over PCIe vendor-defined messages, NC-SI-adjacent sidebands, or board-level links that carry PLDM, SPDM, firmware update commands, inventory telemetry, and event notifications.

Those interfaces matter because they mediate identity, firmware lifecycle, health reporting, routing, attestation, and orchestration. A BMC may ask a retimer for health data. A NIC may surface device inventory through MCTP endpoints. An NVMe enclosure may expose update staging or telemetry over PLDM. A security controller may negotiate SPDM capability or report measurements. When researchers assess a server, appliance, storage array, or telecom baseboard, they frequently lack a compact device that can sit inline on those sideband paths and answer basic but important questions: Which endpoints are visible? Who assigns endpoint IDs? Which components trust unsigned sensor data? Can firmware staging traffic be fuzzed in a narrow and controlled way? Does the platform fall back when attestation is delayed? Can a malicious replacement module clone endpoint identity or poison route discovery without touching the main host network?

MCTP Wraith exists to answer those questions. It is an inline bridge and mutation platform for **MCTP-centric sideband traffic**, designed to observe and selectively influence endpoint discovery, routing control, PLDM commands, SPDM negotiation, event propagation, and management-plane timing assumptions. In transparent mode, it captures and decodes traffic while preserving the original electrical path. In active mode, it can inject bounded delays, rewrite selected fields, mirror evidence streams, stage endpoint-clone personas, and test route-table trust assumptions while keeping hard safety rails around packet rate, mutation scope, and bypass behavior.

The novelty is not just “sniffing SMBus.” Existing lab workflows often use a logic analyzer for physical observation, a generic microcontroller bridge for rough replay, or a software proxy farther up the stack. Those methods miss the precise layer where platform management trust is born: low-level endpoint addressing, binding-specific transport behavior, and early service discovery across mixed media. MCTP Wraith treats that layer as a first-class security surface. It allows a researcher to compare passive discovery, active perturbation, and evidence export in a repeatable workflow, with scenario controls built around how real server components behave.

A red team can use it to validate whether a target platform will accept a cloned endpoint advertisement from an inserted management module. A product security engineer can verify whether SPDM capability negotiation resists version-floor manipulation or timeout shaping. A defender can map which PLDM sensor values are trusted enough to trigger maintenance workflows. A hardware assurance team can test whether route-control messages are authenticated or merely assumed to be honest because they arrived on the “inside” of the board. In all of those cases, the value comes from controlled visibility and precise intervention rather than broad disruption.

## Why the Device Is Original

MCTP Wraith is not a rebrand of a general protocol analyzer, and it is not aimed at the main data path. Its focus is the **internal sideband management fabric** of modern platforms. That makes it distinct from devices that target USB, PCIe DMA, storage buses, or general wireless interception.

What makes it original is the combination of:

1. **Binding-aware inline interception** across MCTP over SMBus/I3C and staging hooks for PCIe VDM bridging.
2. **Endpoint-identity manipulation**, including endpoint clone testing, route poisoning simulation, and bus-owner trust validation.
3. **Protocol-semantic mutation** for PLDM and SPDM fields rather than blind byte corruption.
4. **Safety-first sideband research controls** such as default passive mode, hardware bypass, mutation quotas, route TTL limits, and explicit operator arming.
5. **Evidence signing workflow** so route captures, policy hits, and scenario traces can be exported as auditable artifacts during an engagement or internal validation cycle.

Many security teams know the words MCTP, PLDM, and SPDM from standards discussions, but few have a practical field tool that can be clipped into a chassis interposer harness and used like a sideband MITM microscope. That gap is the reason for this design.

## Threat Model and Attack Surface

MCTP Wraith is designed to model an adversary with temporary or persistent physical access to server internals, backplanes, mezzanine modules, or maintenance interfaces. The tool is not intended to break cryptography directly. It is intended to test whether the platform’s management-plane trust assumptions are sound when the transport path itself becomes untrusted.

### Threats Simulated

- A malicious replacement FRU or service module inserted during depot repair.
- A red-team operator with temporary maintenance-window access inside a server chassis.
- A supply-chain implant that can proxy management-plane traffic but wants to avoid noisy dataplane tampering.
- An insider with access to a backplane harness between a BMC and managed endpoints.
- A component-level adversary attempting to exploit route discovery, endpoint identity, firmware staging, or unauthenticated telemetry.

### Security Questions Under Test

1. Does the BMC trust endpoint-discovery and route advertisements without strong authentication?
2. Can a cloned endpoint ID or timing-shaped hot-join alter route ownership or message delivery?
3. Will SPDM negotiation fall back to weaker assumptions if responses are delayed or fields are minimally rewritten?
4. Are PLDM firmware update stages validated for sequence, size, and state machine integrity?
5. Do sensor readings or health events trigger operational workflows without provenance checks?
6. Is mixed-medium routing between I3C, SMBus, and PCIe VDM bridged safely?
7. Do retimers, NICs, or NVMe sideband agents expose undocumented MCTP commands or unsafe debug states?

### Attack Surface Areas

- **Endpoint discovery:** enumeration, hot-join, dynamic addressing, endpoint ID claims.
- **Routing control:** next-hop selection, route table TTLs, bridge announcements, bus-owner arbitration.
- **PLDM exchanges:** sensor data, inventory, platform monitoring, firmware update state machines.
- **SPDM negotiation:** version offers, capability reporting, transcript timing, measurement retrieval assumptions.
- **Alerting and telemetry:** whether operational software treats sideband assertions as inherently trusted.
- **Cross-domain management bridges:** paths where internal management traffic crosses between physical media.

## Practical Research Goals

MCTP Wraith is built to help answer practical assessment questions, including:

- Which sideband components can talk to each other before the host fully boots?
- Which EIDs appear static versus dynamically assigned?
- Can the platform be coerced into preferring a poisoned route or clone persona?
- Are device firmware update workflows robust against reordered or malformed stage descriptors?
- Does attestation logic depend on timing assumptions that are fragile under path delay?
- Which management values become actionable in orchestration systems?
- Can an inserted module harvest meaningful inventory and topology information without ever touching the main network?

## Hardware Specifications

### Core Processing

- **Primary MCU:** STM32H753ZI, Cortex-M7 up to 480 MHz, selected for strong peripheral support, DMA, USB, Ethernet-class management, and enough headroom for inline decode and policy execution.
- **Timing and bridge FPGA:** Lattice ECP5-25F, used for deterministic bus turnaround, cross-media buffering, route-slot assist logic, programmable delay insertion, alert GPIO handling, and safe bypass sequencing.
- **Secure evidence element:** Microchip ATECC608B for signing capture manifests, scenario metadata, and exported evidence bundles.
- **Local storage:** 128 MB QSPI NOR for policy packs, firmware images, signed capture bundles, and endpoint profile caches.
- **External RAM:** 64 MB HyperRAM for trace buffering and route-table snapshots during bursty events.

### Target-Facing Interfaces

- Dual sideband mezzanine connectors for upstream and downstream harnesses.
- 1.2 V / 1.8 V / 3.3 V tolerant SMBus and I3C interfaces with hot-swap buffers.
- Optional PCIe sideband header for MCTP-over-VDM staging research through external retimer or service-card adapters.
- Isolated alert, reset, presence-detect, and service-button sense lines.
- Digitally switched pull networks to emulate bus-owner, alert, or hot-join conditions under software control.

### Connectivity and Operator I/O

- USB-C for management, trace export, firmware update, and console.
- 10/100 Ethernet for lab automation and headless orchestration.
- Wi-Fi 6 / BLE 5.3 management module for field operation where cabling is limited.
- MicroSD slot for large evidence exports and rule-pack distribution.
- 1.69-inch display for mode, active scenario, alert count, and endpoint map summary.
- Safe / Arm / Trigger hardware buttons.
- Tri-color LED bar and buzzer so active mutation is physically obvious to the operator.

### Sensing and Safety

- Rail-voltage monitors on management-side and target-side I/O domains.
- Current-sense amplifiers on bridge and target rails.
- Thermal sensor near FPGA regulator and level-shifter bank.
- Hardware watchdog.
- Analog bypass path that defaults to transparent operation on fault or power loss.
- Power-up in passive capture mode with mutation disabled until the operator arms a scenario.

### Power

- USB-C PD sink for bench use.
- Single-cell Li-ion or 2-cell pack variant depending enclosure depth.
- Fuel gauge and charger on board.
- Split low-noise rails for MCU, FPGA core, and target I/O domains.

### Form Factor

- Board size target: 92 mm × 58 mm four-layer PCB.
- Low-profile aluminum enclosure with vented sidewalls.
- Mount points aligned for clip-in sled or bench stand use.
- Harness-first design so the board can sit between a BMC sideband header and the managed endpoint path.

## Architecture

### Functional Blocks

1. **Target-facing sideband transceivers** terminate or pass through SMBus/I3C signaling under FPGA supervision.
2. **FPGA timing fabric** enforces deterministic turnaround, mirrors traffic into capture buffers, and exposes safe mutation controls.
3. **MCU control plane** runs discovery logic, endpoint models, PLDM/SPDM-aware policy rules, and evidence packaging.
4. **Secure evidence element** signs exported captures and scenario manifests.
5. **Companion app / web UI** arms scenarios, visualizes endpoint maps, and reviews route changes or alert hits.
6. **Bypass and safety controller** guarantees passive continuity when the board faults or loses power.

### Text Block Diagram

```text
[BMC / Host Root Complex]
          |
   [Hot-swap sideband buffer]
          |
   [FPGA timing + bridge fabric]----[HyperRAM trace buffers]
          |
     [STM32H753 control plane]----[ATECC608B evidence signer]
          |            |            |
          |            |         [QSPI NOR / microSD]
          |            |
          |       [Wi-Fi / BLE module]
          |
   [Downstream sideband buffer]
          |
[Retimer / NIC / NVMe / Backplane Endpoint]
```

### Bus Flow

- Transparent mode: endpoint traffic is mirrored only.
- Policy mode: selected commands are delayed, rewritten, or alerted on.
- Clone mode: the device can advertise a controlled alternate persona while preserving original captures for comparison.
- Evidence mode: all policy hits are timestamped, measured, and signed before export.

## Firmware Design and Decisions

The reference firmware included in `firmware/` is a compile-ready simulation of the control plane. It is not a stub. It models endpoints, routes, policy rules, capture behavior, telemetry evolution, radio status frames, and scenario-driven mutation. The host build path uses `gcc` so the logic can be exercised quickly in CI or on a developer workstation before being ported to the target MCU.

### Design Choices

- **Message-semantic mutation instead of random corruption.** The policy engine targets specific command classes such as SPDM version requests or PLDM sensor payloads.
- **Scenario model rather than ad-hoc toggles.** This makes engagements repeatable and easier to document.
- **Passive-by-default flags and explicit mutation arming.** Safety and evidence integrity are more important than stealth in authorized work.
- **Telemetry as a first-class output.** Current, temperature, captures, mutations, drops, and alerts are tracked to support repeatability.
- **Signed evidence concept.** Even in the simulation, the firmware models evidence-bundle creation because the reportability of a finding matters.

### Included Firmware Components

- `main.c` — runtime model, endpoint topology, route table, event logging, scenario selection, and execution loop.
- `drivers/sideband_bus.c` — synthetic message source, capture ring, delay budget handling, and injection helper.
- `drivers/policy_engine.c` — rule matching, scenario arming, route poisoning, delay insertion, rewrite actions, and alert logic.
- `drivers/pldm_codec.c` — lightweight semantic helpers for command naming, measurement, and controlled payload rewriting.
- `drivers/telemetry.c` — rail, current, temperature, capture, mutation, and evidence counters.
- `drivers/radio_link.c` — status frame queue for a field operator or remote orchestrator.
- `board.h` and `registers.h` — platform constants, runtime structs, flags, and representative register maps.

### Built-In Scenarios

- **route-poison** — tests route-table trust and bridge announcements.
- **spdm-downgrade-probe** — applies narrow rewrites and timing pressure to version negotiation.
- **pldm-stage-fuzz** — mutates firmware stage descriptors while preserving deterministic replay.
- **sensor-ghost** — falsifies selected sensor responses to test operational trust.
- **endpoint-clone** — evaluates whether the platform handles a controlled identity collision safely.

## Companion Application / Software Interface

The `app/` directory contains an offline single-page web companion intended for rapid field use without a package install step. It provides real UI logic for:

- selecting scenarios
- arming / disarming policy state
- editing route entries
- viewing endpoint inventory
- reviewing telemetry counters
- generating a JSON evidence bundle preview
- simulating operator actions locally before a live deployment

The app is deliberately browser-native so it can run from removable media, a jump box, or a lab workstation without a heavier mobile build chain. For a future production build, the same state model can be migrated into React Native if mobile BLE-first deployment becomes a priority.

## Red-Team, Research, and Defensive Use Cases

### Red Teams

- Validate whether a server platform trusts internal management-plane identities too easily.
- Explore low-noise persistence paths that do not touch the primary network stack.
- Demonstrate route poisoning, sensor falsification, or endpoint clone risk to a client using signed evidence.
- Assess whether maintenance workflows can be steered by falsified PLDM events or telemetry.

### Security Researchers

- Map MCTP endpoint topologies on unfamiliar hardware.
- Compare SMBus and I3C binding behaviors under identical scenario loads.
- Study how PLDM and SPDM interact with mixed-medium route bridges.
- Reproduce timing-sensitive management bugs with deterministic delay budgets.

### Penetration Testers

- Turn an opaque internal sideband into an observable assessment surface.
- Capture enough evidence to support a finding even when the bug itself is subtle.
- Test firmware update paths without broad device corruption.
- Identify undocumented management endpoints exposed by NICs, retimers, or storage modules.

### Defensive Engineering Teams

- Verify that route discovery and endpoint ownership are authenticated or otherwise bounded.
- Test operational software against falsified but protocol-valid health data.
- Confirm that fallback, timeout, and retry behavior do not weaken trust decisions.
- Build repeatable regression scenarios after fixing a management-plane bug.

## Example Assessment Workflow

1. Install the harness inline between the BMC sideband header and a managed endpoint chain.
2. Boot in passive mode and record baseline discovery and routing.
3. Export an endpoint map and identify high-value EIDs.
4. Arm `spdm-downgrade-probe` or `route-poison` with mutation quotas.
5. Reboot or retrigger discovery.
6. Compare route, timing, and policy-hit evidence against the passive baseline.
7. Export the signed evidence bundle and attach it to the engagement report.

## KiCad Design Notes

The KiCad project models a real hardware architecture with representative components and named nets for:

- STM32H753 MCU
- ECP5 FPGA
- ATECC608B secure element
- TXS/TCA-class sideband level shifting and hot-swap buffering
- USB-C management port
- QSPI NOR storage
- HyperRAM buffer interface
- upstream and downstream mezzanine headers
- power rails and alert lines

The schematic and PCB files are intentionally lightweight enough to live in this repository while still showing concrete symbols, footprints, and interconnect intent.

## Bill of Materials Summary

- STM32H753ZI MCU
- Lattice ECP5-25 FPGA
- ATECC608B secure element
- Winbond 128 Mbit QSPI NOR
- 64 Mbit HyperRAM
- I3C / SMBus hot-swap buffer devices
- USB-C connector and PD sink controller
- buck regulators and LDO rails
- mezzanine connectors for sideband harnesses
- microSD socket
- small IPS display

## Limitations

- The reference firmware is a simulation layer, not production microcontroller firmware.
- PCIe VDM transport is represented architecturally and in route models but not implemented as a live host stack here.
- Full standard conformance for every PLDM and SPDM subtype is beyond the scope of this first design drop.
- High-assurance deployments would require per-target harness validation and signal-integrity tuning.

## Future Extensions

- live SPDM transcript parser and signed measurement bundle export
- route-learning visualizer with topology diff mode
- hot-join fuzz harness for I3C-heavy platforms
- automated safe rollback of scenario packs after a watchdog event
- optional Rust host tool for bulk evidence ingestion

## Repository Contents

```text
mctp-wraith/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── sideband_bus.c
│       ├── sideband_bus.h
│       ├── policy_engine.c
│       ├── policy_engine.h
│       ├── pldm_codec.c
│       ├── pldm_codec.h
│       ├── telemetry.c
│       ├── telemetry.h
│       ├── radio_link.c
│       └── radio_link.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── index.html
    ├── styles.css
    └── app.js
```

## Closing Summary

MCTP Wraith is a sideband-management security research platform for environments where the most important trust decisions happen off the main network and below ordinary monitoring. Its value is in making that layer visible, controllable, and testable without collapsing into a generic bus sniffer or an unsafe fuzz appliance. It gives authorized operators a practical way to study endpoint identity, route ownership, attestation timing, firmware staging, and operational trust in the MCTP ecosystem, while keeping evidence quality and safety at the center of the design.
