# Retry Ledger verification record

Author: jayis1.

## Executed checks

- Destination checked: `https://github.com/jayis1/hacker-devices.git`; remote HEAD resolves to `main`. Initial tree was clean. `git pull --ff-only origin main` returned already up to date at `4c594f196ecd35b68549e91b8532d09b616f83fa`. No task branch created. Existing directory names and device README purposes were inventoried before selecting the retry-budget concept.
- `make -C firmware clean all test`: passed with Arm GCC 14.2.1 and host GCC 14.2.0. Target Cortex-M4 ELF linked against the STM32F401 memory map; text 3012 bytes, data 0 bytes, BSS 408 bytes. Stack-local policy state is additional RAM use, not included in BSS. ELF and binary are reproducible local build products and are ignored by Git.
- Host budget/encoder regression executable: passed with AddressSanitizer and UndefinedBehaviorSanitizer. Includes strike exhaustion, successful reset, conflicting and held outcomes, lock assertion exactly at the inclusive 20 ms deadline, expiry at 5000 ms, recovery baseline, missing/early lock, prohibited access, acquisition gaps, wrap, saturation, queue overflow, and CRC/buffer bounds.
- Independent-channel chatter regression was observed failing before the fix, then passed with per-channel qualification. The production policy previously qualified the whole vector; this was corrected before publication.
- Production UART-driver executable: passed with UndefinedBehaviorSanitizer, using a Linux memory-mapped test page rather than physical USART hardware. Covers invalid submission, nonblocking TX, timeout across wrap, recovery, and one-shot timeout reporting.
- Production main-loop executable: passed with sanitizer instrumentation and synthetic board clock/serial functions. Confirms a rejected boot transmission and subsequent timeout are counted, policy repeats for late attachment, status continues, and watchdog feeding remains scheduled. This is software execution, not execution on a physical MCU.
- `python3 -m unittest discover -s tests -p test_app.py`: 10 tests passed under Python 3.13.5. Production C encoder fixtures feed the real app parser. Tests exercise corruption, resynchronization after truncation, bounded buffering, wrong policy/fields, duplicate freshness, reset freezes, counter rollback, impossible exposure increments, sticky-fault loss, private exclusive export, actual Linux PTY input/hangup, and terminal restoration after injected constructor failure.
- `xvfb-run -a python3 tests/integration.py`: passed through the actual Tk event loop. Exercises Budget, Events, Integrity, synthetic replay, live PTY input, private JSON export, stale display, unplug detection, absent terminal, FIFO rejection, and oversized regular-file rejection. The PTY is synthetic; no physical USB bridge was available.
- `/usr/bin/python3 tools/check_hardware.py`: KiCad 9.0.2 ERC zero violations; PCB DRC zero violations and zero unconnected items. A freshly exported schematic XML netlist, saved netlist, manifest, and actual PCB pads agree on 14 components, 30 pins, 9 nets, values, and full footprint library identities.
- Schematic exported using `kicad-cli sch export svg`, converted using `rsvg-convert`, and visually inspected. Four labeled input networks, J1 ground-only connector, J2 analog-header mapping, resistor values, and jayis1 title attribution are present and readable. Rendered inspection files remain scratch artifacts, not fabrication deliverables.
- README measured 3226 whitespace-delimited words. Firmware C/header sources measured 627 nonblank lines after removal of block comments, excluding tests. Attribution scan found jayis1 in every intended source/design/document file, including format-valid CAD and JSON metadata.

## Review decisions and limitations

Read-only review identified whole-vector qualification masking, unbounded CLI replay allocation, FIFO replay blocking, and indistinguishable resets after lost boot records. The first three were corrected: qualification is per channel, replay reads a bounded regular file, and FIFO/device replay is rejected without waiting for a writer. The boot-identity limitation is retained explicitly rather than claiming authenticated continuity. A regression demonstrates that a forward-time idle reset without its boot record is indistinguishable; this is a documented limitation, not a successful reset-detection test. Adding a robust persistent session identity requires a separately reviewed flash persistence or trusted host handshake design and is not implied by RL1.

No secrets, payloads, identities, or authentication values are captured. No network service, target command path, radio transmission, new CI workflow, or repository-wide refactor was added. Reused board support and carrier topology retain jayis1 attribution. Only the new device directory is in scope.

Physical construction, programming, electrical loading, pin voltage measurements, USB power behavior, oscillator accuracy, regulator temperature, ESD tolerance, enclosure safety, and operation against a real target remain unverified. Manufacturer documentation download was attempted: the extract backend did not support extraction, then direct HTTPS failed with an HTTP/2 error and an HTTP/1.1 timeout. Register and connector definitions were checked against the existing repository board-support implementation; no claim is made that a fresh manufacturer PDF revision was retrieved during this run. Confirm current UM1724/RM0368/DS9716 and the actual board revision before assembly. The BOM sourcing examples need dimensional and availability review.

This release is a research prototype design with executed software/CAD checks, not a fabricated or production-qualified instrument. Generated test streams are synthetic. Zero CAD-rule violations do not establish electrical safety. The README's legal restrictions, common-ground/3.3 V limitations, no-isolation warning, prospective-segment semantics, and lost-boot caveat are part of the interpretation of every result.

## Reproduction

From `retry-ledger/`:

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 tests/integration.py
/usr/bin/python3 tools/check_hardware.py
```

Regenerate CAD only intentionally with `/usr/bin/python3 tools/generate_hardware.py`, then repeat the hardware checker. To exercise the GUI manually, run `python3 app/audit.py`; select the exact terminal path or use a regular synthetic replay file created by `firmware/build/fixture`. Export files are created exclusively and will not overwrite earlier evidence.
