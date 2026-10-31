# Weight Ceiling — Weighted Live-Resource Budget Witness

Author and creator: jayis1. Hardware revision A. Protocol WC1.
Firmware, application, and tests: GPL-2.0-only. Hardware generation and checking utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. These identifiers cover original project material; installed development tools and component libraries retain their upstream licenses.

Status: unbuilt engineering prototype with target-linked C firmware, an executable Linux desktop application, and native routed KiCad carrier files. See VERIFICATION.md for checks actually executed. Physical fabrication, programming, electrical qualification, oscillator calibration, and target interoperability remain unverified. This is an observation instrument, not a resource controller, safety interlock, intrusion appliance, or compliance certification.

## Purpose and overview

Weight Ceiling observes a security-relevant resource contract that ordinary occupancy counters cannot express: every occupied resource has its own fixed cost, and the sum of live costs must remain within a shared budget. Four unique resource tags have costs one, two, three, and four. The budget is five. Allocating tags zero and three is permitted because their costs sum to five. Allocating tags two and three is a violation because their costs sum to seven, even though both cases have only two occupied resources. Releases return exactly the released resource's cost; elapsed time never restores capacity.

A motivating target is an embedded gateway with several privileged processing descriptors. A descriptor may reserve a different number of protected scratch pages, authenticated processing lanes, or bounded hardware work credits. If the allocator checks only the number of outstanding descriptors, it can overcommit the protected pool while every individual descriptor identity remains unique. A target-side integer underflow, premature refund, or mistaken cost table can similarly admit a combination that should be impossible. The independent observer checks the externally declared allocation and release boundary without collecting buffer contents or invoking operations.

This design is intentionally small and auditable. TAG0 and TAG1 encode one of four actual resource identities. ALLOCATE rises when the target has accepted an allocation. RELEASE rises when that exact resource ceases to consume its reservation. Each resource can be outstanding at most once. Both the identity set and weighted load are retained, so a budget finding can be explained as a particular combination rather than a bare high-water counter. The first violation halts interpretation and preserves the last accepted occupancy for investigation.

Consider a valid rolling sequence: allocate tag three, allocate tag zero, release tag three, allocate tag two, release tag zero. Loads are four, five, one, four, and three. The pool need not empty between admissions, and release order need not match allocation order. A subsequent allocation of tag three is rejected because three plus four exceeds five. The observer retains tag two as occupied and reports a budget fault. It does not pretend the target refused the operation: rejection here means rejection by the observer's model of an allegedly accepted target operation.

## Originality and reuse boundaries

Before implementation, the configured repository was pulled, device directories and available README titles were inventoried, and nearby measurement contracts were read. A repository-wide README search found no weighted-live-budget implementation. Slot Alias tracks unique resource occupancy but imposes no unequal-cost sum. Burst Budget replenishes temporal admission credit, whereas Weight Ceiling refunds capacity only on a named release. Grant Ledger consumes a single authorization grant. Split Approval associates actors with proposals and checks separation of duties. None is a substitute for weighted live reservation accounting.

The distinct contribution is the fixed unequal-cost occupancy contract, exact-ceiling handling, identity-dependent refund, budget-fault precedence, weighted companion interpretation, and independent array-oracle regressions. Originality is relative to this repository, not a claim of worldwide invention, algorithmic novelty, patentability, or a literature review. The passive carrier geometry, register-level board support, startup, bounded UART, CRC implementation, and Linux application transport deliberately reuse jayis1-authored infrastructure. A repeated electrical platform is useful for comparing different diagnostic contracts; it is not presented as a novel analog circuit.

This folder is self-contained. Building firmware, running tests, launching the app, and regenerating CAD do not import other device folders. No unified hardware tree or shared command framework is required. All changes in this release belong to this device directory. The complete instrument is the carrier plus the external processor module and workstation, not an unsupported claim that the passive PCB contains a processor.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Permission must cover opening equipment, diagnostic firmware changes, attachment points, operating conditions, interruptions, and evidence retention. Begin on a low-energy development fixture with hazardous outputs disconnected. Do not attach this unisolated prototype to production access controls, medical equipment, road vehicles, industrial machinery, mains-related circuits, or unknown field wiring. Stop immediately if attachment changes target behavior, causes heating, or alters voltage levels.

The assessed attack surface is resource-accounting integration at an embedded privilege boundary. Relevant defects include using slot count instead of cost, refunding the wrong resource weight, admitting a resource before a preceding reservation actually retires, duplicate allocation, and release of a vacant tag. An authorized red team can use an owner-approved workload to demonstrate a suspected boundary error. A penetration tester can compare the same declared sequence before and after a remediation. A researcher can study scheduling choices that have identical occupancy counts but different aggregate exposure.

The diagnostic adapter is part of the trusted experiment. It must export accepted operations rather than requests that the target correctly rejects. Tags must name four unique resources with the declared fixed weights, not hashes of many resources or categories allowing multiple simultaneous members. A cost is an experiment-defined integer reservation, not measured current, memory usage, risk, or cryptographic strength. Changes in resource cost during occupancy are unsupported. The instrument cannot infer omitted allocations or verify whether an asserted release corresponds to actual hardware retirement.

Trusted elements include the operator, wiring, installed image, processor module, programming interface, and workstation. Potentially untrusted inputs include target signals, USB serial bytes, and replay files. CRC detects accidental corruption; it does not authenticate a device or prove evidence integrity. There is no signed bootloader, protected update service, pairing secret, encrypted export, persistent boot identity, network listener, radio, credential collection, exploit payload, or target command parser. A malicious target can lie on all four wires. A malicious workstation can forge reports. Independent waveform capture and target source review remain necessary.

The firmware never configures target-facing pins as outputs. Only the host-facing USART transmitter drives data toward ST-LINK. Receive-only is a property of this supplied image, not an irreversible hardware diode: physical programming access can replace it. Even passive observation can disturb a target through loading, shared ground, incorrect sequencing, or reversed wiring. A clean run is not proof of production security, freedom from denial of service, or correct reservations outside the observed interface.

## Hardware specifications and bill of materials

The external MCU board is ST NUCLEO-F401RE with STM32F401RET6 Cortex-M4, 512 KiB flash, and 96 KiB SRAM. Firmware uses nominal reset-default 16 MHz HSI, a nominal one-millisecond SysTick, and transmit-only USART2 at 115200 baud, eight data bits, no parity, one stop bit. PA2 carries telemetry toward the module's ST-LINK virtual serial bridge. USB provides the normal development-board power and programming path. No vendor SDK download, RTOS, battery, charger, radio, FPGA, analog sensor, storage card, or wireless backhaul is required.

The Nucleo supplies the processor, regulator, decoupling, reset network, programming interface, and USB bridge. The custom PCB is an 80 by 70 mm, two-layer passive carrier with nominal 1.6 mm thickness, 0.3 mm traces, fourteen through-hole components, thirty connected electrical pads, and nine nets. It connects using short labelled wires; it is not a stacking shield. No mounting holes or enclosure CAD are supplied. Use an insulating support, protective cover, and strain relief, and verify that connector access remains practical after assembly.

| Part | Quantity | Specification |
| --- | --- | --- |
| External processor module | 1 | ST NUCLEO-F401RE, purchased revision verified |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2, vertical, 2.54 mm pitch header |
| J2 | 1 | 1x4, vertical, 2.54 mm pitch header |
| Harness and support | 1 set | Insulated short leads, labels, nonconductive base |

Resistor footprints are DIN0207 with 7.62 mm lead spacing. Confirm body dimensions, lead diameter, purchased header geometry, drill fit, and mating orientation before fabrication. Pricing, stock availability, physical assembly yield, and measured power consumption are not established. A conservative USB source-planning allowance is 5 V at 500 mA, subject to the Nucleo manufacturer's actual power configuration, not a measured draw or USB compliance assertion.

| Signal | Target header | MCU-side J2 | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | 1 | CN8.1 A0 / PA0 | 0 |
| TAG1 | J4.1 | 2 | CN8.2 A1 / PA1 | 1 |
| ALLOCATE | J5.1 | 3 | CN8.3 A2 / PA4 | 2 |
| RELEASE | J6.1 | 4 | CN8.4 A3 / PB0 | 3 |

Pin two of J3 through J6 is ground. Both J1 pins are ground only, never supply inputs. J2 has four signals and no ground or supply contact. A separate lead joins Nucleo ground to J1. Nets ending `_IN` are target-side nodes; unsuffixed nets are MCU-side nodes. Each signal passes through 1 kOhm to a node with a 100 kOhm pulldown. The nominal resistive path to ground is 101 kOhm before target impedance and MCU leakage. Verify actual thresholds, loading, cable capacitance, and noise margin on the bench.

Supported inputs are known common-ground zero-to-3.3 V push-pull diagnostic outputs only. Exclude 5 V TTL, negative voltage, 1.8 V logic, differential buses, coils, unknown industrial interfaces, and isolated grounds that must remain isolated. Series resistors are not an overvoltage safety barrier. There is no dedicated ESD clamp, fuse, isolation, abuse protection, or guaranteed hot-plug behavior. A pulldown cannot distinguish a disconnected lead from a legitimate low state.

Check the purchased module's header map, power jumpers, and virtual-COM solder bridges against ST UM1724 before connection. Consult RM0368 and the STM32F401xD/xE datasheet for register and electrical limits. The inherited register map is explicit in registers.h, not an invented abstraction. Power the observer before enabling target outputs, and remove those outputs before depowering it to avoid back-power through input structures. USB and target grounds become common. Physical compatibility is still a release-to-lab gate even when schematic and software checks pass.

## Architecture and block diagram

```text
Owned diagnostic fixture          Passive carrier             External Nucleo
TAG0 ----------------------------- 1k --+----------------------- PA0
TAG1 ----------------------------- 1k --+----------------------- PA1
ALLOCATE ------------------------- 1k --+----------------------- PA4
RELEASE -------------------------- 1k --+----------------------- PB0
                                        | each MCU node
                                       100k
                                        |
Target ground ------------------------- GND ------------------- USB ground

GPIO -> continuity guard -> bundled tag / independent strobe qualification
     -> prior setup check -> unique occupancy set -> weighted ceiling check
     -> bounded WC1 encoder -> USART2 TX -> ST-LINK USB serial
     -> Linux Ledger / History / Integrity -> explicit private JSON export
```

GPIOA and GPIOB are read sequentially, not atomically. The device is a deliberately slow diagnostic contract observer, not a high-speed logic analyzer. Initial fixture waveforms should provide twenty milliseconds of tag setup, twenty milliseconds of strobe high, and twenty milliseconds with both strobes low before the next operation. Those are conservative starting margins, not calibrated specifications. A scope must establish port-read skew, actual acquisition timing, oscillator tolerance, and pulse visibility. Short or poorly timed events may be filtered or missed.

## Firmware behavior and design decisions

Before starting the observer, independently establish that all four target resources are vacant. Quiet strobes alone cannot establish that fact. WAIT becomes TRACK only after all inputs qualify and both raw and qualified strobes are low. Pre-baseline activity is excluded from coverage. Later desktop attachment is supported by periodic policy announcements and retained status, but restarting the MCU while unknown target reservations remain outstanding is not a valid new baseline.

TAG0 and TAG1 qualify as one bundle, preventing independent-bit settling from manufacturing a tag not actually held by the fixture. Each strobe qualifies independently. A candidate requires three subsequent matching sample intervals after its first observation. Five prior stable tag observations are required at an operation endpoint; setup is not credited on that endpoint itself. Raw tag must match qualified tag. The opposite strobe must be low in the previous qualified, current qualified, and current raw observations. Two simultaneous rising strobes reject both operation edges as interface ambiguity.

An allocation first checks duplicate membership, then calculates existing load plus tag plus one. Duplicate allocation has precedence over budget excess because the resource is already occupied. If the proposed load exceeds five, the allocation is rejected and the accepted occupancy remains unchanged. Exactly five is allowed. A release checks membership, rejects a vacant tag as orphan release, otherwise removes the tag and subtracts precisely its fixed weight. Release order is unrestricted. A released resource may be allocated again while another remains live. There is no timed refill, priority scheme, FIFO retirement, generation check, cancellation operation, or dynamically configurable cost table.

The entire sample update occurs on a speculative copy of the ledger. Counter exhaustion discards that update and freezes the prior accepted accounting, avoiding partial admission or refund. A missing acquisition tick also halts permanently rather than treating unseen activity as idle. Repeated calls at the same timestamp are ignored. Ordinary unsigned timestamp wrap is supported, but a stall spanning a complete modular clock cycle cannot be reconstructed. On halt, only time and the saturating UART-failure diagnostic may advance. Export evidence, reestablish an empty target baseline, and restart deliberately.

Several invariants make the record auditable. Accepted allocations equal accepted releases plus occupied population. Attempts equal allocations plus releases plus rejected edges. Accepted allocations also equal closed nonempty episodes plus one if any resource is active plus admissions made while a peer was already active. Current load is exactly the sum of weights in the active mask. Peak load is a historical maximum, not peak resource count. These checks constrain evidence consistency but cannot establish the truth of the target adapter.

The main loop services GPIO acquisition, bounded UART polling, policy announcements, and watchdog feeding without waiting for a whole packet to transmit. Status is attempted every hundred nominal milliseconds; policy repeats about every second. A copied UART packet expires after a hundred milliseconds if it cannot drain. Timeout and rejected submission increase tx_fail. The independent watchdog has bounded startup waits; startup failure produces a terminal reason. Physical watchdog timing and recovery are not established by host register-model tests.

## Application interface and protocol

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk. The application needs no third-party Python packages. Choose the exact virtual serial path and select New live. Ledger shows active tag/weight pairs, current load, historical peak, counters, and findings. History retains up to 256 accepted snapshots. Integrity shows source, policy compatibility, parser errors, reset metadata, freeze reason, and live freshness. Disconnect preserves evidence but removes live status. A second connection is refused until the first is closed.

Frames are ASCII `WC1,<kind>,<decimal fields>*<four uppercase CRC digits>` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, with polynomial 0x1021, initial 0xffff, no reflection, and no final xor. B contains reset flags. P contains `3,1,5,5`: qualification intervals, nominal sample milliseconds, prior setup observations, and weighted ceiling. The protocol version fixes four resource tags and cost tag plus one. Periodic P does not reset the evidence; B is a one-shot boot announcement.

S fields are `now levels valid halted faults phase active load stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail`. Active is the four-bit occupancy mask. Launched counts accepted allocations, completed accepted releases, rounds closed nonempty episodes, and queued allocations while any other resource was active; queued does not describe a FIFO. Fault bits are 1 budget excess, 2 orphan release, 4 duplicate allocation, 8 interface ambiguity, 16 acquisition gap, 32 counter exhaustion, and 64 watchdog startup failure. Only the first terminal reason is retained.

The desktop requires policy compatibility and two advancing timestamps before labeling live progress. Duplicate timestamps do not renew freshness. Clock or counter rollback, disappearing sticky faults, contradictory duplicates, modified halted evidence, later boot announcements, prolonged reception pauses, and excessive host/device drift freeze interpretation. The drift allowance is bounded, so a gradually growing serial backlog cannot remain fresh indefinitely. Uncalibrated oscillator drift may conservatively require reconnecting. Some resets with a lost boot frame remain indistinguishable without persistent boot identity.

Single-frame validation checks weighted sums and conservation. Sparse transition validation requires enough allocations and releases to explain changed membership and bounds peak growth by newly admitted credits. It also rejects empty-to-empty hidden admissions without episode closure. These are necessary bounds, not exhaustive reconstruction of hidden operation histories. The per-edge engine enforces the actual budget. Sparse status cannot independently prove that every intermediate combination was valid or recover every rejected input waveform.

Replay is explicitly non-live. It accepts regular files up to two MiB, rejects FIFOs and device files, bounds partial lines, resynchronizes on a complete WC1 marker, and counts incomplete EOF tails. Export creates a new mode-0600 JSON file and refuses overwrite or final-component symlinks. Exports contain jayis1 authorship, source, origin, connectivity, freshness, reset details, errors, retained state, and bounded history. Files are unencrypted and unsigned. Use a trusted parent directory, an agreed retention period, and normal workstation controls; delete exports and laboratory fixtures when the authorization ends.

## Reproduction and physical acceptance

From this directory, run `make -C firmware all test`. Requirements are GNU make, host GCC with address and undefined-behavior sanitizers, and arm-none-eabi GCC with newlib/nosys. The target links startup vectors, register drivers, engine, encoder, and linker script into `firmware/build/weight-ceiling.elf`, `.bin`, and `.map`. No test flashes hardware. Run `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v` for app tests and `python3 -m compileall -q app` for byte compilation.

The independent C oracle uses an occupied boolean array and explicit cost table rather than the firmware's membership implementation. It explores allocations and releases through repeated reuse, covering both permitted and prohibited unequal-weight combinations. Dedicated regressions check the exact ceiling, wrong identity lifecycle, duplicate precedence, tied strobes, short pulses, setup violation, chatter, gaps, timestamp wrap, terminal freezing, and transactional counter exhaustion. A synthetic C fixture passes actual encoder output through the real companion parser and GUI replay path. PTY tests exercise connection, screen contents, disconnect, terminal restoration, and export. These are software fixtures, not physical measurements.

Regenerate native CAD with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9 and matching pcbnew bindings. Run `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC, a fresh XML netlist, and exact comparison of references, values, footprints, pins, and nets against PCB pads and the manifest. Passive-only ERC does not validate the external module's power source or target voltage. Run `python3 tools/check_release.py` for README length, substantive production C lines, attribution, and whitespace gates. Build outputs and local KiCad preferences are ignored.

Before fabrication, inspect every connector orientation, resistor footprint, routing path, outline, label, and mechanical support requirement. Before powered use, measure ground compatibility, voltage levels, loading, supply sequence, and GPIO waveforms. Establish an empty baseline and exercise all singleton tags, both allocation orders of the exact-ceiling pairs zero/three and one/two, both release orders, rolling reuse, a prohibited heavy pair, duplicate allocation, and orphan release. Rebaseline between terminal cases. Correlate retained records with independent waveform capture and the target's actual reservation state. Flashing, sustained serial load, electrical qualification, ESD behavior, calibration, and real workload interoperability remain laboratory acceptance gates; this release makes no fabrication-ready or production-qualified claim.
