# Phase 3: PCB Blueprints & Layout - Volt-Glitcher

## PCB Stackup (4-Layer)
1. **Top (Signal/High-Current)**: Primary signal routing, MOSFET Q1 placement, high-current ground planes for crowbar.
2. **Inner 1 (Ground)**: Continuous solid ground plane for impedance control and noise suppression.
3. **Inner 2 (Power)**: Divided power planes (3.3V, 1.2V, Target_VCC).
4. **Bottom (Signal)**: Secondary signal routing, level shifters, decoupling capacitors.

## High-Speed Routing
- **USB 2.0**: Routed as 90-ohm differential pairs on Top layer. No vias if possible.
- **FPGA-MCU Bus**: Routed with length matching and 50-ohm single-ended impedance on Top layer.
- **ADC Interface**: 8-bit parallel bus routed with strictly matched lengths to minimize skew at 100MHz.

## Glitch Path Optimization
The `MOSFET Q1` must be placed as close as possible to the `TARGET_VCC` header to minimize parasitic inductance.
- **Trace Width**: 2mm for `TARGET_VCC` and `GND` return paths involved in the glitch.
- **Vias**: Multiple 0.3mm vias in parallel for high-current paths to reduce resistance and inductance.

## Thermal Management
- **Q1 (Si7116DN)**: Thermal pad connected to a large copper pour on Top and Bottom layers via thermal vias.
- **STM32H7**: Center ground pad (if applicable) connected to the internal ground plane.

## Isolation Barriers
While not fully galvanically isolated, the target interface is separated from the main logic by `U7 (Level Shifter)` and `U4 (Gate Driver)`.
- **Target Ground**: Optional star-point connection to System Ground to reduce noise injection back into the MCU.

## Board Outline
- **Dimensions**: 60mm x 40mm.
- **Mounting**: 4x M2.5 mounting holes in corners.
- **Connectors**:
  - USB-C on the left edge.
  - 10-pin 2.54mm header for Target Interface (VCC, GND, CLK, TRIG_0, TRIG_1, etc.) on the right edge.
  - SWD/JTAG debug headers on the bottom edge (SMD pads).
