# RF Transceiver Tool

Multi-protocol Sub-GHz + 2.4 GHz RF transceiver for security research and protocol analysis.

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue)

## Overview

The RF Transceiver Tool is a portable, USB-powered device for analyzing and testing Sub-GHz (300–928 MHz) and 2.4 GHz ISM band radio protocols. It combines a **CC1101** Sub-GHz transceiver with an **nRF24L01+** 2.4 GHz transceiver, controlled by an **STM32F401CCU6** Cortex-M4 MCU. An onboard SSD1306 OLED provides status display, and USB CDC provides host communication.

**Use cases:** Keyfob protocol analysis, garage door replay research, weather station decoding, nRF24 Enhanced ShockBurst monitoring, IoT device testing, radio frequency auditing.

## Key Specs

| Parameter | Value |
|---|---|
| Sub-GHz Range | 300–928 MHz (CC1101) |
| 2.4 GHz Range | 2400–2525 MHz (nRF24L01+) |
| Sub-GHz Modulation | 2-FSK, GFSK, ASK/OOK, 4-FSK, MSK |
| Sub-GHz Data Rate | 1.2–500 kbps |
| 2.4 GHz Data Rate | 250 kbps, 1 Mbps, 2 Mbps |
| MCU | STM32F401CCU6 (Cortex-M4, 84 MHz) |
| Interface | USB 2.0 CDC (virtual serial) |
| Display | SSD1306 128×64 OLED (I2C) |
| Power | USB 5V / 3.7V LiPo |
| PCB | 85 × 54 mm, 4-layer |
| BOM Cost | ~$45 |

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     Host (PC / Phone)                        │
│                    USB CDC Virtual Serial                     │
└──────────────────────────┬───────────────────────────────────┘
                           │ USB 2.0 FS
┌──────────────────────────┼───────────────────────────────────┐
│                     STM32F401CCU6                            │
│  ┌─────────┐  ┌──────────┐  ┌─────────┐  ┌───────────────┐  │
│  │ SPI1    │  │ SPI2     │  │ I2C1    │  │ USB OTG FS    │  │
│  │ +DMA    │  │ +DMA     │  │         │  │               │  │
│  └────┬────┘  └────┬─────┘  └────┬────┘  └───────┬───────┘  │
│       │            │              │               │           │
│  ┌────┴────┐  ┌────┴─────┐ ┌────┴────┐    ┌─────┴──────┐  │
│  │ CC1101  │  │nRF24L01+ │ │ SSD1306 │    │ USB-C Port │  │
│  │ Sub-GHz │  │ 2.4 GHz  │ │ OLED    │    │            │  │
│  └────┬────┘  └────┬─────┘ └─────────┘    └────────────┘  │
│       │            │                                         │
└───────┼────────────┼─────────────────────────────────────────┘
        │            │
   ┌────┴────┐  ┌────┴─────┐
   │ 433 MHz │  │ 2.4 GHz  │
   │  868    │  │ Antenna  │
   │  915    │  │          │
   │ Antenna │  │          │
   └─────────┘  └──────────┘
```

## Documentation

| Phase | Document | Content |
|---|---|---|
| 1 | [Conceptual Architecture](phase1_conceptual_architecture.md) | Purpose, targets, block diagram, power budget |
| 2 | [Component Selection & Schematics](phase2_component_selection_schematics.md) | BOM, pinouts, netlists, decoupling |
| 3 | [PCB Blueprints & Layout](phase3_pcb_blueprints_layout.md) | Stackup, routing rules, DFM |
| 4 | [Software Stack](phase4_software_stack.md) | MMIO, drivers, USB protocol, memory map |

## Hardware Design Files

| File | Description |
|---|---|
| [kicad/rf-transceiver-tool.kicad_pro](kicad/rf-transceiver-tool.kicad_pro) | KiCad project file |
| [kicad/rf-transceiver-tool.kicad_sch](kicad/rf-transceiver-tool.kicad_sch) | Schematic (MCU, CC1101, nRF24L01+, USB, power) |
| [kicad/rf-transceiver-tool.kicad_pcb](kicad/rf-transceiver-tool.kicad_pcb) | PCB layout (4-layer, 85×54mm) |

## Firmware

| File | Description |
|---|---|
| [firmware/main.c](firmware/main.c) | Main entry, clock config, USB CDC, app loop |
| [firmware/board.h](firmware/board.h) | Pin mappings, LED/button defs |
| [firmware/registers.h](firmware/registers.h) | STM32F401 MMIO register definitions |
| [firmware/drivers/cc1101.c](firmware/drivers/cc1101.c) | CC1101 Sub-GHz driver (SPI1+DMA) |
| [firmware/drivers/cc1101.h](firmware/drivers/cc1101.h) | CC1101 driver header |
| [firmware/drivers/nrf24l01.c](firmware/drivers/nrf24l01.c) | nRF24L01+ 2.4GHz driver (SPI2) |
| [firmware/drivers/nrf24l01.h](firmware/drivers/nrf24l01.h) | nRF24L01+ driver header |
| [firmware/drivers/ssd1306.c](firmware/drivers/ssd1306.c) | SSD1306 OLED display driver (I2C1) |
| [firmware/drivers/ssd1306.h](firmware/drivers/ssd1306.h) | SSD1306 driver header |
| [firmware/drivers/spi_dma.c](firmware/drivers/spi_dma.c) | SPI DMA helper (DMA1/DMA2) |
| [firmware/drivers/spi_dma.h](firmware/drivers/spi_dma.h) | SPI DMA helper header |
| [firmware/usb_descriptors.h](firmware/usb_descriptors.h) | USB CDC device descriptors |
| [firmware/Makefile](firmware/Makefile) | Build system |

## Companion App

| File | Description |
|---|---|
| [app/App.js](app/App.js) | Main React Native app |
| [app/screens/DeviceScreen.js](app/screens/DeviceScreen.js) | Device control and packet log |
| [app/screens/SettingsScreen.js](app/screens/SettingsScreen.js) | RF configuration UI |
| [app/components/StatusCard.js](app/components/StatusCard.js) | Reusable status card |
| [app/utils/protocol.js](app/utils/protocol.js) | Wire protocol definitions |
| [app/package.json](app/package.json) | Dependencies |

## Quick Start

1. **Build firmware:** `cd firmware && make`
2. **Flash:** `st-flash write firmware.bin 0x08004000`
3. **Connect:** Plug device into USB, open serial terminal at 115200 baud
4. **App:** `cd app && npm install && npx react-native run-android`

## License

GPL-2.0