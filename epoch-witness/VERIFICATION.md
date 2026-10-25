# Epoch Witness verification

Author: jayis1

## Executed software and CAD checks

- `make -C firmware all test`: passed. GNU Arm Embedded GCC 14.2.1 linked the Cortex-M4 target with the supplied startup, register map, board drivers, and linker script. Image section sizes: text 3012 bytes, data 0, BSS 408; stack-local engine state is additional runtime RAM and is not included in BSS.
- The host policy/encoder tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. Cases include accepted and equal generations, rollback refusal, setup/hold/short-width violations, timeout/recovery, simultaneous execution/completion, high-water-mark retention across gaps, timer wrap, counter saturation, queue loss, CRC reference vector, output bounds, and 100,000 deterministic noisy-input invariant iterations.
- Production UART-driver tests passed with UBSan and Linux-mapped synthetic registers: invalid submissions, busy transmitter, TXE behavior, stalled-frame timeout, recovery, and timer wrap. These are not physical USART tests.
- Production main-loop scheduling test passed with a synthetic board clock: lost boot transmission, repeated policy, periodic status, watchdog feeding, and transmission-failure accounting.
- `python3 -m unittest discover -s tests -p test_app.py`: ten tests passed. They exercise the real C-generated stream, bytewise receive, CRC rejection, oversized/truncated frames, late attachment, freshness, rollback detection, policy mismatch, reboot freeze, impossible values, contradictory duplicate events, private export, overwrite refusal, FIFO rejection, and replay bounds.
- `xvfb-run -a python3 tests/integration.py`: passed. The real Tk event loop and termios receive implementation consumed a synthetic Linux pseudo-terminal stream. All three screens, retained event rows, live hangup, export provenance, replay, missing-device reconnect, cleared old summaries, and FIFO rejection were checked.
- `/usr/bin/python3 tools/check_hardware.py`: KiCad 9.0.2 ERC zero violations; DRC zero violations and zero unconnected items. Fourteen components, thirty pins, and nine nets match between freshly exported schematic, saved netlist, connectivity manifest, and actual PCB pads, including values and full footprint identities. No waivers were used.
- Schematic SVG export succeeded and its rendered image was visually inspected for component placement, net labels, connector mapping, and title/author fields.
- README contains 3307 whitespace-delimited words. Firmware C and header files contain 605 nonblank lines after removing C block comments, excluding tests and build products.
- Every intended repository file contains jayis1 attribution in a format-valid source comment, document author, symbol property, project text variable, or CAD title metadata. Generated build outputs and KiCad local preferences are ignored.

## Review findings resolved

An independent read-only review identified missing consistency checks for valid status/generation/commit combinations and event values, plus silent acceptance of contradictory same-sequence events. Regression tests reproduced both failures before the parser was changed. The parser now rejects impossible payloads and freezes contradictory events while ignoring exact duplicates. The complete application unit suite and GUI integration test passed afterward. The review did not fabricate hardware evidence or authorize target interaction.

## Reference and physical-validation limits

Fresh manufacturer PDF retrieval failed (HTTP/2 failure and HTTP/1.1 timeouts). An existing cached UM1724 revision 13 copy was read instead: Table 16 confirms NUCLEO-F401RE CN8 A0/PA0, A1/PA1, A2/PA4, A3/PB0 and USART2 TX/PA2; section 6.8 confirms the default ST-LINK virtual COM routing through SB13/SB14. Register definitions and passive platform circuits reuse the repository's receive-only implementation. Latest manufacturer revisions and the purchased module's actual bridges must be checked before assembly.

No physical hardware was available. Fabrication, soldering, SWD programming, electrical loading, oscilloscope timing, watchdog reset behavior on silicon, power sequencing, hot-plug behavior, thermal checks, ESD/EMC, enclosure qualification, and real-target operation are not verified. A passing passive ERC/DRC cannot establish these properties. Test captures are synthetic and deliberately contain a downgrade; they are not observations of an external device.

The delivered artifact is a software- and CAD-checked research prototype design. It is not a production-ready instrument or proof that an evaluated target enforces anti-rollback. Boot identity and the high-water mark are not persistent; the limitations documented in README remain applicable after every passing test.
