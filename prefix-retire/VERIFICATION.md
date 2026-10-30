# Verification record

Author: jayis1.

This release was implemented and checked in standalone-owner mode. Supplied peer reports were advisory, not independent approval of these artifacts. No physical board or target was available. Results below describe executed software/CAD checks, not fabrication readiness or electrical certification.

## Executed checks

- `make -C firmware clean all test`: PASS. Actual Cortex-M4 target linked and binary generated. Text 3132 bytes, data 0 bytes, BSS 1652 bytes. No build artifacts are tracked.
- Host engine tests with address/undefined-behavior sanitizers: PASS. Includes all 24 readiness permutations, interleaved readiness/retirement, repeated batches, Cartesian product of 24 readiness orders and 24 retirement orders (576 combinations), missing readiness, duplicate readiness after retirement, simultaneous and overlapping strobes, tag setup loss, raw opposite-strobe guards, acquisition loss, timestamp wrap, startup attachment, transactional active-prefix counter exhaustion, immutable halted evidence, encoder invariants, and exact buffer boundaries.
- Host USART register-model tests: PASS for bounds, busy rejection, byte service, timeout, wrap, and recovery. This uses mapped ordinary memory, not actual peripheral hardware.
- Production main-loop tests with controlled board/transport functions: PASS for lost boot, periodic late-attachment policy, transmit failure, and watchdog initialization failure before first status.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: PASS, 13 tests. Production C-encoder output is replayed and paced through a Linux PTY into the real Tk event loop. Screens, retained history, JSON export, private permissions, overwrite refusal, double-connect refusal, disconnect, and replay replacement are exercised. Other cases include checksum-valid impossible evidence, conservation, prefix/membership consistency, duplicate freshness, host pause, counter/time rollback, immutable halt, framing recovery, incomplete EOF, FIFO rejection, and oversized replay rejection.
- `/usr/bin/python3 tools/generate_hardware.py`: PASS; native schematic, symbol library, routed PCB, project, XML netlist, and manifest generated.
- `/usr/bin/python3 tools/check_hardware.py`: PASS; zero ERC violations, zero DRC violations, zero unconnected items. Exact agreement across fresh schematic netlist, saved netlist, PCB pads, and manifest: 14 components, 30 pads, nine named nets; values and full footprint identifiers match.
- Schematic exported to SVG with KiCad CLI, rendered using rsvg-convert, and visually inspected: populated resistor/header symbols, legible signal labels, separated pin labels, and title/author visible. Render is a scratch verification artifact, not a committed deliverable.
- `python3 tools/check_release.py`: PASS; 3322 README words and 531 nonblank production C lines after comment removal. Headers and test code are not included in the C count. All release files carry jayis1 attribution. The gate checks required CAD files, prohibited author headers, private-key markers, and Git whitespace errors.

## Environment and reproduction

Executed toolchain: Arm GNU GCC 14.2.1 (20241119; package 15:14.2.rel1-1), host GCC 14.2.0 (Debian 14.2.0-19), Python 3.13.5, KiCad 9.0.2. Firmware uses C11, `-Wall -Wextra -Werror`, Cortex-M4 Thumb, software-float ABI, nosys/newlib, local startup, and local linker script. The app uses only the Python standard library plus the operating system's Tk installation. No package download or application build service is required.

Run the commands above from this folder. The hardware generator requires KiCad Python bindings through `/usr/bin/python3` and standard footprints under `/usr/share/kicad/footprints`. Fixture traffic is synthetic and intentionally ends with a non-prefix retirement finding. A passing test does not mean that this intentionally faulty fixture reports zero faults.

## Limits and release review

The pin map and register layout reuse the existing jayis1 Nucleo platform; a fresh manufacturer-document audit was not performed. MCU programming, board assembly, physical USB behavior, threshold compatibility, electrical loading, back-power, timing accuracy, short-pulse capture, watchdog timing, ESD/EMC, enclosure fit, and regulatory qualification remain unverified. The README explicitly limits this to a slow, common-ground 3.3 V diagnostic adapter. It is not a physical data diode or a trusted attestation device.

Repository novelty was evaluated against existing device purposes, especially Slot Alias, Replay Sieve, Receipt Witness, and Epoch Witness. No worldwide novelty claim is made. There is no target command parser, shell invocation from received frames, network service, downloaded dependency, or real capture in the new package. All writes are confined to prefix-retire; no task branch is used. Publication and remote-default-branch verification are reported by the owner after the commit, rather than embedding a self-referential commit hash in this file.
