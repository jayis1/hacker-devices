# Retire Distance — Resident Admission-Age Witness

Author and creator: jayis1. Hardware revision A. Protocol RT1, policy revision 1.

Status: unbuilt engineering prototype. This design includes target-linked firmware, a Linux graphical companion, and a routed passive carrier. Software and CAD checks do not establish physical electrical performance, successful MCU programming, fabrication quality, timing accuracy or actual target interoperability. See VERIFICATION.md for executed checks. Firmware, app and tests use GPL-2.0-only; CAD utilities use MIT; original carrier design uses CERN-OHL-S-2.0. Installed toolchains and library footprints retain their upstream licenses.

## Purpose and scope

Retire Distance observes a four-resource diagnostic pipeline whose contract forbids retiring a resident until two subsequent admissions have occurred. It retains the ordinal of each resource's most recent admission, independent occupancy, cumulative admissions, and the first contradictory return. Waiting for a long time cannot satisfy this policy. Two later admissions can satisfy it even when neither later resource has finished. This distinction makes the instrument useful for reduced reproductions of pipeline age bookkeeping, premature descriptor retirement, and cancellation paths that accidentally bypass an operation-distance requirement.

The device is not a memory safety mechanism. Two later admissions do not prove that readers stopped, stale callbacks disappeared, DMA completed, or secrets were erased. The target owner must justify the experimental policy independently. The instrument measures an explicitly declared diagnostic relation, not the appropriateness of that relation for production reclamation. Its output is corroborative evidence for source review and waveform analysis, never a claim that arbitrary memory can safely be freed.

Four slow 3.3 V logic indications encode a two-bit resource identity and separate ALLOCATE and RELEASE strobes. The owner supplies these signals on a development target. The observer neither generates target workloads nor issues commands. It does not access target memory, collect payloads, intercept credentials, transmit radio signals, or install persistence. A small vocabulary makes the contract inspectable and permits independent exhaustive tests without pretending to monitor a full high-speed allocator bus.

## Originality and comparison

The repository default branch was pulled and device directories inventoried before recovery. The untracked directory was traced to an interrupted execution of this same authorized task. The firmware had acquired the new policy but the companion retained Oldest Free checks; real encoder tests exposed that incompatible policy before it was replaced. Platform reuse is deliberate and disclosed: startup, registers, UART, CRC, passive carrier layout and desktop transport derive from existing jayis1-authored infrastructure. Internal UQ names are inherited identifiers, not protocol compatibility promises.

The closest inspected contract is Distance Renew, which constrains a new allocation after a prior admission, but permits immediate return. Retire Distance instead constrains the release itself and allows any vacant identity to be admitted. Allocate zero then release zero is legal under Distance Renew and rejected here. Oldest Free chooses a free candidate according to recency, whereas this design imposes no preference among vacancies. Reuse Quarantine waits after a return using elapsed time, whereas this design requires later admissions before return. Prefix Retire enforces numeric batch retirement order; this instrument permits a mature younger resident to retire before an older one. Quiescent Cut tracks the readers captured at a retirement boundary, not ordinal pipeline age.

The distinctive contribution is this release-age contract, its retained ordinal evidence, resident-debt oracle, sparse endpoint checks and operator interpretation. Originality is relative to the reviewed repository; no worldwide prior-art or patentability claim is made. Admission-age algorithms are established ideas. All runtime code is local to this device folder; no shared hardware tree, unrelated repository changes or unified CLI is introduced.

## Exact measurement contract

Start with all four resources independently known free and with a new experimental history. The global admission ordinal starts at zero. A qualified allocation requires that its tag is vacant, increments the ordinal, writes the new ordinal into that tag's stamp, increments that tag's use counter and marks it resident. No preference, lifetime quota, elapsed-time delay or fairness requirement is imposed at admission. Allocating an already resident tag is a duplicate finding.

A release first requires residency. A vacant tag is an orphan finding. Otherwise release is accepted only when global admission ordinal minus that resident's stamp is at least two. Equality is accepted. Release clears occupancy and increments completed returns, but leaves its stamp and lifetime uses intact. Every later admission counts regardless of which identity receives it; releases and idle samples never count. The first contradiction freezes semantic evidence, retaining the pre-rejection resource state and counting the rejected observation separately.

For example, admit zero, one, two and three. Zero and one are mature. Return one before zero: that is valid and demonstrates that this is not FIFO. Admit one again; two is now mature and can return. Admit two again; zero and three remain mature. Return zero and immediately admit zero again: the renewed zero has a fresh stamp and must wait for two further admissions before return. Attempting to return it immediately is the primary finding. The supplied C fixture executes this sequence.

An important consequence is that the pipeline cannot fully drain under this contract. The most recent two admissions remain resident because they have not yet seen two later admissions. A shutdown drain is therefore outside this experimental vocabulary, not an exception silently invented by the observer. Export the run before shutdown and establish a new empty target baseline before resetting the observer. Do not use this instrument if your intended contract permits arbitrary final flushes without another observable mode. The app explicitly displays this limitation.

## Attack surface, threat model and lawful use

Use only on equipment you own or have explicit written authorization to assess. Permission should cover diagnostic firmware modification, enclosure access, common-ground attachment, workloads, interruption limits, recovery and evidence retention. Network-test permission alone does not authorize electrical attachment. Start with an expendable low-energy bench fixture, disconnected from hazardous outputs and production dependencies.

The investigated attack surface is lifecycle integration: a cancellation path that retires too early, an incorrect age comparison, a stamp copied from the wrong descriptor, an ordinal advanced by returns, or an adapter that confuses attempts with accepted operations. An authorized red team can request benign owner-run stress cases. Researchers can compare implementations against this bounded contract. Penetration testers can repeat the same approved reproduction after remediation. None of these uses requires offensive command delivery from this instrument.

Trust is placed in the operator, diagnostic adapter, harness, MCU image and workstation. A dishonest target can fabricate valid indications. Missing pulses can conceal real operations. Each tag must represent one unique resource, not a hash of many objects. CRC detects accidental corruption but authenticates nothing. There is no secure boot, signed log, encrypted export, persistent boot identifier or trusted wall clock. Physical ST-LINK access can replace the firmware. Protect debug access and stored evidence according to the engagement's data policy.

Receive-only describes the supplied target-facing firmware configuration, not an irreversible hardware data diode. Reprogrammed MCU pins could drive through the series resistors. There is no galvanic isolation, surge suppression or certified overvoltage protection. Never attach this prototype to mains-referenced equipment, medical systems, vehicles, deployed access controls, hazardous machinery or unknown field wiring. Stop immediately if attachment changes target behavior or ground conditions.

## Hardware specifications and electrical boundaries

The active module is ST NUCLEO-F401RE, containing STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware uses reset-default nominal 16 MHz HSI and nominal one-millisecond SysTick sampling. USART2 transmits 115200 baud, eight data bits, no parity, one stop bit through PA2 to the Nucleo ST-LINK virtual serial bridge. USART reception is disabled. The purchased module provides regulation, decoupling, reset, USB connectivity, boot configuration and SWD programming; these are not omitted components of a claimed standalone MCU PCB.

The passive carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, using 0.3 mm traces. Fourteen through-hole components connect thirty pads across nine nets. It is cable-connected rather than a mechanically stacking shield. There is no FPGA, radio, battery, charger, analog sensor, SD card or target power output. An enclosure and mounting-hole pattern are not supplied; use insulating supports, underside protection and strain relief.

| Reference | Value / function |
| --- | --- |
| R1, R3, R5, R7 | 100 kOhm, 1%, 0.25 W axial pulldown |
| R2, R4, R6, R8 | 1 kOhm, 1%, 0.25 W axial series resistor |
| J1 | 1x2 2.54 mm header; both pins GND |
| J2 | 1x4 2.54 mm header toward MCU analog-labelled pins |
| J3–J6 | 1x2 2.54 mm headers; signal then ground |
| External module | ST NUCLEO-F401RE, purchased separately |

J2 pin one carries TAG0 to A0/PA0, pin two TAG1 to A1/PA1, pin three ALLOCATE to A2/PA4, and pin four RELEASE to A3/PB0. J3 through J6 pin one accepts the corresponding target output; pin two is ground. J1 joins MCU ground; neither pin is a supply. Confirm connector orientation and purchased board revision before wiring. No VDD conductor is routed on the carrier.

Inputs are restricted to owner-provided push-pull nominal 3.3 V logic with a verified common ground. They are not UART, CAN, RS-232, 5 V, 12 V or arbitrary service-port inputs. Power both sides appropriately before asserting signals; an unpowered MCU can be back-powered through input structures. The series resistors limit small currents but do not make excessive voltage safe. With a high input the nominal DC load is approximately 33 microamps per channel through the series/pulldown network, or approximately 131 microamps with all four high. These calculated figures exclude parasitic currents and are not measurements. Cable capacitance, leakage and ground offset must be evaluated on the assembled system.

Power the Nucleo through its supported ST-LINK USB arrangement. Reserve a conservative USB supply budget of 500 mA for the complete development module and bridge, subject to the actual board's power configuration and host capabilities; this is a supply planning allowance, not measured consumption or an extra carrier load. No battery or custom regulator is designed here. Verify board jumpers, solder bridges, USB power restrictions and module current using ST documentation and physical inspection before use.

Authoritative component references: ST UM1724, STM32 Nucleo-64 boards user manual; ST RM0368, STM32F401 reference manual; and ST DS9716, STM32F401xD/xE datasheet. Manufacturer pages are https://www.st.com/en/evaluation-tools/nucleo-f401re.html and https://www.st.com/en/microcontrollers-microprocessors/stm32f401re.html. Check the current document revision for the purchased board, especially A0–A3 mapping, PA2 virtual COM routing, supply selection and input voltage requirements. Firmware register definitions are local in registers.h and use no downloaded SDK.

## Architecture and firmware

    Owner's diagnostic adapter: TAG0 TAG1 ALLOCATE RELEASE
                  |
       J3–J6 -> series 1k -> sampled signal -> pulldown 100k -> GND
                  |
       J2 -> NUCLEO-F401RE PA0 PA1 PA4 PB0 input-only GPIO
                  |
       1 ms acquisition -> independent qualification -> resident ledger
                  |
       bounded CRC encoder -> USART2 TX -> ST-LINK USB serial
                  |
       Linux companion -> Ledger / History / Integrity -> private JSON

The tag bits qualify together as a bundle; the strobes qualify independently so chatter on one cannot hide the other. Qualification requires three unchanged intervals after the initial candidate sample. Hold the selected tag with both strobes low for twelve nominal milliseconds, assert exactly one strobe for six, then hold both low for six. These margins are for a deliberately slow adapter, not high-speed throughput claims. The engine also requires five setup observations and raw/qualified agreement at the strobe edge. Simultaneous strobes, unsettled tags and opposite raw strobes produce interface ambiguity rather than an invented ordering.

PA and PB are read separately. Acquisition is not atomic across those ports and is not precise edge timestamping. The first fully qualified low-strobe baseline enables TRACK. Held startup strobes do not establish an empty target. The operator must establish that separately. Sampling gaps terminate tracking because silently interpolating missing samples could invent age or miss an illegal return. Duplicate calls at the same timestamp do nothing; ordinary unsigned timer wrap is supported.

Each sample updates a copy of the ledger. Counter exhaustion discards the speculative copy and halts the retained state, preventing half-committed allocation or release. Invariants include accepted admissions equals completed returns plus occupancy; attempts equals admissions plus returns plus rejections; and sum of per-tag uses equals global admission ordinal. Latest admission stamps are unique and the newest two remain resident. The terminal state preserves these relationships while time and transmission diagnostics can continue.

The watchdog uses the independent oscillator with bounded startup waits; failed startup produces a terminal finding before normal evidence. The UART driver is nonblocking, bounded and timeout-aware. Status is attempted every 100 ms and compatibility policy every second; a single boot record reports reset flags without periodic history clearing. Busy transmission skips a status opportunity instead of delaying acquisition. A missed or late status is not automatically a missed sample. Acquisition coverage and transport availability are separate concepts.

## Protocol and companion application

Frames begin RT1, followed by B, P or S, comma-separated unsigned decimal integers, an asterisk, four uppercase hexadecimal CRC digits, then newline. CRC is CRC-16/CCITT with initial value FFFF over the body before the asterisk. B contains reset flags. P contains 3,1,5,4,2,1: qualification, nominal sample interval, setup observations, slot count, minimum later admissions and revision. Periodic P enables late serial attachment without pretending a new boot occurred.

S fields, in order, are now, levels, valid, halted, faults, phase, active, depth, stable, observed, rounds, attempts, launched, completed, rejected, gaps, missed, queued, peak, use0 through use3, stamp0 through stamp3, tx_fail. Masks use tag bits zero and one and strobes bits two and three. Fault values are early return 1, orphan 2, duplicate 4, interface 8, acquisition gap 16, exhaustion 32 and watchdog startup 64. Only the first terminal cause is retained. Rounds remains zero because this policy cannot drain; queued counts admissions while another resource remains present.

The dependency-free Python/Tk Linux desktop opens serial read-only. Run python3 app/console.py from this directory. The Ledger screen displays state, eligibility and findings; History retains the latest 256 accepted snapshots; Integrity displays compatibility, parser errors, reset evidence, freshness and connection provenance. New live starts a fresh host session but never resets the target. Disconnect retains evidence. Replay reads a bounded regular file; FIFO and device input are rejected to avoid blocking the UI. Export JSON creates a private mode-0600 file and refuses overwrite.

The app checks CRC, integer bounds, conservation, fault context, stamps and necessary sparse-transition constraints. It rejects impossible renewal before resident maturity and history migration. Sparse checks are not a proof of every hidden operation order. Acquisition advances and host/device timing are checked separately from receipt; duplicate timestamps cannot refresh progress. Reception pauses or excessive cumulative timing divergence freeze live interpretation. Without a persistent boot identifier, some resets remain indistinguishable from forward traffic. Exports retain these limitations through provenance, errors and connection state rather than presenting retained data as fresh acquisition.

## Build, verification and bench procedure

Build using make -C firmware all and run make -C firmware test. The target requires GNU Arm Embedded GCC and newlib; host tests use GCC with address and undefined-behavior sanitizers. Run python3 tests/test_app.py and xvfb-run -a python3 tests/test_gui.py for companion tests. Generate CAD with /usr/bin/python3 tools/generate_hardware.py and validate using /usr/bin/python3 tools/check_hardware.py with KiCad 9 bindings installed. The validator checks ERC, DRC, unconnected items and exact schematic/PCB/manifest pin, value and footprint agreement.

Before fabrication review the rendered schematic, connector orientation, copper and footprint dimensions. Before connecting a target measure continuity, resistor values, common-ground offset, thresholds and loading. Program only the intended Nucleo using its supported SWD tool and the linked firmware image. Compare qualification and gap behavior against a calibrated analyzer. Test power removal, watchdog recovery, USB unplug and input assertion while unpowered. These physical gates remain outstanding; passing host and CAD tests is not permission to skip them.

For a controlled research run, record the target revision, adapter source, identity mapping, workload, observer image and wiring. Establish the empty baseline, start the observer, then exercise the documented legal rolling sequence and a deliberately premature release in an isolated fixture. Correlate the first terminal finding with independent target logs. Export before resetting anything. A remediation comparison must use the same observable boundaries and workload; different instrumentation can manufacture apparent improvement. Retain only authorized evidence and delete it according to the agreed retention policy.
