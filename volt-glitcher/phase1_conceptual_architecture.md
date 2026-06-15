# Phase 1: Conceptual Architecture — VoltGlitcher Pro

## High-Precision Voltage Glitch Injection Tool

**Document Version:** 1.0  
**Date:** 2026-01-15  
**Author:** Hacker Devices  
**Status:** Final

---

## 1. Executive Summary

The VoltGlitcher Pro is a high-precision fault injection tool designed for hardware security researchers. It generates precisely-timed voltage transients (glitches) on target MCU/SoC power rails to induce computational errors that can bypass security mechanisms such as secure boot, debug lock, read-out protection, and encryption key extraction.

Unlike simple Arduino-based glitchers that offer microsecond-level timing, the VoltGlitcher Pro achieves sub-nanosecond timing precision through an FPGA co-processor architecture, enabling attacks on modern high-speed targets that were previously infeasible.

### Key Differentiators

| Feature | Arduino Glitcher | ChipWhisperer | **VoltGlitcher Pro** |
|---------|------------------|-------------|---------------------|
| Timing precision | ~1 µs | ~1 ns | **<100 ps** |
| Glitch modes | Voltage only | V + Clock | **V + EM + Clock** |
| Trigger inputs | GPIO only | GPIO + UART | **GPIO + UART + JTAG + Manual** |
| FPGA co-processing | No | Yes | **Yes (iCE40HX1K)** |
| Companion app | None | Python | **React Native (mobile+desktop)** |
| Profile storage | None | File-based | **EEPROM (16 profiles)** |
| Open source | Partial | Partial | **Fully open (MIT)** |

---

## 2. System Architecture Overview

```
┌──────────────────────────────────────────────────────────────────────┐
│                        VoltGlitcher Pro System                       │
│                                                                      │
│  ┌─────────────────┐    SPI1    ┌────────────────────────────────┐   │
│  │   STM32F407VGT6  │◄─────────►│  Lattice iCE40HX1K-TQ144      │   │
│  │   (MCU Host)     │           │  (FPGA Timing Co-Processor)    │   │
│  │                  │           │                                │   │
│  │  • USB interface │           │  • Sub-ns delay chains         │   │
│  │  • ADC monitoring│           │  • Pattern match engine        │   │
│  │  • I2C EEPROM    │           │  • PLL reconfiguration         │   │
│  │  • Debug UART    │           │  • Waveform generator          │   │
│  │  • Safety cutoff │           │  • JTAG TAP state tracker     │   │
│  └──────┬───────────┘           └────────┬───────────────────────┘   │
│         │                                │                           │
│  ┌──────▼──────────┐            ┌────────▼──────────────────────┐   │
│  │  USB Micro-B    │            │  Glitch Output Stage          │   │
│  │  (OTG FS)       │            │                               │   │
│  │                  │            │  ┌─── Q1: SiS426DN (VCC shunt)│   │
│  │  EP1 OUT: Cmds  │            │  ├─── Q2: IRLML6344 (EM coil)│   │
│  │  EP2 IN:  Resp  │            │  └─── Q3: BSS84P (clk glitch)│   │
│  │  EP3 IN:  Events│            │                               │   │
│  └─────────────────┘            │  Gate Driver: UCC27511        │   │
│                                  │  Current Sense: INA210       │   │
│  ┌──────────────────┐           └───────────────────────────────┘   │
│  │  Trigger Inputs  │                                               │
│  │                  │                                               │
│  │  TRIG0: GPIO (opto-isolated TLP291)                             │
│  │  TRIG1: UART pattern match                                       │
│  │  TRIG2: JTAG TAP state                                           │
│  │  TRIG3: Manual button                                            │
│  └──────────────────┘                                               │
│                                                                      │
│  ┌──────────────────┐           ┌──────────────────────┐            │
│  │  Power Supplies  │           │  Storage & Config   │            │
│  │                  │           │                      │            │
│  │  U2: TLV62569 → 1.2V       │  U9: CAT24C256 EEPROM │            │
│  │  U3: TLV62569 → 3.3V       │  SD Card (waveforms)  │            │
│  │  U4: TPS7A7001 → 1.8V      │  W25Q128 Flash (FPGA) │            │
│  │  U5: TPS7A7001 → 2.5V      │                      │            │
│  └──────────────────┘           └──────────────────────┘            │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 3. Design Philosophy

### 3.1 Dual-Processor Architecture

The fundamental design decision is the **dual-processor architecture**: an MCU for control/USB/management and an FPGA for timing-critical operations.

**Why not MCU-only?**
- ARM Cortex-M4 at 168 MHz has a minimum instruction time of ~6 ns
- Interrupt latency on STM32F4: 12 cycles minimum = ~71 ns
- GPIO toggle speed: ~6 ns per transition, but software jitter is ±50 ns
- For glitching modern targets running at >100 MHz, we need <10 ns jitter

**Why not FPGA-only?**
- FPGAs lack built-in USB, ADC, and are harder to program
- Configuration management (profiles, storage) needs a file system
- User interface requires a high-level protocol handler
- Cost: MCU+FPGA is cheaper than a large FPGA with soft-core USB

**The split:**
| Task | Processor | Reason |
|------|-----------|--------|
| USB protocol | MCU | Needs USB OTG peripheral |
| ADC monitoring | MCU | Needs 12-bit SAR ADC |
| EEPROM/SD card | MCU | I2C/SPI peripherals |
| Safety cutoff | MCU | Must be independent of FPGA |
| Sub-ns delay chains | FPGA | Requires deterministic timing |
| Pattern matching | FPGA | Parallel comparison at wire speed |
| PLL control | FPGA | Internal PLL for glitch timing |
| Waveform generation | FPGA | BRAM-based arbitrary waveform |
| JTAG TAP tracking | FPGA | Real-time TAP state machine |

### 3.2 Safety-First Design

Glitching involves deliberately short-circuiting target power rails. This is inherently dangerous to both the target and the glitcher hardware. Safety features are not optional:

1. **Hardware overcurrent cutoff**: INA210 current sense amplifier feeds the MCU ADC, which has an analog watchdog that can trigger an interrupt and de-assert glitch outputs within ~200 ns of detecting overcurrent.

2. **Gate driver fault pin**: The UCC27511 gate driver has a built-in fault output that detects desaturation and overcurrent in Q1, providing a hardware-level cutoff independent of MCU software.

3. **Power sequencing**: FPGA power rails must come up in the correct order (1.2V core → 2.5V PLL → 1.8V aux → 3.3V I/O) with proper delays and power-good verification.

4. **Software interlock**: Glitch outputs cannot be asserted unless the system is explicitly armed. The arm state requires FPGA confirmation and ADC watchdog configuration.

5. **Temperature monitoring**: The FPGA reports die temperature, and the fan is automatically controlled to prevent thermal damage during extended glitching sessions.

---

## 4. Glitch Mode Architecture

### 4.1 Voltage Glitch (VCC Shunt)

The primary glitch mode uses a low-Rds(on) N-channel MOSFET (SiS426DN, Rds(on) = 6 mΩ) to momentarily short the target's VCC rail to ground. This causes a brief voltage dip that can cause the target MCU to skip instructions, corrupt register values, or fail cryptographic operations.

**Circuit topology:**
```
VCC_TARGET ───┬──── R_shunt(0.1mΩ) ──── VCC_TARGET_MCU
              │
              ├──── Q1 (SiS426DN N-ch MOSFET)
              │     Gate ← U6 (UCC27511 gate driver) ← FPGA/MCU
              │
             GND (when Q1 ON)
```

**Timing characteristics:**
- Gate driver rise time: 15 ns (UCC27511 into SiS426DN gate)
- MOSFET turn-on: ~5 ns (with 4A gate drive)
- Total glitch assert latency: ~20 ns from gate signal
- Voltage dip depth: Configurable via pulse width (wider = deeper dip)
- Recovery: VCC returns to nominal in ~100-500 ns after glitch ends

**Attack targets:**
- Secure boot bypass (glitch during hash comparison)
- Debug lock bypass (glitch during lock-bit read)
- Fault-based key extraction (DFA on AES/RSA)
- BootROM exploitation (glitch during signature check)

### 4.2 EM Glitch (Electromagnetic Pulse)

An alternative to direct VCC shunting, EM glitching uses a coil placed on top of the target IC to inject transient electromagnetic fields that induce localized current in the die. This is useful when:
- Physical access to target VCC is not available (BGA packages)
- The target has internal voltage regulators
- Non-invasive attack is preferred

**Circuit topology:**
```
VCC_5V ──── EM_COIL ──── Q2 (IRLML6344 N-ch MOSFET) ──── GND
                              Gate ← FPGA (PWM + DAC amplitude)
```

**Coil specifications:**
- 10-20 turns of 0.5mm enameled copper wire
- Coil diameter: 3-5mm (target-dependent)
- Positioned directly over target IC die
- Pulse current: 0.5-2A peak (DAC-controlled)

**Attack characteristics:**
- Non-invasive (no soldering required)
- Weaker effect than VCC shunt (requires more iterations)
- Position-sensitive (coil placement is critical)
- Can target specific die areas with focused coil

### 4.3 Clock Glitch

Clock glitching stretches or inserts clock edges to cause the target to miss instructions or execute them out of order. This is particularly effective against:
- Pipelined processors (ARM Cortex-M series)
- Instruction skip attacks
- Loop counter manipulation

**Circuit topology:**
```
TARGET_CLK_IN ────┬──── Q3 (BSS84P P-ch MOSFET) ──── TARGET_CLK_OUT
                   │    Gate ← FPGA (phase-synced)
                   │
                  GND (when Q3 ON, clock stretched low)
```

**Timing requirements:**
- Must synchronize to target clock frequency
- FPGA PLL locks to target clock and generates glitch at precise phase
- Phase offset configurable in 100 ps increments
- Can target rising or falling edge independently

---

## 5. Trigger Architecture

### 5.1 Trigger Pipeline

```
Trigger Event → FPGA Pattern Match → Delay Chain → Glitch Output
                    ↑                      ↑
              (parallel)             (sub-ns precision)
                    
Trigger Event → MCU Interrupt → Software Delay → GPIO Glitch
                    ↑                      ↑
              (serial)              (µs precision, backup)
```

The dual trigger pipeline provides:
- **Primary path (FPGA)**: Sub-ns latency, used for all precision glitching
- **Secondary path (MCU)**: µs latency, used as backup and for non-time-critical operations

### 5.2 GPIO Trigger (TRIG0)

Opto-isolated digital input via TLP291. Configurable for rising, falling, or both edges. Used when the target provides a GPIO signal that indicates an interesting operation (e.g., a debug pin, status LED, or protocol handshake).

**Isolation rationale:** The glitcher and target may have different ground potentials. Optical isolation prevents ground loops that could damage either device or corrupt glitch timing.

### 5.3 UART Pattern Match (TRIG1)

The target's UART output is connected to both the FPGA (for hardware pattern matching) and the MCU's USART1 (for software matching). The FPGA implements a 4-byte shift register comparator that can detect patterns at full UART speed (up to 460800 baud). The MCU provides a software fallback and can also log the full UART stream.

**Pattern matching algorithm:**
1. Each incoming byte is shifted into a 4-byte register
2. The register is compared against the configured pattern bytes
3. Only bytes with their corresponding mask bit set are compared
4. When all active bytes match, a trigger event is generated

**Use case example:** Triggering on a specific debug message that precedes a cryptographic operation, such as `"AUTH"` or `"SIGN"`.

### 5.4 JTAG State Trigger (TRIG2)

The FPGA monitors the JTAG TCK, TMS, TDI, and TDO signals and tracks the target's TAP state machine in real-time. When the target enters a configured TAP state (e.g., Shift-DR, which is used during boundary scan and debug access), the FPGA fires a trigger.

**IEEE 1149.1 TAP states used for triggering:**
- Run-Test/Idle (normal operation)
- Select-DR-Scan (entering data register access)
- Shift-DR (shifting data through DR)
- Pause-DR (pausing during shift)
- Update-DR (latching shifted data)

**Use case:** Triggering during JTAG authentication to glitch the challenge-response verification.

### 5.5 Manual Trigger (TRIG3)

A physical button on the board (PC13) or a software FIRE command via USB. Used for manual experimentation and when no automated trigger source is available.

---

## 6. Power Supply Architecture

### 6.1 Power Tree

```
USB VBUS (5V)
    │
    ├── U2: TLV62569DBVR → VCC_CORE_1V2 (1.2V/3A) → FPGA core
    │   EN: PA1, PG: PB8
    │
    ├── U3: TLV62569DBVR → VCC_IO_3V3 (3.3V/3A) → FPGA I/O, MCU, peripherals
    │   EN: PA2, PG: PB9
    │
    ├── U4: TPS7A7001DDAR → VCC_AUX_1V8 (1.8V/2A) → FPGA aux bank
    │   EN: PA3
    │
    └── U5: TPS7A7001DDAR → VCC_PLL_2V5 (2.5V/2A) → FPGA PLL
        EN: PA4
```

### 6.2 Power Sequencing

The iCE40HX1K requires a specific power-up sequence:
1. VCC_CORE (1.2V) must be stable before other rails
2. VCC_PLL (2.5V) can come up next
3. VCC_AUX (1.8V) follows
4. VCC_IO (3.3V) comes up last
5. CRESET_B must be held low during power-up
6. CRESET_B released after all rails stable → FPGA begins configuration

Each step has a minimum delay (5-10 ms) and power-good verification before proceeding.

### 6.3 Decoupling Strategy

- **Bulk capacitance**: 10 µF ceramic per rail (0603, X5R, 6.3V)
- **Local decoupling**: 100 nF ceramic per power pin (0402, X5R)
- **High-frequency bypass**: 10 nF ceramic on FPGA core pins (0201, closest to pins)
- **Target VCC rail**: 100 µF electrolytic + 1 µF ceramic (to absorb glitch current)

---

## 7. USB Protocol Architecture

### 7.1 Endpoint Map

| Endpoint | Direction | Type | Size | Purpose |
|----------|-----------|------|------|---------|
| EP1 | OUT | Bulk | 64B | Commands (host → device) |
| EP2 | IN | Bulk | 64B | Responses (device → host) |
| EP3 | IN | Interrupt | 64B | Event notifications |

### 7.2 Command Format

```
Byte 0:     Command code (USB_CMD_xxx)
Byte 1:     Sub-command / mode
Bytes 2-3:  Parameter 1 (LE)
Bytes 4-5:  Parameter 2 (LE)
Bytes 6-7:  Parameter 3 (LE)
Bytes 8-9:  Parameter 4 (LE)
Byte 10:    XOR checksum (bytes 0-9)
Bytes 11-63: Payload (up to 53 bytes)
```

### 7.3 Event Notification Format

```
Byte 0:     Event type (TRIGGER_DETECTED, GLITCH_FIRED, FAULT, etc.)
Byte 1:     Event data (mode-specific)
Bytes 2-3:  Timestamp low (LE)
Bytes 4-5:  Timestamp high (LE)
Bytes 6-7:  Reserved
```

---

## 8. Firmware Architecture

### 8.1 Software Layers

```
┌─────────────────────────────────────┐
│  Application Layer (main.c)         │
│  • USB command processing           │
│  • State machine                    │
│  • Safety monitoring loop           │
├─────────────────────────────────────┤
│  Driver Layer (drivers/)            │
│  • fpga.c: FPGA config & register  │
│  • glitch.c: Glitch engine logic   │
├─────────────────────────────────────┤
│  HAL / Register Layer (registers.h)│
│  • Direct register access           │
│  • Interrupt handlers               │
├─────────────────────────────────────┤
│  Hardware (board.h)                 │
│  • Pin assignments                   │
│  • Peripheral mappings              │
└─────────────────────────────────────┘
```

### 8.2 State Machine

```
IDLE ──(ARM)──► ARMED ──(TRIGGER)──► FIRING ──(COMPLETE)──► IDLE
  ▲                │                     │                     │
  │                │                     │                     │
  │          (DISARM)              (EMERGENCY)           (AUTO_ARM)
  │                │                     │                     │
  └────────────────┴─────────────────────┴─────────────────────┘
                          │
                     (FAULT)
                          │
                          ▼
                       ERROR
```

---

## 9. Threat Model & Security Considerations

This is a security research tool. It is designed to help researchers find and fix vulnerabilities in their own hardware designs. The following security considerations apply:

1. **USB firmware can be modified**: The STM32F407 flash can be reprogrammed via SWD. The USB interface should not be the sole trust boundary.

2. **FPGA bitstream is stored in external flash**: The W25Q128 flash can be read and cloned. Proprietary bitstreams should be encrypted if IP protection is needed.

3. **No authentication on USB commands**: Any connected host can arm and fire the glitcher. In lab environments this is acceptable; in field use, authentication should be added.

4. **Glitch outputs are dangerous**: The VCC shunt can draw >10A from low-impedance supplies. Always use with a current-limited supply and never leave armed when unattended.

---

## 10. References

1. [Fault Attacks on Secure Elements: From Glitch to Flash](https://doi.org/10.1007/978-3-030-42068-0_3)
2. [Voltage Glitching on the STM32F4: A Practical Guide](https://wiki.newae.com/Voltage_Glitch_Atacks)
3. [Lattice iCE40 Programming and Configuration Guide](https://www.latticesemi.com/view-document?document_id=46502)
4. [STM32F407 Reference Manual RM0090](https://www.st.com/resource/en/reference_manual/dm00031020.pdf)
5. [UCC27511 Low-Side Gate Driver Datasheet](https://www.ti.com/lit/ds/symlink/ucc27511.pdf)
6. [INA210 Current Sense Amplifier Datasheet](https://www.ti.com/lit/ds/symlink/ina210.pdf)
7. [IEEE 1149.1 Standard Test Access Port and Boundary-Scan Architecture](https://standards.ieee.org/ieee/1149.1/6981/)