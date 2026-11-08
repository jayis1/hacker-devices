# Bank Pack verification

Author: jayis1.

## Scope and provenance

Executed in standalone-owner mode against jayis1/hacker-devices on its remote default branch main. Advisory peer reports were not treated as independent build evidence. The initial checkout was clean; `git pull --ff-only` reported already up to date at 1e0f2b620d6b951d39da68c5b9f5830166629233. No temporary branch was created. Existing device directories and README descriptions were inventoried, and packing/consolidation terms searched before selection. Bank Separation and Cursor Fit contracts were inspected explicitly. Only bank-pack files are part of this change.

Reusable acquisition, transport, board-support and passive-carrier infrastructure comes from jayis1-authored Cursor Fit. The partial-bank-first rule and its evidence checks are distinct. The independent oracle uses bank population arrays, not the production XOR partner test. An initial run of the new engine test against the inherited cyclic policy failed at the all-initial-tags-eligible assertion, demonstrating that the test distinguishes the policies before implementation.

## Executed checks

- `make -C bank-pack/firmware clean all test`: PASS. ARM GNU GCC 14.2.1, Cortex-M4 Thumb soft-float, C11, warnings treated as errors. Linked image: text 3640 bytes, data 0 bytes, BSS 1692 bytes. Binary produced with objcopy.
- Production engine tests with AddressSanitizer and UndefinedBehaviorSanitizer: PASS, 39,400 independent population-oracle transitions across repeated operation words through depth eight. Tests cover either initial bank, sibling preference, ties between two partial banks, full-pool releases, drain and immediate reuse, duplicate/orphan precedence, raw interface conflict, acquisition gaps, terminal freeze, timestamp wrap, exhaustion rollback and bounded serialization.
- UART tests with UndefinedBehaviorSanitizer: PASS, bounded submission, timeout, recovery and wrap.
- Production main-loop tests with controlled board transport: PASS, lost boot, periodic late-attach policy, transport timeout and watchdog startup failure.
- `xvfb-run -a python3 -m unittest discover -s bank-pack/tests -p 'test_app.py' -v`: PASS, 19 tests under Python 3.13.5. Real C encoder fixtures pass through the actual parser and Tk screens. Tests include all occupancy vectors, CRC-valid impossible partial-bank histories, legal release/fill escape paths, terminal immutability, stamp migration, truncated framing, unsupported policy, stale duplicate telemetry, cumulative drift, rollback, permission denial, non-TTY failure, PTY connection/hangup, termios restoration, replay bounds, private JSON export and overwrite refusal.
- `/usr/bin/python3 bank-pack/tools/generate_hardware.py`: generated populated schematic, board, project, library, manifest and XML netlist.
- `/usr/bin/python3 bank-pack/tools/check_hardware.py`: PASS using KiCad 9.0.2. ERC: zero violations. DRC: zero violations, zero unconnected items. Exact component values, footprint identities and pin-to-net relationships match between fresh schematic export, saved netlist, board pads and manifest: fourteen components, thirty pads, nine nets.
- Schematic SVG exported with kicad-cli, rasterized with rsvg-convert and visually inspected. Bank Pack title and jayis1 attribution are present; signal labels, connector pin numbers and resistor rows are separated and readable. CairoSVG was unavailable; rsvg-convert provided the actual render instead.
- `python3 bank-pack/tools/check_release.py`: PASS before this report was added: 3133 README words, 595 substantive production C lines, 33 attributed source deliverables. The final rerun includes this attributed report. Counts exclude comments, blank C lines, tests, build output, Python caches and KiCad personal preferences.
- `git diff --check`: PASS. Build products are ignored; no dependencies are vendored and no secrets are required.

## Sources and physical limitations

Manufacturer-indexed ST UM1724 text corroborates USART2 PA2/PA3 virtual-COM routing and default bridge configuration. Reference: https://www.st.com/resource/en/user_manual/um1724-.pdf . The purchased module revision, full register/electrical documentation and actual bridge settings still require bench review. Distributor listings corroborate Yageo MFR-25FBF52-1K and MFR-25FBF52-100K axial resistor values, tolerances and body dimensions, and Samtec TSW-104-07-G-S pitch and configuration. Procurement choices are candidates, not a verified purchase or guaranteed mechanical fit.

All traffic used in execution was synthetic. PTY tests validate host transport behavior, not physical USB or MCU operation. No board was fabricated or programmed, no target was attached, and no signal thresholds, oscillator accuracy, power consumption, ESD resilience, back-power behavior or real-target security outcome was measured. ERC covers a passive carrier, not the external module or harness. CAD validation does not establish enclosure safety or production readiness. Sparse telemetry checks are necessary endpoint constraints, not complete hidden-history reconstruction or authentication.
