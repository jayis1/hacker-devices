# Replay Sieve verification

Author: jayis1.

## Executed checks

- Repository baseline: jayis1/hacker-devices, configured remote default branch main; initially clean. `git pull --ff-only` returned already up to date. Existing device directories and design titles were inventoried; closest contracts Slot Alias, Epoch Witness, and Quiescent Cut were read. This work does not touch daily-ideas.
- `make -C replay-sieve/firmware all test`: passed. Real STM32F401 Cortex-M4 target linked with Arm GNU GCC 14.2.1, Thumb soft-float, newlib/nosys, `-Wall -Wextra -Werror`. ELF reported text 3144 bytes, data 0 bytes, BSS 1652 bytes. These are linker measurements, not measured MCU performance.
- Host C engine tests passed with address and undefined-behavior sanitizers. Coverage includes all first-use permutations of four identities and repeated identities in each order, new-session reuse, absent/tied session, unstable tag, held acceptance, raw session withdrawal, short session glitches, startup mid-session, acquisition gap, timer wrap, transactional exhaustion, and an independent boolean-set oracle over 2000 paced admissions.
- UART register-model tests passed: bounds, busy rejection, timeout, recovery, and timer wrap. Main-loop tests passed: missing boot record, repeated late-attachment policy, transmitter timeout diagnostics, watchdog startup failure. These are host-controlled register/scheduler tests, not hardware execution.
- `xvfb-run -a python3 -m unittest discover -s replay-sieve/tests -p test_app.py -v`: 12 tests passed using Python 3.13.5. Real Tk screens and Linux pseudo-terminals were exercised with synthetic traffic from the production C encoder. Tests include live reading, replay, JSON export, overwrite refusal, file permissions, FIFO and oversized-file rejection, connection failure, double connection, termios restoration, hangup, stale data, gradually developing backlog, reset suspicion, malformed/contradictory frames, and bitmap transitions.
- Independent read-only review identified two sparse-telemetry bounds that were too permissive across session changes. Both defects were reproduced as failing checksum-valid frame tests, corrected by the sole writing owner, and verified by the complete application suite. New membership needs new fresh admissions; a completed session cannot contribute unused admission capacity. No review agent wrote repository files.
- `/usr/bin/python3 replay-sieve/tools/generate_hardware.py`: generated native symbols, schematic, PCB, project, XML netlist, and connectivity manifest using KiCad 9.0.2.
- `/usr/bin/python3 replay-sieve/tools/check_hardware.py`: zero ERC violations, zero DRC violations, zero unconnected items. Exact schematic/PCB/manifest equality checked across 14 components, 30 pads, and 9 nets, including values and full footprint identities.
- Schematic and PCB exported with KiCad CLI to SVG, converted with rsvg-convert, and visually inspected. Labels, author attribution, outline, and routed passive topology were visible without identified text collisions. The initial PCB SVG command used the wrong output form and failed; the successful export used `--mode-single` and an explicit file path. CairoSVG was unavailable, so rsvg-convert was used instead.
- `python3 replay-sieve/tools/check_release.py`: README 3276 whitespace-delimited words; 517 nonblank production C lines after comments removed. Tests and headers are excluded from that C count. Authorship, required CAD artifacts, private-key pattern checks, and whitespace checks passed. Generated builds and local KiCad preferences are ignored rather than published.

## Reproduce

From the repository root:

```sh
make -C replay-sieve/firmware all test
xvfb-run -a python3 -m unittest discover -s replay-sieve/tests -p test_app.py -v
/usr/bin/python3 replay-sieve/tools/check_hardware.py
python3 replay-sieve/tools/check_release.py
```

The companion requires no third-party packages. The physical design uses an external NUCLEO-F401RE; the carrier is not a standalone MCU board. Firmware and app share a specified RS1 contract but do not authenticate telemetry or prove all possible valid-looking streams realizable.

## Not verified

No physical board was fabricated, no MCU was programmed, and no electrical target was connected. Timing calibration, GPIO direction on silicon, supply sequencing, loading, input thresholds, watchdog operation, ESD, EMC, enclosure fit, environmental reliability, and regulatory compliance remain outstanding. Fresh manufacturer-document verification was not performed; current UM1724, RM0368, and DS9716 and actual module configuration must be checked before assembly.

No real target security findings are claimed. Fixture streams are synthetic. The review was bounded advisory review in standalone-owner mode, not full collective review or certification. Publish only as an unbuilt research prototype; the README defines the authorization requirements and limitations.
