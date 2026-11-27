# Retire Distance verification

Author: jayis1.

## Executed software checks

- GNU Arm Embedded GCC 14.2.1 target build with Cortex-M4/thumb/soft-float flags, C11, Wall/Wextra/Werror succeeded. ELF is ARM EABI5, entry 0x08000175. Linked sections: text 3784 bytes, data 0, BSS 1692, within 512 KiB flash and 96 KiB RAM; linker reserves at least 4096 bytes stack headroom.
- `make -C firmware all test` passed. The engine test exercised 24,520 transitions against an independent per-resident debt-token oracle, including inclusive maturity, non-FIFO retirement, immediate renewal, orphan and duplicate operations, interface ambiguity, idle time without age credit, sampling gap, timestamp wrap, counter exhaustion and bounded serialization. Engine uses address/undefined sanitizers; serial and main harnesses use undefined-behavior sanitizer.
- Serial tests passed bounds, stalled-transmitter timeout, recovery and wrap. Main-loop tests passed periodic compatibility, lost boot, late attachment, transmit timeout and watchdog-startup behavior using injected platform functions, not an actual MCU.
- `python3 tests/test_app.py`: six tests passed. Real production C fixture frames were parsed; all retained legal prefix pairs passed sparse checks; impossible immature disappearance/renewal and CRC-valid inconsistent status were rejected. Corruption resynchronization, incomplete tail accounting, private exports, overwrite refusal and FIFO replay rejection passed.
- `xvfb-run -a python3 tests/test_gui.py` passed actual Tk Ledger/History/Integrity screens and Linux pseudo-terminal read loop, with paced real C fixture output. It exercised the early-return finding, double-connect prevention, serial attribute restoration, replay, JSON export, invalid serial device and hangup. Fixture waveforms are synthetic, not physical measurements.

## CAD checks

KiCad 9.0.2 generated and parsed native schematic, PCB and project files. `tools/check_hardware.py` returned zero ERC violations, zero DRC violations and zero unconnected items. A freshly exported schematic netlist matched every PCB pad and connectivity-manifest reference, value and footprint: 14 components, 30 pads, nine nets. The saved netlist matched the fresh export. The schematic was exported to SVG, rasterized with rsvg-convert and visually inspected: labelled input rows, values, references and ground connections were legible without overlapping connections.

The schematic uses embedded jayis1-authored passive symbols and standard installed KiCad through-hole footprints. Passive-only ERC does not verify the external Nucleo power circuitry, module jumper settings or input voltage safety. This is a harness-connected passive carrier, not a complete standalone MCU board.

## Scope and release checks

The release checker measured 2,688 README words and 612 nonblank production C lines after stripping comments. Authorship and required KiCad filenames passed. RT1 was searched against repository C sources; matches belonged only to this new device. The only working-tree addition was retire-distance. A read-only review found no high-confidence policy mismatch; its missing verification-document finding is resolved by this file.

The prior interrupted implementation was recovered from the same task's recorded session. No unrelated untracked work was adopted. Existing platform components remain attributed to jayis1, and installed third-party tools retain their original licensing.

## Unverified physical work and source availability

No PCB fabrication, component assembly, MCU flashing, electrical bench testing, real target connection, calibrated timing, EMC/ESD evaluation or production reliability testing occurred. Stack behavior and watchdog timing on actual hardware remain unmeasured. The diagnostic adapter is trusted, and the supplied code cannot establish authenticity or complete hidden operation ordering.

An attempted refresh of the ST UM1724 PDF failed with an HTTP/2 error and an HTTP/1.1 timeout. Manufacturer document identifiers and product links are recorded in README, but the current manual revision was not independently retrieved in this run. Reconfirm the purchased module's connector map, virtual-COM bridges, supply arrangement and electrical limits against its manufacturer documentation before assembly or connection. The design retains the existing platform pin map; CAD consistency checks do not substitute for that physical review.

This package is a verified software/CAD prototype design, not a fabrication-qualified or physically validated instrument.
