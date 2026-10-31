# Verification record

Author: jayis1.

This is standalone-owner verification, not full collective or independent review. Advisory reports informed scope and validation choices. No physical hardware was available; synthetic fixtures and PTYs are not physical measurements.

## Executed checks

- Repository origin: jayis1/hacker-devices. Configured remote default branch: main. Initial checkout was clean; `git pull --ff-only` returned already up to date.
- `make -C firmware all test`: passed with arm-none-eabi GCC 14.2.1 and host GCC. The Cortex-M4 image linked with 3,320 bytes text, zero data, and 1,660 bytes BSS. No target programming occurred.
- Engine tests: 1,832 independent array-oracle branches through six operations, both slot identities, both generation values, allocation and release, arbitrary release ordering, stale release, non-toggling reallocation, simultaneous strobes, pulse rejection, held strobe, setup ambiguity, timestamp wrap, gap detection, terminal immutability, exhaustion, serialization bounds, invalid state, and CRC reference vector passed. Address and undefined-behavior sanitizers were enabled for the engine executable.
- Serial-register-model tests: bounds, busy submission, UART progress, timeout, recovery, and timestamp wrap passed under undefined-behavior sanitizer. These are Linux register-model tests, not real USART measurements.
- Production main-loop harness: lost boot, periodic compatibility policy, transmit failure, timeout accounting, and watchdog startup failure passed.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: all 12 tests passed on Python 3.13.5. Coverage includes real C fixture decoding, malformed frames, policy mismatch, reset handling, EOF, invariants, sparse ownership-cycle requirements, private export/overwrite refusal, FIFO rejection, freshness, open failure, and a real Tk event loop over a Linux PTY. The GUI test verified three tabs, active slot/generation display, double-connect refusal, hangup detection, and evidence retention.
- `python3 -m compileall -q app`: passed.
- Native CAD regenerated with KiCad 9.0.2 and pcbnew. ERC: zero violations. DRC: zero violations and zero unconnected items.
- Fresh schematic netlist, saved netlist, PCB pads, and connectivity manifest agree: 14 components, 30 electrical pads, nine nets. Values and full library footprint identities also match.
- Schematic exported to SVG and rendered to PNG for visual inspection. Four labelled resistor-conditioned input channels, ground-only header, MCU signal header, and jayis1 title attribution were inspected. Rendered scratch previews are not release deliverables.
- Release checker: 3,122 README words and 560 nonblank, comment-stripped production C lines; both exceed the required thresholds. Authorship is checked in every non-build deliverable. Build products, bytecode, and local KiCad preferences are ignored.

## Known limitations and unresolved physical gates

The passive carrier intentionally relies on an external NUCLEO-F401RE for MCU power, clocking, reset, SWD, and USB. Its electrical interfaces are only for known 3.3 V common-ground laboratory logic. It has no isolation or dedicated ESD/overvoltage protection.

Fresh ST UM1724 PDF retrieval failed: the extraction backend cannot extract URLs, direct HTTP/2 download failed, and HTTP/1.1 timed out without data. The inherited repository pin map is retained and its purchased-board verification is an explicit pre-wiring gate, not a newly verified manufacturer claim.

No fabrication, flashing, physical timing calibration, oscillator characterization, power measurement, connector-fit check, electrical interoperability, ESD test, regulatory qualification, or real target experiment was performed. Successful compilation and CAD rule checks do not satisfy those gates.

One generation bit detects disagreement with the immediately preceding owner, not arbitrarily old owner identities after generation wrap. Diagnostic adapters can lie or select incorrect semantic boundaries. CRC and sparse transition checks are integrity aids, not authentication or a complete proof of all hidden operations. Evidence is volatile and some lost-boot resets remain indistinguishable without a persistent boot identity.
