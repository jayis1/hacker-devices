# Phase 4: Software Stack — CAN Creeper

## 4.0 Software Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                    CAN Creeper Firmware Stack                     │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  Application Layer (main.c)                                │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │  │
│  │  │ Frame    │  │ Injection│  │ BLE/USB  │  │ CLI      │   │  │
│  │  │ Logger   │  │ Engine   │  │ Protocol │  │ Console  │   │  │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │  │
│  └───────┼─────────────┼─────────────┼─────────────┼─────────┘  │
│          │             │             │             │              │
│  ┌───────┼─────────────┼─────────────┼─────────────┼─────────┐  │
│  │  Hardware Abstraction Layer (HAL)                          │  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   │  │
│  │  │ CAN  │ │ QSPI │ │ BLE  │ │ USB  │ │ OLED │ │ GPIO │   │  │
│  │  │ FD   │ │ PSRAM│ │ NUS  │ │ CDC  │ │ Disp │ │ Ctrl │   │  │
│  │  │ Drv  │ │ Drv  │ │ Drv  │ │ Drv  │ │ Drv  │ │ Drv  │   │  │
│  │  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘   │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  nRF5 SDK 17.1.0                                           │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │  │
│  │  │ nrf_drv  │ │ nrf_ble  │ │ nrf_usbd │ │ nrf_qspi │      │  │
│  │  │ _spim    │ │ _nus     │ │ _cdc_acm │ │          │      │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  SoftDevice S140 7.3.0 (BLE 5.2 Protocol Stack)            │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │  │
│  │  │ GAP      │ │ GATT     │ │ L2CAP    │ │ Link     │      │  │
│  │  │ (Central │ │ (Server) │ │ (CoC)    │ │ Layer    │      │  │
│  │  │ +Periph) │ │          │ │          │ │          │      │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  ARM Cortex-M4F Hardware (nRF52840)                        │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │  │
│  │  │ NVIC     │ │ SysTick  │ │ MPU      │ │ FPU      │      │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │  │
│  └────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
```

## 4.1 Boot Strategy

### Boot Sequence

```
Power-On Reset (POR)
  │
  ▼
nRF52840 Boot ROM (hardwired)
  │  - Checks APPROTECT fuse (if set, SWD disabled)
  │  - Checks UICR for bootloader address
  │  - If MBR/SD present, jumps to Master Boot Record
  │
  ▼
Master Boot Record (MBR) @ 0x00000000
  │  - SoftDevice S140 v7.3.0 initialization
  │  - Sets up BLE protocol stack
  │  - Configures MPU regions for SD isolation
  │  - Passes control to Application @ 0x00026000
  │
  ▼
Application Entry (main.c)
  │  1. System initialization (clocks, power, GPIO)
  │  2. Peripheral initialization (SPI, QSPI, TWI, USBD)
  │  3. CAN controller initialization (MCP2518FD ×2)
  │  4. PSRAM ring buffer initialization
  │  5. BLE stack initialization (GAP, GATT, NUS service)
  │  6. USB CDC-ACM initialization
  │  7. OLED display initialization
  │  8. Main loop (frame processing, BLE events, USB polling)
  │
  ▼
Main Loop (infinite)
  │  - Process CAN interrupts (frame reception)
  │  - Service BLE SoftDevice events
  │  - Handle USB CDC data
  │  - Execute injection schedules
  │  - Update OLED display
  │  - Manage power states
  │
  └──► (never exits)
```

### Memory Layout After Boot

```
0x00000000 ─ 0x00000FFF:  MBR (4 KB)
0x00001000 ─ 0x00025FFF:  SoftDevice S140 (148 KB)
0x00026000 ─ 0x000FFFFF:  Application (872 KB)
0x10001000 ─ 0x100010FF:  UICR (customer config)
0x10000000 ─ 0x100000FF:  FICR (factory info)
0x20000000 ─ 0x2003FFFF:  RAM (256 KB)
  ┌─ 0x20000000: SoftDevice RAM (caller-provided)
  │   ┌─ 0x20000000: SD private (configurable)
  │   └─ 0x2000A000: SD application-shared
  └─ 0x2000B000: Application RAM (~200 KB)
      ┌─ 0x2000B000: .data / .bss
      ├─ 0x20010000: Heap (configurable)
      └─ 0x20030000: Stack top
```

### UICR Configuration

| Register | Address | Value | Purpose |
|---|---|---|---|
| UICR_APPROTECT | 0x10001208 | 0xFFFFFF00 (enabled) | Disable SWD in production |
| UICR_NRFFW[0] | 0x10001014 | 0x00026000 | Application start address |
| UICR_XTALFREQ | 0x10001024 | 0xFFFFFFFF (auto-detect) | Use default 32 MHz |
| UICR_REGOUT0 | 0x10001304 | 0xFFFFFFF3 | 3.3V regulator, 0 dBm TX power default |

## 4.2 Clock Initialization

### Clock Tree Configuration

```c
// Clock initialization sequence (simplified)
void clock_init(void) {
    // 1. Start 32 MHz HF crystal
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (!NRF_CLOCK->EVENTS_HFCLKSTARTED);
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

    // 2. Start 32.768 kHz LF crystal
    NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (!NRF_CLOCK->EVENTS_LFCLKSTARTED);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

    // 3. Calibrate LFCLK against HFCLK (every 4 seconds)
    NRF_CLOCK->CTIV = 16;  // 16 × 0.25s = 4s interval
    NRF_CLOCK->TASKS_CAL = 1;

    // 4. Configure PCLK16M (16 MHz) and PCLK64M (64 MHz)
    // HFCLK is 64 MHz by default when crystal is running
    // SPIM peripherals use PCLK16M (16 MHz) divided down
    // QSPI uses PCLK64M (64 MHz) with its own divider
}
```

### Clock Frequencies

| Clock | Frequency | Source | Used By |
|---|---|---|---|
| HFCLK | 64 MHz | 32 MHz crystal × PLL | CPU, all high-speed peripherals |
| PCLK64M | 64 MHz | HFCLK | QSPI, TIMER, RTC |
| PCLK16M | 16 MHz | HFCLK ÷ 4 | SPIM, TWIM, UARTE, SAADC |
| PCLK1M | 1 MHz | HFCLK ÷ 64 | WDT, LPCOMP |
| LFCLK | 32.768 kHz | 32.768 kHz crystal | RTC, BLE SoftDevice timing, WDT |

### Peripheral Clock Dividers

| Peripheral | Base Clock | Divider | Result |
|---|---|---|---|
| SPIM0 (CAN0) | PCLK16M (16 MHz) | ÷2 | 8 MHz |
| SPIM1 (CAN1) | PCLK16M (16 MHz) | ÷2 | 8 MHz |
| QSPI | PCLK64M (64 MHz) | ÷1 (80 MHz capable, but 64 MHz used) | 64 MHz |
| TWIM0 (I²C) | PCLK16M (16 MHz) | ÷40 | 400 kHz |
| USBD | PCLK64M (64 MHz) | Internal PLL | 48 MHz (USB FS) |
| RTC0 | LFCLK (32.768 kHz) | ÷1 | 32.768 kHz |
| TIMER0 | PCLK16M (16 MHz) | ÷16 | 1 MHz (µs timestamp) |

## 4.3 MMIO Register Map

### nRF52840 Peripheral Base Addresses

| Peripheral | Base Address | Size | Description |
|---|---|---|---|
| CLOCK | 0x40000000 | 0x1000 | Clock control |
| POWER | 0x40000000 | 0x1000 | Power management (shared with CLOCK) |
| SPIM0 | 0x40003000 | 0x1000 | SPI Master 0 (CAN0) |
| SPIM1 | 0x40004000 | 0x1000 | SPI Master 1 (CAN1) |
| QSPI | 0x40029000 | 0x1000 | Quad SPI |
| TWIM0 | 0x40003000 | 0x1000 | I²C Master 0 (shared base, different instance) |
| USBD | 0x40027000 | 0x1000 | USB Device |
| TIMER0 | 0x40008000 | 0x1000 | Timer 0 (µs counter) |
| TIMER1 | 0x40009000 | 0x1000 | Timer 1 (injection scheduler) |
| TIMER2 | 0x4000A000 | 0x1000 | Timer 2 (watchdog kick) |
| RTC0 | 0x4000B000 | 0x1000 | Real-Time Counter 0 |
| RTC1 | 0x4000C000 | 0x1000 | Real-Time Counter 1 (BLE timeslot) |
| RTC2 | 0x4000D000 | 0x1000 | Real-Time Counter 2 |
| GPIOTE | 0x40006000 | 0x1000 | GPIO Tasks and Events |
| P0 | 0x50000000 | 0x1000 | GPIO Port 0 |
| P1 | 0x50000300 | 0x1000 | GPIO Port 1 |
| SAADC | 0x40007000 | 0x1000 | ADC (battery monitor) |
| NVMC | 0x4001E000 | 0x1000 | Non-Volatile Memory Controller |
| WDT | 0x40010000 | 0x1000 | Watchdog Timer |
| RADIO | 0x40001000 | 0x1000 | 2.4 GHz Radio (BLE) |
| FICR | 0x10000000 | 0x1000 | Factory Information |
| UICR | 0x10001000 | 0x1000 | User Information |

### Key Register Offsets (SPIM0 Example)

| Register | Offset | Description |
|---|---|---|
| TASKS_START | 0x000 | Start SPI transaction |
| TASKS_STOP | 0x004 | Stop SPI transaction |
| EVENTS_END | 0x100 | Transaction complete |
| EVENTS_ENDRX | 0x104 | RX buffer full |
| EVENTS_ENDTX | 0x108 | TX buffer done |
| INTENSET | 0x304 | Interrupt enable set |
| INTENCLR | 0x308 | Interrupt enable clear |
| ENABLE | 0x500 | Peripheral enable (write 0x07 for SPIM) |
| PSEL_SCK | 0x508 | SCK pin select |
| PSEL_MOSI | 0x50C | MOSI pin select |
| PSEL_MISO | 0x510 | MISO pin select |
| PSEL_CSN | 0x514 | CSN pin select |
| FREQUENCY | 0x524 | Clock frequency (0x04000000 = 8 MHz) |
| RXD_PTR | 0x534 | EasyDMA RX buffer pointer |
| RXD_MAXCNT | 0x538 | EasyDMA RX max count |
| TXD_PTR | 0x53C | EasyDMA TX buffer pointer |
| TXD_MAXCNT | 0x540 | EasyDMA TX max count |
| CONFIG | 0x554 | SPI mode (0x00 = Mode 0) |
| ORC | 0x5C0 | Over-read character (0xFF) |

### Key Register Offsets (QSPI)

| Register | Offset | Description |
|---|---|---|
| TASKS_ACTIVATE | 0x000 | Activate QSPI (must be called first) |
| TASKS_READSTART | 0x004 | Start read operation |
| TASKS_WRITESTART | 0x008 | Start write operation |
| TASKS_ERASESTART | 0x00C | Start erase operation |
| EVENTS_READY | 0x100 | QSPI peripheral ready |
| INTENSET | 0x304 | Interrupt enable |
| ENABLE | 0x500 | Enable QSPI |
| PSEL_CSN0 | 0x508 | CSN0 pin (PSRAM) |
| PSEL_CSN1 | 0x50C | CSN1 pin (Flash) |
| PSEL_SCK | 0x510 | SCK pin |
| PSEL_IO0 | 0x514 | IO0 pin |
| PSEL_IO1 | 0x518 | IO1 pin |
| PSEL_IO2 | 0x51C | IO2 pin |
| PSEL_IO3 | 0x520 | IO3 pin |
| IFCONFIG0 | 0x524 | Interface config (PPSIZE, WRITEOC, READOC, ADDRMODE) |
| IFCONFIG1 | 0x528 | Interface config (SCKFREQ, SAMPLE, SPI_MODE, DPN) |
| CINSTRCONF | 0x530 | Custom instruction config |
| CINSTRDAT0 | 0x534 | Custom instruction data byte 0 |
| CINSTRDAT1 | 0x538 | Custom instruction data byte 1 |
| DURATION | 0x53C | Duration for custom instructions |
| READ_SRC | 0x540 | Read source address |
| READ_DST | 0x544 | Read destination pointer (RAM) |
| READ_CNT | 0x548 | Read count |
| WRITE_SRC | 0x550 | Write source pointer (RAM) |
| WRITE_DST | 0x554 | Write destination address |
| WRITE_CNT | 0x558 | Write count |
| ERASE_ADDR | 0x560 | Erase start address |
| ERASE_LEN | 0x564 | Erase length |
| XIPOFFSET | 0x570 | XIP offset |
| XIPEN | 0x574 | XIP enable |

### Key Register Offsets (USBD)

| Register | Offset | Description |
|---|---|---|
| TASKS_STARTEPIN[0] | 0x000 | Start IN transfer on EP0 |
| TASKS_STARTEPOUT[0] | 0x020 | Start OUT transfer on EP0 |
| EVENTS_USBRESET | 0x100 | USB reset detected |
| EVENTS_STARTED | 0x104 | USB started |
| EVENTS_EP0SETUP | 0x110 | SETUP packet received |
| EVENTS_EP0DATADONE | 0x114 | EP0 data stage done |
| EVENTS_EPDATA | 0x118 | Data event on endpoint |
| SHORTS | 0x200 | Shortcut register |
| INTEN | 0x300 | Interrupt enable |
| ENABLE | 0x500 | Enable USB |
| USBPULLUP | 0x504 | D+ pull-up enable |
| DPDMVALUE | 0x508 | D+/D- line state |
| DTOGGLE | 0x50C | Data toggle |
| EPMODE | 0x510 | Endpoint mode |
| EPINEN | 0x514 | IN endpoint enable |
| EPOUTEN | 0x518 | OUT endpoint enable |
| EPSTATUS | 0x51C | Endpoint status |
| EPDATASTATUS | 0x520 | Endpoint data status |
| USBADDR | 0x524 | USB device address |
| BMREQUESTTYPE | 0x540 | bmRequestType from SETUP |
| BREQUEST | 0x544 | bRequest from SETUP |
| WVALUEL | 0x548 | wValue low byte |
| WVALUEH | 0x54C | wValue high byte |
| WINDEXL | 0x550 | wIndex low byte |
| WINDEXH | 0x554 | wIndex high byte |
| WLENGTHL | 0x558 | wLength low byte |
| WLENGTHH | 0x55C | wLength high byte |
| EPOUT[0].PTR | 0x560 | EP0 OUT data pointer |
| EPOUT[0].MAXCNT | 0x564 | EP0 OUT max count |
| EPOUT[0].AMOUNT | 0x568 | EP0 OUT received amount |
| EPIN[0].PTR | 0x580 | EP0 IN data pointer |
| EPIN[0].MAXCNT | 0x584 | EP0 IN max count |
| EPIN[0].AMOUNT | 0x588 | EP0 IN transferred amount |
| EPIN[1].PTR | 0x5A0 | EP1 IN (CDC data) pointer |
| EPIN[1].MAXCNT | 0x5A4 | EP1 IN max count |
| EPIN[1].AMOUNT | 0x5A8 | EP1 IN amount |
| EPOUT[1].PTR | 0x5C0 | EP1 OUT (CDC data) pointer |
| EPOUT[1].MAXCNT | 0x5C4 | EP1 OUT max count |
| EPOUT[1].AMOUNT | 0x5C8 | EP1 OUT amount |

### Key Register Offsets (TIMER0 — µs Counter)

| Register | Offset | Description |
|---|---|---|
| TASKS_START | 0x000 | Start timer |
| TASKS_STOP | 0x004 | Stop timer |
| TASKS_COUNT | 0x008 | Count (increment by 1) |
| TASKS_CLEAR | 0x00C | Clear timer |
| TASKS_CAPTURE[0] | 0x040 | Capture to CC[0] |
| EVENTS_COMPARE[0] | 0x140 | Compare match on CC[0] |
| SHORTS | 0x200 | Shortcut register |
| INTENSET | 0x304 | Interrupt enable |
| MODE | 0x504 | Timer mode (0 = Timer) |
| BITMODE | 0x508 | Bit width (3 = 32-bit) |
| PRESCALER | 0x510 | Prescaler (4 = ÷16 → 1 MHz from 16 MHz) |
| CC[0] | 0x540 | Capture/Compare register 0 |

### Key Register Offsets (GPIOTE)

| Register | Offset | Description |
|---|---|---|
| TASKS_OUT[0] | 0x000 | Set OUT[0] pin |
| TASKS_CLR[0] | 0x030 | Clear OUT[0] pin |
| EVENTS_IN[0] | 0x100 | Input event on IN[0] |
| INTENSET | 0x304 | Interrupt enable |
| CONFIG[0] | 0x510 | Channel 0 config (Mode, Pin, Polarity, OutInit) |

### Key Register Offsets (P0/P1 GPIO)

| Register | Offset | Description |
|---|---|---|
| OUT | 0x004 | Write GPIO port |
| OUTSET | 0x008 | Set individual bits |
| OUTCLR | 0x00C | Clear individual bits |
| IN | 0x010 | Read GPIO port |
| DIR | 0x014 | Direction (1=output, 0=input) |
| DIRSET | 0x018 | Set direction to output |
| DIRCLR | 0x01C | Set direction to input |
| PIN_CNF[n] | 0x700 + n*4 | Pin configuration (Pull, Drive, Sense, Input buffer) |

## 4.4 GPIO Initialization

### Pin Configuration Sequence

```c
// GPIO initialization for all pins
void gpio_init(void) {
    // CAN0 SPI pins
    NRF_P0->PIN_CNF[17] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // CSN
    NRF_P0->PIN_CNF[28] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);  // MISO
    NRF_P0->PIN_CNF[29] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // MOSI
    NRF_P0->PIN_CNF[30] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // SCK

    // CAN1 SPI pins
    NRF_P0->PIN_CNF[11] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // MOSI
    NRF_P0->PIN_CNF[12] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);  // MISO
    NRF_P0->PIN_CNF[14] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // SCK
    NRF_P0->PIN_CNF[22] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // CSN

    // CAN interrupt inputs
    NRF_P0->PIN_CNF[15] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                           (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos);  // CAN1 INT
    NRF_P0->PIN_CNF[16] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                           (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos);  // CAN0 INT

    // CAN transceiver control
    NRF_P1->PIN_CNF[0] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);   // CAN0 STB
    NRF_P1->PIN_CNF[1] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);   // CAN1 STB
    NRF_P1->PIN_CNF[10] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // CAN0 EN
    NRF_P1->PIN_CNF[11] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // CAN1 EN

    // Termination FET control
    NRF_P1->PIN_CNF[2] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);   // CAN0 TERM
    NRF_P1->PIN_CNF[3] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);   // CAN1 TERM

    // LEDs
    NRF_P0->PIN_CNF[31] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // LED1 Red
    NRF_P1->PIN_CNF[4] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);   // LED2 Green
    NRF_P1->PIN_CNF[5] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);   // LED3 Blue

    // Button
    NRF_P1->PIN_CNF[6] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                          (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);    // SW1

    // Battery voltage sense (analog input)
    NRF_P0->PIN_CNF[4] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                          (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);  // AIN2

    // QSPI pins (configured by QSPI peripheral, but set initial state)
    NRF_P0->PIN_CNF[13] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // CSN0
    NRF_P0->PIN_CNF[19] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);  // CSN1

    // I²C pins (configured by TWIM peripheral)
    NRF_P0->PIN_CNF[26] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);  // SDA
    NRF_P0->PIN_CNF[27] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                           (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);  // SCL

    // USB pins (configured by USBD peripheral)
    // D+ and D- are fixed-function, no PIN_CNF needed

    // Set initial output states
    NRF_P1->OUTCLR = (1 << 0) | (1 << 1);   // STB low = normal mode
    NRF_P1->OUTSET = (1 << 10) | (1 << 11);  // EN high = enabled
    NRF_P1->OUTCLR = (1 << 2) | (1 << 3);    // TERM off initially
    NRF_P0->OUTSET = (1 << 17) | (1 << 22);  // SPI CS high (inactive)
    NRF_P0->OUTSET = (1 << 13) | (1 << 19);  // QSPI CS high (inactive)
}
```

## 4.5 Device Drivers

### 4.5.1 MCP2518FD CAN FD Controller Driver

The MCP2518FD driver provides a complete SPI-based interface to the CAN FD controller. It handles:

- **Initialization**: Reset, configuration register setup, FIFO configuration, bit timing
- **Frame transmission**: Write CAN/CAN FD frames to TX FIFO with automatic arbitration
- **Frame reception**: Read frames from RX FIFO with timestamp capture
- **Error handling**: Monitor TEC/REC counters, bus-off recovery
- **Filter configuration**: Set up acceptance filters for selective frame capture
- **Mode control**: Normal, Listen-Only (passive sniff), Loopback, Bus Monitor

#### MCP2518FD Register Map (SPI Address Space)

| Register | Address | Size | Description |
|---|---|---|---|
| CON[0-3] | 0x000-0x003 | 4×32-bit | Configuration registers |
| NBTCFG[0-3] | 0x004-0x007 | 4×32-bit | Nominal Bit Time Configuration |
| DBTCFG[0-3] | 0x008-0x00B | 4×32-bit | Data Bit Time Configuration |
| TDC[0-3] | 0x00C-0x00F | 4×32-bit | Transmitter Delay Compensation |
| TBC | 0x010 | 32-bit | Time Base Counter |
| TSCON[0-3] | 0x014-0x017 | 4×32-bit | Timestamp Control |
| VEC[0-3] | 0x018-0x01B | 4×32-bit | Interrupt Vector |
| INT[0-3] | 0x01C-0x01F | 4×32-bit | Interrupt Flags |
| RXIF | 0x020 | 32-bit | RX Interrupt Flags |
| TXIF | 0x024 | 32-bit | TX Interrupt Flags |
| RXOVIF | 0x028 | 32-bit | RX Overflow Interrupt Flags |
| TXATIF | 0x02C | 32-bit | TX Attempt Interrupt Flags |
| TXREQ | 0x030 | 32-bit | TX Request |
| TREC[0-3] | 0x034-0x037 | 4×32-bit | Transmit/Receive Error Counters |
| BDIA[0-3] | 0x038-0x03B | 4×32-bit | Bus Diagnostic |
| TEFCON[0-3] | 0x040-0x043 | 4×32-bit | TEF Configuration |
| TEFSTA[0-3] | 0x044-0x047 | 4×32-bit | TEF Status |
| TEFUA[0-3] | 0x048-0x04B | 4×32-bit | TEF User Address |
| TXQCON[0-3] | 0x050-0x053 | 4×32-bit | TX Queue Configuration |
| TXQSTA[0-3] | 0x054-0x057 | 4×32-bit | TX Queue Status |
| TXQUA[0-3] | 0x058-0x05B | 4×32-bit | TX Queue User Address |
| FIFOCON[1-31][0-3] | 0x060+ | 4×32-bit each | FIFO Configuration (31 FIFOs) |
| FIFOSTA[1-31][0-3] | 0x0A0+ | 4×32-bit each | FIFO Status |
| FIFOUA[1-31][0-3] | 0x0E0+ | 4×32-bit each | FIFO User Address |
| FLTCON[0-31][0-3] | 0x200+ | 4×32-bit each | Filter Configuration (32 filters) |
| FLTOBJ[0-31][0-3] | 0x280+ | 4×32-bit each | Filter Object (mask/ID) |
| MASK[0-31][0-3] | 0x300+ | 4×32-bit each | Filter Mask |
| OSC | 0xE00 | 32-bit | Oscillator Control |
| OSCSTA | 0xE04 | 32-bit | Oscillator Status |
| IOCON[0-3] | 0xE08-0xE0B | 4×32-bit | I/O Configuration |
| CANCTRL[0-3] | 0xE0C-0xE0F | 4×32-bit | CAN Control |
| CRC[0-3] | 0xE10-0xE13 | 4×32-bit | CRC |

#### SPI Instruction Set

| Instruction | Opcode | Description |
|---|---|---|
| RESET | 0x00 | Reset device |
| READ | 0x03 | Read from register (followed by 16-bit address) |
| WRITE | 0x02 | Write to register (followed by 16-bit address) |
| READ_CRC | 0x0B | Read with CRC verification |
| WRITE_CRC | 0x0A | Write with CRC |
| WRITE_SAFE | 0x0C | Write with error checking |

#### Bit Timing Configuration (500 kbps Nominal, 4 Mbps Data)

```c
// Nominal bit time: 500 kbps
// Clock: 40 MHz → TQ = 25 ns
// Bit time = 2000 ns = 80 TQ
// SyncSeg = 1 TQ, PropSeg = 31 TQ, PhaseSeg1 = 24 TQ, PhaseSeg2 = 24 TQ
// SJW = 8 TQ
// BRP = 0 (div by 1)

#define NBTCFG_500KBPS_BRP     0
#define NBTCFG_500KBPS_TSEG1   55  // PropSeg + PhaseSeg1 - 1
#define NBTCFG_500KBPS_TSEG2   23  // PhaseSeg2 - 1
#define NBTCFG_500KBPS_SJW     7   // SJW - 1

// Data bit time: 4 Mbps (CAN FD)
// TQ = 25 ns, Bit time = 250 ns = 10 TQ
// SyncSeg = 1, PropSeg = 3, PhaseSeg1 = 3, PhaseSeg2 = 3
// SJW = 2, BRP = 0

#define DBTCFG_4MBPS_BRP       0
#define DBTCFG_4MBPS_TSEG1    5   // PropSeg + PhaseSeg1 - 1
#define DBTCFG_4MBPS_TSEG2    2   // PhaseSeg2 - 1
#define DBTCFG_4MBPS_SJW      1   // SJW - 1
```

#### FIFO Configuration

```
FIFO 1: RX FIFO for CAN0, 32 messages deep, store timestamp
FIFO 2: RX FIFO for CAN0 FD frames, 16 messages deep
FIFO 3: TX FIFO for CAN0, 8 messages deep, highest priority
FIFO 4: TX FIFO for CAN0 FD frames, 4 messages deep
FIFO 5-8: Reserved for CAN0 filters
FIFO 9-12: Same configuration for CAN1
```

#### Driver API

```c
// can_fd_driver.h
typedef struct {
    uint8_t channel;        // 0 or 1
    uint32_t nominal_brp;
    uint32_t nominal_tseg1;
    uint32_t nominal_tseg2;
    uint32_t nominal_sjw;
    uint32_t data_brp;
    uint32_t data_tseg1;
    uint32_t data_tseg2;
    uint32_t data_sjw;
    bool fd_enable;
    bool brs_enable;
    bool listen_only;       // Passive sniff mode
} can_fd_config_t;

typedef struct {
    uint32_t id;            // CAN ID (11 or 29 bit)
    uint8_t dlc;            // Data length code
    uint8_t data[64];       // Payload
    uint32_t timestamp;     // Hardware timestamp (µs)
    uint8_t flags;          // IDE, RTR, FD, BRS, ESI
} can_frame_t;

// API functions
int can_fd_init(uint8_t channel, const can_fd_config_t *config);
int can_fd_deinit(uint8_t channel);
int can_fd_transmit(uint8_t channel, const can_frame_t *frame);
int can_fd_receive(uint8_t channel, can_frame_t *frame);  // Non-blocking, returns 0 if no frame
int can_fd_set_mode(uint8_t channel, uint8_t mode);  // NORMAL, LISTEN_ONLY, LOOPBACK, BUS_MONITOR
int can_fd_set_filter(uint8_t channel, uint8_t filter_num, uint32_t mask, uint32_t id);
int can_fd_get_error_counters(uint8_t channel, uint8_t *tec, uint8_t *rec);
int can_fd_get_bus_status(uint8_t channel);  // Returns BUS_OFF, ERROR_PASSIVE, ERROR_ACTIVE
void can_fd_isr_handler(uint8_t channel);     // Called from GPIO interrupt
```

### 4.5.2 QSPI PSRAM Driver

The QSPI driver manages the APS6404L 8MB PSRAM for frame buffering.

#### APS6404L Command Set

| Command | Opcode | Description |
|---|---|---|
| READ | 0x03 | Standard SPI read (1-bit) |
| FAST_READ | 0x0B | Fast read (1-bit, with dummy) |
| FAST_READ_QUAD | 0xEB | Quad I/O fast read |
| WRITE | 0x02 | Standard SPI write (1-bit) |
| WRITE_QUAD | 0x38 | Quad I/O write |
| ENTER_QUAD | 0x35 | Enter quad mode |
| RESET_QUAD | 0xF5 | Exit quad mode |
| READ_ID | 0x9F | Read manufacturer/device ID |
| BURST_LENGTH | 0xC0 | Set burst length (wrap) |

#### Ring Buffer Implementation

```c
// psram_driver.h
#define PSRAM_BASE_ADDR         0x00800000  // QSPI XIP base
#define PSRAM_SIZE              (8 * 1024 * 1024)  // 8 MB
#define PSRAM_CH0_BUFFER_OFFSET 0x00000000
#define PSRAM_CH0_BUFFER_SIZE   (512 * 1024)  // 512 KB = 16K frames × 32 bytes
#define PSRAM_CH1_BUFFER_OFFSET 0x00080000
#define PSRAM_CH1_BUFFER_SIZE   (512 * 1024)
#define PSRAM_SCRIPT_OFFSET     0x00100000
#define PSRAM_SCRIPT_SIZE       (1 * 1024 * 1024)  // 1 MB
#define PSRAM_DBC_OFFSET        0x00200000
#define PSRAM_DBC_SIZE          (2 * 1024 * 1024)  // 2 MB

typedef struct {
    uint32_t write_ptr;     // Current write position (incremented by driver)
    uint32_t read_ptr;      // Current read position (incremented by protocol handler)
    uint32_t frame_count;   // Number of frames currently in buffer
    uint32_t overflow_count;// Number of dropped frames due to buffer full
    uint32_t base_offset;   // Base offset in PSRAM
    uint32_t buffer_size;   // Total buffer size in bytes
} ring_buffer_t;

// API
int psram_init(void);
int psram_deinit(void);
int psram_ring_init(ring_buffer_t *ring, uint32_t base_offset, uint32_t size);
int psram_ring_write(ring_buffer_t *ring, const uint8_t *data, uint32_t len);
int psram_ring_read(ring_buffer_t *ring, uint8_t *data, uint32_t len, uint32_t *frames_read);
int psram_ring_peek(ring_buffer_t *ring, uint8_t *data, uint32_t len);
int psram_ring_clear(ring_buffer_t *ring);
uint32_t psram_ring_available(ring_buffer_t *ring);
int psram_direct_read(uint32_t addr, uint8_t *data, uint32_t len);
int psram_direct_write(uint32_t addr, const uint8_t *data, uint32_t len);
```

### 4.5.3 BLE NUS Driver

The BLE driver implements the Nordic UART Service for wireless frame transfer.

#### GATT Service Definition

```
Primary Service: Nordic UART Service (UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E)
  ├─ TX Characteristic (UUID: 6E400002-...) — NOTIFY, device → app
  │   Properties: Notify
  │   Max length: 244 bytes (BLE 5.2 Data Length Extension)
  ├─ RX Characteristic (UUID: 6E400003-...) — WRITE_NO_RESPONSE, app → device
  │   Properties: Write Without Response
  │   Max length: 244 bytes
  └─ Flow Control Characteristic (custom, UUID: 6E400004-...)
      Properties: Notify, Read
      Value: 2-byte available buffer space (little-endian)
```

#### BLE Connection Parameters

| Parameter | Value | Notes |
|---|---|---|
| Connection interval min | 7.5 ms | Fastest allowed by BLE spec |
| Connection interval max | 15 ms | Balance throughput vs power |
| Slave latency | 0 | No skipped connection events |
| Supervision timeout | 4000 ms | 4 seconds |
| Data length extension | Enabled | Up to 251 bytes per packet |
| MTU size | 247 bytes | Maximum for DLE |
| PHY | 2M preferred | Double throughput |
| TX power | 0 dBm | Default, adjustable |

#### BLE Driver API

```c
// ble_nus_driver.h
typedef void (*ble_nus_rx_callback_t)(const uint8_t *data, uint16_t len);
typedef void (*ble_nus_tx_complete_callback_t)(void);

typedef struct {
    ble_nus_rx_callback_t rx_callback;
    ble_nus_tx_complete_callback_t tx_complete_callback;
    uint16_t max_tx_size;
    uint16_t max_rx_size;
} ble_nus_config_t;

// API
int ble_nus_init(const ble_nus_config_t *config);
int ble_nus_start_advertising(void);
int ble_nus_stop_advertising(void);
int ble_nus_disconnect(void);
int ble_nus_tx(const uint8_t *data, uint16_t len);
int ble_nus_tx_pending(void);  // Returns number of queued TX packets
bool ble_nus_is_connected(void);
int ble_nus_get_mtu(void);
int ble_nus_set_tx_power(int8_t dbm);
void ble_nus_event_handler(const ble_evt_t *event);  // Called from SD event dispatcher
```

### 4.5.4 USB CDC-ACM Driver

The USB driver implements CDC-ACM for wired frame transfer.

#### USB Descriptor Configuration

```
Device Descriptor:
  - VID: 0x1915 (Nordic Semiconductor)
  - PID: 0xCAFE (CAN Creeper)
  - bcdDevice: 0x0100 (Rev 1.0)
  - Class: 0x02 (CDC)
  - Subclass: 0x00
  - Protocol: 0x00

Configuration Descriptor:
  - 1 configuration
  - 2 interfaces:
    - Interface 0: CDC Communication (ACM)
      - 1 endpoint: EP2 IN (interrupt, 16 bytes, 64ms interval)
      - Class requests: SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE
    - Interface 1: CDC Data
      - 2 endpoints: EP1 IN (bulk, 64 bytes), EP1 OUT (bulk, 64 bytes)

String Descriptors:
  - String 0: Language ID (0x0409 English)
  - String 1: "Nous Research"
  - String 2: "CAN Creeper"
  - String 3: Serial number (from FICR device ID)
```

#### CDC-ACM Line Coding

| Parameter | Value |
|---|---|
| Baud rate | 921600 bps (ignored — raw USB, but reported for host compatibility) |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |

#### USB Driver API

```c
// usb_cdc_driver.h
typedef void (*usb_cdc_rx_callback_t)(const uint8_t *data, uint16_t len);
typedef void (*usb_cdc_tx_complete_callback_t)(void);
typedef void (*usb_cdc_state_callback_t)(bool connected);

typedef struct {
    usb_cdc_rx_callback_t rx_callback;
    usb_cdc_tx_complete_callback_t tx_complete_callback;
    usb_cdc_state_callback_t state_callback;
} usb_cdc_config_t;

// API
int usb_cdc_init(const usb_cdc_config_t *config);
int usb_cdc_deinit(void);
int usb_cdc_tx(const uint8_t *data, uint16_t len);
int usb_cdc_tx_pending(void);
bool usb_cdc_is_connected(void);
void usb_cdc_event_handler(const app_usbd_event_t *event);
```

### 4.5.5 SSD1306 OLED Driver

```c
// oled_driver.h
#define SSD1306_I2C_ADDR    0x3C
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_PAGES       8  // 8 pages × 8 rows = 64 pixels

typedef enum {
    OLED_FONT_5X7,
    OLED_FONT_8X16
} oled_font_t;

// API
int oled_init(void);
int oled_clear(void);
int oled_display(void);  // Push framebuffer to display
int oled_set_pixel(uint8_t x, uint8_t y, bool on);
int oled_draw_char(uint8_t x, uint8_t y, char c, oled_font_t font);
int oled_draw_string(uint8_t x, uint8_t y, const char *str, oled_font_t font);
int oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
int oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool fill);
int oled_set_contrast(uint8_t contrast);  // 0-255
int oled_power_on(void);
int oled_power_off(void);
int oled_invert(bool invert);
```

## 4.6 Interrupt Architecture

### Interrupt Priority Assignment

| Priority | IRQ | Peripheral | Handler | Notes |
|---|---|---|---|---|
| 0 (highest) | RADIO | BLE SoftDevice | SD internal | Time-critical BLE events |
| 1 | TIMER0 | µs counter | timer0_isr | Timestamp capture |
| 2 | GPIOTE | CAN INT pins | gpiote_isr | CAN frame arrival |
| 3 | SPIM0_SPIM1 | CAN SPI | spim_isr | SPI transaction complete |
| 4 | QSPI | PSRAM/Flash | qspi_isr | QSPI transfer done |
| 5 | USBD | USB | usbd_isr | USB events |
| 6 (lowest) | RTC0 | System tick | rtc0_isr | 1ms system tick |
| — | SWI0 | SoftDevice callback | swi0_isr | SD → App events |
| — | SWI1 | Application deferred | swi1_isr | Deferred work queue |

### CAN Frame Reception Interrupt Flow

```
CAN Bus Frame Arrives
  │
  ▼
TJA1445 RXD pin toggles
  │
  ▼
MCP2518FD decodes frame, stores in RX FIFO
  │
  ▼
MCP2518FD nINT pin asserts (active low)
  │
  ▼
nRF52840 GPIOTE detects falling edge on P0.16/P0.15
  │
  ▼
GPIOTE_IRQHandler():
  │  - Read MCP2518FD INT register to identify interrupt source
  │  - If RXIF (receive interrupt):
  │      - Read frame from MCP2518FD FIFO via SPI (EasyDMA)
  │      - SPIM END event triggers SPIM_IRQHandler
  │  - If TEFIF (timestamp event):
  │      - Read timestamp from TEF FIFO
  │
  ▼
SPIM0_SPIM1_IRQHandler():
  │  - Frame data now in RAM buffer
  │  - Capture TIMER0->CC[0] for software timestamp
  │  - Build frame metadata (ID, DLC, flags, timestamp)
  │  - Write to PSRAM ring buffer via QSPI
  │  - If BLE connected: Queue frame for BLE NUS notification
  │  - If USB connected: Queue frame for USB CDC TX
  │  - Update frame counter on OLED
```

### BLE Event Flow

```
SoftDevice RADIO interrupt (highest priority)
  │
  ▼
SD internal processing
  │
  ▼
SWI0_IRQHandler() (lower priority, deferred):
  │  - Dispatch BLE events to application
  │  - GAP events: Connected, Disconnected, Advertising timeout
  │  - GATT events: Write received (RX data), Notification sent (TX complete)
  │  - Update connection state, MTU size, PHY
```

## 4.7 DMA Architecture

### EasyDMA Usage

nRF52840 peripherals use EasyDMA for zero-CPU data transfers:

| Peripheral | DMA Direction | Buffer Location | Trigger |
|---|---|---|---|
| SPIM0 RX | Peripheral → RAM | can0_rx_buffer[64] | SPI transaction |
| SPIM0 TX | RAM → Peripheral | can0_tx_buffer[64] | SPI transaction |
| SPIM1 RX | Peripheral → RAM | can1_rx_buffer[64] | SPI transaction |
| SPIM1 TX | RAM → Peripheral | can1_tx_buffer[64] | SPI transaction |
| QSPI READ | PSRAM → RAM | ring_buffer_temp[256] | QSPI read task |
| QSPI WRITE | RAM → PSRAM | ring_buffer_temp[256] | QSPI write task |
| USBD EP1 IN | RAM → USB | cdc_tx_buffer[64] | USB IN token |
| USBD EP1 OUT | USB → RAM | cdc_rx_buffer[64] | USB OUT token |
| TWIM0 TX | RAM → OLED | oled_cmd_buffer[8] | I²C transaction |
| SAADC | ADC → RAM | adc_sample_buffer[1] | ADC sample task |

### DMA Buffer Management

```c
// Double-buffering for CAN SPI transactions
static uint8_t can0_rx_buf[2][64];  // Ping-pong buffers
static uint8_t can0_tx_buf[2][64];
static volatile uint8_t can0_active_buf = 0;

// QSPI ring buffer write uses scatter-gather via repeated QSPI write tasks
// Each write: 32 bytes (one frame entry) to PSRAM at ring write_ptr
// After write: increment write_ptr, wrap if needed

// USB CDC uses circular buffer for TX
static uint8_t cdc_tx_ring[1024];
static volatile uint16_t cdc_tx_head = 0;
static volatile uint16_t cdc_tx_tail = 0;
```

## 4.8 Power Management

### Power State Machine

```
                    ┌──────────┐
          ┌────────▶│  ACTIVE  │◀────────┐
          │         │ 64 MHz   │         │
          │         │ BLE+USB  │         │
          │         └─────┬────┘         │
          │               │              │
          │    no activity│  │activity    │
          │    30 seconds │  │(CAN frame, │
          │               │  │BLE/USB)    │
          │         ┌─────▼────┐         │
          │         │   IDLE   │─────────┘
          │         │ 16 MHz   │
          │         │ BLE only │
          │         └─────┬────┘
          │               │
          │    no BLE     │
          │    connection │
          │    60 seconds │
          │               │
          │         ┌─────▼────┐
          │         │  SLEEP   │
          │         │ SYSTEM_ON│
          │         │ LFCLK    │
          │         └─────┬────┘
          │               │
          │    button     │
          │    press      │
          └───────────────┘
```

### Power Mode Transitions

```c
// Enter IDLE mode
void power_enter_idle(void) {
    // Disable USB (if not connected)
    if (!usb_cdc_is_connected()) {
        NRF_USBD->ENABLE = 0;
    }
    // Reduce CPU frequency
    // (nRF52840 doesn't support dynamic frequency scaling directly;
    //  we use WFE/WFI for idle cycles instead)
    // Put CAN transceivers in standby
    NRF_P1->OUTSET = (1 << 0) | (1 << 1);  // STB high = standby
    // Disable OLED
    oled_power_off();
    // Keep BLE advertising/connected
}

// Enter SLEEP mode
void power_enter_sleep(void) {
    // Stop HFCLK
    NRF_CLOCK->TASKS_HFCLKSTOP = 1;
    // Disable all peripherals except RTC and WDT
    // Enter SYSTEM_ON sleep (CPU stops, peripherals can wake)
    // WFE will wake on BLE radio event, RTC tick, or GPIO (button)
    __WFE();
}
```

### Battery Monitoring

```c
// Read battery voltage via SAADC on P0.04 (AIN2)
// Voltage divider: 2:1 (2× 100kΩ), so ADC reads VBAT/2
// ADC reference: internal 0.6V, gain = 1/6 → input range 0–3.6V
// VBAT = ADC_value × (3.6V / 4096) × 2

#define BATTERY_FULL_MV      4200
#define BATTERY_LOW_MV       3400
#define BATTERY_CRITICAL_MV  3100

uint16_t battery_read_mv(void) {
    int16_t sample;
    nrf_saadc_sample_convert(2, &sample);  // Channel 2 = AIN2
    // sample is 12-bit signed, range depends on gain
    uint32_t mv = (uint32_t)((sample * 3600UL) / 4096UL) * 2;
    return (uint16_t)mv;
}
```

## 4.9 Watchdog Configuration

```c
// WDT configuration: 8 second timeout, reloaded in main loop
void wdt_init(void) {
    // Configure WDT
    NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos);  // Pause in sleep
    NRF_WDT->CRV = 32768 * 8;  // 8 seconds at 32.768 kHz
    NRF_WDT->RREN = 0x01;      // Enable reload request 0
    NRF_WDT->TASKS_START = 1;  // Start watchdog
}

// In main loop:
void wdt_kick(void) {
    NRF_WDT->RR[0] = WDT_RR_RR_Reload;  // Reload request 0
}
```

## 4.10 Build System

### Toolchain

| Component | Version | Notes |
|---|---|---|
| GNU ARM Embedded Toolchain | 10.3-2021.10 | arm-none-eabi-gcc |
| nRF5 SDK | 17.1.0 | Nordic Semiconductor |
| SoftDevice S140 | 7.3.0 | BLE 5.2 stack |
| Make | 4.3+ | Build system |
| Python 3 | 3.8+ | nrfutil for programming |
| nrfutil | 6.1+ | Nordic command-line tools |

### Makefile Structure

```makefile
# Toolchain
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# SDK paths
SDK_ROOT = /opt/nrf5_sdk_17.1.0
SD_PATH = $(SDK_ROOT)/components/softdevice/s140/hex

# Chip
CPU = cortex-m4
FPU = fpv4-sp-d16
FLOAT_ABI = hard

# Defines
DEFINES = -DNRF52840_XXAA
DEFINES += -DBOARD_CAN_CREEPER
DEFINES += -DSOFTDEVICE_PRESENT
DEFINES += -DS140
DEFINES += -DAPP_TIMER_V2
DEFINES += -DAPP_TIMER_V2_RTC1_ENABLED

# Flags
CFLAGS = -mcpu=$(CPU) -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Werror -O2 -g3
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-strict-aliasing -fno-builtin
CFLAGS += $(DEFINES)

ASMFLAGS = -mcpu=$(CPU) -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += $(DEFINES)

LDFLAGS = -mcpu=$(CPU) -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
LDFLAGS += -mthumb -mabi=aapcs
LDFLAGS += -L$(SDK_ROOT)/modules/nrfx/mdk
LDFLAGS += -Tlinker.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage

# Sources
C_SRC = main.c
C_SRC += drivers/can_fd_driver.c
C_SRC += drivers/psram_driver.c
C_SRC += drivers/ble_nus_driver.c
C_SRC += drivers/usb_cdc_driver.c
C_SRC += drivers/oled_driver.c
C_SRC += drivers/protocol_handler.c
C_SRC += drivers/injection_engine.c

SDK_C_SRC = $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_spim.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_qspi.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_twim.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_timer.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rtc.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_saadc.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_wdt.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/timer/app_timer.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/usbd/app_usbd.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c
SDK_C_SRC += $(SDK_ROOT)/components/ble/common/ble_advdata.c
SDK_C_SRC += $(SDK_ROOT)/components/ble/ble_services/ble_nus/ble_nus.c

# Includes
INC = -I.
INC += -Idrivers
INC += -I$(SDK_ROOT)/modules/nrfx
INC += -I$(SDK_ROOT)/modules/nrfx/mdk
INC += -I$(SDK_ROOT)/modules/nrfx/hal
INC += -I$(SDK_ROOT)/modules/nrfx/drivers/include
INC += -I$(SDK_ROOT)/components
INC += -I$(SDK_ROOT)/components/libraries
INC += -I$(SDK_ROOT)/components/softdevice/s140/headers
INC += -I$(SDK_ROOT)/components/softdevice/common

# Objects
OBJ = $(C_SRC:.c=.o) $(SDK_C_SRC:.c=.o)

# Targets
all: can_creeper.hex can_creeper.bin

can_creeper.elf: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

can_creeper.hex: can_creeper.elf
	$(OBJCOPY) -O ihex $< $@

can_creeper.bin: can_creeper.elf
	$(OBJCOPY) -O binary $< $@

flash: can_creeper.hex
	nrfutil pkg generate --hw-version 52 --sd-req 0x0100 \
		--application can_creeper.hex --application-version 1 \
		can_creeper.zip
	nrfutil dfu usb-serial -pkg can_creeper.zip -p /dev/ttyACM0

flash_swd: can_creeper.hex
	nrfjprog --program can_creeper.hex --sectorerase --reset

erase:
	nrfjprog --eraseall

size: can_creeper.elf
	$(SIZE) $@

clean:
	rm -f $(OBJ) can_creeper.elf can_creeper.hex can_creeper.bin can_creeper.zip
```

### Linker Script (linker.ld)

```
MEMORY
{
  FLASH (rx) : ORIGIN = 0x00026000, LENGTH = 0x000DA000  /* 872 KB for app */
  RAM (rwx)  : ORIGIN = 0x2000B000, LENGTH = 0x00035000  /* 212 KB for app */
}

SECTIONS
{
  .text :
  {
    KEEP(*(.vectors))
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  .data : AT (ADDR(.text) + SIZEOF(.text))
  {
    __data_start__ = .;
    *(.data*)
    . = ALIGN(4);
    __data_end__ = .;
  } > RAM

  .bss :
  {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM

  .heap (NOLOAD) :
  {
    __heap_start__ = .;
    . = . + 0x8000;  /* 32 KB heap */
    __heap_end__ = .;
  } > RAM

  .stack (NOLOAD) :
  {
    __stack_top__ = ORIGIN(RAM) + LENGTH(RAM);
  } > RAM
}
```

## 4.11 Firmware Update (OTA DFU)

### DFU Architecture

```
Application Area: 0x00026000 - 0x000FFFFF (872 KB)
  ├─ Active firmware: 0x00026000 - 0x0006DFFF (280 KB)
  ├─ DFU settings page: 0x0006E000 - 0x0006EFFF (4 KB)
  └─ Reserved for future: 0x0006F000 - 0x000FFFFF

OTA DFU Process:
  1. App receives new firmware image via BLE NUS or USB CDC
  2. Image stored in W25Q128JV external flash (offset 0x000000)
  3. Image validated (CRC32, signature check)
  4. DFU settings written to internal flash
  5. System reset → MBR detects DFU settings → Bootloader mode
  6. Bootloader reads image from external flash → Programs internal flash
  7. Bootloader validates new image → Jumps to application
```

## 4.12 Wire Protocol (Binary)

### Frame Format (Device → App)

```
Byte 0:     Start byte (0xAA)
Byte 1:     Command ID
Byte 2-3:   Payload length (16-bit, little-endian, excluding header and CRC)
Byte 4-N:   Payload
Byte N+1-2: CRC-16/CCITT (over bytes 1 through N)
```

### Command IDs

| ID | Name | Direction | Description |
|---|---|---|---|
| 0x01 | FRAME_RX | Device→App | Received CAN frame |
| 0x02 | FRAME_TX | App→Device | Transmit CAN frame |
| 0x03 | FRAME_TX_ACK | Device→App | TX frame acknowledged (or error) |
| 0x04 | CONFIG_SET | App→Device | Set configuration |
| 0x05 | CONFIG_GET | App→Device | Get configuration |
| 0x06 | CONFIG_RSP | Device→App | Configuration response |
| 0x07 | STATUS_REQ | App→Device | Request device status |
| 0x08 | STATUS_RSP | Device→App | Device status response |
| 0x09 | SCRIPT_LOAD | App→Device | Load injection script |
| 0x0A | SCRIPT_START | App→Device | Start script execution |
| 0x0B | SCRIPT_STOP | App→Device | Stop script execution |
| 0x0C | SCRIPT_STATUS | Device→App | Script execution status |
| 0x0D | DBC_UPLOAD | App→Device | Upload DBC file |
| 0x0E | DBC_LIST | App→Device | List stored DBC files |
| 0x0F | DBC_LIST_RSP | Device→App | DBC file list response |
| 0x10 | BUS_STATUS | Device→App | Periodic bus status update |
| 0x11 | ERROR_EVENT | Device→App | Error notification |
| 0x12 | PING | App→Device | Keep-alive ping |
| 0x13 | PONG | Device→App | Keep-alive response |
| 0x14 | DFU_START | App→Device | Start DFU mode |
| 0x15 | DFU_DATA | App→Device | DFU image chunk |
| 0x16 | DFU_COMPLETE | App→Device | DFU image complete |

### FRAME_RX Payload (Command 0x01)

```
Byte 0-3:   Timestamp (32-bit, µs, little-endian)
Byte 4:     Channel (0 or 1)
Byte 5:     Flags:
              Bit 0: IDE (0=11-bit, 1=29-bit)
              Bit 1: RTR (0=data, 1=remote)
              Bit 2: FD (0=classical, 1=CAN FD)
              Bit 3: BRS (bit rate switch)
              Bit 4: ESI (error state indicator)
              Bit 5-7: Reserved
Byte 6-9:   CAN ID (32-bit, little-endian, bits 28:0 valid)
Byte 10:    DLC (4-bit, 0-15)
Byte 11-N:  Data bytes (length determined by DLC)
```

### FRAME_TX Payload (Command 0x02)

```
Byte 0:     Channel (0 or 1)
Byte 1:     Flags (same as FRAME_RX)
Byte 2-5:   CAN ID (32-bit, little-endian)
Byte 6:     DLC
Byte 7:     TX flags:
              Bit 0: One-shot (no retry)
              Bit 1: High priority
              Bit 2-7: Reserved
Byte 8-N:   Data bytes
```

### CONFIG_SET Payload (Command 0x04)

```
Byte 0:     Config ID:
              0x00: CAN bitrate (nominal)
              0x01: CAN FD data bitrate
              0x02: Channel mode (normal/listen-only)
              0x03: Termination enable
              0x04: BLE TX power
              0x05: LED brightness
              0x06: Filter add
              0x07: Filter clear
              0x08: Timestamp reset
Byte 1:     Channel (0, 1, or 0xFF for both)
Byte 2-N:   Config-specific data
```

### STATUS_RSP Payload (Command 0x08)

```
Byte 0-3:   Uptime (32-bit, seconds)
Byte 4-5:   Battery voltage (16-bit, mV, little-endian)
Byte 6:     BLE connected (0/1)
Byte 7:     USB connected (0/1)
Byte 8-9:   CAN0 bus status (16-bit):
              Bit 0-7: TEC
              Bit 8-15: REC
Byte 10-11: CAN1 bus status
Byte 12-15: CAN0 frame count (32-bit, total received)
Byte 16-19: CAN1 frame count
Byte 20-23: CAN0 overflow count
Byte 24-27: CAN1 overflow count
Byte 28:    Temperature (signed, °C, from nRF52840 TEMP sensor)
Byte 29-31: Reserved
```

### CRC-16/CCITT Implementation

```c
// CRC-16/CCITT (0xFFFF initial, polynomial 0x1021)
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
```

## 4.13 Injection Engine

### Script Format

Injection scripts are JSON-based, stored in PSRAM or external flash:

```json
{
  "name": "UDS Security Access Brute Force",
  "version": "1.0",
  "channel": 0,
  "bitrate": 500000,
  "fd_enable": false,
  "delay_us": 100,
  "repeat": 100,
  "frames": [
    {
      "id": 0x7E0,
      "ide": 0,
      "dlc": 8,
      "data": [0x02, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00],
      "delay_after_us": 50000
    },
    {
      "id": 0x7E0,
      "ide": 0,
      "dlc": 8,
      "data": [0x02, 0x27, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00],
      "delay_after_us": 50000
    }
  ]
}
```

### Injection Scheduler

```c
// injection_engine.h
typedef struct {
    uint8_t channel;
    can_frame_t frame;
    uint32_t delay_after_us;  // Delay after this frame before next
    uint32_t repeat_count;     // 0 = infinite
    uint32_t repeat_interval_us;
} injection_step_t;

typedef struct {
    char name[32];
    injection_step_t *steps;
    uint16_t step_count;
    uint16_t current_step;
    uint32_t total_repeats;
    uint32_t repeat_counter;
    bool running;
    bool paused;
} injection_script_t;

// API
int injection_script_load(injection_script_t *script, const uint8_t *json_data, uint16_t len);
int injection_script_start(injection_script_t *script);
int injection_script_stop(injection_script_t *script);
int injection_script_pause(injection_script_t *script);
int injection_script_resume(injection_script_t *script);
void injection_timer_isr(void);  // Called by TIMER1 at injection intervals
```

## 4.14 Error Handling Strategy

### Error Categories

| Category | Examples | Response |
|---|---|---|
| CAN Bus Errors | Bus-off, error-passive, arbitration loss | Log event, notify app, attempt recovery |
| SPI Errors | Timeout, CRC mismatch | Retry 3×, then reset MCP2518FD |
| QSPI Errors | PSRAM write failure | Mark buffer as degraded, notify app |
| BLE Errors | Disconnection, GATT error | Re-advertise, reconnect automatically |
| USB Errors | Enumeration failure, buffer overrun | Reset USBD peripheral, re-enumerate |
| Memory Errors | Stack overflow, heap exhaustion | WDT reset, log cause in retention RAM |
| Power Errors | Battery critical | Enter low-power mode, notify app, save state |

### Error Logging

```c
// Error log stored in retention RAM (survives WDT reset but not power cycle)
#define ERROR_LOG_MAGIC     0xDEADBEEF
#define ERROR_LOG_MAX       16

typedef struct {
    uint32_t magic;
    uint32_t count;
    struct {
        uint32_t timestamp;
        uint16_t code;
        uint16_t data;
    } entries[ERROR_LOG_MAX];
} error_log_t;

// Retention RAM section (not cleared on soft reset)
__attribute__((section(".noinit"))) error_log_t g_error_log;
```

## 4.15 Testing and Validation

### Unit Test Framework

```c
// test harness (compiled for host with mock HAL)
#ifdef HOST_TEST
void test_can_frame_serialize(void) {
    can_frame_t frame = {
        .id = 0x7E0,
        .dlc = 8,
        .data = {0x02, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
        .timestamp = 12345678,
        .flags = 0x00,
    };
    uint8_t buf[32];
    uint16_t len = protocol_serialize_frame_rx(&frame, 0, buf);
    ASSERT(len == 20);  // 4(ts) + 1(ch) + 1(flags) + 4(id) + 1(dlc) + 8(data) + 1(extra)
    ASSERT(buf[0] == 0xAA);  // Start byte
    ASSERT(buf[1] == 0x01);  // FRAME_RX command
}
#endif
```

### Hardware-in-the-Loop Testing

1. Connect CAN Creeper to known-good CAN bus (e.g., CANable, PCAN-USB)
2. Send 10,000 frames at 500 kbps from reference device
3. Verify CAN Creeper captures all frames with correct timestamps
4. Verify BLE NUS delivers frames without loss at 8000 fps
5. Verify USB CDC delivers frames without loss at full bus speed
6. Test injection: send frames from app, verify on reference device
7. Test MITM mode: insert between two CANable devices, verify forwarding
8. Test error recovery: induce bus-off, verify automatic recovery
9. Test power cycling: remove/reapply power, verify state restoration
10. Test OTA DFU: update firmware over BLE, verify new version boots

## 4.16 Device Tree / Configuration

### Flash Configuration Page (FCP)

```c
// Stored at 0x0006E000 (DFU settings page, repurposed for config)
typedef struct __attribute__((packed)) {
    uint32_t magic;             // 0xCAFECAFE
    uint32_t version;           // Config structure version
    char device_name[16];       // BLE device name
    uint8_t ble_tx_power;       // 0 = 0 dBm
    uint8_t default_channel;    // 0 or 1
    uint32_t can0_nominal_brp;
    uint32_t can0_nominal_tseg1;
    uint32_t can0_nominal_tseg2;
    uint32_t can0_nominal_sjw;
    uint32_t can0_data_brp;
    uint32_t can0_data_tseg1;
    uint32_t can0_data_tseg2;
    uint32_t can0_data_sjw;
    uint32_t can1_nominal_brp;
    uint32_t can1_nominal_tseg1;
    uint32_t can1_nominal_tseg2;
    uint32_t can1_nominal_sjw;
    uint32_t can1_data_brp;
    uint32_t can1_data_tseg1;
    uint32_t can1_data_tseg2;
    uint32_t can1_data_sjw;
    uint8_t can0_mode;          // 0=normal, 1=listen-only, 2=bus-monitor
    uint8_t can1_mode;
    uint8_t can0_termination;   // 0=off, 1=on
    uint8_t can1_termination;
    uint8_t oled_contrast;
    uint8_t led_brightness;
    uint32_t crc32;             // CRC over all fields except magic and crc32
} device_config_t;
```

## 4.17 Security Features

### Firmware Signature Verification

```c
// Ed25519 signature verification for OTA images
// Public key embedded in firmware (provisioned at factory)
static const uint8_t g_ota_public_key[32] = {
    0x12, 0x34, 0x56, ...  // 32-byte Ed25519 public key
};

int dfu_verify_image(const uint8_t *image, uint32_t image_len,
                     const uint8_t *signature) {
    // Verify Ed25519 signature over SHA-256 hash of image
    // Using micro-ecc or tweetnacl-embedded library
    return ed25519_verify(signature, image, image_len, g_ota_public_key);
}
```

### Secure Boot

- APPROTECT fuse set in production → SWD disabled
- BPROT regions configured to protect bootloader and SoftDevice from application writes
- UICR write locked after provisioning

### BLE Security

- LE Secure Connections pairing (AES-CCM, ECDH P-256)
- Numeric comparison for MITM protection
- Bonding supported (store keys in Flash)
- No Just Works pairing (requires user interaction)

## 4.18 Performance Optimizations

### Critical Path Optimizations

1. **CAN Frame Reception**: SPI read uses EasyDMA, zero CPU overhead during transfer. Frame processing in deferred SWI handler.
2. **PSRAM Ring Buffer Write**: QSPI write uses EasyDMA. Write pointer update is atomic (single 32-bit write).
3. **BLE NUS TX**: Frame queued in software FIFO. BLE TX triggered on BLE_GATTS_EVT_HVN_TX_COMPLETE (previous notification complete).
4. **USB CDC TX**: Double-buffered. While one buffer transmits via EasyDMA, next frame prepared in other buffer.
5. **Timestamp Capture**: Hardware TIMER0 running at 1 MHz. Capture via TASKS_CAPTURE triggered in GPIOTE ISR (before any processing).

### Measured Latencies (Target)

| Operation | Target Latency | Notes |
|---|---|---|
| CAN frame arrival → timestamp capture | <2 µs | GPIOTE ISR entry + TIMER capture |
| CAN frame arrival → PSRAM storage | <50 µs | SPI read (8 MHz, ~32 bytes) + QSPI write |
| CAN frame arrival → BLE notification queued | <100 µs | Full processing pipeline |
| BLE notification queued → over-the-air | <15 ms | Depends on connection interval |
| App TX command → CAN frame on bus | <500 µs | SPI write + MCP2518FD arbitration |
