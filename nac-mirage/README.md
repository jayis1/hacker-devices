# NAC Mirage — Inline PoE, LLDP-MED, and NAC Deception Bridge

**Author:** jayis1  
**License:** Hardware — CERN-OHL-S v2 · Firmware — GPL-2.0-only · App — MIT  
**Version:** 1.0.0

> **Legal / Ethical Notice:** NAC Mirage is designed strictly for authorized security research, defensive validation, red-team exercises, product security testing, and lab use performed with explicit written permission from the owner of the infrastructure and connected endpoints. Do not deploy or operate this device against production networks, life-safety systems, public infrastructure, medical environments, emergency telephony, or third-party assets without authorization.

---

## 1. Overview

NAC Mirage is a **field-deployable inline Ethernet security research platform** built to test an under-examined trust boundary in modern enterprise and operational technology environments: the interaction among **Power over Ethernet (PoE)**, **LLDP / LLDP-MED discovery**, **802.1X / NAC workflows**, and **endpoint auto-provisioning behavior** that takes place before a defender thinks of an endpoint as fully online.

The device sits physically **between a switch and an endpoint**. On one side it faces the network access switch, on the other side it faces the target device such as a VoIP handset, badge reader, IP camera, digital signage player, thin client, conference room appliance, industrial controller interface, or hardened access terminal. NAC Mirage is not simply a passive tap and not just a transparent bridge. It is an **active policy broker** that can:

- Observe and log PoE power classification and draw behavior.
- Capture and mutate LLDP and LLDP-MED advertisements.
- Delay or reshape endpoint link establishment.
- Intercept or stall NAC-related timing in authorized lab scenarios.
- Present decoy VLAN, voice VLAN, and provisioning metadata to endpoints.
- Explore whether embedded or enterprise devices over-trust switching fabric metadata.
- Preserve a capture trail so defenders can reproduce and fix discovered weaknesses.

That matters because a large class of devices make early trust decisions based on infrastructure metadata rather than user-driven workflows. A VoIP phone may request firmware and a provisioning file because of a switch-advertised voice VLAN. A camera may downgrade features or trigger recovery behavior under constrained PoE budget. A badge reader may obtain a maintenance address range after LLDP hints and NAC sequencing align in a specific way. A hardened tablet dock or conference controller may assume the first environment it sees is genuine and safe. Those are operational assumptions, but they are also **attack surface**.

NAC Mirage gives red teams, defenders, and hardware/software vendors a way to validate whether those assumptions hold when the network edge behaves in deceptive but plausible ways. Instead of attacking only after the endpoint has obtained an IP address, NAC Mirage lets a researcher assess the **pre-auth and pre-provisioning path** where infrastructure identity, power budget, VLAN hints, and timing signals can shape what the endpoint does next.

---

## 2. Why NAC Mirage Is Novel

Security tooling for Ethernet edge assessments usually falls into one of a few categories:

1. **Passive taps** that record traffic but do not shape the endpoint’s view of the access network.
2. **Standard inline bridges** that forward frames without applying deceptive infrastructure behavior.
3. **NAC bypass tooling** that focuses on logical protocol games at the host level once a machine is already connected.
4. **PoE analyzers** that measure power but do not integrate with VLAN or LLDP-MED deception.
5. **VoIP labs** that can emulate phone ecosystems but are not designed as portable inline red-team hardware.

NAC Mirage intentionally combines those domains into one cohesive platform. Its novelty comes from treating the first few seconds of edge attachment as a security boundary with multiple coupled variables:

- **Power identity**: how much power is available and how the endpoint reacts to reduced or shaped headroom.
- **Infrastructure identity**: what the endpoint believes about the switch, port, and provisioning domain.
- **Role identity**: whether the endpoint believes it is in a voice, camera, access-control, or data port context.
- **Timing identity**: whether EAPOL, DHCP, LLDP-MED, and higher-layer provisioning events arrive in the expected sequence.

NAC Mirage is therefore not just “an Ethernet implant” or “a PoE tool.” It is a **research platform for infrastructure trust coercion**. It exists to answer questions like:

- Will an IP phone accept a false voice VLAN advertisement and request provisioning from a staging environment?
- Can a camera or badge reader be nudged into a recovery or diagnostic state through carefully constrained PoE budget changes?
- Do access terminals enforce LLDP and NAC coherence, or do they trust whichever early metadata arrives first?
- Can defenders detect infrastructure impersonation when the manipulation occurs below the normal endpoint logging surface?

That makes the device useful not only for offensive simulation but also for **design validation**, **purple-team exercises**, and **product assurance testing**.

---

## 3. Purpose and Practical Utility

NAC Mirage is designed for environments where edge-connected devices are operationally important and often under-monitored. Typical targets include:

- Enterprise VoIP phones and conference room devices.
- IP cameras and physical security appliances.
- Badge readers and building automation endpoints.
- Retail kiosks and digital signage systems.
- Thin clients, branch office appliances, and hardened service consoles.
- OT-adjacent Ethernet endpoints that depend on PoE and infrastructure hints.

Practical uses include:

- **Testing whether endpoint provisioning logic is too trusting.**
- **Measuring the security impact of voice VLAN misdirection.**
- **Studying how endpoints behave under partial PoE degradation.**
- **Checking whether defensive monitoring catches LLDP-MED inconsistencies.**
- **Validating switch and NAC hardening against infrastructure impersonation or relay conditions.**
- **Reproducing early-boot or pre-user behaviors that are otherwise difficult to stage with software-only tools.**

The device is especially useful for red teams because many organizations harden laptops and servers well, yet trust their edge infrastructure model by default. Phones, cameras, kiosks, and readers are often treated as “managed devices” even when their attachment logic has never been adversarially tested. NAC Mirage provides that adversarial lens.

---

## 4. Attack Surface and Threat Model

### 4.1 Attack Surface

NAC Mirage focuses on a blended attack surface made of electrical, link-layer, and provisioning metadata:

1. **PoE classification and power draw behavior** — whether endpoints change features, boot paths, or trust assumptions when budget changes.
2. **LLDP / LLDP-MED advertisements** — switch identity, port role, data VLAN, voice VLAN, location, and power negotiation hints.
3. **802.1X / NAC timing** — whether delay, suppression, or sequencing changes cause unexpected authorization paths.
4. **DHCP / provisioning cascades** — whether endpoints follow a different chain of servers when coerced into alternate VLANs or operational profiles.
5. **Operational workflows** — whether staff trust a phone, panel, or controller that appears “normal” after attachment even though the first seconds were manipulated.

### 4.2 Adversary Model

NAC Mirage assumes a researcher or authorized red team has:

- Short-duration physical access to insert an inline device.
- Prior reconnaissance indicating the endpoint relies on PoE and switch metadata.
- A requirement to preserve service continuity when possible.
- A need to record exactly what was observed and altered.

For vendor testing, the assumed adversary may be:

- A malicious or compromised access switch.
- A rogue inline adapter placed in a ceiling space or cabinet.
- A malicious service technician tool.
- A supply-chain or maintenance workflow substitution.

### 4.3 Security Questions the Device Helps Answer

- Does the endpoint validate that voice VLAN and NAC results are coherent, or does it trust the first plausible LLDP advertisement?
- Can power budget shaping induce fallback interfaces, maintenance services, or provisioning retries?
- Do access switches or SIEM detections notice inconsistent LLDP identity versus authenticated authorization state?
- Are there device classes that will register to a call-control or management environment based solely on infrastructure hints?
- Can security teams document and detect infrastructure impersonation without waiting for malware-level indicators?

---

## 5. Hardware Specifications

### 5.1 Core Processing and Packet Path

- **MCU:** STM32H753IIK6
  - Chosen for deterministic real-time control, multiple high-speed peripherals, mature embedded tooling, and enough SRAM/flash to coordinate policy, capture indexing, and management APIs.
- **Inline packet path accelerator:** Xilinx Artix-7 XC7A35T
  - Used as a low-latency packet steering and timestamp engine for ingress/egress mirroring, relay control, and pre-filtering.
- **Dual PHYs:** 2 × KSZ9031RNX Gigabit Ethernet PHYs
  - One PHY faces the switch side and one faces the endpoint side.
- **Memory:** W25Q128JV QSPI flash + PSRAM buffer
  - Used for capture retention, profile storage, and deferred export.

### 5.2 Radio and Local Management

- **Secondary wireless management SoC:** ESP32-C6-WROOM-1
  - Provides Wi-Fi 6 / BLE management link for the companion app.
  - Segregated from the inline forwarding path to reduce perturbation risk.

### 5.3 Power Subsystem

- **PoE PD controller:** TPS2378
  - Supports powered-device extraction and classification in the management domain.
- **Primary converter:** LTC1871-based 48 V to system rail conversion.
- **Fail-open relay path:** Hardware relay / bypass arrangement that defaults to safe continuity if the control plane fails.
- **Transient protection:** TVS arrays on Ethernet pairs and management lines.

### 5.4 Connectivity

- 2 × shielded RJ45 MagJacks.
- Internal programming/debug header.
- Secure BLE / Wi-Fi management link for the companion app.
- QSPI flash for local capture retention.

### 5.5 Form Factor

- Inline in-wall / under-desk / field pouch form factor.
- Approximate target dimensions: **195 mm × 95 mm × 22 mm**.
- Designed so the relay defaults to a safe mode if firmware crashes or power collapses.

### 5.6 Sensors and Support Features

- Current/voltage sensing on the PoE-derived management rail.
- Relay position feedback.
- Tamper switch support for deployment awareness.
- Status LEDs can be disabled in covert or low-profile mode.

---

## 6. Architecture and Block Diagram

```text
        Switch / PSE Side                             Endpoint Side
    ┌─────────────────────┐                      ┌─────────────────────┐
    │   RJ45 MagJack J1   │                      │   RJ45 MagJack J2   │
    └─────────┬───────────┘                      └─────────┬───────────┘
              │                                                    │
         ┌────▼─────┐                                        ┌─────▼────┐
         │ KSZ9031A │                                        │ KSZ9031B │
         └────┬─────┘                                        └─────┬────┘
              │                 RGMII / relay fabric                │
              └───────────────┬─────────────────────────────────────┘
                              │
                       ┌──────▼──────┐
                       │ Artix-7 FPGA│  inline steering, timestamping,
                       │  packet path│  mirror and relay control
                       └──────┬──────┘
                              │
                       ┌──────▼──────┐
                       │ STM32H753   │ policy engine, capture index,
                       │ control MCU │ LLDP/NAC logic, export API
                       └──────┬──────┘
             ┌───────────────┼───────────────────┐
             │               │                   │
      ┌──────▼──────┐  ┌─────▼─────┐      ┌─────▼─────────┐
      │ QSPI Flash  │  │  PSRAM    │      │ ESP32-C6 Mgmt│
      │ capture log │  │ frame buf │      │ BLE/Wi-Fi app│
      └─────────────┘  └───────────┘      └───────────────┘
             │
      ┌──────▼──────┐
      │ TPS2378 +   │  PoE-derived management power,
      │ power stage │  sensing, bypass monitoring
      └─────────────┘
```

The architecture deliberately splits responsibilities:

- The **FPGA** manages packet timing and safe inline behavior.
- The **MCU** owns policy, capture metadata, LLDP parsing, radio coordination, and operator controls.
- The **ESP32-C6** isolates the mobile management link from the main inline data path.

This separation keeps the management plane from becoming the bottleneck in forwarding decisions while still enabling policy-rich experiments.

---

## 7. Firmware Design and Decisions

The firmware included in `firmware/` is a compile-ready simulation-oriented control plane in portable C authored by **jayis1**. It models the behavior of NAC Mirage’s major software subsystems and demonstrates the policy engine design:

- `main.c` sets up profiles, status tracking, subsystem initialization, and a simulated engagement.
- `drivers/bridge.c` simulates the inline bridge, packet staging, LLDP handling, EAPOL drop policy, and alert conditions.
- `drivers/lldp.c` implements LLDP-MED-style metadata parsing and mutation behavior.
- `drivers/poe.c` models PoE budget observation and controlled brownout-style shaping.
- `drivers/capture.c` stores structured event captures with timestamps and decision reasons.
- `drivers/radio.c` models secure operator pairing and status publication.
- `board.h` and `registers.h` define the hardware abstraction contract.

### 7.1 Policy Profiles

NAC Mirage uses named engagement profiles rather than raw register twiddling so an operator can reproduce behavior:

- **transparent** — observe only.
- **lab-phantom-phone** — mutate VLAN and voice metadata to emulate a staging phone environment.
- **camera-brownout** — shape PoE budget to test resilience under constrained power.
- **voice-vlan-decoy** — present alternate voice/data role assumptions.
- **nac-delay** — delay or suppress selected NAC timing paths.
- **staged-relay** — combined deceptive profile with LLDP mutation, link delay, and PoE shaping.

### 7.2 Safety-Oriented Logic

Even though this is a red-team research design, the firmware makes safety a first-class consideration:

- The runtime status tracks **bypass engaged** and **relay open** states separately.
- Capture logs explicitly label whether a frame was **forwarded**, **mutated**, **dropped**, or **alerting**.
- The architecture assumes a **fail-open hardware path** on watchdog failure.
- The companion app emphasizes authorization and operational coordination.

### 7.3 Why a Simulated Firmware Layout Is Useful

The included code is intentionally structured to be portable and readable rather than bound to a vendor SDK. That makes it suitable for:

- Algorithm review.
- Rapid host-side testing.
- Porting to the final BSP.
- Training additional researchers on the device architecture.

In a hardware bring-up phase, the same subsystem boundaries would map cleanly to PHY drivers, DMA rings, FPGA mailbox channels, and persistent storage services.

---

## 8. Application / Software Interface

The companion application in `app/` is a React Native / Expo interface authored by **jayis1**. It is structured around practical operator workflows rather than generic dashboards.

### Included Screens

1. **Overview**
   - Displays current profile, observed/mutated frame counts, PoE budget, and advertised VLAN.
2. **Profiles**
   - Allows selection of prebuilt policy packs such as Voice VLAN Decoy or Staged Relay.
3. **Capture**
   - Presents a clear decision timeline showing LLDP rewrites, EAPOL suppression, PoE events, and alerts.
4. **Safety**
   - Reinforces authorization boundaries and deployment safety expectations.

### Intended Interface Model

In a fully realized hardware build, the app would communicate with the ESP32-C6 over a mutually authenticated BLE or Wi-Fi session and expose:

- Arming / disarming of policy profiles.
- Relay and bypass status.
- Live power telemetry.
- LLDP neighbor snapshots.
- Export of capture events.
- Engagement notes and evidence packaging.

The app is deliberately lightweight and understandable so it can be extended into a richer field operations interface later.

---

## 9. Use Cases

### 9.1 Red Teams

- Validate whether VoIP handsets trust alternate voice VLAN hints.
- Test whether physical security devices can be manipulated into maintenance or diagnostic flows.
- Explore whether conference room or branch appliances behave differently when edge identity is spoofed.
- Simulate a malicious inline maintenance adapter during a physical intrusion exercise.

### 9.2 Security Researchers

- Study LLDP-MED trust assumptions in embedded endpoints.
- Compare vendor behavior under reduced PoE headroom.
- Evaluate logging and telemetry gaps around pre-provisioning manipulation.
- Build reproducible experiments for disclosure or product hardening guidance.

### 9.3 Defenders / Blue Teams

- Check whether switch logs, NAC dashboards, or SIEM alerts expose the manipulations.
- Assess whether access policies depend on assumptions rather than cryptographic binding.
- Exercise incident response for rogue inline adapters.
- Validate segmentation around voice, signage, and camera environments.

### 9.4 Product Security / Vendors

- Adversarially validate endpoint behavior before release.
- Confirm that devices do not over-trust LLDP role, power, or VLAN metadata.
- Reproduce field conditions that normal protocol analyzers do not simulate well.

---

## 10. Detailed Operational Concept

A typical authorized lab assessment might look like this:

1. The researcher inserts NAC Mirage between an access switch and an enterprise IP phone.
2. In **transparent** mode, the device records baseline LLDP-MED, EAPOL timing, DHCP classing, and PoE draw behavior.
3. The researcher switches to **voice-vlan-decoy** to present an alternate voice VLAN and provisioning context.
4. NAC Mirage observes whether the endpoint requests firmware, configuration, certificates, or SIP registration differently.
5. If testing resilience, the operator transitions to **camera-brownout** or **staged-relay** to see whether constrained power or delayed authorization changes behavior.
6. The capture log and app telemetry are exported into the engagement notes.

The point is not random disruption. The point is **controlled environmental deception** with enough evidence to support remediation.

---

## 11. Repository Layout

```text
nac-mirage/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   └── drivers/
│       ├── bridge.c
│       ├── bridge.h
│       ├── capture.c
│       ├── capture.h
│       ├── lldp.c
│       ├── lldp.h
│       ├── poe.c
│       ├── poe.h
│       ├── radio.c
│       └── radio.h
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

## 12. Design Tradeoffs

### FPGA + MCU instead of MCU-only

A pure MCU implementation could forward and inspect lower bandwidth traffic, but introducing an FPGA gives tighter control over relay timing, lower-latency mirroring, and a clearer path to deterministic packet treatment while the MCU handles richer policy decisions.

### ESP32-C6 as a separate management island

Separating the operator radio from the forwarding plane reduces the chance that companion-app activity perturbs packet timing. It also simplifies secure pairing and future remote export workflows.

### Fail-open bias

Because this device is intended for authorized research around operational systems, a safe bypass path is critical. The design therefore assumes continuity first and policy intervention second.

### Capture-first philosophy

Every interesting action should be recorded with a human-readable decision trace. That makes discoveries defensible and reproducible rather than anecdotal.

---

## 13. Legal and Ethical Boundaries

NAC Mirage is a research and validation platform. It should only be used where the operator has explicit authorization covering:

- Physical insertion of inline hardware.
- Manipulation of network edge metadata.
- Testing of PoE resilience and link-layer provisioning behavior.
- Capture and analysis of endpoint responses.

It must **not** be used against public infrastructure, emergency systems, third-party facilities, or production environments without approval. Many of the behaviors it studies can disrupt normal operations if misused. Responsible handling, change control, and stakeholder coordination are mandatory.

---

## 14. Future Extensions

Potential next-generation features include:

- Time-synchronized packet export for correlation with switch logs.
- Optional SFP-based fiber variant for secure building and campus edge research.
- More complete 802.1X relay experimentation modes for lab-only protocol studies.
- Signed policy bundles and chain-of-custody capture export.
- Environmental sensing for enclosure-open and movement detection.
- A richer provisioning emulation service for phone and signage ecosystems.

---

## 15. Closing Summary

NAC Mirage is an original inline security research device by **jayis1** that targets a gap between conventional network testing and real-world edge trust. It addresses the fact that many endpoints make meaningful decisions before defenders normally begin observing them. By combining PoE shaping, LLDP-MED mutation, NAC timing control, structured capture, and a clear operator interface, the platform enables realistic and responsible assessment of edge infrastructure trust.

If your goal is to understand whether devices trust the network edge too early, too broadly, or too implicitly, NAC Mirage is designed to answer that question in a practical, evidence-driven way.
