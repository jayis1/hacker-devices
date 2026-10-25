# Coincidence Guard verification

Author: jayis1.

## Scope

This report concerns the source and CAD in this device directory. All stimuli used during this run were synthetic. No physical prototype was assembled, programmed, connected to a target, or measured. This is a verified software/CAD prototype release, not fabrication or operational approval.

## Executed checks

- `make -C firmware clean all test`: passed with warnings treated as errors. Actual Cortex-M4 target ELF linked and BIN produced using arm-none-eabi-gcc 14.2.1 (20241119). ELF section sizes: text 2812 bytes, data 0, BSS 1372. Target flags are cortex-m4, thumb, soft floating point, reset HSI clock. No vendor SDK dependency is required.
- C state-machine tests with AddressSanitizer and UndefinedBehaviorSanitizer: passed. Covered both first-consent orders, simultaneous inputs, exact 250 ms acceptance, 251 ms rejection, anti-tie-down hand exchange, permission-before-pair, simultaneous pair/permission, inhibit, withdrawal, all-low recovery, missed ticks, duplicate ticks, bouncing vectors, uptime wrap, age saturation, counter saturation, queue overflow, all sixteen input vectors after pairing, CRC reference vector, and encoder output bounds.
- Production main-loop test with injected board/transport functions: passed. Covered rejected boot submission and retry, periodic policy announcements, regular status, continued watchdog feeding, accepted pair, and transport loss accounting. The production main function is included directly; this is not target execution.
- Production UART driver with host memory mapped at peripheral addresses and UBSan: passed. Covered null, empty, oversized, and busy submissions; TX-ready progression; timeout across timestamp wrap; timeout clearing; and recovery. Ordinary mapped host memory does not validate electrical UART timing or register side effects.
- `python3 -m unittest discover -s tests -p test_app.py -v`: six tests passed under Python 3.13.5. Covered freshness, replay provenance, counter rollback, boot versus repeated policy, truncated-frame resynchronization, hostile length/numeric bounds, incompatible policy, bounded events, exclusive private export, and serial attribute restoration on normal close, constructor failure, and hangup.
- `python3 tests/integration.py`: passed. Real C encoder output was replayed through the actual CLI, including a lost boot record followed by policy recovery. Separate Linux pseudo-terminals exercised the actual serial and curses paths, all three screens, live export, mode-0600 permissions, overwrite refusal, unplug, and clean exit. This establishes application functionality with simulated transport, not physical USB behavior.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native KiCad artifacts successfully.
- `/usr/bin/python3 tools/check_hardware.py`: passed with KiCad 9.0.2. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML, saved netlist, actual PCB pads, and manifest agree on fourteen components, thirty pins, nine nets, values, and footprint identifiers.
- Schematic SVG and combined copper/silkscreen PCB SVG exported through KiCad, rendered with rsvg-convert, and visually inspected. Labels, separated component rows, resistor connectivity, connector locations, and board outline were visible without obvious overlap. This is not a mechanical or fabrication signoff.
- README measured at 3245 whitespace-delimited words. Production firmware C/header files contain 634 physical lines, or 563 nonblank lines after removing C comments; tests are excluded. These counts describe size, not correctness.
- A separate read-only code review found no blocking correctness issue in the actual firmware, protocol, application, tests, or contract documentation. Its packaging note to supply this verification report is resolved by this file.

## Reference checks and limitations

The stored ST UM1724 Rev 13 manual was inspected for NUCLEO-F401RE CN8 mapping A0/PA0, A1/PA1, A2/PA4, A3/PB0 and USART2 PA2-to-ST-LINK routing. Section 6.8 describes default SB13/SB14 ON and SB62/SB63 OFF for the virtual COM path. Verify bridges on the purchased board rather than assuming a previous owner's configuration.

A fresh ST cmsis-device-f4 STM32F401xE header was retrieved from the manufacturer's GitHub repository. GPIO, RCC, USART2, and IWDG bases and relevant structure offsets were checked against the local register definitions. The direct ST PDF retrieval timed out; it was not represented as a successful fresh datasheet download. The manual available locally and the fetched vendor register header supported the specific cross-checks above. Neither vendor material is copied into the authored deliverable.

References:

- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- https://github.com/STMicroelectronics/cmsis-device-f4/blob/master/Include/stm32f401xe.h

The firmware uses the external development board's existing power, decoupling, and USB circuits. The custom carrier is not isolated and has no active protection. Input thresholds, loading, ESD behavior, back-power behavior, clock error, real sampling jitter, watchdog timing, cable effects, environmental performance, and regulatory suitability remain unverified. Those are blockers to physical deployment until measured and reviewed, not failures hidden by host tests. Do not connect to a safety-related or hazardous controller.

The app's reset inference is heuristic, CRC is unauthenticated, and JSON export is an unsigned bounded snapshot. A compatible LIVE indication is transport freshness only. The README specifies these limits and the exact observed timing contract.

## Reproduction

Run commands above from this device directory. Use the system Python for KiCad tools because pcbnew is supplied by the KiCad installation, not a project pip dependency. Build products, Python caches, and KiCad local preferences are ignored. Do not publish generated captures from real engagements. Review every staged file before committing, especially if another process has touched the worktree.
