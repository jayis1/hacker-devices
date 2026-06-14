# PHANTOM — USB HID Emulation Payload Injector

A compact, multi-profile USB HID emulation device for authorized security research and penetration testing. PHANTOM appears as a standard USB flash drive until activated, then switches to a programmable HID keyboard/mouse/consumer-control device capable of executing DuckyScript payloads with precise timing control.

> **⚠️ Legal Notice:** This device is designed for **authorized security research and penetration testing only**. Unauthorized access to computer systems is illegal in most jurisdictions. The authors assume no liability for misuse. Always obtain proper authorization before testing any system you do not own. This tool is intended solely for defensive security research, red team engagements with written authorization, and educational purposes.

---

## Specifications

| Feature | Specification |
|---------|--------------|
| **MCU** | Raspberry Pi RP2040 (dual ARM Cortex-M0+ @ 133 MHz, 264 KB SRAM) |
| **External Flash** | W25Q128JVSIQ 16 MB QSPI (payload storage, 100+ profiles) |
| **WiFi Module** | ESP32-C3-MINI-1 (WiFi 6 + BLE 5, AT-command over UART) |
| **Display** | SSD1306 128×64 OLED (I²C, monochrome) |
| **Input** | EC11 rotary encoder with push button (profile navigation & select) |
| **USB Interface** | RP2040 native USB 1.1 Full-Speed device controller |
| **USB Modes** | HID Keyboard, HID Mouse, HID Consumer Control, Mass Storage (stealth) |
| **Payload Format** | DuckyScript 1.0+ compatible with extensions |
| **Profiles** | Up to 128 stored profiles with metadata |
| **Stealth Mode** | Appears as USB mass storage device until activated |
| **Kill Switch** | Hardware slide switch — physically disconnects USB data lines |
| **Geofencing** | WiFi-based network detection to restrict payload execution by location |
| **Power (USB)** | USB bus-powered (5 V, < 100 mA typical, 500 mA max) |
| **Power (Battery)** | 3.7 V 500 mAh LiPo with MCP73831 charger, ~8 hours standby |
| **Charging** | USB bus power charges LiPo, ~2 hours full charge |
| **Dimensions** | 50 mm × 25 mm × 12 mm (USB-A stick form factor) |
| **Weight** | ~15 g (without battery), ~25 g (with battery) |
| **BOM Cost** | ~$18 (1K volume), < $35 assembled |
| **Operating Temp** | 0°C to 60°C (commercial grade) |

## Key Features

### Multi-Profile Payload System
- **128 payload profiles** stored in 16 MB SPI flash
- **DuckyScript 1.0 compatible** parser with PHANTOM extensions
- **Per-profile configuration**: name, icon, key delay, repeat count, auto-execute flag
- **Profile selection** via rotary encoder and OLED display — no host software needed
- **Sequential execution**: chain multiple profiles with conditional branching

### Stealth Operation
- **Mass storage disguise**: Device enumerates as a normal USB flash drive by default
- **Hardware kill switch**: Physical slide switch disconnects USB D+/D- lines via analog switch
- **Geofencing**: Payload execution restricted to approved WiFi networks (SSID whitelist)
- **Timed execution**: Schedule payload execution at specific times or after delays

### Wireless Control
- **WiFi 6 (ESP32-C3)**: Remote payload trigger, configuration, and file transfer over HTTP
- **BLE 5.0**: Companion app connectivity for real-time control and monitoring
- **AT-command interface**: Simple UART bridge between RP2040 and ESP32-C3

### DuckyScript Extensions
All standard DuckyScript 1.0 commands plus PHANTOM extensions:

| Command | Description |
|---------|-------------|
| `STRING` | Type a string with default keystroke delay |
| `DELAY` | Wait N milliseconds |
| `REM` | Comment line |
| `DEFAULT_DELAY` | Set default delay between commands |
| `GUI` / `WINDOWS` | Windows/Super key |
| `SHIFT` / `CTRL` / `ALT` | Modifier keys |
| `UP` / `DOWN` / `LEFT` / `RIGHT` | Arrow keys |
| `REPEAT n` | Repeat last command N times |
| `HOLD` | Hold a key down |
| `RELEASE` | Release a held key |
| `MOUSE_MOVE dx dy` | Move mouse cursor |
| `MOUSE_CLICK left\|right\|middle` | Click mouse button |
| `MOUSE_SCROLL n` | Scroll mouse wheel |
| `CONSUMER key` | Consumer control key (play/pause, vol+, etc.) |
| `PROFILE name` | Jump to another profile |
| `IF_NETWORK ssid` | Conditional: execute next block only on matching network |
| `WAIT_FOR_NETWORK ssid` | Block until specified network detected |
| `STEALTH_ON` | Switch to HID mode (from mass storage) |
| `STEALTH_OFF` | Switch back to mass storage mode |
| `LED_R/G/B hex` | Set RGB LED color |
| `OLED_LINE n text` | Display text on OLED line n |
| `RANDOM_DELAY min max` | Random delay between min and max ms |

## Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                      PHANTOM Top-Level Block                        │
│                                                                     │
│  ┌──────────────┐    ┌──────────────────────────────────────┐      │
│  │  USB-A       │    │          RP2040 MCU                   │      │
│  │  Connector   │◄──►│  ┌──────────┐  ┌──────────────────┐ │      │
│  │  (Power +    │    │  │ Dual     │  │ USB FS Device    │ │      │
│  │   Data)      │    │  │ Cortex-  │  │ Controller        │ │      │
│  │              │    │  │ M0+      │  │ - HID Keyboard    │ │      │
│  └──────────────┘    │  │ @133MHz  │  │ - HID Mouse       │ │      │
│                       │  └────┬─────┘  │ - HID Consumer   │ │      │
│  ┌──────────────┐    │       │        │ - MSC (stealth)   │ │      │
│  │  W25Q128     │    │       │        └────────┬──────────┘ │      │
│  │  16MB SPI    │◄──►│       │                 │            │      │
│  │  Flash       │    │  ┌────┴──────────┐      │            │      │
│  └──────────────┘    │  │ QSPI / SPI0   │      │            │      │
│                       │  │ Boot Flash    │      │            │      │
│  ┌──────────────┐    │  └───────────────┘      │            │      │
│  │  ESP32-C3    │    │                          │            │      │
│  │  WiFi 6 +    │◄──►│  ┌───────────────┐       │            │      │
│  │  BLE 5       │    │  │ UART1         │       │            │      │
│  │  (AT cmd)    │    │  │ (AT bridge)   │       │            │      │
│  └──────────────┘    │  └───────────────┘       │            │      │
│                       │                          │            │      │
│  ┌──────────────┐    │  ┌───────────────┐       │            │      │
│  │  SSD1306     │◄──►│  │ I²C0          │       │            │      │
│  │  128×64 OLED │    │  │ (display)     │       │            │      │
│  └──────────────┘    │  └───────────────┘       │            │      │
│                       │                          │            │      │
│  ┌──────────────┐    │  ┌───────────────┐       │            │      │
│  │  EC11 Rotary │◄──►│  │ GPIO          │       │            │      │
│  │  Encoder     │    │  │ (encoder,     │       │            │      │
│  │  + Button    │    │  │  kill switch) │       │            │      │
│  └──────────────┘    │  └───────────────┘       │            │      │
│                       │                          │            │      │
│  ┌──────────────┐    │  ┌───────────────┐       │            │      │
│  │  MCP73831    │◄──►│  │ ADC           │       │            │      │
│  │  LiPo Charger│    │  │ (battery mon) │       │            │      │
│  └──────────────┘    │  └───────────────┘       │            │      │
│                       └──────────────────────────────────────┘      │
│  ┌──────────────┐                                                    │
│  │  3.7V 500mAh │    ┌──────────────┐                               │
│  │  LiPo Battery│◄──►│  Power Mux   │◄── VBUS (5V)                 │
│  └──────────────┘    │  & Regulation │                               │
│                       └──────────────┘                               │
│                                                                     │
│  ┌──────────────┐    ┌──────────────┐                               │
│  │  Kill Switch │    │  WS2812B     │                               │
│  │  (slide)     │    │  RGB LED     │                               │
│  └──────────────┘    └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
```

## Pin Assignments

### RP2040 Pin Map (QFN-56)

| Pin | GPIO | Net | Function | Notes |
|-----|------|-----|----------|-------|
| 1 | — | GND | Ground | |
| 2 | GPIO0 | FLASH_QSPI_SCLK | QSPI clock | Boot flash |
| 3 | GPIO1 | FLASH_QSPI_SS | QSPI chip select | Boot flash |
| 4 | GPIO2 | FLASH_QSPI_SD0 | QSPI data 0 | Boot flash |
| 5 | GPIO3 | FLASH_QSPI_SD1 | QSPI data 1 | Boot flash |
| 6 | GPIO4 | FLASH_QSPI_SD2 | QSPI data 2 | Boot flash |
| 7 | GPIO5 | FLASH_QSPI_SD3 | QSPI data 3 | Boot flash |
| 8 | — | RUN | Reset | Active-low reset |
| 9 | — | GND | Ground | |
| 10 | GPIO6 | ENC_A | Encoder phase A | Rotary encoder |
| 11 | GPIO7 | ENC_B | Encoder phase B | Rotary encoder |
| 12 | GPIO8 | ENC_BTN | Encoder button | Active-low, internal pull-up |
| 13 | GPIO9 | KILL_SW | Hardware kill switch | Active-low = kill mode |
| 14 | GPIO10 | OLED_SDA | I²C data | SSD1306 display |
| 15 | GPIO11 | OLED_SCL | I²C clock | SSD1306 display |
| 16 | GPIO12 | ESP_TX | UART TX → ESP32-C3 | AT commands |
| 17 | GPIO13 | ESP_RX | UART RX ← ESP32-C3 | AT responses |
| 18 | GPIO14 | ESP_RST | ESP32-C3 reset | Active-low |
| 19 | GPIO15 | LED_DATA | WS2812B data | Status RGB LED |
| 20 | GPIO16 | EXT_FLASH_CS | SPI flash CS | W25Q128 payload storage |
| 21 | GPIO17 | EXT_FLASH_SCLK | SPI flash clock | W25Q128 |
| 22 | GPIO18 | EXT_FLASH_MOSI | SPI flash MOSI | W25Q128 |
| 23 | GPIO19 | EXT_FLASH_MISO | SPI flash MISO | W25Q128 |
| 24 | GPIO20 | USB_SWITCH | USB mode switch | HIGH=HID, LOW=MSC |
| 25 | GPIO21 | BAT_SENSE | Battery voltage ADC | Voltage divider input |
| 26 | GPIO22 | LED_EN | LED enable | HIGH=LED on |
| 27 | GPIO23 | — | Not connected | |
| 28 | — | GND | Ground | |
| 29 | GPIO24 | BOOT_BTN | Boot button | Active-low |
| 30 | GPIO25 | — | User GPIO | Breakout pad |
| 31 | GPIO26/ADC0 | BAT_SENSE | Battery ADC | 0–3.3V range |
| 32 | GPIO27/ADC1 | — | Spare ADC | Breakout pad |
| 33 | GPIO28/ADC2 | — | Spare ADC | Breakout pad |
| 34 | — | GND | Ground | |
| 35 | — | 3V3 | 3.3V rail | Decoupled |
| 36 | — | 1V8 | 1.8V core | Decoupled |
| 37 | GPIO0 (USB) | USB_DP | USB D+ | Via analog switch |
| 38 | GPIO1 (USB) | USB_DM | USB D- | Via analog switch |
| 39 | — | VBUS | USB 5V | Power input |
| 40 | — | GND | Ground | |
| 41 | — | GND | Ground | |

### ESP32-C3-MINI-1 Pin Map

| Pin | Net | Function | Notes |
|-----|-----|----------|-------|
| TXD0 | ESP_RX | UART TX → RP2040 | 115200 baud |
| RXD0 | ESP_TX | UART RX ← RP2040 | AT command mode |
| EN | ESP_RST | Reset from RP2040 | Active-low |
| GPIO8 | WIFI_STATUS | WiFi connected indicator | HIGH=connected |
| GPIO9 | BLE_STATUS | BLE connected indicator | HIGH=connected |
| VDD | 3V3 | Power | 3.3V rail |
| GND | GND | Ground | |

### SSD1306 OLED (I²C, 128×64)

| Pin | Net | Function |
|-----|-----|----------|
| VDD | 3V3 | Power |
| GND | GND | Ground |
| SCL | OLED_SCL | I²C clock (GPIO11) |
| SDA | OLED_SDA | I²C data (GPIO10) |

### W25Q128JVSIQ SPI Flash (16 MB)

| Pin | Net | Function |
|-----|-----|----------|
| CS | EXT_FLASH_CS | Chip select (GPIO16) |
| DO | EXT_FLASH_MISO | Data out (GPIO19) |
| DI | EXT_FLASH_MOSI | Data in (GPIO18) |
| CLK | EXT_FLASH_SCLK | Clock (GPIO17) |
| WP | 3V3 | Write protect (always protected) |
| HOLD | 3V3 | Hold off (always enabled) |
| VCC | 3V3 | Power |
| GND | GND | Ground |

## Power Architecture

```
USB VBUS (5V) ──┬──────────────────────┬────────────────────┐
                 │                      │                    │
            ┌────┴─────┐         ┌──────┴──────┐      ┌──────┴──────┐
            │  MCP73831 │         │   AP2112K   │      │   AP2112K   │
            │  Charger  │         │  3.3V LDO   │      │  1.8V LDO   │
            │  (LiPo)   │         │  (500mA)    │      │  (300mA)    │
            └────┬──────┘         └──────┬──────┘      └──────┬──────┘
                 │                      │                    │
          LiPo 3.7V               VDD_3V3              VDD_1V8
          500mAh                  ┌──┴──┐               ┌──┴──┐
                 │                 │      │               │      │
           ┌─────┴─────┐     RP2040   ESP32-C3      RP2040   Flash
           │  500mAh   │     SSD1306  W25Q128      (core)   (core)
           │  LiPo     │     OLED
           └───────────┘
```

**Power Modes:**

| Mode | Current Draw | Duration |
|------|-------------|----------|
| USB active (HID) | ~45 mA | Continuous |
| USB active (MSC stealth) | ~30 mA | Continuous |
| Battery standby | ~2 mA (OLED off) | ~250 hours |
| Battery active (HID) | ~55 mA | ~8 hours |
| Battery WiFi active | ~120 mA (burst) | ~4 hours |
| Sleep (deep) | ~0.5 mA | ~1000 hours |

## BOM Summary

| Ref | Part | Description | Qty | Unit Price |
|-----|------|-------------|-----|-----------|
| U1 | RP2040 | Dual Cortex-M0+, QFN-56 | 1 | $1.00 |
| U2 | W25Q128JVSIQ | 16MB SPI Flash, SOIC-8 | 1 | $0.85 |
| U3 | ESP32-C3-MINI-1 | WiFi 6 + BLE 5, module | 1 | $3.50 |
| U4 | SSD1306 | 128×64 OLED, I²C | 1 | $2.50 |
| U5 | MCP73831T-2ACI/OT | LiPo charger, SOT-23-5 | 1 | $0.45 |
| U6 | AP2112K-3.3 | 3.3V LDO, 600mA, SOT-23-5 | 1 | $0.25 |
| U7 | AP2112K-1.8 | 1.8V LDO, 300mA, SOT-23-5 | 1 | $0.25 |
| U8 | TS3USB221A | USB analog switch, VQFN-10 | 1 | $0.75 |
| SW1 | Kill switch | Slide switch, SPDT | 1 | $0.15 |
| ENC1 | EC11 | Rotary encoder with button | 1 | $0.80 |
| D1 | WS2812B-2020 | RGB LED, 3.5×3.5mm | 1 | $0.12 |
| J1 | USB-A plug | USB-A male, SMT | 1 | $0.35 |
| Y1 | ABM8-12.000MHZ | 12 MHz crystal, 3.2×2.5mm | 1 | $0.25 |
| BAT1 | 502030 | 3.7V 500mAh LiPo | 1 | $3.00 |
| Misc | Passives | R/C/L, 0402 | ~30 | $1.50 |
| PCB | 50×25mm | 4-layer, ENIG finish | 1 | $1.50 |
| | | **Total** | | **~$17.72** |

## Security Features

### Hardware Kill Switch
The physical slide switch (SW1) disconnects the USB D+ and D- data lines via a TS3USB221A analog switch. When the kill switch is in the OFF position:
- USB data lines are physically disconnected from the host
- Only USB power (VBUS) remains connected for charging
- No HID commands can be sent regardless of firmware state
- The kill switch cannot be overridden by software

### Stealth Mode
By default, PHANTOM enumerates as a USB Mass Storage Class device, appearing as a normal flash drive. This provides:
- Plausible deniability during transport
- No HID enumeration alerts on the target host
- File storage for legitimate data
- Switching to HID mode via: rotary encoder selection, BLE command, WiFi trigger, or DuckyScript `STEALTH_ON` command

### Geofencing
The ESP32-C3 WiFi module can scan for nearby networks and restrict payload execution:
- **Whitelist mode**: Payloads only execute when connected to an approved SSID
- **Blacklist mode**: Payloads are blocked on specific networks (e.g., corporate monitoring)
- **GPS-free location**: Uses WiFi BSSID database for location approximation
- Network scans are performed before each payload execution

### Encrypted Payload Storage
- Payloads stored on W25Q128 SPI flash can be AES-128 encrypted
- Encryption key derived from device UID + user passphrase
- Decryption happens in SRAM — plaintext never written to flash
- Brute-force protection: 5-second lockout per failed decryption attempt

## Firmware Architecture

```
┌──────────────────────────────────────────────────┐
│                   Application                     │
│  ┌──────────┐ ┌──────────────┐ ┌──────────────┐ │
│  │ Profile   │ │ Payload      │ │ Stealth      │ │
│  │ Manager   │ │ Engine       │ │ Controller   │ │
│  └────┬──────┘ └──────┬───────┘ └──────┬───────┘ │
│       │               │                │          │
│  ┌────┴───────────────┴────────────────┴───────┐ │
│  │              DuckyScript Parser              │ │
│  └──────────────────┬───────────────────────────┘ │
│                     │                              │
│  ┌──────────────────┴───────────────────────────┐ │
│  │            HAL (Hardware Abstraction)          │ │
│  │  ┌─────────┐ ┌────────┐ ┌────────┐ ┌──────┐ │ │
│  │  │ USB HID │ │ SPI    │ │ I²C    │ │ UART │ │ │
│  │  │ Driver  │ │ Flash  │ │ OLED   │ │ WiFi │ │ │
│  │  └─────────┘ └────────┘ └────────┘ └──────┘ │ │
│  └───────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────┐ │
│  │          RP2040 Hardware (registers, clocks)  │ │
│  └───────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘
```

## Directory Structure

```
badusb-injector/
├── README.md                              # This file
├── phase1_conceptual_architecture.md      # Threat model, attack surfaces, data flow
├── phase2_component_selection_schematics.md # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md        # Stackup, routing, thermal, DFM
├── phase4_software_stack.md               # MMIO, clock init, USB stack, payload engine
├── kicad/
│   ├── device.kicad_pro                   # KiCad project file
│   ├── device.kicad_sch                   # Full schematic with real components
│   └── device.kicad_pcb                   # Board outline with placed footprints
├── firmware/
│   ├── Makefile                            # Cross-compile for ARM Cortex-M0+
│   ├── main.c                              # Main firmware & USB HID engine
│   ├── board.h                             # Pin definitions & board config
│   ├── registers.h                         # MCU register definitions
│   ├── usb_descriptors.h                  # USB HID descriptors
│   └── drivers/
│       ├── flash_storage.h                 # SPI flash payload storage header
│       ├── flash_storage.c                 # W25Q128 operations
│       ├── usb_hid.h                       # USB HID driver header
│       ├── usb_hid.c                       # Keyboard, mouse, consumer control
│       ├── wifi_bridge.h                  # WiFi bridge header
│       └── wifi_bridge.c                  # ESP32-C3 AT command interface
└── app/
    ├── package.json                        # React Native dependencies
    ├── App.js                              # Navigation & entry point
    ├── screens/
    │   ├── DeviceScreen.js                # Connection status & device info
    │   ├── PayloadScreen.js               # Payload management & editing
    │   └── SettingsScreen.js              # Device configuration & geofencing
    ├── components/
    │   ├── BleContext.js                  # BLE state management
    │   └── PayloadEditor.js               # DuckyScript editor with highlighting
    └── utils/                              # (future: protocol.js)
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Install Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git
export PICO_SDK_PATH=./pico-sdk

# Build firmware
cd firmware/
make

# Flash via SWD (Picoprobe)
make flash

# Or drag UF2 to mass storage
make uf2
cp build/phantom.uf2 /media/$USER/RPI-RP2/
```

### Companion App

```bash
cd app/
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios
```

### Loading Payloads

**Via OLED + Rotary Encoder:**
1. Power on device
2. Use rotary encoder to scroll through profiles
3. Press encoder button to select and execute

**Via BLE Companion App:**
1. Connect to PHANTOM via BLE
2. Create or edit payloads in the DuckyScript editor
3. Push to device and execute remotely

**Via WiFi:**
1. Connect to PHANTOM's AP or configure station mode
2. Access web interface at `http://phantom.local`
3. Upload DuckyScript files, trigger execution

## Payload Example

```
REM PHANTOM Payload - Windows Credential Harvest
REM For authorized pentesting only
DELAY 1000
GUI r
DELAY 500
STRING cmd
DELAY 300
ENTER
DELAY 500
STRING echo Payload executed at %date% %time%
ENTER
REM Switch back to stealth mode
STEALTH_OFF
```

## Operating Modes

| Mode | Description | USB Enumeration | Power |
|------|-------------|----------------|-------|
| **Stealth (MSC)** | Default mode — appears as flash drive | Mass Storage | USB bus |
| **HID Keyboard** | Active payload execution | Keyboard + Mouse + Consumer | USB bus |
| **HID + WiFi** | Active with wireless control | Keyboard + Mouse + Consumer + CDC | USB bus |
| **Battery Stealth** | Portable with OLED navigation | Disconnected | LiPo |
| **Battery HID** | Deployed payload execution | Disconnected | LiPo |
| **Sleep** | Ultra-low power standby | Disconnected | LiPo |

## Geofencing Configuration

```json
{
  "geofence": {
    "mode": "whitelist",
    "networks": [
      { "ssid": "PentestLab-5G", "bssid": "AA:BB:CC:DD:EE:FF", "allowed": true },
      { "ssid": "RedTeam-Office", "bssid": "11:22:33:44:55:66", "allowed": true }
    ],
    "fail_action": "block",
    "scan_interval_ms": 30000
  }
}
```

## USB Protocol Details

### Device Descriptors

#### HID Mode (Attack Mode)

| Field | Value |
|-------|-------|
| bDeviceClass | Miscellaneous (0xEF) |
| bDeviceSubClass | IAD (0x02) |
| bDeviceProtocol | Common (0x01) |
| idVendor | 0x1A86 |
| idProduct | 0x5001 |
| bNumConfigurations | 1 |
| Configurations | 3 (Keyboard, Mouse, Consumer) |

#### Keyboard Interface (Interface 0)
- Boot-compatible HID keyboard
- 8-byte report: modifier byte + reserved + 6 keycodes
- 1-byte output report: LED state (NumLock, CapsLock, ScrollLock, etc.)
- EP1 IN (interrupt, 8 bytes, 1ms interval)
- EP4 OUT (interrupt, 1 byte, 1ms interval)

#### Mouse Interface (Interface 1)
- Boot-compatible HID mouse
- 4-byte report: buttons (3 bits) + X + Y + wheel
- EP2 IN (interrupt, 4 bytes, 1ms interval)

#### Consumer Control Interface (Interface 2)
- Media key support (play/pause, volume, scan)
- 2-byte report: consumer keycode
- EP3 IN (interrupt, 2 bytes, 1ms interval)

#### MSC Mode (Stealth Mode)

| Field | Value |
|-------|-------|
| bDeviceClass | Mass Storage (0x08) |
| bDeviceSubClass | SCSI (0x06) |
| bDeviceProtocol | BOT (0x50) |
| idProduct | 0x5002 |
| Endpoints | EP5 IN/OUT (bulk, 64 bytes) |

### Report Structures

```c
// Keyboard Report (8 bytes)
typedef struct __attribute__((packed)) {
    uint8_t modifier;    // Ctrl/Shift/Alt/GUI bits
    uint8_t reserved;    // Must be 0
    uint8_t keycode[6];  // Up to 6 simultaneous keys
} hid_keyboard_report_t;

// Mouse Report (4 bytes)
typedef struct __attribute__((packed)) {
    uint8_t buttons;     // Left/Right/Middle
    int8_t  x;          // X displacement (-127 to +127)
    int8_t  y;          // Y displacement (-127 to +127)
    int8_t  wheel;      // Scroll displacement
} hid_mouse_report_t;

// Consumer Control Report (2 bytes)
typedef struct __attribute__((packed)) {
    uint16_t key;        // Consumer keycode
} hid_consumer_report_t;
```

## DuckyScript Reference

### Standard Commands

| Command | Arguments | Description |
|---------|-----------|-------------|
| `STRING` | text | Type a string of text |
| `DELAY` | ms | Wait for N milliseconds |
| `DEFAULT_DELAY` | ms | Set delay between commands |
| `ENTER` | — | Press Enter key |
| `TAB` | — | Press Tab key |
| `ESCAPE` | — | Press Escape key |
| `SPACE` | — | Press Space key |
| `BACKSPACE` | — | Press Backspace key |
| `CAPSLOCK` | — | Toggle CapsLock |
| `UP` | — | Press Up arrow |
| `DOWN` | — | Press Down arrow |
| `LEFT` | — | Press Left arrow |
| `RIGHT` | — | Press Right arrow |
| `INSERT` | — | Press Insert key |
| `HOME` | — | Press Home key |
| `END` | — | Press End key |
| `PAGEUP` | — | Press Page Up key |
| `PAGEDOWN` | — | Press Page Down key |
| `DELETE` | — | Press Delete key |
| `F1`–`F12` | — | Press function key |
| `GUI` | key | Press GUI (Win/Cmd) + key |
| `SHIFT` | key | Press Shift + key |
| `CTRL` | key | Press Ctrl + key |
| `ALT` | key | Press Alt + key |
| `CTRL-ALT` | key | Press Ctrl+Alt + key |
| `CTRL-SHIFT` | key | Press Ctrl+Shift + key |
| `REPEAT` | n | Repeat previous command N times |
| `HOLD` | key | Hold a key down (doesn't release) |
| `RELEASE` | — | Release all held keys |
| `REM` | text | Comment (ignored during execution) |

### PHANTOM Extensions

| Command | Arguments | Description |
|---------|-----------|-------------|
| `MOUSE_MOVE` | dx dy | Move mouse by dx, dy pixels |
| `MOUSE_CLICK` | left/right/middle | Click mouse button |
| `MOUSE_SCROLL` | n | Scroll mouse wheel by N ticks |
| `CONSUMER` | key | Press consumer/media key |
| `STEALTH_ON` | — | Switch to HID mode |
| `STEALTH_OFF` | — | Switch back to MSC mode |
| `LED_R` | value | Set red LED (0–255 hex) |
| `LED_G` | value | Set green LED (0–255 hex) |
| `LED_B` | value | Set blue LED (0–255 hex) |
| `OLED_LINE` | num text | Display text on OLED line |
| `RANDOM_DELAY` | min max | Random delay between min and max ms |
| `IF_NETWORK` | ssid | Conditional: execute if SSID visible |
| `WAIT_FOR_NETWORK` | ssid | Block until SSID found or timeout |
| `PROFILE` | name | Jump to another payload profile |

### Example Payloads

#### Windows: Open Calculator
```
REM Open Calculator on Windows
DELAY 1000
GUI r
DELAY 500
STRING calc.exe
ENTER
```

#### Linux: Open Terminal
```
REM Open Terminal on Linux
DELAY 500
CTRL ALT t
DELAY 1000
STRING echo "Hello from PHANTOM"
ENTER
```

#### macOS: Spotlight Search
```
REM macOS Spotlight Search
DELAY 500
GUI SPACE
DELAY 500
STRING terminal
DELAY 300
ENTER
```

#### Advanced: Conditional Execution
```
REM Only execute if on approved network
IF_NETWORK PentestLab-5G
DELAY 500
GUI r
DELAY 500
STRING cmd.exe
ENTER
DELAY 1000
STRING whoami
ENTER
```

## Hardware Design Details

### Block Diagram

```
                          ┌──────────────────────────────────────────────┐
                          │              PHANTOM PCB 50×25mm              │
                          │                                              │
  USB-A ──────────────────┤─ D+ D- ──────┐                              │
  Connector               │              │                              │
  (5V, D+, D-)            │     ┌────────┴─────────┐                   │
      │                   │     │   RP2040 MCU      │                   │
      │                   │     │  (Cortex-M0+ x2)  │                   │
      │                   │     │  133MHz, 264KB SRAM│                   │
      ├───────────────────┤     │                    │                   │
      │                   │     │  ┌──────────────┐ │                   │
      │  5V Bus Power     │     │  │ USB 1.1 FS   │─┘                   │
      │                   │     │  │ Device Ctrl   │                     │
      ├───────┬───────────┤     │  └──────────────┘                     │
      │       │           │     │                                        │
      │       │   ┌───────┤     │  ┌─────┐  ┌──────┐  ┌─────┐         │
      │       │   │MCP73831│     │  │SPI0 │  │I2C0  │  │UART0│         │
      │       │   │Charger │     │  │     │  │      │  │     │         │
      │       │   └───┬───┘     │  └──┬──┘  └──┬───┘  └──┬──┘         │
      │       │       │         │     │        │         │              │
      │       │   ┌───┴───┐     │  ┌──┴──┐  ┌─┴──┐  ┌──┴───┐        │
      │       │   │LiPo   │     │  │W25Q128│  │SSD1306│  │ESP32 │        │
      │       │   │500mAh │     │  │16MB  │  │OLED │  │C3-M1│        │
      │       │   │3.7V   │     │  │Flash │  │128×64│  │WiFi │        │
      │       │   └───────┘     │  └─────┘  └─────┘  │+BLE │        │
      │       │                 │                     └──┬───┘        │
      │       │                 │                        │              │
      │   ┌───┴───┐             │  ┌─────────────┐  ┌───┴───┐        │
      │   │3.3V   │             │  │EC11 Rotary  │  │WS2812B│        │
      │   │LDO    │             │  │Encoder      │  │LED    │        │
      │   │RT9013 │             │  │+ Push Btn  │  │Status │        │
      │   └───────┘             │  └─────────────┘  └───────┘        │
      │                        │                                        │
      │                        │  ┌──────────┐                        │
      │                        │  │Kill Switch│──── USB MUX ──┐       │
      │                        │  │(Slide)   │    (TS3A47515)│       │
      │                        │  └──────────┘               │       │
      └────────────────────────┴──────────────────────────────┘       │
                                                                      │
                          ┌───────────────────────────────────────────┘
                          │    (Kill switch disconnects USB data lines)
```

### Power Architecture

```
                    ┌─────────┐
   USB 5V ─────────┤         │
   (VBUS)          │ MCP73831│──── LiPo Charger
                    │  (U3)   │     ┌──────────┐
                    └────┬────┘     │ 3.7V LiPo │
                         │          │  500mAh   │
                    ┌────┴────┐    └─────┬─────┘
                    │  5V Bus  │          │
                    │  Power   │     ┌────┴────┐
                    └────┬────┘     │ Battery │
                         │          │ Monitor │
                    ┌────┴────┐    │ (ADC)   │
                    │ RT9013  │    └─────────┘
                    │ 3.3V LDO│
                    └────┬────┘
                         │
                    ┌────┴────────────────────────────┐
                    │           3.3V Rail                │
                    ├──────────┬───────────┬───────────┤
                    │          │           │           │
               ┌────┴──┐ ┌────┴──┐ ┌─────┴──┐ ┌─────┴──┐
               │RP2040 │ │W25Q128│ │SSD1306 │ │ESP32-C3│
               │  VCC   │ │  VCC  │ │  VCC   │ │  VCC   │
               └───────┘ └───────┘ └────────┘ └────────┘
```

### Flash Memory Map

| Offset | Size | Description |
|--------|------|-------------|
| 0x000000 | 4 KB | Header (magic, version, profile count) |
| 0x001000 | 4 KB | Profile index (128 entries × 32 bytes) |
| 0x002000 | 124 KB | Profile metadata (names, flags, stats) |
| 0x020000 | 15.75 MB | Profile payload data |
| 0xFF0000 | 64 KB | Geofence configuration |
| 0xFFFC00 | 1 KB | WiFi/BLE configuration |
| 0xFFFF00 | 256 B | Device settings |

### Profile Entry Structure

```c
typedef struct {
    uint32_t offset;        // Start offset in flash data area
    uint32_t size;          // Payload size in bytes
    uint32_t flags;         // Profile flags
    char     name[16];      // Null-terminated profile name
    uint32_t crc32;         // Payload CRC32 integrity check
} profile_entry_t;          // 32 bytes per entry

// Profile flags:
//   bit 0: Encrypted payload
//   bit 1: Auto-execute on insertion
//   bit 2: Geofence restricted
//   bit 3: Enabled
```

### GPIO Pin Map (RP2040 QFN-56)

| Pin | Function | Direction | Notes |
|-----|----------|-----------|-------|
| 0-5 | QSPI Flash | — | Boot flash (do not reassign) |
| 6 | ENC_A | Input | Rotary encoder phase A |
| 7 | ENC_B | Input | Rotary encoder phase B |
| 8 | ENC_BTN | Input | Rotary encoder push (active-low) |
| 9 | KILL_SW | Input | Hardware kill switch (active-low) |
| 10 | I2C0_SDA | I/O | SSD1306 OLED data |
| 11 | I2C0_SCL | Output | SSD1306 OLED clock |
| 12 | UART0_TX | Output | ESP32-C3 RX |
| 13 | UART0_RX | Input | ESP32-C3 TX |
| 14 | ESP_RST_N | Output | ESP32-C3 reset (active-low) |
| 15 | WS2812B | Output | Status LED data (PIO0) |
| 16 | SPI0_CSn | Output | W25Q128 chip select |
| 17 | SPI0_SCK | Output | W25Q128 clock |
| 18 | SPI0_MOSI | Output | W25Q8 data to flash |
| 19 | SPI0_MISO | Input | W25Q8 data from flash |
| 20 | USB_MUX | Output | USB mode control (HIGH=HID) |
| 21 | BAT_SENSE | Input | Battery ADC (ADC1) |
| 22 | LED_EN | Output | WS2812B enable |
| 24 | BOOT_BTN | Input | Boot button (active-low) |
| 46-47 | USB_DP/DM | — | USB data (fixed-function) |

## Firmware Build Instructions

### Prerequisites

```bash
# Install ARM toolchain
sudo apt install gcc-arm-none-eabi

# Clone Pico SDK
cd firmware/
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Build
cd ..
make clean
make
```

### Output Files

| File | Description |
|------|-------------|
| `build/phantom.elf` | ELF with debug symbols |
| `build/phantom.bin` | Raw binary |
| `build/phantom.uf2` | UF2 for drag-and-drop programming |
| `build/phantom.hex` | Intel HEX format |

### Flashing

1. **UF2 Bootloader**: Hold BOOT button, plug into USB, drag `phantom.uf2` to the mounted drive
2. **OpenOCD**: `openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "program phantom.elf verify reset exit"`
3. **SWD**: Via RP2040 SWDIO/SWCLK pads on test points

## Mobile App Setup

### Development

```bash
cd app/
npm install
npx react-native run-android   # Android
npx react-native run-ios       # iOS (requires macOS)
```

### Features
- BLE device scanning and connection
- Real-time device status (state, battery, active profile)
- Payload editor with DuckyScript syntax highlighting
- Profile management (create, edit, delete, upload)
- WiFi configuration for geofencing
- Kill switch status monitoring
- Firmware update over BLE

## Security Considerations

### Hardware Security
- **Kill switch** physically disconnects USB data lines via analog switch (TS3A47515)
- **Stealth mode** enumerates as generic mass storage device
- **Encrypted payloads** (AES-128-CTR) stored in SPI flash
- **Boot chain verification** — RP2040 boot2 validates flash signature

### Software Security
- **Geofencing** prevents payload execution outside approved networks
- **Profile CRC32** integrity verification before execution
- **No backdoors** — all functionality requires physical or BLE authentication
- **Configurable auto-execution timeout** — profiles don't auto-run indefinitely

### Recommended Deployment Practices
1. Always enable kill switch when transporting
2. Use stealth mode during approach to target
3. Verify geofencing before engagement
4. Test payloads in isolated VMs before field deployment
5. Maintain audit logs of all engagements
6. Obtain written authorization before any testing

## Troubleshooting

| Issue | Solution |
|-------|---------|
| Device not recognized | Check kill switch position, try different USB port |
| OLED blank | Verify I2C connections (SDA=GPIO10, SCL=GPIO11) |
| WiFi not connecting | Check UART wiring (TX=GPIO12, RX=GPIO13) |
| Encoder not responding | Check GPIO 6-8 connections, verify 10k pull-ups |
| Flash write failures | Verify SPI connections, check CS pin (GPIO16) |
| Battery not charging | Check MCP73831 STAT pin, verify LiPo connection |
| LED not working | Check WS2812B data (GPIO15) and enable (GPIO22) |

## Disclaimer

This device is a **security research tool** designed exclusively for:

- **Authorized penetration testing** with written permission from system owners
- **Security education** in classroom and lab environments
- **Defensive security research** to understand and mitigate HID injection attacks
- **Red team assessments** conducted under proper rules of engagement

**Illegal use of this device to access computer systems without authorization is a crime.** The developers assume no responsibility for misuse. Always comply with applicable laws and regulations.

## License

| Component | License |
|-----------|---------|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| React Native companion app | MIT |

## Acknowledgments

- DuckyScript language by Hak5 (used under compatibility, not affiliation)
- RP2040 USB stack inspired by TinyUSB and Pico-SDK
- ESP32-C3 AT firmware by Espressif Systems
- SSD1306 driver based on Adafruit SSD1306 library
- WS2812B PIO program derived from Pico-SDK example

---

*PHANTOM — Hide in plain sight. Strike with precision. Stay within the law.*