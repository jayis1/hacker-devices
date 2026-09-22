# PDM Trust Probe hardware review notes

Author: jayis1

Status: A0 reference design — unverified. Do not fabricate or connect to valuable equipment without independent electrical review.

The schematic and PCB use KiCad 8 S-expression source, real library identifiers, assigned footprints, named nets, a four-layer stack, an Edge.Cuts outline, and representative routing. `kicad-cli` was not available in the generation environment, so ERC and DRC have not been run. Before fabrication, open both files in KiCad 8, update symbols from the official libraries, annotate, import the schematic netlist into PCB, and resolve every ERC/DRC finding.

Review the STM32H563 exact pin mapping, iCE40UP5K BGA escape, SN74AXC4T245 direction/enable strapping, TMUX1574 bandwidth and fail state, target-VIO absolute maximums, USB-C CC resistors, power sequencing, decoupling, FPGA configuration flash, ESD leakage, relay topology, and all FFC pinouts. Add mounting holes, test pads, impedance constraints, full power circuitry, and manufacturing outputs only after review.

Privacy and safety are design requirements: the passive receiver path must not retain raw PDM data; the active path must accept only fixed non-speech patterns; target VIO is sensed and never sourced; and reset, watchdog timeout, or power loss must select bypass. These properties require schematic, RTL, firmware, and bench verification rather than documentation alone.
