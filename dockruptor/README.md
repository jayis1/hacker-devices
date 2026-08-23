# Dockruptor — Inline USB-C Power Delivery and Alt-Mode Adversary Platform

**Author:** jayis1  
**License:** Hardware — CERN-OHL-S v2 · Firmware — GPL-2.0-only · App — MIT  
**Version:** 1.0.0

> **Legal / Ethical Notice:** Dockruptor is designed strictly for authorized security research, defensive validation, red-team exercises, and product security testing performed with explicit written permission from the system owner. Do not connect, operate, or evaluate this platform against production assets, third-party equipment, public charging infrastructure, or end-user devices without authorization.

---

## 1. Overview

Dockruptor is a **portable inline USB-C and USB Power Delivery adversary platform** built for security researchers who need to test the trust assumptions surrounding modern docking stations, kiosks, rugged tablets, secure laptops, industrial handhelds, conference room controllers, field service terminals, and embedded appliances that rely on **USB-C power negotiation and Alternate Mode state changes** as if they were purely electrical events rather than attack surfaces.

That assumption is no longer safe. USB-C is not just a connector; it is a policy bus. The moment a target is attached to a dock, power brick, display, car charger, field console, or portable battery, it begins making decisions based on messages exchanged over the **CC1/CC2 configuration channels**. Those messages determine source and sink roles, advertised power contracts, cable identity, mode capability, data-role swaps, VCONN sourcing, vendor defined messages, and whether the system should enter DisplayPort, Thunderbolt-compatible, USB4, debug accessory, or vendor-specific operating modes. In many environments, that negotiation happens before an operator thinks of the device as "online."

Dockruptor is meant to exploit that blind spot for defenders and authorized red teams. It physically sits **inline between a host and a dock or power source** and presents itself as a deterministic policy manipulator, recorder, and fault injector for USB-C control traffic. Instead of only sniffing packets after the OS enumerates a device, Dockruptor lets an operator:

- Record Power Delivery capability advertisements and accepted contracts.
- Clone, suppress, reorder, or mutate source and sink PDO advertisements.
- Force role swaps, soft resets, and hard reset timing tests.
- Delay or deny entry into DisplayPort Alt Mode or USB4 tunneling.
- Rewrite cable identity and eMarker metadata to emulate unsafe or downgraded accessories.
- Create repeatable pre-boot conditions against kiosk, BIOS, recovery, and lock-screen workflows.
- Log every negotiation transition with timestamped safety annotations.

The practical value is substantial. Organizations increasingly deploy USB-C docks as trusted extensions of secure endpoints. A field laptop may accept firmware updates through a dock NIC, map displays through an alt-mode bridge, expose PCIe paths through Thunderbolt compatibility, trust its charger for safe power, and unlock administrative workflows whenever an approved docking profile is detected. Defensive teams often validate the software path while overlooking the **link policy path** that exists before USB enumeration, before user-space policy enforcement, and sometimes before secure boot handoff is complete.

Dockruptor gives researchers a way to validate whether those decisions can be manipulated by a malicious charger, weaponized conference room hub, swapped kiosk cable, or compromised service dock.

---

## 2. Why This Device Is Novel

Many existing USB-C tools focus on one narrow slice of the ecosystem:

- Basic trigger boards request fixed voltages but do not inspect or alter protocol behavior.
- Passive analyzers observe CC traffic but cannot actively enforce alternate policies.
- Thunderbolt and USB4 products focus on high-speed signaling and DMA concerns after mode entry.
- Charger fuzzers target power bricks in a bench setup but are not optimized for fieldable inline red-team use.

Dockruptor deliberately combines **inline CC interception**, **policy-controlled mutation**, **identity spoofing**, **event capture**, **battery-backed covert deployment**, and a **mobile operator interface** into one instrument. The goal is not lab-only protocol decoding; the goal is a practical red-team device that can be clipped between a target and a trusted accessory long enough to answer high-value security questions.

Novel design traits include:

1. **Dual-sided USB-C policy brokering** rather than single-ended trigger behavior.
2. **Cable identity virtualization**, allowing eMarker and SOP′/SOP″ response emulation for cable trust testing.
3. **Alt-mode coercion engine** that can selectively downgrade, stall, or reshape mode entry.
4. **Pre-enumeration adversary logic**, useful even when the target never reaches a full USB bus session.
5. **Field-safe bypass architecture** with a hardware relay path that defaults to continuity on fault or battery collapse.
6. **Engagement profiles** for kiosk, secure laptop dock, hardened tablet, and hostile charger scenarios.

---

## 3. Purpose and Security Utility

Dockruptor exists to answer questions that ordinary endpoint tools cannot:

- Will a secure laptop accept lower-than-expected power and silently enter a throttled or recovery state that changes operator behavior?
- Can a red team suppress DisplayPort alt-mode just long enough to push a user toward plugging in a second, attacker-controlled accessory?
- Does a kiosk unlock privileged maintenance paths only when a “trusted” dock identity appears?
- Can a device be induced to source VCONN or change data role in a way that exposes unexpected peripherals?
- How resilient is BIOS or bootloader logic to malformed PD vendor-defined messages?
- Do embedded tablets validate cable and dock identity, or do they merely trust the first acceptable contract?
- Can corporate docking standards be abused to force charging-only mode, preventing a user from noticing that data lanes are absent or rerouted?

For defenders, those questions translate into concrete assessment goals:

- Validate dock allow-listing and accessory trust logic.
- Exercise hardware-rooted policy before the OS can respond.
- Reproduce malicious charging infrastructure conditions.
- Test high-assurance laptops, field equipment, and rugged devices that may spend most of their life attached to external USB-C accessories.
- Document the operational impact of PD/alt-mode tampering in a repeatable and measurable way.

---

## 4. Attack Surface and Threat Model

### 4.1 Attack Surface

Dockruptor targets the **USB-C control and policy plane** rather than only the USB data plane. Specific attack surfaces include:

1. **CC line negotiation** for source/sink attach detection and role resolution.
2. **USB Power Delivery messaging** including Source_Capabilities, Request, Accept, PS_RDY, Soft_Reset, and structured VDMs.
3. **Cable identity and eMarker trust**, especially where hosts gate functionality on cable capability.
4. **Alternate Mode entry**, especially DisplayPort and vendor-specific mode transitions.
5. **Role swaps** including PR_SWAP, DR_SWAP, and VCONN swap handling.
6. **Boot-stage behavior** when the system is not yet under full OS policy control.
7. **Operational workflows** that assume a known-good dock, charger, or conference room adapter.

### 4.2 Likely Defenders and Targets

Common environments where Dockruptor is useful include:

- Enterprise hot-desk and hoteling fleets using managed docks.
- Executive or SOC laptops with strict software hardening but permissive accessory trust.
- OT field tablets and handhelds charged through service cradles.
- Kiosks and digital signage players that rely on USB-C for both display and maintenance access.
- Secure workspace conference systems with one-cable docking assumptions.
- Rugged Android or Windows endpoints used by logistics, healthcare, utilities, or aviation teams.

### 4.3 Adversary Model

Dockruptor assumes a researcher or red team operator has:

- Short-term physical access to insert an inline adapter.
- Familiarity with target charging and docking behavior.
- A need to avoid obvious endpoint instability while still changing policy outcomes.
- A desire to preserve evidence and exact negotiation traces.

The device is also useful for vendors performing **adversarial product validation** against their own hardware. In that case, the adversary model is the malicious accessory vendor, malicious public charger, or compromised supply-chain dock.

### 4.4 Security Questions Dockruptor Helps Answer

- Does the target authenticate trusted accessories in hardware, firmware, or not at all?
- Can malformed or replayed VDM responses trigger unsafe state transitions?
- How much variance in PDO ordering or power limits can the endpoint tolerate before reliability degrades?
- Is cable capability enforced or merely advisory?
- What happens when a device is offered a plausible but slightly incorrect alt-mode path?
- Are maintenance or developer menus exposed only when specific accessories are detected?

### 4.5 Operational Safety Boundary

Dockruptor is not intended for uncontrolled power abuse, destructive over-voltage scenarios, or arbitrary high-speed lane fault injection. The design specifically constrains itself to **authorized low-level policy manipulation** on the CC/PD path while providing a relay bypass path and conservative power guardrails. It is a security assessment tool, not a sabotage device.

---

## 5. Hardware Specifications

### 5.1 Core Processing

- **Primary MCU:** STM32H735IGK6
  - Cortex-M7 @ 550 MHz
  - Large SRAM for capture buffers, policy execution, and UI telemetry
  - High-speed SPI, I2C, ADC, timer blocks, USB FS for service mode
- **Control-plane coprocessor:** ESP32-C6-WROOM-1
  - BLE 5 for quiet close-range operator control
  - Wi-Fi 6 2.4 GHz for bench mode and log export
- **PD/CC state engine assist:** ICE40UP5K FPGA
  - Deterministic edge timing, protocol assist, and SOP/SOP′/SOP″ framing support

### 5.2 USB-C Front End

- **2 × USB-C receptacles** configured as target-side and accessory-side inline ports
- **2 × FUSB307B USB Type-C / PD controllers** under MCU supervision
- **TUSB1046A-DCI** retimer / mux for DisplayPort alt-mode path continuity testing
- **Load-switch and ideal-diode arrangement** for safe VBUS path management
- **VCONN switching stage** with current monitoring
- **Hardware bypass relay** that preserves passive continuity on watchdog or power fault

### 5.3 Measurement and Telemetry

- INA231 or equivalent high-side monitor on VBUS
- ADC channels for battery, thermals, VCONN current, and rail health
- Optional hall sensor to detect enclosure open state for lab chain-of-custody logging
- MicroSD card slot for long-form trace capture

### 5.4 Connectivity

- BLE 5 LE control link for mobile app
- Wi-Fi control and export mode for lab use
- USB-C service port exposing a CDC console and firmware update endpoint
- Sideband debug pads for SWD and FPGA programming

### 5.5 Power

- 1-cell 3200 mAh LiPo pack
- USB-C charging and passthrough service power
- BQ25713 buck-boost charger / power-path controller
- 3.3 V, 1.2 V, and 1.8 V rails for MCU, FPGA, and front-end logic
- Estimated run time: 5-7 hours capture-only, 3-4 hours active mutation plus wireless control

### 5.6 Mechanical Form Factor

- Approximate enclosure: **118 mm × 68 mm × 19 mm**
- Aluminum or glass-filled nylon shell
- USB-C ports on opposite edges to support clean inline placement
- OLED status strip and tri-color role LEDs
- Concealable pouch-friendly profile for field assessments

---

## 6. System Architecture

Dockruptor is split into four cooperating planes:

1. **Physical inline plane** — preserves VBUS, CC, and alt-mode sideband continuity.
2. **Protocol engine plane** — captures and rewrites PD policy messages within deterministic bounds.
3. **Management plane** — applies rules, stores traces, exposes telemetry, and coordinates fail-safe transitions.
4. **Operator plane** — BLE/Wi-Fi connected app for scenario selection, live state viewing, and evidence export.

### 6.1 Text Block Diagram

```text
   [Target Host Port]                        [Dock / Charger Port]
           │                                         │
      USB-C Receptacle                         USB-C Receptacle
           │                                         │
      ESD / CM Chokes                           ESD / CM Chokes
           │                                         │
       FUSB307B A                                 FUSB307B B
           │            SOP / SOP' / SOP''           │
           ├───────────────┬───────────────┬─────────┤
           │               │               │
           │        iCE40UP5K Timing / Framing Assist
           │               │               │
           └────── STM32H735 Policy and Capture Engine ──────┐
                              │                              │
                      QSPI / microSD storage           ESP32-C6 BLE/Wi-Fi
                              │                              │
                         OLED / LEDs                    Mobile Application
                              │
                        Bypass Relay + Watchdog
```

### 6.2 Data and Control Flow

- The FUSB307B devices terminate and observe attach state, CC signaling, and PD message flow on both ends.
- The FPGA provides timing-safe framing assistance, message buffering, and fast path acknowledgment windows.
- The STM32H7 is the policy brain. It decides whether to forward, mutate, replay, suppress, or synthesize messages.
- The power subsystem continually evaluates battery margin, thermal budget, and relay state; if unsafe, the bridge reverts to passive continuity.
- The ESP32-C6 exposes a mobile control plane, profile upload, and trace export service.

---

## 7. Firmware Design

Dockruptor firmware is intentionally modular and split across several subsystems:

### 7.1 `pd_engine`

This driver maintains a model of both ports, active contracts, role state, message counters, and alt-mode progress. It parses simplified PD frames, tracks VDM identity fields, and produces consistent summaries that can be used by the app and capture stack.

### 7.2 `policy`

The policy layer applies operator-selected rules. Example rule classes include:

- Reorder PDOs to push a device toward an underpowered but still valid contract.
- Clamp advertised current or voltage ceilings.
- Deny alt-mode entry during a specific boot window.
- Force cable identity downgrade to simulate a low-capability passive cable.
- Trigger periodic soft resets to test recovery logic.
- Allow only charging-only behavior while keeping telemetry running.

The implementation is bounded: it tracks strike counts, timing budget, and guardrail reasons. If a rule triggers too often or exceeds the jitter budget, the device automatically degrades to observe-only mode.

### 7.3 `capture`

The capture layer stores every event of interest as a structured record: timestamp, direction, message type, mutation result, state transition, battery condition, and safety annotations. Captures can be exported over BLE or Wi-Fi and are also summarized locally for on-device review.

### 7.4 `radio`

The radio stack is modeled as a control-plane service with profile loading, telemetry serialization, simple command processing, and evidence export. In a production firmware build, this module would bridge to the ESP32-C6; in the simulation-oriented reference implementation included here, it provides deterministic mock commands for repeatable testing.

### 7.5 `power`

The power module tracks battery percentage, thermals, bypass relay state, and whether a safe operating envelope still exists. It is intentionally opinionated: if the battery sags or the policy loop becomes unstable, the relay path is asserted and the operator is notified.

### 7.6 Safety and Reliability Decisions

Key design decisions include:

- **Default-bypass philosophy:** continuity beats cleverness when system health is uncertain.
- **Explicit mutation accounting:** every changed capability or role transition is logged.
- **Profile-based engagement:** prevents ad hoc field changes that are hard to reproduce later.
- **No high-speed lane rewriting in reference design:** Dockruptor focuses on CC/PD and mode policy, not destructive line-rate packet tampering.
- **Authoritative evidence capture:** every operator action is reflected in a structured event log.

---

## 8. Companion Application

The companion app is a React Native / Expo application written by **jayis1** and intended for tablets or phones used during a test. The app presents:

- **Dashboard:** live attach state, negotiated voltage/current, alt-mode state, cable identity summary, and battery margin.
- **Policy Console:** profile selection, mutation knobs, role-swap tests, alt-mode denial windows, and charging-only enforcement.
- **Identity View:** cable eMarker and dock identity snapshots plus spoof targets.
- **Capture Browser:** chronological event log with filter chips and export markers.
- **Safety Screen:** relay status, thermal state, safe-mode countdowns, and authorized-use reminder.

The UI is deliberately mobile-first because Dockruptor is expected to be used in hallways, conference rooms, cabinets, field sites, and live exercise spaces where a full laptop console is awkward.

---

## 9. Use Cases

### 9.1 Red Teams

- Test whether a managed laptop can be nudged into a weaker or unusual dock profile before login.
- Simulate a malicious public charger at an executive travel site.
- Evaluate whether a field tablet exposes maintenance capabilities after a dock identity replay.
- Downgrade a cable identity to provoke operator behavior that leads to secondary accessory insertion.

### 9.2 Product Security Teams

- Validate firmware handling of malformed or delayed VDM sequences.
- Confirm contract negotiation behavior across cable classes.
- Ensure boot firmware does not unlock hidden paths on trusted accessory appearance alone.
- Test UI warnings when an endpoint is power constrained or alt-mode is denied.

### 9.3 Penetration Testers

- Determine if kiosk devices trust “charging” accessories too readily.
- Build evidence for risks around conference room docks and unmanaged adapters.
- Demonstrate pre-OS trust abuse without needing high-speed bus attacks.

### 9.4 Industrial / Field Service Assessments

- Review service cradles and rugged tablet trust assumptions.
- Check if maintenance handhelds accept downgraded or spoofed cable roles that alter behavior.
- Validate whether portable embedded endpoints distinguish between approved and unapproved power accessories.

---

## 10. Example Engagement Workflow

1. Insert Dockruptor inline between the target and its normal dock or charger.
2. Allow the system to learn baseline behavior in observe-only mode for several attach cycles.
3. Export the baseline trace and note accepted PDOs, VDM identity, and alt-mode timing.
4. Enable one mutation rule at a time, such as cable downgrade or delayed DisplayPort entry.
5. Observe endpoint UI, boot logs, or operational behavior.
6. If instability occurs, let Dockruptor auto-bypass and capture the trigger reason.
7. Export the resulting evidence set for the engagement report.

This workflow is intentionally conservative. The device is built for **repeatable validation**, not chaos-first fuzzing.

---

## 11. Design Files Included

This folder contains:

- `firmware/` — compile-ready C reference firmware and simulation harness.
- `kicad/` — schematic, PCB, and KiCad project files for the hardware concept.
- `app/` — React Native companion application with live operational screens.

All design artifacts are credited to **jayis1**.

---

## 12. Bill of Materials Highlights

Representative primary components:

- STM32H735IGK6 MCU
- ESP32-C6-WROOM-1 wireless module
- Lattice iCE40UP5K FPGA
- 2 × FUSB307B USB-C / PD controllers
- TUSB1046A-DCI retimer / mux
- BQ25713 charger / power-path IC
- INA231 power monitor
- Winbond W25Q128JV QSPI flash
- microSD socket
- 0.96" OLED status display
- LiPo pack and protection circuit

---

## 13. Ethical and Legal Disclaimer

Dockruptor is for **authorized use only**. Any deployment against systems, chargers, docks, kiosks, laptops, tablets, or embedded equipment without explicit permission may violate law, policy, contract, or safety expectations. Because USB-C negotiation can affect power state, display routing, and attached peripherals, every test must be planned with rollback steps, operator awareness, and asset-owner consent.

---

## 14. Closing Notes

Dockruptor is intentionally designed at the seam between hardware trust and operator trust. The most valuable attacks against modern endpoints are often not the loudest ones; they are the subtle shifts in policy that occur before the endpoint decides what world it is plugged into. By making that trust boundary visible, controllable, and testable, Dockruptor gives defenders a way to move USB-C security assessment earlier in the chain—where many real risks begin.
