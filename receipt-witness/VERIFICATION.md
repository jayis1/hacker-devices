# Receipt Witness verification

Author: jayis1.

## Executed checks

- Repository identity: `https://github.com/jayis1/hacker-devices.git`; remote HEAD is main. Starting tree clean. `git pull --ff-only origin main` completed before implementation. Existing device directories and README purposes inventoried; no changes to daily-ideas or unrelated device directories.
- Target build: `make -C receipt-witness/firmware all test`, Arm GNU GCC 14.2.1, Cortex-M4 Thumb, soft float, warnings as errors. Linked ELF reports text 2728 bytes, data 0 bytes, BSS 408 bytes. Stack-local FIFO and frame storage are additional to BSS. Binary generated successfully; linker reserves at least 4096 bytes for stack.
- C policy and encoder tests pass under address/undefined-behavior sanitizers: normal receipts, same-tick append, same-tick premature durability, orphan acknowledgments, blocked action, append deadlines at 50/51, durability deadlines at 200/201, FIFO overflow, FIFO wrap and full-capacity replacement, acquisition gaps, fault-tick operation conservation, held pulses, independent qualification under chatter, short-pulse rejection, timer wrap, saturating arithmetic, CRC known vector, and bounded output.
- Production UART tests pass with a Linux memory mapping and undefined-behavior sanitizer: invalid buffers, busy submission, nonblocking transmission, timeout, timeout reset, recovery, and wrapping clock. This is not a hardware USART test.
- Production main-loop test passes with synthetic clock/transport: rejected boot submission, timeout accounting, three periodic policies, twenty statuses, and 2100 watchdog feeds. This is not physical watchdog timing validation.
- `xvfb-run -a python3 -m unittest discover -s receipt-witness/tests -p test_app.py -v`: 13 tests pass with Python 3.13.5. Includes real C fixture parsing one byte at a time, CRC/resynchronization, bounded malformed input, queue and append conservation, counter and sticky-fault regression, duplicate freshness, reset freeze, timer wrap, private no-overwrite JSON export, FIFO/oversized replay refusal, terminal restoration on open failure, buffered-hangup logic, and real Tk event-loop operation through a Linux PTY.
- The Tk/PTy test verifies all nine fixture status rows rendered, receipt count displayed, hangup detected, and live provenance retained after disconnect. Synthetic fixtures are not measured target evidence. Buffered POLLIN+HUP behavior is separately tested with a mocked driver response because operating systems differ in whether final bytes survive disconnect.
- KiCad 9.0.2: generator produced native schematic, routed PCB, project, symbol library, manifest, and XML netlist. `tools/check_hardware.py` reports zero ERC violations, zero DRC violations, zero unconnected items, and matching 14 components / 30 pins / 9 nets across schematic, PCB, manifest, and saved netlist. No violation exclusions were added.
- Schematic SVG exported successfully and rendered to PNG for visual inspection. Four labelled resistor channels and ground-only connector are visible; native files remain the authoritative editable design.
- README: 3242 whitespace-delimited words. Production firmware: 611 physical C/header lines, 552 nonblank lines after removal of block comments, excluding tests and generated build outputs. Reused board, register, serial, and startup support is explicitly identified in README rather than claimed as new electrical functionality.
- Every tracked deliverable contains format-valid jayis1 attribution. Build products, Python caches, and KiCad local preferences are excluded. No dependency downloads, CI workflows, credentials, or unrelated repository changes were introduced.

## Review corrections

Read-only review identified loss of a simultaneous ACTION when timeout or orphan durability invalidated correlation. A failing regression reproduced it before correction; the policy now counts and abandons that operation while preserving blocked evidence. Additional failing tests reproduced impossible append-conservation records accepted by the app; validation now bounds appends belonging to abandoned operations. Serial hangup handling drains bytes retained by the driver before declaring disconnect. Deadline/FIFO, terminal restoration, and buffered-hangup regressions cover the relevant paths.

## Limits and outstanding physical work

No board has been manufactured, assembled, flashed, electrically measured, or timing-calibrated. No real target or physical USB device participated in these checks. ERC/DRC do not prove the external Nucleo harness mapping, supply sequencing, ground safety, ESD resilience, connector fit, or target loading. Component availability and purchased mechanical dimensions must be checked before procurement. A calibrated logic analyzer is needed to qualify the diagnostic adapter and compare its pulse timing with sampled results.

The protocol has neither authentication nor persistent boot identity. A target can fabricate diagnostic receipts; an observer reboot whose boot frame is lost can sometimes appear as forward traffic. Counters stop being algebraically invertible at saturation. The app's latest 256 statuses are a bounded history, not a complete event trace or forensic chain of custody. No claim of fabrication readiness, logging compliance, secure erasure, or physical operation follows from these tests.

## Reproduction

Run from repository root:

    make -C receipt-witness/firmware all test
    xvfb-run -a python3 -m unittest discover -s receipt-witness/tests -p test_app.py -v
    /usr/bin/python3 receipt-witness/tools/check_hardware.py

To regenerate hardware, run `tools/generate_hardware.py` with `/usr/bin/python3` before the checker; the KiCad pcbnew module and standard footprint libraries must be installed. To inspect a synthetic capture, redirect `receipt-witness/firmware/build/fixture` into a scratch file and use `python3 receipt-witness/app/audit.py --replay <capture>`. Use `--export <new-json-path>` for a headless report. Do not confuse this synthetic replay with a physical measurement.
