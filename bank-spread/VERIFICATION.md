# Bank Spread verification

Author: jayis1.

## Scope and provenance

Executed in degraded standalone-owner mode, not full collective review. Advisory reports were not treated as build evidence. The live checkout was /root/hacker-devices, remote https://github.com/jayis1/hacker-devices.git. Remote HEAD resolved to main at 4a6cee4d78360ca98e96236b3eb1c57e040013e3; `git pull --ff-only` reported already up to date. No task branch was created. Only bank-spread deliverables are included; daily-ideas was not modified.

The initial untracked directory contained only tests/test_spread.c. Session history established it as an interrupted earlier execution of the same authorized cron task. The test was recovered without overwriting it. Running it against Bank Pack failed at the first sibling-eligibility assertion, demonstrating the intended distinction before implementation. Existing directories and README policy descriptions were inspected; Bank Pack, Bank Separation and Wear Balance were compared explicitly. Reused infrastructure is disclosed in README.md, not presented as a new electrical circuit.

## Executed checks

- `make -C bank-spread/firmware clean all test`, followed by a final rebuild after edits: PASS. ARM GNU GCC 14.2.1, Cortex-M4 Thumb soft-float, C11, warnings treated as errors. Target ELF links with 3676 text bytes, zero data bytes and 1692 BSS bytes. Objcopy produces the flash binary.
- Independent occupancy-array regression: PASS across 95,240 operation transitions through depth eight, all sixteen occupancy subsets, arbitrary returns, full drain and repeated same-tag reuse. The companion engine test independently exercises the same transition count plus qualifier boundaries, duplicate/orphan/interface precedence, terminal freeze, timestamp wrap, exhaustion rollback and bounded serialization. UndefinedBehaviorSanitizer covers both; the engine suite also uses AddressSanitizer.
- UART tests: PASS for bounds, timeout, recovery and wrap. Production main-loop tests: PASS for periodic late-attach policy, lost boot transmission, serial timeout and watchdog-startup failure. These use controlled host substitutes for physical registers.
- `xvfb-run -a python3 -m unittest discover -s bank-spread/tests -p 'test_*.py' -v`: PASS, 21 tests with Python 3.13.5. Tests include real C-encoder fixtures through the actual parser and Tk screens, PTY input and hangup, serial setting restoration, permission denial, all occupancy vectors, CRC-valid impossible histories, legal barrier escape paths, stamp migration, terminal immutability, framing recovery, policy mismatch, stale duplicates, drift, rollback, bounded replay, private export and overwrite refusal.
- Independent legal-history enumeration: PASS for 17,260 sparse prefix-to-end pairs through six operations. A new negative test first demonstrated that an exhaustion endpoint could invent an admission without any additional acquired observation. The transition validator now bounds accepted operations and attempts by acquired observations, including the first halt transition; the regression passes.
- `/usr/bin/python3 bank-spread/tools/generate_hardware.py`: PASS. Generated actual populated schematic, routed board, project, symbol library, saved XML netlist and component manifest.
- `/usr/bin/python3 bank-spread/tools/check_hardware.py`: PASS using KiCad 9.0.2. Zero ERC violations, zero DRC violations and zero unconnected items. Fresh schematic export, saved netlist, board pads, component values, footprint identifiers and manifest agree: fourteen components, thirty pads, nine nets.
- Schematic exported with kicad-cli, rasterized with rsvg-convert and visually inspected. The title is Bank Spread receive-only carrier; jayis1 attribution is present. Connector pin labels and the four resistor rows are separated and readable.
- Release gate before this report: PASS, 2882 README words, 594 nonblank production C lines after removing comments, and 35 attributed deliverables. Counts exclude tests from the C threshold and exclude build products, caches and KiCad personal preferences from file checks. Final release check includes this report.

## Review and limitations

Self-review checked firmware/companion eligibility agreement, named protocol identity, serial bounds, generated net connectivity, attribution and task scope. No additional writing agent or independent collective review was used. No package dependencies were added. Build outputs are ignored and are not published as source artifacts. The target linker build is distinct from host simulation: it does not establish successful physical boot.

Manufacturer-indexed ST UM1724 text corroborates USART2 PA2/PA3 virtual-COM routing and default SB13/SB14 enabled with SB62/SB63 disabled. Reference: https://www.st.com/resource/en/user_manual/um1724-.pdf . Purchased board revision, complete register/electrical documentation, resistor/header mechanical fit and actual bridge settings still require bench review. Procurement entries are candidates, not verified purchases.

All exercised traffic was synthetic. No board was fabricated, no MCU was programmed, no real target was connected, and no logic thresholds, sampling accuracy, power consumption, ESD resilience, back-power behavior or security consequence was measured. ERC validates the passive carrier, not the external module, harness or enclosure. Sparse checks are necessary endpoint constraints, not authenticated or complete reconstructions of hidden operations. These physical and evidentiary limitations are intentional release disclosures, not completed validation claims.
