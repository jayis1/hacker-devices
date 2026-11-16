# Reserve Last verification

Author: jayis1.

Execution mode: standalone owner implementation and self-review. Advisory reports were considered; this is not a completed independent collective review. The actual target repository is jayis1/hacker-devices, not daily-ideas or creative-projects. Baseline was 22e52856c5c855f4af44421178b3226a487af0f7 on configured default branch main; git pull --ff-only reported already up to date.

## Executed checks

- A new reserve test was first compiled against the inherited minimum-free engine. It failed at the assertion that an initially free ordinary identity two is eligible. The reserve rule was then implemented.
- `make -C firmware clean all test` target-linked the firmware with ARM GNU GCC 14.2.1, C11, Cortex-M4 Thumb, and -Wall -Wextra -Werror. ELF size: 3612 bytes text, zero initialized data, 1692 bytes BSS. The first main-loop test exposed an inherited LF1 expectation; it was changed to RZ1 and the complete test target passed.
- Separate independent occupancy-array and vacant-list oracles each checked 10232 operation transitions through depth seven. All occupancy subsets, retained reserve after ordinary returns, duplicate precedence, orphan returns, interface ambiguity, gap freeze, wrap, serialization and transactional exhaustion passed. Engine testing uses address/undefined sanitizers; reserve, serial and main-loop testing uses undefined-behavior instrumentation.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py'` passed 24 tests. These include real C-encoder fixtures, Tk screens, serial PTYs and hangup, private export and overwrite refusal, replay bounds, permission denial, terminal restoration, malformed framing, reset/rollback, drift, sparse ordinary-vacancy rejection, legal hidden fill, and retained reserve after ordinary returns. Legal sparse prefix pairs are independently enumerated through depth six.
- `/usr/bin/python3 tools/generate_hardware.py` produced native CAD sources and exported the saved netlist using KiCad 9.0.2.
- `/usr/bin/python3 tools/check_hardware.py` reported zero ERC violations, zero DRC violations, and zero unconnected items. Exact schematic/PCB/manifest/saved-netlist agreement passed for 14 components, 30 pads and 9 nets, including values and full footprint identities.
- The schematic was exported to SVG, rasterized using rsvg-convert, and visually inspected. Components, net labels, connector mapping, title and jayis1 attribution were readable without obvious overlap. Optional CairoSVG was unavailable; rsvg-convert provided the actual rendered inspection.
- `python3 tools/check_release.py` verified the README exceeds 2000 words and production C exceeds 500 substantive noncomment lines. It checks every deliverable for jayis1 attribution, rejects conflicting author notices and private-key material, requires the native KiCad files, and runs git diff --check.

## Review and limits

The platform drivers and register definitions are unchanged from the credited source. The reserve rule and sparse validator were reviewed against independent occupancy reasoning, including arbitrary ordinary choices and nonpreemptive reserve retention. No target-driving API, radio, credential collector, remote upload, dependency installation, or workflow automation was added. All changes are confined to reserve-last. Build products and local preferences are ignored.

Software and CAD checks do not establish manufacturability, electrical safety, or actual target behavior. This is an unbuilt prototype: no fabrication, physical flashing, bench voltage checks, oscillator calibration, ESD testing, or live-target interoperability was performed. The external Nucleo module revision, VCP solder bridges and silicon electrical limits require independent verification before assembly. Passive-only ERC cannot validate the external module supply. Sparse endpoint validation is not complete history reconstruction; CRC is not authentication.

Publication is performed directly on main with jayis1 author and committer identity. The final run report supplies the actual commit after push and remote readback; no temporary task branch is used.
