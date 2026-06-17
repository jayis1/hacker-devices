# Phase 1: Conceptual Architecture — HDMI-Siphon

**Author: jayis1**  
**Version: 1.0.0**

---

## 1. Purpose

The HDMI-Siphon is a transparent HDMI inline interposer that captures, manipulates, and exfiltrates video signals between an HDMI source and a display. It operates without introducing perceptible latency or visual artifacts, making it undetectable during normal operation.

## 2. Performance Targets

| Parameter | Target | Rationale |
|---|---|---|
| Max resolution | 1920×1080@60Hz (1080p60) | Covers 95%+ of business displays, kiosks, signage |
| Pixel clock | 165 MHz (HDMI 1.4a max) | Standard HDMI 1.4a limit |
| Passthrough latency | < 2 pixel clocks (< 12 ns @ 165 MHz) | Undetectable by human vision |
| Frame capture time | ≤ 1 frame period (16.7 ms @ 60 Hz) | Capture entire frame without tearing |
| EDID switch time | < 100 ms | Hot-plug detection acceptable |
| CEC bit rate | ~3.6 kHz (CEC spec: 3.57 ±1.79 kHz) | Per HDMI-CEC specification |
| Battery life (covert) | ≥ 2 hours continuous capture | Sufficient for assessment window |
| WiFi throughput | ≥ 2 Mbps | 1080p JPEG frames at ~1 fps |
| Form factor | 65×45×15 mm | Same as standard HDMI coupler |

## 3. Constraints

- **Cost target:** Under $40 BOM at 100 units
- **No active cooling** — fanless operation, <1W total power
- **No external antennas** — PCB trace antenna for WiFi/BLE
- **HDMI 1.4a compliance** — not HDMI 2.0 (no 4K60 needed)
- **No perceptible visual degradation** — must pass visual inspection by user
- **Must work with HDCP sources** — device must negotiate HDCP to maintain transparency, even if stripping

## 4. Top-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           HDMI-Siphon                                   │
│                                                                         │
│  ┌──────────┐  24-bit RGB   ┌──────────┐  24-bit RGB   ┌──────────┐  │
│  │ HDMI RX  │────+sync─────▶│  FPGA    │────+sync─────▶│ HDMI TX  │  │
│  │ TFP401   │◀────I²C──────│iCE40UP5K│◀────I²C──────▶│ TFP410   │  │
│  └──────────┘               │          │               └──────────┘  │
│       │                      │  SDRAM  │                      │       │
│       ▼                      │  Ctrl   │                      ▼       │
│  ┌──────────┐               │ 32MB    │                ┌──────────┐  │
│  │   EDID   │               │   IS42  │                │  HDMI    │  │
│  │  EEPROM  │               │ S16160  │                │  OUTPUT  │  │
│  │  24LC02  │               └────┬────┘                └──────────┘  │
│  └──────────┘                    │                                     │
│       ▲                         │                                     │
│       │    ┌────────────────────┴────────────────┐                    │
│       │    │           ESP32-S3                   │                    │
│       └────┤   GPIO  ◀──────▶  FPGA SPI Slave     │                    │
│            │   UART  ◀──────▶  CH340 USB-Serial   │                    │
│            │   SD    ◀──────▶  microSD Card        │                    │
│            │   WiFi  ◀──────▶  2.4 GHz Antenna    │                    │
│            │   BLE   ◀──────▶  Bluetooth          │                    │
│            │   Batt  ◀──────▶  LP503048 Li-Po     │                    │
│            │   I²C   ◀──────▶  TCA9548 Mux Bus    │                    │
│            └───────────────────────────────────────┘                    │
└─────────────────────────────────────────────────────────────────────────┘
```

## 5. Bus Topology

| Bus | Speed | Devices |
|---|---|---|
| Pixel data bus | 165 MHz, 24-bit parallel | TFP401 → FPGA → TFP410 |
| FPGA-SDRAM bus | 166 MHz, 16-bit data + address/ctrl | FPGA ↔ IS42S16160 |
| FPGA-ESP32 SPI | 20 MHz, full-duplex | FPGA (slave) ↔ ESP32 (master) |
| EDID I²C bus | 100 kHz | TFP401, TFP410, 24LC02, ESP32 (via mux) |
| CEC bus | 3.6 kHz open-drain | HDMI IN, HDMI OUT, ESP32 (GPIO) |
| SD Card SPI | 20 MHz | ESP32 ↔ microSD |
| USB UART | 115200 baud | ESP32 ↔ CH340 ↔ USB-C |

## 6. Power Domains

| Domain | Voltage | Source | Consumers |
|---|---|---|---|
| HDMI_5V | 5.0 V | HDMI IN or USB-C | TFP401, TFP410, MCP73831 |
| VCC_3V3 | 3.3 V | XC6206P332MR from 5V | FPGA I/O, ESP32, SD, EEPROM, level shifters |
| VCC_1V8 | 1.8 V | XC6206P182MR from 5V | FPGA core, SDRAM |
| PLL_VCC | 1.2 V | Internal FPGA regulator | FPGA PLL |

## 7. Signal Flow (Passthrough Mode)

1. HDMI source transmits TMDS-encoded video + I²C DDC/EDID
2. TFP401 deserializes TMDS to 24-bit parallel RGB + DE/HSYNC/VSYNC + pixel clock
3. FPGA receives parallel data; in passthrough mode, routes directly to output FIFO
4. FPGA drives TFP410 input bus with same 24-bit RGB + syncs at same pixel clock
5. TFP410 serializes back to TMDS and drives HDMI output
6. Total latency: < 10 ns (two FPGA IOB flip-flops + wire delay)

## 8. Signal Flow (Capture Mode)

1. Same as passthrough but FPGA also writes incoming pixel data to SDRAM frame buffer
2. On frame start (VSYNC rising), FPGA begins writing pixels to buffer A
3. On next VSYNC, buffer A is complete; FPGA switches to buffer B (double buffering)
4. FPGA signals ESP32 via IRQ that a frame is ready
5. ESP32 reads frame from SDRAM via SPI at up to 20 MHz (~100 MB/s)
6. ESP32 compresses frame to JPEG and writes to microSD or transmits via WiFi
7. ESP32 signals FPGA to release buffer A

**Author: jayis1**
