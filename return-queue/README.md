# Return Queue — First-Return-First Resource Recycler Witness

Author and creator: jayis1. Hardware revision A. Protocol RQ1, policy revision 1.

Status: unbuilt engineering prototype. The supplied image has been compiled and linked for the target MCU, the application has been exercised with synthetic records and Linux pseudo-terminals, and the routed carrier has passed automated CAD checks. These results do not establish physical operation. Fabrication, programming, electrical bring-up, calibrated timing and target interoperability remain unverified. Executed checks are listed in VERIFICATION.md.

Original firmware, application and tests: GPL-2.0-only. Hardware utilities: MIT. Original carrier design: CERN-OHL-S-2.0. Standard KiCad footprints, purchased modules and installed tools retain their upstream provenance and licenses.

## Purpose and practical utility

Return Queue observes a resource recycler whose declared policy is to append each returned identity to the tail of its available-resource queue. It identifies allocation paths that accidentally prepend a completed descriptor, use numeric priority, or confuse admission age with return age. Such bugs can leave resource occupancy perfectly consistent while violating a documented recycling contract. The device retains independent return ordinals and freezes its diagnostic model at the first allocation inconsistent with the expected queue head.

A useful laboratory target is an owner-instrumented security appliance with four reusable request descriptors, audit buffers or session contexts. The owner exports committed allocation and return events through slow diagnostic GPIO signals. Harmless workloads exercise completion, cancellation, full drain and recovery. The instrument compares identities without collecting resource contents, pointers, credentials, packet payloads or user traffic. A finding can guide a source review of the recycler rather than requiring broad capture of sensitive application data.

First-return-first selection is not inherently secure. Some targets intentionally prefer the most recently returned resource for locality, whereas others intentionally rotate availability to reduce immediate reuse. Return Queue is useful only when the target actually promises its specific queue policy. A mismatch is evidence of disagreement with that diagnostic contract, not proof of exploitable memory corruption, adequate cleanup, identity isolation or fairness between clients. Security impact requires independent reproduction and analysis.

## Exact measurement contract

There are four exclusive identities, numbered zero through three. At the beginning of an experiment all are free, and their queue order, head first, is zero, one, two, three. ALLOCATE reports an allocation already committed by the target. A legal allocation removes the queue head and marks that identity occupied. Selecting an occupied identity is a duplicate allocation. Selecting another free identity is a head-bypass finding. A completely occupied pool has no permissible allocation.

RELEASE reports the completed return of an occupied identity. It clears that occupancy and appends the identity to the tail, after every resource already free. Any occupied resource may return at any time: return order itself is unconstrained. Returning a free identity is an orphan return. Full drain preserves the learned queue order rather than restoring numeric order. Idle time changes nothing. This is not a time-based quarantine, minimum-use-distance rule or fixed round-robin schedule.

The implementation stores each identity's last successful return ordinal. Successful returns increment the global completed count and retain that ordinal in the returned array. Among free identities, the smallest return ordinal wins. Virgin identities have zero ordinals, precede every returned identity, and break their initial tie numerically. Occupied identities are excluded. Admission ordinals and cumulative uses are retained for accounting and diagnostics but never determine queue priority. A return ordinal stays attached to its identity during subsequent occupancy and changes only on its next return.

Several examples distinguish the policy. Allocate zero, return zero, allocate zero is invalid: the available queue is now one, two, three, zero. Fill all four, return three, return one: three must be allocated next, even though one has a smaller numeric identity and was admitted earlier. Fill all four, return three, allocate three is legal because it is the only vacancy. Repeatedly returning and allocating that sole vacancy remains legal while its peers stay occupied. After a full drain, the next four allocations follow the actual return order, not its reverse.

At the first semantic or interface fault, interpretation halts permanently for that observer run. It retains the last coherent occupancy and histories and counts the rejected observation. This does not mean the target rejected the operation: the target may already have diverged. Post-halt eligibility describes the retained observer model only. Export the evidence, inspect the harness and target, and reestablish the initial experimental state before resetting the observer.

## Originality, comparison and recovery provenance

The synchronized jayis1/hacker-devices directory inventory and README searches were checked before completing this device. The closest contracts differ materially. Return Stack prepends returns and chooses the greatest return ordinal; Return Queue appends returns and chooses the smallest. Oldest Free chooses by last admission, not last return, and permits arbitrary virgin ties. Queue Overtake constrains the departure order of occupied items; this design permits arbitrary departures and constrains later admissions from the vacant set. Lowest Fit ignores history and chooses numeric minimum. The distinguishing traces above separate those policies even with identical physical wiring.

An untracked partial return-queue directory was traced to an interrupted execution of this same authorized task. It contained renamed platform files, but still implemented Return Stack semantics and lacked its own tests, documentation and CAD outputs. The first new queue-specific regression failed on that inherited comparator before correction. The completed design was rebuilt and retested; inherited or previously generated build products were not used as validation evidence.

The passive carrier, register definitions, startup, qualification, CRC, bounded UART transport and desktop transport deliberately reuse jayis1-authored infrastructure. Internal UQ-prefixed names are retained platform identifiers; RQ1 is the device-specific wire marker. The contribution is the integrated first-return-first diagnostic contract, independent queue oracle, retained ordinal evidence, sparse telemetry validation and operating procedure. FIFO algorithms and resistor networks are established engineering. This is originality within the inspected repository, not a worldwide novelty or patentability claim. Runtime files are self-contained in this directory; there is no shared systems tree or dependency on daily-ideas.

## Attack surface, threat model and legal boundaries

Use only on equipment you own or have explicit written authorization to assess. Authorization must cover enclosure access, diagnostic instrumentation, attachment points, workloads, service interruption, restoration and evidence retention. Permission to scan a network is not permission to attach a PCB or replace firmware. Start with a low-energy laboratory fixture whose production dependencies and hazardous outputs are disconnected.

The investigated attack surface is resource bookkeeping: completion identity, cancellation paths, stale queue pointers, duplicate admission, double return, and inconsistent normal versus recovery paths. Authorized red teams can ask a system owner to expose suspected allocator behavior through harmless workloads. Researchers can compare implementation revisions against the same reduced trace. Penetration testers can reproduce an approved diagnostic finding after remediation. The device does not create workloads, inject target commands, exploit a vulnerability, access memory, transmit radio signals or implement persistence.

The trusted base includes target instrumentation, diagnostic adapter, wiring, MCU image, operator and workstation. A malicious adapter can fabricate or omit events. A compromised host can rewrite exports. Physical programming access can replace firmware. CRC detects accidental corruption but supplies no authenticity or replay protection. There is no secure boot, attestation, persistent boot identity, trusted wall clock, encrypted storage or signed update mechanism. Treat operation timing and resource histories as potentially sensitive engagement data even though payload capture is absent.

Receive-only describes the supplied firmware and application, not a certified hardware data diode. Replacement firmware could drive GPIOs. Ground loops, loading, shorts or back-power can disturb a target without intentional transmission. Do not attach to unknown field wiring, mains-referenced equipment, occupied access controls, vehicles, medical devices or hazardous machinery. Stop if attachment changes target behavior. Legal wording does not substitute for electrical qualification or operator supervision.

## Hardware specifications and bill of materials

The external active module is ST NUCLEO-F401RE containing STM32F401RET6, a Cortex-M4 MCU with 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI and derives a nominal one-millisecond SysTick. USART2 transmits through PA2 at 115200 baud, eight data bits, no parity and one stop bit to ST-LINK's USB virtual COM port. The purchased development board supplies regulation, decoupling, reset, SWD, USB and programming access. Those are external module functions, not missing circuits in a purported standalone MCU board.

The custom carrier is a cable-connected, two-layer, 80 by 70 mm PCB with nominal 1.6 mm thickness and 0.3 mm tracks. It is not an Arduino stacking shield. Four signal paths each contain a 1 kOhm series resistor and a 100 kOhm pull-down on the MCU side. Four two-pin target headers carry signal and ground. A four-pin MCU signal header and a two-pin ground-only header complete the interface. The carrier has no supply rail, radio, FPGA, antenna, battery, charger, analog sensor, isolated power or target power output. Use an insulating base and strain relief; enclosure CAD and mounting holes are not supplied.

| References | Quantity | Selection |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE; verify purchased revision |
| R1, R3, R5, R7 | 4 | Yageo MFR-25FBF52-100K, 100 kOhm, 1%, axial |
| R2, R4, R6, R8 | 4 | Yageo MFR-25FBF52-1K, 1 kOhm, 1%, axial |
| J1, J3, J4, J5, J6 | 5 | Harwin M20-9990245, 1x2, 2.54 mm vertical header |
| J2 | 1 | Harwin M20-9990445, 1x4, 2.54 mm vertical header |
| Harness/support | 1 set | Short labelled insulated leads and nonconductive base |

Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Verify purchased dimensions, lead forming, hole fit and mating polarity before fabrication. Stock and prices have not been checked. Reserve a conservative 5 V, 500 mA USB source allowance for the external module as planning headroom, not measured power consumption or a USB-compliance claim. The passive carrier cannot power the target.

| Signal | Target pin | MCU header | Nucleo connector | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target connector's second pin is ground. Both J1 pins are ground, never power inputs. J2 carries only signals; connect module ground separately through J1. The `_IN` nets are target-side nets; the short signal names denote MCU-side nodes. Disconnected signals look low and cannot be distinguished from legitimate low outputs. No cable-continuity diagnostic is present.

Only known common-ground push-pull signals between zero and 3.3 V are within the proposed interface. Exclude 5 V, 1.8 V, differential, negative and unknown-voltage sources. Series resistors provide neither isolation nor dedicated ESD/surge protection. There are no level translators or external clamps. Validate offsets, leakage, thresholds and harness capacitance on the assembled fixture. Power the observer before enabling target signals, and remove target signals before depowering it. USB and target grounds become common.

Manufacturer references are ST UM1724 for the NUCLEO-F401RE connector map and USART bridge configuration, RM0368 for peripheral registers, and the STM32F401xD/xE datasheet for electrical limits. The default virtual COM connection uses SB13/SB14 on and SB62/SB63 off; check the purchased board rather than assuming factory configuration. UM1724: https://www.st.com/resource/en/user_manual/dm00105823-stm32-nucleo-64-boards-mb1136-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned diagnostic adapter       Passive carrier        External Nucleo
TAG0 ----------------------------- 1k --+---------------- PA0
TAG1 ----------------------------- 1k --+---------------- PA1
ALLOCATE ------------------------- 1k --+---------------- PA4
RELEASE -------------------------- 1k --+---------------- PB0
                                        | each MCU node
                                       100k
                                        |
Target ground ------------------------- GND ------------- module GND

GPIO -> continuity gate -> bundled tag / independent strobe qualification
     -> setup and opposite-strobe guards -> earliest-free return comparison
     -> transactional histories -> RQ1 status and compatibility encoder
     -> USART2 TX -> ST-LINK USB serial -> Ledger / History / Integrity
```

Target-facing internal GPIO pulls are disabled. PA and PB are read separately; acquisition is not atomic across ports. This is a slow diagnostic observer, not a native-speed protocol analyzer. A diagnostic adapter must export committed operations in order and must not silently overflow its own buffer. Instrumentation can alter timing and mask a race. Corroborate findings with source logs and independent waveforms before drawing conclusions about production behavior.

## Firmware design and timing decisions

The C11 image has no RTOS, heap allocation, filesystem, received-command parser or downloaded vendor SDK dependency. Startup initializes data and BSS and supplies core vectors. The linker places flash at 0x08000000 and RAM at 0x20000000, reserving stack headroom. Board support initializes input GPIOs, reset-cause capture, SysTick, USART2 and the independent watchdog. USART reception remains disabled. The main loop samples on advancing ticks, services at most one transmit byte each iteration and offers status every nominal 100 ms when the queue permits.

The two tag bits qualify as one bundle, while allocation and release strobes qualify independently. A changed candidate needs three subsequent matching samples. Bundling prevents individually settled tag bits from synthesizing an unobserved identity. Independent strobes prevent unrelated chatter from indefinitely hiding an operation. A legal strobe edge requires five prior stable tag observations, raw/qualified agreement, and absence of the opposite strobe in the relevant observations. Simultaneous qualified allocation and release are ambiguous and rejected rather than arbitrarily ordered.

Use conservative fixture waveforms: keep both strobes low, set the tag for at least twelve nominal milliseconds, assert exactly one strobe for at least six milliseconds, then return it low for at least six milliseconds while retaining the tag. These are laboratory recommendations rather than calibrated specifications. Short pulses may disappear through qualification; sub-sample glitches are outside guaranteed observation. Start with both strobes low and reset the target's actual free queue. Quiet pins alone do not prove the baseline.

Any advancing timestamp delta other than one halts correlation as an acquisition gap. Repeated timestamps do nothing; ordinary unsigned wrap is supported. Accounting runs on a speculative copy and commits atomically. If a counter would overflow, the sample is discarded and the prior coherent state is retained with an exhaustion fault. Watchdog initialization uses bounded waits and produces a terminal fault if initialization fails. Physical watchdog reset and programming have not been tested.

The transmit queue holds at most 768 bytes. Submission rejects empty, oversized and overlapping frames. Polling does not wait for readiness; a queued frame exceeding its 100 ms timeout is abandoned with a diagnostic increment. Status time can advance after acquisition has halted, but measurement fields remain frozen. Serial freshness therefore does not mean valid ongoing acquisition. There is no host command to clear findings. Reset starts a different experiment and requires a corresponding controlled target baseline.

## Application interface and evidence format

Frames are ASCII `RQ1,TYPE,decimal,...*HHHH` followed by LF. CRC-16/CCITT uses polynomial 0x1021, initial 0xffff, no reflection and no final XOR over bytes before the asterisk. B reports captured reset flags. P reports `3,1,5,4,0,1`: qualifier, nominal sample interval, setup requirement, resource count, initial head and policy revision. Compatibility announcements repeat about once per second so late attachment need not impersonate a reboot.

S fields, in order, are now, levels, valid, halted, faults, phase, active, depth, stable, observed, rounds, attempts, launched, completed, rejected, gaps, missed, queued, peak, use0 through use3, stamp0 through stamp3, ret0 through ret3, and tx_fail. All are unsigned 32-bit integers. Fault bits 1, 2, 4, 8, 16, 32 and 64 identify head bypass, orphan return, duplicate allocation, interface ambiguity, acquisition gap, exhaustion and watchdog startup failure. WAIT is phase zero and TRACK is one. Halt clears qualification validity but retains evidence.

The companion is a Linux desktop application using standard Python tkinter and termios because local serial investigation is the intended workflow. Run `/usr/bin/python3 return-queue/app/console.py` from the repository root. New live opens the selected tty read-only at 115200 baud, flushes buffered input and starts a new session. Disconnect restores terminal attributes while preserving evidence. A second connection is refused until the first closes. There are no configuration, transmit or update controls.

Ledger shows counters, occupied identities, eligible head and findings. History retains the last 256 accepted snapshots. Integrity shows source, provenance, policy compatibility, parser errors, reset hints, age and frozen-session reasons. Replay accepts regular files up to two MiB and rejects FIFOs and device files. Export JSON creates a new private mode-0600 file, refuses overwrite and final-component symlinks, and includes author jayis1 plus session integrity metadata. Exports are not encrypted or uploaded. Choose a trusted destination directory.

Validation checks CRC, field bounds, policy compatibility and accounting invariants. Admissions equal returns plus occupancy; per-tag uses sum to admissions; return ordinals require corresponding returns. Sparse transitions cannot migrate stamps without matching operations. A continuously free identity with no admission is a queue barrier: initially later free identities and initially busy identities cannot pass it. An earlier free identity can be consumed once, but cannot return and be allocated again while that barrier stays untouched. Tests distinguish that case from a legal interval that consumes the barrier before renewal.

These are necessary endpoint checks, not reconstruction of every hidden operation. Plausible forged histories remain possible. Two advancing status snapshots are required before transport is considered live. Duplicate timestamps do not refresh age. Rollbacks, contradictory duplicates, disappearing faults and changing halted evidence freeze interpretation. Long reception pauses or excessive cumulative host/device timing divergence require reconnecting rather than relabelling backlog as fresh. Without persistent boot identity some resets are indistinguishable; oscillator drift can also force conservative reconnects. Replay is never live.

## Building, verification and supervised bring-up

Required tools are GNU Arm Embedded GCC with newlib/nosys, host GCC, Make, Python 3 with tkinter, KiCad 9 and its system Python pcbnew module. Xvfb supports display-backed tests without a physical display. No pip or npm dependencies are needed. From the repository root:

```sh
make -C return-queue/firmware clean all test
xvfb-run -a /usr/bin/python3 -m unittest discover -s return-queue/tests -v
/usr/bin/python3 return-queue/tools/check_hardware.py
/usr/bin/python3 return-queue/tools/check_release.py
```

The build emits ELF, binary and map under ignored firmware/build. Host tests compare repeated operation words with an independent literal FIFO and exercise every return permutation, only-vacancy renewal, duplicate/orphan rejection, setup ambiguity, acquisition gaps, frozen states, timestamp wrap and exhaustion. Main-loop and serial tests cover compatibility announcements, lost boot records, queue bounds, timeouts and watchdog-start failure. Application tests use real C-encoder synthetic output, actual GUI widgets and Linux pseudo-terminals, including hangup, restoration, replay, malformed records and private exports. Synthetic records are not physical measurements.

`/usr/bin/python3 return-queue/tools/generate_hardware.py` regenerates this device's CAD files and connectivity manifest; preserve intentional manual edits before using it. The checker runs ERC and DRC, exports a fresh schematic XML netlist, and compares every reference, value, footprint and pad/net assignment with the board and manifest. Passing checks does not validate an external supply, real harness, component tolerances or assembly workmanship.

Before assembly, inspect the rendered schematic, routing, pin numbers and purchased component dimensions. With power removed, check continuity and shorts. Connect the observer module alone first. Program the ELF with a supervised ST-LINK/SWD workflow suitable for the purchased module; no automatic flashing command or programming result is claimed. Verify current consumption, reset defaults, baud rate and input levels before connecting an owned fixture. Scope each tag/strobe independently and repeat the documented legal and deliberately invalid traces slowly.

Record module revision, firmware revision, wiring, instrumentation revision, initial queue order and limitations with each experiment. Export before resetting either side. Minimize repeatable findings and corroborate them outside the observer. A missing finding is not proof of memory safety: omitted operations, lying instrumentation, disconnected wires, narrow pulses and unexercised paths remain outside the assurance provided. Retain evidence only for the authorized engagement and dispose of it under the agreed retention policy.
