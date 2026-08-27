# Author: jayis1
from pathlib import Path
from textwrap import dedent

root = Path('/root/hacker-devices/poe-whisper')
(root / 'firmware' / 'drivers').mkdir(parents=True, exist_ok=True)
(root / 'kicad').mkdir(parents=True, exist_ok=True)
(root / 'app' / 'screens').mkdir(parents=True, exist_ok=True)
(root / 'app' / 'components').mkdir(parents=True, exist_ok=True)
(root / 'app' / 'utils').mkdir(parents=True, exist_ok=True)

files = {}

files['README.md'] = dedent('''
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
''').strip() + '\n'

files['firmware/board.h'] = dedent('''
/*
 * board.h - PoE Whisper board and shared definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef POE_WHISPER_BOARD_H
#define POE_WHISPER_BOARD_H

#include <stdint.h>
#include <stddef.h>

#define PW_AUTHOR "jayis1"
#define PW_DEVICE_NAME "PoE Whisper"
#define PW_MAX_EVENTS 128
#define PW_MAX_PROFILE_NAME 32
#define PW_MAX_LLDP_PAYLOAD 128
#define PW_MAX_CURRENT_SAMPLES 64
#define PW_MAX_RADIO_FRAME 196
#define PW_SAFE_TEMP_C 78.0f
#define PW_SAFE_CURRENT_MA 1350.0f
#define PW_SAFE_BROWNOUT_MS 180u
#define PW_DEFAULT_BUDGET_W 30.0f
#define PW_BT_MAX_BUDGET_W 71.0f

typedef enum {
    PW_MODE_PASSIVE = 0,
    PW_MODE_PROFILED = 1,
    PW_MODE_ACTIVE = 2,
    PW_MODE_SAFE_ROLLBACK = 3
} pw_mode_t;

typedef enum {
    PW_CLASS_0 = 0,
    PW_CLASS_1 = 1,
    PW_CLASS_2 = 2,
    PW_CLASS_3 = 3,
    PW_CLASS_4 = 4,
    PW_CLASS_5 = 5,
    PW_CLASS_6 = 6,
    PW_CLASS_7 = 7,
    PW_CLASS_8 = 8
} pw_poe_class_t;

typedef enum {
    EVENT_BOOT = 1,
    EVENT_PSE_FINGERPRINT,
    EVENT_PD_CLASS_PRESENTED,
    EVENT_LLDP_CAPTURED,
    EVENT_LLDP_SPOOFED,
    EVENT_BROWNOUT_EXECUTED,
    EVENT_MPS_JITTER,
    EVENT_CURRENT_PATTERN,
    EVENT_TEMP_ALERT,
    EVENT_ROLLBACK,
    EVENT_OPERATOR_COMMAND
} pw_event_code_t;

typedef struct {
    uint32_t timestamp_ms;
    pw_event_code_t code;
    char message[112];
} pw_event_t;

typedef struct {
    char name[PW_MAX_PROFILE_NAME];
    pw_poe_class_t advertised_class;
    float requested_power_w;
    uint32_t brownout_ms;
    float brownout_target_v;
    uint8_t lldp_spoof;
    uint8_t mps_jitter;
    uint8_t passive_only;
} pw_profile_t;

typedef struct {
    float line_voltage_v;
    float line_current_ma;
    float board_temp_c;
    float allocated_power_w;
    pw_poe_class_t detected_class;
    pw_mode_t mode;
    uint8_t link_up;
    uint8_t bypass_enabled;
    uint8_t thermal_shutdown;
} pw_status_t;

#endif
''').strip() + '\n'

files['firmware/registers.h'] = dedent('''
/*
 * registers.h - simulation register shims for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_REGISTERS_H
#define POE_WHISPER_REGISTERS_H

#include <stdint.h>

typedef struct {
    uint32_t ctrl;
    uint32_t status;
    uint32_t brownout_ticks;
    uint32_t relay_mask;
    uint32_t temp_raw;
    uint32_t current_raw;
    uint32_t voltage_raw;
} pw_fpga_regs_t;

typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t crc_errors;
    uint32_t last_opcode;
} pw_radio_regs_t;

extern pw_fpga_regs_t PW_FPGA;
extern pw_radio_regs_t PW_RADIO;

#define PW_CTRL_ENABLE_ACTIVE   (1u << 0)
#define PW_CTRL_FORCE_BYPASS    (1u << 1)
#define PW_CTRL_BROWNOUT_ARMED  (1u << 2)
#define PW_CTRL_LLDP_SPOOF      (1u << 3)
#define PW_CTRL_MPS_JITTER      (1u << 4)

#define PW_STATUS_PSE_PRESENT   (1u << 0)
#define PW_STATUS_PD_PRESENT    (1u << 1)
#define PW_STATUS_LINK_UP       (1u << 2)
#define PW_STATUS_THERMAL_FAULT (1u << 3)
#define PW_STATUS_OVERCURRENT   (1u << 4)

#endif
''').strip() + '\n'

files['firmware/drivers/poe_port.h'] = dedent('''
/*
 * poe_port.h - PoE port modeling and policy hooks
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_POE_PORT_H
#define POE_WHISPER_POE_PORT_H

#include "../board.h"

typedef struct {
    char vendor[24];
    uint32_t detection_us;
    uint8_t bt_capable;
    float budget_w;
    float startup_voltage_v;
} pw_pse_fingerprint_t;

typedef struct {
    pw_poe_class_t presented_class;
    float requested_power_w;
    float delivered_voltage_v;
    float delivered_current_ma;
    uint8_t mps_valid;
    uint8_t negotiation_complete;
} pw_port_state_t;

void pw_port_init(pw_port_state_t *state);
void pw_port_apply_profile(pw_port_state_t *state, const pw_profile_t *profile);
pw_pse_fingerprint_t pw_port_fingerprint_pse(const char *vendor_hint);
void pw_port_negotiate(pw_port_state_t *state, const pw_pse_fingerprint_t *pse, pw_event_t *events, size_t *event_count, size_t max_events);
void pw_port_sample(pw_port_state_t *state, pw_status_t *status, uint32_t tick_ms);
float pw_port_estimate_budget(const pw_port_state_t *state, const pw_pse_fingerprint_t *pse);
const char *pw_port_class_name(pw_poe_class_t poe_class);

#endif
''').strip() + '\n'

files['firmware/drivers/lldp.h'] = dedent('''
/*
 * lldp.h - LLDP / LLDP-MED helpers for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_LLDP_H
#define POE_WHISPER_LLDP_H

#include "../board.h"

typedef struct {
    char chassis_id[32];
    char port_id[32];
    char system_name[48];
    char platform[48];
    float requested_power_w;
    uint8_t priority;
} pw_lldp_profile_t;

size_t pw_lldp_build_advertisement(const pw_lldp_profile_t *profile, uint8_t *out, size_t out_len);
int pw_lldp_parse_summary(const uint8_t *buf, size_t len, pw_lldp_profile_t *out);
pw_lldp_profile_t pw_lldp_default_profile(const char *system_name, float requested_power_w);
void pw_lldp_format_summary(const pw_lldp_profile_t *profile, char *out, size_t out_len);

#endif
''').strip() + '\n'

files['firmware/drivers/signature.h'] = dedent('''
/*
 * signature.h - Current signature analysis for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_SIGNATURE_H
#define POE_WHISPER_SIGNATURE_H

#include "../board.h"

typedef struct {
    float samples_ma[PW_MAX_CURRENT_SAMPLES];
    size_t count;
    float mean_ma;
    float peak_ma;
    float variance;
    char inferred_state[40];
} pw_signature_result_t;

void pw_signature_reset(pw_signature_result_t *result);
void pw_signature_feed(pw_signature_result_t *result, float sample_ma);
void pw_signature_finalize(pw_signature_result_t *result);
float pw_signature_anomaly_score(const pw_signature_result_t *result, float expected_mean_ma);

#endif
''').strip() + '\n'

files['firmware/drivers/relay.h'] = dedent('''
/*
 * relay.h - Relay and brownout control for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_RELAY_H
#define POE_WHISPER_RELAY_H

#include "../board.h"

typedef struct {
    uint8_t bypass_enabled;
    uint8_t brownout_active;
    uint32_t brownout_remaining_ms;
    float target_voltage_v;
} pw_relay_state_t;

void pw_relay_init(pw_relay_state_t *state);
int pw_relay_schedule_brownout(pw_relay_state_t *state, uint32_t duration_ms, float target_voltage_v, pw_event_t *event);
void pw_relay_tick(pw_relay_state_t *state, pw_status_t *status, uint32_t step_ms);
void pw_relay_force_bypass(pw_relay_state_t *state, pw_status_t *status, const char *reason, pw_event_t *event);

#endif
''').strip() + '\n'

files['firmware/drivers/radio.h'] = dedent('''
/*
 * radio.h - Operator link framing for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_RADIO_H
#define POE_WHISPER_RADIO_H

#include "../board.h"

typedef enum {
    PW_OP_HELLO = 1,
    PW_OP_STATUS,
    PW_OP_LOAD_PROFILE,
    PW_OP_ARM_BROWNOUT,
    PW_OP_SET_MODE,
    PW_OP_ACK,
    PW_OP_NACK
} pw_radio_opcode_t;

typedef struct {
    pw_radio_opcode_t opcode;
    uint8_t payload[PW_MAX_RADIO_FRAME];
    size_t length;
} pw_radio_frame_t;

void pw_radio_init(void);
size_t pw_radio_encode(pw_radio_opcode_t opcode, const uint8_t *payload, size_t length, uint8_t *out, size_t out_len);
int pw_radio_decode(const uint8_t *buf, size_t len, pw_radio_frame_t *out);
uint8_t pw_radio_crc8(const uint8_t *buf, size_t len);

#endif
''').strip() + '\n'

files['firmware/drivers/poe_port.c'] = dedent('''
/*
 * poe_port.c - PoE port modeling and policy hooks
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include "poe_port.h"

static float class_to_nominal_power(pw_poe_class_t poe_class)
{
    switch (poe_class) {
        case PW_CLASS_1: return 4.0f;
        case PW_CLASS_2: return 7.0f;
        case PW_CLASS_3: return 15.4f;
        case PW_CLASS_4: return 30.0f;
        case PW_CLASS_5: return 45.0f;
        case PW_CLASS_6: return 60.0f;
        case PW_CLASS_7: return 75.0f;
        case PW_CLASS_8: return 90.0f;
        case PW_CLASS_0:
        default: return 13.0f;
    }
}

const char *pw_port_class_name(pw_poe_class_t poe_class)
{
    switch (poe_class) {
        case PW_CLASS_0: return "class-0";
        case PW_CLASS_1: return "class-1";
        case PW_CLASS_2: return "class-2";
        case PW_CLASS_3: return "class-3";
        case PW_CLASS_4: return "class-4";
        case PW_CLASS_5: return "class-5";
        case PW_CLASS_6: return "class-6";
        case PW_CLASS_7: return "class-7";
        case PW_CLASS_8: return "class-8";
        default: return "class-unknown";
    }
}

void pw_port_init(pw_port_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->presented_class = PW_CLASS_4;
    state->requested_power_w = PW_DEFAULT_BUDGET_W;
    state->delivered_voltage_v = 52.0f;
    state->delivered_current_ma = 210.0f;
    state->mps_valid = 1;
}

void pw_port_apply_profile(pw_port_state_t *state, const pw_profile_t *profile)
{
    if (!state || !profile) {
        return;
    }
    state->presented_class = profile->advertised_class;
    state->requested_power_w = profile->requested_power_w;
}

pw_pse_fingerprint_t pw_port_fingerprint_pse(const char *vendor_hint)
{
    pw_pse_fingerprint_t fp;
    memset(&fp, 0, sizeof(fp));
    if (vendor_hint && strstr(vendor_hint, "Cisco")) {
        snprintf(fp.vendor, sizeof(fp.vendor), "Cisco Catalyst");
        fp.detection_us = 1820;
        fp.bt_capable = 1;
        fp.budget_w = 60.0f;
        fp.startup_voltage_v = 54.1f;
    } else if (vendor_hint && strstr(vendor_hint, "Aruba")) {
        snprintf(fp.vendor, sizeof(fp.vendor), "Aruba Access");
        fp.detection_us = 2050;
        fp.bt_capable = 1;
        fp.budget_w = 45.0f;
        fp.startup_voltage_v = 53.4f;
    } else {
        snprintf(fp.vendor, sizeof(fp.vendor), "Generic PSE");
        fp.detection_us = 2400;
        fp.bt_capable = 0;
        fp.budget_w = 30.0f;
        fp.startup_voltage_v = 51.2f;
    }
    return fp;
}

float pw_port_estimate_budget(const pw_port_state_t *state, const pw_pse_fingerprint_t *pse)
{
    float class_power = class_to_nominal_power(state->presented_class);
    float headroom = pse->budget_w - class_power;
    if (headroom < 0.0f) {
        headroom = 0.0f;
    }
    if (state->requested_power_w < class_power) {
        return class_power;
    }
    if (state->requested_power_w > pse->budget_w) {
        return pse->budget_w;
    }
    return state->requested_power_w + (headroom * 0.15f);
}

static void push_event(pw_event_t *events, size_t *event_count, size_t max_events, pw_event_code_t code, const char *msg, uint32_t ts)
{
    if (*event_count >= max_events) {
        return;
    }
    events[*event_count].timestamp_ms = ts;
    events[*event_count].code = code;
    snprintf(events[*event_count].message, sizeof(events[*event_count].message), "%s", msg);
    (*event_count)++;
}

void pw_port_negotiate(pw_port_state_t *state, const pw_pse_fingerprint_t *pse, pw_event_t *events, size_t *event_count, size_t max_events)
{
    char line[112];
    snprintf(line, sizeof(line), "PSE fingerprint vendor=%s detect=%uus budget=%.1fW", pse->vendor, pse->detection_us, pse->budget_w);
    push_event(events, event_count, max_events, EVENT_PSE_FINGERPRINT, line, 6);

    snprintf(line, sizeof(line), "PD presents %s request=%.1fW", pw_port_class_name(state->presented_class), state->requested_power_w);
    push_event(events, event_count, max_events, EVENT_PD_CLASS_PRESENTED, line, 11);

    state->negotiation_complete = 1;
    state->delivered_voltage_v = pse->startup_voltage_v;
    state->delivered_current_ma = (state->requested_power_w * 1000.0f) / state->delivered_voltage_v;
    if (state->delivered_current_ma < 120.0f) {
        state->delivered_current_ma = 120.0f;
    }

    snprintf(line, sizeof(line), "Allocated %.1fW at %.1fV / %.0fmA", pw_port_estimate_budget(state, pse), state->delivered_voltage_v, state->delivered_current_ma);
    push_event(events, event_count, max_events, EVENT_PD_CLASS_PRESENTED, line, 18);

    if (state->requested_power_w > pse->budget_w) {
        snprintf(line, sizeof(line), "Requested power exceeds budget; PSE likely clamps to %.1fW", pse->budget_w);
        push_event(events, event_count, max_events, EVENT_OPERATOR_COMMAND, line, 22);
    }
}

void pw_port_sample(pw_port_state_t *state, pw_status_t *status, uint32_t tick_ms)
{
    float modulation = (float)((tick_ms / 10u) % 9u) * 4.5f;
    status->line_voltage_v = state->delivered_voltage_v;
    status->line_current_ma = state->delivered_current_ma + modulation;
    status->allocated_power_w = (status->line_voltage_v * status->line_current_ma) / 1000.0f;
    status->detected_class = state->presented_class;
    status->link_up = 1;
}
''').strip() + '\n'

files['firmware/drivers/lldp.c'] = dedent('''
/*
 * lldp.c - LLDP / LLDP-MED helpers for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include "lldp.h"

static size_t write_tlv(uint8_t type, const uint8_t *payload, size_t payload_len, uint8_t *out, size_t offset, size_t out_len)
{
    if (offset + payload_len + 2 > out_len || payload_len > 511u) {
        return offset;
    }
    uint16_t header = (uint16_t)(((uint16_t)type << 9) | (payload_len & 0x1FFu));
    out[offset++] = (uint8_t)(header >> 8);
    out[offset++] = (uint8_t)(header & 0xFFu);
    memcpy(&out[offset], payload, payload_len);
    return offset + payload_len;
}

pw_lldp_profile_t pw_lldp_default_profile(const char *system_name, float requested_power_w)
{
    pw_lldp_profile_t p;
    memset(&p, 0, sizeof(p));
    snprintf(p.chassis_id, sizeof(p.chassis_id), "pw-inline-01");
    snprintf(p.port_id, sizeof(p.port_id), "eth-inline-a");
    snprintf(p.system_name, sizeof(p.system_name), "%s", system_name ? system_name : "PoE Whisper");
    snprintf(p.platform, sizeof(p.platform), "inline-poe-research");
    p.requested_power_w = requested_power_w;
    p.priority = 2;
    return p;
}

size_t pw_lldp_build_advertisement(const pw_lldp_profile_t *profile, uint8_t *out, size_t out_len)
{
    size_t off = 0;
    off = write_tlv(1, (const uint8_t *)profile->chassis_id, strlen(profile->chassis_id), out, off, out_len);
    off = write_tlv(2, (const uint8_t *)profile->port_id, strlen(profile->port_id), out, off, out_len);
    off = write_tlv(5, (const uint8_t *)profile->system_name, strlen(profile->system_name), out, off, out_len);
    off = write_tlv(6, (const uint8_t *)profile->platform, strlen(profile->platform), out, off, out_len);

    char power[48];
    snprintf(power, sizeof(power), "power=%.1fW priority=%u", profile->requested_power_w, profile->priority);
    off = write_tlv(127, (const uint8_t *)power, strlen(power), out, off, out_len);
    out[off++] = 0;
    out[off++] = 0;
    return off;
}

int pw_lldp_parse_summary(const uint8_t *buf, size_t len, pw_lldp_profile_t *out)
{
    size_t off = 0;
    memset(out, 0, sizeof(*out));
    while (off + 2 <= len) {
        uint16_t header = (uint16_t)((buf[off] << 8) | buf[off + 1]);
        uint8_t type = (uint8_t)(header >> 9);
        uint16_t tlv_len = header & 0x1FFu;
        off += 2;
        if (off + tlv_len > len) {
            return -1;
        }
        if (type == 0) {
            break;
        } else if (type == 1) {
            snprintf(out->chassis_id, sizeof(out->chassis_id), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 2) {
            snprintf(out->port_id, sizeof(out->port_id), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 5) {
            snprintf(out->system_name, sizeof(out->system_name), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 6) {
            snprintf(out->platform, sizeof(out->platform), "%.*s", (int)tlv_len, (const char *)&buf[off]);
        } else if (type == 127) {
            sscanf((const char *)&buf[off], "power=%fW priority=%hhu", &out->requested_power_w, &out->priority);
        }
        off += tlv_len;
    }
    return 0;
}

void pw_lldp_format_summary(const pw_lldp_profile_t *profile, char *out, size_t out_len)
{
    snprintf(out, out_len, "LLDP chassis=%s port=%s system=%s power=%.1fW prio=%u",
             profile->chassis_id,
             profile->port_id,
             profile->system_name,
             profile->requested_power_w,
             profile->priority);
}
''').strip() + '\n'

files['firmware/drivers/signature.c'] = dedent('''
/*
 * signature.c - Current signature analysis for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "signature.h"

void pw_signature_reset(pw_signature_result_t *result)
{
    memset(result, 0, sizeof(*result));
    snprintf(result->inferred_state, sizeof(result->inferred_state), "unknown");
}

void pw_signature_feed(pw_signature_result_t *result, float sample_ma)
{
    if (result->count < PW_MAX_CURRENT_SAMPLES) {
        result->samples_ma[result->count++] = sample_ma;
    }
}

void pw_signature_finalize(pw_signature_result_t *result)
{
    float sum = 0.0f;
    result->peak_ma = 0.0f;
    for (size_t i = 0; i < result->count; ++i) {
        sum += result->samples_ma[i];
        if (result->samples_ma[i] > result->peak_ma) {
            result->peak_ma = result->samples_ma[i];
        }
    }
    result->mean_ma = result->count ? sum / (float)result->count : 0.0f;

    float variance_sum = 0.0f;
    for (size_t i = 0; i < result->count; ++i) {
        float delta = result->samples_ma[i] - result->mean_ma;
        variance_sum += delta * delta;
    }
    result->variance = result->count ? variance_sum / (float)result->count : 0.0f;

    if (result->peak_ma > 900.0f && result->variance > 20000.0f) {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "motor-or-ir-burst");
    } else if (result->mean_ma > 520.0f && result->variance > 6000.0f) {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "radio-boot");
    } else if (result->mean_ma > 250.0f) {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "steady-online");
    } else {
        snprintf(result->inferred_state, sizeof(result->inferred_state), "idle-or-low-power");
    }
}

float pw_signature_anomaly_score(const pw_signature_result_t *result, float expected_mean_ma)
{
    float mean_delta = fabsf(result->mean_ma - expected_mean_ma);
    float variance_term = sqrtf(result->variance) * 0.05f;
    float peak_term = result->peak_ma > expected_mean_ma ? (result->peak_ma - expected_mean_ma) * 0.02f : 0.0f;
    return mean_delta * 0.08f + variance_term + peak_term;
}
''').strip() + '\n'

files['firmware/drivers/relay.c'] = dedent('''
/*
 * relay.c - Relay and brownout control for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include "relay.h"

void pw_relay_init(pw_relay_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->target_voltage_v = 52.0f;
}

int pw_relay_schedule_brownout(pw_relay_state_t *state, uint32_t duration_ms, float target_voltage_v, pw_event_t *event)
{
    if (duration_ms == 0 || duration_ms > PW_SAFE_BROWNOUT_MS || target_voltage_v < 32.0f) {
        return -1;
    }
    state->brownout_active = 1;
    state->brownout_remaining_ms = duration_ms;
    state->target_voltage_v = target_voltage_v;
    if (event) {
        event->timestamp_ms = 0;
        event->code = EVENT_BROWNOUT_EXECUTED;
        snprintf(event->message, sizeof(event->message), "Brownout armed for %ums target=%.1fV", duration_ms, target_voltage_v);
    }
    return 0;
}

void pw_relay_tick(pw_relay_state_t *state, pw_status_t *status, uint32_t step_ms)
{
    status->bypass_enabled = state->bypass_enabled;
    if (!state->brownout_active) {
        return;
    }
    if (state->brownout_remaining_ms <= step_ms) {
        state->brownout_remaining_ms = 0;
        state->brownout_active = 0;
        status->line_voltage_v = 52.0f;
    } else {
        state->brownout_remaining_ms -= step_ms;
        status->line_voltage_v = state->target_voltage_v;
    }
}

void pw_relay_force_bypass(pw_relay_state_t *state, pw_status_t *status, const char *reason, pw_event_t *event)
{
    state->bypass_enabled = 1;
    state->brownout_active = 0;
    state->brownout_remaining_ms = 0;
    status->bypass_enabled = 1;
    if (event) {
        event->timestamp_ms = 0;
        event->code = EVENT_ROLLBACK;
        snprintf(event->message, sizeof(event->message), "Bypass forced: %s", reason ? reason : "unspecified");
    }
}
''').strip() + '\n'

files['firmware/drivers/radio.c'] = dedent('''
/*
 * radio.c - Operator link framing for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <string.h>
#include "radio.h"
#include "../registers.h"

void pw_radio_init(void)
{
    memset(&PW_RADIO, 0, sizeof(PW_RADIO));
}

uint8_t pw_radio_crc8(const uint8_t *buf, size_t len)
{
    uint8_t crc = 0x42u;
    for (size_t i = 0; i < len; ++i) {
        crc ^= buf[i];
        for (uint8_t b = 0; b < 8; ++b) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

size_t pw_radio_encode(pw_radio_opcode_t opcode, const uint8_t *payload, size_t length, uint8_t *out, size_t out_len)
{
    if (length + 4 > out_len || length > PW_MAX_RADIO_FRAME) {
        return 0;
    }
    out[0] = 0xA5u;
    out[1] = (uint8_t)opcode;
    out[2] = (uint8_t)length;
    if (length && payload) {
        memcpy(&out[3], payload, length);
    }
    out[3 + length] = pw_radio_crc8(&out[1], length + 2);
    PW_RADIO.tx_count++;
    PW_RADIO.last_opcode = opcode;
    return length + 4;
}

int pw_radio_decode(const uint8_t *buf, size_t len, pw_radio_frame_t *out)
{
    if (!buf || len < 4 || buf[0] != 0xA5u) {
        return -1;
    }
    uint8_t payload_len = buf[2];
    if ((size_t)payload_len + 4 != len) {
        PW_RADIO.crc_errors++;
        return -2;
    }
    if (pw_radio_crc8(&buf[1], payload_len + 2) != buf[len - 1]) {
        PW_RADIO.crc_errors++;
        return -3;
    }
    out->opcode = (pw_radio_opcode_t)buf[1];
    out->length = payload_len;
    memcpy(out->payload, &buf[3], payload_len);
    PW_RADIO.rx_count++;
    PW_RADIO.last_opcode = out->opcode;
    return 0;
}
''').strip() + '\n'

files['firmware/main.c'] = dedent('''
/*
 * main.c - PoE Whisper simulation firmware
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "registers.h"
#include "drivers/poe_port.h"
#include "drivers/lldp.h"
#include "drivers/signature.h"
#include "drivers/relay.h"
#include "drivers/radio.h"

pw_fpga_regs_t PW_FPGA;
pw_radio_regs_t PW_RADIO;

static void add_event(pw_event_t *events, size_t *count, pw_event_code_t code, uint32_t ts, const char *message)
{
    if (*count >= PW_MAX_EVENTS) {
        return;
    }
    events[*count].timestamp_ms = ts;
    events[*count].code = code;
    snprintf(events[*count].message, sizeof(events[*count].message), "%s", message);
    (*count)++;
}

static pw_profile_t build_profile(const char *name)
{
    pw_profile_t p;
    memset(&p, 0, sizeof(p));
    snprintf(p.name, sizeof(p.name), "%s", name);

    if (strcmp(name, "camera-reboot-window") == 0) {
        p.advertised_class = PW_CLASS_4;
        p.requested_power_w = 24.5f;
        p.brownout_ms = 120u;
        p.brownout_target_v = 36.5f;
        p.lldp_spoof = 1;
        p.mps_jitter = 0;
    } else if (strcmp(name, "phone-class-downgrade") == 0) {
        p.advertised_class = PW_CLASS_2;
        p.requested_power_w = 6.5f;
        p.brownout_ms = 0u;
        p.brownout_target_v = 0.0f;
        p.lldp_spoof = 1;
        p.mps_jitter = 1;
    } else if (strcmp(name, "badge-reader-mps-jitter") == 0) {
        p.advertised_class = PW_CLASS_3;
        p.requested_power_w = 12.5f;
        p.brownout_ms = 80u;
        p.brownout_target_v = 40.0f;
        p.lldp_spoof = 0;
        p.mps_jitter = 1;
    } else {
        p.advertised_class = PW_CLASS_4;
        p.requested_power_w = 20.0f;
        p.brownout_ms = 0u;
        p.brownout_target_v = 0.0f;
        p.passive_only = 1;
    }
    return p;
}

static void print_banner(void)
{
    printf("%s simulation firmware by %s\n", PW_DEVICE_NAME, PW_AUTHOR);
    printf("Authorized security research use only.\n\n");
}

static void simulate_current_trace(pw_signature_result_t *sig, const pw_profile_t *profile)
{
    pw_signature_reset(sig);
    float base = profile->requested_power_w > 0.1f ? (profile->requested_power_w * 1000.0f / 52.0f) : 180.0f;
    for (size_t i = 0; i < PW_MAX_CURRENT_SAMPLES; ++i) {
        float sample = base;
        if (i < 12) {
            sample += 150.0f + (float)i * 8.0f;
        } else if (i > 28 && i < 36) {
            sample += 420.0f;
        } else if (profile->mps_jitter && (i % 9u == 0u)) {
            sample -= 90.0f;
        } else {
            sample += (float)((int)(i % 5u) - 2) * 14.0f;
        }
        pw_signature_feed(sig, sample);
    }
    pw_signature_finalize(sig);
}

static void emit_lldp_sequence(const pw_profile_t *profile, pw_event_t *events, size_t *event_count)
{
    pw_lldp_profile_t tx = pw_lldp_default_profile("PoE Whisper Inline", profile->requested_power_w);
    uint8_t frame[PW_MAX_LLDP_PAYLOAD];
    char summary[120];
    size_t frame_len = pw_lldp_build_advertisement(&tx, frame, sizeof(frame));
    pw_lldp_profile_t parsed;
    if (pw_lldp_parse_summary(frame, frame_len, &parsed) == 0) {
        pw_lldp_format_summary(&parsed, summary, sizeof(summary));
        add_event(events, event_count, EVENT_LLDP_CAPTURED, 30u, summary);
    }

    if (profile->lldp_spoof) {
        parsed.requested_power_w += 5.0f;
        snprintf(parsed.system_name, sizeof(parsed.system_name), "camera-maint-inline");
        pw_lldp_format_summary(&parsed, summary, sizeof(summary));
        add_event(events, event_count, EVENT_LLDP_SPOOFED, 42u, summary);
    }
}

static void maybe_trigger_brownout(const pw_profile_t *profile, pw_relay_state_t *relay, pw_status_t *status, pw_event_t *events, size_t *event_count)
{
    if (!profile->brownout_ms) {
        return;
    }
    pw_event_t evt;
    if (pw_relay_schedule_brownout(relay, profile->brownout_ms, profile->brownout_target_v, &evt) == 0) {
        evt.timestamp_ms = 57u;
        if (*event_count < PW_MAX_EVENTS) {
            events[(*event_count)++] = evt;
        }
        for (uint32_t t = 0; t < profile->brownout_ms; t += 20u) {
            pw_relay_tick(relay, status, 20u);
        }
    }
}

static void enforce_safety(pw_status_t *status, pw_relay_state_t *relay, pw_event_t *events, size_t *event_count)
{
    if (status->board_temp_c > PW_SAFE_TEMP_C) {
        pw_event_t evt;
        pw_relay_force_bypass(relay, status, "thermal ceiling exceeded", &evt);
        evt.timestamp_ms = 88u;
        if (*event_count < PW_MAX_EVENTS) {
            events[(*event_count)++] = evt;
        }
        status->mode = PW_MODE_SAFE_ROLLBACK;
        status->thermal_shutdown = 1;
    }
    if (status->line_current_ma > PW_SAFE_CURRENT_MA) {
        pw_event_t evt;
        pw_relay_force_bypass(relay, status, "overcurrent ceiling exceeded", &evt);
        evt.timestamp_ms = 89u;
        if (*event_count < PW_MAX_EVENTS) {
            events[(*event_count)++] = evt;
        }
        status->mode = PW_MODE_SAFE_ROLLBACK;
    }
}

static void simulate_operator_protocol(const char *profile_name)
{
    uint8_t payload[64];
    uint8_t encoded[96];
    pw_radio_frame_t decoded;
    size_t profile_len = strlen(profile_name);
    memcpy(payload, profile_name, profile_len);
    size_t packet_len = pw_radio_encode(PW_OP_LOAD_PROFILE, payload, profile_len, encoded, sizeof(encoded));
    if (packet_len > 0 && pw_radio_decode(encoded, packet_len, &decoded) == 0) {
        printf("radio: opcode=%u payload='%.*s' crc_errors=%u\n",
               decoded.opcode,
               (int)decoded.length,
               decoded.payload,
               PW_RADIO.crc_errors);
    }
}

static void print_status(const pw_status_t *status, const pw_profile_t *profile, const pw_signature_result_t *sig)
{
    printf("status: mode=%d class=%s link=%u bypass=%u voltage=%.1fV current=%.0fmA power=%.1fW temp=%.1fC\n",
           status->mode,
           pw_port_class_name(status->detected_class),
           status->link_up,
           status->bypass_enabled,
           status->line_voltage_v,
           status->line_current_ma,
           status->allocated_power_w,
           status->board_temp_c);
    printf("profile: %s request=%.1fW brownout=%ums target=%.1fV lldp_spoof=%u mps_jitter=%u\n",
           profile->name,
           profile->requested_power_w,
           profile->brownout_ms,
           profile->brownout_target_v,
           profile->lldp_spoof,
           profile->mps_jitter);
    printf("signature: mean=%.1fmA peak=%.1fmA variance=%.1f inferred=%s anomaly_score=%.2f\n",
           sig->mean_ma,
           sig->peak_ma,
           sig->variance,
           sig->inferred_state,
           pw_signature_anomaly_score(sig, 320.0f));
}

static void print_events(const pw_event_t *events, size_t count)
{
    puts("events:");
    for (size_t i = 0; i < count; ++i) {
        printf("  [%03ums] code=%d %s\n", events[i].timestamp_ms, events[i].code, events[i].message);
    }
}

int main(int argc, char **argv)
{
    const char *vendor = argc > 1 ? argv[1] : "Cisco";
    const char *profile_name = argc > 2 ? argv[2] : "camera-reboot-window";

    print_banner();
    pw_radio_init();

    pw_status_t status;
    memset(&status, 0, sizeof(status));
    status.mode = PW_MODE_PROFILED;
    status.board_temp_c = 41.5f;

    pw_port_state_t port;
    pw_port_init(&port);

    pw_profile_t profile = build_profile(profile_name);
    pw_port_apply_profile(&port, &profile);

    pw_pse_fingerprint_t pse = pw_port_fingerprint_pse(vendor);

    pw_event_t events[PW_MAX_EVENTS];
    size_t event_count = 0;
    add_event(events, &event_count, EVENT_BOOT, 0u, "PoE Whisper booted in authorized research mode");
    pw_port_negotiate(&port, &pse, events, &event_count, PW_MAX_EVENTS);

    emit_lldp_sequence(&profile, events, &event_count);
    pw_port_sample(&port, &status, 25u);

    pw_signature_result_t sig;
    simulate_current_trace(&sig, &profile);
    char sig_msg[112];
    snprintf(sig_msg, sizeof(sig_msg), "Current signature inferred=%s peak=%.1fmA", sig.inferred_state, sig.peak_ma);
    add_event(events, &event_count, EVENT_CURRENT_PATTERN, 51u, sig_msg);

    pw_relay_state_t relay;
    pw_relay_init(&relay);
    maybe_trigger_brownout(&profile, &relay, &status, events, &event_count);

    if (profile.mps_jitter) {
        add_event(events, &event_count, EVENT_MPS_JITTER, 72u, "Maintain-power signature jitter enabled for research profile");
    }

    status.board_temp_c = profile.brownout_ms ? 46.0f : 39.0f;
    enforce_safety(&status, &relay, events, &event_count);
    status.mode = status.bypass_enabled ? PW_MODE_SAFE_ROLLBACK : PW_MODE_ACTIVE;

    simulate_operator_protocol(profile_name);
    print_status(&status, &profile, &sig);
    print_events(events, event_count);

    return 0;
}
''').strip() + '\n'

files['firmware/Makefile'] = dedent('''
# PoE Whisper firmware Makefile
# Author: jayis1
# SPDX-License-Identifier: GPL-2.0-only

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?= -lm

PROJECT = poe_whisper_sim
BUILD_DIR = build
SRC_DIR = .
DRV_DIR = $(SRC_DIR)/drivers

SRCS = $(SRC_DIR)/main.c \
       $(DRV_DIR)/poe_port.c \
       $(DRV_DIR)/lldp.c \
       $(DRV_DIR)/signature.c \
       $(DRV_DIR)/relay.c \
       $(DRV_DIR)/radio.c

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

all: $(BUILD_DIR)/$(PROJECT)

$(BUILD_DIR)/$(PROJECT): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BUILD_DIR)/$(PROJECT)
	./$(BUILD_DIR)/$(PROJECT)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
''').strip() + '\n'

files['app/package.json'] = dedent('''
{
  "name": "poe-whisper-app",
  "version": "1.0.0",
  "description": "Companion application for PoE Whisper by jayis1",
  "author": "jayis1",
  "license": "GPL-2.0-only",
  "main": "App.js",
  "scripts": {
    "start": "react-native start",
    "lint": "eslint ."
  },
  "dependencies": {
    "react": "18.2.0",
    "react-native": "0.72.6"
  },
  "devDependencies": {
    "eslint": "^8.57.0"
  }
}
''').strip() + '\n'

files['app/components/StatCard.js'] = dedent('''
// StatCard.js - PoE Whisper companion UI component
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatCard({ label, value, accent }) {
  return (
    <View style={[styles.card, accent && { borderColor: accent }]}>
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, accent && { color: accent }]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#0f172a',
    borderRadius: 14,
    padding: 14,
    borderWidth: 1,
    borderColor: '#1e293b',
    minWidth: '47%',
  },
  label: {
    color: '#94a3b8',
    fontSize: 12,
    marginBottom: 6,
    textTransform: 'uppercase',
  },
  value: {
    color: '#f8fafc',
    fontSize: 20,
    fontWeight: '800',
  },
});
''').strip() + '\n'

files['app/components/EventList.js'] = dedent('''
// EventList.js - PoE Whisper event renderer
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function EventList({ events }) {
  return (
    <View style={styles.wrap}>
      {events.map((event) => (
        <View key={`${event.ts}-${event.message}`} style={styles.item}>
          <Text style={styles.ts}>{event.ts}</Text>
          <View style={styles.body}>
            <Text style={styles.code}>{event.code}</Text>
            <Text style={styles.msg}>{event.message}</Text>
          </View>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 10 },
  item: {
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 12,
    borderWidth: 1,
    borderColor: '#1f2937',
    flexDirection: 'row',
    gap: 12,
  },
  ts: { color: '#22d3ee', fontWeight: '700', width: 58 },
  body: { flex: 1, gap: 3 },
  code: { color: '#e2e8f0', fontWeight: '700' },
  msg: { color: '#94a3b8', lineHeight: 18 },
});
''').strip() + '\n'

files['app/components/ToggleCard.js'] = dedent('''
// ToggleCard.js - PoE Whisper control card
// Author: jayis1
import React from 'react';
import { Pressable, Text, StyleSheet } from 'react-native';

export default function ToggleCard({ title, description, active, onPress }) {
  return (
    <Pressable style={[styles.card, active && styles.active]} onPress={onPress}>
      <Text style={styles.title}>{title}</Text>
      <Text style={styles.desc}>{description}</Text>
      <Text style={styles.state}>{active ? 'Enabled' : 'Disabled'}</Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#0f172a',
    borderColor: '#1e293b',
    borderWidth: 1,
    borderRadius: 14,
    padding: 14,
    gap: 8,
  },
  active: {
    borderColor: '#f59e0b',
    backgroundColor: '#3f2a03',
  },
  title: { color: '#f8fafc', fontWeight: '800', fontSize: 16 },
  desc: { color: '#94a3b8', lineHeight: 18 },
  state: { color: '#fde68a', fontWeight: '700' },
});
''').strip() + '\n'

files['app/screens/DashboardScreen.js'] = dedent('''
// DashboardScreen.js - PoE Whisper overview screen
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import StatCard from '../components/StatCard';

export default function DashboardScreen({ status }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Inline Power State</Text>
      <View style={styles.grid}>
        <StatCard label="Allocated Power" value={`${status.power} W`} accent="#22d3ee" />
        <StatCard label="Advertised Class" value={status.poeClass} accent="#f59e0b" />
        <StatCard label="Voltage" value={`${status.voltage} V`} accent="#34d399" />
        <StatCard label="Thermal Margin" value={`${status.tempMargin} C`} accent="#f87171" />
      </View>
      <Text style={styles.note}>{status.summary}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 14 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  note: {
    color: '#94a3b8',
    lineHeight: 20,
    backgroundColor: '#0f172a',
    borderRadius: 14,
    padding: 14,
  },
});
''').strip() + '\n'

files['app/screens/ProfilesScreen.js'] = dedent('''
// ProfilesScreen.js - PoE Whisper profile selector
// Author: jayis1
import React from 'react';
import { View, Text, Pressable, StyleSheet } from 'react-native';

export default function ProfilesScreen({ profiles, selected, onSelect }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Research Profiles</Text>
      {profiles.map((profile) => (
        <Pressable
          key={profile.id}
          style={[styles.card, selected === profile.id && styles.cardActive]}
          onPress={() => onSelect(profile.id)}
        >
          <Text style={styles.name}>{profile.name}</Text>
          <Text style={styles.desc}>{profile.description}</Text>
          <Text style={styles.meta}>{profile.meta}</Text>
        </Pressable>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 12 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  card: {
    backgroundColor: '#111827',
    borderRadius: 14,
    padding: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
    gap: 6,
  },
  cardActive: {
    borderColor: '#22d3ee',
    backgroundColor: '#082f49',
  },
  name: { color: '#f8fafc', fontWeight: '800' },
  desc: { color: '#cbd5e1', lineHeight: 18 },
  meta: { color: '#94a3b8', fontSize: 12 },
});
''').strip() + '\n'

files['app/screens/CaptureScreen.js'] = dedent('''
// CaptureScreen.js - PoE Whisper capture view
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import EventList from '../components/EventList';

export default function CaptureScreen({ events }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Telemetry and Event Capture</Text>
      <Text style={styles.subtitle}>Current signatures, LLDP activity, and bounded power actions.</Text>
      <EventList events={events} />
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 12 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  subtitle: { color: '#94a3b8', lineHeight: 20 },
});
''').strip() + '\n'

files['app/screens/InjectorScreen.js'] = dedent('''
// InjectorScreen.js - PoE Whisper active control screen
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import ToggleCard from '../components/ToggleCard';

export default function InjectorScreen({ toggles, onToggle }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Power Manipulation Controls</Text>
      <ToggleCard
        title="LLDP Identity Override"
        description="Advertise alternate power requests and endpoint identity for authorized validation."
        active={toggles.lldpSpoof}
        onPress={() => onToggle('lldpSpoof')}
      />
      <ToggleCard
        title="Maintain-Power Jitter"
        description="Introduce bounded MPS irregularity to evaluate PSE keepalive behavior."
        active={toggles.mpsJitter}
        onPress={() => onToggle('mpsJitter')}
      />
      <ToggleCard
        title="Brownout Window"
        description="Arm a short voltage sag during the selected boot phase with hardware rollback."
        active={toggles.brownout}
        onPress={() => onToggle('brownout')}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 12 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
});
''').strip() + '\n'

files['app/screens/SafetyScreen.js'] = dedent('''
// SafetyScreen.js - PoE Whisper safety checklist
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SafetyScreen({ checklist }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Authorized-Use Safety Checklist</Text>
      {checklist.map((item) => (
        <View key={item} style={styles.card}>
          <Text style={styles.bullet}>•</Text>
          <Text style={styles.text}>{item}</Text>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 10 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  card: {
    flexDirection: 'row',
    gap: 10,
    padding: 14,
    backgroundColor: '#111827',
    borderRadius: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
  },
  bullet: { color: '#22d3ee', fontWeight: '800', fontSize: 18 },
  text: { color: '#cbd5e1', lineHeight: 20, flex: 1 },
});
''').strip() + '\n'

files['app/utils/protocol.js'] = dedent('''
// protocol.js - PoE Whisper app protocol model
// Author: jayis1
export const AUTHOR = 'jayis1';

export const profiles = [
  {
    id: 'camera-reboot-window',
    name: 'Camera Reboot Window',
    description: 'Class-4 profile with bounded 120 ms brownout during camera codec initialization.',
    meta: '24.5 W request · LLDP spoof enabled · safe rollback armed',
  },
  {
    id: 'phone-class-downgrade',
    name: 'Phone Class Downgrade',
    description: 'Reduce advertised class and add MPS jitter to observe handset behavior under constrained power.',
    meta: '6.5 W request · LLDP spoof enabled · no voltage sag',
  },
  {
    id: 'badge-reader-mps-jitter',
    name: 'Badge Reader MPS Jitter',
    description: 'Apply bounded keepalive irregularity and short droop for physical-security controller testing.',
    meta: '12.5 W request · MPS jitter enabled · 80 ms droop',
  },
];

export const eventFeed = [
  { ts: '000ms', code: 'BOOT', message: 'PoE Whisper booted in authorized research mode.' },
  { ts: '006ms', code: 'PSE', message: 'Fingerprint matched Cisco Catalyst bt-capable source.' },
  { ts: '030ms', code: 'LLDP', message: 'Captured endpoint power request advertisement at 24.5 W.' },
  { ts: '042ms', code: 'SPOOF', message: 'Injected LLDP identity override with +5.0 W request delta.' },
  { ts: '057ms', code: 'DROOP', message: 'Armed bounded 120 ms brownout to 36.5 V during boot window.' },
  { ts: '072ms', code: 'SIG', message: 'Current signature labeled motor-or-ir-burst, possible camera IR engage.' },
];

export const safetyChecklist = [
  'Operate only with written authorization and an agreed rollback window.',
  'Verify the endpoint is non-life-safety critical before active power manipulation.',
  'Confirm thermal cutoff, bypass relay, and watchdog rollback before insertion.',
  'Record baseline behavior in passive mode prior to enabling profiles.',
  'Do not exceed bounded droop windows or defeat hardware interlocks.',
];

export const buildStatus = (selectedProfile, toggles) => ({
  power: selectedProfile === 'phone-class-downgrade' ? '6.5' : selectedProfile === 'badge-reader-mps-jitter' ? '12.5' : '24.5',
  poeClass: selectedProfile === 'phone-class-downgrade' ? 'Class 2' : selectedProfile === 'badge-reader-mps-jitter' ? 'Class 3' : 'Class 4',
  voltage: toggles.brownout ? '36.5' : '52.0',
  tempMargin: toggles.brownout ? '32' : '37',
  summary: `Profile ${selectedProfile} loaded. LLDP spoof ${toggles.lldpSpoof ? 'active' : 'inactive'}, MPS jitter ${toggles.mpsJitter ? 'active' : 'inactive'}, brownout ${toggles.brownout ? 'armed' : 'off'}.`,
});
''').strip() + '\n'

files['app/App.js'] = dedent('''
// App.js - PoE Whisper companion application
// Author: jayis1
import React, { useMemo, useState } from 'react';
import { SafeAreaView, ScrollView, View, Text, Pressable, StyleSheet } from 'react-native';
import DashboardScreen from './screens/DashboardScreen';
import ProfilesScreen from './screens/ProfilesScreen';
import CaptureScreen from './screens/CaptureScreen';
import InjectorScreen from './screens/InjectorScreen';
import SafetyScreen from './screens/SafetyScreen';
import { AUTHOR, profiles, eventFeed, safetyChecklist, buildStatus } from './utils/protocol';

const tabs = ['Overview', 'Profiles', 'Capture', 'Injector', 'Safety'];

export default function App() {
  const [activeTab, setActiveTab] = useState('Overview');
  const [selectedProfile, setSelectedProfile] = useState('camera-reboot-window');
  const [toggles, setToggles] = useState({
    lldpSpoof: true,
    mpsJitter: false,
    brownout: true,
  });

  const status = useMemo(() => buildStatus(selectedProfile, toggles), [selectedProfile, toggles]);

  const handleToggle = (key) => {
    setToggles((prev) => ({ ...prev, [key]: !prev[key] }));
  };

  return (
    <SafeAreaView style={styles.safeArea}>
      <ScrollView contentContainerStyle={styles.container}>
        <Text style={styles.brand}>PoE Whisper</Text>
        <Text style={styles.byline}>Inline PoE negotiation research companion app by {AUTHOR}</Text>
        <View style={styles.tabRow}>
          {tabs.map((tab) => (
            <Pressable
              key={tab}
              style={[styles.tab, activeTab === tab && styles.tabActive]}
              onPress={() => setActiveTab(tab)}
            >
              <Text style={[styles.tabText, activeTab === tab && styles.tabTextActive]}>{tab}</Text>
            </Pressable>
          ))}
        </View>

        {activeTab === 'Overview' && <DashboardScreen status={status} />}
        {activeTab === 'Profiles' && (
          <ProfilesScreen profiles={profiles} selected={selectedProfile} onSelect={setSelectedProfile} />
        )}
        {activeTab === 'Capture' && <CaptureScreen events={eventFeed} />}
        {activeTab === 'Injector' && <InjectorScreen toggles={toggles} onToggle={handleToggle} />}
        {activeTab === 'Safety' && <SafetyScreen checklist={safetyChecklist} />}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#020617',
  },
  container: {
    padding: 18,
    gap: 18,
  },
  brand: {
    color: '#f8fafc',
    fontSize: 30,
    fontWeight: '900',
  },
  byline: {
    color: '#94a3b8',
    lineHeight: 20,
  },
  tabRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 10,
  },
  tab: {
    backgroundColor: '#111827',
    borderRadius: 999,
    paddingVertical: 10,
    paddingHorizontal: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
  },
  tabActive: {
    borderColor: '#22d3ee',
    backgroundColor: '#082f49',
  },
  tabText: {
    color: '#9ca3af',
    fontWeight: '700',
  },
  tabTextActive: {
    color: '#ecfeff',
  },
});
''').strip() + '\n'

files['kicad/device.kicad_pro'] = dedent('''
{
  "board": {
    "design_settings": {
      "defaults": {
        "board_outline_line_width": 0.1,
        "copper_line_width": 0.2,
        "silk_line_width": 0.12
      }
    }
  },
  "meta": {
    "author": "jayis1",
    "project_name": "PoE Whisper",
    "description": "Inline 802.3af/at/bt power negotiation manipulator for authorized security research"
  },
  "schematic": {
    "page_layout_descr_file": ""
  }
}
''').strip() + '\n'

files['kicad/device.kicad_sch'] = dedent('''
(kicad_sch (version 20231120) (generator "Hermes Agent")
  (uuid "2c89ba11-73a2-4a65-a477-c74963f62d70")
  (paper "A3")
  (title_block
    (title "PoE Whisper")
    (company "jayis1")
    (comment 1 "Inline 802.3af/at/bt negotiation manipulator")
    (comment 2 "Author: jayis1")
    (comment 3 "Authorized use only")
  )
  (lib_symbols
    (symbol "Connector:RJ45_Shielded" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "MCU_ST_STM32H5:STM32H563RITx" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "FPGA_Lattice:MachXO3LF-2100C-5BG256C" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "RF_Module:ESP32-C3-WROOM-02" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "Sensor_Current:INA238AIDGSR" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "Relay:G6K-2F-Y" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "Device:Q_NMOS_GSD" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "Device:R_Network08" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
    (symbol "Regulator_Switching:TPS54202" (pin_names (offset 1.016)) (in_bom yes) (on_board yes))
  )
  (symbol (lib_id "Connector:RJ45_Shielded") (at 33.02 92.71 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000001")
    (property "Reference" "J1" (at 33.02 74.93 0) (effects (font (size 1.27 1.27))))
    (property "Value" "RJ45_PSE_IN" (at 33.02 110.49 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Connector_RJ:RJ45_Amphenol_RJHSE538X" (at 33.02 92.71 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "Connector:RJ45_Shielded") (at 33.02 160.02 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000002")
    (property "Reference" "J2" (at 33.02 142.24 0) (effects (font (size 1.27 1.27))))
    (property "Value" "RJ45_PD_OUT" (at 33.02 177.8 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Connector_RJ:RJ45_Amphenol_RJHSE538X" (at 33.02 160.02 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "MCU_ST_STM32H5:STM32H563RITx") (at 132.08 90.17 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000003")
    (property "Reference" "U1" (at 132.08 54.61 0) (effects (font (size 1.27 1.27))))
    (property "Value" "STM32H563RIT6" (at 132.08 125.73 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Package_QFP:LQFP-64_10x10mm_P0.5mm" (at 132.08 90.17 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "FPGA_Lattice:MachXO3LF-2100C-5BG256C") (at 198.12 91.44 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000004")
    (property "Reference" "U2" (at 198.12 57.15 0) (effects (font (size 1.27 1.27))))
    (property "Value" "MachXO3LF-2100C" (at 198.12 125.73 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Package_BGA:caBGA-256_14x14mm_Layout16x16_P0.8mm" (at 198.12 91.44 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "RF_Module:ESP32-C3-WROOM-02") (at 132.08 160.02 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000005")
    (property "Reference" "U3" (at 132.08 142.24 0) (effects (font (size 1.27 1.27))))
    (property "Value" "ESP32-C3-MINI-1" (at 132.08 177.8 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "RF_Module:ESP32-C3-MINI-1" (at 132.08 160.02 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "Sensor_Current:INA238AIDGSR") (at 198.12 160.02 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000006")
    (property "Reference" "U4" (at 198.12 146.05 0) (effects (font (size 1.27 1.27))))
    (property "Value" "INA238" (at 198.12 173.99 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Package_TO_SOT_SMD:WSON-10-1EP_3x3mm_P0.5mm_EP1.6x2.6mm" (at 198.12 160.02 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "Relay:G6K-2F-Y") (at 254.0 92.71 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000007")
    (property "Reference" "K1" (at 254.0 80.01 0) (effects (font (size 1.27 1.27))))
    (property "Value" "BYPASS_RELAY" (at 254.0 105.41 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Relay_SMD:Relay_DPDT_Omron_G6K-2F-Y" (at 254.0 92.71 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "Device:Q_NMOS_GSD") (at 252.73 160.02 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000008")
    (property "Reference" "Q1" (at 252.73 146.05 0) (effects (font (size 1.27 1.27))))
    (property "Value" "BROWNOUT_FET_STAGE" (at 252.73 173.99 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Package_SO:PowerPAK_SO-8_Single" (at 252.73 160.02 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "Device:R_Network08") (at 87.63 160.02 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000009")
    (property "Reference" "RN1" (at 87.63 147.32 0) (effects (font (size 1.27 1.27))))
    (property "Value" "CLASS_SIG_LADDER" (at 87.63 172.72 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Resistor_SMD:R_Array_Convex_8x0602" (at 87.63 160.02 0) (effects (font (size 1.27 1.27)) hide))
  )
  (symbol (lib_id "Regulator_Switching:TPS54202") (at 87.63 92.71 0)
    (unit 1) (in_bom yes) (on_board yes) (uuid "11111111-aaaa-bbbb-cccc-000000000010")
    (property "Reference" "U5" (at 87.63 80.01 0) (effects (font (size 1.27 1.27))))
    (property "Value" "5V_BUCK" (at 87.63 105.41 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "Package_TO_SOT_SMD:SOT-23-6" (at 87.63 92.71 0) (effects (font (size 1.27 1.27)) hide))
  )
  (wire (pts (xy 46.99 86.36) (xy 71.12 86.36)))
  (wire (pts (xy 46.99 99.06) (xy 71.12 99.06)))
  (wire (pts (xy 46.99 153.67) (xy 71.12 153.67)))
  (wire (pts (xy 46.99 166.37) (xy 71.12 166.37)))
  (wire (pts (xy 99.06 92.71) (xy 118.11 92.71)))
  (wire (pts (xy 99.06 160.02) (xy 118.11 160.02)))
  (wire (pts (xy 147.32 92.71) (xy 182.88 92.71)))
  (wire (pts (xy 147.32 160.02) (xy 182.88 160.02)))
  (wire (pts (xy 213.36 92.71) (xy 238.76 92.71)))
  (wire (pts (xy 213.36 160.02) (xy 238.76 160.02)))
  (global_label "POE_VIN" (shape input) (at 55.88 83.82 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "POE_RETURN" (shape bidirectional) (at 55.88 101.6 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "CLASS_CTL[0..7]" (shape output) (at 101.6 156.21 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "BROWNOUT_GATE" (shape output) (at 236.22 156.21 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "I2C_SCL" (shape bidirectional) (at 181.61 153.67 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "I2C_SDA" (shape bidirectional) (at 181.61 166.37 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "+3V3" (shape output) (at 105.41 88.9 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "+5V" (shape output) (at 105.41 96.52 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
  (global_label "GND" (shape bidirectional) (at 105.41 104.14 0) (fields_autoplaced) (effects (font (size 1.27 1.27))))
)
''').strip() + '\n'

files['kicad/device.kicad_pcb'] = dedent('''
(kicad_pcb (version 20231120) (generator "Hermes Agent")
  (general
    (thickness 1.6)
    (legacy_teardrops no)
  )
  (paper "A4")
  (title_block
    (title "PoE Whisper")
    (company "jayis1")
    (comment 1 "Inline 802.3af/at/bt negotiation manipulator")
    (comment 2 "Author: jayis1")
    (comment 3 "Authorized use only")
  )
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user)
    (33 "F.Adhes" user)
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user)
    (37 "F.SilkS" user)
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (44 "Edge.Cuts" user)
  )
  (setup
    (pad_to_mask_clearance 0)
    (pcbplotparams
      (layerselection 0x00010fc_ffffffff)
      (plotreference true)
      (plotvalue true)
    )
  )
  (net 0 "")
  (net 1 "GND")
  (net 2 "+3V3")
  (net 3 "+5V")
  (net 4 "POE_VIN")
  (net 5 "POE_RETURN")
  (net 6 "I2C_SCL")
  (net 7 "I2C_SDA")
  (net 8 "BROWNOUT_GATE")
  (net 9 "CLASS0")
  (net 10 "CLASS1")
  (gr_rect (start 20 20) (end 120 72) (stroke (width 0.1) (type solid)) (fill none) (layer "Edge.Cuts"))
  (gr_text "PoE Whisper\nAuthor: jayis1\nAuthorized use only" (at 72 28) (layer "F.SilkS")
    (effects (font (size 1.5 1.5) (thickness 0.22))))
  (footprint "Connector_RJ:RJ45_Amphenol_RJHSE538X" (layer "F.Cu")
    (at 28 46 90)
    (property "Reference" "J1" (at 0 -10 90) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))
    (property "Value" "RJ45_PSE_IN" (at 0 10 90) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))
    (pad "1" thru_hole circle (at -4.445 -3.81 90) (size 1.6 1.6) (drill 0.9) (layers "*.Cu" "*.Mask") (net 4 "POE_VIN"))
    (pad "2" thru_hole circle (at -3.175 -6.35 90) (size 1.6 1.6) (drill 0.9) (layers "*.Cu" "*.Mask") (net 5 "POE_RETURN"))
  )
  (footprint "Connector_RJ:RJ45_Amphenol_RJHSE538X" (layer "F.Cu")
    (at 112 46 270)
    (property "Reference" "J2" (at 0 -10 90) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))
    (property "Value" "RJ45_PD_OUT" (at 0 10 90) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))
    (pad "1" thru_hole circle (at -4.445 -3.81 270) (size 1.6 1.6) (drill 0.9) (layers "*.Cu" "*.Mask") (net 4 "POE_VIN"))
    (pad "2" thru_hole circle (at -3.175 -6.35 270) (size 1.6 1.6) (drill 0.9) (layers "*.Cu" "*.Mask") (net 5 "POE_RETURN"))
  )
  (footprint "Package_QFP:LQFP-64_10x10mm_P0.5mm" (layer "F.Cu")
    (at 60 46)
    (property "Reference" "U1" (at 0 -7.4 0) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))
    (property "Value" "STM32H563RIT6" (at 0 7.4 0) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))
    (pad "1" smd rect (at -5.7 -3.75) (size 1.4 0.3) (layers "F.Cu" "F.Paste" "F.Mask") (net 6 "I2C_SCL"))
    (pad "2" smd rect (at -5.7 -3.25) (size 1.4 0.3) (layers "F.Cu" "F.Paste" "F.Mask") (net 7 "I2C_SDA"))
    (pad "3" smd rect (at -5.7 -2.75) (size 1.4 0.3) (layers "F.Cu" "F.Paste" "F.Mask") (net 8 "BROWNOUT_GATE"))
    (pad "4" smd rect (at -5.7 -2.25) (size 1.4 0.3) (layers "F.Cu" "F.Paste" "F.Mask") (net 2 "+3V3"))
  )
  (footprint "RF_Module:ESP32-C3-MINI-1" (layer "F.Cu")
    (at 88 46)
    (property "Reference" "U3" (at 0 -8 0) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))
    (property "Value" "ESP32-C3-MINI-1" (at 0 10 0) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))
    (pad "1" smd rect (at -8.2 -6.35) (size 1.2 0.7) (layers "F.Cu" "F.Paste" "F.Mask") (net 2 "+3V3"))
    (pad "2" smd rect (at -8.2 -5.08) (size 1.2 0.7) (layers "F.Cu" "F.Paste" "F.Mask") (net 1 "GND"))
  )
  (footprint "Package_BGA:caBGA-256_14x14mm_Layout16x16_P0.8mm" (layer "F.Cu")
    (at 88 28)
    (property "Reference" "U2" (at 0 -8.5 0) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))
    (property "Value" "MachXO3LF-2100C" (at 0 8.5 0) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))
    (pad "A1" smd circle (at -6 -6) (size 0.45 0.45) (layers "F.Cu" "F.Mask") (net 9 "CLASS0"))
    (pad "A2" smd circle (at -5.2 -6) (size 0.45 0.45) (layers "F.Cu" "F.Mask") (net 10 "CLASS1"))
    (pad "B1" smd circle (at -6 -5.2) (size 0.45 0.45) (layers "F.Cu" "F.Mask") (net 8 "BROWNOUT_GATE"))
  )
  (segment (start 23.555 42.19) (end 52 42.19) (width 0.4) (layer "F.Cu") (net 4))
  (segment (start 23.555 39.65) (end 52 39.65) (width 0.4) (layer "F.Cu") (net 5))
  (segment (start 68 42.25) (end 82 42.25) (width 0.25) (layer "F.Cu") (net 6))
  (segment (start 68 42.75) (end 82 42.75) (width 0.25) (layer "F.Cu") (net 7))
  (segment (start 68 43.25) (end 82 22.8) (width 0.25) (layer "F.Cu") (net 8))
)
''').strip() + '\n'

for rel, content in files.items():
    path = root / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)

print('generated', len(files), 'files under', root)
