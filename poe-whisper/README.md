# PoE Whisper — Inline 802.3af/at/bt Power-Negotiation Manipulator for Security Research

![status](https://img.shields.io/badge/status-design-green) ![author](https://img.shields.io/badge/author-jayis1-orange) ![license](https://img.shields.io/badge/license-GPL--2.0-blue) ![hardware](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> Author: **jayis1**  
> Hardware License: **CERN-OHL-S v2**  
> Firmware/App License: **GPL-2.0-only**

---

## Legal and Ethical Notice

**PoE Whisper is for authorized use only.** This design is intended strictly for defensive security research, red-team exercises conducted under written authorization, hardware validation, and laboratory study of Power over Ethernet devices and their supporting management protocols. Deploying inline power manipulation hardware against networks, cameras, access-control systems, industrial endpoints, telephones, wireless access points, or any other infrastructure that you do not own or explicitly control may violate criminal law, civil law, wiretap restrictions, tampering statutes, physical security policies, safety regulations, and contractual obligations. The author, **jayis1**, provides this design for educational and authorized assessment purposes only and assumes no liability for misuse, outages, unsafe operation, or downstream damages.

PoE manipulation carries real operational risk. Many field devices trust their upstream power source completely; coordinated power dips, class renegotiation, or LLDP-MED spoofing can force reboots, trigger storage corruption, desynchronize clocks, or disrupt surveillance coverage. Use only in controlled test windows with rollback plans and stakeholder approval.

---

## 1. Device Purpose and Overview

**PoE Whisper** is a new inline hardware implant for security research that sits between a Power Sourcing Equipment (PSE) port—typically a switch, injector, or midspan—and a Powered Device (PD) such as an IP camera, door controller, wireless access point, VoIP phone, badge reader, thin client, or embedded edge appliance. The device observes and selectively alters the full trust boundary where electrical power delivery, discovery, negotiation, and management converge.

Traditional network taps focus on packets and ignore the fact that modern enterprise Ethernet often carries both **data and authority**. Power over Ethernet is not merely a voltage source; it is a policy plane. During startup, the switch probes for a valid detection signature, classifies the endpoint, decides how much budget to allocate, optionally exchanges LLDP or LLDP-MED power information, and then maintains power while the endpoint's behavior may depend on negotiated class, cable quality, and power headroom. That entire sequence becomes a powerful attack surface when studied from an inline, protocol-aware, electrically capable interposer.

PoE Whisper was invented specifically to explore that neglected surface. It combines an **STM32H563** control MCU, a **Lattice MachXO3LF** policy FPGA/CPLD fabric for sub-microsecond timing control, differential current/voltage sensing, dual Ethernet magnetics passthrough, and a BLE/Wi-Fi control coprocessor. The result is a device that can remain electrically transparent in passive monitor mode or become an active man-in-the-middle for PoE negotiation and power-behavior experiments.

The novel aspect of PoE Whisper is that it treats power negotiation as a first-class offensive and defensive research channel. Instead of simply cutting power like a smart PDU or replaying Ethernet frames like a tap, it can:

- fingerprint a switch's 802.3af/at/bt detection cadence,
- emulate alternate PD signatures to explore budget allocation logic,
- inject carefully bounded brownout waveforms during boot phases,
- spoof LLDP power requests and inventory identity,
- force power-class downgrades or renegotiation loops,
- map how devices respond when data stays up but power policy shifts,
- correlate current draw with application state for side-channel telemetry,
- and test how resilient security cameras, door readers, and OT edge devices are when their upstream power assumptions are violated.

This makes the device practical for red teams validating physical resilience, for embedded defenders hardening field devices, and for hardware researchers studying PoE controller behavior under adversarial timing.

---

## 2. Why This Device Matters

PoE endpoints are widely deployed in places where physical access is realistic and impact is immediate: office ceilings, conference rooms, utility closets, exterior gates, building lobbies, factory lines, branch offices, retail stores, transport hubs, and temporary event infrastructure. Security programs often assume that if the Ethernet link is encrypted and the switch is managed, the edge is safe. In practice, many of the most important devices in a building—surveillance, wireless, badge access, room controls, and small industrial bridges—depend on PoE startup policy that almost nobody monitors.

An inline adversary or tester who manipulates that policy can explore questions that packet-only tools cannot answer:

- Does a camera boot into a weaker mode after repeated power-class instability?
- Does an access point temporarily expose an unprovisioned SSID after a brownout reboot?
- Can a VoIP phone be pushed into maintenance mode through constrained power?
- Does a door controller fail open or fail secure when LLDP power is withdrawn without losing Ethernet carrier?
- Can dynamic current signatures reveal when a camera pan motor moves, when IR emitters turn on, or when a lock strike fires?
- Will a switch trust spoofed LLDP-MED inventory from an inline implant and adjust power budget incorrectly?

PoE Whisper exists to answer those questions with instrumentation rather than guesswork.

---

## 3. Attack Surface and Threat Model

### 3.1 Attack Surface

PoE Whisper targets several layered surfaces simultaneously:

1. **Physical pair power negotiation** — IEEE 802.3 detection, classification, maintain-power signature, and power event timing.
2. **Power-management protocols** — LLDP / LLDP-MED TLVs that request or advertise power class, priority, and inventory identity.
3. **Endpoint boot behavior under power stress** — filesystem integrity, secure boot fallback, crash loops, RF startup state, and watchdog recovery.
4. **Current-consumption side channels** — device activity inference from differential shunt and Hall sensor traces.
5. **Power budget trust decisions** — switch allocation logic when confronted with replayed, delayed, or malformed signaling.
6. **Operational blind spots** — conditions where data forwarding persists while power quality is intentionally perturbed.

### 3.2 Adversary Model

The assumed operator has brief authorized physical access to an exposed Ethernet run or patch lead and can insert an inline interposer between PSE and PD. No switch credentials are required for baseline monitoring. Advanced features optionally leverage observation of LLDP or known endpoint profiles to tune active modes.

### 3.3 Security Goals

PoE Whisper supports authorized evaluation of whether target infrastructure:

- tolerates power sag and rapid renegotiation safely,
- validates or blindly trusts LLDP power requests,
- leaks operational state in current signatures,
- becomes less secure after constrained or repeated power restarts,
- and can detect an inline electrical MITM at all.

### 3.4 Safety Boundaries

The design intentionally includes protective limits: bounded brownout duration, overcurrent cutout, thermal shutdown, passive bypass relays, and default boot in capture-only mode. These are not a substitute for authorization or engineering judgment, but they reduce accidental damage during legitimate testing.

---

## 4. Hardware Specifications

| Category | Specification |
|---|---|
| Main MCU | **STM32H563RIT6** — ARM Cortex-M33 @ 250 MHz, TrustZone capable, rich timers, ADCs, FDCAN/UART/SPI/I2C |
| Timing / policy fabric | **Lattice MachXO3LF-2100C** used for deterministic PoE edge timing, comparator latching, relay sequencing |
| Wireless control | **ESP32-C3-MINI-1** for BLE 5 + Wi-Fi AP mode for local operator control |
| Ethernet data path | 2x gigabit magnetics modules, passive differential passthrough with optional tap ADC headers |
| PoE controller | **TPS2373-4** style PD front-end combined with controlled emulation network and high-side gate stage |
| Measurement | INA238 current/voltage monitor, shunt amplifier, ADS7042 fast ADC sampler, NTC thermistor |
| Switching elements | Back-to-back MOSFET ideal-diode stage, bypass relay pair, programmable class resistor ladder, maintain-power signature shaper |
| Connectors | RJ45 in / RJ45 out, USB-C service, Tag-Connect debug footprint, JST battery backup |
| Power modes | Passive parasitic monitor, fully powered inline active mode, bench-assisted calibration mode |
| Form factor | 94 mm × 52 mm 4-layer rigid PCB with shield can over power stage |
| Enclosure concept | Slim inline aluminum shell with magnetic lid and strain-relieved patch leads |
| Indicators | Covert tri-color LED, buzzer disable jumper, tactile safe-mode switch |
| Author marking | All silkscreen, metadata, and documentation credit **jayis1** |

### Radio and Connectivity Summary

- BLE for short-range field operation from a phone.
- Wi-Fi AP mode for laptop control where BLE is unreliable in noisy environments.
- USB-C serial console for lab calibration and firmware updates.
- Passive Ethernet passthrough that does not terminate or switch payload traffic.

### Sensor Summary

- Line current and voltage telemetry per powered pair set.
- Fast transient capture during detection/classification.
- Board temperature and power-stage thermal state.
- Link and relay state from FPGA fabric.

---

## 5. Architecture

### 5.1 Functional Blocks

```
                 ┌──────────────────────────────────────────────────────┐
   PSE RJ45 IN   │                    PoE Whisper                       │   PD RJ45 OUT
 ┌─────────────┐ │                                                      │ ┌─────────────┐
 │ Data Pairs  ├─┼──── Passive Magnetics / Differential Data Path ─────┼─┤ Data Pairs  │
 │ Power Pairs ├─┼──── Detection / Class / MPS Interposer Stage ───────┼─┤ Power Pairs │
 └─────────────┘ │                 │                     │              │ └─────────────┘
                 │                 │                     │              │
                 │       ┌─────────▼─────────┐  ┌───────▼────────┐     │
                 │       │ MachXO3LF Fabric  │  │ STM32H563 MCU   │     │
                 │       │ edge timing /     │  │ policy engine,  │     │
                 │       │ relay arbitration │  │ logging, safety │     │
                 │       └─────────┬─────────┘  └───────┬────────┘     │
                 │                 │                     │              │
                 │       ┌─────────▼────────┐   ┌───────▼─────────┐    │
                 │       │ Sensing Frontend │   │ ESP32-C3 Control │    │
                 │       │ current/voltage  │   │ BLE/Wi-Fi UI     │    │
                 │       └─────────┬────────┘   └───────┬─────────┘    │
                 │                 │                    USB-C           │
                 └─────────────────┴────────────────────────────────────┘
```

### 5.2 Data Path Philosophy

The Ethernet data path is intentionally boring: it is a low-disturbance passthrough. PoE Whisper is not meant to be another packet bridge or transparent firewall. Its value is that it can sit inline without disrupting normal Ethernet traffic while separately controlling the power-domain assumptions beneath that traffic. That separation lets a researcher ask whether security controls remain stable when power policy changes but Layer 2 framing does not.

### 5.3 Power Manipulation Engine

The power manipulation engine has three coordinated elements:

1. **Signature Emulation Network** — switched resistor ladders and timing gates to present alternative PD detection and classification behavior.
2. **Maintain-Power Signature Shaper** — controlled load pulses and hold intervals to study PSE keepalive logic.
3. **Brownout / Slew Controller** — MOSFET stage that can generate short, bounded voltage droops without hard cable disconnect.

Those elements are sequenced by the MachXO3LF so timing stays deterministic even when the MCU is busy streaming telemetry.

### 5.4 Safety and Transparency

A relay bypass pair can return the line to direct pass-through on watchdog expiry, thermal fault, or operator safe-mode selection. This is essential in field assessments where the tester must fail safe if the control plane crashes.

---

## 6. Firmware Design and Decisions

The firmware in `firmware/` is intentionally substantial and structured as compile-ready C, authored by **jayis1**, with a host-buildable simulation mode so researchers can exercise the logic before targeting hardware. The design uses multiple modules:

- `poe_port` models detection, classification, allocation, and inline state.
- `lldp` parses and synthesizes compact LLDP/LLDP-MED power and identity profiles.
- `signature` turns current traces into operational events and anomaly scores.
- `relay` enforces bounded brownouts, bypass, and safety cutoffs.
- `radio` exposes a simple framed command protocol for the companion app.
- `main.c` orchestrates scenarios, applies profiles, and prints a deterministic simulation transcript.

### Design Rationale

- **Host-compilable first**: the firmware compiles with GCC on a Linux workstation, making the logic inspectable without vendor SDK baggage.
- **Clear control/data separation**: line timing and relay arbitration are abstracted so future hardware-specific backends can replace the simulation layer cleanly.
- **Traceable decisions**: every power-policy action carries reason codes and event logging.
- **Bounded active behavior**: no indefinite power disruption primitives exist; every disruptive action is constrained by time and current ceilings.
- **Reproducible profiles**: attack and validation profiles are encoded as data tables so red-team reports can reproduce results exactly.

### Firmware Capabilities

- PSE fingerprinting and budget estimation
- PD class spoofing and LLDP power request shaping
- current-draw signature capture with event labeling
- bounded brownout scheduling during selected boot windows
- passive capture-only mode
- safety interlock mode with thermal and overcurrent rollback
- simple operator command and telemetry framing over radio link

---

## 7. Application / Software Interface

The companion application in `app/` is a React Native design by **jayis1**. It is intentionally real rather than a placeholder: it includes multiple screens, protocol helpers, device status cards, capture rendering, scenario toggles, and safety checklist views.

### App Screens

1. **Overview Dashboard** — link state, allocated wattage, active profile, thermal margin, and event counters.
2. **Profiles** — one-touch loading of research scenarios such as `camera-reboot-window`, `phone-class-downgrade`, `ap-llpd-spoof`, and `badge-reader-mps-jitter`.
3. **Capture** — current-signature event stream, LLDP advertisements, and brownout actions.
4. **Injector** — toggles for class spoofing, LLDP identity override, and power droop duration presets.
5. **Safety** — interlock state, rollback checklists, and operator reminders.

### Protocol

The app consumes a simple structured protocol represented in `app/utils/protocol.js`. In a lab implementation, this would map to BLE characteristics or a Wi-Fi websocket. For this design package, it provides deterministic mock data and transforms that mirror how the firmware modules think about profiles and events.

---

## 8. Use Cases for Red Teams, Researchers, and Penetration Testers

### 8.1 Surveillance Camera Resilience Testing

Many IP cameras perform secure boot, initialize storage, power IR emitters, start codecs, and then join VMS infrastructure—all within a power envelope that changes sharply across the first 20 seconds. PoE Whisper can characterize that boot waveform, identify vulnerable windows, and then repeat a bounded brownout exactly during those windows. A researcher can evaluate whether the camera corrupts storage, comes up in an insecure maintenance mode, drops to default credentials, delays certificate enrollment, or exposes open RTSP before the hardening agent starts.

### 8.2 Wireless Access Point Post-Reboot Exposure

Enterprise APs often broadcast temporary provisioning SSIDs, reset mesh backhaul state, or revert radio power settings briefly after non-graceful power events. Using PoE Whisper, a team can maintain Ethernet link presence while repeatedly nudging the power rail to reproduce edge reboots and observe whether post-boot onboarding windows expose soft spots.

### 8.3 Access-Control and Physical Security Validation

Door controllers, intercoms, badge readers, and elevator edge nodes are frequently PoE-powered and physically reachable. The device can evaluate whether they fail open, fail secure, or enter degraded network states under constrained power or LLDP withdrawal. That is highly relevant in real-world physical red-team scenarios.

### 8.4 Side-Channel Occupancy Inference

Current signatures can reveal meaningful behavior without touching packet payloads. Camera pan motors, IR LED activation, relay strikes, speaker use, touchscreen wake events, or AP radio load can all create recognizable power patterns. PoE Whisper gives defenders a way to understand how much operational metadata leaks through the power plane alone.

### 8.5 Switch Policy Hardening

Blue teams can place the device inline during acceptance testing to validate whether switch firmware safely handles malformed or shifting classification, whether LLDP power requests are sanity checked, and whether logs clearly indicate negotiation anomalies.

### 8.6 OT and Building Automation Research

In industrial and smart-building environments, many small Linux gateways, BACnet bridges, and serial concentrators are powered over Ethernet yet monitored primarily at the IP layer. PoE Whisper lets researchers test the underlay assumptions those devices quietly depend on.

---

## 9. Threat Model Details

### Assets at Risk

- device availability,
- physical access workflows,
- recorded surveillance continuity,
- edge network trust,
- endpoint configuration integrity,
- and operator awareness.

### Likely Defender Blind Spots

- switch logs usually summarize the final power class, not the transient history;
- EDR agents rarely monitor why a device rebooted electrically;
- many field devices lack tamper evidence for inline cable implants;
- facilities teams and cyber teams often own different halves of the problem.

### Attacker Constraints

- inline insertion usually requires touching a visible patch lead,
- long sustained overcurrent is prevented by hardware interlocks,
- active manipulation is intentionally bounded to support safe authorized testing.

---

## 10. Hardware Bring-Up Plan

1. Validate passive Ethernet insertion loss with the relay bypass path engaged.
2. Calibrate voltage/current measurement chain against a programmable PoE load.
3. Confirm signature ladder timing through the MachXO3LF on af/at/bt sources.
4. Verify bypass fallback on MCU watchdog reset and thermal trip.
5. Reproduce LLDP power negotiation with multiple switch vendors.
6. Profile common endpoints and store scenario templates.

---

## 11. Directory Layout

```
poe-whisper/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── poe_port.c
│       ├── poe_port.h
│       ├── lldp.c
│       ├── lldp.h
│       ├── signature.c
│       ├── signature.h
│       ├── relay.c
│       ├── relay.h
│       ├── radio.c
│       └── radio.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── package.json
    ├── App.js
    ├── components/
    ├── screens/
    └── utils/
```

---

## 12. Practical Novelty Summary

PoE Whisper is not just another tap, reboot switch, or packet injector. Its novelty is that it turns **electrical negotiation itself** into a controlled research interface. It lives at the seam between physical infrastructure and logical trust, which is where many overlooked failures happen. By combining transparent data passthrough with programmable power behavior, the design gives authorized testers a way to evaluate resilience that neither packet-only tools nor coarse smart PDUs can provide.

For red teams, it opens a realistic path to testing how critical edge devices behave when power policy is manipulated rather than packet payloads. For defenders, it exposes failure modes before adversaries do. For embedded engineers, it provides a clear architecture for instrumenting and hardening the PoE attack surface.

All design materials in this folder were created with **jayis1** credited as the author.
