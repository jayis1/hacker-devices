# Sticky Trip verification

Author: jayis1.

This record describes executed software/CAD checks, not a fabricated or programmed instrument. Work used standalone-owner execution with advisory review, not full collective hardware review.

## Executed checks

- `make -C firmware clean all test`: passed with warnings treated as errors. ARM GNU GCC 14.2.1 linked an ELF32 ARM EABI5 soft-float executable at reset entry 0x08000175. Linked sections: 3364 bytes text, zero initialized data, 1680 bytes BSS. A binary image was produced. Native GCC 14.2.0 ran the engine with address/undefined-behavior sanitizers, and serial/main-loop tests with the configured host checks.
- Engine regressions passed: retained obligation, authorized fresh clear, initial-high clear, held-clear authority change, simultaneous newly qualified authorization, set precedence, all 256 stable input-vector transitions, unrelated chatter, gaps, ordinary timer wrap, duplicate sample, exhaustion rollback, invalid episode capacity, bounded encoder, and the standard CRC check vector. Gap/missed exhaustion preserves historical values and produces a serializable halt.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: ten tests passed under Python 3.13.5. The real C encoder fixture was read through the parser and a Linux pseudo-terminal with an actual Tk event loop. Checks exercised screens, live progress, disconnect/hangup, replay, export permissions and overwrite refusal, malformed input, impossible evidence, duplicate/rollback/growth freezes, backlog, later-boot provenance, and forged obligation changes.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native schematic, project, PCB, symbols, manifest, and XML netlist from the authored carrier design.
- `/usr/bin/python3 tools/check_hardware.py`: KiCad 9.0.2 ERC zero violations; DRC zero violations and zero unconnected items. Exact component/value/footprint/pin/net comparison passed for fourteen components, thirty pads, nine nets across fresh schematic export, saved netlist, manifest, and PCB.
- Schematic exported through KiCad SVG and rendered to PNG; visual inspection found readable signal labels, connected passive symbols, correct title and jayis1 attribution. Temporary renders are not release artifacts.
- `tools/check_release.py` mechanically enforces required paths, at least 2000 README words, at least 500 nonblank/noncomment production `.c` lines excluding headers and tests, and jayis1 attribution in every nonignored release file. The README contains 3193 whitespace-delimited words; production `.c` files contain 533 nonblank/noncomment lines.

## Review corrections

Read-only implementation review identified missing episode-capacity bounds, an insufficient temporal obligation check in the host, and reset metadata being replaced after a later boot. Regressions reproduced all three failures before correction. Both validators now require total observations not to exceed episode count times maximum. The host rejects unaccounted obligation changes without a gap/halt, preserves original reset provenance, and records the later boot separately. All ten application tests passed after these corrections.

## Explicit limits

No physical MCU programming, carrier fabrication, serial USB hardware trial, voltage measurement, load measurement, timing calibration, watchdog reset trial, EMC/ESD qualification, environmental test, or certification was performed. Host register models and pseudo-terminals are software simulations. The fixture is synthetic, not a laboratory capture. Builds and passive-only ERC cannot validate the purchased Nucleo's jumpers, power configuration, input voltage safety, or the target's honesty.

An attempted fresh download of ST UM1724 failed with an HTTP/2 transport error and subsequent HTTP/1.1 timeouts. Pin/register assignments reuse the repository's documented F401 platform; no successful fresh manufacturer-document retrieval is claimed. Check the current manufacturer manual, RM0368, datasheet, and actual board revision before assembly or connection. The carrier has no powered MCU symbol because the Nucleo is explicitly external, not because MCU support circuitry is omitted from an integrated board.

The source is a reproducible engineering design, not a deployment-ready alarm appliance. Remaining acceptance is physical bring-up under the README's authorized-use and electrical restrictions.
