# Scrub Credit verification

Author: jayis1.

## Scope and provenance

This is a standalone-owner implementation and review, not a collective sign-off. Advisory reports supplied no repository measurements. The verified destination was jayis1/hacker-devices, configured default branch main. The checkout was clean before `git pull --ff-only`, which reported already up to date. No temporary branch was created. Existing device directories and README titles were inventoried; Erase Witness, Grant Ledger, and Slot Affinity contracts were read to distinguish per-resource cleanup credit from those designs. Existing jayis1-authored platform facilities were reused explicitly; the new resource policy and tests live entirely in this directory.

## Executed checks

| Check | Command / evidence | Result |
| --- | --- | --- |
| Test-first C policy | `make -C firmware test` before engine implementation | Expected compile failure for missing engine; then passed after implementation |
| Test-first companion contract | `python3 -m unittest discover -s tests -p test_contract.py` before contract implementation | Expected import failure; then passed after implementation |
| Target build | `make -C firmware all test` | Cortex-M4 ELF and binary linked with Arm GCC 14.2.1; text 3284 bytes, data 0, BSS 1660 |
| C policy | AddressSanitizer and UndefinedBehaviorSanitizer executable | 3066 independent oracle branches, plus directed lifecycle and failure tests passed |
| Serial transport | UndefinedBehaviorSanitizer Linux MMIO model | Bounds, timeout, recovery, and timer-wrap tests passed |
| Production main loop | Controlled board/transport test | Lost boot, periodic late-attach policy, timeout, and failed-watchdog startup passed |
| Python and desktop | `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` | 18 tests passed, including Tk event-loop / Linux PTY integration |
| Python syntax | `python3 -m compileall -q app` | Passed |
| CAD generation | `/usr/bin/python3 tools/generate_hardware.py` | Native schematic, board, project, symbols, manifest, and netlist generated |
| Electrical rules | `/usr/bin/python3 tools/check_hardware.py`, KiCad 9.0.2 | Zero ERC violations |
| Board rules | Same checker | Zero DRC violations; zero unconnected items |
| Connectivity | Fresh schematic XML compared against actual PCB pads and manifest | 14 components, 30 pads, 9 nets; reference, value, footprint and pin/net agreement passed |
| Visual schematic | KiCad SVG export, rsvg-convert PNG render, visual inspection | Four resistor/input paths and connector labels readable; no evident overlap |
| Release gates | `python3 tools/check_release.py` | README exceeds 2000 words; 564 nonblank noncomment production C lines, excluding headers/tests; author attribution passed |
| Patch hygiene | `git diff --check` | Passed |

The C oracle is independent of the implementation's bit masks: it uses a two-element array of dirty, clean, or occupied states and explores legal interleavings while checking each rejected operation. Directed cases include credit for the wrong resource, duplicate cleanup, cleanup during occupancy, reuse without cleanup, concurrent occupancy, both return orders, reserved opcode, simultaneous strobes, raw opposite-strobe overlap, short rejected pulses, chatter, acquisition gaps, ordinary timer wrap, terminal freeze, and counter exhaustion without granting a partial credit.

The Python companion consumes real C-encoder synthetic output. Negative tests use CRC-valid but contradictory credit movement, not merely damaged packets. Further tests cover cyclic reachability, repeated reuse, invalid masks and counters, malformed/resynchronized framing, unsupported policy, reset detection, truncated EOF, FIFO and oversized replay rejection, duplicate timestamps, stale backlog, private exclusive export, symlink refusal, serial-open failures, and restoring terminal attributes after an injected setup failure. The GUI test receives through a real Linux pseudoterminal, switches all three screens, detects hangup, replays the encoder fixture, invokes JSON export, and preserves evidence on a subsequent failed port open.

## Electrical reference review

The official UM1724 manufacturer result exposed Table 16's NUCLEO-F401RE mapping (CN8.1–4: PA0, PA1, PA4, PB0) and section 7.10's ST-LINK virtual-COM solder bridge configuration. These agree with board.h, board_levels(), carrier nets, and README. Direct PDF retrieval from the first ST URL failed with HTTP/2 failure and HTTP/1.1 timeouts; the official-manual indexed excerpt was used instead. No successful local PDF download is claimed. Firmware register definitions and reset-clock assumptions reuse the existing STM32F401 platform; actual purchased-board revision must still be checked before assembly.

## Not verified and not claimed

No PCB fabrication, assembly, MCU flashing, electrical measurement, calibrated sample timing, live target run, physical USB interoperability, ESD/EMC qualification, battery test, or manufacturing approval was performed. No physical device or target was connected. Pseudoterminals, MMIO models, and encoder fixtures are software test mechanisms, not substitute physical evidence.

The carrier is passive and unisolated, so zero ERC does not approve the external module power or target voltage domain. Zero DRC does not establish footprint procurement compatibility or assembly quality. Timing is nominal HSI-based diagnostic timing. The observer cannot verify actual erasure, target instrumentation truthfulness, unseen short pulses, or some resets whose boot records were lost. Reports are unsigned. These limitations are part of the measurement contract, not hidden acceptance waivers.

Publication is performed separately by committing and pushing on main, followed by a remote branch-tip read-back. A passing local test alone is not evidence of publication.
