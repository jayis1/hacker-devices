# Alias Depth — Bounded Reference-Multiplicity Evidence Bench

Author and creator: jayis1. Hardware revision A. Wire protocol AD1, policy revision 1.

Status: unbuilt engineering prototype. This directory provides target-linked C firmware, a functioning Linux desktop companion, and routed native KiCad carrier files. Executed checks are recorded in VERIFICATION.md. No physical fabrication, MCU programming, electrical qualification, oscillator calibration, or real-target experiment is claimed. This instrument observes a declared diagnostic contract; it is not a memory-safety boundary, reference-count implementation, protective interlock, or security certificate.

Original firmware, application, and tests: GPL-2.0-only. Original CAD utilities: MIT. Original carrier design: CERN-OHL-S-2.0. External tools, purchased modules, and standard component libraries retain their own provenance and licenses.

## Purpose and practical value

Alias Depth measures the multiplicity of outstanding references to four named objects. A reference-counted buffer can legitimately have several simultaneous users, so an exclusive-ownership alarm produces false positives. Conversely, a single live/not-live bit cannot tell whether a callback released the last reference or only one of several aliases. Alias Depth reconstructs a bounded integer for each object from deliberately slow RETAIN and DROP indications, preserves each object's cumulative retain history, and stops at the first underflow or declared ceiling breach.

A motivating experiment is a privileged embedded service that passes the same decoded request buffer through validation, audit preparation, and response completion. The reduced test contract permits at most three outstanding references to each tracked object. An error path might run cleanup twice, or a repeated callback might accumulate another alias after all three sanctioned holders already exist. Ordinary aggregate logs can obscure which object owns the outstanding credit. The external observer retains the implicated tag and coherent pre-operation counts without reading buffer contents, addresses, keys, credentials, or customer identities.

The device is useful when the owner can instrument a development target and serialize actual committed reference changes. It does not discover reference operations on an unmodified processor, snoop a memory bus, or reconstruct invisible aliases. The adapter is part of the measurement chain. A report that occurs before the actual reference change, a silently overflowing adapter queue, or a reused two-bit identifier while the old object remains alive invalidates the interpretation. Slow diagnostic instrumentation can also remove a race being investigated; in that case use a different acquisition method.

## Exact contract and distinguishing examples

Four object tags, zero through three, each begin with zero references. RETAIN adds exactly one reference to the selected object if its old count is below three. An object's first RETAIN opens its lifetime; subsequent RETAIN operations deliberately name the same live object. DROP removes exactly one reference if its old count is positive. Only the last DROP removes the object from the live-object mask. An object that has reached zero can immediately begin a new lifetime. Other objects do not need to drain, and there is no stack, FIFO, allocation-order, recency, or fairness requirement.

The ceiling is per object, not a shared pool budget. Three references on each of all four objects is legal. Four on one object is illegal even if every other object has zero. The three-reference ceiling is an explicit small-fixture policy, not a discovered property of arbitrary software or the numerical capacity of a target counter. Targets whose supported contract exceeds that ceiling are incompatible with this policy revision. Do not relabel their ordinary behavior a vulnerability.

For example, RETAIN zero, RETAIN zero, DROP zero leaves object zero live with one reference. RETAIN two followed by DROP zero is legal and leaves only object two live. RETAIN zero three times followed by a fourth RETAIN zero produces a ceiling finding and preserves three references, rather than publishing the rejected fourth reference. DROP two from the all-zero baseline produces underflow. RETAIN one, DROP one, RETAIN one is legal reuse and increases its cumulative retain counter to two.

The instrument does not identify individual holders. If a target drops the wrong alias while another reference to the same object exists, arithmetic can remain plausible and no finding is guaranteed. Nor does a transition to zero prove that memory was freed, erased, or made inaccessible. There is no separate reclamation signal. A missing DROP can remain as a nonzero count without exceeding the ceiling; this version has no lifetime timeout and does not automatically call that a leak. These limits are essential to interpreting a clean trace.

## Repository-local originality and platform reuse

The configured repository is jayis1/hacker-devices. Its remote default branch was resolved as main and pulled before design work. Device directories and README purpose titles were inventoried. Searches for reference counting, reference multiplicity, and alias depth were followed by inspection of the closest ownership contracts. No equivalent per-object multiplicity witness was found in the inspected repository. This is a repository-local contribution, not a claim of worldwide novelty or patentability; reference counting is established engineering.

Slot Alias rejects a second outstanding allocation with the same tag. Alias Depth explicitly accepts it and retains the object after a partial drop. Read Share represents distinct reader clients and one exclusive writer, and still rejects duplicate acquisition by a client. Alias Depth's tags are objects, not clients or access modes. Context Stack associates returns with nesting; Alias Depth permits arbitrary inter-object drop order and repeated same-tag acquisition without a global nesting relation. Bank Pack constrains allocation destination by bank occupancy; reference multiplicity here has no bank selection rule.

The carrier topology, STM32 board support, startup, bounded UART driver, CRC framing, qualification logic, and desktop transport reuse jayis1-authored Bank Pack infrastructure. The per-object multiplicity policy, live-reference telemetry, sparse per-object conservation checks, reference display, handle-token oracle, and experiment contract are specific to this device. Internal UQ-prefixed identifiers and generic counter names are retained platform implementation details. AD1 and its policy tuple define the external interface. The directory is self-contained and imports no sibling device at runtime.

## Attack surface, threat model, and legal disclaimer

Use only on systems you own or have explicit written authorization to assess. Permission must cover physical attachment, diagnostic instrumentation, firmware changes, approved workloads, expected interruption, recovery, and evidence retention. Network testing permission does not automatically authorize opening an enclosure or changing embedded firmware. Begin on a low-energy laboratory fixture with consequential outputs disconnected. Do not use this unisolated prototype on deployed door controls, vehicles, medical equipment, dangerous machinery, mains-referenced circuits, or unknown field wiring.

The investigated attack surface includes reference accounting across callback completion, cancellation, retries, error cleanup, and shared-buffer handoff. An authorized red team can ask an owner to expose a suspected duplicate cleanup through a harmless reduced workload. Security researchers can compare cleanup implementations and interleavings. Penetration testers can rerun an approved regression after remediation, attaching qualified evidence rather than assertions inferred from a software log alone. A diagnostic policy violation is a candidate implementation defect; source review and independent functional evidence are still required to establish exploitability or impact.

Trusted components are the adapter, harness, MCU image, purchased module, operator, and workstation. A malicious adapter can lie or omit every operation. Serial data and replay files are parser inputs, but CRC only detects accidental corruption; it is not authentication, a signature, or replay protection. The design has no secure boot, persistent boot identity, cryptographic attestation, encrypted storage, trusted wall clock, network listener, remote updater, or cloud upload. Access to ST-LINK permits firmware replacement. Protect the workstation and programming port physically and retain exports according to engagement policy.

Receive-only means this firmware configures target-facing GPIO as inputs and disables UART reception. The companion opens the serial endpoint read-only and sends no commands. It is not an irreversible electrical diode: replacement firmware could drive those pins. Input loading, shorts, ground offsets, and back-power remain possible even with correct software. Keep the device visible, label the harness, agree on a stop procedure, and disconnect if attachment changes the target's behavior. Absence of a finding does not prove complete observation, safety, or security.

## Hardware specifications and bill of materials

The compute module is an external ST NUCLEO-F401RE containing an STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware assumes nominal reset-default 16 MHz HSI operation. SysTick supplies one-millisecond acquisition ticks. USART2 TX uses PA2 at 115200 baud, eight data bits, no parity, one stop bit, through the module's ST-LINK virtual COM bridge. The purchased module supplies regulation, decoupling, MCU, reset, USB conversion, and SWD access; those functions are not missing parts of an allegedly standalone carrier.

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm tracks. Fourteen through-hole components provide thirty electrical pads and nine nets. The board connects through short labelled leads, not as a stacking shield. There is no radio, antenna, FPGA, analog sensor, battery, charging circuit, removable storage, isolated supply, or target-power output. Provide a nonconductive base, strain relief, and underside cover; enclosure models and mounting holes are not included.

| Reference | Quantity | Candidate specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE; verify actual board revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header |
| J2 | 1 | 1x4 vertical 2.54 mm header |
| Harness and support | 1 set | Insulated leads and nonconductive mounting |

Resistors use 7.62 mm formed lead spacing. Standard KiCad axial resistor and vertical pin-header footprints are embedded on the PCB with full library identities. Check purchased part dimensions, lead diameter, drill fit, connector mating height, and resistor rating before fabrication. No current stock, price, yield, or production qualification is claimed. Reserve a 5 V source capable of 500 mA for development-board planning, subject to its documented USB/jumper configuration; that is headroom, not a measured operating or standby current.

| Signal | Target pin | Carrier-to-MCU pin | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| RETAIN | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| DROP | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target connector's second pin is ground. Both J1 pins are ground only, never supply inputs. J2 has four signals and no ground, so a separate module-ground lead to J1 is mandatory. Names ending `_IN` are target-side nodes; unsuffixed signal names are MCU-side nodes after the series resistor. Each MCU node has a 100 kOhm pulldown. A disconnected input therefore looks low and cannot establish cable continuity. The nominal DC resistor path to ground is 101 kOhm before leakage or source impedance.

Support only verified common-ground, push-pull zero-to-3.3 V diagnostic signals. Exclude negative, 5 V, 1.8 V, differential, mains-related, and unknown-voltage signals. The 1 kOhm resistor is not qualified surge or ESD protection. There is no level translator, dedicated clamp, fuse, galvanic isolation, or hot-plug guarantee. Power the observer before enabling target outputs; disable those outputs before depowering it. USB ground and target ground become common. Physical validation must establish loading, thresholds, cable capacitance, leakage, ground offset, temperature, and power-sequencing behavior.

## Architecture and block diagram

```text
Owned target diagnostic adapter     Passive carrier          External Nucleo
TAG0 ------------------------------- 1k --+------------------- PA0
TAG1 ------------------------------- 1k --+------------------- PA1
RETAIN ----------------------------- 1k --+------------------- PA4
DROP ------------------------------- 1k --+------------------- PB0
                                          | each MCU node
                                         100k
                                          |
Target ground --------------------------- GND --------------- module/USB GND

GPIO -> continuity guard -> bundled tag / independent strobe qualification
     -> prior setup and opposite-strobe checks -> per-object reference counts
     -> transactional accounting / first fault -> bounded AD1 CRC snapshots
     -> USART2 TX -> ST-LINK USB -> Ledger / History / Integrity -> private JSON
```

GPIOA and GPIOB are read separately, not latched simultaneously. This is a deliberately slow diagnostic instrument, not a processor-speed bus analyzer. An owner-provided adapter must serialize committed reference changes without changing their order or silently dropping records. The target may need a special reduced workload rather than a production-rate stream. Establish and document the relation between each two-bit tag and a single tracked object; hashing arbitrary concurrent objects into four buckets is invalid.

## Firmware design, acquisition, and failure behavior

C11 firmware uses no RTOS, heap, network stack, or external SDK. The startup vector installs reset and SysTick handlers, initializes data and BSS, and enters the acquisition loop. The linker specifies flash and SRAM. Board support configures GPIO, captures reset flags, starts SysTick, sets up TX-only USART, and attempts independent-watchdog startup. The main loop samples when time advances, services at most one UART byte per poll, publishes status every nominal 100 milliseconds, and repeats the policy announcement every nominal second for late serial attachment.

The two tag bits qualify together as a bundle. RETAIN and DROP qualify independently, so chatter on one strobe does not indefinitely postpone the other. A candidate followed by three matching intervals establishes qualification. A valid operation requires five prior stable tag observations, raw/qualified tag agreement at acceptance, and the incoming strobe still raw-high. Its opposite strobe must be low in the previous qualified, current qualified, and current raw states. Simultaneous rising strobes are an interface fault, not an invented operation order. Holding a strobe high produces one event.

Begin with all four target reference counts independently established as zero, then restart the observer. Quiet strobes establish acquisition readiness but cannot prove an empty target. For a conservative diagnostic waveform, hold the selected tag with both strobes low for twelve nominal milliseconds, one strobe high for six, and both low for six. Tests use this waveform. These are software intervals, not calibrated electrical limits. Pulses shorter than qualification can disappear; a raw glitch is not automatically a recorded target operation.

Each sample applies to a speculative copy of the ledger. If a counter cannot advance, the complete sample update is discarded and prior coherent evidence becomes terminal. RETAIN increments the selected count, total outstanding depth, per-tag cumulative retains, global retain ordinal, and that tag's last-retain stamp. DROP decrements only the selected count and total depth. The live mask clears that tag only at zero. A global pool-drain counter advances when the final reference across every object disappears; it is not the number of individual object destructions.

`launched` means accepted retains; `completed` means accepted drops; `queued` means accepted retains while any reference already existed; `rounds` means full-pool drains. `uses[]` and `stamp[]` retain history through partial drops, individual zero crossings, and full drain. `depth` is the sum of references, not population of the live-object mask. `peak` is the maximum observed total reference depth. This vocabulary is documented because confusing objects with aliases would defeat the purpose of the instrument.

Repeated timestamps are ignored. A skipped acquisition tick halts correlation rather than treating the missing time as stable input. Ordinary unsigned timestamp wrap is supported. Interface faults precede semantic checks. Fault bits are ceiling 1, underflow 2, reserved 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32, and watchdog startup failure 64. Only one reason is retained; reserved bit 4 is rejected. Halt clears qualification and freezes measurement history while transport time and TX-failure diagnostics may continue. A retained count after a fault describes the last coherent observation, not the target's current state.

## Companion application and wire interface

Run `python3 app/console.py` on Linux with Tk support. No third-party Python packages are required. Enter a verified serial path and select New live. There is no device-discovery sweep or automatic probing. Ledger shows per-object reference multiplicities, current eligible RETAIN tags, live objects, cumulative counts, stamps, and findings. History displays up to 256 accepted snapshots. Integrity retains source, provenance, parser errors, reset information, compatibility, freshness, and freeze reason. Eligibility is descriptive evidence, never an authorization to manipulate the target.

AD1 records are comma-separated ASCII integers followed by an asterisk, four uppercase hexadecimal CRC digits, and LF. CRC-16/CCITT starts at 0xffff and covers the bytes before the asterisk. B records contain reset flags. P records contain `[3,1,5,4,3,1]`: qualification intervals, nominal sample period, prior setup observations, object count, per-object reference ceiling, and policy revision. S field order is defined in `app/contract.py` and emitted by `firmware/wire.c`. Failed serialization returns no record rather than submitting a partial frame.

Snapshot checks require bounded per-object counts, exact live-mask membership, unique retained ordinals, plausible semantic findings, and conservation. Accepted retains equal accepted drops plus total depth and equal the sum of cumulative per-object retains. Attempts equal accepted retains plus drops plus rejected edges. Across sparse snapshots, each object's inferred drops equal its new retains plus its old reference count minus its new reference count. Those inferred drops must be nonnegative and sum to the global drop increment. This rejects movement of reference credit between objects that balanced aggregate totals would miss. Stamps may change only with a corresponding retain, and peak growth needs sufficient new retains.

These checks are necessary endpoint conditions, not reconstruction of every hidden operation. A malicious CRC-valid stream can still hide an intermediate ceiling excursion followed by a compensating drop. Firmware observes every acquired qualified operation, but periodic telemetry does not send the complete raw waveform. Findings depend on trusting that firmware and adapter. There is no implied cryptographic evidence chain.

Two advancing compatible records are needed for live progress. Duplicate timestamps do not refresh freshness. Contradictory duplicates, rollback, later boot records, impossible transitions, long reception pauses, and excessive cumulative host/device clock divergence freeze interpretation. Drift allowance is capped. Some resets remain indistinguishable without a persistent boot identity; uncalibrated oscillator drift can conservatively reject an otherwise healthy session. A halted device can continue sending fresh transport records without performing fresh acquisition.

Disconnect preserves evidence and restores terminal settings. A second connection is refused until the first closes. Replay accepts regular files up to two MiB, rejects devices and FIFOs, and counts a truncated EOF tail. Framing resynchronizes on the complete AD1 marker. JSON export creates a new owner-only file, refuses overwrite, and includes provenance and integrity even for empty sessions. Exports are neither encrypted nor signed, and nothing is uploaded automatically. Reconnecting does not reset embedded state or establish a new empty target baseline.

## Build, verification, and physical bring-up

From this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

ARM GNU GCC with newlib/nosys support builds the actual Cortex-M4 ELF and binary. Native GCC runs address/undefined-sanitized engine tests and register-model UART tests. Python tests exercise real C-encoder fixtures, parser rejection, desktop screens, pseudoterminal transport, disconnect, replay, and private export. KiCad 9 with distro Python bindings and standard footprints performs ERC, DRC, and exact schematic/PCB/manifest comparisons. Use `/usr/bin/python3 tools/generate_hardware.py` to regenerate CAD; build products and personal KiCad preferences are ignored.

| Requirement | Implementation | Verification approach |
| --- | --- | --- |
| Bounded aliases per object | `refs[]`, eligibility, last-drop clearing | Independent handle-token oracle and all reference vectors |
| No cross-object credit | Uses, stamps, sparse reference deltas | CRC-valid impossible migration fixtures |
| Safe loss behavior | Gap guard, speculative updates, halt | Boundary, overflow, timestamp-wrap tests |
| Useful local evidence | Three desktop screens and JSON export | Real event loop, PTY, replay, overwrite refusal |
| Electrical consistency | Native symbols, footprints, routed nets | ERC, DRC, fresh XML-to-pad comparison |

Before assembly, inspect the schematic rendering, PCB, actual footprints, and development-board revision. Manufacturer references are ST UM1724, particularly its NUCLEO-F401RE connector table and USART section, RM0368 for peripheral registers, and the STM32F401xD/xE datasheet for electrical limits. UM1724 identifies CN8 pins one through four as PA0, PA1, PA4, and PB0. Its default VCP arrangement has SB13/SB14 on and SB62/SB63 off. Reference: https://www.st.com/resource/en/user_manual/um1724-.pdf . Confirm the purchased board rather than relying only on this description.

Check an unpowered carrier for shorts, resistor values, and pin-one orientation. Power the observer with the target disconnected, verify telemetry and watchdog behavior, then enable a known low-energy diagnostic fixture. Program through the module's supported SWD process after reviewing power configuration. Firmware assumes reset-default clocks rather than arbitrary bootloader leftovers. Compare diagnostic waveforms against an independent instrument, including repeated same-tag retain, partial drop, last drop, interleaved objects, full drain, intentional synthetic underflow, and the exact three-to-four ceiling boundary.

Record tag mapping, adapter revision, firmware revision, approved workload, and physical test limitations with each export. Confirm that retained counts after a fault are not displayed as current target truth. Software builds and clean CAD checks do not replace fabrication review, electrical bring-up, timing calibration, ESD testing, or regulatory assessment. A successful trace establishes only that the accepted diagnostic evidence met this explicitly limited reference-multiplicity contract.
