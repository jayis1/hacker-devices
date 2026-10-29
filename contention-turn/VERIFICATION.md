# Contention Turn verification

Author: jayis1.

## Execution mode and scope

Standalone owner implementation and publication, with advisory material and a read-only independent software review. This is not full collective approval. The author and creator of the repository deliverables is jayis1. Work is limited to contention-turn/ in jayis1/hacker-devices; no daily-ideas content or unrelated repository files were modified.

The initial checkout was clean on the remote's configured main branch. `git pull --ff-only` reported already up to date. Existing directories and README titles were inventoried. Closest contracts examined included Admission Priority, Baton Order, Starvation Window, Handoff Audit, and Pulse Envelope. A proposed dead-time observer was discarded because Handoff Audit already covers it. Contention Turn instead implements demand-aware round robin with explicit unseeded history and recovery after ambiguous admissions. No worldwide novelty claim is made.

## Executed checks

Commands were run from the repository root:

- `make -C contention-turn/firmware clean all test`: PASS. Arm GNU GCC 14.2.1 produced a Cortex-M4 ELF and binary with warnings treated as errors. The linker reported text 3408 bytes, data 0 bytes, BSS 1676 bytes. Startup vectors, linker script, GPIO/SysTick/watchdog support, and transmit-only USART driver are included.
- Four C test executables passed. Engine and edge suites used AddressSanitizer plus UndefinedBehaviorSanitizer. Serial and scheduler tests used UndefinedBehaviorSanitizer. Tests include demand-aware alternation, legitimate uncontended repetition, late demand during held ownership, exhaustive request/side/predecessor combinations, mirrored simultaneous arrival/withdrawal, direct handover and reseeding, independent-channel chatter, gaps, wrap, startup high, counter exhaustion, overlap overflow, and transactional simultaneous collisions.
- A deterministic independent last-served oracle checked 10,000 admissions against production engine counters and preference. This is synthetic software stimulus, not physical traffic.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s contention-turn/tests -p 'test_*.py' -v`: PASS, 17 tests, Python 3.13.5. A real Tk event loop and Linux serial pseudo-terminal exercised screens, advancing data, duplicate connection refusal, hangup, and retained provenance. Other tests covered corruption, CRC-valid impossible records, unsupported policy, rollback/boot inference, backlog, duplicate timestamps, setup restoration, private exports, overwrite refusal, FIFO/oversize rejection, failed-open evidence preservation, and empty replay handling.
- The actual C encoder produced both the ordinary fixture and a gap/rebaseline/reseed/terminal-halt fixture consumed by the Python model. The latter was added test-first: its initial expected gap assertion failed before recovery-fixture implementation; the completed suite passes.
- `/usr/bin/python3 contention-turn/tools/generate_hardware.py`: PASS with KiCad 9.0.2, creating native schematic, routed PCB, project, authored symbol library, connectivity manifest, and XML netlist.
- `/usr/bin/python3 contention-turn/tools/check_hardware.py`: PASS. ERC: zero violations. DRC: zero violations and zero unconnected items. Exact comparison confirmed 14 components, 30 electrical pads, and 9 nets across fresh schematic netlist, saved netlist, PCB pads, values, footprint identities, and manifest.
- Schematic SVG export and PNG rendering succeeded. Visual inspection found the four request/grant channels, series resistors, pulldowns, ground-only J1, signal J2, and jayis1 title attribution legible without apparent crossed labels. This is not electrical measurement or purchased-footprint dimensional verification.
- `python3 contention-turn/tools/check_release.py`: PASS. README has 3364 whitespace-delimited words. Production .c files contain 551 nonblank lines after stripping comments, excluding headers and tests. Every delivered text file has jayis1 attribution. Required native KiCad files exist. Whitespace and private-key-pattern checks passed.

## Advisory review and response

A separate read-only software reviewer found no concrete correctness defects in the engine, encoder, companion, or documented round-robin semantics. Its nonblocking recommendations were applied by the owner: mirrored B-first edge cases, reseeding after direct handover, simultaneous-collision/overlap overflow rollback, and C-to-Python gap/reseed/halt coverage. The full owner validation sequence above passed after these additions. Review is advisory and does not substitute for the executed checks or hardware qualification.

## Explicitly not verified

No physical carrier was fabricated or assembled. No MCU was programmed or exercised on a Nucleo. Actual target GPIO acquisition, 115200-baud electrical behavior, loading, oscillator accuracy, inter-port skew, brownout behavior, watchdog timing, USB power consumption, ESD/EMC, enclosure mechanics, and regulatory suitability remain untested. Manufacturer board revision, solder bridges, component dimensions, connector orientation, and ground compatibility must be checked before construction and attachment.

The carrier is passive and unisolated; clean ERC cannot validate the external Nucleo power domain or safe target voltages. The interface is slow common-ground 3.3 V diagnostic logic only. GPIO indications are not authenticated and cannot prove real work completion or an underlying security property against a lying target. Counter/CRC checks are necessary consistency bounds, not cryptographic evidence or exhaustive waveform-realizability proofs. Passing software and CAD tests means the design artifacts are build-tested, not production-qualified.
