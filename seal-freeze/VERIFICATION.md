# Verification — Seal Freeze revision A

Author: jayis1.

Execution mode: standalone owner implementation and self-review. Advisory reports informed release gates; no independent collective approval is claimed.

## Executed software checks

- `make -C seal-freeze/firmware clean all test` passed with ARM GNU GCC 14.2.1, C11, `-Wall -Wextra -Werror`. Cortex-M4 firmware linked and binary conversion completed. Final image section sizes: text 3460 bytes, data 0, BSS 1680. Host compilation is not substituted for this target link.
- Production policy tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. Cases include all 16 captured/subsequent value pairs, all 256 stable input-vector transitions, initial-high seal, reset with seal high, exact qualification boundary, simultaneous data/seal, withdrawal without recapture, gaps, cross-channel chatter, timer wrap, transactional exhaustion, encoder rejection and bounded output. A deterministic 100,000-step varying-plateau trace also passed serializer invariants.
- UART tests passed: bounds, timeout, recovery and timestamp wrap. Production-main scheduler tests passed with modeled peripherals: lost boot, repeating policy and watchdog startup failure. These modeled peripherals are not silicon validation.
- `xvfb-run -a python3 -m unittest discover -s seal-freeze/tests -p test_app.py -v` passed all ten tests. The suite uses actual C-encoded synthetic data and the actual Tk event loop with Linux pseudo-terminals. It checks replay, private export, overwrite refusal, invalid snapshots, capture immutability, duplicate and rollback detection, framing and EOF, stale live data, constructor cleanup, duplicate connection refusal, termios restoration and disconnection.
- `python3 seal-freeze/tools/check_release.py` checks the 2000-word and 500-production-C-line thresholds and author credit in every deliverable. README: 3271 whitespace-delimited words. Production `.c` files: 542 nonblank, noncomment lines, excluding headers and tests. Build products and local preferences are ignored.

## Executed CAD checks

- Native generation with KiCad 9.0.2 / pcbnew succeeded.
- `kicad-cli sch erc --exit-code-violations`: zero violations.
- `kicad-cli pcb drc --exit-code-violations`: zero violations and zero unconnected items.
- Fresh XML netlist versus PCB versus manifest: exact agreement across 14 components, 30 pads, 9 nets, values and full footprint identifiers. Saved netlist also agrees.
- Schematic exported to PDF and rendered to PNG for visual inspection. Four signal channels, resistor values, ground-only J1, MCU harness J2 and author/title block were inspected. There are no rule exclusions or waived errors in the checker.

## Reference and review boundaries

The existing receive-only platform supplies the reused MCU/register and carrier foundation. The ST UM1724 indexed manufacturer result, Table 16, corroborates CN8 A0=PA0, A1=PA1, A2=PA4 and A3=PB0. Its USART section identifies PA2/PA3 and default SB13/SB14 ON, SB62/SB63 OFF. Direct PDF retrieval encountered an HTTP/2 error and HTTP/1.1 timeouts; a newly downloaded full-manual audit is not claimed. Recheck the purchased board revision and manufacturer electrical limits before assembly.

Manufacturer reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf

Self-review traced reset precedence, setup ordering, capture retention under seal withdrawal, independent qualification, gap recovery, transactional overflow and desktop state transitions. Reset segment counting includes reset assertion even when seal remains high; such a reset ends an old contract without necessarily establishing a usable new baseline. This distinction prevents a legitimate reset from looking like unexplained obligation loss in sparse telemetry.

## Not performed

No physical board was fabricated or programmed. USB interoperability, actual input thresholds, current consumption, oscillator accuracy, watchdog timing, port skew, back-power behavior, ESD, EMC, environmental reliability, mechanical assembly and deployment suitability remain unverified. The carrier has no galvanic isolation or dedicated input protection. Rule checks do not prove safe wiring or manufacturability. The software fixture is synthetic and no file is presented as a hardware capture.
