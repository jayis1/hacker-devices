# Round Cover — Unordered Coverage-Gated Resource Renewal Witness

Author and creator: jayis1. Hardware revision A. Wire protocol RC1, policy revision 1.
Firmware, companion and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. Installed tools and standard component libraries retain their existing licenses and authorship.

Status: unbuilt engineering prototype. This directory contains target-linked firmware, a working Linux desktop companion and routed native KiCad sources. Executed checks are recorded in VERIFICATION.md. No fabrication, MCU programming, measured electrical performance, timing calibration or physical target interoperability is claimed. The device observes a declared policy; it is not an allocator, protective interlock, security authenticator or certification instrument.

## Purpose and practical value

Round Cover detects premature renewal in a four-identity resource schedule. Each identity must receive exactly one accepted allocation in an unordered coverage round before any identity can receive another allocation. Allocations within a round may occur in any permutation. Each resource also has ordinary exclusive occupancy, and releases may occur in any order. Crucially, releasing a resource does not remove its membership from an incomplete coverage round. Neither draining the pool nor waiting quietly renews that membership.

The motivating laboratory application is a gateway that distributes diagnostic work across four security-monitoring partitions. The scheduler promises one accepted visit to each partition before starting the next sweep, but completion times vary and work can overlap. A faulty scheduler may clear its visited bit when a fast partition completes, repeatedly serve that partition and omit a slower participant. Ordinary occupancy checks see no double allocation. A fixed cyclic-order checker would reject valid shuffled sweeps. Round Cover instead checks the actual unordered, no-repeat-until-covered promise.

A second application is testing a four-bank background integrity scanner in an expendable emulator. Tags identify four stable scan banks. ALLOCATE means the scanner accepted a visit, not that it found or repaired anything. RELEASE means that visit relinquished its bank. The observer can show that the scanner begins a third visit to one bank while another bank has received only one visit. It cannot prove that scanning occurred, inspect memory, or establish that a bank is free of malicious content. Those questions need independent functional evidence.

The policy deliberately favors coverage over throughput. A busy identity that is still owed its next visit remains part of the obligation. If all currently free identities already participated in the current round, there is no eligible allocation until the busy laggard releases and receives its owed visit. This can look like unnecessary backpressure in a general-purpose allocator. Use this instrument only when the target explicitly promises coverage-gated renewal; it is not an appropriate universal fairness oracle.

## Exact policy and distinguishing examples

All four resources begin free, with zero observed admissions. A valid allocation selects a free identity whose cumulative admission count equals the minimum count across all four identities, including occupied identities. Its count then increases by one. A valid release selects an occupied identity, clears occupancy and leaves every admission count unchanged. Every accepted snapshot consequently has a maximum/minimum count difference of at most one. The minimum is the number of completed coverage rounds. Identities above that minimum have already participated in the current partial round.

For example, allocate 2, release 2, allocate 0, release 0 leaves counts [1,0,1,0]. Either 1 or 3 may be allocated next. Another allocation of 0 is a coverage violation even though it is free and the pool is empty. After allocating both missing identities, a new round begins without requiring a collective release signal. The new round may use a different permutation. Resources still occupied retain exclusivity and must be released before reuse.

A useful discriminating trace starts by allocating all four resources. Keep resource 3 occupied. Release and reallocate each of 0, 1 and 2, then release them again. Counts are now [2,2,2,1], and only 3 is occupied. No free resource is eligible: choosing any of 0, 1 or 2 would begin its next round early. Releasing 3 and allocating it completes coverage and makes free identities eligible again. This trace separates mandatory coverage from work-conserving least-used-free selection.

Coverage rounds and occupancy episodes are different concepts. The protocol field named `rounds` is inherited platform terminology for completed nonempty occupancy episodes, ending whenever the pool drains. It is not the coverage-round count. The companion explicitly derives and labels completed coverage rounds from the minimum per-tag count. A pool can drain several times within one coverage round, or stay continuously occupied while several coverage rounds finish. Never substitute one counter for the other in a report.

## Originality and disclosed reuse

The synchronized jayis1/hacker-devices repository was inventoried before implementation, including its device directories and README titles. The closest purpose and measurement contracts were inspected: Use Quota, Wear Balance, Barrier Mask and Baton Order. Use Quota applies unequal, fixed, nonrefundable lifetime limits. Wear Balance compares only free resources and intentionally permits global count imbalance when laggards are busy. Barrier Mask accumulates completion identities before one collective release. Baton Order requires a fixed cyclic sequence of level-indicated owners.

Round Cover instead observes arbitrary per-resource lifetimes with a renewable, order-free coverage obligation that includes busy participants. It has no fixed cyclic order, no collective barrier-release strobe, no fixed lifetime quota and no busy-resource exemption. Its contribution is this measurement contract, the busy-laggard discrimination, the count/set equivalence, corresponding desktop interpretation and independent coverage-set tests. Originality is relative to the reviewed repository, not a worldwide prior-art search or a claim that coverage scheduling is a newly invented algorithm.

The passive carrier topology, MCU startup, register definitions, GPIO acquisition, CRC framing, UART driver and desktop transport reuse existing jayis1-authored Use Quota infrastructure. This reuse is intentional and is not represented as new electrical invention. Inherited `UQ_` C identifiers remain internal implementation names; they do not identify the wire protocol or permit interoperability with Use Quota. The directory is self-contained, with no runtime imports from sibling devices and no shared hardware tree dependency.

## Attack surface, threat model and authorized use

Use only on equipment you own or have explicit written authorization to assess. Scope should identify the diagnostic adapter, target image, wiring points, resource mapping, accepted workload, operator, stop conditions and evidence-retention policy. Begin with an emulator and harmless work items. Do not attach this unisolated design to deployed access controls, medical equipment, vehicles, machinery with hazardous outputs, mains-related interfaces or unknown industrial wiring. Disconnect and investigate if attachment changes target behavior, logic levels or temperature.

The relevant attack surface is the scheduler's renewal bookkeeping. Candidate defects include clearing a visit flag at release, treating an empty pool as a completed sweep, comparing only free candidates when the specification requires all identities, losing the visited set after an unrelated operation, or attributing one partition's admission to another partition. An authorized red team can use a harmless owner-approved workload to expose a scheduling-policy deviation. A penetration tester can repeat it after remediation. No exploit execution, credential collection or unauthorized access is required.

The diagnostic adapter is trusted and must report actual accepted operations, not requests that were rejected by the target. Each tag identifies one stable resource, not a hash collision, a resource category or an identity reassigned during capture. Every allocation and release must be reported in order using the timing contract below. The observer cannot detect intentionally omitted or fabricated operations. A compromised target can lie on every input, and a compromised workstation can modify exports. A coverage finding is evidence about indications, not proof of missing security checks or exploitability.

The supplied firmware keeps target pins configured as inputs and disables USART reception. There is no target command path, radio, network service, remote update endpoint, hidden storage or automatic target discovery. Receive-only firmware is not an irreversible hardware diode: physical programming access can replace it with an image that drives GPIOs. Protect the module, workstation, harness and ST-LINK connection. CRC detects accidental corruption, not forgery. There is no secure boot, authenticated pairing, signed evidence, persistent boot identity, encrypted export or trusted clock.

Start the observer only after independently establishing an empty target pool at the beginning of a fresh coverage campaign. Quiet strobes do not establish that premise. Resetting the observer destroys volatile history and must not be used to excuse a target's unfinished round. Reconnecting the app does not clear MCU history, but begins new host provenance. An observer reset or missed acquisition requires a new, explicitly baselined experiment. Preserve prior evidence before restarting.

## Hardware specifications and bill of materials

The processor is an external ST NUCLEO-F401RE module containing STM32F401RET6, a Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware enters from reset and uses the nominal 16 MHz HSI oscillator, a nominal one-millisecond SysTick and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. PA2 reaches the module's ST-LINK virtual COM bridge. The purchased development module provides regulation, decoupling, reset, SWD programming and USB connectivity; these functions are not imaginary components on the passive carrier.

The custom PCB is an 80 by 70 mm, two-copper-layer, nominal 1.6 mm carrier. It has fourteen through-hole components, thirty connected electrical pads and nine named nets. Tracks are 0.3 mm wide. It is harness-connected, not a stacking shield. There are no radios, antennas, FPGA, batteries, chargers, active sensors, storage cards or target power outputs. No enclosure model or mounting holes are provided; use insulated supports, underside protection and strain relief.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External processor module | 1 | ST NUCLEO-F401RE, purchased revision checked before assembly |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness and supports | 1 set | Short labelled insulated leads and nonconductive bench support |

Resistor footprints use DIN0207 bodies and 7.62 mm pitch. Verify purchased body dimensions, lead diameter, drill fit, mating header geometry and connector orientation before ordering. Component stock and pricing have not been checked. Provision a suitable USB source for the purchased module; a 5 V, 500 mA planning allowance is not measured consumption or a certification claim. The carrier has no independent supply rail and supplies no target power.

| Signal | Target signal pin | MCU header | Nucleo connector / GPIO | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Pin two on every target header is ground. Both J1 contacts are ground only. J2 has four signals and no ground or supply pin, so a separate Nucleo ground lead to J1 is mandatory. Each `_IN` node passes through 1 kOhm to the MCU node, with a 100 kOhm pulldown at that node. Only known common-ground, push-pull zero-to-3.3 V diagnostics are supported. Five-volt signals, negative voltage, 1.8 V logic, differential wiring and unknown field interfaces are excluded.

The resistors are not an isolation barrier, hot-plug guarantee, surge protector or ESD qualification. USB ground becomes common with the target. Power the observer before enabling target signals and disable them before depowering it to reduce back-power risk. Review source impedance, ground offset, leakage, input thresholds and lead capacitance on the assembled fixture. The nominal static resistance path is 101 kOhm before MCU leakage; that fact does not establish dynamic loading or electrical safety.

Before assembly consult ST UM1724, Table 16 and section 7.10, the STM32F401xD/xE datasheet and RM0368. Verify the actual module revision, CN8 mapping, power jumpers and virtual-COM solder bridges. The inherited pin map must be checked against the purchased board, not merely copied into a harness. Manufacturer manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . CAD checks cover the passive carrier, not the configuration or safety of that external module.

## Architecture and block diagram

```text
Owned diagnostic adapter          Passive carrier           External Nucleo
TAG0 ------------------------------ 1k --+------------------ PA0
TAG1 ------------------------------ 1k --+------------------ PA1
ALLOCATE -------------------------- 1k --+------------------ PA4
RELEASE --------------------------- 1k --+------------------ PB0
                                         | each MCU node
                                        100k
                                         |
Target ground ---------------------------+---- J1 ---------- module GND

GPIO samples -> continuity guard -> bundled tag / independent strobes
 -> prior setup check -> exclusive occupancy + all-identity coverage frontier
 -> transactional evidence -> RC1 checked encoder -> USART2 TX -> ST-LINK USB
 -> Linux Ledger / History / Integrity -> explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not atomically latched across ports. This is a slow diagnostic observer, not a high-speed bus analyzer or race-condition oscilloscope. A suitable adapter stretches already accepted operations without changing their order. If instrumentation changes the scheduler behavior being investigated, the result does not generalize to the uninstrumented target.

## Firmware design and timing contract

The firmware is C11 without an RTOS, heap, filesystem or downloaded SDK. Startup initializes data and BSS and installs the core vector table. The linker places flash at 0x08000000 and SRAM at 0x20000000 with stack headroom. Drivers configure GPIO, SysTick, USART, reset-cause capture and the independent watchdog. The UART has a bounded 768-byte transmit buffer, refuses concurrent submissions and performs at most one byte write per poll. A hundred-millisecond transmission timeout abandons the pending frame and increments a saturating diagnostic.

The two-bit tag qualifies as one bundle to avoid synthesizing a tag from differently delayed bits. ALLOCATE and RELEASE qualify independently so chatter on one cannot indefinitely conceal the other. A candidate needs its initial observation and three matching intervals. An operation additionally requires five prior stable tag observations, raw/qualified tag agreement at its endpoint, a still-high raw selected strobe and the opposite strobe low in prior qualified, current qualified and raw states. Two simultaneous rising strobes are ambiguous and never assigned an arbitrary order.

For the conservative fixture used by tests, present the tag with both strobes low for twelve nominal milliseconds, assert one strobe for six milliseconds, then hold both low for six milliseconds. Held strobes generate only one event. Short pulses can be filtered or missed and are outside the supported interface contract. A quiet qualified baseline starts tracking, but the operator must independently establish empty occupancy and a fresh campaign. There is no hardware acknowledgment to pace the adapter.

Interface validation precedes resource semantics. Allocation first rejects an already occupied identity, then checks the all-identity coverage frontier. Release rejects an unoccupied identity. Fault bits are early renewal 1, orphan release 2, duplicate allocation 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog startup failure 64. A semantic rejection retains the last accepted occupancy and counts; it does not imply that the target refused the operation that triggered it.

Each acquired observation is evaluated on a speculative state copy. If any increment would overflow, that sample is discarded and the prior coherent state is halted. This prevents partially updated occupancy or coverage. Repeated timestamps do nothing; ordinary unsigned timer rollover is supported. Skipping a sample halts correlation instead of treating unseen time as stable. Any policy or continuity fault is terminal. After halt, time and transport diagnostics may advance, but accepted evidence stays frozen. The watchdog continues to be fed after a policy fault so the finding is not deliberately rebooted away.

## Companion application and wire interface

Run `python3 app/console.py` on Linux with Python Tk support. The application uses only the Python standard library and needs no package download, cloud account or network service. Enter a verified serial path, preferably `/dev/serial/by-id/`, and choose New live. The application does not probe unknown devices. A second connection is refused while a port remains owned. Disconnect restores terminal attributes. Failed opens preserve existing evidence rather than replacing it with an empty session.

Ledger displays active identities, cumulative uses, completed coverage rounds, current covered membership, eligible free tags and decoded findings. Eligibility is an explanation of retained observer state, never authorization to perform an operation. History retains up to 256 accepted snapshots. Integrity records source, live/replay provenance, connection state, reset hints, parser errors, freshness and frozen-session reasons. Replay accepts regular files of at most two MiB, rejects devices and FIFOs, and counts an incomplete final frame. Export creates a new mode-0600 JSON file, refuses overwrite and final-path symlinks, and includes metadata even without completed work. Reports credit jayis1.

RC1 records are ASCII `RC1,type,decimal-fields*CCCC` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, using initial value 0xffff and polynomial 0x1021. B contains reset flags once at boot. P repeats `[3,1,5,4,1]`: qualification intervals, nominal sample period, prior tag setup, resource count and coverage policy revision. P repeats each second for late attachment without clearing history. S is scheduled every hundred milliseconds when transport is available and contains these fields in exact order:

```text
now levels valid halted faults phase active depth stable
observed rounds attempts launched completed rejected gaps missed
queued peak use0 use1 use2 use3 tx_fail
```

Encoder and companion check unsigned bounds, masks, depth, peak, count spread and conservation. Admissions equal releases plus active depth and the sum of all per-tag use counts. Attempts equal admissions plus releases plus rejected edges. Closed occupancy episodes plus the nonempty indicator plus overlapping admissions equal admissions. Per-tag histories cannot decrease or migrate across sparse records. Necessary transition checks constrain membership changes, counter growth and peak changes; they are not a reconstruction of every hidden operation order.

Freshness requires advancing device time, compatible policy and a live connection, not merely receipt of repeated bytes. Contradictory duplicate timestamps, counter rollback, changes to halted evidence, reception pauses or divergent host/device progress freeze interpretation. Drift allowance is bounded rather than growing without limit. CRC-valid but impossible snapshots are rejected. Some observer resets remain indistinguishable when boot information is lost because no persistent boot identity exists. Replay and synthetic fixtures are never represented as physical measurements.

## Build, verification and bench acceptance

From this directory run `make -C firmware all test` using GNU Make, host GCC and Arm bare-metal GCC with newlib/nosys support. The target build selects Cortex-M4 Thumb soft-float, C11 and warnings as errors. Host engine tests run under AddressSanitizer and UndefinedBehaviorSanitizer. The independent oracle uses a coverage set cleared on its fourth distinct admission, rather than duplicating the engine's minimum-count representation. Tests explore repeated operation words, arbitrary permutations across rounds, arbitrary release order, busy-laggard blocking, rollover, setup faults, gaps, counter exhaustion and immutable halted evidence.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v` for desktop and protocol tests. These execute real C-encoder records through the parser and Tk interface, including serial pseudo-terminals, all screens, disconnect/reconnect, hangup, replay, private export and overwrite refusal. The pseudo-terminal transport is synthetic and does not establish USB hardware interoperability. Application syntax can also be checked with `/usr/bin/python3 -m compileall -q app`.

With KiCad 9 Python bindings and standard footprints installed, regenerate intentionally using `/usr/bin/python3 tools/generate_hardware.py`, then run `/usr/bin/python3 tools/check_hardware.py`. The generator produces authored symbols, wires, labels, actual footprint pads, routed tracks, a closed outline, project settings, a connectivity manifest and a schematic netlist. The checker runs ERC/DRC and compares every reference, value, footprint identity and pin/net mapping across schematic, PCB, saved netlist and manifest. Inspect a rendered schematic as well. Run `python3 tools/check_release.py` for content, attribution and whitespace gates.

Before physical testing, inspect unpowered resistance, shorts, header orientation and ground continuity. Program the target image using the purchased module's normal ST-LINK workflow only after reviewing power and reset configuration. First verify idle RC1 policy and status with target outputs disabled. Exercise a single partial round, premature reuse, complete shuffled rounds and the busy-laggard trace on an expendable fixture. Compare waveforms with evidence, measure input loading and calibrate time before any broader engagement. Do not infer physical validation from a successful linker or a clean CAD report.

## Evidence, limitations and reporting

A useful research report records target and adapter revisions, resource mapping, fresh-campaign procedure, observer image identity, host provenance and the first contradiction. Preserve the counts and occupancy at halt and describe the violating target admission separately from the observer's retained accepted state. Compare the same supervised workload before and after remediation. A passing campaign demonstrates only that the observed indication sequence met this narrow contract; it says nothing about unobserved operations, actual scan quality, authorization or successful cleanup.

Minimize saved captures and retain them only under engagement policy. No user payloads are needed, but resource schedules can still reveal operational timing. JSON exports are neither encrypted nor signed. Ordinary deletion does not guarantee secure erasure on flash storage. Outstanding work includes fabrication, electrical loading, MCU programming, real reset/watchdog behavior, timing calibration, ESD review, enclosure design and physical interoperability. This remains a visible, supervised, authorized laboratory prototype rather than a deployment-ready product.
