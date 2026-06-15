# Phase 4: Software Stack — VoltGlitcher Pro

## Complete Firmware, FPGA, and Companion App Architecture

**Document Version:** 1.0  
**Date:** 2026-01-15  
**Author:** Hacker Devices  
**Status:** Final

---

## 1. Software Architecture Overview

The VoltGlitcher Pro software stack consists of three major components:

1. **MCU Firmware** (STM32F407VGT6 — C/bare-metal)
2. **FPGA Bitstream** (iCE40HX1K — Verilog/Yosys)
3. **Companion App** (React Native — JavaScript)

These communicate through well-defined interfaces:

```
┌───────────────────┐     USB      ┌──────────────────┐     SPI      ┌──────────────────┐
│   Companion App   │◄───────────►│   MCU Firmware   │◄───────────►│  FPGA Bitstream  │
│   (React Native)  │   EP1-EP3   │   (main.c +      │   SPI1       │   (Verilog)      │
│                   │             │   drivers/)      │              │                  │
│  • Parameter UI   │             │  • USB protocol  │              │  • Delay chains  │
│  • Live monitor   │             │  • Safety mon    │              │  • Pattern match  │
│  • Statistics     │             │  • ADC sampling  │              │  • Waveform gen   │
│  • Profile mgmt   │             │  • EEPROM I/O    │              │  • PLL control    │
│  • Data export    │             │  • State machine │              │  • JTAG tracker   │
└───────────────────┘             └──────────────────┘              └──────────────────┘
```

---

## 2. MCU Firmware Architecture

### 2.1 File Structure

```
firmware/
├── Makefile              # Build system (arm-none-eabi-gcc)
├── main.c                # Main application, ISRs, USB command handler
├── board.h               # Pin definitions, peripheral mappings, board API
├── registers.h           # STM32F407 register definitions, FPGA register map
├── usb_descriptors.h     # USB device/config/string descriptors, protocol structs
├── stm32f407vg.ld        # Linker script
└── drivers/
    ├── fpga.h            # FPGA driver interface
    ├── fpga.c            # FPGA config, register access, waveform upload
    ├── glitch.h          # Glitch engine interface
    └── glitch.c          # Glitch logic, profile storage, calibration
```

### 2.2 Boot Sequence

```
1. Reset vector → SystemInit (CMSIS)
2. clock_init()
   - HSE on → PLL config (8MHz × 336/8/2 = 168 MHz)
   - Flash wait states (5 WS)
   - AHB/APB1/APB2 prescalers
   - Peripheral clock enables
3. gpio_init()
   - All pin mode/speed/pull configuration
   - Glitch outputs start LOW (safe state)
   - FPGA CRESET_B asserted (FPGA held in reset)
4. spi1_init() — FPGA communication bus
5. usart1_init() — Trigger UART
6. usart2_init() — Debug console
7. tim2_init() — Glitch pulse timer
8. tim3_init() — Trigger timestamp capture
9. adc1_init() — Analog monitoring
10. exti_init() — GPIO trigger interrupt
11. board_power_fpga_on()
    - Power sequence: 1.2V → 2.5V → 1.8V → 3.3V
    - Wait for power-good on each rail
12. fpga_configure()
    - Release CRESET_B
    - Stream bitstream from W25Q128 flash
    - Verify CDONE goes high
    - Test register read (VERSION register)
13. glitch_init() — Set default configuration
14. glitch_load_profile(0) — Load startup profile from EEPROM
15. Enter main loop
```

### 2.3 Interrupt Priority Scheme

| Priority | IRQ | Purpose | Max Latency |
|----------|-----|---------|-------------|
| 0 (highest) | SysTick | 1ms system tick | Deterministic |
| 1 | TIM2 | Glitch pulse complete | <6 ns (ISR entry) |
| 2 | EXTI0 | GPIO trigger event | <12 ns |
| 3 | ADC | Analog watchdog (overcurrent) | <12 ns |
| 4 | TIM3 | Trigger timestamp capture | <24 ns |
| 5 | USART1 | UART trigger pattern | <24 ns |
| 6 | USB OTG FS | USB data transfer | <120 ns |
| 7 (lowest) | I2C1 | EEPROM access | Best-effort |

**Rationale:**
- Glitch timing (TIM2) must have the lowest latency to de-assert outputs precisely
- Safety cutoff (ADC watchdog) must be nearly as fast
- USB transfers are buffered and can tolerate moderate latency
- EEPROM operations are never time-critical

### 2.4 Main Loop Structure

```c
while (1) {
    /* 1. Process USB commands (EP1 OUT polling or IRQ-driven) */
    if (usb_ep1_data_available()) {
        process_usb_command(g_ep1_out_buf);
    }

    /* 2. Periodic safety monitoring (every 100ms) */
    if (g_tick_ms % 100 == 0) {
        check_fpga_status();
        check_power_rails();
        check_temperature();
    }

    /* 3. Manual trigger button (PC13) */
    if (manual_trigger_pressed()) {
        debounce_and_fire();
    }

    /* 4. Wait for next interrupt (WFI saves power) */
    __WFI();
}
```

### 2.5 USB Command Processing

The USB command handler processes 64-byte packets received on EP1 OUT. Each command is dispatched to the appropriate handler:

```
USB_CMD_GET_STATUS     → Read global state flags
USB_CMD_SET_GLITCH_CFG → Parse 40-byte config, apply to glitch engine
USB_CMD_GET_GLITCH_CFG → Serialize current config, send response
USB_CMD_ARM            → Arm glitch engine, enable trigger detection
USB_CMD_DISARM         → Disable all outputs, clear armed state
USB_CMD_FIRE           → Manual glitch fire (no trigger delay)
USB_CMD_GET_RESULTS    → Return glitch count, VCC, current, faults
USB_CMD_SET_TRIGGER    → Configure trigger mode and edge
USB_CMD_FPGA_WRITE     → Write 16-bit value to FPGA register
USB_CMD_FPGA_READ      → Read 16-bit value from FPGA register
USB_CMD_FPGA_LOAD_BIT  → Trigger FPGA reconfiguration from flash
USB_CMD_EEPROM_READ    → Read bytes from EEPROM
USB_CMD_EEPROM_WRITE   → Write bytes to EEPROM
USB_CMD_ADC_READ       → Read ADC channel, return raw + mV
USB_CMD_CALIBRATE      → Run self-calibration routine
USB_CMD_SET_PROFILE    → Save config to EEPROM profile slot
USB_CMD_GET_PROFILE    → Load config from EEPROM profile slot
USB_CMD_WAVEFORM_LOAD  → Upload waveform samples to FPGA BRAM
USB_CMD_WAVEFORM_START → Begin waveform playback
USB_CMD_WAVEFORM_STOP  → Stop waveform playback
USB_CMD_GET_TIMESTAMP  → Read last trigger timestamp
USB_CMD_RESET          → Emergency reset all systems
```

### 2.6 Glitch Engine State Machine

```
                    ┌─────────┐
                    │  IDLE   │
                    └────┬────┘
                         │ ARM command
                         ▼
                    ┌─────────┐
            ┌───────│  ARMED  │◄───────┐
            │       └────┬────┘        │
            │            │ Trigger     │ auto_arm
            │            │ detected   │ = true
            │            ▼            │
            │       ┌──────────┐      │
            │       │ DELAYING │      │
            │       │ (FPGA)   │      │
            │       └────┬─────┘      │
            │            │ Delay expired
            │            ▼            │
            │       ┌──────────┐      │
            │       │ FIRING   │──────┘
            │       │ (output  │
            │       │  active) │
            │       └────┬─────┘
            │            │ Pulse complete
            │            ▼
            │       ┌──────────┐
            │       │ COOLDOWN │
            └──────►│ (safety) │
       DISARM       └──────────┘
```

### 2.7 Safety Monitoring

The firmware implements multiple safety layers:

**Hardware safety (independent of firmware):**
1. ADC analog watchdog: Fires interrupt when target VCC drops below threshold. The ISR de-asserts glitch outputs within ~200 ns.
2. Gate driver fault: The UCC27511 has a hardware fault output that disables the driver when desaturation is detected.
3. FPGA emergency: If the MCU sends FPGA_CTRL_RESET, the FPGA de-asserts all outputs.

**Software safety (in main loop):**
1. Power-good monitoring: Every 100ms, check PB8 (1.2V PG) and PB9 (3.3V PG). If either goes low, emergency cutoff.
2. FPGA status polling: Read FPGA_REG_STATUS every 100ms. Check FAULT and OVERTEMP bits.
3. Current monitoring: ADC reads shunt current every main loop iteration. If above max_current_ma, disarm.
4. Temperature monitoring: Read FPGA temperature. If >85°C, reduce glitch rate. If >100°C, emergency cutoff.

---

## 3. FPGA Bitstream Architecture

### 3.1 Verilog Module Hierarchy

```
top.v
├── spi_register_if.v        # SPI command parser + register file
├── glitch_controller.v      # Glitch output MUX and timing
├── delay_chain.v            # Carry-chain based sub-ns delay
├── uart_pattern_match.v      # 4-byte UART pattern comparator
├── jtag_tap_tracker.v       # IEEE 1149.1 TAP state machine
├── trigger_mux.v            # Trigger source selection and arming
├── waveform_generator.v     # BRAM-based arbitrary waveform
├── pll_controller.v         # PLL reconfiguration interface
├── temperature_sensor.v     # Ring-oscillator temperature sensor
└── fan_pwm.v                # PWM output for cooling fan
```

### 3.2 SPI Register Interface

The FPGA implements a simple register-based interface accessed via SPI1. The MCU writes configuration registers and reads status registers through this interface.

**Protocol:**
```
Write: [CMD=0x00|ADDR][DATA_HI][DATA_LO]
Read:  [CMD=0x80|ADDR][DUMMY][DUMMY] → [DUMMY][DATA_HI][DATA_LO]
```

**Register map (30 registers × 16 bits = 60 bytes):**

| Address | Name | R/W | Default | Description |
|---------|------|-----|---------|-------------|
| 0x00 | VERSION | R | 0x0100 | Bitstream version (BCD: 1.0) |
| 0x01 | STATUS | R | 0x0001 | Status flags (CONFIGURED, PLL_LOCKED, etc.) |
| 0x02 | CTRL | R/W | 0x0000 | Control bits (ENABLE, ARM, FIRE_NOW, RESET) |
| 0x03 | TRIG_MODE | R/W | 0x0000 | Trigger mode select |
| 0x04 | TRIG_DELAY_LO | R/W | 0x0000 | Delay [15:0] in 100ps units |
| 0x05 | TRIG_DELAY_HI | R/W | 0x0000 | Delay [23:16] |
| 0x06 | GLITCH_WIDTH_LO | R/W | 0x03E8 | Width [15:0] in 100ps units (default: 100ns) |
| 0x07 | GLITCH_WIDTH_HI | R/W | 0x0000 | Width [23:16] |
| 0x08 | GLITCH_SHAPE | R/W | 0x0000 | Pulse shape (0=rect, 1=tri, 2=gauss, 3=saw, 4=custom) |
| 0x09 | GLITCH_MODE | R/W | 0x0001 | Glitch mode (1=VCC, 2=EM, 3=CLK) |
| 0x0A | PLL_CTRL | R/W | 0x0001 | PLL divisor (integer part) |
| 0x0B | PLL_FRAC_LO | R/W | 0x0000 | PLL fractional divisor [15:0] |
| 0x0C | PLL_FRAC_HI | R/W | 0x0000 | PLL fractional divisor [23:16] |
| 0x0D-0x10 | UART_PATTERN0-3 | R/W | 0x0000 | UART trigger pattern bytes |
| 0x11 | UART_MASK | R/W | 0xFFFF | UART pattern match mask |
| 0x12 | JTAG_STATE | R/W | 0x0000 | JTAG TAP state trigger value |
| 0x13 | GPIO_TRIG_CFG | R/W | 0x0001 | GPIO trigger edge config |
| 0x14 | REPEAT_COUNT | R/W | 0x0001 | Glitch burst count |
| 0x15 | REPEAT_DELAY | R/W | 0x0000 | Inter-glitch delay (FPGA clocks) |
| 0x16 | PHASE_OFFSET | R/W | 0x0000 | Clock glitch phase offset |
| 0x17 | DAC_VAL | R/W | 0x0200 | EM pulse DAC value (0-1023) |
| 0x18 | TIMESTAMP_LO | R | 0x0000 | Last trigger timestamp [15:0] |
| 0x19 | TIMESTAMP_HI | R | 0x0000 | Last trigger timestamp [31:16] |
| 0x1A | GLITCH_COUNT_LO | R | 0x0000 | Glitch fire counter [15:0] |
| 0x1B | GLITCH_COUNT_HI | R | 0x0000 | Glitch fire counter [31:16] |
| 0x1C | WAVEFORM_ADDR | R/W | 0x0000 | Waveform RAM write address |
| 0x1D | WAVEFORM_DATA | R/W | 0x0000 | Waveform RAM write data |
| 0x1E | WAVEFORM_CTRL | R/W | 0x0000 | Waveform playback control |
| 0x1F | FAN_CTRL | R/W | 0x0000 | Fan PWM value (0-255) |

### 3.3 Delay Chain Architecture

The FPGA uses carry-chain elements to implement sub-nanosecond delay lines. Each iCE40 LC contains a fast carry chain with ~50 ps per tap.

```
trigger_in ──► [Carry Chain: 256 taps × 50ps = 12.8ns range]
                    │
                    ├── MUX (selected by TRIG_DELAY register)
                    │
                    └──► glitch_output
```

**Implementation details:**
- 2 independent delay chains (one for primary glitch, one for repeat offset)
- Each chain: 256 taps, 50 ps resolution, 12.8 ns range
- For longer delays: integer clock cycle delay + fractional carry chain
- Total delay = (integer_cycles × Tclk) + (taps × 50ps)
- Jitter: <25 ps (single carry chain tap)

### 3.4 UART Pattern Matcher

```
UART_RX ──► [Shift Register: 4 bytes] ──► [Comparator] ──► trigger_out
                         │                      ▲
                         │                      │
                         └──────────────────────┘
                           (compared against PATTERN registers
                            with per-byte MASK enable)
```

**Operation:**
1. Each incoming UART byte is shifted into a 32-bit register
2. The comparator checks each byte position against the pattern bytes
3. A byte position is only compared if the corresponding mask bit is set
4. When all active bytes match, trigger_out is asserted for one FPGA clock cycle
5. Pattern can be 1-4 bytes long (unused bytes set mask=0)

**Example: Match "AUTH" in UART stream**
- PATTERN0 = 0x41 ('A'), PATTERN1 = 0x55 ('U'), PATTERN2 = 0x54 ('T'), PATTERN3 = 0x48 ('H')
- MASK = 0xFF (all bytes compared)
- When "AUTH" appears in UART stream → trigger fires

### 3.5 JTAG TAP State Tracker

The FPGA implements the IEEE 1149.1 TAP state machine in hardware:

```
TCK ──► [State Machine: 16 states] ──► current_state
TMS ──►        │
               └──► comparator(current_state == JTAG_STATE register) ──► trigger_out
```

**Supported trigger states:**
- Test-Logic-Reset (0x00)
- Run-Test/Idle (0x01)
- Select-DR-Scan (0x02)
- Capture-DR (0x03)
- Shift-DR (0x04)
- Exit1-DR (0x05)
- Pause-DR (0x06)
- Exit2-DR (0x07)
- Update-DR (0x08)
- Select-IR-Scan (0x09)
- Capture-IR (0x0A)
- Shift-IR (0x0B)
- Exit1-IR (0x0C)
- Pause-IR (0x0D)
- Exit2-IR (0x0E)
- Update-IR (0x0F)

### 3.6 Waveform Generator

The waveform generator plays arbitrary amplitude profiles from BRAM:

```
trigger ──► [Address Counter: 10-bit] ──► [BRAM: 1024×16] ──► [DAC interface]
                  │                            ▲
                  │                            │
                  └── [Loop/One-shot control] ◄── WAVEFORM_CTRL register
```

**Features:**
- 1024 samples per waveform (fits in 2 KB BRAM)
- 16-bit amplitude resolution per sample
- Playback speed: controlled by FPGA PLL (1 sample per PLL clock)
- Loop mode: wraps around to sample 0 after sample 1023
- Trigger mode: waits for trigger event before starting playback
- Supports Gaussian, triangle, sawtooth, and custom shapes

---

## 4. Companion App Architecture

### 4.1 Technology Stack

| Component | Technology | Version |
|-----------|-----------|---------|
| Framework | React Native | 0.73.4 |
| Navigation | React Navigation 6 | 6.1.9 |
| USB | WebUSB / react-native-usb-serial | 2.0.3 |
| BLE | react-native-ble-plx | 3.1.1 |
| Charts | Custom (no dependencies) | — |
| State | React Context + hooks | — |
| Platform | iOS + Android | — |

### 4.2 Screen Structure

```
App.js (Navigation Container)
├── Tab: Control (ControlScreen.js)
│   ├── Connection status
│   ├── ARM/DISARM/FIRE buttons
│   ├── Status dashboard (VCC, current, temp, faults)
│   ├── Glitch counters (total, triggers, successes)
│   └── Event log
│
├── Tab: Config (GlitchConfig.js)
│   ├── Mode selector (Voltage/EM/Clock)
│   ├── Pulse shape selector
│   ├── Timing parameters (delay, width, repeat)
│   ├── Mode-specific settings
│   ├── Trigger configuration
│   ├── UART pattern editor
│   └── Safety options
│
└── Tab: Analysis (AnalysisScreen.js)
    ├── Success rate tracker
    ├── Glitch timeline chart
    ├── Voltage/current analysis
    ├── Fault status display
    ├── Mode summary
    └── Data export (JSON)
```

### 4.3 Protocol Implementation (protocol.js)

The protocol module implements the full binary USB command interface:

**Packet structure:**
```
[CMD][SUBCMD][P1_LE16][P2_LE16][P3_LE16][P4_LE16][CHECKSUM][PAYLOAD:53]
```

**Key classes/functions:**
- `VoltGlitcherProtocol`: Main class, manages USB connection and command queue
- `buildCommandPacket()`: Constructs 64-byte command packets with checksum
- `parseResponsePacket()`: Validates and parses device responses
- `parseEventPacket()`: Decodes EP3 event notifications
- `serializeGlitchConfig()` / `deserializeGlitchConfig()`: Config ↔ binary conversion
- `deserializeResults()`: Parse glitch results from device response

**Connection flow:**
1. App requests USB device with VID/PID filter
2. Opens device, claims interface 0
3. Starts polling intervals for results (500ms) and events (100ms)
4. Loads current device config and status
5. User interactions trigger command packets via the protocol class

### 4.4 Data Flow

```
User taps "ARM"
    → App: actions.arm()
    → Protocol: _sendCommand(CMD.ARM)
    → Build 64-byte packet with CMD=0x04
    → USB transferOut(EP1)
    → MCU receives on EP1 OUT
    → process_usb_command()
    → glitch_arm(&g_glitch_cfg)
    → FPGA: write CTRL register (ARM bit)
    → FPGA enables trigger detection
    → USB transferIn(EP2) ← Response: STATUS.OK
    → App updates state to ARMED
    
Trigger event occurs
    → FPGA detects trigger
    → FPGA asserts GLITCH output after delay
    → MCU: TIM2 ISR fires on pulse complete
    → MCU: glitch_count++
    → MCU: EP3 event packet (GLITCH_FIRED)
    → App: eventPollingInterval reads EP3
    → App: onEvent callback
    → App: Vibration feedback
    → App: Update counter display
```

---

## 5. Build System

### 5.1 MCU Firmware Build

```bash
# Toolchain setup
sudo apt install gcc-arm-none-eabi openocd

# Build
cd firmware
make clean && make

# Flash via ST-Link
make flash-stlink

# Or via OpenOCD
make flash
```

**Output files:**
- `volt-glitcher.elf` — Debug symbols
- `volt-glitcher.bin` — Raw binary for ST-Link
- `volt-glitcher.hex` — Intel HEX for universal flashers
- `volt-glitcher.lst` — Disassembly listing
- `volt-glitcher.map` — Memory map

### 5.2 FPGA Bitstream Build

```bash
# Toolchain: Yosys + nextpnr-ice40
sudo apt install yosys nextpnr-ice40 icestorm

# Synthesize
cd fpga
make synth    # Yosys: Verilog → netlist
make pnr      # nextpnr: place & route
make pack     # IcePack: ASCII → binary bitstream
make flash    # IceProg: write to W25Q128 flash
```

### 5.3 Companion App Build

```bash
# Android
cd app
npm install
npx react-native run-android

# iOS
npx react-native run-ios

# Release build
cd android && ./gradlew assembleRelease
```

---

## 6. Testing Strategy

### 6.1 Firmware Unit Tests

| Test | Method | Coverage |
|------|--------|----------|
| Clock init | Check CFGR register values | 100% |
| GPIO init | Verify MODER/OSPEEDR for all pins | 100% |
| SPI transfer | Loopback (MOSI→MISO) | Byte-level |
| ADC read | Known voltage on PA0 | ±5% |
| I2C EEPROM | Write/read/verify pattern | Byte-level |
| Glitch timing | Oscilloscope on PC6 | ±10ns |
| Safety cutoff | Overcurrent injection | <500ns |
| USB protocol | Send each command, verify response | All commands |

### 6.2 Integration Tests

| Test | Setup | Pass Criteria |
|------|-------|---------------|
| FPGA configuration | Power cycle + config | CDONE high in <500ms |
| Glitch fire cycle | ARM → Trigger → Glitch | Pulse width within 10% |
| UART trigger | Send pattern on USART1 | Trigger detected within 1 byte |
| Profile save/load | Write profile, read back | Config matches |
| Auto-arm | Fire glitch, wait | Re-armed within 100ms |
| Emergency cutoff | Force overcurrent | Outputs off in <500ns |
| Waveform upload | Load 1024 samples | Playback matches uploaded data |

### 6.3 Companion App Tests

| Test | Method | Platform |
|------|--------|----------|
| Connect/disconnect | USB device selection | Android + Chrome |
| Config apply | Set parameters, verify device response | All |
| ARM/DISARM | Toggle armed state, verify LED | All |
| Event reception | Fire glitch, check event log | All |
| Data export | Export JSON, verify structure | All |
| Chart rendering | Generate 100+ events, render chart | All |

---

## 7. Calibration Procedure

### 7.1 Self-Calibration

The VoltGlitcher Pro includes a built-in calibration routine that:

1. **Measures baseline VCC**: 64 samples of target VCC with no glitch active, computes average
2. **Measures baseline current**: Same for shunt current
3. **Computes offsets**: VCC offset = measured - expected, current offset = measured - 0
4. **Calibrates FPGA delay chain**: The FPGA measures actual per-tap delay using a ring oscillator and stores correction factors
5. **Sets ADC watchdog thresholds**: Based on calibrated VCC nominal value

### 7.2 External Calibration

For higher accuracy, an external oscilloscope can be used:

1. Connect scope probe to glitch output connector (J3 pin 1)
2. Set delay = 0, width = 100ns, mode = voltage glitch
3. Fire manual glitch
4. Measure actual pulse width on scope
5. Compare with configured width → compute correction factor
6. Repeat for various widths (50ns, 100ns, 500ns, 1µs, 10µs)
7. Store correction table in EEPROM

---

## 8. Profile System

### 8.1 EEPROM Profile Format

Each profile is 64 bytes stored in the CAT24C256 EEPROM:

```
Offset  Size  Field
0x00    2     Magic (0x5647 = "VG")
0x02    1     Profile ID (0-15)
0x03    1     Name length (0-16)
0x04    16    Name (ASCII, null-padded)
0x14    40    Glitch config (usb_glitch_config_t packed)
0x3C    1     Checksum (XOR of bytes 0x00-0x3B)
0x3D    3     Reserved (0xFF)
```

### 8.2 Default Profiles

| Slot | Name | Mode | Delay | Width | Trigger |
|------|------|------|-------|-------|---------|
| 0 | Default VCC | Voltage | 0 ns | 100 ns | Manual |
| 1 | STM32F1 Boot | Voltage | 500 ns | 50 ns | GPIO |
| 2 | STM32F4 Debug | Voltage | 200 ns | 80 ns | UART "AUTH" |
| 3 | EM General | EM | 0 ns | 1 µs | Manual |
| 4 | Clock Skip | Clock | 0 ps | 10 ns | FPGA |

---

## 9. Common Attack Recipes

### 9.1 STM32F1 Secure Boot Bypass

**Target:** STM32F103C8T6 (Blue Pill)  
**Attack:** Glitch during read-option-bytes comparison  
**Configuration:**
- Mode: Voltage glitch
- Trigger: GPIO on BOOT0 pin (rising edge = bootloader entry)
- Delay: 450 ns after trigger (tuned empirically)
- Width: 40 ns (just enough to skip one instruction)
- Repeat: 1
- Profile: Slot 1

**Procedure:**
1. Connect glitch output to target VCC (between decoupling cap and die)
2. Connect trigger input to BOOT0 pin
3. Apply config and arm
4. Reset target (BOOT0 high → bootloader starts)
5. Glitch fires during option bytes comparison
6. If successful: debug access unlocked (mark success!)
7. If not: adjust delay ±50 ns and retry

### 9.2 AES Fault Attack (DFA)

**Target:** Any MCU running AES-128 in software  
**Attack:** Inject fault during 8th round MixColumns  
**Configuration:**
- Mode: Voltage glitch
- Trigger: UART pattern (target prints "ENCRYPT" before AES)
- Delay: 2000 ns (2 µs after trigger, tuned for target clock)
- Width: 80 ns
- Repeat: 1
- Profile: Slot 2

**Analysis:**
1. Collect pairs of correct/faulty ciphertexts
2. Apply DFA differential equations (Piret & Quisquater 2003)
3. Each faulty ciphertext reveals 4 bytes of round key
4. After 4-8 successful faults, full key recovered

### 9.3 EM Glitch for BGA Package

**Target:** QFP/BGA MCU with internal voltage regulator  
**Attack:** Non-invasive EM pulse during authentication  
**Configuration:**
- Mode: EM glitch
- Trigger: Manual (press button when authentication starts)
- Delay: 0 ns
- Width: 2 µs
- EM amplitude: 768 (75%)
- Repeat: 3 (burst of 3 pulses, 1µs apart)
- Profile: Slot 3

**Procedure:**
1. Wind 15-turn coil (3mm diameter, 0.5mm wire)
2. Place coil directly over target die (use acetone to remove package if needed)
3. Start target authentication sequence
4. Press manual trigger when target enters crypto operation
5. Observe result: if authentication bypassed, mark success
6. Adjust coil position and amplitude

---

## 10. Security & Responsible Disclosure

The VoltGlitcher Pro is a **security research tool**. Users must:

1. **Only test devices they own or have explicit authorization to test**
2. **Follow responsible disclosure** when finding vulnerabilities
3. **Comply with local laws** regarding security testing equipment
4. **Never use for unauthorized access** to systems or data

The tool includes no attack-specific payloads. All glitch parameters must be configured by the user based on their understanding of the target. The device is a precision timing instrument — what it's used for is the responsibility of the operator.

---

## 11. Debugging & Troubleshooting

### 11.1 Common Issues

**FPGA won't configure (CDONE stays low):**
- Check 1.2V and 3.3V power rails with multimeter
- Verify CRESET_B goes high after power-up
- Check SPI1 connections (SCK, MOSI, NSS)
- Try fpga_configure_from_buf() with a known-good bitstream via USB
- Check W25Q128 flash has valid bitstream at offset 0

**Glitch output not firing:**
- Verify device is ARMED (LED2 yellow)
- Check trigger source is configured correctly
- Verify FPGA STATUS register has TRIGGER_ARM bit set
- Check gate driver enable (PC7) is being asserted
- Use oscilloscope on Q1 gate (PC6) to verify pulse

**ADC readings wrong:**
- Run calibration first (USB_CMD_CALIBRATE)
- Check voltage divider on PA0 (10k/10k)
- Verify VDDA supply is clean (LC filter)
- Check ADC sample time (56 cycles may be too short for high-Z sources)

**USB not recognized:**
- Check USB cable (some cables are power-only)
- Verify USB descriptors are correct (VID=0x1209, PID=0xAC11)
- Check PA11/PA12 alternate function configuration
- Verify VBUS sensing is working

**Overcurrent faults on every glitch:**
- Target VCC may have very low impedance
- Reduce pulse width (start at 10ns, increase gradually)
- Check max_current_ma setting in config
- Verify INA210 sense amplifier offset calibration

### 11.2 Debug Console

The firmware outputs debug messages on USART2 (PA2/PA3) at 115200 baud. Connect a USB-TTL adapter to see:
- Boot messages ("VoltGlitcher v1.0 booting...")
- FPGA configuration status
- Trigger events
- Fault conditions
- Profile load/save confirmations

### 11.3 FPGA Register Debug

Use the companion app or direct USB commands to read FPGA registers:
```
CMD=0x0B (FPGA_READ), SUBCMD=register_address
```

Key registers to check:
- 0x00 (VERSION): Should read 0x0100
- 0x01 (STATUS): Check PLL_LOCKED, CONFIGURED bits
- 0x1A-0x1B (GLITCH_COUNT): Should increment after each fire
- 0x18-0x19 (TIMESTAMP): Captures last trigger time

### 11.4 Oscilloscope Verification

For precise glitch timing verification:
1. Connect scope CH1 to J3 pin 1 (VCC_TARGET)
2. Connect scope CH2 to PC6 (Q1 gate drive)
3. Set trigger on CH2 rising edge
4. Fire manual glitch
5. Verify:
   - Gate drive rise time < 20ns
   - VCC dip depth and duration match config
   - No ringing or oscillation on VCC

### 11.5 Timing Budget Analysis

**End-to-end glitch latency breakdown:**

| Path | Stage | Latency |
|------|-------|---------|
| Trigger → FPGA | Input pin → Pattern match | 5-15 ns |
| FPGA → Delay | Delay chain traversal | Configured delay (0-12.8ns) |
| FPGA → Output | Output buffer | 3-5 ns |
| Gate driver | UCC27511 propagation | 15-30 ns |
| MOSFET | SiS426DN turn-on | 5-10 ns |
| **Total (FPGA path)** | | **28-60 ns + configured delay** |

| Path | Stage | Latency |
|------|-------|---------|
| Trigger → MCU | EXTI interrupt entry | 12 cycles (71 ns) |
| ISR → GPIO | Software toggle | 2-4 cycles (12-24 ns) |
| Gate driver | UCC27511 propagation | 15-30 ns |
| MOSFET | SiS426DN turn-on | 5-10 ns |
| **Total (MCU path)** | | **98-135 ns** |

The FPGA path is ~2× faster than the MCU path, which is why the FPGA handles all precision timing. The MCU path serves as a backup and for non-time-critical operations.

---

## 12. Advanced Techniques

### 12.1 Parameter Sweeping

For systematic exploration of glitch parameter space, the companion app supports automated parameter sweeps. The protocol implements this through repeated ARM → fire → measure → adjust cycles.

**Sweep dimensions:**
- Delay: Sweep from 0 to max_delay in configurable steps (e.g., 10ns)
- Width: Sweep from min_width to max_width (e.g., 5ns steps)
- Both can be swept independently or as a 2D grid

**Automation flow:**
```
for delay in range(0, 10us, 10ns):
    for width in range(10ns, 500ns, 5ns):
        config.delay_ns = delay
        config.width_ns = width
        send(SET_GLITCH_CFG, config)
        send(ARM)
        wait_for_glitch_or_timeout(1s)
        send(DISARM)
        if target_did_something_interesting:
            mark_success()
        record_result(delay, width, outcome)
```

### 12.2 Multi-Glitch Bursts

The repeat_count and repeat_delay_ns parameters enable burst-mode glitching:
- Multiple glitches in rapid succession
- Can target different pipeline stages
- Effective against targets with redundant checks
- Inter-glitch delay configurable down to 100ns

### 12.3 Clock Synchronization

For clock glitch mode, the FPGA must synchronize to the target's clock:
1. Target clock is fed into FPGA_CLKIN pin
2. FPGA PLL locks to target clock
3. PLL multiplies to higher frequency for phase resolution
4. Phase offset register controls which edge to glitch
5. Achieves <100ps phase resolution

### 12.4 Combining Triggers

The FPGA supports composite trigger logic:
- AND: Two conditions must be met simultaneously
- OR: Either condition triggers
- SEQUENCE: Condition A must occur before B within a timeout
- Example: UART pattern "AUTH" AND GPIO debug-pin-high

---

## 13. Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-01-15 | Initial release: all three glitch modes, four trigger types, companion app |

---

## 14. References

1. Piret, G. & Quisquater, J.-J. (2003). "A Differential Fault Attack Technique against SPN Structures." CHES 2003.
2. Dehbaoui, A. et al. (2013). "Voltage Glitch Attacks on Secure Elements." eSmart 2013.
3. Korak, T. et al. (2014). "Glitch Attacks on Cryptographic Circuits." DSD 2014.
4. Lattice Semiconductor. "iCE40 Programming and Configuration Guide." FPGA-TN-02001.
5. STMicroelectronics. "STM32F407 Reference Manual." RM0090.
6. Texas Instruments. "UCC27511 Low-Side Gate Driver." SLUS813.
7. IEEE. "1149.1 Standard Test Access Port." IEEE Std 1149.1-2001.
8. ChipWhisperer Project. https://github.com/newaetech/chipwhisperer
9. Glitch Project. https://github.com/Glitch-Works/Glitch
10. Fault Injection Testing Framework. NIST SP 800-171A.