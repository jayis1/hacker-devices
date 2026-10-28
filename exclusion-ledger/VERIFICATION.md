# Exclusion Ledger verification record

Author: jayis1.

## Scope

Standalone-owner implementation and verification. Advisory reports informed the acceptance gates; they were not independent reviews of these completed artifacts. This record describes software execution and CAD checks, not fabrication, programming, or electrical validation. No physical target or development board was connected.

## Executed checks

- Repository identity: origin is https://github.com/jayis1/hacker-devices.git. Remote HEAD resolves to main. Required initial `git pull --ff-only` completed with “Already up to date.” Only exclusion-ledger files were introduced; no task branch was created.
- Inventory: existing directories and README introductions reviewed; mutual-exclusion search and detailed Starvation Window contract reviewed to distinguish the six-pair observer from the existing two-requester fairness instrument.
- Test-first engine compilation failed because engine.h did not exist, before the new engine implementation was added.
- `make -C firmware clean all test`: passed with ARM GNU GCC 14.2.1, C11, Cortex-M4/thumb software floating point, `-Wall -Wextra -Werror`. Final target ELF: 3032 bytes text, zero initialized data, 1736 bytes BSS. ELF and binary generated locally; build products are ignored.
- Production engine and wire tests passed under AddressSanitizer and UndefinedBehaviorSanitizer. Cases cover all sixteen input masks, every pair, segment entry/exit timing, requalification across missing ticks, independent qualification under unrelated chatter, duplicate ticks, timestamp wrap, transactional counter exhaustion, serializer buffer bounds, and 100000 deterministic pseudo-random samples compared with a separately implemented qualification/pair oracle.
- CRC-16/CCITT-FALSE known vector `123456789` equals 0x29B1; empty message, corrupt message, and invalid pointer/length combinations tested.
- Serial driver tests passed with modeled Linux-mapped registers: buffer limits, busy refusal, ready-byte servicing, timeout, recovery, and tick wrap. This is not silicon validation.
- Production scheduler tests passed with modeled board functions: initial boot transmission failure, recurring policy, regular status, watchdog feeds, and watchdog startup failure producing valid halted status before telemetry.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: twelve tests passed. Real Tk screens and Linux pseudo-terminal receive path exercised with synthetic records emitted by the production C encoder. Tests cover disconnect, replay, private JSON export, overwrite refusal, failed replacement connection, malformed and checksum-valid impossible records, resynchronization, partial EOF, reset/policy behavior, duplicates, rollback, timing bounds, stale reception, and cumulative backlog detection.
- KiCad 9.0.2 generator completed: fourteen components and nine nets. Native schematic, PCB, project, symbol library, saved XML netlist, and connectivity manifest generated.
- KiCad ERC: zero violations. PCB DRC: zero violations and zero unconnected items. Fresh schematic XML netlist, saved netlist, manifest, and actual PCB pads agree on fourteen components, thirty pads, nine nets, values, and full library footprint identifiers.
- Schematic SVG exported and rendered with rsvg-convert; visual inspection found legible separated labels, correct ground-only connector, four resistor/input rows, and jayis1 title-block attribution. Python cairosvg was absent; the installed SVG renderer was used instead. No new graphics dependency was installed.
- README word count: 3305. Firmware has 517 C source lines and 626 C/header lines; 591 C/header lines remain after stripping comments and blank lines. These counts exclude tests, application code, generated CAD, build outputs, and linker script.
- Authorship audit confirmed jayis1 in all deliverable files before this record was added. This record also credits jayis1. No agent attribution was introduced. Build/cache/local KiCad preference files are ignored.

## Engineering review

Pair indices are 01, 02, 03, 12, 13, 23 in C, Python, documentation, and tests. GRANT0–3 correspond to PA0, PA1, PA4, PB0 and J3–J6. Serial output remains PA2 toward ST-LINK only. Reused peripheral implementation and carrier geometry remain attributed to jayis1. No GitHub Actions workflow, cloud service, hidden command channel, or active target stimulus was added.

Owner review checked conservative segment endpoints, gap-induced segment splitting, sticky findings, atomic counter updates, continuing pair observation while unrelated qualification is incomplete, and freezing of halted evidence. The observed count is whole-vector coverage and is explicitly not a denominator for per-pair exposure. Pair durations can overlap in time and must not be summed as wall-clock duration. Parser consistency checks are necessary bounds, not authentication or a complete mathematical realizability proof.

## Outstanding physical work

Fabrication, assembly, continuity measurements, MCU programming, real ST-LINK USB communication, ground/loading checks, GPIO electrical levels, back-power behavior, oscillator calibration, watchdog recovery on silicon, ESD/EMC, enclosure suitability, environmental limits, and regulatory compliance remain unverified. Native CAD validity and software tests do not establish any of these properties. The device is explicitly an unbuilt prototype design and is restricted to authorized, non-hazardous bench research.
