# Burst Budget — Replenishing Admission-Credit Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol BB1. Firmware and companion application: GPL-2.0-only. Hardware generation and checking utilities: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, an executable Linux desktop application, and routed native KiCad files. Physical fabrication, programming, electrical qualification, oscillator calibration, and deployment have not been performed. This is an observation instrument, not a rate limiter, protective interlock, safety controller, or certificate of security compliance.

## Purpose, overview, and originality

Burst Budget investigates a temporal policy that is easily lost between application code and peripheral behavior: a device may admit a short burst of privileged operations, but must replenish permission gradually rather than acquiring a fresh full allowance at every clock boundary. An implementation using calendar-second counters can admit two adjacent bursts around a boundary. An implementation that preserves fractional credit while already full can make another operation available earlier than intended. An independent observer makes these differences visible without collecting credentials or operation payloads.

The instrument receives four slow diagnostic outputs from an authorized development target. ADMIT pulses when a protected operation actually begins. THROTTLE indicates that the target considers its admission budget exhausted. RESET_N describes the target's reset state, and INHIBIT forbids admissions independently of available credit. The observer maintains its own integer token bucket, consumes one token per qualified admission edge, and reports excess admissions, forbidden admissions, missing throttle indications, and acquisition continuity loss. It never issues a request or prevents a target operation.

Repository directories and related README scopes were inspected before choosing this concept. Grant Ledger consumes individually granted single-use credit. Retry Ledger counts authentication failures until lockout. Dwell Budget measures rolling occupancy in a sample ring. Lease Watch expires heartbeat authorization, while Starvation Window measures waiting and peer bypasses. Burst Budget instead measures continuously replenished discrete admission credit, full-bucket fractional-credit discard, empty-bucket throttle response, and credit conservation across reset and observation loss. Its originality claim is local to this repository, not a worldwide patent or prior-art assertion. The passive carrier topology and small STM32 support layer intentionally reuse existing work authored by jayis1; the new measurement contract is not disguised as a new analog circuit.

Dedicated hardware is useful when the target's own logger is delayed, blocked, or located in the same task as the suspected limiter. A separate MCU observes the diagnostic outputs without relying on the target's reporting transport. This independence is limited: a malicious target can lie on those outputs, and a compromised observer can lie in its serial reports. Use source inspection and an independent logic analyzer to corroborate important findings.

## Authorized use, attack surface, and threat model

Use this instrument only on equipment you own or are explicitly authorized in writing to assess. Agree on the particular test points, diagnostic adapter, firmware revision, permitted workload, interruptions, and evidence-retention period. Permission to assess an application does not imply permission to modify its hardware. Do not connect to deployed access controls, medical equipment, moving machinery, road vehicles, emergency systems, mains-related circuits, or unknown field wiring. Disconnect hazardous actuators and production dependencies before developing the fixture.

The intended assessment surface is an admission limiter's integration with the operation it is supposed to regulate. Relevant defects include boundary-reset bursts, an off-by-one empty check, accidental credit retention after reset, refill running faster than intended, and privileged operations continuing under inhibit. The supplied software has no credential collection, exploit payload, brute-force client, network scan, persistence, radio transmission, or target command interface. Red-team use means a supervised laboratory demonstration of a policy defect, not unauthorized interference with an operational system.

Trust boundaries include the diagnostic harness, MCU GPIO sampling, bare-metal firmware, ST-LINK USB serial bridge, workstation parser, replay files, and exported report. The workstation and installed firmware are trusted. Input bytes are nevertheless parsed as potentially malformed. CRC catches accidental serial corruption; it is not authentication. There is no pairing secret, encrypted channel, secure boot, signed evidence, protected update service, persistent boot identity, or tamper-resistant storage. Physical SWD access can replace firmware. Keep the apparatus in a controlled bench environment and record image hashes independently when chain of custody matters.

Receive-only refers to the supplied target-facing firmware behavior. The GPIO hardware itself remains programmable and is not a one-way security barrier. The carrier shares ground with the workstation through the NUCLEO. Electrical loading, cable reversal, and power sequencing can affect the assessed target even without transmitted commands. Stop immediately if attachment causes resets, heating, or voltage changes. Do not describe an absence of findings as proof that a production system is secure.

## Hardware specification and bill of materials

The complete instrument consists of an external ST NUCLEO-F401RE development board, the supplied passive carrier, short insulated leads, and a Linux workstation. The STM32F401RET6 processor provides Cortex-M4 execution, 512 KiB flash, and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, a nominal one-millisecond SysTick, and USART2 at 115200 baud, eight data bits, no parity, one stop bit. It uses no PLL, RTOS, dynamically allocated memory, vendor SDK, or radio stack.

The NUCLEO supplies MCU decoupling, regulation, USB power handling, reset, SWD programming, and the ST-LINK virtual COM bridge. These circuits are external module facilities, not silently omitted parts of a standalone MCU PCB. The carrier is an 80 by 70 mm, two-layer, nominal 1.6 mm board with fourteen through-hole components, nine nets, 0.3 mm tracks, and a closed outline. It is jumper-connected rather than a mechanically mating shield. There are no mounting holes; use an insulating support and strain relief. No enclosure files are supplied.

| Reference | Quantity | Requirement |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE / MB1136 |
| R1, R3, R5, R7 | 4 | 100 kΩ, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Straight 1x2, 2.54 mm through-hole header |
| J2 | 1 | Straight 1x4, 2.54 mm through-hole header |
| Harness | 1 set | Labelled insulated signal/ground leads, preferably below 20 cm |

The resistor footprint is KiCad `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal`. The headers use the standard `Connector_PinHeader_2.54mm` vertical footprints. Verify purchased component dimensions and mating clearances before assembly. There are no batteries, chargers, FPGA, antennas, active sensors, storage cards, target power outputs, or isolated supplies. Omitting unnecessary interfaces reduces both complexity and accidental exposure.

## Connector mapping and electrical limits

| Signal | Target connector | MCU harness | NUCLEO point | Firmware bit |
| --- | --- | --- | --- | --- |
| ADMIT | J3 pin 1 | J2 pin 1 | CN8 A0 / PA0 | 0 |
| THROTTLE | J4 pin 1 | J2 pin 2 | CN8 A1 / PA1 | 1 |
| RESET_N | J5 pin 1 | J2 pin 3 | CN8 A2 / PA4 | 2 |
| INHIBIT | J6 pin 1 | J2 pin 4 | CN8 A3 / PB0 | 3 |
| GND | J3–J6 pin 2 | J1 both pins | NUCLEO GND | not applicable |

J1 is ground only. Never attach a supply to either pin. Names ending in `_IN` are target-side nets before their series resistor; plain signal names are MCU-side nodes with individual pulldowns. The square footprint pad marks pin one. RESET_N is active low; the other inputs are active high. No polarity configuration command exists.

Only common-ground, nominal zero-to-3.3 V push-pull diagnostic outputs are supported. At nominal 3.3 V, each 1 kΩ/100 kΩ network draws approximately 32.67 µA and presents approximately 3.267 V to its MCU input, ignoring leakage and resistor tolerance. Four high channels dissipate about 0.431 mW in their resistor networks. These are calculated DC values, not measured prototype results. The MCU module and USB bridge dominate consumption. Reserve a conservative 5 V, 500 mA bench USB source budget; this is not a measured current claim or a USB compliance result.

There is no galvanic isolation, TVS protection, qualified overvoltage clamp, reverse-current blocker, fuse, or level translator on the carrier. The series resistor is not a safety barrier. Do not connect 5 V, 1.8 V, negative voltages, RS-232, differential field buses, relay coils, or industrial 24 V signals. Power the observer first, check shared-ground compatibility, connect ground before signals, remove signals before powering down, and never leave an energized target connected to an unpowered MCU. Input protection structures can otherwise back-power the module.

The connector mapping follows ST UM1724 Table 16. Its USART guidance describes the normal virtual-COM arrangement with SB13/SB14 on and SB62/SB63 off; verify the actual board revision and solder bridges. Firmware configures PA2 for USART2 transmit toward ST-LINK, not toward the target. PA3 is not configured as a receiver. Register definitions follow the STM32F401 RM0368 peripheral map, not a generic similarly named MCU.

## Architecture and block diagram

```text
Authorized 3V3 diagnostic fixture       Passive carrier A
 ADMIT ------ J3 --- 1k ---+---------- PA0 / A0
 THROTTLE --- J4 --- 1k ---+---------- PA1 / A1
 RESET_N ---- J5 --- 1k ---+---------- PA4 / A2
 INHIBIT ---- J6 --- 1k ---+---------- PB0 / A3
                          | each node has its own 100k pulldown
 Common GND --------------+---------- NUCLEO GND ---- USB host GND

 GPIO samples -> independent qualifiers -> token bucket / throttle check
             -> conservation validation -> BB1 status and policy frames
             -> bounded USART2 TX -> ST-LINK virtual COM -> Linux desktop
                                                          Budget tab
                                                          History tab
                                                          Integrity tab
                                                          private JSON
```

GPIOA and GPIOB are read sequentially. They are not simultaneously latched and inter-port skew has not been measured. Qualified observation times are not exact electrical edge timestamps. Do not infer submillisecond precedence or use workstation receipt time as a substitute for acquisition time. Short pulses can be missed entirely; this instrument is intended for deliberately slow diagnostic adapters rather than high-speed bus taps.

## Exact firmware measurement contract

Each channel is qualified independently. After a candidate changes, it must remain unchanged for three further one-millisecond samples before becoming qualified. Thus a change requires the initial observation plus three matching observations. Qualification on one line is not postponed by chatter on another. Use high and low pulse durations of at least ten milliseconds when constructing the adapter. Initial attachment to an already-high ADMIT line does not establish a rising edge and is not counted as a new admission.

Startup begins UNKNOWN with zero credits. A new prospective observation segment starts after all inputs are qualified and twenty evaluated quiet ticks have occurred. Quiet means ADMIT low, RESET_N high, and INHIBIT low in both the qualified vector and current raw vector. THROTTLE may be high because an empty bucket legitimately asks the target to throttle. The resulting TRACKING state starts with zero tokens and zero fractional progress. It does not assume a historical full bucket. Allow four seconds of continuous tracking before the first full-capacity burst experiment.

Capacity is four integer tokens. Each thousand continuously evaluated tracking milliseconds mints one token, unless the bucket is full. While full, fractional progress is discarded. The tick that consumes from a previously full bucket does not count toward the next refill interval. This avoids banking nearly complete refill progress while full. INHIBIT forbids admissions but does not stop replenishment during an otherwise valid segment; this is an explicit policy choice, not a universal limiter recommendation.

Each qualified ADMIT rising edge is assigned exactly one category. Outside TRACKING, or with qualified INHIBIT high, it is denied. Inside TRACKING with no credit, it is excess. Otherwise it is allowed and consumes one token. Denied and excess admissions are observations of a violation, not claims that the target actually rejected work. They do not create negative debt or consume nonexistent credit. Held-high ADMIT consumes only once. Reset assertion is processed before admission on the same qualified vector; an admission coincident with reset is denied.

Admission is evaluated before refill. If an empty bucket becomes eligible to mint a token on the same evaluated tick as an admission edge, that admission remains excess and the newly minted token is retained for later work. This conservative endpoint rule does not pretend that same-tick observations establish a favorable physical ordering. Likewise, throttle absence is evaluated before refill rather than erased retroactively by a token appearing at that endpoint.

While tracking with an empty bucket, an empty-age counter advances to twenty evaluated ticks and saturates there. On the twentieth empty evaluation and later, qualified THROTTLE must be high. Every missing evaluation increments the missing-throttle sample count and sets a sticky finding. When consumption creates emptiness, that tick is the first empty evaluation. Refilling clears empty age. Excessively early throttle is not a finding: the target may conservatively throttle for another reason. THROTTLE does not grant or revoke credits.

A sample-time jump other than one millisecond records a gap, discards remaining tokens, resets qualification, and returns to UNKNOWN. Unobserved time earns no credit. A qualified low RESET_N also discards tokens and returns to UNKNOWN. A new baseline is mandatory before replenishment resumes. Lifetime findings and counters remain until the observer itself resets. Credit conservation is explicit: minted equals allowed plus discarded plus currently held tokens. Admission conservation is admissions equals allowed plus excess plus denied. These equations are checked before serialization and in the companion parser.

All accounting counters are unsigned 32-bit. Rather than silently wrapping, a prospective overflowing increment enters HALTED before mutating its accounting group. HALTED preserves acquisition and accounting fields while device time and transmitter diagnostics can still progress. The millisecond timestamp intentionally wraps modulo 32 bits. RAM contents are not persistent across observer resets; there is no flash journal, historical restoration, or command to clear findings.

## Runtime, transport, and application

The main loop samples before servicing telemetry. USART transmission copies a bounded frame into its own buffer, writes at most one byte per poll when TXE is set, and abandons a blocked transfer after one hundred milliseconds. It never waits for the host. The independent watchdog has bounded initialization waits; failure creates a terminal fault before status transmission. Normal operation feeds it from the main loop. Watchdog timing depends on uncalibrated LSI and must be bench-tested. Status is scheduled approximately every hundred milliseconds. A boot record is sent once, while policy is repeated approximately every second so late attachment can establish compatibility without falsely declaring a new session.

BB1 frames are ASCII `BB1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC is four uppercase hexadecimal digits using CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xffff, no reflection or final XOR, over bytes before the asterisk. B contains MCU reset flags. P contains `4,1000,20,3,20`: capacity, refill period, empty grace, qualification intervals, and baseline ticks. S contains eighteen unsigned fields in this order:

`now, phase, levels, qualified, tokens, refill, empty_age, faults, admissions, allowed, excess, denied, missing, gaps, segments, tx_fail, minted, discarded`.

Phase values are UNKNOWN zero, TRACKING one, HALTED two. Fault bits zero through six mean excess admission, denied admission, missing throttle, acquisition gap, transmit failure, exhausted counter, and watchdog startup failure. Every accepted record is below 384 bytes. There is no inbound command protocol and no remote management endpoint.

The companion uses Python's standard library and Tk because a local Linux serial desktop is appropriate for this bench workflow. Run `python3 app/console.py` from this directory. Select the exact virtual-COM path, then New live session. Budget shows credits, refill progress, admission categories, missing throttle, and named findings. History shows the retained snapshots, limited to the latest 256. Integrity shows provenance, connection, compatibility, parser errors, boot flags, freshness, and freeze reasons. Disconnect preserves evidence; replacing a session intentionally clears it, so export first.

Freshness requires a connected live source, compatible policy, and advancing accepted device time within one second. Identical duplicates do not renew freshness. A prolonged receive pause freezes the session rather than accepting queued old frames as live. Clock divergence beyond a 500 ms allowance plus five percent of host elapsed time also freezes it. This generous drift allowance is not a synchronization guarantee. Counter rollback, disappearing sticky findings, contradictory duplicates, and acquisition changes after HALTED freeze the session. Conservation, ranges, increment limits, and previous fractional refill progress reject selected impossible records, but are not a proof that every accepted summary has a realizable underlying waveform.

Replay reads only regular files, opened nonblocking, and caps input at two MiB before allocation. A truncated EOF tail is counted as an error. Empty recordings fail. Live framing resynchronizes on the full BB1 marker after a partial frame. Reports include author, protocol, provenance, connection, freshness, parser errors, warnings, boot metadata, status, and history, even when no admissions occurred. Export creates a new file exclusively with mode 0600 and refuses overwrites. There is no upload or hidden database. Delete reports with approved workstation retention tools; exports are neither signed nor encrypted.

Some observer resets remain indistinguishable if their boot packet is lost and later counters and timestamps happen to progress. A physically controlled connection, known image, supervised reset procedure, and independent evidence are therefore essential. Reconnect after any transport discontinuity, and do not claim continuous attested measurement across resets.

## Build, tests, and design inspection

Required tools are GNU Make, ARM GNU GCC with newlib/nosys support, host GCC, Python 3 with Tk, KiCad 9 CLI/Python bindings and standard footprints, and Xvfb for headless GUI tests. No package download or application dependency install is needed. From this device directory:

```sh
make -C firmware all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 -m unittest discover -s tests -p test_gui.py
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
```

The target build includes startup vectors and a linker script for flash at 0x08000000. Program using an authorized ST-LINK workflow and hardware reset into the documented HSI clock configuration. Do not use debugger scripts that silently switch clocks. Host policy tests use sanitizers and include burst exhaustion, refill, exact-boundary precedence, inhibit, reset, independent qualification, gap recovery, timestamp wrap, and counter exhaustion. UART tests use mapped host memory, not a physical peripheral. The GUI test uses a real Tk event loop and Linux PTY with production C-encoder packets, exercises screens, private export, overwrite refusal, disconnection, replay, and reconnection.

The CAD checker executes ERC and DRC with violations treated as errors, exports a fresh XML schematic netlist, and compares all references, values, footprint identities, pad numbers, and nets against the PCB and connectivity manifest. Real embedded symbols and footprints are supplied. Passive symbols cannot validate the external MCU supply, input voltage, or module circuitry. Rule-clean CAD is not proof of manufacturability or electrical qualification. Inspect rendering, connector orientation, purchased dimensions, soldering access, and board support before ordering.

## Practical research workflows and remaining validation

For an authorized red-team demonstration, instrument a sacrificial development target with the four diagnostic signals. Reset the target to a documented state, observe a fresh baseline, wait for four credits, and issue a short burst through the target's own test interface. A fifth admitted operation before replenishment should appear as excess. Record target logs and an independent logic trace alongside the exported observation. The observer itself generates no workload or attack traffic.

For product security review, compare a fixed-calendar limiter with the specified continuously replenished model using bursts near the calendar boundary. Then test reset during partial depletion and inhibit during otherwise available credit. For penetration-test reporting, show the meaning of every diagnostic wire, expected policy, exact target revision, qualified-sampling limitations, and remediation retest. Never equate an indicated admission with a proven unauthorized data access without separate evidence.

Before deployment, fabricate and inspect a carrier, verify continuity unpowered, bring up the NUCLEO without a target, measure loading and thresholds, confirm clock and serial accuracy, exercise watchdog recovery, and test attach/detach power sequences. Validate short pulses, asynchronous boundary timing, reset behavior, host stalls, and cable failure against a calibrated analyzer. ESD, EMC, environmental, USB, and regulatory qualification remain outstanding. See VERIFICATION.md for executed software and CAD checks rather than assumed physical success.

## Component references

- ST UM1724, STM32 Nucleo-64 boards MB1136: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf — Table 16 pin map and USART solder-bridge guidance.
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf — GPIO, RCC, USART and watchdog register definitions.

Author of this design, firmware, application, and documentation: jayis1.
