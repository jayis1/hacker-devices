# Alias Depth verification record

Author: jayis1.

## Scope and status

Single-owner execution with advisory review, not full collective review. All original artifacts credit jayis1. This is an unbuilt engineering prototype: successful software and CAD checks are not physical hardware qualification. The package stays inside the device-specific directory of jayis1/hacker-devices.

## Executed checks

Toolchain: ARM GNU GCC 14.2.1 (20241119), native GCC 14.2.0, Python 3.13.5, KiCad CLI 9.0.2. The app uses Python standard-library Tk, termios and select; Xvfb supplies the test display. No third-party application package installation was needed.

- `make -C firmware clean all test`: PASS. The actual Cortex-M4 target linked and converted to a binary. ELF size reported 3,708 bytes text, zero initialized data, and 1,708 bytes BSS. Stack usage, physical execution and timing are not measured by those section sizes.
- Native engine tests with address and undefined-behavior sanitizers: PASS. An independent boolean handle-token oracle checked 216,264 transitions in operation words through depth seven. It covers all four tags, repeated acquisition, arbitrary interleaving, partial drops, underflow and per-object ceiling rejection. Separate tests fill all twelve reference positions, drain them, reuse a tag, and exercise simultaneous/raw conflict, acquisition loss, frozen terminal evidence, counter exhaustion rollback, duplicate timestamps, ordinary timer wrap and encoder bounds.
- Native UART register-model tests: PASS. Invalid submission, busy queue, bounded service, exact timeout, recovery and timestamp wrap were exercised. This is a Linux mapped-register model, not a physical UART measurement.
- Production main-loop harness: PASS. Repeated compatibility announcements, lost boot frame, busy transport, TX timeout diagnostics and watchdog-startup failure were exercised using controlled board functions.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v`: PASS, 15 test methods. Includes all 256 per-object reference vectors; legal, underflow and ceiling fixtures from the real C engine/encoder; CRC-valid impossible reference migration and stamp migration; halt immutability; parser recovery and truncated tail; incompatible policy and later boot; stale duplicates, reception pause, cumulative drift and rollback; replay size/type rejection; private export and overwrite refusal; serial-constructor failure restoration; GUI screens, PTY reads, double-connect refusal, final termios restoration, hangup, replay, permission denial and export.
- `/usr/bin/python3 tools/generate_hardware.py` followed by `tools/check_hardware.py`: PASS. Native KiCad schematic, PCB, project, local symbol library and XML netlist generated. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML, saved XML, actual board pads and manifest agree for 14 components, 30 pads and 9 nets, including values and full footprint library identities. No rule exclusions were added.
- KiCad schematic export to PDF and raster rendering: completed and visually inspected. Labels, four series/pulldown channels, connector identities and jayis1 attribution are legible without obvious overlap. SVG export also succeeded; optional CairoSVG conversion was unavailable, so the PDF/poppler rendering path was used instead.
- `python3 tools/check_release.py`: PASS. README contains 3,471 whitespace-delimited words. Production `.c` files contain 593 nonblank lines after stripping comments, excluding headers and tests. Required native CAD files exist; project deliverables carry jayis1 attribution. The check also rejects private-key markers and runs `git diff --check`.
- Protocol marker search found AD1 in this device's encoder and no sibling `wire.c` collision. A focused read-only advisory correctness review reported no actionable findings; the owner retains responsibility for acceptance and publication.

Tests were written before adding the new engine implementation and app contract. Initial native compilation failed because the new engine header did not yet exist; initial app discovery failed because its new contract module did not yet exist. Later full builds and tests passed. These failures establish the initial missing implementation, not comprehensive mutation-test coverage.

## Electrical cross-check and limitations

ST's manufacturer-indexed UM1724 connector table for NUCLEO-F401RE identifies CN8 A0/A1/A2/A3 as PA0/PA1/PA4/PB0. Its USART section identifies PA2/PA3 and the default ST-LINK VCP solder-bridge arrangement. The board driver maps those inputs to sample bits zero through three; the carrier labels match that mapping. Check the current complete manual and actual purchased revision before wiring: https://www.st.com/resource/en/user_manual/um1724-.pdf .

The external Nucleo supplies MCU, power regulation, reset, decoupling and programming. ERC on this passive carrier cannot establish the external source voltage, ground compatibility, module solder bridges, logic thresholds or physical safety. No target was probed. No real reference trace was captured. All fixture data is synthetic.

Still unperformed: fabrication and assembly, physical firmware programming, scope correlation, power/current measurements, oscillator calibration, harness and connector fit, loading and back-power measurements, watchdog reset behavior on actual silicon, USB unplug behavior on a real module, ESD/EMC and environmental testing, regulatory review, and target-adapter fidelity validation. No secure boot, cryptographic evidence authenticity, persistent boot identity, or proof of actual memory reclamation is supplied.

## Reproduction

Run commands from the device directory as shown in README.md. Build output and personal KiCad preferences are ignored. A source-only checkout requires rebuilding before running app tests, because the GUI tests intentionally consume fixtures emitted by the production C encoder. No remote service or physical target is needed for these checks.
