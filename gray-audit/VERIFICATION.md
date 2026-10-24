# Gray Audit verification record

Author: jayis1

## Scope

This record covers source, target linkage, host execution, application integration, and CAD validation. It does not claim that a board was fabricated, firmware was flashed, or electrical behavior was measured. Tests are synthetic and explicitly separate from physical evidence.

## Executed checks

Commands were run from the repository root:

```sh
make -C gray-audit/firmware clean all test
python3 -m unittest discover -s gray-audit/tests -p 'test_app.py'
python3 gray-audit/tests/integration.py
/usr/bin/python3 gray-audit/tools/check_hardware.py
```

Results:

- ARM target: successful compile, link, and binary extraction with `-Wall -Wextra -Werror`. GNU Arm Embedded GCC 14.2.1. ELF sections: 3,288 bytes text, zero data, 1,424 bytes BSS. This is a real Cortex-M4 target build, not a host-only syntax check.
- Core tests: passed under address and undefined-behavior sanitizers with host GCC 14.2.0. Includes legal movement, all sixteen state transitions on both axes, cycle completion, reversal, illegal diagonal rejection, sample-gap continuity, duplicate tick rejection, timer rollover, both signed position limits, unsigned saturation, queue overflow/draining, encoder capacity failure and invalid axis handling.
- Serial driver tests: passed under undefined-behavior sanitizer. A Linux mapped-register model exercises the unchanged production driver with invalid submissions, busy rejection, byte-by-byte transmission, non-ready peripheral, timeout across timer rollover, timeout acknowledgement and recovery. This does not simulate real UART electrical timing.
- Python unit suite: four tests passed on Python 3.13.5. Covers CRC/schema errors, ranges and invalid event axes, fragmented input, oversized-frame discard/recovery, ordinary timestamp wrap, rollback inference, delayed events, private export permissions and overwrite refusal.
- PTY integration: passed. Compiles a synthetic fixture against the production C analyzer and frame encoder, sends its frames through a separate serial pseudo-terminal to the real curses application, and checks position/cycle/illegal counts. Exercises all three screens, export, repeat-export refusal, disconnect display and clean quit.
- KiCad 9.0.2 ERC: zero violations. DRC: zero violations and zero unconnected items. No rule violations were excluded or waived.
- Connectivity checker: 14 components, 30 pins, 10 nets match between a fresh schematic XML export, actual PCB pads and the design manifest. Component values and complete library-qualified footprint identifiers also agree.
- Schematic exported to PDF and visually inspected: references, values, net labels, connector pins and jayis1 title credit render. The custom board is a passive carrier; the external Nucleo board is explicitly outside the custom schematic boundary.
- Source-size check: 514 nonblank, non-comment lines in production `.c` files; 633 including production headers. Tests, generator output and third-party code are excluded. README has 3,432 whitespace-separated words.
- Independent read-only review found no blocking code defect. Its missing-verification-document finding is resolved by this report. The noted transition/serial coverage gaps were strengthened with exhaustive two-axis transition checks and the serial register-model test before the final successful run.

## Reference and electrical checks

A cached manufacturer UM1724 Rev 13 manual was consulted for the Nucleo Arduino analog-header mapping and default ST-LINK USART bridges. Fresh retrieval of the ST-hosted PDF failed first with an HTTP/2 error and then a bounded timeout; no successful fresh download is claimed. The README links the primary source and requires checking the actual board revision before assembly.

Nominal resistor-network calculations were recomputed: 3.3 V / 11 kΩ = 0.300 mA per closed contact; 0.300 V at the sense node; four closed channels consume 1.20 mA. These values are not measurements. Manufacturer part drawings, purchasing availability, contact wetting-current compatibility and physical tolerances remain assembly-review responsibilities.

## Known limitations and remaining acceptance work

- No fabricated prototype, programmed MCU, measured GPIO thresholds, measured USB supply behavior, oscilloscope traces or encoder dataset.
- Sampling is software polling at nominal millisecond intervals, not a guaranteed high-speed capture engine. GPIOA/GPIOB inter-port skew is unmeasured. Entire between-sample events can be invisible.
- Reset-entry firmware assumes the documented HSI default configuration, no bootloader and no debugger-modified clock setup.
- No isolation or field-input protection; only detached voltage-free mechanical contacts are permitted. No powered encoder or live controller connection is supported.
- Counters and position are observations, not authenticated or calibrated shaft motion. Mechanical chatter is not automatically malicious activity.
- A lost boot frame can make some MCU restarts indistinguishable from continuity; timestamp-based restart inference is heuristic.
- CRC is not authentication. Exports are private local files, not signed forensic evidence. File permissions and host trust remain operator responsibilities.
- KiCad's passive-pin ERC does not validate external supply behavior, development-board hardware or cable correctness. Clean DRC does not establish EMC, assembly yield or electrical safety.
- No physical watchdog/brownout recovery, ESD, environmental, regulatory or enclosure testing has been performed.

The README includes the bring-up procedure and directs operators to resolve those physical checks before any use beyond a supervised laboratory prototype.
