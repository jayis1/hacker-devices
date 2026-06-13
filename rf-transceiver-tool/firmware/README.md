# RF Transceiver Tool — Firmware

Production-quality firmware for the STM32F401CCU6-based RF Transceiver Tool.

## Features

- **CC1101 driver** — Sub-GHz (300–928 MHz) TX/RX with DMA, RSSI scanning, channel hopping
- **nRF24L01+ driver** — 2.4 GHz Enhanced ShockBurst TX/RX, channel scanning, multi-pipe
- **SSD1306 driver** — 128×64 OLED display with text and RSSI bar rendering
- **USB CDC** — Virtual serial port for host control and data capture
- **Button UI** — Mode cycling (BTN1) and action trigger (BTN2)

## Architecture

```
                    ┌─────────────────────┐
                    │      main.c          │
                    │  (app loop, buttons) │
                    └─────────┬───────────┘
                              │
            ┌─────────────────┼──────────────────┐
            │                 │                   │
    ┌───────┴───────┐ ┌──────┴──────┐ ┌────────┴────────┐
    │   CC1101      │ │  nRF24L01+  │ │   SSD1306       │
    │   driver      │ │  driver     │ │   driver         │
    │   (SPI1+DMA)  │ │  (SPI2)     │ │   (I2C1)        │
    └───────┬───────┘ └──────┬──────┘ └────────┬────────┘
            │                 │                   │
    ┌───────┴─────────────────┴───────────────────┴────┐
    │                   board.h                         │
    │            (pin defs, clock config, GPIO)          │
    └───────────────────────┬───────────────────────────┘
                            │
                    ┌───────┴────────┐
                    │  registers.h   │
                    │  (MMIO defs)   │
                    └────────────────┘
```

## Building

### Prerequisites

- `arm-none-eabi-gcc` toolchain (v12+)
- `st-link` or `STM32CubeProgrammer` for flashing

### Build

```bash
cd firmware/
make clean
make all
```

### Output files

- `firmware.elf` — ELF with debug symbols
- `firmware.bin` — Raw binary for flashing at `0x08004000`
- `firmware.map` — Linker map with memory usage

### Flashing

**Via ST-Link (SWD):**
```bash
make flash
```

**Via USB DFU (hold BTN1 during reset):**
```bash
dfu-util -a 0 -s 0x08004000:leave -D firmware.bin
```

**Via UART bootloader:**
```bash
stm32flash -w firmware.bin /dev/ttyUSB0
```

## Memory Layout

| Region | Address | Size | Content |
|---|---|---|---|
| Bootloader | 0x08000000 | 16 KB | USB DFU bootloader (separate) |
| Application | 0x08004000 | 240 KB | Main firmware (this code) |
| Config store | 0x0803C000 | 8 KB | RF calibration data, profiles |

## Wire Protocol

The firmware communicates with the companion app over USB CDC (115200 baud virtual serial). See `app/utils/protocol.js` for the full command specification.

### Command format

```
[0xAA] [CMD_ID] [LEN] [PAYLOAD...] [CRC8]
```

- `0xAA` — Start-of-packet marker
- `CMD_ID` — Command ID (1 byte)
- `LEN` — Payload length (1 byte, max 252)
- `PAYLOAD` — Variable length data
- `CRC8` — CRC-8 over CMD_ID + LEN + PAYLOAD

### Key Commands

| CMD_ID | Name | Direction | Description |
|---|---||---|
| 0x01 | PING | Host→Device | Echo test |
| 0x02 | GET_STATUS | Host→Device | Get current mode and RSSI |
| 0x10 | CC1101_INIT | Host→Device | Initialize CC1101 |
| 0x11 | CC1101_CONFIG | Host→Device | Configure CC1101 (freq, modulation, rate) |
| 0x12 | CC1101_RX | Host→Device | Start CC1101 RX mode |
| 0x13 | CC1101_TX | Host→Device | Transmit CC1101 packet |
| 0x14 | CC1101_SCAN | Host→Device | Scan CC1101 channels |
| 0x15 | CC1101_PACKET | Device→Host | Received CC1101 packet data |
| 0x20 | NRF24_INIT | Host→Device | Initialize nRF24L01+ |
| 0x21 | NRF24_CONFIG | Host→Device | Configure nRF24L01+ (channel, rate, power) |
| 0x22 | NRF24_RX | Host→Device | Start nRF24L01+ RX mode |
| 0x23 | NRF24_TX | Host→Device | Transmit nRF24L01+ packet |
| 0x24 | NRF24_SCAN | Host→Device | Scan nRF24L01+ channels |
| 0x25 | NRF24_PACKET | Device→Host | Received nRF24L01+ packet data |
| 0x30 | DISPLAY_TEXT | Host→Device | Show text on SSD1306 |
| 0x31 | DISPLAY_BAR | Host→Device | Show RSSI bar on SSD1306 |
| 0xFF | ERROR | Device→Host | Error response |

## License

GPL-2.0