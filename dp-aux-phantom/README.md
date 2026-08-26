# DP AUX Phantom

**Author:** jayis1  
**Copyright:** jayis1  
**License:** GPL-2.0-only  
**Status:** Conceptual hardware design with compile-ready firmware simulation, companion application, and KiCad project skeleton  
**Legal / Ethical Notice:** DP AUX Phantom is intended **only** for authorized security research, interoperability validation, red-team lab simulation, and hardware assessment on systems, docks, displays, and environments you own or are explicitly permitted to test.

## Overview

DP AUX Phantom is a novel inline **USB-C / DisplayPort Alt Mode instrumentation appliance** for display security research. The device sits between a host and a monitor, projector, dock, KVM, or conference-room display chain and gives the operator controlled visibility into the **DisplayPort AUX channel**, **HPD behavior**, **EDID identity**, and selected **USB-C CC / Power Delivery state** relevant to display negotiation. Rather than acting as a generic video sniffer or a pure cable tester, DP AUX Phantom focuses on the underexplored trust boundary between a source GPU/OS stack and a sink-side device that advertises capabilities through AUX and EDID.

Modern enterprise environments treat displays and docks as low-risk peripherals, but a large amount of policy, driver logic, and trusted identity still lives in the discovery and negotiation path. Hosts read EDID strings, parse detailed timing descriptors, inspect DPCD capability bytes, decide whether MST is available, and react to HPD transitions that can force re-enumeration. Conference-room systems, SOC docks, digital signage endpoints, kiosk controllers, and secure workstation setups all rely on this interaction. That makes the display path a compelling area for security research: not because it should be used recklessly, but because display enumeration is widely trusted, rarely monitored, and often implemented deep inside firmware and kernel graphics stacks.

DP AUX Phantom is designed to let a security researcher answer questions such as:

- How does a given OS or GPU driver react if a sink claims different link rates or lane counts than expected?
- Can a dock or monitor identity be masked, normalized, or emulated without breaking functionality?
- Do HPD pulses trigger sensitive application behavior, credential prompts, desktop rearrangement, USB retimers, or conference-room control sequences?
- How do secure environments handle MST topology discovery, dock swaps, projector hotplug, and repeated EDID reads?
- Are there brittle parser assumptions in display enumeration paths that appear only when real timing and cable conditions are involved?

The device concept is intentionally practical. It is shaped like a compact inline pod with **USB-C upstream and downstream ports**, a small internal **FPGA** for time-sensitive AUX path handling, a **microcontroller** for orchestration and policy, local **EDID storage**, and a **wireless control plane** for field operation. Instead of trying to ingest full high-speed DisplayPort lanes, the design leaves the high-bandwidth video path electrically transparent through retimers/redrivers and concentrates its intelligence on the control plane that makes the source trust the sink.

## Purpose and Research Value

The central idea behind DP AUX Phantom is simple: a host often trusts what the display path says about itself. If the sink advertises different capabilities, exposes a different serial number, or drops/raises HPD at a strategic moment, the host software stack may take actions that are meaningful from a security, reliability, or operational standpoint. Most red-team toolkits heavily cover Ethernet, Wi-Fi, RF, USB HID, NFC, BLE, serial, CAN, and storage buses. Far fewer tools address **display trust surfaces** with enough fidelity to be useful during real hardware assessment.

This makes DP AUX Phantom valuable for:

1. **Conference-room and meeting-space assessments** where laptops connect to shared displays, USB-C docks, or integrated room compute systems.
2. **Secure workstation testing** where docking paths, KVMs, and monitor trust assumptions matter.
3. **Embedded display stack research** on kiosks, industrial HMIs, signage players, and field laptops.
4. **Driver robustness testing** by exploring DPCD, EDID, and HPD edge cases without rewriting the target operating system.
5. **Purple-team training** by demonstrating that “just a display cable” can still represent a meaningful logic and trust surface.

## Attack Surface and Threat Model

DP AUX Phantom models the display chain as a trust boundary with several distinct surfaces.

### 1. EDID Trust Surface
The host consumes EDID data to identify the monitor, supported modes, vendor, serial number, extension blocks, HDR capabilities, audio descriptors, and more. Enterprise fleet logic, docking software, room systems, or compliance tooling may log or react to that identity. A display identity that changes unexpectedly can expose assumptions in software inventory, hot-desk logic, or secure-presentation workflows.

### 2. DPCD / AUX Capability Negotiation
DisplayPort sources read DPCD registers to determine sink capabilities such as revision, maximum link rate, lane count, and optional features. If those values are inconsistent, downgraded, or mutated, the source stack may retry, change link training behavior, or select alternate display policies. This can reveal resilience weaknesses or trigger operational side effects useful in authorized testing.

### 3. HPD Timing and State Changes
Hot Plug Detect is not just a physical convenience signal. Repeated HPD transitions can trigger device re-enumeration, display pipeline resets, desktop mode changes, application notifications, and in some dock stacks, auxiliary peripheral resets. Timing-sensitive HPD experiments are useful for understanding what the host does under unstable or manipulated display conditions.

### 4. Dock / MST Topology Emulation
Shared workspace environments often use docks or MST hubs. If a sink path suddenly advertises MST-related behavior, the host may enumerate additional logical display surfaces or execute topology discovery routines that are rarely tested. This is relevant to interoperability research and to red-team emulation of malicious or deceptive peripheral chains.

### 5. USB-C Configuration Channel Context
While DP AUX Phantom is not a full malicious PD injector, it observes and policy-binds enough CC / Alt Mode context to keep experiments grounded in realistic cable orientation, attach state, and mode entry sequencing. That improves the realism of tests involving USB-C displays, travel docks, and monitor hubs.

### Threat Model Scope
DP AUX Phantom is designed for **authorized inline research** against the logic layer of display negotiation. It is **not** a covert content-exfiltration implant for encrypted video payloads and it is **not** intended to defeat HDCP or intercept user content. The research target is the control plane: capability advertisement, identity presentation, status signaling, and host reaction.

## Hardware Specifications

### Core Processing
- **Primary MCU:** RP2350-class microcontroller for policy control, logging, BLE/Wi-Fi coordination, and operator commands.
- **Timing / Logic Fabric:** Lattice iCE40UP5K FPGA for deterministic AUX-path interception, HPD pulse timing, and low-latency control-state transitions.

### Display / USB-C Path
- **Upstream Port:** USB-C receptacle toward host.
- **Downstream Port:** USB-C receptacle toward display / dock / projector.
- **AUX Path Handling:** Differential AUX path observed and policy-routed through analog switching plus FPGA-assisted buffering.
- **HPD Control:** Sink HPD sense plus FPGA-generated pulse/hold capability under firmware policy.
- **High-Speed Video Path:** Intended to be routed through retimer/redriver stages with minimal disturbance; design emphasis is on the control plane rather than content capture.

### Control and Sideband
- **CC / PD Controllers:** Dual FUSB302B-class controllers to track attachment state and DisplayPort Alt Mode orientation metadata.
- **EDID / Identity Storage:** 24LC64 or similar EEPROM for baseline EDID cache, synthetic identities, and replay profiles.
- **Wireless Backhaul:** ESP32-C3-class companion radio for BLE setup and Wi-Fi telemetry relay.
- **Local Storage:** QSPI flash or microSD footprint for trace capture, profile packs, and offline session export.

### Power
- **Input:** Bus-powered from USB-C VBUS with ideal-diode / protection stage.
- **Regulation:** Buck-boost stage generating 3.3 V logic rail and optional 1.2 V FPGA core rail.
- **Field Mode:** Optional small LiPo backup or supercap for graceful logging during fast unplug events.

### Sensors / Instrumentation
- HPD state monitor
- VBUS presence monitor
- Cable orientation / CC state capture
- Optional temperature sensor for enclosure thermal validation

### Form Factor
- Pocket inline pod sized approximately 85 mm × 42 mm × 14 mm
- Two short captive USB-C pigtails or two panel-mount receptacles
- Status LED strip hidden behind smoked lens
- Side button for safe-bypass / transparent mode

## Architecture

At a block level, DP AUX Phantom is built around a split-control architecture.

1. **USB-C host connector** enters the device.
2. **USB-C sink connector** exits the device toward the target monitor or dock.
3. High-speed main-link lanes are routed through a transparent retimer / redriver path.
4. The **AUX differential pair** is branched into an analog front end and presented to the FPGA.
5. The **FPGA** handles deterministic observation, mutation windows, and HPD pulse sequencing.
6. The **MCU** loads policies, interprets traces, selects profiles, and communicates with the operator app.
7. **PD / CC controllers** inform the MCU about attach state and Alt Mode context.
8. **EEPROM / flash** stores baseline EDIDs, replay variants, and captured transaction sets.
9. **BLE/Wi-Fi module** exposes a field interface for the app.

### Text Block Diagram

```text
Host USB-C
   │
   ├── High-speed DP lanes ── Retimer/Redriver ──> Sink USB-C
   │
   ├── AUX+/AUX- ── Analog front end ── FPGA ── Policy gate ──> Sink AUX
   │
   ├── HPD sense <──────────── FPGA pulse / hold control ──────┘
   │
   └── CC/PD controllers ──> MCU ──> BLE/Wi-Fi companion link
                              │
                              ├── EEPROM / QSPI flash / trace store
                              └── Operator policy engine
```

The key design decision is that the FPGA, not the MCU, owns the critical timing edge around AUX and HPD. That makes the platform useful for realistic negotiation experiments instead of coarse post-facto logging.

## Firmware Details and Design Decisions

The provided firmware in `firmware/` is a compile-ready host simulation that models how the embedded code would behave on the target hardware.

### Firmware Modules
- `main.c` initializes the system, loads profiles, attaches a simulated sink, runs policy ticks, and prints a trace summary.
- `drivers/auxbus.c` models a series of AUX and I2C-over-AUX transactions including DPCD reads, link training writes, and EDID fetches.
- `drivers/policy.c` implements profile-specific mutations such as EDID serial masking, dock emulation, forced link-rate changes, HPD-associated rule hits, and deterministic fuzzing.
- `drivers/pd.c` models USB-C / PD attach context and HPD policy behavior.
- `drivers/radio.c` simulates a BLE/Wi-Fi telemetry plane.
- `drivers/capture.c` logs structured events for export and replay.

### Design Philosophy
The firmware is intentionally policy-driven. Researchers should not need to rewrite low-level code for every experiment. A profile such as `dock-emulator`, `edid-mask`, `lt-slowroll`, or `hpd-bounce` changes behavior across multiple subsystems in a coordinated way.

### Safety-Oriented Constraints
The design avoids features that would encourage unsafe or unauthorized use. For example:
- It focuses on metadata/control-path manipulation rather than video payload interception.
- It explicitly documents authorized-use requirements.
- It supports a transparent baseline mode so researchers can compare mutated behavior against a no-touch path.
- It assumes the operator is studying robustness and trust decisions, not bypassing media protections.

### Why RP2350 + iCE40UP5K?
The MCU is responsible for flexibility, storage, OTA profile updates, and app communication. The FPGA provides deterministic state transitions and buffering where software interrupt jitter would be too sloppy. This hybrid architecture keeps the platform affordable and hackable while still being realistic enough for timing-sensitive display research.

## Companion Application / Software Interface

The `app/` directory contains a React Native companion app by jayis1. The app is structured around four practical screens:

1. **Overview** – high-level telemetry, active profile, sink identity, negotiated link parameters, and alert/rule counters.
2. **Profiles** – quick selection among transparent, EDID masking, dock emulation, AUX fuzzing, and HPD timing modes.
3. **Capture** – human-readable event feed showing intercepted requests and policy effects.
4. **Safety** – operator checklist emphasizing authorized use and reproducibility.

In a full hardware implementation, the app would talk to the device over BLE for setup and local Wi-Fi for richer streaming sessions. A minimal JSON protocol would include:
- `status_snapshot`
- `select_profile`
- `arm_capture`
- `request_trace_page`
- `export_session`
- `enter_transparent_bypass`

## Red-Team, Research, and Penetration-Testing Use Cases

### Conference-Room Security Assessment
Insert DP AUX Phantom between a corporate laptop and a shared conference-room display chain to study how the room stack reacts to dock-like identity changes, HPD pulses, or MST discovery in a tightly controlled maintenance window.

### Dock Trust Validation
Evaluate whether endpoint software or local workflows implicitly trust a dock/monitor identity for asset recognition, workspace setup, or policy binding.

### GPU Driver Robustness Research
Use transparent mode first, then selectively force lower link rates, altered lane counts, or unusual EDID strings to uncover driver instability, retry loops, or parser assumptions.

### Embedded Signage and Kiosk Testing
Many kiosks and signage players boot directly into graphics pipelines with little operational visibility. DP AUX Phantom lets a tester inspect how these devices behave when a sink identity changes without needing firmware access to the kiosk itself.

### MST / Topology Logic Exploration
Where legal and appropriate, researchers can explore how MST-capable hosts and room systems enumerate shared display chains, especially in hot-desk and docking deployments.

### Incident Reproduction Tool
When a display interoperability bug appears only in the field, DP AUX Phantom can capture a baseline interaction and replay or approximate the edge case during lab analysis.

## Practical Operator Workflow

1. Insert device inline in transparent mode.
2. Capture baseline AUX / EDID / HPD sequence for the target system.
3. Save trace with target host, OS, cable, and sink metadata.
4. Select one constrained mutation profile, such as EDID serial masking.
5. Repeat connection and compare host behavior.
6. Escalate to HPD pulse or dock emulation only if the test plan authorizes it.
7. Export findings, restore transparent mode, and remove the device.

This workflow makes the tool useful in real engagements because it promotes controlled deltas rather than chaotic experimentation.

## KiCad Design Notes

The `kicad/` directory includes a real-text KiCad project set with:
- `device.kicad_pro`
- `device.kicad_sch`
- `device.kicad_pcb`

The project models major design elements including USB-C connectors, MCU, FPGA, dual CC controllers, EEPROM, radio module, power regulation, named nets, and board outline. A production-ready spin would still require signal-integrity review, retimer selection, impedance-controlled routing, ESD protection detail, footprint validation, and full manufacturing rules. The included files are meant to establish concrete architecture and connectivity rather than claim finished SI closure.

## Limitations and Future Work

A future hardware revision could add:
- swappable retimer mezzanines for DP 1.4 vs DP 2.x research,
- explicit microSD logging,
- isolated debug UART,
- controlled sink-side EEPROM hot-swap,
- PoE or battery sled for long kiosk sessions,
- richer per-transaction scripting.

Another promising extension would be a profile language that describes transaction-level conditions and bounded actions, allowing repeatable fuzz cases and safer research campaigns.

## Conclusion

DP AUX Phantom fills a gap in security research tooling by treating the display negotiation path as a first-class hardware trust surface. It is novel because it does not merely sniff cables, emulate a USB gadget, or inject keystrokes. Instead, it targets the subtle but important logic between host and sink: EDID identity, DPCD capability exchange, AUX transaction flow, and HPD timing. For authorized red-team operators, hardware researchers, and interoperability engineers, that combination makes it a practical instrument for assessing conference rooms, docks, signage systems, secure workstations, and display stacks that are usually assumed to be boring peripheral plumbing.

Every design file, firmware component, and app artifact in this folder credits **jayis1** as the author. Use responsibly, document every test, and operate only with explicit authorization.
