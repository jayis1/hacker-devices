# PHANTOM — Phase 4: Software Stack

## 1. Memory Map & MMIO

### 1.1 RP2040 Address Map

| Address Range | Size | Peripheral | Notes |
|--------------|------|-----------|-------|
| 0x00000000 | 16 MB | XIP (QSPI Flash) | Boot flash, execute-in-place |
| 0x10000000 | 16 MB | XIP (QSPI Flash) | Alias, non-cached |
| 0x20000000 | 264 KB | SRAM | Main data memory |
| 0x40000000 | — | APB Peripherals | See below |
| 0x50000000 | — | AHB Peripherals | DMA, USB, QSPI |
| 0xE0000000 | — | Cortex-M0+ Private | SCB, SysTick, NVIC |

### 1.2 APB Peripheral Base Addresses

| Peripheral | Base Address | Offset | Notes |
|-----------|-------------|--------|-------|
| TIMER | 0x40054000 | 0x54000 | Hardware timer |
| CLOCKS | 0x40008000 | 0x08000 | Clock control |
| PADS_BANK_0 | 0x4001C000 | 0x1C000 | GPIO pad control |
| IO_BANK_0 | 0x40014000 | 0x14000 | GPIO function select |
| SPI0 | 0x4003C000 | 0x3C000 | Payload flash SPI |
| SPI1 | 0x40040000 | 0x40000 | Reserved SPI |
| UART0 | 0x40034000 | 0x34000 | ESP32-C3 UART |
| I2C0 | 0x40044000 | 0x44000 | OLED display |
| ADC | 0x4004C000 | 0x4C000 | Battery voltage |
| PWM | 0x40050000 | 0x50000 | PWM slices |
| PIO0 | 0x50200000 | — | Programmable IO (WS2812B) |
| PIO1 | 0x50300000 | — | Programmable IO |
| USB | 0x50110000 | — | USB FS device controller |
| DMA | 0x50000000 | — | DMA controller |
| XIP_CTRL | 0x50000000 | — | XIP flash controller |
| XIP_SSI | 0x5000C000 | — | QSPI controller |

### 1.3 Key Register Definitions

```c
// Clock registers (CLOCKS_BASE = 0x40008000)
#define CLOCKS_CLK_REF_CTRL      (CLOCKS_BASE + 0x00)  // Reference clock control
#define CLOCKS_CLK_SYS_CTRL      (CLOCKS_BASE + 0x04)  // System clock control
#define CLOCKS_CLK_PERI_CTRL    (CLOCKS_BASE + 0x08)  // Peripheral clock control
#define CLOCKS_CLK_USB_CTRL     (CLOCKS_BASE + 0x0C)  // USB clock control
#define CLOCKS_CLK_ADC_CTRL     (CLOCKS_BASE + 0x10)  // ADC clock control

// Reset registers
#define RESETS_RESET_CTRL        (RESETS_BASE + 0x00)  // Reset control
#define RESETS_RESET_DONE        (RESETS_BASE + 0x08)  // Reset done status

// GPIO registers (IO_BANK_0_BASE = 0x40014000)
#define GPIO_CTRL(n)             (IO_BANK_0_BASE + 0x04 + (n * 8))  // GPIO function select
#define GPIO_OUT                 (SIO_BASE + 0x010)  // GPIO output value
#define GPIO_OE                  (SIO_BASE + 0x020)  // GPIO output enable
#define GPIO_IN                  (SIO_BASE + 0x004)  // GPIO input value
#define GPIO_INTE0               (IO_BANK_0_BASE + 0xF0)  // Interrupt enable

// Pad control registers
#define PAD_CTRL(n)              (PADS_BANK_0_BASE + 0x04 + (n * 4))

// SPI0 registers (SPI0_BASE = 0x4003C000)
#define SPI0_SSPCR0              (SPI0_BASE + 0x00)  // Control register 0
#define SPI0_SSPCR1              (SPI0_BASE + 0x04)  // Control register 1
#define SPI0_SSPDR               (SPI0_BASE + 0x08)  // Data register
#define SPI0_SSPSR               (SPI0_BASE + 0x0C)  // Status register
#define SPI0_SSPCPSR             (SPI0_BASE + 0x10)  // Clock prescale
#define SPI0_SSPDMACR            (SPI0_BASE + 0x14)  // DMA control

// UART0 registers (UART0_BASE = 0x40034000)
#define UART0_DR                 (UART0_BASE + 0x00)  // Data register
#define UART0_RSR                (UART0_BASE + 0x04)  // Receive status
#define UART0_FR                 (UART0_BASE + 0x18)  // Flag register
#define UART0_IBRD               (UART0_BASE + 0x24)  // Integer baud rate
#define UART0_FBRD               (UART0_BASE + 0x28)  // Fractional baud rate
#define UART0_LCR_H             (UART0_BASE + 0x2C)  // Line control
#define UART0_CR                 (UART0_BASE + 0x30)  // Control register
#define UART0_IFLS               (UART0_BASE + 0x34)  // FIFO level select
#define UART0_IMSC               (UART0_BASE + 0x38)  // Interrupt mask
#define UART0_ICR                (UART0_BASE + 0x44)  // Interrupt clear

// I2C0 registers (I2C0_BASE = 0x40044000)
#define I2C0_IC_CON              (I2C0_BASE + 0x00)  // Control register
#define I2C0_IC_TAR              (I2C0_BASE + 0x04)  // Target address
#define I2C0_IC_DATA_CMD         (I2C0_BASE + 0x10)  // Data command
#define I2C0_IC_STATUS           (I2C0_BASE + 0x18)  // Status
#define I2C0_IC_FS_SCL_HCNT     (I2C0_BASE + 0x1C)  // Fast mode SCL high count
#define I2C0_IC_FS_SCL_LCNT     (I2C0_BASE + 0x20)  // Fast mode SCL low count
#define I2C0_IC_ENABLE           (I2C0_BASE + 0x6C)  // Enable

// ADC registers (ADC_BASE = 0x4004C000)
#define ADC_CS                   (ADC_BASE + 0x00)  // Control and status
#define ADC_RESULT               (ADC_BASE + 0x04)  // Conversion result
#define ADC_FCS                  (ADC_BASE + 0x08)  // FIFO control
#define ADC_FIFO                 (ADC_BASE + 0x0C)  // FIFO data

// USB registers (USB_BASE = 0x50110000)
#define USB_MAIN_CTRL            (USB_BASE + 0x40)  // Main control
#define USB_SIE_CTRL             (USB_BASE + 0x50)  // SIE control
#define USB_SIE_STATUS           (USB_BASE + 0x54)  // SIE status
#define USB_BUFF_STATUS          (USB_BASE + 0x58)  // Buffer status
#define USB_BUFF_CPU_SHOULD_HANDLE (USB_BASE + 0x5C)
#define USB_EP0_BUF_CTRL         (USB_BASE + 0x80)  // EP0 buffer control
#define USB_EP_BUF_CTRL(n)       (USB_BASE + 0x80 + (n * 4))  // EP buffer control
#define USB_INT_EP_CTRL          (USB_BASE + 0x100)  // Interrupt endpoint control
#define USB_INTS                 (USB_BASE + 0x108)  // Interrupt status
#define USB_INTE                 (USB_BASE + 0x10C)  // Interrupt enable
#define USB_INTF                 (USB_BASE + 0x110)  // Interrupt force
#define USB_SETUP_PACKET         (USB_BASE + 0x114)  // Setup packet buffer
```

## 2. Clock Initialization

### 2.1 Clock Tree

```
12 MHz Crystal (Y1)
  │
  ├──► PLL SYS (125 MHz) ──► System Clock (133 MHz)
  │    (PLL setup: refdiv=1, vco_freq=1500 MHz, postdiv1=6, postdiv2=7)
  │
  └──► PLL USB (48 MHz) ──► USB Clock
       (PLL setup: refdiv=1, vco_freq=1440 MHz, postdiv1=5, postdiv2=30)
```

### 2.2 Clock Configuration

```c
void clocks_init(void) {
    // Step 1: Configure XOSC (12 MHz crystal)
    xosc_init();  // Enable 12 MHz crystal oscillator
    
    // Step 2: Configure PLL_SYS (133 MHz for system clock)
    // VCO = 12 MHz * 125 = 1500 MHz
    // SYS = 1500 / 6 / 2 = 125 MHz (close enough, we'll use 133)
    pll_init(PLL_SYS, 1, 1500 * MHZ, 6, 2);
    
    // Step 3: Configure PLL_USB (48 MHz for USB)
    // VCO = 12 MHz * 120 = 1440 MHz
    // USB = 1440 / 5 / 30 = 48 MHz (note: divided by 2 in postdiv)
    pll_init(PLL_USB, 1, 1440 * MHZ, 5, 30);
    
    // Step 4: Switch system clock to PLL_SYS
    // clk_sys = PLL_SYS output (133 MHz)
    CLOCKS->CLK_SYS_CTRL = (CLOCKS->CLK_SYS_CTRL & ~0x03) | 0x01;  // Source = PLL_SYS
    
    // Step 5: Configure peripheral clock
    // clk_peri = clk_sys (133 MHz)
    CLOCKS->CLK_PERI_CTRL = 0x01;  // Enable, source = clk_sys
    
    // Step 6: Configure USB clock
    // clk_usb = PLL_USB output (48 MHz)
    CLOCKS->CLK_USB_CTRL = (0x01 << 5) | 0x02;  // Source = PLL_USB
    
    // Step 7: Configure ADC clock
    // clk_adc = PLL_USB (48 MHz)
    CLOCKS->CLK_ADC_CTRL = (0x01 << 5) | 0x02;
    
    // Step 8: Configure reference clock
    // clk_ref = XOSC (12 MHz)
    CLOCKS->CLK_REF_CTRL = 0x02;  // Source = XOSC
    
    // Step 9: Enable clock to peripherals
    RESETS->RESET = ~(
        (1 << RESETS_RESET_SPI0)   |  // SPI0 for payload flash
        (1 << RESETS_RESET_UART0)  |  // UART0 for ESP32-C3
        (1 << RESETS_RESET_I2C0)   |  // I2C for OLED
        (1 << RESETS_RESET_ADC)    |  // ADC for battery
        (1 << RESETS_RESET_USB)    |  // USB device
        (1 << RESETS_RESET_PWM)    |  // PWM for buzzer
        (1 << RESETS_RESET_PIO0)   |  // PIO0 for WS2812B
        (1 << RESETS_RESET_PIO1)   |  // PIO1 reserved
        (1 << RESETS_RESET_IO_BANK0)  // GPIO
    );
    
    // Wait for resets to complete
    while ((RESETS->RESET_DONE & 0x1FF) != 0x1FF);
}
```

## 3. USB Stack Architecture

### 3.1 USB Device Controller (RP2040 Native)

The RP2040 includes a native USB 1.1 Full-Speed device controller with 16 endpoints (15 IN + 15 OUT). The USB stack provides:

- **Endpoint 0**: Control (SETUP) — device enumeration, configuration
- **Endpoint 1 IN**: HID Keyboard — keystroke reports
- **Endpoint 2 IN**: HID Mouse — mouse movement/click reports
- **Endpoint 3 IN**: HID Consumer Control — media key reports
- **Endpoint 4 OUT**: HID LED — keyboard LED state (Caps Lock, etc.)
- **Endpoint 5 IN**: Mass Storage (BOT) — CBW/CSW/DA TA transfers (stealth mode)

### 3.2 USB Mode Switching

The firmware supports two USB configurations:

**Configuration 1: Mass Storage (Stealth Mode)**
- Interface 0: Mass Storage (BOT, SCSI transparent)
- Endpoint 5 IN/OUT for bulk data
- Appears as a standard USB flash drive
- Used for payload file transfer and normal operation

**Configuration 2: HID Composite (Attack Mode)**
- Interface 0: HID Keyboard (boot protocol)
- Interface 1: HID Mouse (boot protocol)
- Interface 2: HID Consumer Control
- Endpoint 1 IN: Keyboard reports
- Endpoint 2 IN: Mouse reports
- Endpoint 3 IN: Consumer reports
- Endpoint 4 OUT: Keyboard LED state

Mode switching is triggered by:
1. Rotary encoder press (physical)
2. BLE command from companion app
3. WiFi HTTP API
4. DuckyScript `STEALTH_ON` command
5. Automatic timer

### 3.3 USB Descriptor Summary

```
Device Descriptor:
  bDeviceClass: 0xEF (Miscellaneous)
  bDeviceSubClass: 0x02 (Common Class)
  bDeviceProtocol: 0x01 (Interface Association)
  idVendor: 0x1A86 (custom)
  idProduct: 0xPH01 (PHANTOM-01)
  bcdDevice: 0x0100
  iManufacturer: "Hacker Devices"
  iProduct: "PHANTOM HID Injector"
  iSerial: "<unique_id>"
  bNumConfigurations: 2

Configuration 1 (Stealth):
  MSC interface, 1 bulk endpoint pair, max power 100mA

Configuration 2 (HID):
  Composite HID (Keyboard + Mouse + Consumer), 4 endpoints, max power 100mA
```

### 3.4 USB Interrupt Handler

```c
void usb_isr(void) {
    uint32_t ints = USB->INTS & USB->INTE;
    
    // Setup packet received
    if (ints & USB_INTS_SETUP_REQ) {
        usb_handle_setup();
        USB->INTS = USB_INTS_SETUP_REQ;  // Clear
    }
    
    // Buffer status changed
    if (ints & USB_INTS_BUFF_STATUS) {
        usb_handle_buffer_status();
        USB->INTS = USB_INTS_BUFF_STATUS;  // Clear
    }
    
    // Bus reset
    if (ints & USB_INTS_BUS_RESET) {
        usb_handle_reset();
        USB->INTS = USB_INTS_BUS_RESET;  // Clear
    }
    
    // Suspend detected
    if (ints & USB_INTS_DEV_SUSPEND) {
        usb_handle_suspend();
        USB->INTS = USB_INTS_DEV_SUSPEND;  // Clear
    }
    
    // Resume detected
    if (ints & USB_INTS_DEV_RESUME) {
        usb_handle_resume();
        USB->INTS = USB_INTS_DEV_RESUME;  // Clear
    }
}
```

## 4. DuckyScript Parser & Payload Engine

### 4.1 Payload Storage Format

```
SPI Flash Layout (W25Q128JVSIQ, 16 MB):

Offset      Size        Content
────────    ─────────   ──────────────────
0x000000    4 KB        Header (magic, version, profile count)
0x001000    4 KB        Profile index table (128 entries × 32 bytes)
0x002000    124 KB      Profile metadata (128 profiles × 960 bytes)
0x020000    15.75 MB    Profile data (variable size, chained)
0xFF0000    64 KB        Geofence configuration
0xFFFC00    1 KB         WiFi configuration
0xFFFF00    256 bytes    Device settings
0xFFFFF0    16 bytes     Boot flags

Profile Index Entry (32 bytes):
  [0:3]   Offset (uint32_t)     — Start offset in profile data area
  [4:7]   Size (uint32_t)      — Payload size in bytes
  [8:11]  Flags (uint32_t)     — Encrypted, auto-execute, geofenced
  [12:27] Name (char[16])      — Null-terminated profile name
  [28:31] CRC32 (uint32_t)     — Payload integrity check
```

### 4.2 DuckyScript Parser State Machine

```
┌───────────┐
│  IDLE     │◄─────────────────────────────┐
└─────┬─────┘                              │
      │ Line read                           │
      ▼                                    │
┌───────────┐                              │
│  PARSE    │── Error ───► ┌──────────┐    │
│  LINE     │             │  ERROR   │    │
└─────┬─────┘             └──────────┘    │
      │ Success                           │
      ▼                                    │
┌───────────┐                              │
│  EXECUTE  │── Complete ────────────────┘│
│  COMMAND  │                              │
└─────┬─────┘                              │
      │ Need more input                     │
      │ (STRING, HOLD, etc.)                │
      ▼                                    │
┌───────────┐                              │
│  QUEUE    │──────────────────────────────┘
│  NEXT     │
└───────────┘
```

### 4.3 Supported DuckyScript Commands

| Command | Syntax | HID Action | Notes |
|---------|--------|-----------|-------|
| STRING | `STRING text` | Type each character | US keyboard layout |
| DELAY | `DELAY n` | Wait n ms | Timer-based |
| DEFAULT_DELAY | `DEFAULT_DELAY n` | Set inter-command delay | Applied between all commands |
| REM | `REM comment` | No action | Comment line |
| GUI | `GUI key` | Mod+key | Windows/Super key |
| SHIFT | `SHIFT key` | Mod+key | Shift modifier |
| CTRL | `CTRL key` | Mod+key | Control modifier |
| ALT | `ALT key` | Mod+key | Alt modifier |
| CTRL-ALT | `CTRL-ALT key` | Mod+Mod+key | Ctrl+Alt combo |
| CTRL-SHIFT | `CTRL-SHIFT key` | Mod+Mod+key | Ctrl+Shift combo |
| UP | `UP` | Arrow key up | |
| DOWN | `DOWN` | Arrow key down | |
| LEFT | `LEFT` | Arrow key left | |
| RIGHT | `RIGHT` | Arrow key right | |
| ENTER | `ENTER` | Return key | |
| TAB | `TAB` | Tab key | |
| ESCAPE | `ESCAPE` | Escape key | |
| SPACE | `SPACE` | Space bar | |
| BACKSPACE | `BACKSPACE` | Delete key | |
| CAPSLOCK | `CAPSLOCK` | Caps Lock | |
| REPEAT | `REPEAT n` | Repeat last command | n = count |
| HOLD | `HOLD key` | Press and hold key | Until RELEASE |
| RELEASE | `RELEASE key` | Release held key | |
| MOUSE_MOVE | `MOUSE_MOVE dx dy` | Move cursor | -127 to +127 |
| MOUSE_CLICK | `MOUSE_CLICK left\|right\|middle` | Click button | |
| MOUSE_SCROLL | `MOUSE_SCROLL n` | Scroll wheel | Positive=up |
| CONSUMER | `CONSUMER key` | Consumer key | Play/pause, vol, etc. |
| PROFILE | `PROFILE name` | Jump to profile | Chain profiles |
| IF_NETWORK | `IF_NETWORK ssid` | Conditional block | Check WiFi scan |
| WAIT_FOR_NETWORK | `WAIT_FOR_NETWORK ssid` | Block until found | Timeout 30s |
| STEALTH_ON | `STEALTH_ON` | Switch to HID mode | USB re-enumeration |
| STEALTH_OFF | `STEALTH_OFF` | Switch to MSC mode | USB re-enumeration |
| LED_R/G/B | `LED_R/G/B hex` | Set LED color | WS2812B |
| OLED_LINE | `OLED_LINE n text` | Display text | Line 0-7 |
| RANDOM_DELAY | `RANDOM_DELAY min max` | Random delay | min..max ms |

### 4.4 Payload Execution Engine

The payload engine runs on RP2040 Core 0:

```c
typedef enum {
    PHANTOM_STATE_STEALTH,   // USB MSC mode
    PHANTOM_STATE_HID,       // USB HID mode
    PHANTOM_STATE_EXECUTING,  // Active payload execution
    PHANTOM_STATE_GEOFENCE,  // Checking geofence
    PHANTOM_STATE_ERROR,     // Error state
    PHANTOM_STATE_SLEEP,     // Low power sleep
} phantom_state_t;

typedef struct {
    phantom_state_t state;
    uint8_t current_profile;
    uint32_t default_delay_ms;
    uint32_t last_command_hash;
    uint8_t held_keys[8];    // Currently held keys
    uint8_t held_key_count;
    bool geofence_approved;
    bool kill_switch_active;
} phantom_engine_t;
```

### 4.5 Core 0 / Core 1 Division

**Core 0 (Main Loop):**
- Profile selection (rotary encoder + OLED)
- DuckyScript parser and command execution
- Profile management (load, save, delete)
- WiFi/BLE command processing

**Core 1 (USB & Display):**
- USB device controller management
- HID report generation
- USB enumeration and mode switching
- SSD1306 OLED refresh (30 fps)

## 5. WiFi Bridge (ESP32-C3)

### 5.1 AT Command Interface

The ESP32-C3 runs Espressif's AT firmware and communicates with the RP2040 via UART at 115200 baud:

| Command | Description | Response |
|---------|-------------|----------|
| `AT` | Test connection | `OK` |
| `AT+RST` | Reset module | `ready` |
| `AT+CWMODE=1` | Station mode | `OK` |
| `AT+CWMODE=2` | SoftAP mode | `OK` |
| `AT+CWMODE=3` | Station+AP mode | `OK` |
| `AT+CWJAP="ssid","pass"` | Connect to AP | `WIFI CONNECTED` / `OK` |
| `AT+CIFSR` | Get IP address | IP info |
| `AT+CIPSTART="TCP","ip",port` | Open TCP connection | `OK` / `CONNECT` |
| `AT+CIPSEND=n` | Send n bytes | `>` (data prompt) |
| `AT+CIPCLOSE` | Close connection | `OK` |
| `AT+CIPSERVER=1,port` | Start TCP server | `OK` |
| `AT+CWLAPOPT=0,0,-1,-1,0` | Set scan params | `OK` |
| `AT+CWLAP` | Scan for APs | AP list |

### 5.2 Custom AT Extensions

The RP2040 firmware adds custom commands via the ESP32-C3 AT passthrough:

| Command | Description | Response |
|---------|-------------|----------|
| `PHANTOM+STATUS` | Get device status | JSON status |
| `PHANTOM+EXECUTE=n` | Execute profile n | `OK` or `ERROR` |
| `PHANTOM+STEALTH=ON\|OFF` | Switch USB mode | `OK` |
| `PHANTOM+GEOFENCE=CHECK` | Check current network | JSON result |
| `PHANTOM+PROFILES` | List profiles | JSON list |
| `PHANTOM+UPLOAD=start` | Begin payload upload | `READY` |
| `PHANTOM+UPLOAD=data` | Transfer payload data | `OK` |
| `PHANTOM+UPLOAD=end` | End payload upload | `OK` or `CRC ERROR` |

### 5.3 BLE Service Architecture

```
PHANTOM BLE Services:

Service: PHANTOM Control (0xPH01)
  Characteristic: Profile List (0xPH10)
    - Read: Get profile list
    - Notify: Profile list changed
  Characteristic: Execute (0xPH11)
    - Write: Execute profile by ID
    - Notify: Execution status
  Characteristic: Status (0xPH12)
    - Read: Get current state
    - Notify: State changed

Service: PHANTOM Data (0xPH02)
  Characteristic: Upload (0xPH20)
    - Write: Upload payload data (chunked)
    - Notify: Upload progress
  Characteristic: Download (0xPH21)
    - Read: Download payload data (chunked)
  Characteristic: Log (0xPH22)
    - Read: Get execution log
    - Notify: New log entry

Service: PHANTOM Config (0xPH03)
  Characteristic: WiFi Config (0xPH30)
    - Read/Write: WiFi SSID/password
  Characteristic: Geofence (0xPH31)
    - Read/Write: Geofence rules
  Characteristic: Device Settings (0xPH32)
    - Read/Write: General settings
  Characteristic: Battery (0xPH33)
    - Read: Battery voltage/level
    - Notify: Battery low warning

Service: Device Information (0x180A)
  Characteristic: Manufacturer Name (0x2A29)
    - Read: "Hacker Devices"
  Characteristic: Model Number (0x2A24)
    - Read: "PHANTOM-01"
  Characteristic: Firmware Revision (0x2A26)
    - Read: "1.0.0"
  Characteristic: Serial Number (0x2A25)
    - Read: <unique_id>
```

## 6. Boot Sequence

### 6.1 Boot Flow

```
Power-On / Reset
     │
     ▼
RP2040 Boot ROM (16KB, factory)
     │
     ├── Check FLASH_QSPI for valid image
     │   ├── Valid? ──► Load and execute from XIP
     │   └── Invalid? ──► USB BOOTSEL mode
     │
     ▼
Boot2 (Stage 2, 256 bytes in flash)
     │
     ├── Initialize QSPI flash for XIP
     ├── Configure SSI for fast access
     │
     ▼
Main Firmware Entry
     │
     ├── 1. clocks_init()         — Configure PLLs, system clock
     ├── 2. gpio_init()           — Configure all GPIO pins
     ├── 3. spi_flash_init()      — Initialize W25Q128 payload flash
     ├── 4. uart_init()           — Configure UART0 for ESP32-C3
     ├── 5. i2c_init()            — Configure I2C0 for OLED
     ├── 6. adc_init()            — Configure ADC for battery
     ├── 7. usb_init()            — Configure USB device controller
     ├── 8. oled_init()           — Initialize SSD1306 display
     ├── 9. encoder_init()        — Configure rotary encoder
     ├── 10. wifi_init()          — Reset and configure ESP32-C3
     ├── 11. profiles_load()      — Load profile index from flash
     ├── 12. geofence_load()      — Load geofence configuration
     ├── 13. display_splash()     — Show boot splash on OLED
     │
     ▼
Main Loop (Core 0)
     │
     ├── Read kill switch (GPIO9)
     │   ├── Kill mode? ──► Disable USB data, show warning
     │   └── Normal? ──► Continue
     │
     ├── Read USB mode (stealth or HID)
     │   ├── Stealth: USB MSC active, show profile list
     │   └── HID: USB HID active, ready for execution
     │
     ├── Process rotary encoder input
     │   ├── Scroll: Change selected profile
     │   └── Press: Execute selected profile
     │
     ├── Process BLE commands (from ESP32-C3)
     │   ├── List profiles
     │   ├── Upload payload
     │   ├── Execute profile
     │   └── Configure settings
     │
     ├── Process WiFi commands
     │   └── HTTP API requests
     │
     ├── Update OLED display
     │
     └── Loop
```

### 6.2 Core 1 Initialization

```c
void core1_entry(void) {
    // Core 1 handles USB and display
    
    // 1. Initialize USB device controller
    usb_device_init();
    
    // 2. Start USB enumeration (stealth mode by default)
    usb_enumerate_msc();
    
    // 3. Main loop
    while (1) {
        // Handle USB interrupts
        usb_task();
        
        // Refresh OLED display (30 fps)
        oled_refresh();
        
        // Process HID reports if in HID mode
        if (engine.state == PHANTOM_STATE_HID) {
            hid_task();
        }
        
        // Small delay to prevent watchdog
        busy_wait_us(100);
    }
}
```

## 7. OLED Display Driver

### 7.1 SSD1306 Command Set

```c
// SSD1306 I2C commands
#define SSD1306_SET_DISP_OFF         0xAE
#define SSD1306_SET_DISP_ON          0xAF
#define SSD1306_SET_MUX_RATIO        0xA8
#define SSD1306_SET_DISP_OFFSET      0xD3
#define SSD1306_SET_DISP_START_LINE  0x40
#define SSD1306_SET_SEG_REMAP        0xA1
#define SSD1306_SET_COM_SCAN_DIR     0xC8
#define SSD1306_SET_COM_PIN_CFG      0xDA
#define SSD1306_SET_CONTRAST         0x81
#define SSD1306_SET_PRECHARGE        0xD9
#define SSD1306_SET_VCOM_DESEL       0xDB
#define SSD1306_SET_ENTIRE_ON        0xA5
#define SSD1306_SET_NORM_INV         0xA6
#define SSD1306_SET_CHARGE_PUMP      0x8D
#define SSD1306_SET_MEM_ADDR_MODE    0x20
#define SSD1306_SET_COL_ADDR         0x21
#define SSD1306_SET_PAGE_ADDR        0x22

// Display configuration
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES    8  // 64/8 = 8 pages
#define SSD1306_I2C_ADDR 0x3C

// Font: 6x8 monospaced (128 chars)
extern const uint8_t font_6x8[128][6];

// Framebuffer in SRAM
static uint8_t framebuffer[SSD1306_WIDTH * SSD1306_PAGES];
```

### 7.2 Display Modes

| Mode | Content | Update Rate |
|------|---------|-------------|
| Boot splash | "PHANTOM" logo, version | Once |
| Profile select | Profile list, encoder highlight | 30 fps |
| Executing | Progress bar, current command | 30 fps |
| Error | Error message, description | Once |
| Stealth | "USB Drive" (off in deep stealth) | Once |
| Config | WiFi settings, geofence config | On change |
| Battery | Battery icon, voltage | 1 fps |

## 8. Rotary Encoder Driver

### 8.1 EC11 Quadrature Decode

```c
// Rotary encoder state machine
typedef struct {
    uint8_t last_state;      // Previous AB state
    int32_t position;         // Current position (signed)
    int32_t velocity;        // Rotations per second
    bool button_pressed;     // Current button state
    bool button_long_press;  // Long press detected (>2s)
    absolute_time_t press_time;  // Button press timestamp
} encoder_t;

// GPIO interrupt handler for encoder
void encoder_gpio_isr(uint gpio, uint32_t events) {
    static const int8_t enc_table[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    
    uint8_t new_state = (gpio_get(ENC_A) << 1) | gpio_get(ENC_B);
    encoder.last_state = (encoder.last_state << 2) | new_state;
    encoder.position += enc_table[encoder.last_state & 0x0F];
}

// Timer-based button handler
void encoder_button_task(void) {
    bool current = !gpio_get(ENC_BTN);  // Active-low
    
    if (current && !encoder.button_pressed) {
        encoder.press_time = get_absolute_time();
        encoder.button_pressed = true;
    } else if (!current && encoder.button_pressed) {
        int64_t duration = absolute_time_diff_us(encoder.press_time, get_absolute_time());
        if (duration > 2000000) {  // 2 seconds
            encoder.button_long_press = true;
        }
        encoder.button_pressed = false;
    }
}
```

## 9. Flash Storage Driver

### 9.1 W25Q128JVSIQ Operations

```c
// Flash commands
#define W25Q_CMD_READ_DATA        0x03
#define W25Q_CMD_READ_FAST        0x0B
#define W25Q_CMD_PAGE_PROGRAM     0x02
#define W25Q_CMD_SECTOR_ERASE    0x20
#define W25Q_CMD_BLOCK_ERASE_32K 0x52
#define W25Q_CMD_BLOCK_ERASE_64K 0xD8
#define W25Q_CMD_CHIP_ERASE      0xC7
#define W25Q_CMD_WRITE_ENABLE    0x06
#define W25Q_CMD_WRITE_DISABLE   0x04
#define W25Q_CMD_READ_STATUS_1   0x05
#define W25Q_CMD_READ_STATUS_2   0x35
#define W25Q_CMD_READ_STATUS_3   0x15
#define W25Q_CMD_READ_JEDEC_ID  0x9F

// Flash layout constants
#define FLASH_SECTOR_SIZE    4096   // 4 KB
#define FLASH_BLOCK_SIZE     65536  // 64 KB
#define FLASH_PAGE_SIZE      256    // 256 bytes
#define FLASH_TOTAL_SIZE     16777216  // 16 MB

// Profile storage area starts at 128 KB
#define PROFILE_INDEX_OFFSET   0x001000
#define PROFILE_META_OFFSET    0x002000
#define PROFILE_DATA_OFFSET    0x020000
```

## 10. Build System

### 10.1 Makefile Structure

```makefile
# PHANTOM Firmware Build System

PICO_SDK_PATH ?= ./pico-sdk
CROSS_COMPILE ?= arm-none-eabi-

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size

CFLAGS = -O2 -Wall -Wextra -Werror \
         -mcpu=cortex-m0plus -mthumb \
         -ffreestanding -nostdlib \
         -I$(PICO_SDK_PATH)/src/common \
         -I. -Idrivers

LDFLAGS = -T phantom.ld -nostdlib -specs=nosys.specs

SRCS = main.c \
       drivers/flash_storage.c \
       drivers/usb_hid.c \
       drivers/wifi_bridge.c

OBJS = $(SRCS:.c=.o)

TARGET = phantom

all: $(TARGET).hex $(TARGET).uf2

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).uf2: $(TARGET).elf
	pico-tool uf2 convert $< $@

$(TARGET).elf: $(OBJS) phantom.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

flash: $(TARGET).elf
	openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
		-c "program $< verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).uf2

.PHONY: all flash clean
```

### 10.2 Linker Script

```ld
/* phantom.ld - RP2040 Linker Script */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x10000000, LENGTH = 16M
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 264K
}

SECTIONS
{
    .text : {
        *(.boot2)        /* Boot stage 2 must be first */
        *(.text*)
        *(.rodata*)
    } > FLASH

    .data : {
        *(.data*)
    } > SRAM AT > FLASH

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > SRAM

    .stack (NOLOAD) : {
        . = ALIGN(8);
        __stack_top = . + 0x2000;  /* 8KB stack per core */
    } > SRAM
}
```

## 11. Security Features

### 11.1 Payload Encryption

All stored payloads are encrypted with AES-128-CTR:
- Key derived from: RP2040 UID (8 bytes) + user passphrase (variable)
- IV generated from profile index + sector offset
- Encryption/decryption happens in SRAM — plaintext never written to flash
- Failed decryption (wrong passphrase) results in 5-second lockout

### 11.2 Firmware Integrity

- Boot2 verifies main firmware CRC32 before execution
- Flash write protection enabled for boot2 region
- Firmware update only via signed UF2 files
- Recovery mode via BOOTSEL button (holds GPIO24 low at reset)

### 11.3 Kill Switch

The hardware kill switch (SW1) physically disconnects USB D+/D- lines via the TS3USB221A analog switch. Software cannot override this:
- GPIO9 reads kill switch state
- When active: firmware disables USB HID engine, displays "KILL MODE" on OLED
- USB power (VBUS) remains connected for battery charging
- No data can be transmitted regardless of software state