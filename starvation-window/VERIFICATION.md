# Starvation Window verification

Author: jayis1.

## Executed acceptance checks

The following checks were run against this revision on the development host. They are software and CAD evidence, not physical measurements.

- `make -C firmware clean all test`: successful Cortex-M4 target link and binary generation with GNU Arm Embedded GCC 14.2.1 (20241119), newlib/nosys, warnings treated as errors. Final target size: 3312 bytes text, 0 bytes initialized data, 912 bytes BSS. Linker reserves at least 4096 bytes for stack.
- Native compiler: GCC 14.2.0. Four C test executables pass: measurement engine, fairness policy, production main loop with a deterministic board boundary, and production UART driver using mapped test registers. Engine/fairness/main tests use AddressSanitizer and UndefinedBehaviorSanitizer; UART uses UndefinedBehaviorSanitizer because it intentionally maps the peripheral address range.
- Engine tests cover exact secondary dead-time boundaries, direct switching, overlap duration, independent request contradictions, timestamp wrap, duplicate ticks, acquisition gaps, counter exhaustion, minimum/histogram integrity, encoder bounds, and the CRC known-answer vector. A deterministic 100,000-sample mixed-activity run asserts snapshot invariants after every sample.
- Fairness tests cover 99 versus 100 observed waiting intervals, held demand without repeated findings, own-service release, three peer bypasses, both requester directions, simultaneous request/peer-service arrival, request withdrawal, and gap recovery. The primary fairness algorithm runs in the same sampling path as the target firmware.
- `python3 -m unittest discover -s tests -p test_app.py`: eight tests pass on Python 3.13.5. Synthetic fixtures come from the real C encoder, including repeated competing service and both-domain starvation. Tests cover parser corruption, incomplete/empty replay, duplicate timestamps, rollback suspicion, impossible histogram/minimum combinations, waiting without demand, private exclusive export, provenance retention, FIFO rejection and replay size limits.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_gui.py`: two real Tk event-loop tests pass. A Linux PTY exercises connection, real C-encoded input, all three tabs, export, hangup/disconnection, replay, missing serial path and reception-backlog freezing. This is not a physical USB or ST-LINK test.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native schematic, routed PCB, project, symbol library, exported netlist and connectivity manifest with KiCad 9.0.2 bindings.
- `/usr/bin/python3 tools/check_hardware.py`: zero ERC violations, zero DRC violations, zero unconnected items. Fresh schematic export, saved netlist, PCB pads and manifest agree on fourteen components, thirty pads and nine nets, including values and footprint library identities.
- Schematic SVG exported using KiCad and rasterized for visual inspection. Four resistor/input channels, ground-only header, MCU signal header and jayis1 title attribution are present. No physical assembly inspection is implied.
- README mechanically counted at 3580 whitespace-separated words. Production firmware contains 599 nonblank, noncomment C source lines plus 103 corresponding header lines; tests, generated CAD and build outputs are excluded from these totals.
- First-party deliverables checked for jayis1 attribution. Build output, Python caches and KiCad local preferences are ignored. Independent read-only review found no blockers after minimum/histogram integrity fixes and the primary fairness design review.

## Repository-relative distinction

The initial break-before-make-only candidate was rejected after reading Handoff Audit's complete purpose. The published device instead implements per-requester waiting ages, peer service bypass budgets and sticky starvation episodes. Its secondary mutual-exclusion diagnostics and passive carrier are shared engineering infrastructure, not claims of originality. No existing device was overwritten or modified.

## Outstanding physical gates

No board has been fabricated, assembled, programmed or electrically tested. No actual target signals, private data or credentials were captured. GPIO ordering, reset clock assumptions, power sequencing, watchdog timing, loading, connector orientation, UART delivery, oscillator calibration, ESD/EMC behavior and long-duration operation require bench validation on the exact module revision. The firmware is compile/link verified, not silicon validated. The CAD is rule/connectivity checked, not certified manufacturing-ready.

Before any deployment, follow the README's authorized-use scope and physical bring-up sequence. Confirm the target's real fairness contract; the fixed 100 ms / three-bypass policy is a declared laboratory criterion, not a universal vulnerability threshold. CRC and consistency checks provide no authentication or trusted attestation. A clean trace is not proof of availability, confidentiality or absence of sub-sample faults.
