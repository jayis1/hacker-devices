# Read Share — Shared-Reader / Exclusive-Writer Ownership Witness

Author and creator: jayis1. Hardware revision A. Protocol RS1.
Firmware, application and tests: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. This directory contains target-linked firmware, an operational Linux desktop companion, and routed native KiCad carrier files. Executed checks are documented in VERIFICATION.md. No board has been fabricated, flashed, electrically qualified, or timing calibrated for this release. The instrument observes a diagnostic contract; it does not enforce a lock, prevent memory corruption, or certify the security of a target.

## Purpose and practical utility

Read Share checks a relationship that an ordinary mutual-exclusion alarm gets wrong: several readers may legitimately hold a resource together, but a writer must hold it alone. An owner-provided adapter exposes admission and release events with a two-bit identity. Tags zero, one, and two represent three distinct reader clients. Tag three represents one writer. The observer reconstructs outstanding ownership, accepts reader sharing, and stops with retained evidence when an incompatible admission, duplicate acquisition, or orphan release occurs.

A motivating application is an embedded appliance whose audit export, policy inspection, and status renderer read a protected configuration while a maintenance path rewrites it. A patch might accidentally admit a reader while the maintenance writer still owns the configuration. Conversely, a generic overlap alarm would report every legitimate pair of readers and produce unusable evidence. Read Share distinguishes these cases without reading the configuration, storing credentials, or driving the target. The diagnostic adapter should report effective ownership, not attempted acquisition or a request placed on a queue.

The instrument also records sharing as positive coverage. Its shared counter counts accepted reader acquisitions made while another reader remains active. This does not measure throughput or prove fairness, but it demonstrates that a regression workload exercised shared admission rather than merely passing because every client ran sequentially. Completed ownership cohorts are counted when the final active owner releases. A reader may leave and rejoin while another reader remains; that is a legal new acquisition within the same cohort, not a duplicate.

For example, acquire reader zero, acquire reader one, release reader zero, reacquire reader zero, release reader one, release reader zero is a valid cohort. It contains three admissions, three releases, and two shared admissions. A following writer acquisition and release is a second cohort. Acquiring any reader before that writer releases is a conflict. Acquiring the writer again while it is already active is classified as duplicate admission instead of a generic conflict, preserving a more specific cause.

## Originality relative to this repository

Repository directories were listed after pulling the default branch. Searches of device READMEs for shared-reader and reader/writer contracts found no existing equivalent. The closest inspected designs were Slot Alias, Exclusion Ledger, Lock Rank, Epoch Witness, and Dependency Diamond. Slot Alias tracks four independent exclusive identities: different occupied slots never conflict. Here, writer tag three conflicts with every reader despite being a different identity. Exclusion Ledger treats every pair of simultaneous grants as a collision; here reader pairs are explicitly permitted. Lock Rank checks acquisition order across two resources and cannot express this one-resource sharing contract.

Dependency Diamond remembers finished jobs until graph completion and checks prerequisite completion before admission. This design has no prerequisites, no dependency graph, and no requirement that all four tags occur. An identity becomes reusable immediately after its release. Epoch Witness tracks ordered firmware-generation announcements, not live ownership. The contribution is the read-sharing policy, retained cohort membership, admission-class conservation, and corresponding tests and research workflow. Reader/writer locks themselves are established engineering. No worldwide novelty, patentability, or unprecedented circuit claim is made.

The passive carrier, STM32 support, bounded transmitter, CRC encoding pattern, and desktop transport deliberately reuse existing jayis1-authored platform work from Dependency Diamond. The new directory is self-contained; it does not import another device at runtime. Reuse avoids inventing a different electrical interface merely for appearance. Standard KiCad footprint geometry and manufacturer documentation retain their upstream provenance; project authorship does not claim authorship of those external assets.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Scope must cover instrumentation, physical attachment, diagnostic firmware, approved workload, possible interruption, and retention of evidence. Permission to assess an application does not automatically authorize opening its appliance or attaching to internal signals. Begin on an isolated low-energy development fixture with dangerous actuators disconnected. Never attach this unisolated prototype to deployed access controls, medical equipment, vehicles, machinery, mains-related circuits, or unidentified field wiring.

An authorized red team can ask the owner to expose a suspected ownership race through an ordinary lab workload. A penetration tester can compare pre-fix and post-fix traces while retaining evidence of actual shared-reader coverage. A security researcher can examine cleanup paths, reader reentry, and writer admission after the last reader exits. This instrument does not generate exploit traffic, induce races, change permissions, steal credentials, persist on the target, or hide its presence. There is no radio, network listener, target transmitter, cloud upload, or autonomous attack mode.

The assessed attack surface is the target's resource-ownership boundary. Relevant defects include admitting a writer before readers drain, admitting a reader during mutation, duplicate callbacks, unmatched cleanup, and mislabeled identities. A dishonest or incorrect adapter can falsify every indication. A diagnostic release that occurs before the actual critical section ends invalidates the meaning of the measurement. Review the adapter with the owner and corroborate findings with source review, functional effects, and independent waveform capture. A sampled contract violation is not automatically an exploitable security vulnerability.

Trusted components include the operator, harness, Nucleo board, observer image, programming interface, and workstation. Serial frames and replay files are untrusted parser inputs. CRC detects accidental corruption but is not authentication. There is no signed log, trusted wall clock, persistent boot identifier, secure element, encrypted export, secure bootloader, or cryptographically authenticated update. An attacker with programming access can replace the observer image and drive nominal input pins. Receive-only therefore describes this firmware, not a certified physical diode.

Physical attachment can disturb the target through common ground, loading, shorts, or back-power. Keep the harness visible and labelled, agree on a stop procedure, and disconnect if attachment changes behavior. A broken input can appear low through its pulldown. Absence of a finding does not establish correctness, complete observation, or safety. RAM evidence disappears on reset. Follow engagement-specific retention requirements; ordinary file deletion is not secure erasure.

## Hardware specifications and power

The compute module is an external ST NUCLEO-F401RE, MB1136 family, using the STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware assumes the nominal reset-default 16 MHz HSI clock and a one-millisecond SysTick. USART2 sends 115200-baud, eight-bit, no-parity, one-stop-bit telemetry through PA2 to the ST-LINK virtual COM bridge. The Nucleo supplies regulation, decoupling, reset, SWD programming, and USB connectivity. Those facilities are not omitted onboard carrier components: the carrier is intentionally passive.

The board is 80 by 70 mm, two layers, nominally 1.6 mm thick, with 0.3 mm routed tracks and a closed rectangular outline. Fourteen through-hole components provide thirty electrical pads on nine nets. It is connected by a harness rather than stacked as a pin-compatible shield. No enclosure model or mounting holes are supplied. Use insulating support, strain relief, and a protective cover appropriate to the bench. There is no battery, charger, FPGA, antenna, analog sensor, active level shifter, isolated supply, or target power output.

| Reference | Quantity | Component specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical through-hole 2.54 mm header |
| J2 | 1 | 1x4 vertical through-hole 2.54 mm header |
| Harness/support | 1 set | Labelled insulated leads and nonconductive support |

Resistors use DIN0207 bodies and 7.62 mm lead spacing. Headers use standard single-row vertical KiCad footprints. Select purchased parts against actual body size, mating geometry, lead spacing, and environmental requirements. No stock availability, price, assembly yield, or connector fit is promised. Substituting another MCU module requires rechecking clocks, reset behavior, registers, pins, programming, and serial routing rather than simply changing the name in the BOM.

| Function | Target header pin 1 | J2 signal pin | External Nucleo connection |
| --- | --- | --- | --- |
| TAG0 | J3 | 1 | CN8 A0 / PA0 |
| TAG1 | J4 | 2 | CN8 A1 / PA1 |
| ACQUIRE | J5 | 3 | CN8 A2 / PA4 |
| RELEASE | J6 | 4 | CN8 A3 / PB0 |

Pin two of each target header is ground. Both J1 pins are ground only; neither accepts a supply. J2 has no power or ground pin. Connect a separate Nucleo ground lead to J1. Names ending `_IN` are target-side nodes before the series resistor; names without that suffix are MCU-side nodes with pulldowns. Verify pin-one orientation and continuity before attachment rather than trusting harness colors.

Only known common-ground zero-to-3.3 V push-pull logic is supported. The nominal resistor path from each target signal to ground is 101 kOhm before leakage and source impedance. The 1 kOhm resistor does not constitute a qualified overvoltage barrier. There is no dedicated ESD clamp, isolation, reverse-current protection, fuse, or hot-plug guarantee. Five-volt, negative, 1.8 V, differential, relay, industrial, and unknown-voltage signals are outside the supported contract.

Power the Nucleo through its documented ST-LINK USB connector after checking the actual board revision, jumpers, and solder bridges. A conservative planning allowance is a 5 V source capable of 500 mA; consumption has not been measured and this is not USB certification. USB ground joins target ground. Power the observer before enabling target indications, and remove those indications before observer power. Otherwise MCU protection structures can back-power an unpowered device. Validate ground offsets, leakage, thresholds, loading, and sequencing physically before interpreting results.

Manufacturer references are ST UM1724 for MB1136 boards and RM0368 for STM32F401 peripherals. The software register map is reused from the existing platform. Check current documents and the purchased revision before wiring; a CAD check cannot establish external-module configuration or electrical interoperability.

## Architecture and block diagram

```text
Owner-controlled diagnostic adapter       Passive carrier         Nucleo
TAG0 ------------------------------------ 1k --+------------------- PA0
TAG1 ------------------------------------ 1k --+------------------- PA1
ACQUIRE --------------------------------- 1k --+------------------- PA4
RELEASE --------------------------------- 1k --+------------------- PB0
                                               | each channel
                                              100k
                                               |
Target reference ----------------------------- GND ---------------- USB GND

GPIO samples -> continuity guard -> bundled tag / independent strobe qualification
             -> prior setup and opposite-strobe guard -> ownership policy
             -> transactional counters -> RS1 CRC framing -> USART2 TX
             -> ST-LINK USB -> Linux Ledger / History / Integrity screens
```

GPIOA and GPIOB are read separately. There is no atomic cross-port latch or high-speed trace capture. Deliberately slow diagnostic strobes are required. Begin with twenty milliseconds of tag setup, twenty milliseconds high, and twenty milliseconds low per event, then validate waveforms independently. Qualification cannot calibrate oscillator error, inter-port skew, interrupt latency, wiring delay, or connector loading. Do not map processor-speed acquisitions directly onto these wires and assume all activity was observed.

## Firmware contract and design decisions

On startup the observer waits for qualified low strobes. The operator must first establish that the target resource has no owners; quiet wires alone cannot prove that. Startup during an existing critical section is unsupported. Three unchanged intervals qualify the bundled two-bit tag and each strobe separately. Tag chatter cannot indefinitely conceal an asserted strobe. Five prior matching tag observations establish setup before an operation; setup cannot be earned on the qualifying event itself. The raw tag must match the qualified tag and the opposite raw and qualified strobe must be low.

A qualified ACQUIRE rising edge is an admission event, not a held ownership level. A held strobe produces only one event. Tags zero through two may be admitted while other readers remain. Writer tag three requires an empty active set, and readers cannot enter while the writer is active. Duplicate acquisition of an active identity takes precedence over incompatibility. RELEASE must identify a currently active owner. Release order is arbitrary. Upgrades, downgrades, recursive locks, and multiple writers are not encoded: implement those as separately reviewed release/acquire operations or use another contract.

The active bitmap stores current owners. The seen bitmap retains identities that participated in the currently nonempty cohort, even after their release. It clears when the last owner releases. It is diagnostic membership, not a prohibition on reuse. The rounds counter counts closed nonempty cohorts. Launched and completed count accepted admissions and releases. Readers and writers divide accepted admissions by class. Shared counts reader admissions into an already nonempty reader cohort; it does not count time or all reader pairs. Attempts includes both accepted operations and rejected edges.

The following invariants are checked before serialization and in the companion: launched equals completed plus active population; launched equals readers plus writers; launched equals rounds plus one if a cohort remains active plus shared; attempts equals launched plus completed plus rejected. Active ownership cannot mix writer and reader bits. Active identities must belong to seen, and seen cannot mix reader and writer identities. These constraints detect corrupted or impossible evidence but are not a proof that an adversary could not forge a plausible record.

The first policy or interface fault halts ownership accounting. Acquisition discontinuity, counter exhaustion, or watchdog initialization failure also halts. A simultaneous pair of rising strobes counts two rejected attempts and an interface fault. Other policy faults count one. Active and cohort evidence is retained; later raw samples do not silently repair it. Reestablish an owner-free target baseline and restart the observer for another run. There is intentionally no automatic recovery from quiet strobes, because an owner could still exist internally.

All per-sample accounting updates are transactional. If a bounded counter would overflow, the prior evidence is retained and the exhaustion fault is set. Time continues to advance in terminal status; transmit-failure diagnostics can also advance. A missed sample invalidates continuous observation and records the gap rather than treating absent time as stable qualification. Unsigned modular time supports ordinary timestamp wrap, but gaps and ambiguous long host intervals are not silently accepted. A stuck owner has no timeout: this is an ownership compatibility witness, not a deadlock or starvation detector.

The implementation has no heap, RTOS, vendor SDK dependency, target command receiver, or configurable remote policy. Startup copies initialized data, clears BSS, supplies the vector table, and enters the production scheduler. The linker targets the F401RE memory map and reserves stack headroom. USART transmission is nonblocking and bounded to one byte per service call. A stalled frame times out instead of stalling acquisition. Independent watchdog initialization is bounded and startup failure is reflected before the first status record.

## Application and wire interface

Run `python3 app/console.py` on Linux with Python 3 and Tk installed. No pip packages or network services are needed. Select the serial path and choose New live. The app configures 115200 8N1 input, refuses a second connection while a port is owned, and restores terminal settings when possible. Disconnect preserves evidence and provenance but removes live status. Permission errors, missing paths, and unplug events are shown rather than silently creating synthetic measurements.

Ledger displays all counters, masks, phase, and fault descriptions. History retains at most 256 accepted snapshots. Integrity shows origin, compatibility, parser errors, reset indications, freshness, and frozen-session warnings. Replay loads a regular file bounded to two MiB and never labels it live. Export JSON creates a new mode-0600 file and refuses overwrite; it includes retained history and integrity metadata even when no cohort completed. Exports are plaintext and should stay in engagement-controlled storage.

RS1 frames are ASCII bodies followed by `*`, four uppercase CRC hexadecimal digits, and LF. CRC-16/CCITT starts at 0xffff and covers the body before the asterisk. B contains the captured reset flags, P announces the policy values `3,1,5,4`, and S contains unsigned decimal fields in this exact order:

```text
now levels valid halted faults phase active seen stable observed rounds
attempts launched completed rejected gaps missed readers writers shared tx_fail
```

Policy repeats about once per second to support late host attachment without pretending that a new boot occurred. Status is attempted about every hundred milliseconds. The host requires policy compatibility and advancing snapshots for live indication. Duplicate timestamps do not renew freshness. Counter rollback, impossible transitions, disappearing faults, and changed terminal accounting freeze the session. Host/device progress checks and a bounded reception-pause rule prevent a backlog from indefinitely masquerading as current evidence. A reconnect starts a new host session; it is not proof of a new physical observer boot.

Malformed lines increment parser errors. The complete RS1 marker resynchronizes after a truncated frame, and replay checks incomplete tails at EOF. CRC-valid impossible ownership records are rejected too. There is no resend or acknowledgment, so a lost record remains lost. Without a persistent boot identity, certain observer resets that produce plausible forward counters and timestamps cannot be distinguished from continuity. Preserve this limitation in any report; compatibility and freshness are not authentication.

## Build, validation, and bench workflow

From this directory run `make -C firmware clean all test`. The build needs GNU Arm embedded GCC, binutils, newlib, a host GCC, and make. It emits `firmware/build/read-share.elf` and `.bin`. Host tests exercise the real engine, encoder, scheduler, and transmitter with synthetic inputs or register models. They do not execute the binary on silicon. Use the Nucleo's documented SWD/ST-LINK procedure to program only the owned observer after reviewing the image and board identity. Recovery is by local SWD reprogramming, not an implemented over-the-air updater.

Run `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v` for the application and real Tk event-loop tests. They use Linux pseudo-terminals and the production C encoder, not a fabricated API response. Run `/usr/bin/python3 tools/check_hardware.py` with KiCad 9 Python bindings and CLI to check ERC, DRC, and exact schematic/PCB/manifest agreement. `tools/generate_hardware.py` regenerates the authored carrier. `python3 tools/check_release.py` checks documentation length, substantive production C size, attribution, required files, and whitespace.

Before a real run, review the target adapter, establish an unowned baseline, inspect voltage and ground compatibility, verify harness continuity, and start with harmless slow events. Exercise a lone reader, overlapping readers, reader leave/rejoin, last-reader release, and a lone writer. Then use an authorized diagnostic fixture to represent reader/writer incompatibility without creating a dangerous target condition. Compare independent waveform evidence with retained counters. Stop after any acquisition or interface fault and establish a new baseline rather than continuing an apparently healthy graph.

Troubleshoot missing data by checking serial permissions, port selection, reset, TX routing, and periodic policy reception. A permanently idle trace can mean missing instrumentation or a broken wire, not successful verification. Repeated interface faults suggest inadequate setup, short pulses, overlapping strobes, or ambiguous mapping. A host freeze calls for preserving the report and investigating continuity; repeatedly reconnecting until a clean display appears destroys useful context. Keep the original evidence and label every simulated fixture as synthetic.

## Remaining engineering work

Fabrication review, purchased-part verification, enclosure design, MCU programming, electrical bring-up, timing calibration, watchdog timing, current consumption, brownout behavior, ESD, EMC, and physical USB interoperability remain unverified. Passive-only ERC cannot validate the external power source. A routed board and a linked image are engineering artifacts, not a production qualification. The complete measurement additionally depends on a correct owner-built adapter and independently established baseline. Treat findings as bounded diagnostic evidence, never as automatic authorization, enforcement, safety certification, or proof of absence of a vulnerability.
