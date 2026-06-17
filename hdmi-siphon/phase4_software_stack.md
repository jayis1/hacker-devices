# Phase 4: Software Stack & Firmware Architecture — HDMI-Siphon

**Author: jayis1**  
**Version: 1.0.0**

---

## 1. Software Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│                     COMPANION APPLICATIONS                            │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────────┐ │
│  │ React Native │  │  Python CLI  │  │ Web Browser (HTTP API)     │ │
│  │ Mobile App   │  │  hdmi-siphon │  │ http://192.168.4.1:80      │ │
│  │ (iOS/Android)│  │  -cli.py     │  │ WebSocket ws://:8080       │ │
│  └──────┬───────┘  └──────┬───────┘  └──────────────┬─────────────┘ │
│         │                 │                          │               │
│         │      JSON over WebSocket / BLE GATT        │               │
│         └─────────────────┼──────────────────────────┘               │
│                           ▼                                         │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                    ESP32-S3 (Control Plane)                    │  │
│  │                                                                │  │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────┐  │  │
│  │  │ HTTP/WS    │  │ BLE GATT   │  │ Capture    │  │ CEC    │  │  │
│  │  │ Server     │  │ Server     │  │ Controller │  │ Monitor│  │  │
│  │  └────────────┘  └────────────┘  └────────────┘  └────────┘  │  │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────┐  │  │
│  │  │ SD Card    │  │ EDID       │  │ Battery    │  │ WiFi   │  │  │
│  │  │ Driver     │  │ Manager    │  │ Monitor    │  │ Manager│  │  │
│  │  └────────────┘  └────────────┘  └────────────┘  └────────┘  │  │
│  │                                                                │  │
│  │  ┌──────────────────────────────────────────────────────┐      │  │
│  │  │              SPI Master (20 MHz)                     │      │  │
│  │  └──────────────────────┬───────────────────────────────┘      │  │
│  └─────────────────────────┼──────────────────────────────────────┘  │
│                            ▼                                         │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │              FPGA iCE40UP5K (Real-Time Video)                  │  │
│  │                                                                │  │
│  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  │  │
│  │  │SDRAM   │  │Frame   │  │OSD     │  │CEC     │  │EDID    │  │  │
│  │  │Ctrl    │  │Buffer  │  │Overlay │  │Ctrl    │  │Sniffer │  │  │
│  │  └────────┘  └────────┘  └────────┘  └────────┘  └────────┘  │  │
│  │  ┌────────┐  ┌────────┐  ┌────────┐                           │  │
│  │  │Pixel   │  │Capture │  │SPI     │                           │  │
│  │  │Passthru│  │Engine  │  │Slave   │                           │  │
│  │  └────────┘  └────────┘  └────────┘                           │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                            │                                         │
│              ┌─────────────┴─────────────┐                           │
│              ▼                           ▼                           │
│  ┌──────────────────┐     ┌──────────────────┐                       │
│  │ HDMI RX (TFP401) │     │ HDMI TX (TFP410) │                       │
│  │ TMDS → 24-bit RGB│     │ 24-bit RGB→TMDS  │                       │
│  └──────────────────┘     └──────────────────┘                       │
└──────────────────────────────────────────────────────────────────────┘
```

## 2. FreeRTOS Task Descriptions

| Task | Priority | Stack (B) | Period | Description |
|---|---|---|---|---|
| `main_task` | 5 | 4096 | 1s | System health, FPGA polling, event dispatch |
| `wifi_task` | 4 | 8192 | — | WiFi AP/STA + HTTP/WebSocket server |
| `capture_task` | 3 | 4096 | Blocking | Frame transfer and SD write |
| `cec_task` | 2 | 3072 | 5ms | CEC bus polling |
| `battery_task` | 1 | 2048 | 5s | ADC reading and shutdown |
| `ble_task` | 4 | 4096 | 2s | BLE advertising and notifications |

## 3. Memory Map

| Region | Size | Content |
|---|---|---|
| ESP32-S3 PSRAM | 8 MB | Frame buffer, HTTP buffers, JPEG workspace |
| ESP32-S3 IRAM | 512 KB | Code, ISR vectors, DMA descriptors |
| ESP32-S3 DRAM | 320 KB | Stack, heap, driver data |
| FPGA BRAM | 120 Kbit | Line buffers, OSD font, FIFOs |
| SDRAM (IS42S16160) | 32 MB | Two 12 MB frame buffers + 8 MB scratch |
| SD Card | 2-32 GB | FAT32, captured frames, logs, config |

## 4. SPI Protocol Detail

### Register Transaction Timing

```
SPI Transaction (4 bytes):
┌─────┬─────┬─────┬─────┐
│ CMD/│ ADDR│ DATA│ DATA│
│ ADDR│ LOW │ HIGH│ LOW │
└─────┴─────┴─────┴─────┘
  bit   bit   bit   bit
   7     6-0   5-3   4-0

Write: CMD = 0x80 | (addr_hi & 0x0F)
Read:  CMD = 0x00 | (addr_hi & 0x0F)
```

### Burst Transfer

For frame data transfer, burst mode reads consecutive 32-bit words:

1. ESP32 writes FRAME_XFER_START: initiates DMA read from SDRAM
2. ESP32 reads FRAME_DATA_PORT in a loop at SPI speed (20 MHz → ~2.5 MB/s)
3. For a 1080p frame (6.2 MB): ~2.5 seconds transfer time
4. With DMA and quad-SPI: could reach ~10 MB/s → ~0.6 second transfer

## 5. FPGA Bitstream Configuration

The iCE40UP5K FPGA is configured by the ESP32-S3 at boot:

1. ESP32 holds FPGA_CRESET low for 10ms
2. ESP32 releases FPGA_CRESET high
3. FPGA begins sampling SPI slave configuration (if configured as master)
4. In this design, the FPGA is pre-configured via a serial flash or ESP32 loads the bitstream
5. ESP32 monitors FPGA_DONE pin; if not asserted within 5s, logs error and retries
6. Once FPGA is configured, ESP32 writes initial register state (passthrough mode, SDRAM init)

## 6. Boot Sequence

```
1. Power-on / Reset
2. ESP32 ROM bootloader
3. ESP32 loads main firmware from flash
4. main() → app_main()
5. NVS flash init
6. GPIO init (all pins to default state)
7. SPI bus init (20 MHz master)
8. I²C bus init (100 kHz master)
9. FPGA configuration (load bitstream)
10. FPGA register init (passthrough mode, SDRAM auto-init)
11. SD Card mount (FAT32, /sdcard)
12. EDID manager init (read display EDID)
13. WiFi init (AP mode, SSID="HDMI-Siphon-AP")
14. HTTP + WebSocket server start (port 80/8080)
15. BLE GATT service start
16. FreeRTOS task creation
17. Main loop begins (1s periodic health checks)
18. System ready → RGB LED solid green
```

## 7. Power Management

| State | Power | Behavior |
|---|---|---|
| Active (passthrough) | ~450 mA @ 5V | Full operation, WiFi+BLE streaming status |
| Active (capture) | ~550 mA @ 5V | Capturing and writing frames to SD |
| Covert | ~350 mA @ 5V | WiFi off, BLE off, LEDs off, capture only |
| Deep Sleep | ~50 µA | Timer wake, GPIO wake (button or HDMI detect) |
| Shutdown | 0 µA | Battery disconnect via MOSFET; USB-C plug restores |

**Author: jayis1**
