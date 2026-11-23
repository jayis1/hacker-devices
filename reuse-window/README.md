# Reuse Window — Per-Identity Rolling Admission Witness

Author and creator: jayis1. Hardware revision A. Protocol WN1, policy revision 1.
Firmware, application and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. Installed toolchains and standard KiCad footprints retain their upstream licensing and attribution.

Status: unbuilt engineering prototype design with target-linked firmware, a functioning Linux desktop companion and routed native KiCad sources. Software and CAD validation are described in VERIFICATION.md. Fabrication, flashing, electrical bring-up, calibrated timing and real target interoperability have not been performed. This is an observer, not a rate limiter, protective interlock or security certification.

## Purpose and practical value

Reuse Window checks a specific resource-lifecycle promise: each of four identifiable resources may be admitted at most twice inside any rolling window of 1000 acquired ticks. Each identity is independently exclusive while occupied. Releasing a resource makes it vacant, but does not erase its recent admission history. A third admission before the oldest retained admission expires produces a terminal finding. Expiration occurs before admission at the same acquired endpoint, so the exact boundary is accepted.

The motivating experiment is a small embedded service with recyclable privileged-work descriptors. The owner intends to limit repeated reuse of each descriptor, but a faulty implementation clears rate history when the descriptor is freed or when the pool drains. Another implementation counts calendar seconds instead of a sliding interval and permits a burst across a calendar boundary. The instrument observes the externally declared accepted allocations and releases, retains the two latest admission deadlines per identity, and distinguishes those errors from legitimate replenishment through elapsed observed time.

Four identities and a two-admission limit are deliberately small laboratory parameters, not production security recommendations. They support reduced reproductions using harmless dummy operations. They do not establish a safe rate for cryptographic work, stale-pointer mitigation, authentication, flash wear or any particular product. A product's actual policy must be justified separately. The adapter must emit indications for actual accepted operations, not attempted requests that the target correctly rejected.

For a free tag, its first and second admissions can occur close together, provided each earlier occupancy has been released. A third admission is denied by the observer while both earlier admissions remain inside the rolling window. Another tag can still be admitted independently. A long-lived occupied resource cannot be allocated again merely because its timers expired. Releasing other tags does not buy capacity. An empty pool does not clear deadlines. These distinctions make the device useful for diagnosing accidental shared-budget accounting and free-list reconstruction bugs.

## Repository-local distinction and disclosed reuse

The repository directory inventory and README titles were reviewed, including the detailed nearby contracts for Use Quota, Reuse Quarantine, Distance Renew and Burst Budget. Use Quota has nonrefundable lifetime limits: time cannot replenish them. Reuse Quarantine starts an exclusion timer at return, rather than retaining admission times. Distance Renew counts intervening operations rather than elapsed acquisition ticks. Burst Budget implements a shared token bucket with gradual refill; it does not implement independent two-entry sliding admission histories for named occupied resources.

The contribution here is the identity-bound rolling admission contract, exact expiration semantics, independently tested timestamp-history oracle, corresponding sparse telemetry checks and operator-facing evidence. Originality is relative to the inspected repository, not worldwide invention or patentability. Sliding-window rate limiting itself is established engineering practice. The passive carrier and platform code reuse jayis1-authored infrastructure already present in the repository; they are not presented as a newly invented circuit. Internal UQ-prefixed symbols are inherited implementation names, while the wire marker is WN1. This directory is self-contained and imports no sibling device at runtime.

This run recovered an interrupted implementation of the same authorized task rather than silently adopting unrelated user changes. Fresh builds and tests were performed against the recovered source. The completed package adds the missing operating specification and verification record so readers can distinguish tested software from unbuilt physical hardware.

## Attack surface, threat model and authorized use

Use only on equipment you own or have explicit written authorization to assess. Agree on diagnostic instrumentation, electrical attachment, identity mapping, workload limits, possible interruptions, recovery and evidence retention. Permission for a network assessment does not automatically authorize opening equipment or wiring test points. Start on an isolated low-energy development fixture with hazardous outputs and production dependencies disconnected.

The assessed surface is admission bookkeeping across resource allocation, release, idle periods and pool drain. An authorized red team can demonstrate a reduced rate-policy contradiction with owner-provided diagnostics. Security researchers can compare a correct implementation with a deliberately defective emulator. Penetration testers can repeat an agreed workload after remediation. The device does not generate that workload, issue target commands, exploit memory, collect credentials, install persistence or transmit radio signals.

The operator, diagnostic adapter, observer image, harness and workstation are trusted. Each two-bit identity must designate one resource, not a lossy hash or a class containing simultaneous members. Diagnostic strobes must preserve operation order. A malicious target can emit plausible indications while violating its internal policy. Therefore a clean trace is not proof of security, and a finding is not automatically an exploitable vulnerability. Corroborate relevant results with source review, functional tests and independent waveforms.

Receive-only describes the supplied firmware's target-facing GPIO configuration, not an immutable hardware data diode. Reprogrammed pins could drive through the resistors. There is no secure boot, signed update service, persistent boot identifier, encrypted transport, authenticated pairing, protected key store or tamper-resistant logging. CRC detects accidental corruption, not forgery. Physical SWD access can replace firmware. Keep the instrument and workstation under bench control and preserve image provenance separately when evidence integrity matters.

Do not connect this unqualified carrier to medical devices, vehicles, deployed access controls, industrial safety systems, mains-referenced circuits or unidentified wiring. Stop if attachment changes voltage, temperature, reset behavior or signal timing. A common-ground observer can disturb a target even when it sends no commands.

## Hardware specifications

The active module is an external ST NUCLEO-F401RE with STM32F401RET6, a Cortex-M4 MCU with 512 KiB flash and 96 KiB SRAM. Firmware uses reset-default nominal 16 MHz HSI, nominal one-millisecond SysTick and USART2 transmit at 115200 baud, eight data bits, no parity and one stop bit. PA2 sends telemetry toward the ST-LINK virtual COM bridge. USART reception is disabled. The Nucleo supplies regulation, decoupling, reset, SWD programming and USB connectivity; those facilities are not claimed as components on the passive carrier.

The custom carrier is 80 by 70 mm, two copper layers and nominal 1.6 mm thickness. Fourteen through-hole components provide thirty pads and nine nets. Tracks are 0.3 mm wide. The carrier connects by labelled leads, not by mechanically stacking as a Nucleo shield. There is no FPGA, radio, antenna, battery, charger, removable storage or analog sensor. No enclosure or mounting holes are supplied: provide insulated support, underside protection and strain relief.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, purchased revision checked |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness/support | 1 set | Short insulated leads and nonconductive mounting |

Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Headers use the standard KiCad Connector_PinHeader_2.54mm library. Check purchased body dimensions, lead diameter, drill fit and mating orientation before assembly. Supply availability and actual power consumption have not been measured. A suitable 5 V USB source is required for the purchased module; reserving 500 mA is a planning allowance, not a measured load or compliance claim. The carrier has no supply rail and does not power the target.

| Signal | Target header | MCU harness | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Pin two of each target header is ground. Both J1 contacts are ground only: neither is a power input. J2 has four signals and no ground, so a separate module-ground connection to J1 is mandatory. Nets ending in _IN are target-side nodes before their series resistors. The plain signal nodes have the individual 100 kOhm pulldowns and connect to the MCU.

Only known common-ground zero-to-3.3 V push-pull diagnostics are supported. There is no isolation, TVS protector, qualified overvoltage clamp, fuse or level translation. Five-volt, 1.8 V, negative, differential and unknown signals are unsupported. Power the observer before enabling target outputs; connect ground before signals and disable signals before removing observer power. Input protection can otherwise back-power the MCU. Disconnection tends low but cannot be identified separately from a genuine low signal.

Manufacturer references are ST UM1724 for module connectors, power and solder bridges; RM0368 for the STM32F401 register map; and the STM32F401xD/xE datasheet for electrical limits. Consult the actual purchased board revision before changing bridges. The Nucleo manual is https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf and the MCU product page is https://www.st.com/en/microcontrollers-microprocessors/stm32f401re.html . Revalidate physical pinout and USB serial routing before programming or attachment.

## Architecture and block diagram

```text
Authorized target outputs       Passive carrier         External Nucleo
TAG0 ---------------------------- 1k --+---------------- PA0
TAG1 ---------------------------- 1k --+---------------- PA1
ALLOCATE ------------------------ 1k --+---------------- PA4
RELEASE ------------------------- 1k --+---------------- PB0
                                       | per channel
                                      100k
                                       |
Target GND --------------------------- GND ------------- USB GND

GPIO reads -> continuity guard -> bundled tag / separate strobe qualifiers
           -> prior setup check -> occupancy + two deadlines per identity
           -> transactional counters -> WN1 CRC records -> bounded UART TX
           -> ST-LINK virtual serial -> Ledger / History / Integrity screens
```

GPIOA and GPIOB are read sequentially, not with an atomic cross-port latch. This is slow diagnostic instrumentation, not an instruction-speed logic analyzer. Begin with twenty milliseconds of stable tag, twenty milliseconds strobe high and twenty milliseconds both strobes low for each operation. Confirm waveforms independently before reducing margins. Pulses below the sample interval may be invisible. The observer cannot determine ordering inside one sample.

## Firmware semantics and design decisions

The tag bits qualify as one bundle, while allocation and release qualify independently. Each group requires three consecutive intervals of unchanged raw input. A raw change restarts that group's run. The tag must additionally satisfy the prior five-tick setup condition at a qualifying operation edge. The selected strobe must still be raw high, and the opposite strobe must be low in raw, previous qualified and current qualified states. Simultaneous rising strobes are counted as ambiguous attempts and halt the observer.

WAIT ends after complete input qualification with both strobes clear. That is only an electrical baseline. The operator must independently establish an empty target pool and a fresh experimental admission history. Silence cannot prove that target resources are free or that earlier admissions expired. Resetting the observer while retaining target history destroys correlation rather than granting fresh permission.

Each identity retains older and newer remaining deadlines. Zero means expired. On each continuous acquired tick the nonzero deadlines decrement before processing an operation. Admission requires vacancy and an expired older deadline. It shifts the previous newer deadline into the older position and starts a new 1000-tick deadline. The represented interval is half-open on the left: an admission exactly 1000 ticks old no longer counts. Release affects occupancy but not deadlines or cumulative use.

A duplicate allocation takes precedence over rolling-window violation. Release of a vacant identity is an orphan finding. First contradiction terminates semantic tracking while preserving prior accepted occupancy and history. This rejection is the observer's classification; the target may already have accepted the operation. Further telemetry can continue with advancing wall timestamps, but halted deadlines and accounting do not recover or replenish. Export and begin a genuinely new experiment rather than attempting to clear a fault through the app.

Every sample is computed in a temporary Ledger and committed atomically. Counter exhaustion discards speculative changes and halts the prior ledger. A missed acquisition tick halts rather than treating missing time as continuous qualification or allowing it to replenish permission. Unsigned tick differences support ordinary timer wrap; bounded countdowns avoid resurrecting ancient deadlines. Repeated calls at the same timestamp do not create observations. Watchdog startup is bounded and failures are reflected before initial telemetry. UART service is nonblocking, bounded and timeout-aware.

The source tree contains main scheduling, explicit register definitions, board drivers, CRC, serializer, startup vector table and linker script. No dynamic allocator, RTOS or vendor SDK is required. Rebuild the complete image when changing policy: the companion checks the exact policy announcement and cannot negotiate arbitrary limits. Do not change constants without updating host invariants, independent tests and this interpretation.

## Application and wire interface

Run `python3 reuse-window/app/console.py` on Linux with Tk installed. The companion uses the standard library and termios, with no network service or third-party Python package requirement. New live opens a read-only serial session, clears incoming buffered bytes and starts fresh host provenance. Disconnect retains evidence while removing connected status. Attempting a second connection is refused until the first is closed. A failed open leaves existing evidence available.

Ledger shows counters, occupied identities, cumulative uses, older/newer deadlines, findings and calculated eligible tags. Eligibility describes retained state, not a target authorization; it is not permission to proceed after HALT. History displays up to 256 accepted snapshots. Integrity shows provenance, parser errors, reset evidence, session freezes, reception age and connection state. Export JSON creates a new private file and refuses overwrite. The export contains metadata even when no complete experiment has occurred. Protect exported paths according to the engagement's retention rules.

Wire records are ASCII `WN1,kind,decimal-fields*CCCC` followed by LF. The uppercase four-hex-digit CRC covers the body before the asterisk using CRC-16/CCITT polynomial 0x1021 with initial value 0xffff. B contains reset flags. P contains `3,1,5,4,2,1000`: qualification, policy revision, setup, identities, per-window limit and window length. S contains the ordered fields in app/contract.py: state and accounting, four cumulative uses, four last-admission ordinals, older/newer deadlines for each tag and TX-failure count. Periodic P announcements permit late attachment without repeatedly pretending the observer reset. Status is scheduled about every 100 nominal milliseconds; policy about every second.

The parser bounds frames and numeric fields, resynchronizes at the complete marker, checks CRC and rejects impossible individual states. Sparse-transition checks constrain admission and release conservation, per-tag history, deadline aging and rolling capacity. These are necessary endpoint checks, not a complete reconstruction of hidden operations. A forged internally plausible stream is still possible. A reset may also remain indistinguishable if boot telemetry was lost and counters appear forward; no permanent boot identifier is provided.

The exported live flag means progressing, compatible serial telemetry, not valid physical acquisition or absence of a HALT. Inspect halted, valid, faults and phase separately. Duplicate timestamps do not refresh reception. Reception pauses and excessive host/device drift freeze a live session and require reconnection. A conservative bounded drift allowance can require reconnecting a healthy uncalibrated oscillator. Replay is always labelled replay, rejects nonregular or oversized files and counts an incomplete tail as a framing error. No replay can turn synthetic data into a hardware measurement.

## Building, testing and bring-up

From the repository root run `make -C reuse-window/firmware clean all test`. Requirements include GNU make, host GCC, Arm GNU Embedded GCC with newlib/nosys, and binutils. The output ELF and BIN are in firmware/build and are ignored by Git. The linker targets the external F401RE module. Host tests include an independent timestamp-list policy oracle, exact boundary and wrap cases, transport bounds, startup policy retransmission and watchdog failure. These are actual compiled tests, not assertions inferred from source size.

Run `xvfb-run -a python3 -m unittest discover -s reuse-window/tests -p 'test_app.py' -v` for parser, session and real Tk event-loop checks with synthetic serial PTYs. The C encoder produces the fixtures used by the app tests. Run `/usr/bin/python3 reuse-window/tools/check_hardware.py` with KiCad 9 Python bindings and kicad-cli to check ERC, DRC and exact schematic/PCB/manifest connectivity. Run `python3 reuse-window/tools/check_release.py` for authorship, word count, substantive production C count and whitespace checks. CAD generation is optional and overwrites generated design files; ordinary validation does not regenerate them.

Before physical use, inspect assembly and pin-one orientation, verify ground continuity and absence of shorts, and compare the cable map with the module manual. Program only an owned, disconnected Nucleo using its supported ST-LINK procedure; verify the image, reset configuration and HSI assumptions. Check diagnostic levels and observer timing with a separate instrument before connecting a target. First exercise a harmless synthetic fixture, then the owner-approved reduced reproduction. No flashing or physical measurement was performed for this release.

A useful bench acceptance sequence establishes a fresh empty baseline, admits and releases one tag twice, verifies that a premature third admission halts, and repeats in a fresh experiment at the exact expiration boundary. Repeat across all identities and with another identity remaining occupied. Separately interrupt acquisition, disconnect serial and test target-output removal. Save evidence before resetting and record the target adapter revision, workload, wiring and oscillator characterization. None of these future bench steps should be confused with the software/CAD results already executed.
