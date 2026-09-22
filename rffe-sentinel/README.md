# RFFE Sentinel — Inline MIPI RFFE Trust and Fault Analyzer

Author: jayis1

Hardware revision: A0

Firmware protocol: RSCP/1

License: CERN-OHL-S-2.0 hardware; MIT firmware and application

> LEGAL AND ETHICAL NOTICE: RFFE Sentinel is for authorized security research, product validation, interoperability testing, and red-team work performed with explicit written permission. Radio front-end control can affect emissions, receiver behavior, battery safety, and regulatory compliance. Use only on equipment you own or are specifically authorized to test, inside a shielded laboratory or otherwise compliant setup. The design does not include an RF transmitter and its default mode is fail-open passive observation. jayis1 does not authorize interference, operation outside regional limits, surveillance, or modification of third-party equipment.

## Purpose and overview

RFFE Sentinel is a compact inline analyzer and policy firewall for the MIPI Alliance RF Front-End Control Interface (RFFE). RFFE is the low-pin-count bus that lets a cellular, Wi-Fi, GNSS, or multi-radio baseband configure power amplifiers, low-noise amplifiers, antenna tuners, envelope trackers, filters, and RF switches. It is fast, timing-sensitive, usually inaccessible to software observability, and often treated as trustworthy merely because it is routed inside a product. That assumption matters: one malformed register write can select the wrong antenna path, disable receive protection, force excessive gain, disturb coexistence, or place a power amplifier in an unsafe state.

The device sits between an RFFE controller and its front-end peripherals. A de-energized, normally closed DPDT signal relay preserves the original connection when RFFE Sentinel is unpowered. When explicitly armed, two independent electrical ports and a small FPGA observe command frames, verify parity and bus-park timing, timestamp transactions, and forward approved frames. A policy engine on an STM32G474 microcontroller compares writes with an operator-supplied allowlist, tracks temporal sequences, and records violations. In a shielded, authorized lab, the operator may enable deterministic fault campaigns: suppress one transaction, delay a write within a configured budget, substitute a safe value, or inject a parity error. These controls are deliberately bounded and require a physical arm button plus an expiring software lease.

RFFE Sentinel solves a gap between a generic oscilloscope and baseband software logging. Oscilloscopes show voltage but do not understand extended register writes, broadcast IDs, timing windows, or device state. Software logs show intended operations but cannot prove what appeared on the wire. RFFE Sentinel correlates both layers. It can expose compromised radio firmware, unsafe vendor sequences, integration defects, bus contention, intermittent flex-cable faults, and undocumented peripheral behavior without demodulating or transmitting user traffic.

This concept does not duplicate the repository's broad RF receivers, radio replay tools, I3C analyzer, or general embedded bus taps. It targets the RFFE electrical profile, command grammar, sub-microsecond timing, and front-end safety semantics. It never processes baseband samples, packet payloads, subscriber identifiers, or over-the-air content.

## Scope and non-goals

The A0 hardware architecture targets RFFE v1.x through v3.x command shapes that fit its 52 MHz capture path, 1.2 V and 1.8 V I/O domains, up to sixteen slave IDs, standard and extended register transactions, read turnarounds, and bus-park validation. The included host-compilable firmware reference currently decodes the fixed 25-bit, one-byte standard transaction fixture format used by its tests; extended framing, physical edge qualification, and true bus-park measurement belong in the FPGA/embedded port and are explicitly not claimed as implemented. The hardware is intended for bench fixtures, development boards, removable radio modules, and products whose RFFE trace can be broken at a flex connector or rework interposer.

It is not an RF jammer, cellular interceptor, spectrum monitor, IMSI collector, modem unlocker, or universal logic analyzer. It does not bypass signed baseband firmware. It cannot make an electrically unsafe setup safe. It does not certify compliance with FCC, ETSI, CE, UKCA, or carrier requirements. The A0 PCB is an engineering reference that must receive design review, signal-integrity simulation, KiCad ERC/DRC, and controlled bring-up before fabrication or connection to valuable equipment.

## Attack surface and threat model

### Assets

The primary assets are front-end integrity, regional configuration, RF component lifetime, receiver availability, emission compliance, and the validity of test evidence. Secondary assets include device identity in RFFE registers, vendor tuning tables, and fault traces. Captures may expose proprietary configuration, so the application stores them locally and provides explicit deletion.

### Trust boundaries

There are five boundaries. The upstream RFFE controller is untrusted because compromised baseband firmware or a buggy driver may emit harmful writes. Downstream peripherals are untrusted because counterfeit or damaged parts can hold SDATA, return unexpected values, or claim the wrong USID. The Sentinel firmware is privileged but treats every FPGA record and host command as untrusted. The companion workstation is outside the real-time safety boundary and cannot directly toggle pins. Finally, USB and capture files are hostile inputs: framing, lengths, counters, and checksums are validated before use.

### Adversaries and failures

* A malicious or compromised radio firmware component tries to alter a PA bias, antenna tuner, or switch state outside an approved operating profile.
* A supply-chain substituted peripheral uses an unexpected product ID or responds on a conflicting address.
* A test engineer accidentally replays a configuration for a different band or board revision.
* A malformed capture file attempts integer overflow, policy bypass, or excessive allocation in the app.
* Electrical faults create runt pulses, contention, incorrect bus park, or timing violations that software logs cannot see.
* An unauthorized physical user attempts to arm injection or retain sensitive traces.
* Power loss occurs while a policy is active or while the device is forwarding traffic.

### Defensive controls

The default state is transparent fail-open bypass. Active intervention requires the physical ARM button, a USB command containing a fresh monotonic nonce, and a lease of at most five minutes. The FPGA has hard bounds: it cannot drive SDATA outside a recognized frame window, cannot exceed a configured edge count, and releases the line when its watchdog expires. Firmware policies can only deny, delay, or replace values in explicitly enumerated register ranges. There is no wildcard injection rule. Safety-critical deny rules cannot be disabled without another physical confirmation.

Capture records use fixed-size fields and CRC-32C. The parser rejects unknown protocol versions, oversized payloads, non-monotonic sequence numbers, and invalid directions. The application never interprets captures as executable code. Exports redact USB serial identifiers by default. Factory reset erases policies, pairing state, and capture metadata. Production builds should enable STM32 read-out protection, secure boot through a signed first-stage loader, and disable SWD after provisioning; A0 leaves SWD accessible for bring-up and says so rather than claiming a security property it does not yet implement.

### Residual risks

Active insertion adds capacitance and propagation delay. Incorrect level selection can damage a 1.2 V target. A policy that permits a syntactically valid but semantically unsafe sequence can still cause harm. The bypass switch cannot preserve traffic during every imaginable brownout transient. FPGA or firmware defects may drop frames. For these reasons, intervention is a lab-only feature, the first connection must be passive, and a scope should verify signal integrity before active mode.

## Hardware specification

| Function | Selected part / characteristic | Rationale |
|---|---|---|
| Control MCU | STM32G474RET6, Cortex-M4F at 170 MHz, 512 KiB flash, 128 KiB SRAM | Fast timers, USB device, CRC, comparators, mature toolchain |
| Timing FPGA | Lattice iCE40UP5K-SG48 | Deterministic DDR sampling, forwarding, parity, and turnaround control |
| Port translators | Two NVT2002DP bidirectional open-drain translators with selectable 1.2/1.8 V references | Separate upstream/downstream domains and measurable isolation |
| Bypass | Omron G6K-2F-Y DPDT signal relay with MOSFET coil driver and flyback diode | De-energized, unpowered direct path |
| Voltage monitor | ADS7042 12-bit 1 MSPS ADC plus resistor dividers | Captures SDATA/SCLK high level and detects contention droop |
| Current monitor | INA226 on target reference input | Detects wiring faults and records fixture current |
| Secure identity | ATECC608B | Device certificate and signed capture manifest support |
| Storage | 16 MiB MX25L128 QSPI NOR | Circular event journal; no radio and no cloud dependency |
| Host link | USB-C, USB 2.0 full-speed CDC | Deterministic local control and power |
| User controls | ARM pushbutton, MODE pushbutton, RGB status LED | Physical authorization and visible intervention state |
| Target connectors | Two 6-pin 0.5 mm FFC plus 0.1-inch debug headers | Fixture integration and bench access |
| Power | USB 5 V; TPS62172 3.3 V; TPS62840 selectable 1.2/1.8 V translator rail | Target VIO is sensed only and is never sourced by A0 |
| Board | 52 mm × 28 mm, four layers, controlled return plane | Portable but large enough for safe routing and test points |

The target header exposes SCLK, SDATA, VIO sense, ground, optional fixture detect, and shield. RFFE Sentinel A0 never sources target VIO; it senses the target reference and enables an independently generated, matching translator rail only after validating the voltage. A separate powered-target fixture would require its own reviewed schematic and is outside this design.

### Power and signal budget

The design consumes about 95 mA from USB in passive capture and up to 145 mA during active forwarding with flash writes. Target inputs present less than 8 pF per net in bypass mode. The active path budget is 4 ns translator delay plus 3 ns FPGA decision delay at nominal conditions. Policy decisions requiring MCU involvement never happen within a live frame; the MCU preloads a compact rule table into FPGA RAM. This avoids pretending a 170 MHz software loop can safely arbitrate every 52 MHz edge.

## Architecture and block diagram

```text
               authorized target controller
                  SCLK      SDATA      VIO
                    |          |         |
              +-----+----------+---------+-----+
              | UPSTREAM PORT / ESD / VIO SENSE |
              +-----+----------+---------------+
                    |          |
             +------v----------v-------+
             | normally closed bypass  |<---- reset supervisor
             +------+------------+-----+
                    |            |
          +---------v--+      +--v----------+
          | translator |      | translator  |
          | upstream   |      | downstream  |
          +------+-----+      +------+------+
                 |                   |
                 +------+-+----------+
                        | |
                 +------v-v------------------+
                 | iCE40UP5K timing fabric   |
                 | capture / forward / CRC   |
                 | parity / policy CAM / WDT |
                 +-------+-------------+-----+
                         | SPI         | evidence FIFO
                 +-------v-------------v-----+
                 | STM32G474 control plane   |
                 | policy compiler / journal |
                 | USB RSCP/1 / health checks|
                 +----+----------+-----------+
                      |          |
                 QSPI flash   ATECC608B
                      |
                    USB-C  <----> local companion app

             +--------------------------------+
             | DOWNSTREAM PORT / ESD / SENSE  |
             +-------------+------------------+
                           |
                  authorized RFFE peripherals
```

The data plane and control plane are deliberately separated. The FPGA observes every edge and creates a normalized transaction record. A 64-entry content-addressable table stores rule predicates: USID mask, command class, register range, value mask, action, and maximum delay. The MCU compiles human-readable policy into that table before arming. If the table misses, the configured default is observe-and-forward. If clocks stop, FIFOs overflow, firmware stalls, or USB disconnects, intervention ends and bypass engages.

## Firmware design

The firmware in `firmware/` is a host-compilable reference implementation of the safety state machine, RFFE frame decoder, policy engine, circular evidence journal, and RSCP/1 command parser. Hardware register definitions live in `registers.h`; pin and timing contracts live in `board.h`. The host build uses simulated register storage so parser and state-machine behavior can be tested without pretending to exercise physical RFFE timing.

Four states constrain behavior: SAFE_BYPASS, PASSIVE_CAPTURE, ARMED_FORWARD, and FAULT_LATCHED. Boot always enters SAFE_BYPASS. Passive capture may be requested over USB without physical presence, because it never drives target lines. ARMED_FORWARD requires a debounced hardware signal and a valid lease. Any rail alarm, FPGA watchdog event, malformed rule table, FIFO overflow, or host keepalive failure transitions to FAULT_LATCHED and asserts bypass. Clearing a fault records an event and still returns to SAFE_BYPASS rather than resuming intervention.

The reference decoder recognizes the test fixture's 25-bit one-byte transaction, extracts slave address, command nibble, eight-bit register, value, and odd parity, and records raw bits alongside normalized fields. It provides the state-machine and API boundary for the FPGA-backed embedded decoder. Extended addresses, variable byte counts, electrical start/park qualification, and read turnaround are hardware-port work, not simulated by the host build. Unknown command classes are preserved and forwarded in passive mode; they cannot match an active substitution rule.

The policy engine is deterministic. Rules are evaluated by ascending priority. DENY suppresses a complete transaction only when the physical arm lease is valid. SUBSTITUTE applies a bit mask to a single write byte and cannot change addresses, lengths, or direction. DELAY is capped at 2 microseconds and rejected for read turnarounds. ALERT never changes traffic. Rate rules count operations in 10 ms windows and can latch a fault if an amplifier register is thrashed.

The evidence journal uses fixed 64-byte records with sequence, microsecond timestamp, decoded fields, electrical flags, selected action, and CRC-32C. Records are first queued in RAM, then appended to flash pages. Power loss may lose the current page but cannot invalidate previously committed pages. A boot scan stops at the first invalid record. No payload contains over-the-air samples or subscriber data.

### RSCP/1 application protocol

USB messages use a 20-byte header followed by at most 512 bytes: magic `RSCP`, version, message type, flags, sequence, nonce, payload length, and a 16-bit integrity tag folded from CRC-32C over the header fields and payload. All integers are little-endian. This compact host-reference framing is versioned as RSCP/1; a production transport may promote the tag to a full 32-bit CRC in a future major version. Requests receive either an ACK with the same sequence or an ERROR containing a stable error code. The host waits 750 ms and retries an idempotent query twice. Mutating operations include a nonce and are never automatically retried.

Supported commands include GET_STATUS, SET_MODE, LOAD_POLICY, ARM_LEASE, LIST_EVENTS, READ_EVENTS, CLEAR_EVENTS, GET_CAPABILITIES, and FACTORY_RESET. SET_MODE cannot enter ARMED_FORWARD by itself. ARM_LEASE fails unless the physical button was pressed during the preceding ten seconds. Protocol version mismatches fail closed and report the device's supported major version.

### Build and test

```sh
cd firmware
make clean check
./rffe-sentinel --demo
```

The host target uses a standard C11 compiler with warnings promoted to errors. An embedded port should replace simulated MMIO functions, provide the FPGA bitstream loader, connect TinyUSB, and use the vendor startup/linker files. Those board-support pieces are intentionally not misrepresented as tested here. The logic shared with that port is buildable and exercised by `--self-test`.

## Companion application

The `app/` directory contains a local-first browser application requiring only Node.js for tests and any modern browser for operation. It models the production interface without depending on an app store or cloud service. The dashboard shows connection state, hardware revision, mode, lease expiry, rail measurements, event rate, parity failures, and bypass status. The timeline filters evidence by USID, command, register, action, and electrical warning. The policy screen creates explicit register-range rules and refuses wildcard active actions. The safety screen presents the authorized-use notice, explains the physical arm step, displays an intervention countdown, and offers immediate return to bypass.

The included demo transport produces deterministic synthetic records so all screens can be exercised without hardware. A production WebUSB adapter can replace it after adding an RSCP/1 transport module; the current application tests CRC-32C, policy validation, and evidence filtering rather than claiming a hardware connection. Capture export emits JSON with protocol metadata and device-identity redaction. Import accepts only JSON files under 1 MiB with the expected author, protocol, and event-array schema. Delete is local and immediate; there is no telemetry.

Run it with:

```sh
cd app
npm test
npm run start
```

Then open the printed local URL. The application separates observation and intervention visually: passive controls are blue, intervention controls are amber, and a latched fault is red. It never labels a software click as sufficient authorization.

## Security-research use cases

### Baseband and driver validation

A product security team can record the exact RFFE sequence during modem boot, band changes, transmit enable, receive diversity changes, and shutdown. The wire trace is compared with the software driver's audit log. Differences identify compromised firmware, undocumented coprocessor behavior, race conditions, or logging blind spots.

### Front-end allowlist development

During normal operation in a shielded chamber, engineers collect register ranges and temporal sequences for every supported radio mode. They convert this evidence into an allowlist, replay the test matrix, and observe unexpected writes. The goal is not to block unknown traffic immediately but to build a reviewable model of intended behavior.

### Fault-injection resilience

With written authorization and a sacrificial fixture, a researcher can drop a tuner update, delay a switch transition, corrupt parity, or substitute a bounded value. They can then verify whether firmware detects the fault, shuts down transmission, retries safely, and records a useful diagnostic. Campaign limits prevent unattended indefinite intervention.

### Supply-chain and repair analysis

RFFE peripheral IDs, supported registers, turnaround timing, and electrical signatures can distinguish expected assemblies from substitutions or counterfeit modules. The device cannot prove authenticity alone, but it provides reproducible physical evidence for a larger attestation process.

### Coexistence incident triage

Intermittent Wi-Fi/cellular/GNSS failures may originate in front-end switch timing rather than protocol stacks. Correlating RFFE events with external chamber instruments helps find unsafe overlap, stale tuner state, or a component that holds SDATA. Because the Sentinel does not capture RF payloads, it can reduce privacy exposure compared with broad radio recording.

### Red-team trust-boundary demonstration

An authorized red team can show that a hardened application processor still depends on a physically exposed control bus. A passive trace demonstrates the attack surface; a carefully bounded denied write demonstrates the effect of bus mediation without deploying persistence, extracting communications, or targeting third parties.

## Design decisions

An iCE40UP5K is used instead of bit-banging because edge timing and release behavior must be deterministic. The STM32G474 was selected for timer depth, USB, analog supervision, and wide tool support rather than wireless connectivity. USB-only management prevents the analyzer from adding a new radio attack surface. QSPI NOR is used instead of microSD because fixed records, bounded capacity, and predictable power-loss behavior are more important than removable bulk storage.

The fail-open bypass favors target availability during analyzer failure. In a safety certification fixture, a fail-closed variant may be appropriate, but that is not the default. Active policy is precompiled into FPGA RAM because consulting firmware per transaction is neither fast nor reliable. Substitution is restricted to one data byte because arbitrary frame synthesis would expand both hazard and verification scope.

## Verification status and limitations

| Item | Status |
|---|---|
| C policy, parser, journal, and state-machine host build | Implemented and tested by `make check` |
| App CRC, policy validation, evidence filtering, and safety UI tests | Implemented and tested by `npm test` |
| KiCad textual structure and named component/net presence | Checked in repository validation |
| KiCad ERC/DRC and fabrication outputs | Not run; `kicad-cli` is not installed in the authoring environment |
| FPGA gateware | Architecture specified; not included in A0 repository deliverable |
| Hardware timing and signal integrity | Requires simulation and physical prototype |
| Secure boot and locked debug | Production recommendation, not implemented in host reference |
| Regulatory compliance | Not certified |

The KiCad files contain real symbols, footprints, named nets, a four-layer outline, and routed representative nets, but they remain an unverified engineering design. Do not fabricate before independent schematic review, ERC/DRC, footprint confirmation against datasheets, impedance review, and controlled first-article bring-up.

## Bring-up plan

First inspect every footprint and connector orientation against manufacturer drawings. Run ERC and DRC in KiCad 8, then export a netlist and compare SCLK, SDATA, VIO, bypass select, FPGA SPI, USB, and rails with `board.h`. Populate power supplies and USB protection only; verify current and ripple. Populate MCU and SWD, run host-derived self-tests on target, then populate FPGA and validate its watchdog with no target attached. Characterize bypass capacitance and active-path delay using a pattern generator and scope at 1.2 V and 1.8 V. Connect only a disposable RFFE fixture in a shielded lab. Begin in bypass, then passive capture, and enable bounded intervention last.

## Bill of materials summary

The principal parts are STM32G474RET6, iCE40UP5K-SG48, two NVT2002DP translators, an Omron G6K-2F-Y normally closed bypass relay with MOSFET driver, ADS7042 ADC, INA226 monitor, ATECC608B identity device, MX25L128 QSPI flash, TPS62172 and TPS62840 regulators, USBLC6-2SC6 ESD protection, USB-C receptacle, two FFC connectors, SWD header, buttons, LED, precision dividers, and decoupling. Expected prototype BOM is approximately USD 48 excluding PCB assembly and fixture cables.

## Responsible use and disclosure

Record written scope before connecting the device. Prefer passive evidence collection. Use a shielded enclosure and dummy loads for active radio-front-end testing. Minimize retention, redact identifiers, encrypt exported evidence at rest, and delete it when the engagement ends. Stop if behavior suggests unsafe emissions, thermal stress, battery stress, or target instability. Report discovered product vulnerabilities through the vendor's security channel with reproduction details that do not expose third-party communications.

RFFE Sentinel is a research instrument, not permission. Authorization, privacy law, radio regulation, product-safety obligations, and professional judgment remain the operator's responsibility.
