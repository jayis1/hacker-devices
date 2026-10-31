# Reuse Quarantine verification record

Author: jayis1.

Scope: standalone-owner implementation with advisory review, not full collective sign-off. This record describes executable software and CAD checks, not a built or physically tested instrument.

Executed checks:

- `make -C firmware clean all test`: passed. Cortex-M4 Thumb soft-float ARM ELF and binary linked with warnings treated as errors. Size output: 3428 text bytes, 0 data bytes, 1668 BSS bytes. ARM machine and soft-float ABI confirmed with `arm-none-eabi-readelf -h`.
- Production C line gate: 578 nonblank, noncomment C lines, excluding tests and headers.
- Host engine suite passed under address and undefined-behavior sanitizers. It exercises both identities and duration classes at deadline-minus-one, exact deadline, and deadline-plus-one; independent absolute-deadline oracle scenarios; overlapping short/long quarantine; simultaneous expirations; expiry rollback on exhaustion; gap freeze with pending timers; invalid operations; timestamp wrap; and bounded serialization.
- Host serial and production scheduler suites passed. These use controlled register/board models, not physical peripheral tests. They cover packet bounds, timeout, recovery, wrap, lost boot transmission, repeating compatibility policy, and watchdog initialization failure.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v`: 21 tests passed. Includes actual Tk tabs and serial PTY reception, hangup, replay, export, private permissions, overwrite refusal, terminal restoration, malformed framing, CRC-valid impossible snapshots, and real C-encoder fixture ingestion. All fixtures are synthetic.
- `python3 -m compileall -q app`: passed.
- Native KiCad generation and checking passed: 14 components, 30 pads, 9 nets. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML, saved netlist, PCB pad nets, values, footprint identifiers, and connectivity manifest agree.
- Schematic SVG exported and rendered to PNG for visual inspection. Symbols, signal labels, connector mapping, and jayis1 title attribution are readable without overlapping labels.
- README length gate: 3302 words. Attribution gate passed for all deliverables before this verification record was added; rerun the same gate to include this file. Build outputs and local KiCad preferences are ignored.

Review correction: a read-only advisory review identified that a halted sparse snapshot could debit different elapsed times from the two still-running timers. A failing regression reproduced this omission. The companion now derives one shared acquisition count for both timers. Another regression checks that a hidden new return cannot consume time still owed to the old quarantine. Both negative cases and their valid neighboring cases pass. The review did not certify fabrication or deployment.

Reproduce from this directory using the commands above and `/usr/bin/python3 tools/check_hardware.py`, followed by `python3 tools/check_release.py`. Generation requires KiCad 9 Python bindings and standard footprints; firmware requires GNU Arm Embedded GCC and newlib; desktop tests require Linux, Tk, and Xvfb. No downloaded SDK, network dependency, or third-party Python package is needed.

Remaining laboratory gates: assembly, MCU programming, electrical continuity and loading measurements, power sequencing, actual watchdog behavior, HSI/sample-clock calibration, USB endurance, physical deadline correlation, enclosure, ESD characterization, and validation that target indications faithfully represent real resource lifetime. Suggested procurement parts are not verified stock. CRC and structural checks provide no authentication. Sparse validation gives necessary bounds rather than full schedule reconstruction. No safe-reuse, vulnerability-exploitation, compliance, or tamper-proof-evidence claim is made.
