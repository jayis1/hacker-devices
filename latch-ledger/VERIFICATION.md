# Latch Ledger revision-A verification

Author: jayis1

## Executed checks

The following checks were executed in the implementation environment. These are software and design-file results, not physical measurements.

| Check | Result |
|---|---|
| ARM target build | PASS: ARM GCC 14.2.1, Cortex-M4 soft-float, C11, `-Wall -Wextra -Werror`; linked ELF and generated binary |
| Linked size | 2924 bytes text, 0 initialized data, 1744 bytes BSS |
| Host monitor/encoder tests | PASS with AddressSanitizer and UndefinedBehaviorSanitizer; debounce, missed-sample qualification, timestamp crossing, queue overflow, CRC, output bounds, saturation |
| Python unit tests | PASS: five tests covering CRC, stream recovery, invalid schemas, session boundaries, gaps, inferred restarts and freshness metadata |
| C-to-Python protocol integration | PASS: deterministic C-generated fixture parsed by the actual app; expected contact and pair results checked |
| Terminal app integration | PASS: real curses interface on a Linux pseudo-terminal, three screens, private export, overwrite refusal, serial disconnect, clean exit |
| Schematic ERC | PASS: zero violations, including warnings |
| PCB DRC | PASS: zero violations and zero unconnected items |
| Cross-artifact connectivity | PASS: 14 components, 30 pins, 10 nets; fresh KiCad schematic export agrees exactly with PCB and design manifest |
| Schematic rendering | PASS: exported PDF rendered and visually inspected; four channel rows, connectors, values and net labels present |
| README content | 3200 whitespace-delimited words at initial verification; required architecture, threat model, interfaces, use cases, limitations, attribution and disclaimer present |
| Firmware content | 572 nonblank, non-block-comment lines across firmware C sources and headers at initial verification; tests excluded |
| Attribution | Every deliverable file contains jayis1; generated build/cache/local-preference files are ignored |
| Independent review | Approved after fixes for missed-sample debounce qualification, per-row freshness, and lost boot-record handling |

## Repeat the checks

From the `latch-ledger` directory:

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p 'test_app.py'
python3 tests/integration.py
python3 tools/check_hardware.py
```

The target build uses the real linker script and startup implementation, not a host-only syntax check. The app integration fixture is synthetic and labelled as such in its source. It exercises implementation logic but cannot establish hardware timing.

`tools/generate_hardware.py` regenerates the design files using KiCad 9's Python interface and installed footprint libraries. Regeneration is not part of ordinary verification because it writes files. Always rerun ERC, DRC and connectivity comparison after regeneration. The checker invokes real KiCad exports; it does not merely compare the generator's own manifest against itself.

## Manufacturer documentation checked

ST UM1724 revision 13, table 16, page 43: CN6 pin 4 is 3V3; pins 6/7 are ground; CN8 pins 1/2/3/4 map to PA0/PA1/PA4/PB0. The USART section specifies the default PA2/PA3 connection through ST-LINK with SB13/SB14 enabled. ST's STM32F401xE device header was consulted for register bases and peripheral bit positions. The README provides source links, including the university mirror used when ST's direct PDF endpoint timed out.

Nominal resistor-divider calculations were evaluated programmatically: 3.3 V across 11 kΩ gives 0.3 mA; the 1 kΩ lower leg gives 0.3 V; four closed channels draw 1.2 mA. These are nominal design calculations and exclude the development board's own consumption.

## Not verified; required before relying on hardware

- Fabrication, assembly, dimensional fit of purchased parts, and bring-up.
- Programming or operation of a physical NUCLEO-F401RE.
- Actual input thresholds, noise susceptibility, oscillator accuracy, and transition timing.
- USB disconnect behavior of the physical ST-LINK bridge; the tested disconnect uses a pseudo-terminal.
- Watchdog timeout distribution, brownout behavior, thermal behavior, or contact wetting performance.
- ESD, emissions, immunity, safety certification, or operation outside the narrow dry-contact bench scope.

No production readiness, isolation, secure boot, signed evidence, or certification is implied. The full design is provided for authorized engineering evaluation with those boundaries explicitly retained.
