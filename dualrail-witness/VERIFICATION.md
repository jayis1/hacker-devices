# Dualrail Witness verification

Author: jayis1.

## Executed software and CAD checks

- Clean build: `make -C firmware clean all test` passed with ARM GNU GCC 14.2.1 and warnings treated as errors. The real Cortex-M4 image linked, with 3,052 bytes text, zero initialized data, and 872 bytes BSS. ELF inspection reports ARM EABI5, soft-float ABI, executable image, and entry point 0x08000175. Startup, linker script, board driver, UART, engine, and encoder are included.
- Four native C test executables passed. Engine, boundary, and main-loop tests use AddressSanitizer and UndefinedBehaviorSanitizer; the host-mapped UART register test uses UndefinedBehaviorSanitizer. The mapping tests software only, not an actual USART.
- Boundary tests cover fault qualification at 19, 20, and 21 milliseconds of silent invalid encoding; valid-state exhaustion of episode counters, duration counters, gap/missed diagnostics, and transmit diagnostics; and 10,000 deterministic mixed-vector batches with consistency and serialization checks at every sampled tick.
- Python 3.13.5: six model/parser/replay/export tests passed. Inputs include real production C-encoder synthetic packets, truncated-prefix recovery, malformed records, unsupported policy, selected impossible valid-CRC summaries, duplicate freshness, counter rollback, FIFO rejection, oversized replay, and empty replay.
- Three real Tk integration tests passed under Xvfb. Linux pseudo-terminals supplied paced C-generated snapshots. Tests exercised the Permission, History, and Integrity tabs, native JSON export logic, overwrite refusal, mode 0600, replay, disconnection/hangup, reconnect clearing, permission failure, serial attribute restoration after constructor failure, and host-pause backlog freeze. These are synthetic-transport tests, not a physical USB or target test.
- KiCad 9.0.2 generation and checking passed: zero ERC violations, zero DRC violations, zero unconnected items. Fresh schematic netlist, saved netlist, actual PCB pads, values, footprint library identifiers, and connectivity manifest agree: fourteen components, thirty pads, nine nets.
- Schematic SVG export succeeded. A PNG rendering was visually inspected: four distinct channel rows, readable resistor/header labels, correct left/right net-label placement, and jayis1 title-block authorship. Visual inspection is not a manufacturing signoff.
- README word count: 3,404 by whitespace splitting. Production `.c` files total 521 lines, including 501 nonblank lines after removing block comments; tests and headers are excluded from this count. These counts supplement functional checks rather than replacing them.
- All deliverable files contain format-valid jayis1 attribution. Build products, Python caches, and KiCad local preferences are ignored.
- Independent read-only review found no concrete blocking correctness issue. Its boundary/exhaustion coverage suggestions were implemented and passed; the missing verification record is supplied by this document.

## Research and limitations

Repository directories and README titles were inventoried after `git pull --ff-only origin main`. Handoff Audit and Quorum Witness were read in full to distinguish the complementary-code measurement contract. No global novelty claim is made. Existing jayis1 support-layer and carrier patterns are reused explicitly.

The ST manual's Table 16 mapping and virtual-COM solder-bridge guidance were retrieved through indexed manufacturer documentation. Direct PDF download attempts failed (HTTP/2 failure and HTTP/1.1 timeout); those downloads are not claimed as successful. Register definitions follow the existing STM32F401 support layer and RM0368. The physical module, purchased components, solder bridges, power budget, and actual electrical limits require a board-specific review before assembly.

Not performed: fabrication, assembly, ST-LINK programming, startup on silicon, electrical measurements, oscillator calibration, exact acquisition timing, physical watchdog recovery, ESD/EMC qualification, environmental or thermal qualification, USB interoperability, and deployment. No physical hardware measurements or fabricated captures are included. ERC/DRC cover the passive carrier, not the external Nucleo module or a target's electrical compatibility.

There is no secure updater, persistent evidence storage, authenticated pairing, or target command path. Brownout during persistent writes and remote command replay are therefore not implemented capabilities to be tested. Resets discard volatile evidence; some unannounced resets remain indistinguishable. CRC-valid input can be forged. Reports are ordinary unsigned local files, and snapshot invariants are not a complete waveform realizability proof.

## Reproduction

From this device directory:

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 -m unittest discover -s tests -p test_gui.py
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
arm-none-eabi-readelf -h firmware/build/dualrail-witness.elf
```

Use the system Python carrying KiCad bindings for hardware scripts. No downloaded application dependencies or vendor source relabeling are required. KiCad standard footprints retain their library identities; original board composition and authored symbols are credited to jayis1.
