# Context Stack verification record

Author: jayis1.

## Executed checks

The release was built and checked locally from source. This record describes software and CAD validation, not a fabricated or programmed instrument.

- `make -C firmware clean all test`: passed. Arm GNU GCC 14.2.1 linked the actual Cortex-M4 image against the STM32F401RE memory layout. Size: 3,388 bytes text, zero initialized data, 1,676 bytes BSS. ELF, binary, map, native executables and caches are ignored build products.
- Engine test: passed with AddressSanitizer and UndefinedBehaviorSanitizer. Exercises nested matching, mismatched return, underflow, depth-eight capacity and ninth-entry overflow, simultaneous operations, active-stack acquisition loss, permanent invalidation, unstable setup, rejected short pulses, equal-tag reentrancy, transactional exhaustion, encoder capacity refusal, timer wrap and 100,000 randomized input samples with validity checks after each sample.
- UART register-model test: passed with UndefinedBehaviorSanitizer. Exercises submission bounds, busy refusal, byte ordering, stalled output timeout, wrap and recovery. This is modeled memory, not physical USART testing.
- Production main-loop test: passed with UndefinedBehaviorSanitizer. Exercises repeated policy after a rejected boot frame, periodic status, watchdog feeds and failed-watchdog startup before any evidence is emitted.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: all 12 tests passed. Python 3.13.5; real Tk event loop and Linux pseudo-terminals. Covers the production-C synthetic fixture, framing/CRC, incompatible policy, impossible valid-CRC stack snapshots, duplicates, rollback, pause/backlog detection, file limits, FIFO refusal, export permissions and overwrite refusal, all screens, replay/export UI, failed connection preservation and serial hangup.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native schematic, library symbols, PCB, project, XML netlist and connectivity manifest using KiCad 9.0.2.
- `/usr/bin/python3 tools/check_hardware.py`: passed. Schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. Exact schematic/PCB/manifest agreement for 14 components, 30 electrical pads, nine nets, component values and full footprint identities. No rule waivers were required.
- Schematic exported to SVG and rendered to PNG for visual inspection. Four labelled channel rows, resistor values, input/header ordering, ground connections and author title are visible without overlapping labels. Render files remain local scratch outputs, not release dependencies.
- Read-only review found no blocking engine/app correctness defects. Its missing verification-document observation is resolved by this file. Release readiness remains limited to the checks explicitly listed here.

## Reproduce

From the device directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

`check_release.py` enforces README word count, at least 500 nonblank/noncomment production `.c` lines (tests and headers excluded), attribution on every distributable file, required deliverables, no symlinks and a basic literal-secret pattern check. It does not claim a comprehensive security audit. Firmware headers, source and copied platform components preserve jayis1 attribution.

## Known limits and outstanding work

No fabricated board, actual Nucleo programming, silicon execution, target attachment, electrical loading test, timing calibration, ESD/EMC assessment, environmental qualification or safety certification has occurred. Zero CAD violations do not establish correct assembly or safe deployment. Check manufacturer documents, purchased components, harness and board revision before fabrication; a current ST manual could not be retrieved through the configured web extractor during this run.

The stack begins empty by an operator-controlled target baseline convention. Quiet signals cannot prove an empty target stack. Any correlation loss requires a new actual target baseline and observer restart; automatic resynchronization is deliberately absent. Four tags are context classes, not unique call identities. Equal-tag substitutions are invisible. CRC and serial freshness checks do not authenticate a sender or guarantee reset detection. Capture is aggregate evidence, not an edge trace.

Authorization must cover physical attachment and diagnostic target instrumentation. Never deploy on a safety-critical or production system on the strength of this validation record.
