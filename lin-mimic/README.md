# LIN Mimic

**Author and creator: jayis1**  
**Copyright © 2026 jayis1**  
**Status:** Revision 1 research design

LIN Mimic is a fail-open, two-sided Local Interconnect Network (LIN) security assessment instrument. It sits between an automotive or industrial LIN controller and a peripheral, learns the schedule and signal behavior without transmitting, and can then reproduce a selected node in a controlled laboratory harness. Unlike a generic logic analyzer, USB-to-LIN cable, or CAN tool, LIN Mimic treats the two sides as separate trust domains. It records direction, timing, voltage, current, parity, checksum mode, collisions, and behavioral drift while a hardware bypass relay preserves normal operation whenever power, firmware, or operator authorization is absent.

The practical problem is that LIN is inexpensive and pervasive but difficult to assess safely. Seat controllers, rain sensors, smart alternators, HVAC actuators, steering-wheel switches, lighting modules, battery sensors, and industrial subassemblies often trust any electrically valid response at the expected identifier. Existing adapters generally expose one bus and leave schedule reconstruction, safe isolation, responder emulation, and evidence capture to ad-hoc scripts. LIN Mimic combines these tasks in one pocket instrument while making passive observation the default and requiring a physical button plus a short-lived host unlock before any generated traffic is possible.

> **Authorized use only.** Connect LIN Mimic only to systems you own or have explicit written permission to test. Automotive networks can command moving mechanisms, disable safety functions, overheat wiring, or create unsafe vehicle behavior. Perform active tests on a fused bench harness with actuators restrained and emergency power removal available. Never fuzz a vehicle in motion or a safety-critical production system. The author, jayis1, provides this research design without warranty; the operator is responsible for legal compliance and physical safety.

## Purpose and original contribution

LIN Mimic is designed around *differential behavioral identity*. It observes a real peripheral on one side and the controller on the other, derives a schedule model, then permits an assessor to replace the peripheral while measuring whether the controller detects the substitution. Conversely, it can replay controller headers toward an isolated peripheral to map undocumented responses. Because the board has two independent transceivers and a normally-closed bypass, it can distinguish requests from responses and isolate a device without rewiring the harness.

Its novel workflow is **learn, constrain, mimic, compare**:

1. **Learn:** capture only, with both conductors joined through the de-energized relay. Build identifier, period, jitter, length, checksum, and byte-change models.
2. **Constrain:** create an explicit allowlist and rate budget. Diagnostic identifiers `0x3c` and `0x3d` begin disabled. Record target voltage before enabling transmitters.
3. **Mimic:** open the bypass and emulate only approved responder frames, either with static data, bounded mutations, or app-defined signal transforms.
4. **Compare:** score controller reactions, timing tolerance, checksum enforcement, freshness behavior, and error recovery against the baseline.

This is not another CAN injector. LIN is a single-wire, single-master scheduled protocol with different electrical and timing behavior. The design specifically handles protected identifiers, classic and enhanced checksums, dominant break detection, response slots, and LIN’s 12 V physical layer.

## Attack surface and threat model

### Assets under test

The primary assets are command integrity, sensor authenticity, actuator availability, diagnostic-session boundaries, schedule integrity, and the confidentiality of proprietary signal mappings. A typical controller assumes that a response arriving after its header came from the intended slave. Basic LIN has no cryptographic source authentication, freshness guarantee, or confidentiality. Some products add rolling counters or application checksums, but implementation quality varies.

### Adversary capabilities modeled

LIN Mimic models an adversary with temporary physical access to an exposed harness, service connector, headlamp assembly, seat loom, battery sensor lead, or development fixture. The adversary may passively infer identifiers and timing; disconnect or impersonate one peripheral; alter selected response bytes; replay a previously valid response; introduce timing jitter; provide malformed parity or checksum values; issue legal diagnostic requests; or test whether a controller notices silence and collisions.

The tool intentionally does not automate destructive voltage faults, continuous dominant-bus denial, immobilizer bypass, key extraction, or uncontrolled diagnostic exploitation. It has no high-energy glitch output. Those omissions narrow the design to protocol assurance and responder-authentication research.

### Questions the instrument answers

- Does the controller accept a substituted node with the right identifier but the wrong timing fingerprint?
- Are rolling counters, application CRCs, or plausibility checks present and enforced?
- Does a slave respond to undocumented identifiers or diagnostics outside its intended state?
- Are classic and enhanced LIN checksums selected correctly for each identifier?
- How much response delay, jitter, silence, or malformed traffic causes fallback behavior?
- Can one compromised peripheral affect unrelated schedule slots?
- Does the controller distinguish an electrical collision from an absent node?
- Can a captured response be replayed after ignition or power-state changes?

### Trust boundaries

The host application, USB link, firmware parser, controller-side transceiver, target-side transceiver, and attached network are separate trust boundaries. Captured bus bytes are hostile input. USB commands are untrusted until length and CRC checks pass. An app unlock is insufficient by itself: active modes also require the physical ARM button and expire after 30 seconds. The normally closed relay is the safety authority; firmware must energize it to split the bus. Loss of power restores continuity.

Residual risks remain. A welded relay, incorrect harness wiring, transceiver failure, firmware defect, or mistaken signal interpretation can affect the target. The board cannot determine whether a seemingly benign byte controls a hazardous actuator. Physical interlocks, fusing, a bench supply current limit, and a test plan remain mandatory.

## Hardware specification

- **MCU:** STM32G474CBT6, Arm Cortex-M4F at up to 170 MHz, 128 KiB flash, 32 KiB SRAM. Its independent USARTs support LIN break detection while timers provide microsecond timestamps.
- **Physical layer:** two TJA1021T/20 LIN transceivers, one facing the controller and one facing the peripheral. Each has independent sleep control and local ESD protection.
- **Isolation topology:** Panasonic TQ2SA-L2-5V latching DPDT relay wired normally closed across LIN and ground reference. In the unpowered state the original network remains connected. The second pole provides positive contact-state sensing.
- **Protection:** 1 kW bidirectional automotive TVS on each LIN side, 33 Ω series damping, reverse-polarity MOSFET, 500 mA resettable fuse, load-dump-rated 36 V buck regulator, USB ESD array, and keyed connectors.
- **Measurement:** divided 0–24 V LIN/target supply ADC input and INA180 current monitor for tool consumption and gross fault detection. This is diagnostic measurement, not calibrated instrumentation.
- **Connectivity:** USB-C device port exposing CDC for the framed protocol and DFU for signed firmware updates. No radio is fitted, reducing remote attack surface and avoiding accidental connection during vehicle work.
- **Controls:** guarded ARM pushbutton, tri-color status LED, bypass-state LED driven from relay feedback, and recessed reset.
- **Power:** 7–24 V protected target input or 5 V USB. Target power is sensed but never sourced onto LIN. Typical consumption is below 90 mA at 12 V.
- **Connectors:** keyed three-pin controller and target ports (`VBAT`, `LIN`, `GND`), USB-C, SWD development pads, and two scope test loops.
- **Form factor:** 82 × 48 mm four-layer PCB in a 3D-printable 92 × 58 × 24 mm enclosure. Keep the two harness ports on opposite ends to make the trust boundary visible.

## Architecture

```text
 Controller harness                                            Peripheral
 VBAT/GND/LIN                                                     harness
      │                                                              │
 [TVS + sense]                                                  [TVS + sense]
      │                                                              │
 [TJA1021 A]──USART2 RX/TX──┐                         ┌──USART3──[TJA1021 B]
      │                     │                         │               │
      ├────── normally-closed DPDT fail-open bypass ─┤               │
      │                     │                         │               │
      │                  [ STM32G474 ]                │               │
      │                timers / policy /              │               │
      │              dual LIN decoders / ADC          │               │
      │                     │                                         │
      └── protected buck ───┼── USB-C CDC/DFU ── Companion app       │
                            └── ARM button + LEDs + relay feedback
```

The PCB separates dirty automotive power and LIN protection from the 3.3 V digital region. The top layer carries short protected LIN routes. Layer 2 is an uninterrupted ground plane, layer 3 carries power, and layer 4 carries low-speed control. Transceivers sit next to their connectors; the MCU and USB connector remain behind the protection boundary. Test points expose both receive lines, relay drive, 3.3 V, and ground.

The relay name can sound contradictory: it is a *fail-open security boundary* but electrically uses normally closed contacts to fail transparent. In normal capture and on power loss, the original bus is uninterrupted. For controlled isolation, firmware energizes the relay to separate the sides. The transceivers then allow the MCU to relay approved traffic or emulate one endpoint. A watchdog reset immediately removes relay drive.

## Firmware

Firmware is written in portable C11. `main.c` owns the event loop and never blocks on host input. `drivers/lin.c` implements PID parity, classic and enhanced checksum calculation, a per-side receive state machine, frame finalization, timeout recovery, and transmit helpers. `drivers/policy.c` centralizes authorization, mode transitions, identifier masks, voltage windows, expiration, and per-second rate limits. `drivers/protocol.c` implements an escaped binary USB protocol with CRC-16/CCITT. `board.c` contains the STM32 register-level hardware layer and a deterministic host simulation used by CI.

Every observed frame records a microsecond timestamp, source side, protected ID, data length, payload, checksum family, parity state, checksum state, and collision flag. A 64-entry latest-frame cache supports immediate mimic templates. Production firmware should move longer traces through USB continuously; it does not pretend the MCU’s RAM is a forensic archive.

### Modes

- **Observe:** default. Relay is transparent and no firmware path may transmit. Both receivers capture the same conversation from their respective electrical sides.
- **Inline:** sides are logically supervised and valid frames may be bridged. This mode is useful for selective suppression and timing measurement but carries more availability risk.
- **Emulate:** an isolated endpoint responds to approved headers using learned or operator-provided data.
- **Fuzz:** applies only bounded, explicit mutations to allowed identifiers. Rate, voltage, physical-arm, session expiry, frame length, and PID validity checks still apply.

The firmware’s unlock token is deliberate friction rather than cryptography. Security rests on physical control of the device, the ARM button, short session lifetime, and a wired USB connection. A production derivative can add challenge-response with a secure element, but that does not replace physical safety controls.

### Binary host protocol

Packets start with `0x7e`; `0x7d` escapes itself and the delimiter by XOR with `0x20`. The body contains command, payload length, payload, and little-endian CRC-16. Commands request status, unlock an armed session, choose a mode, set the 64-bit identifier allowlist, transmit one frame, or lock immediately. Device events report frames, counters, target voltage, current, arm state, and mode. Parser length is bounded before copying, and bad CRC packets produce no action.

Build and smoke-test the portable path:

```bash
cd firmware
make test
```

For target builds, use `arm-none-eabi-gcc`, the STM32G474 startup file and linker script, and the STM32 USB CDC implementation. The weak USB hooks in `board.c` are the intended integration seam. Production releases should enable readout protection only after recovery procedures and signed images are validated.

## Companion application

The companion is a React/Vite desktop-friendly web interface intended for Chromium Web Serial. It provides a connection panel, live safety state, counters, an identifier inventory, frame timeline, and an explicit transmit composer. The parser and encoder are separate from the UI and have automated tests. The interface does not silently reconnect or automatically enter active mode. Active controls show the hardware-arm and unlock state, and the stop button emits a lock command immediately.

A normal session begins in capture mode. The analyst labels IDs, exports a JSON evidence record, compares period and payload changes, selects a narrow allowlist, presses ARM, unlocks, and only then enters emulation. The UI preserves raw bytes rather than guessing signal semantics. Analysts can layer DBC/LDF knowledge onto exported records without changing the evidence.

## Security research workflows

### Peripheral identity validation

Capture a known-good session through ignition, wake, steady state, and sleep. Disconnect the peripheral side, select only its response IDs, and emulate recorded values. Observe whether the controller logs a fault, accepts stale values, checks response timing, or validates counters. Increment one signal while preserving transport checksum to separate transport integrity from application integrity.

### Undocumented identifier mapping

Isolate a spare peripheral on a current-limited bench supply. Replay only header IDs at conservative rates while recording responses. This reveals supported frame IDs without exposing a complete vehicle. Diagnostic IDs remain excluded unless an approved test plan explicitly enables them.

### Robustness and fault recovery

For one non-safety-critical response, test silence, one checksum error, delayed response, bounded jitter, and a single payload-bit mutation. Record the controller’s time to detection and recovery. LIN Mimic’s counters distinguish a policy denial from a transmitted fault, preserving auditability.

### Supply-state correlation

Measure target voltage with every trace and compare responses during controlled brownout. This can expose fail-open actuator behavior, reset loops, or frames sent before application state is initialized. The onboard ADC is suitable for correlation; use a properly rated oscilloscope for electrical certification.

### Red-team physical-access exercise

Place the unit at a realistic harness access point and measure the time and prerequisites needed to identify a peripheral. The exercise can validate tamper seals, connector access controls, intrusion logging, and whether host ECUs authenticate endpoint behavior. Findings should focus on mitigations: application counters, plausibility checks, authenticated higher-level commands, segmentation, inaccessible routing, and diagnostic gating.

## Design decisions and limitations

A radio was intentionally omitted. Wireless control would add pairing, key storage, RF emissions, and remote attack paths without improving a bench-harness assessment. USB provides power-independent logs and a clear physical control boundary. The latching relay consumes power only during transitions, but the design defaults its contacts to transparent regardless of firmware state.

STM32G474 was selected over a Linux module because deterministic break timing, rapid startup, lower power, and a small trusted code base matter more than local storage or a graphical interface. Two genuine LIN transceivers are used instead of resistor-level UART adapters because automotive thresholds, slew control, wake behavior, and fault tolerance are part of the experiment.

Revision 1 is not galvanically isolated. Controller and peripheral grounds must share a safe reference, as standard LIN nodes do. The board is not qualified to ISO 7637, ISO 16750, functional-safety standards, or road use. It does not decode arbitrary LDF files on-device, infer physical units, defeat secure diagnostics, or guarantee that an emulated frame is safe. The schematic and PCB are engineering-source artifacts and must receive ERC, DRC, footprint, creepage, thermal, and automotive-transient review before fabrication.

## Repository layout

```text
lin-mimic/
├── README.md
├── firmware/
│   ├── main.c, board.c, board.h, registers.h, Makefile
│   └── drivers/lin.*, policy.*, protocol.*
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── package.json, index.html
    ├── src/main.jsx, protocol.js, style.css
    └── test/protocol.test.js
```

## Responsible validation checklist

Before active use, verify connector pinout with a meter; use a fused current-limited bench source; restrain actuators; confirm the bypass is closed with power removed; confirm Observe mode generates no waveform; validate both TVS orientations and relay feedback; capture a baseline; document the exact allowed IDs and rate; keep diagnostic IDs disabled unless needed; have an emergency disconnect; export logs; lock the session before changing wiring; and disclose findings through the system owner’s approved process.

LIN Mimic is useful because it makes a subtle trust assumption measurable: whether a scheduled single-wire network can tell that the expected physical device has been replaced. Its value is not unrestricted packet generation. Its value is repeatable evidence, bounded experiments, direction-aware capture, and a hardware architecture that returns to transparent operation when anything fails.
