# Return Stack — Released-Resource Free-List Head Witness

Author and creator: jayis1. Hardware revision A. Protocol RS1, policy revision 1.

Status: unbuilt engineering prototype. This package contains target-linked C firmware, a working Linux desktop application, and native routed KiCad carrier sources. Software and CAD checks are recorded in VERIFICATION.md. Fabrication, physical programming, electrical qualification, calibrated timing and actual target interoperability have not been performed. The device is an observer of a diagnostic contract, not an allocator, protection mechanism, security certificate or safety interlock.

Original firmware, application and tests: GPL-2.0-only. Original hardware utilities: MIT. Original carrier design: CERN-OHL-S-2.0. Purchased modules, installed tools and standard KiCad footprints retain their upstream provenance and licenses.

## Purpose and practical value

Return Stack investigates whether a resource recycler actually returns freed identities to the head of its free list. An embedded service can maintain correct occupancy while corrupting the ordering of its available descriptors. A cancellation path that appends rather than prepends, a stale cached head, or a completion handler that substitutes numeric priority can silently select the wrong free resource. Ordinary successful-request counts will not expose that difference. Return Stack retains return order independently and freezes its evidence at the first admission inconsistent with the declared list.

A practical target is an owner-instrumented embedded security service with four reusable request descriptors, audit buffers or context slots. The owner exports committed admissions and releases over a slow diagnostic adapter. Harmless workloads exercise normal completion, cancellation, complete drain and immediate resubmission. The observer checks which identity is reused without collecting pointers, credentials, addresses, packet payloads or resource contents. A source-level trace and independent waveform capture should establish that the adapter faithfully represents the suspected defect.

Last-return-first reuse is not intrinsically secure or insecure. Its usefulness depends on the target's documented allocation contract. A locality-oriented implementation may intentionally reuse its most recently returned descriptor; a hardened implementation may intentionally do the opposite. Applying this observer to the latter would produce meaningless findings. Security impact, including stale-state reuse or broken cleanup expectations, needs separate confirmation. This instrument proves neither erasure nor isolation and does not turn a policy mismatch into an exploit.

## Exact measurement contract

The pool contains four exclusive resource identities, zero through three. At experimental start, all identities are free and their free-list order, head first, is zero, one, two, three. ALLOCATE reports an admission already committed by the target. The selected identity must be free and must equal the current head. Accepting the operation removes that head. Selecting an occupied identity is a duplicate allocation; selecting a different free identity is a free-list-head bypass. A full pool has no admissible allocation.

RELEASE reports the completed return of an occupied identity. The observer clears that identity's occupancy and pushes it onto the front of the free list. Any occupied identity may return at any time; release order is unrestricted. An unoccupied identity cannot be returned again and produces an orphan-release fault. There is no nested-call return discipline, mandatory release rank, fairness rotation, minimum reuse delay or requirement to visit every identity.

The implementation represents ordering with return ordinals rather than a mutable linked list. Each successful release increments the global completed count and stores that ordinal against its identity. The eligible free identity has the greatest retained return ordinal; never-returned identities tie at zero and are ordered numerically. Occupied identities are excluded from the comparison. Every accepted admission separately retains its admission ordinal and cumulative use count, but those quantities do not choose the free-list head.

For example, allocate zero, release zero, allocate zero is legal even though three identities have never been used. Allocate zero and one, release zero and then one: the next eligible identity is one, not zero. Reallocate one, then zero: both operations are legal. After filling all four slots, release three and then one: the next allocation must be one even though three has the greater numeric identity. Complete drain preserves return ordering; it does not restore the virgin numeric list. Idle time does not change priority.

At the first rejected semantic or interface observation, interpretation stops permanently for that run. The previous coherent occupancy and histories remain available. The rejected observation is counted without pretending that the target rejected its own operation. Eligibility displayed after a halt refers to the retained observer model, not the actual target state or permission to continue testing. Export the evidence, inspect the target and harness, and establish a fresh controlled baseline before resetting the observer.

## Distinction from existing devices and provenance

The synchronized jayis1/hacker-devices repository was inventoried by device directory and README content. Closely related contracts were read rather than comparing names alone. Lowest Fit chooses the minimum numeric free identity and explicitly ignores return order. Cursor Fit advances a cyclic search cursor on admission, not release. Oldest Free chooses the least-recently admitted free identity. Context Stack constrains nested returns and allows equal-tag reentrancy; Return Stack has unique resources, arbitrary return order and constrained subsequent admission. These contracts accept different operation traces.

The return-stack directory was recovered from an interrupted execution of this same authorized task after its provenance was confirmed in the originating task history. Fresh target builds, host tests and application tests were run; inherited build products were not treated as evidence. The passive carrier, register map, startup, qualification, CRC, bounded UART and desktop transport reuse jayis1-authored repository infrastructure. UQ-prefixed C identifiers are retained internal platform names, whereas RS1 identifies this device on the wire.

The original contribution is the integrated diagnostic instrument: released-resource ordering, independent literal-list oracle, return-ordinal evidence, sparse-history checks, companion display and operating procedure. Free-list stacks themselves are established engineering, and the resistor circuit is intentionally conventional. No worldwide novelty, patentability or new electronics topology claim is made. Runtime files are self-contained within this folder. There is no shared systems tree, unified CLI, external server dependency or import from daily-ideas.

## Attack surface, threat model and authorized use

Use only on equipment you own or have explicit written authorization to assess. Scope must cover enclosure access, target instrumentation, attachment points, approved workloads, service interruption, restoration and evidence retention. Permission for network scanning does not imply permission to attach wires or alter diagnostic firmware. Begin on a low-energy laboratory fixture with production dependencies and consequential outputs disconnected.

The investigated attack surface is resource bookkeeping: cancellation queues, completion identity, stale head pointers, double returns, duplicate admissions and fast-path versus recovery-path policy differences. Authorized red teams can ask an owner to expose a suspected allocator regression through harmless operations. Researchers can compare implementation revisions under identical reduced traces. Penetration testers can repeat an approved reproduction after remediation and attach the retained finding to their report. The device itself generates no target workload, sends no commands, transmits no radio signals and performs no exploitation.

The trusted base includes the diagnostic adapter, operator, MCU image, purchased development board, wiring and workstation. A malicious adapter can fabricate every indication or omit inconvenient events. A compromised host can rewrite exports. Physical programming access can replace the firmware. The CRC detects accidental corruption, not forgery, authentication failure or replay. There is no secure boot, signed updater, persistent boot identifier, trusted wall clock, encrypted storage or attested evidence. Treat timing and use counts as potentially sensitive engagement data even though payloads are absent.

Receive-only describes the supplied firmware and application, not a certified hardware data diode. The GPIOs could become outputs under replacement firmware. Input loading, ground loops, shorts and back-power can disturb an attached target even without intentional transmission. Do not connect to mains-referenced equipment, unknown field wiring, vehicles, medical devices, occupied access controls or hazardous machinery. Stop immediately if attachment changes behavior. The legal disclaimer does not replace electrical qualification or informed operator supervision.

## Hardware specification and bill of materials

The active module is an external ST NUCLEO-F401RE with STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. The firmware assumes reset-default nominal 16 MHz HSI and derives a nominal one-millisecond SysTick. USART2 transmits 115200 baud, eight data bits, no parity and one stop bit through PA2 to ST-LINK's USB virtual COM interface. The purchased module supplies regulation, decoupling, USB connection, reset, SWD and programming access. These are external module facilities, not imaginary circuits omitted from a standalone MCU schematic.

The custom carrier is a cable-connected, two-layer 80 by 70 mm PCB with nominal 1.6 mm thickness and 0.3 mm tracks. It contains fourteen through-hole components, thirty pads and nine nets. There is no FPGA, radio, antenna, battery, charger, analog sensor, removable storage, isolated power supply or target power output. It is not a mechanically stacking Arduino shield. Provide nonconductive supports, underside protection and strain relief; mounting holes and enclosure CAD are not supplied.

| Reference | Quantity | Selection |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, purchased revision checked before use |
| R1, R3, R5, R7 | 4 | Yageo MFR-25FBF52-100K, 100 kOhm, 1%, axial |
| R2, R4, R6, R8 | 4 | Yageo MFR-25FBF52-1K, 1 kOhm, 1%, axial |
| J1, J3, J4, J5, J6 | 5 | Harwin M20-9990245, 1x2, 2.54 mm vertical header |
| J2 | 1 | Harwin M20-9990445, 1x4, 2.54 mm vertical header |
| Harness and support | 1 set | Short labelled insulated leads and insulating base |

Verify body dimensions, drilled-hole fit, resistor lead forming and mating connector polarity against purchased parts before fabrication. Resistor footprints are DIN0207 with 7.62 mm lead spacing. Supplier stock and cost have not been checked. Reserve a conservative 5 V, 500 mA USB source allowance for the external module as planning headroom, not measured consumption or USB-compliance evidence. The passive carrier has no supply rail and cannot power the target.

| Signal | Target contact | MCU contact | Nucleo connector | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target connector's second pin is ground. Both J1 pins are ground, never supply inputs. J2 carries only signals; connect module ground separately through J1. Each target `_IN` net passes through a 1 kOhm series resistor to its MCU node, with 100 kOhm from that node to ground. Disconnected wires appear low and cannot be distinguished from legitimate low indications. No cable-integrity monitoring is provided.

Only known common-ground push-pull signals between zero and 3.3 V are within the proposed interface. Exclude 5 V, 1.8 V, negative, differential and unknown-voltage sources. The series resistors are not isolation, dedicated ESD protection or surge suppression. There are no level translators or external clamps. Validate ground offset, loading, leakage, thresholds and harness capacitance on the assembled fixture. Power the observer before enabling target outputs, and remove target signals before depowering it. USB and target grounds become common.

Manufacturer references are ST UM1724, particularly the NUCLEO-F401RE connector table and USART bridge configuration; RM0368 for peripherals; and the STM32F401xD/xE datasheet for electrical limits. UM1724 specifies the default VCP connection with SB13/SB14 on and SB62/SB63 off. Check the actual module revision rather than assuming those bridges remain factory configured. Reference: https://www.st.com/resource/en/user_manual/dm00105823-stm32-nucleo-64-boards-mb1136-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned diagnostic adapter         Passive carrier          External Nucleo
TAG0 ------------------------------ 1k --+---------------- PA0
TAG1 ------------------------------ 1k --+---------------- PA1
ALLOCATE -------------------------- 1k --+---------------- PA4
RELEASE --------------------------- 1k --+---------------- PB0
                                         | each MCU node
                                        100k
                                         |
Target ground -------------------------- GND ------------- module GND

GPIO -> continuity check -> bundled tag / independent strobe qualification
     -> tag setup and opposite-strobe guards -> free-list-head comparison
     -> transactional occupancy + admission/return ordinals -> RS1 encoder
     -> USART2 TX -> ST-LINK USB serial -> Ledger / History / Integrity
```

Internal target-facing GPIO pulls are disabled. PA and PB are read separately, so the acquisition is not atomic across ports. The instrument is a slow diagnostic observer, not a native-speed bus analyzer. An adapter must export committed events in order without silently overflowing its own queue. Instrumentation can perturb the behavior under study; document that uncertainty and corroborate a finding outside the observer.

## Firmware design and timing decisions

The C11 image has no RTOS, heap allocation, filesystem, command parser or downloaded vendor SDK dependency. Startup initializes data and BSS and supplies core vectors. The linker maps flash at 0x08000000 and SRAM at 0x20000000 with explicit stack headroom. Board support initializes input GPIO, reset-cause capture, SysTick, USART2 and the independent watchdog. USART reception remains disabled. The main loop samples on advancing ticks, services at most one UART byte per iteration and offers status every nominal 100 ms when the transmit queue permits.

Qualification treats the two tag bits as one bundle and the strobes independently. A changed candidate needs three subsequent matching samples before qualification. This prevents separately settled tag bits from synthesizing an unintended identity, while unrelated chatter cannot postpone the other strobe forever. Before an accepted strobe rise, the qualified tag requires five prior stable observations. Raw and qualified tag values must agree at the edge, and the opposite strobe must be low in the relevant raw and qualified observations. Simultaneous qualified operations are ambiguous and rejected.

Use conservative laboratory waveforms: both strobes low, set the tag and hold it for at least twelve nominal milliseconds; assert exactly one strobe for at least six milliseconds; return it low for at least six milliseconds while retaining the tag. These are fixture recommendations, not calibrated electrical specifications. A pulse shorter than the qualification window may disappear entirely. The observer does not measure every raw transition and cannot guarantee detection of sub-sample glitches. Start with both strobes low and the target's list explicitly reset; quiet wires alone do not establish the baseline.

Any advancing timestamp delta other than one tick halts correlation with an acquisition-gap fault. Repeated calls at the same timestamp do nothing. Ordinary unsigned timer wrap is supported. Counter updates occur on a speculative ledger copy; if an increment would overflow, the sample's accounting is discarded and the last committed state is retained with an exhaustion fault. The watchdog starts through bounded status waits; initialization failure is a terminal finding. A hard fault after watchdog activation should cause a reset, but physical watchdog recovery has not been tested.

The transmit queue is bounded to 768 bytes. Submission refuses empty, oversized or overlapping frames. Polling never waits for UART readiness and abandons a queued frame at its 100 ms timeout, recording a transport diagnostic. Status time can continue advancing after a halt while measurement fields remain frozen. Therefore fresh serial reception does not imply valid ongoing acquisition. No host command clears history. Reset is a new experiment, not automatic authorization to forget an interrupted target lifetime.

## Wire protocol and companion application

Frames are ASCII `RS1,TYPE,decimal,...*HHHH` followed by LF. The checksum is CRC-16/CCITT with polynomial 0x1021, initial 0xffff, no reflection and no final XOR, over the ASCII bytes preceding the asterisk. TYPE B carries captured reset flags. TYPE P carries the exact policy tuple `3,1,5,4,0,1`: qualifier, nominal sample interval, setup requirement, resource count, initial head and policy revision. Policy repeats about once per second to support late attachment without emitting false reset notifications.

TYPE S contains, in order: now, levels, valid, halted, faults, phase, active, depth, stable, observed, rounds, attempts, launched, completed, rejected, gaps, missed, queued, peak, use0 through use3, stamp0 through stamp3, ret0 through ret3, and tx_fail. All are unsigned 32-bit values. The first fault is sticky: bit values 1, 2, 4, 8, 16, 32 and 64 mean head bypass, orphan return, duplicate admission, interface ambiguity, acquisition gap, counter exhaustion and watchdog startup failure respectively. WAIT is phase zero; TRACK is one. A halt clears qualification validity without deleting retained evidence.

The Linux application uses Python's standard tkinter and termios facilities rather than a mobile framework because direct local serial inspection is the intended workflow. Run `/usr/bin/python3 return-stack/app/console.py` from the repository root. New live opens the selected tty read-only at 115200 baud, flushes queued input and begins a new host session. Disconnect restores terminal attributes and preserves evidence. A second connection is refused until the first closes. No target configuration or firmware-update controls exist; the policy is fixed and announced explicitly.

Ledger displays the counters, current occupied identities, eligible head and fault description. History retains up to 256 accepted snapshots. Integrity shows source, live-versus-replay provenance, parser errors, reset metadata, age, frozen-session reasons and transport failures. Replay accepts only regular files up to two MiB; FIFOs and devices are refused. Export JSON creates a new file with private mode 0600, refuses overwrite and final-component symlinks, and includes author jayis1 plus provenance and integrity metadata. It does not upload data or encrypt exports.

The parser bounds framing, validates checksums, enforces the exact policy and rejects impossible endpoint accounting. Admission counts equal returns plus occupancy; per-resource uses sum to admissions; return ordinals must belong to actual returned resources. Across snapshots, return and admission stamps cannot migrate without matching operations. An identity that stayed free with no admissions blocks reuse of initially lower-priority free identities. An initially occupied identity may legitimately return and become the head, so endpoint eligibility alone must not reject that hidden sequence.

These are necessary sparse-history checks, not a complete reconstruction proof. A compromised source can forge plausible records. Two advancing snapshots are needed before transport is considered live. Duplicate timestamps do not refresh age; contradictory duplicates, rollback, disappearing faults and changed halted evidence freeze interpretation. A long reception pause or excessive host/device progress divergence requires explicit reconnect rather than treating backlog as current. Some resets remain indistinguishable without a persistent boot identifier, and oscillator drift can cause conservative reconnection. Replay is never labelled live.

## Build, verification and supervised bring-up

Required tools are GNU Arm Embedded GCC with newlib/nosys support, host GCC, Make, Python 3 with tkinter, KiCad 9 and its system Python pcbnew module. Xvfb supports display-backed tests on a headless machine. No pip, npm or network package installation is required by the application. From the repository root run:

```sh
make -C return-stack/firmware clean all test
xvfb-run -a /usr/bin/python3 -m unittest discover -s return-stack/tests -v
/usr/bin/python3 return-stack/tools/check_hardware.py
/usr/bin/python3 return-stack/tools/check_release.py
```

The target build emits ELF, binary and map under ignored firmware/build. The hardware generator is a maintenance command, `/usr/bin/python3 return-stack/tools/generate_hardware.py`, that rewrites only this device's CAD outputs and manifest. Back up intentional CAD edits before using it. The hardware checker runs ERC and DRC, exports a fresh XML netlist, and compares references, values, footprints and every pad/net assignment against the PCB and saved connectivity manifest. Passing those checks does not validate an external supply or unmodelled harness.

Host tests compare operation sequences against an independent literal free-list oracle, including every release permutation, immediate reuse, duplicate/orphan faults, overlap, setup guards, timestamp wrap and exhaustion. Serial tests exercise bounds, timeout and recovery; main-loop tests cover lost boot telemetry, periodic late-attach policy and watchdog-start failure. Application tests feed the real C encoder's synthetic records through validation and actual GUI screens, use Linux PTYs for connection/hangup paths, and verify private exports and evidence retention. Synthetic fixtures are not physical measurements.

Before assembly, inspect the rendered schematic, board routing, connector numbering and actual BOM dimensions. With power removed, check continuity against the netlist and absence of shorts. Connect only the observer module first. Program the ELF with a supervised ST-LINK/SWD tool suitable for the purchased module; there is no automatic flash target or claimed programming result in this release. Check reset defaults, current consumption, serial framing and input levels before attaching an owned test fixture. Confirm each tag and strobe independently with a scope, then replay the documented legal and deliberately invalid diagnostic traces at slow rates.

Record module revision, firmware revision, wiring, target instrumentation revision, baseline free-list order and observed limitations with each experiment. Export before resetting either side. A repeated fault should be minimized to a short trace and checked against source logs and independent waveforms. A missing fault is not evidence of general memory safety: hidden operations, lying instrumentation, narrow pulses, disconnected wires and untested resource paths remain outside the assurance provided. Retain exports only for the authorized engagement and delete them under its agreed policy.
