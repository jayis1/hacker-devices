# Edge Fit verification record

Author: jayis1.

Verified on 2026-11-24 in the jayis1/hacker-devices checkout. This is a software and CAD verification record for an unbuilt engineering prototype, not a physical test certificate. Review was standalone-owner review with advisory reports, not independent collective approval.

## Recovery and scope

The existing untracked edge-fit directory was traced to the interrupted November 23 execution of this same authorized device task. Its originating session recorded the device-specific component-count oracle tests and implementation work. Provenance was reconfirmed against session local-d5685326290ef088cd78f038832d4a6859d1e68b5a6c279ea1cf2e0d11595484, including creation of tests/test_edge.c. The interrupted output was recovered rather than discarded or treated as unrelated work. All tests below were freshly executed again for publication; old build products and the pre-existing verification report were not accepted as execution evidence.

The checkout remote is https://github.com/jayis1/hacker-devices.git. Remote HEAD names main. The initial pull was fast-forward-only and reported already up to date at b08b809ae17d0eaca69525d86f18c6a38dcf5088. Publication is restricted to edge-fit; no other repository or shared hardware tree is involved. No task branch is needed.

## Executed checks

Commands are run from the repository root unless specified otherwise.

| Command or inspection | Observed result |
| --- | --- |
| `make -C edge-fit/firmware clean all test` | Target compilation and linking passed with warnings treated as errors; fresh ELF and binary generated |
| Target size | 3660 bytes text, 0 initialized data, 1692 bytes BSS |
| Host engine tests | 10,344 independent component-count oracle transitions passed, plus occupancy masks, policy findings, malformed interfaces, wrap and exhaustion checks |
| Host serial tests | Bounds, timeout, recovery and wrap passed |
| Production main-loop tests | Late-attachment policy, lost boot, timeout and watchdog-startup checks passed |
| `xvfb-run -a python3 -m unittest discover -s edge-fit/tests -p 'test_app.py' -v` | 25 tests passed, including actual Tk screens and Linux pseudo-terminal integration |
| `/usr/bin/python3 edge-fit/tools/check_hardware.py` | ERC: zero violations; DRC: zero violations and zero unconnected items |
| Fresh schematic netlist versus PCB and manifest | Exact match: 14 components, 30 pads, 9 nets, including values and full footprint library identities |
| `python3 edge-fit/tools/check_release.py` | README exceeds 2000 words; production C exceeds 500 substantive lines; every deliverable contains jayis1 attribution |
| Release counts before this record | 3199 README words; 598 nonblank, noncomment production C lines; tests and headers excluded from the C total |
| Native SVG exports and visual inspection | Schematic labels and component references readable; PCB routing and warning silkscreen inspected |

Engine host tests use AddressSanitizer and UndefinedBehaviorSanitizer. Serial and main-loop tests use UndefinedBehaviorSanitizer. The target is STM32F401RE, Cortex-M4 Thumb, soft-float, nominal reset HSI clock. Tool versions observed: arm-none-eabi-gcc 14.2.1 (20241119), KiCad CLI 9.0.2, Python 3.13.5.

The graphical tests instantiate the real companion, exercise its three screens, pass actual C-encoder fixtures through replay, open a Linux PTY for live input, check disconnect and termios restoration, refuse duplicate connection and overwrite, and preserve evidence after permission denial. These are synthetic software traces, not fabricated physical measurements. Sparse-history tests separately cover impossible hidden interior admissions and legal neighboring admissions before interior reuse.

## Cross-artifact traceability

| Contract | Implementation and evidence |
| --- | --- |
| Four diagnostic bits, input-only | README connector table; firmware/board.h; drivers/board.c GPIO mapping; J2 and four series/pulldown channels in schematic and PCB |
| No free-run splitting on admission | firmware/engine.c ledger_eligible; tests/test_edge.c independent connected-component oracle; app/contract.py endpoint extraction and bounded path search |
| First fault freezes coherent occupancy | Transactional ledger_sample and ledger_halt; host fault/exhaustion tests; companion halted-immutability tests |
| EF1 identity and compatibility | firmware/wire.c periodic policy; main-loop late-attachment test; companion foreign-protocol and policy-reset tests |
| Evidence export and transport failure | app/console.py private exclusive JSON export, receive-only serial and disconnect handling; PTY and GUI export tests |
| Real electrical connectivity | Native schematic export, native PCB pads and connectivity.json compared by tools/check_hardware.py |

## Limitations and release boundary

No board has been fabricated, assembled, flashed or electrically measured. Register-level hardware drivers are target-linked, but host tests do not emulate MCU electrical behavior. External Nucleo pin mapping, actual solder-bridge configuration, oscillator tolerance, watchdog behavior, diagnostic pulse margins, resistor loading, harness polarity, voltage compatibility and interference must be checked on the purchased hardware before use. The passive-only schematic does not validate the external module's supply circuit.

The instrument is not a hardware-enforced data diode, authenticated logger, general memory monitor, security interlock or proof of exploitability. Necessary sparse endpoint checks do not prove every hidden ordering. Authorization must cover physical attachment and diagnostic instrumentation. No global originality or patentability claim is made; the distinct contribution is the no-split accepted-trace contract and instrument workflow relative to the inspected repository.

Build products, Python caches, local KiCad preferences and transient render outputs are excluded from version control. Native SVG exports were rasterized using rsvg-convert for visual inspection; CairoSVG was unavailable and was not needed. Git HTTPS publication uses the existing Git credential helper; the separate gh CLI is not authenticated. A Git push dry run succeeded. Commit and remote publication identifiers are reported by the executing task after the remote is read back, rather than guessed in this pre-commit record.
