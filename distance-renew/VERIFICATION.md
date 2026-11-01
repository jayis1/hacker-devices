# Distance Renew verification record

Author: jayis1.

## Readiness

Build-verified, CAD-checked design prototype; not fabricated or hardware validated. Review was performed in standalone-owner mode using advisory requirements, not a full collective review. All hardware, firmware, application and documentation authorship is jayis1. Reused jayis1-authored Paired Lead infrastructure is disclosed in README.md. Third-party tool and footprint licenses remain unchanged.

## Executed checks

- Repository preflight: clean main at 65367aeef41badb9f86683ed2d763d35974fe170, origin https://github.com/jayis1/hacker-devices.git, remote symbolic HEAD main. Fast-forward-only git pull completed before concept selection. Existing directory names, README titles and closest policy contracts were inspected. No task branch was created and no daily-ideas repository was touched.
- Test-first engine check failed because engine.h was not yet present. After implementation, the independent recent-two-identity queue oracle checked 81,000 operation transitions through depth eight. Engine eligibility uses ordinals, not this queue representation. Explicit tests cover zero/one/two intervening admissions, all identity rotations, three-resource cycles excluding the fourth, occupied-resource duplicate precedence, simultaneous strobes, raw setup ambiguity, acquisition loss, terminal freeze, clock wrap and transactional diagnostic overflow.
- make -C firmware all test passed. Arm GCC 14.2.1 linked the actual Cortex-M4 target with warnings as errors: text 3640 bytes, data 0, BSS 1692. ELF and raw binary were produced under ignored firmware/build/. Host GCC 14.2.0 ran the engine with AddressSanitizer and UndefinedBehaviorSanitizer. UART tests exercised bounded submit, backpressure, timeout, recovery and timestamp wrap against mapped host registers. Scheduler tests exercised lost boot, repeated compatibility, transport failure and watchdog-start failure using controlled board functions.
- A new recent-ordinal regression failed before strengthening validation: a checksum-valid endpoint could omit an ordinal that must still exist in the last three admissions. Matching C and Python checks now reject that state. Counter/stamp conservation, unique nonzero stamps, latest-three coverage and sparse stamp mutation checks are exercised.
- xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v passed 15 tests under Python 3.13.5. Real Tk screens consumed actual C-encoder fixtures. Linux PTYs exercised serial receive, double-open refusal, hangup and terminal-setting restoration. Tests also cover invalid fields, impossible ordinal evidence, incompatible policy, boot after evidence, parser resynchronization, corrupt CRC, truncated tails, stale live data, FIFO/oversized replay rejection, private JSON creation and overwrite refusal. Fixtures are synthetic, not physical measurements.
- python3 -m compileall -q app passed. Imports resolve in the documented executable application and test entrypoints; the app uses no third-party Python dependencies.
- KiCad CLI 9.0.2 generation and validation passed: zero ERC violations, zero DRC violations and zero unconnected items. Independently exported schematic connectivity matched PCB pads, saved netlist and manifest for 14 components, 30 pads and 9 nets. Reference values and full library footprint identifiers matched. Native schematic SVG was rendered and visually inspected: component references, input labels, resistor values, connector map and jayis1 title-block attribution were legible without observed overlap.
- tools/check_release.py passed with 3347 README words and 589 nonblank production C lines after removing comments, excluding tests and headers. All deliverable text formats include jayis1 attribution. Build products, Python caches and KiCad local preferences are ignored. Whitespace checks passed.

## Limits and physical acceptance gates

No assembled Nucleo/carrier was available to this run. No real GPIO capture, ST-LINK programming, USB interoperability test, oscillator calibration, reset/brownout campaign, leakage measurement, ESD test or regulatory evaluation occurred. Passive-symbol ERC cannot validate the external Nucleo or make this unisolated circuit safe for arbitrary voltages. The reused pin map was checked internally; a fresh manufacturer-document electrical review was not performed. Review the actual board revision, VCP bridges, component drawings, ground continuity, input loading and signal thresholds before assembly.

Sparse host checks are necessary bounds, not proof that every hidden operation followed policy. CRC is not authentication. Admission distance is not elapsed-time quarantine, reclamation safety or proof of sanitization. Resetting volatile observer state does not discharge target obligations. The README's authorized-use limits and bench acceptance procedure remain mandatory.

## Publication

The release is intended for direct publication on the configured main branch with the commit title "Add distance-renew: receive-only admission-distance resource reuse witness". The final remote commit identity and clean/default-branch readback are reported by the execution result rather than embedded here as an unverifiable self-referential hash.
