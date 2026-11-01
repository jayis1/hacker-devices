# Rank Drain — Fixed-Priority Resource Teardown Witness

Author and creator: jayis1. Hardware revision A. Protocol RK1, policy revision 1.
Firmware, companion and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier: CERN-OHL-S-2.0. Standard library footprints and installed tools retain their upstream provenance and licenses.

Status: unbuilt engineering prototype. Target-linked firmware, a Linux desktop application and routed native KiCad files are included. Software and CAD checks are recorded in VERIFICATION.md. No fabrication, MCU programming, electrical qualification, calibrated timing or physical target integration has been performed. This instrument observes a diagnostic contract; it does not enforce teardown, remove privileges or certify a system secure.

## Purpose and practical value

Rank Drain observes whether a small security-sensitive resource pool releases its highest-ranked live resource before any lower-ranked resource. Four identities, zero through three, have immutable numeric ranks. Admission order is unrestricted. A free identity can be admitted immediately, including immediately after its previous release. The only ordering obligation is at release: the named identity must be the maximum of the currently occupied set. The instrument retains the occupied set at the first contradictory release and reports the observed tag, accepted admissions, accepted releases and rejected operations.

The motivating experiment is a recovery service whose teardown contract gives dependent capabilities higher ranks than their supporting contexts. An owner-controlled laboratory build exposes actual admission and release boundaries. A cleanup path may appear correct during ordinary reverse-order unwinding yet release a supporting context too soon after asynchronous admissions reorder the resources. The fixed ranking remains meaningful even when admission order changes. A separate observer can preserve that contradiction without collecting protected payloads, pointers, credentials or executable addresses.

Consider admissions three, zero, two. The next legal release is three, followed by two, then zero if there are no further admissions. Last-in-first-out would choose two, and first-in-first-out would choose three followed by zero. Those are different contracts. A useful regression explicitly varies admission order instead of testing only ascending admissions, where descending release happens to resemble a stack. The application exposes both cumulative uses and the next releasable identity so an operator can see which ordering rule is being evaluated.

The numerical hierarchy is a deliberately bounded diagnostic model, not a universal resource-management recommendation. Many real allocators permit arbitrary return order and should not be judged by this device. The owner must first document why its particular teardown boundary requires descending rank. A finding establishes disagreement between exported events and that declared contract. Security impact, such as retained access or corrupted cleanup state, still needs independent confirmation using the owner's approved functional tests.

## Exact observed contract

Begin with all four target resources known free and both operation strobes low. Quiet wires cannot prove emptiness; the operator must establish the target baseline independently. ALLOCATE announces an operation already accepted by the target, not an attempted request that the target denied. RELEASE announces the agreed teardown boundary, not the beginning of asynchronous cleanup. A two-bit tag identifies the resource. Tags are unique while live, but may be reused without a lifetime quota, quarantine, coverage round or waiting period.

An allocation of an occupied tag is a duplicate fault. An allocation of any free tag is legal, including a lower tag while higher tags are live. A release of an unoccupied tag is an orphan fault, taking precedence over rank comparison. A release of an occupied tag with any higher occupied tag is a rank fault. A release of the highest occupied tag removes only that tag. There is no imposed admission hierarchy, fixed successor cycle, FIFO order, stack order, fairness promise or requirement to use all four identities.

New admissions may interleave with drain. For example, admit three and one, release three, then admit three again. Releasing one now violates the contract because three is live again. Conversely, admit three and zero, release three, release zero, admit zero, release zero is legal. Waiting does not change eligibility. Complete drain ends an occupancy episode but does not erase cumulative use counters or last-admission stamps. Such stamps support history checks; their values do not determine release priority.

At the first interface or policy contradiction the engine halts semantic interpretation. Rejected observations are counted but do not alter previously accepted occupancy. This is not evidence that the target rejected the operation: the diagnostic adapter normally reports something the target already did. The retained set is the observer's last coherent model. Export it and investigate rather than treating the displayed next tag as permission to continue. A new run requires a new owner-established target baseline and observer reset.

## Repository-relative originality and reuse

The synchronized jayis1/hacker-devices directory inventory and README titles were checked before selection. The closest contracts were inspected directly. Lock Rank constrains ascending acquisition by each client and explicitly permits arbitrary release order. Context Stack learns a nested LIFO return order from arrival and permits equal-tag reentrancy. Queue Overtake learns FIFO departure order from arrival. Rank Drain instead uses exclusive identities with fixed descending release priority independent of arrival. Distance Renew constrains reuse by intervening admissions; immediate reuse is legal here.

This is a new research instrument within the reviewed repository, not a claim to invent priority queues or to establish worldwide novelty or patentability. Its contribution is the precise fixed-rank teardown contract, passive diagnostic implementation, independent descending-scan test oracle, sparse higher-incumbent consistency check and operator-facing evidence. The passive resistor carrier, startup, register definitions, UART driver, qualification framework and desktop transport intentionally reuse jayis1-authored Distance Renew infrastructure. Internal UQ-prefixed symbols are inherited implementation names, not protocol compatibility claims.

The folder is self-contained. It imports no sibling project at runtime and adds no shared hardware tree, unified-system CLI, external service or GitHub workflow. The hardware is openly described as a reused carrier rather than a new electrical topology. This distinction allows novel measurement semantics to be reviewed separately from the familiar electrical interface and avoids hiding platform reuse behind a different device name.

## Attack surface, threat model and legal use

Use only on equipment you own or have explicit written authorization to assess. Written scope should name the device, firmware instrumentation, physical attachment points, harmless workloads, interruption limits, recovery plan and evidence retention. Authorization for a network penetration test does not automatically authorize opening equipment or connecting grounds. Begin with a low-energy development fixture with hazardous outputs and production dependencies disconnected.

The investigated attack surface is cleanup bookkeeping across asynchronous security contexts: wrong-tag release, premature lower-rank teardown, duplicate allocation, missing admission and incorrect event boundary attribution. An authorized red team can ask the owner to exercise an approved error path and retain a reduced reproduction. A researcher can compare multiple cleanup implementations under varied admission permutations. A penetration tester can repeat the same bounded workload after remediation. The instrument itself does not run workloads, exploit flaws, discover hosts, intercept secrets or send commands to a target.

The operator, target diagnostic adapter, harness, MCU image and workstation are trusted. A malicious target can omit or forge indications. CRC detects accidental transport corruption, not authenticity or replay. A hostile serial source can manufacture plausible records; a compromised workstation can rewrite exports. There is no secure boot, secure element, signed log, persistent boot identity, trusted wall clock, encrypted telemetry, network listener or authenticated update channel. Physical ST-LINK access can replace the firmware.

Receive-only is a property of the supplied GPIO configuration, not a certified data diode. Different firmware could drive the same pins through the resistors. Even correct firmware can disturb equipment through loading, common grounds, shorts or back-power. Do not attach this unqualified prototype to mains-referenced equipment, medical systems, vehicles, occupied access controls, industrial machinery or unknown field wiring. Stop if attachment changes target operation, voltage or temperature. A clean trace is neither security certification nor evidence that every event was observed.

## Hardware specifications and BOM

The active module is an external ST NUCLEO-F401RE, MB1136 family, with STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware uses reset-default nominal 16 MHz HSI, nominal one-millisecond SysTick and USART2 TX at 115200 baud, eight data bits, no parity and one stop bit. PA2 connects to the board's ST-LINK virtual COM bridge. The purchased module supplies regulation, decoupling, reset, USB conversion and SWD programming; these are external module facilities, not omitted components on a supposed standalone MCU PCB.

The custom passive carrier is 80 by 70 mm with two copper layers, nominal 1.6 mm thickness and 0.3 mm tracks. Fourteen through-hole components provide thirty pads on nine named nets. It connects through labelled leads rather than stacking directly onto Nucleo headers. No FPGA, radio, antenna, battery, charger, active sensor, removable storage, level translator or isolated supply is present. No enclosure or mounting-hole design is supplied; use insulating supports, underside protection and strain relief.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2, 2.54 mm through-hole header |
| J2 | 1 | Vertical 1x4, 2.54 mm through-hole header |
| Harness and supports | 1 set | Short insulated labelled leads, nonconductive base |

Resistor footprints use DIN0207 bodies with 7.62 mm lead spacing. Check purchased component drawings, body size, lead diameter, drill fit and mating orientation before procurement. No stock or pricing guarantee is made. Reserve a conservative 5 V, 500 mA USB supply allowance for the purchased module; this is planning headroom, not a measured power budget or a USB compliance result. The passive carrier does not power the target.

| Signal | Target header | MCU header | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target header's second pin is ground. Both J1 pins are ground, never power inputs. J2 has four signal pins and no ground: a separate module ground lead to J1 is mandatory. Each target net ending in _IN passes through 1 kOhm to the MCU node, with 100 kOhm from that node to ground. An unplugged signal is indistinguishable from a valid low level.

Only known common-ground, push-pull zero-to-3.3 V diagnostics are supported. Negative, five-volt, 1.8 V, differential and unknown signals are excluded. A series resistor is not isolation or surge protection. Check ground offset, source impedance, MCU thresholds, leakage and harness capacitance. Power the observer before enabling target signals; remove target signals before depowering it. USB ground becomes target ground. Electrical and ESD qualification remain required before attachment.

Manufacturer references are ST UM1724 for the Nucleo-64 connectors and bridge configuration, RM0368 for STM32F401 registers, and the STM32F401xD/xE datasheet for device limits. Confirm purchased module revision, USART2 VCP solder bridges and power jumpers. The inherited pin map was checked for consistency with firmware and CAD; this run does not claim new bench measurements or a fresh manufacturer-document electrical review. Reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned target diagnostics       Passive carrier          Nucleo-F401RE
TAG0 ----------------------------- 1k --+---------------- PA0
TAG1 ----------------------------- 1k --+---------------- PA1
ALLOCATE ------------------------- 1k --+---------------- PA4
RELEASE -------------------------- 1k --+---------------- PB0
                                        | each node
                                       100k
                                        |
Target ground --------------------------+--- J1 -------- module GND

GPIO -> continuity guard -> bundled tag / independent strobe qualification
 -> prior setup and opposite-strobe guard -> exclusive occupancy
 -> descending live-rank release check -> transactional retained evidence
 -> RK1 encoder -> bounded USART2 TX -> ST-LINK USB serial
 -> Linux Ledger / History / Integrity -> explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not simultaneously latched. This is a slow diagnostic observer, not a processor trace analyzer, high-speed bus decoder or glitch detector. The owner adapter must stretch indications while preserving their order and semantic meaning. Start with twelve nominal milliseconds of stable tag and low strobes, six milliseconds with exactly one strobe high, then six milliseconds low. Independently inspect the actual waveform. If instrumentation changes scheduling or removes the original race, do not generalize the observed result to the uninstrumented product.

## Firmware and failure behavior

Firmware is C11 without RTOS, dynamic allocation, filesystem, downloaded SDK or command receiver. Startup initializes data and BSS and supplies Cortex-M vectors. The linker places flash at 0x08000000 and SRAM at 0x20000000 with stack headroom. Drivers implement input GPIO, SysTick, USART2 TX, reset-cause capture and independent watchdog support. Target pins remain inputs with internal pulls disabled. The USART receiver is disabled; no host command can alter ranks or clear retained findings.

The tag bits qualify as a bundle to avoid artificial intermediate identities from independently delayed bits. Allocation and release qualify independently so chatter on one does not indefinitely suppress the other. Each candidate requires its first sample plus three matching intervals. An operation also requires five prior stable tag observations, raw/qualified tag agreement, the selected raw strobe still high and the opposite strobe low in previous qualified, current qualified and raw states. Held strobes count once. Short pulses may be rejected or missed and are not claimed as covered events.

Interface checks precede rank semantics. Fault bits are rank violation 1, orphan release 2, duplicate allocation 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog startup failure 64. Simultaneous rising strobes count as two rejected observations, not an invented allocation/release order. A quiet qualified baseline enables tracking but does not establish that the target was empty. The phase remains WAIT until that baseline and TRACK thereafter, with halted status reported independently.

Every sample updates a speculative copy. If any checked counter exhausts, the complete update rolls back and the previous coherent state halts. Accepted ordinal counters never wrap. Ordinary unsigned millisecond wrap is supported, repeated timestamps do nothing and skipped samples halt rather than crediting unobserved stability. After halt, uptime and transport diagnostics may advance but semantic evidence freezes. The watchdog continues to be fed so a policy violation is not deliberately erased by reboot.

The UART driver owns a bounded 768-byte buffer, refuses overlapping submissions and services at most one ready byte per main-loop iteration. A hundred-millisecond transport timeout abandons an incomplete record and increments a saturating failure count. No serial wait blocks acquisition. Status is scheduled every hundred milliseconds when the transmitter is available. Compatibility policy repeats approximately every second for late attachment; the boot record is sent once rather than masquerading as a periodic reset.

## Companion application and protocol

Run python3 app/console.py on Linux with Tk installed. The standard-library application requires no dependency download, cloud account or network service. Select a verified serial path, preferably under /dev/serial/by-id/, and choose New live. No arbitrary-device scanning occurs. A second serial connection is refused until disconnect. Original terminal attributes are restored on close and setup failure. Failed replacement connections preserve retained evidence.

Ledger displays all counters, active identities with cumulative uses, last-admission ordinals, decoded findings and the next releasable tag. That is the highest retained occupied identity, not a target command or authorization, especially after HALT. History shows up to 256 snapshots. Integrity displays origin, compatibility, parser errors, reset hints, freshness and freeze reasons. Replay and live provenance remain separate after disconnect. The interface deliberately emphasizes retained observations rather than presenting a green certification badge.

Replay accepts only an opened regular file of at most two MiB; devices and FIFOs are rejected without blocking. An incomplete final frame counts as corruption. Empty and incompatible captures are rejected. Export creates a new mode-0600 JSON file, refuses overwrite and final-component symlinks, and includes metadata even if no operation completed. Exports are not signed or encrypted. Use the owner's retention policy; ordinary deletion is not secure erasure on flash-backed media.

RK1 frames contain ASCII RK1,type,decimal-fields followed by an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE covers preceding bytes with initial value 0xffff and polynomial 0x1021. B contains reset flags. P contains [3,1,5,4,3,1]: qualification intervals, nominal sampling period, prior tag setup, identity count, highest rank and descending-release policy revision. S field order is:

```text
now levels valid halted faults phase active depth stable
observed rounds attempts launched completed rejected gaps missed
queued peak use0 use1 use2 use3 stamp0 stamp1 stamp2 stamp3 tx_fail
```

Launched is the accepted-admission ordinal. Rounds counts closed nonempty occupancy episodes, not rank sweeps. Queued counts admissions while another identity is live. Admissions equal releases plus active population and the sum of per-tag uses. Attempts equal admissions plus releases plus rejected observations. Nonzero last-admission stamps are distinct, bounded by launched and consistent with per-tag counts. The latest ordinal must remain owned by a stamp, even if its resource has already been released.

Across sparse snapshots, cumulative histories cannot decrease or migrate. Changed stamps require matching new admissions. Per-tag releases are derived from use growth and occupancy change. A higher identity live at the old endpoint with zero derived releases remained present for the whole interval; therefore no lower identity can have been released, even in a hidden release/reallocate cycle. A higher identity that actually released and reentered may legitimately surround a lower release, so that path is not rejected merely because both endpoints show it occupied.

These are necessary endpoint constraints, not complete reconstruction of hidden operations or authentication of a hostile sender. The MCU checks each qualified event; a plausible sparse trace may still conceal unobserved excursions. Live freshness needs compatible policy, an open port and advancing timestamps. Duplicates do not refresh it. Rollback, contradictory duplicates, disappearing faults, changing halted evidence, long reception pauses and excessive host/device time divergence freeze interpretation. Drift allowance is capped. Reconnect flushes input and starts new host provenance without resetting the MCU. Some missed resets remain indistinguishable without a permanent boot identity.

## Build, validation and physical acceptance

From this directory run make -C firmware all test using GNU Make, host GCC and Arm bare-metal GCC/newlib. The Makefile builds firmware/build/rank-drain.elf and .bin for Cortex-M4 Thumb soft-float, with warnings as errors. Host engine tests use address and undefined-behavior sanitizers. The independent oracle uses a boolean occupancy array and a descending numeric scan rather than production bit shifts. It explores repeated operations through depth eight, checks each resulting occupancy and fault, and separately covers every occupied subset and candidate release. Tests also cover immediate reuse, arbitrary arrivals, raw ambiguity, gaps, repeated timestamps, wrap and exhaustion rollback.

Run xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v for parser, contract and actual Tk event-loop tests. Real C-encoder fixtures exercise accepted rank drain and retained rank faults. Linux pseudo-terminals exercise serial attachment, hangup, attribute restoration, screens and private export. These are synthetic inputs, not physical GPIO or USB measurements. Run python3 -m compileall -q app for syntax checks. UART and main-loop tests also cover timeout, lost boot, periodic compatibility and watchdog startup failure.

With KiCad 9 and Python bindings installed, /usr/bin/python3 tools/generate_hardware.py regenerates native symbols, schematic, routed board, project and connectivity manifest. Then /usr/bin/python3 tools/check_hardware.py runs ERC and DRC, exports a fresh netlist and compares every reference, value, full footprint identity and pad/net assignment across schematic, PCB and manifest. Passive-only ERC cannot validate the external power source or module configuration. Render the schematic and inspect text and connector labels separately from automated rule checks.

Before physical acceptance, inspect unpowered shorts, resistor values, connector orientation, harness continuity and ground arrangement. Verify module jumpers, program through ST-LINK and inspect idle policy/status before enabling any diagnostic output. Exercise descending release after varied arrival permutations, lower-rank rejection, immediate reuse, interleaved higher-rank reentry, orphan and duplicate operations, acquisition loss and reset. Compare reports with independent waveforms, measure loading, calibrate timing and check watchdog recovery. Fabrication, electrical safety, mechanical packaging, ESD, EMC and regulatory gates remain open until tested on the actual assembly.
