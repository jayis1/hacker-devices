# Phase 4: Software Stack - Volt-Glitcher

## Boot Strategy
1. **STM32H7 Bootloader**: Factory USB DFU bootloader for initial flashing.
2. **Custom Bootloader**: Secure bootloader with firmware signature verification (using STM32 hardware crypto).
3. **FPGA Bitstream**: The MCU stores the iCE40 bitstream in its internal flash and loads it into the FPGA via SPI during the boot sequence.

## Clock & GPIO Init
- **MCU**: 480MHz (Cortex-M7), 240MHz (AXI bus).
- **FPGA**: 100MHz internal oscillator (calibrated by MCU).
- **GPIO**:
  - `PE0`: Input, EXTI (Trigger from FPGA).
  - `PC0`: Output, Push-Pull (Manual Glitch).
  - `PA0-PA15`: FMC (Parallel data to FPGA).

## Device Drivers
- **FPGA Manager**: Handles iCE40 configuration and register access via SPI.
- **Glitch Controller**: High-level driver to set pulse width, delay, and trigger conditions. Uses DMA to stream power trace data from the FPGA/ADC.
- **USB Stack**: CDC-ACM for CLI control, Custom Vendor class for high-speed data transfer (power traces).
- **BLE Stack**: Nordic SoftDevice (if using NRF52) or STM32 BLE stack to provide GATT services for the mobile app.

## MMIO Registers (FPGA-side)
| Offset | Name | Description |
|--------|------|-------------|
| 0x00 | `CTRL` | Bit 0: Enable, Bit 1: Reset, Bit 2: Arm Trigger |
| 0x04 | `GLITCH_WIDTH` | Pulse width in clock cycles (1ns resolution) |
| 0x08 | `GLITCH_DELAY` | Delay from trigger to pulse (1ns resolution) |
| 0x0C | `TRIG_VAL` | 16-bit pattern to match on trigger inputs |
| 0x10 | `TRIG_MASK` | Mask for pattern matching |
| 0x14 | `STATUS` | Bit 0: Triggered, Bit 1: Glitch Complete |

## Build Instructions
1. **Firmware**:
   ```bash
   cd firmware/
   make
   # Flash via OpenOCD
   openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program build/volt-glitcher.elf verify reset exit"
   ```
2. **FPGA**:
   ```bash
   cd fpga/
   yosys -p "synth_ice40 -top top -json top.json" top.v
   nextpnr-ice40 --up5k --package sg48 --json top.json --pcf top.pcf --asc top.asc
   icepack top.asc top.bin
   ```
3. **App**:
   ```bash
   cd app/
   npm install
   npx react-native run-android
   ```
