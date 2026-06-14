# Phase 4 — Software Stack: CAN Bus Infiltrator

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On (OBD-II 12V or USB-C 5V)
  │
  ├── MP2307DN buck converter stabilizes → 5.0 V
  ├── RT9193 LDO stabilizes → 3.3 V
  ├── STM32F407 NRST de-asserts (10 kΩ pull-up + 100 nF delay ~5 ms)
  │
  ├── STM32 Bootloader (System Memory @ 0x1FFF0000)
  │   ├── Check BOOT0 pin (pulled LOW → boot from Flash)
  │   ├── Check BOOT1 pin (pulled LOW → boot from Flash)
  │   └── Jump to Flash @ 0x08000000
  │
  ├── Custom Bootloader (Flash @ 0x08000000, 16 KB)
  │   ├── Initialize .data, .bss, stack
  │   ├── Verify application CRC32 in flash
  │   ├── Check DFU flag in backup register (RTC BKPSRAM)
  │   │   ├── If DFU flag set → enter USB DFU mode (STM32 built-in DFU)
  │   │   └── If no flag → continue to application
  │   ├── Check USB-CDC 'DFU' command received within 500 ms timeout
  │   ├── Validate Ed25519 signature on application region
  │   └── Jump to application @ 0x08004000
  │
  └── Application (Flash @ 0x08004000)
      ├── Copy .data from Flash to SRAM
      ├── Zero .bss
      ├── Call SystemInit() — set clocks
      ├── Call main() — initialize drivers
      └── Enter main loop
```

### 1.2 Memory Map

| Region | Address | Size | Content |
|---|---|---|---|
| Bootloader | 0x08000000 | 16 KB | Signed DFU bootloader |
| App Flash | 0x08004000 | 992 KB | Application code |
| App CRC | 0x080FFC00 | 4 bytes | CRC32 of app region |
| App Signature | 0x080FFC04 | 64 bytes | Ed25519 signature |
| Config Sector | 0x080FFD00 | 512 bytes | Device config (CAN baud, filters) |
| Option Bytes | 0x1FFFC000 | — | Write protection, read protection level 1 |
| SRAM | 0x20000000 | 128 KB | Main SRAM |
| CCM SRAM | 0x10000000 | 64 KB | Core-coupled (fuzzer engine) |
| Backup SRAM | 0x40024000 | 4 KB | Battery-backed (DFU flags) |

## 2. Clock Configuration

### 2.1 Clock Tree

```
HSE = 8 MHz (Y1 crystal)
  │
  ├── PLL Configuration:
  │   PLLM = 8   → PLL input = 1 MHz
  │   PLLN = 336 → PLL VCO = 336 MHz
  │   PLLP = 2   → SYSCLK = 168 MHz
  │   PLLQ = 7   → USB OTG FS clock = 48 MHz
  │
  ├── AHB Prescaler = 1   → HCLK = 168 MHz
  ├── APB1 Prescaler = 4  → APB1 = 42 MHz (PCLK1)
  ├── APB2 Prescaler = 2  → APB2 = 84 MHz (PCLK2)
  │
  ├── LSE = 32.768 kHz (Y2 crystal) → RTC
  └── HSI = 16 MHz (fallback, not used in normal operation)
```

### 2.2 Peripheral Clock Enables

```c
// In board.h — RCC enable bits

// AHB1 (GPIO ports)
#define RCC_AHB1ENR_GPIOA    (1 << 0)
#define RCC_AHB1ENR_GPIOB    (1 << 1)
#define RCC_AHB1ENR_GPIOC    (1 << 2)
#define RCC_AHB1ENR_GPIOD    (1 << 3)
#define RCC_AHB1ENR_GPIOE    (1 << 4)

// APB1 (CAN, SPI, I2C, etc.)
#define RCC_APB1ENR_CAN1     (1 << 25)
#define RCC_APB1ENR_CAN2     (1 << 26)
#define RCC_APB1ENR_SPI3     (1 << 15)
#define RCC_APB1ENR_I2C1     (1 << 21)
#define RCC_APB1ENR_TIM2     (1 << 0)
#define RCC_APB1ENR_TIM3     (1 << 1)

// APB2 (USB, etc.)
#define RCC_APB2ENR_SYSCFG   (1 << 14)
#define RCC_APB2ENR_USART1   (1 << 4)

// AHB1 (DMA)
#define RCC_AHB1ENR_DMA2     (1 << 22)
#define RCC_AHB1ENR_BKPSRAM  (1 << 18)
```

## 3. GPIO Initialization

### 3.1 Complete Pin Configuration

```c
typedef struct {
    GPIO_TypeDef *port;
    uint8_t pin;
    uint8_t mode;       // 0=Input, 1=Output, 2=Alternate, 3=Analog
    uint8_t alternate;  // AF number (0-15)
    uint8_t speed;      // 0=Low, 1=Medium, 2=High, 3=VeryHigh
    uint8_t pull;       // 0=NoPull, 1=PullUp, 2=PullDown
    uint8_t otype;      // 0=PushPull, 1=OpenDrain
} gpio_config_t;

static const gpio_config_t gpio_configs[] = {
    // CAN Channel 1
    {GPIOB,  0, 2, AF9,  3, 0, 0},  // CAN1_RX  — AF9
    {GPIOB,  1, 2, AF9,  3, 0, 0},  // CAN1_TX  — AF9

    // CAN Channel 2
    {GPIOE, 10, 2, AF9,  3, 0, 0},  // CAN2_RX  — AF9
    {GPIOE, 11, 2, AF9,  3, 0, 0},  // CAN2_TX  — AF9

    // SPI3 to nRF52840
    {GPIOB,  3, 2, AF6,  3, 0, 0},  // SPI3_SCK — AF6
    {GPIOB,  4, 2, AF6,  3, 0, 0},  // SPI3_MISO— AF6
    {GPIOB,  5, 2, AF6,  3, 0, 0},  // SPI3_MOSI— AF6
    {GPIOD,  7, 1, 0,   3, 1, 0},  // SPI3_NSS — GPIO output, pull-up

    // USB OTG FS
    {GPIOA, 11, 2, AF10, 3, 0, 0}, // USB_DM   — AF10
    {GPIOA, 12, 2, AF10, 3, 0, 0}, // USB_DP   — AF10

    // I2C1 (EEPROM)
    {GPIOA, 15, 2, AF4,  2, 1, 1}, // I2C1_SCL — AF4, OD, pull-up
    {GPIOB,  6, 2, AF4,  2, 1, 1}, // I2C1_SDA — AF4, OD, pull-up

    // SDIO (microSD)
    {GPIOC,  8, 2, AF12, 3, 0, 0}, // SDIO_D0
    {GPIOC,  9, 2, AF12, 3, 0, 0}, // SDIO_D1
    {GPIOC, 10, 2, AF12, 3, 0, 0}, // SDIO_CK
    {GPIOC, 11, 2, AF12, 3, 0, 0}, // SDIO_D2  (using PC11)
    {GPIOC, 12, 2, AF12, 3, 0, 0}, // SDIO_D3  (using PC12)
    {GPIOD,  2, 2, AF12, 3, 0, 0}, // SDIO_CMD

    // SWD Debug
    {GPIOA, 13, 2, AF0,  0, 0, 0}, // SWDIO    — AF0
    {GPIOA, 14, 2, AF0,  0, 0, 0}, // SWCLK    — AF0

    // nRF control
    {GPIOB,  7, 1, 0,   1, 0, 0},  // nRF_BOOT — GPIO output
    {GPIOE,  1, 0, 0,   0, 1, 0},  // nRF_IRQ  — GPIO input, pull-up

    // CAN transceiver standby
    {GPIOE,  2, 1, 0,   1, 0, 0},  // CAN1_STB — GPIO output (LOW = active)
    {GPIOE,  3, 1, 0,   1, 0, 0},  // CAN2_STB — GPIO output

    // LED data
    {GPIOE,  7, 1, 0,   3, 0, 0},  // LED_DIN  — GPIO output (WS2812B)

    // Reset
    {GPIOA,  0, 0, 0,   0, 1, 0},  // NRST     — input w/ pull-up (handled by HW)
};
```

## 4. MMIO Registers — bxCAN (STM32F407)

### 4.1 CAN1 Register Map (Base: 0x40006400)

| Offset | Name | Bits | Description |
|---|---|---|---|
| 0x000 | CAN_MCR | 32 | Master control register |
| 0x004 | CAN_MSR | 32 | Master status register |
| 0x008 | CAN_TSR | 32 | Transmit status register |
| 0x00C | CAN_RF0R | 32 | Receive FIFO 0 register |
| 0x010 | CAN_RF1R | 32 | Receive FIFO 1 register |
| 0x014 | CAN_IER | 32 | Interrupt enable register |
| 0x018 | CAN_ESR | 32 | Error status register |
| 0x01C | CAN_BTR | 32 | Bit timing register |
| 0x180 | CAN_S1 | 80 | Filter 0 mailbox (std ID) |
| 0x1AC | CAN_Fi1 | 80 | Filter bank registers |
| 0x200 | CAN_TDLHR | 32 | TX mailbox 0 high |
| 0x204 | CAN_TDLLR | 32 | TX mailbox 0 low |
| 0x208 | CAN_TDLHR | 32 | TX mailbox 1 high |
| 0x20C | CAN_TDHLR | 32 | TX mailbox 1 low |
| 0x210 | CAN_TDLHR2| 32 | TX mailbox 2 high |
| 0x214 | CAN_TDHHR2| 32 | TX mailbox 2 low |
| 0x240 | CAN_RDLHR | 32 | RX FIFO 0 high |
| 0x244 | CAN_RDLLR | 32 | RX FIFO 0 low |
| 0x280 | CAN_RDLHR1| 32 | RX FIFO 1 high |
| 0x284 | CAN_RDLLR1| 32 | RX FIFO 1 low |

### 4.2 Key Register Bit Definitions

```c
// CAN_MCR bits
#define CAN_MCR_INRQ     (1 << 0)   // Initialization request
#define CAN_MCR_SLEEP    (1 << 1)   // Sleep mode
#define CAN_MCR_TXFP     (1 << 2)   // Transmit FIFO priority
#define CAN_MCR_RFLM     (1 << 3)   // Receive FIFO locked mode
#define CAN_MCR_NART     (1 << 4)   // No automatic retransmission
#define CAN_MCR_AWUM     (1 << 5)   // Auto wake-up mode
#define CAN_MCR_ABOM     (1 << 6)   // Auto bus-off management
#define CAN_MCR_TTCM     (1 << 7)   // Time triggered communication mode
#define CAN_MCR_RESET    (1 << 15)  // Software reset

// CAN_BTR bits (for 500 kbps with APB1 = 42 MHz)
#define CAN_BTR_BRP(x)   ((x) << 0)    // Baud rate prescaler
#define CAN_BTR_TS1(x)   ((x) << 16)    // Time segment 1
#define CAN_BTR_TS2(x)   ((x) << 20)    // Time segment 2
#define CAN_BTR_SJW(x)   ((x) << 24)    // Resynchronization jump width
// For 500 kbps: BRP=5, TS1=13, TS2=2, SJW=1 → 42MHz/(5+1)/(1+13+2) = 500kbps
// For 250 kbps: BRP=11, TS1=13, TS2=2, SJW=1
// For 1 Mbps:   BRP=2, TS1=13, TS2=2, SJW=1

// CAN_TSR bits
#define CAN_TSR_TME0     (1 << 26)  // TX mailbox 0 empty
#define CAN_TSR_TME1     (1 << 27)  // TX mailbox 1 empty
#define CAN_TSR_TME2     (1 << 28)  // TX mailbox 2 empty
#define CAN_TSR_RQCP0    (1 << 0)   // Request completed mailbox 0

// CAN_IER bits
#define CAN_IER_FMPIE0   (1 << 1)   // FIFO 0 message pending interrupt
#define CAN_IER_FMPIE1   (1 << 4)   // FIFO 1 message pending interrupt
#define CAN_IER_TMEIE    (1 << 1)   // Transmit mailbox empty interrupt
#define CAN_IER_ERRIE    (1 << 2)   // Error interrupt
#define CAN_IER_BOFIE    (1 << 9)   // Bus-off interrupt
#define CAN_IER_EPVIE    (1 << 10)  // Error passive interrupt
#define CAN_IER_EWGIE    (1 << 11)  // Error warning interrupt

// CAN_ESR bits
#define CAN_ESR_EWGF      (1 << 0)  // Error warning flag
#define CAN_ESR_EPVF      (1 << 1)  // Error passive flag
#define CAN_ESR_BOFF      (1 << 2)  // Bus-off flag
#define CAN_ESR_REC(x)    (((x) >> 24) & 0xFF)  // Receive error counter
#define CAN_ESR_TEC(x)    (((x) >> 16) & 0xFF)  // Transmit error counter
```

### 4.3 CAN Filter Configuration (for sniffing all frames)

```c
// CAN filter register base (CAN1 only, shared by CAN1 and CAN2)
#define CAN_FILTER_BASE   0x40006400 + 0x200  // 0x40006600

// Accept all frames (sniffer mode)
// Filter bank 0: Accept all 11-bit IDs
// F0R1 = 0x00000000, F0R2 = 0x00000000
// Mask  = 0x00000000 (don't care = accept all)
void can_set_sniffer_filter(uint8_t can_num) {
    volatile uint32_t *fmr  = (uint32_t*)(CAN1_BASE + 0x214);
    volatile uint32_t *fa1r = (uint32_t*)(CAN1_BASE + 0x224);
    volatile uint32_t *fs1r = (uint32_t*)(CAN1_BASE + 0x20C);
    volatile uint32_t *fm1r = (uint32_t*)(CAN1_BASE + 0x210);

    uint8_t bank = (can_num == 1) ? 0 : 14; // CAN1 uses banks 0-13, CAN2 uses 14-27

    *fmr |= (1 << 0);             // FINIT = 1 (filter init mode)
    *fa1r |= (1 << bank);          // Activate filter bank
    *fs1r &= ~(1 << bank);         // 16-bit scale (single 32-bit = 0)
    *fm1r &= ~(1 << bank);         // Mask mode (identifier mode = 0)

    // Set filter values (accept all)
    volatile uint32_t *fr = (uint32_t*)(CAN1_BASE + 0x240 + bank * 8);
    fr[0] = 0x00000000;  // FR1
    fr[1] = 0x00000000;  // FR2

    *fmr &= ~(1 << 0);            // FINIT = 0 (exit filter init)
}
```

## 5. Device Drivers

### 5.1 CAN Driver (can.c / can.h)

```c
// can.h — CAN bus driver for STM32F407 bxCAN
#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>
#include <stddef.h>

#define CAN_STD_ID   0
#define CAN_EXT_ID   1

#define CAN_MAX_DATA_LEN  8

typedef struct {
    uint32_t id;            // 11-bit or 29-bit CAN ID
    uint8_t  id_type;       // CAN_STD_ID or CAN_EXT_ID
    uint8_t  dlc;           // Data Length Code (0-8)
    uint8_t  data[8];       // Payload
    uint8_t  rtr;           // Remote Transmission Request
    uint32_t timestamp_us;  // Hardware timestamp (TIM2)
} can_frame_t;

typedef enum {
    CAN_BAUD_125K  = 0,
    CAN_BAUD_250K  = 1,
    CAN_BAUD_500K  = 2,
    CAN_BAUD_1M    = 3,
    CAN_BAUD_CUSTOM = 4
} can_baudrate_t;

typedef enum {
    CAN_MODE_NORMAL    = 0,
    CAN_MODE_LISTEN    = 1,  // Listen-only (silent sniffer)
    CAN_MODE_LOOPBACK  = 2,  // Internal loopback (test)
    CAN_MODE_SILENT_LOOPBACK = 3
} can_mode_t;

// Callback for received frames
typedef void (*can_rx_callback_t)(const can_frame_t *frame, uint8_t channel);

int  can_init(uint8_t channel, can_baudrate_t baud, can_mode_t mode);
int  can_set_filter(uint8_t channel, uint32_t id, uint32_t mask, uint8_t bank);
int  can_set_sniffer(uint8_t channel);  // Accept all frames
int  can_transmit(uint8_t channel, const can_frame_t *frame);
int  can_receive(uint8_t channel, can_frame_t *frame, uint32_t timeout_ms);
void can_register_rx_callback(uint8_t channel, can_rx_callback_t cb);
void can_enable_interrupts(uint8_t channel);
void can_disable_interrupts(uint8_t channel);
uint8_t can_get_error_state(uint8_t channel);
void can_reset_bus(uint8_t channel);

#endif // CAN_DRIVER_H
```

```c
// can.c — CAN bus driver implementation (excerpt)
#include "can.h"
#include "registers.h"
#include "board.h"

#define CAN1_BASE  0x40006400
#define CAN2_BASE  0x40006800

#define CAN_MCR(b)    *(volatile uint32_t*)((b) + 0x00)
#define CAN_MSR(b)    *(volatile uint32_t*)((b) + 0x04)
#define CAN_TSR(b)    *(volatile uint32_t*)((b) + 0x08)
#define CAN_RF0R(b)   *(volatile uint32_t*)((b) + 0x0C)
#define CAN_IER(b)    *(volatile uint32_t*)((b) + 0x14)
#define CAN_ESR(b)    *(volatile uint32_t*)((b) + 0x18)
#define CAN_BTR(b)    *(volatile uint32_t*)((b) + 0x1C)

// TX mailbox registers
#define CAN_TIxR(b, n)  *(volatile uint32_t*)((b) + 0x180 + (n)*0x10)
#define CAN_TDLxR(b, n) *(volatile uint32_t*)((b) + 0x184 + (n)*0x10)
#define CAN_TDHxR(b, n) *(volatile uint32_t*)((b) + 0x188 + (n)*0x10)

// RX FIFO registers
#define CAN_RIxR(b, f)  *(volatile uint32_t*)((b) + 0x1B0 + (f)*0x10)
#define CAN_RDLxR(b, f) *(volatile uint32_t*)((b) + 0x1B4 + (f)*0x10)
#define CAN_RDHxR(b, f) *(volatile uint32_t*)((b) + 0x1B8 + (f)*0x10)

static can_rx_callback_t rx_callbacks[2] = {NULL, NULL};

static uint32_t get_base(uint8_t channel) {
    return (channel == 0) ? CAN1_BASE : CAN2_BASE;
}

static void can_set_baud(uint32_t base, can_baudrate_t baud) {
    uint32_t btr = 0;
    switch (baud) {
        case CAN_BAUD_125K:  btr = CAN_BTR_BRP(11) | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1); break;
        case CAN_BAUD_250K:  btr = CAN_BTR_BRP(5)  | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1); break;
        case CAN_BAUD_500K:  btr = CAN_BTR_BRP(5)  | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1); break;
        case CAN_BAUD_1M:    btr = CAN_BTR_BRP(2)  | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1); break;
        default: break;
    }
    CAN_BTR(base) = btr;
}

int can_init(uint8_t channel, can_baudrate_t baud, can_mode_t mode) {
    uint32_t base = get_base(channel);

    // Enable CAN peripheral clock
    if (channel == 0) RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    else { RCC->APB1ENR |= RCC_APB1ENR_CAN1EN | RCC_APB1ENR_CAN2EN; }

    // Exit sleep mode
    CAN_MCR(base) &= ~CAN_MCR_SLEEP;

    // Request initialization
    CAN_MCR(base) |= CAN_MCR_INRQ;
    uint32_t timeout = 10000;
    while (!(CAN_MSR(base) & (1 << 0))) { if (--timeout == 0) return -1; }

    // Configure: no auto-retransmit for injection, time-triggered for timestamping
    CAN_MCR(base) = CAN_MCR_NART | CAN_MCR_ABOM | CAN_MCR_TTCM;

    // Set baud rate
    can_set_baud(base, baud);

    // Set mode
    if (mode == CAN_MODE_LISTEN) CAN_BTR(base) |= (1 << 30);       // SILM
    if (mode == CAN_MODE_LOOPBACK) CAN_BTR(base) |= (1 << 30) | (1 << 31); // LBKM + SILM

    // Exit initialization
    CAN_MCR(base) &= ~CAN_MCR_INRQ;
    timeout = 10000;
    while ((CAN_MSR(base) & (1 << 0))) { if (--timeout == 0) return -1; }

    // Configure sniffer filter (accept all)
    can_set_sniffer(channel);

    // Enable FIFO message pending interrupt
    CAN_IER(base) |= CAN_IER_FMPIE0 | CAN_IER_EWGIE | CAN_IER_EPVIE | CAN_IER_BOFIE;

    return 0;
}

int can_transmit(uint8_t channel, const can_frame_t *frame) {
    uint32_t base = get_base(channel);

    // Find empty mailbox
    uint32_t tsr = CAN_TSR(base);
    uint8_t mailbox;
    if (tsr & CAN_TSR_TME0)      mailbox = 0;
    else if (tsr & CAN_TSR_TME1)  mailbox = 1;
    else if (tsr & CAN_TSR_TME2)  mailbox = 2;
    else return -1; // All mailboxes full

    // Build TIxR register
    uint32_t tir = 0;
    if (frame->id_type == CAN_EXT_ID) {
        tir |= (1 << 2);  // IDE bit
        tir |= (frame->id << 3);
    } else {
        tir |= (frame->id << 21);
    }
    if (frame->rtr) tir |= (1 << 1);  // RTR bit
    tir |= (frame->dlc << 16);

    CAN_TIxR(base, mailbox) = tir;
    CAN_TDLxR(base, mailbox) = *(uint32_t*)&frame->data[0];
    CAN_TDHxR(base, mailbox) = *(uint32_t*)&frame->data[4];

    // Request transmission
    CAN_TIxR(base, mailbox) |= (1 << 0); // TXRQ bit

    return 0;
}

// CAN1 RX interrupt handler
void CAN1_RX0_IRQHandler(void) {
    can_frame_t frame;
    uint32_t base = CAN1_BASE;

    if (CAN_RF0R(base) & (1 << 0)) { // FMP0: message pending
        uint32_t rir = CAN_RIxR(base, 0);
        frame.id_type = (rir & (1 << 2)) ? CAN_EXT_ID : CAN_STD_ID;
        if (frame.id_type == CAN_EXT_ID)
            frame.id = (rir >> 3) & 0x1FFFFFFF;
        else
            frame.id = (rir >> 21) & 0x7FF;
        frame.rtr = (rir >> 1) & 1;
        frame.dlc = (rir >> 16) & 0xF;
        uint32_t rdl = CAN_RDLxR(base, 0);
        uint32_t rdh = CAN_RDHxR(base, 0);
        for (int i = 0; i < 4; i++) frame.data[i]   = (rdl >> (i*8)) & 0xFF;
        for (int i = 0; i < 4; i++) frame.data[i+4] = (rdh >> (i*8)) & 0xFF;

        frame.timestamp_us = TIM2->CNT; // Hardware timestamp from TIM2

        // Release FIFO
        CAN_RF0R(base) |= (1 << 5); // RFOM0

        if (rx_callbacks[0]) rx_callbacks[0](&frame, 0);
    }
}
```

### 5.2 SPI/BLE Driver (ble_spi.c / ble_spi.h)

```c
// ble_spi.h — SPI interface to nRF52840 BLE co-processor
#ifndef BLE_SPI_DRIVER_H
#define BLE_SPI_DRIVER_H

#include <stdint.h>

#define BLE_SPI_BASE    SPI3_BASE
#define BLE_PKT_MAX    256

// Command opcodes (STM32 → nRF)
#define BLE_CMD_CONNECT_SCAN    0x01
#define BLE_CMD_DISCONNECT      0x02
#define BLE_CMD_SEND_DATA       0x03
#define BLE_CMD_GET_STATUS      0x04
#define BLE_CMD_SET_ADV_DATA    0x05
#define BLE_CMD_START_ADV       0x06
#define BLE_CMD_STOP_ADV        0x07

// Response codes (nRF → STM32)
#define BLE_RSP_ACK            0x00
#define BLE_RSP_NACK           0x01
#define BLE_RSP_DATA           0x02
#define BLE_RSP_CONNECTED      0x03
#define BLE_RSP_DISCONNECTED   0x04

typedef struct {
    uint8_t  opcode;
    uint8_t  seq;
    uint16_t length;
    uint8_t  data[BLE_PKT_MAX];
} __attribute__((packed)) ble_packet_t;

int  ble_spi_init(void);
int  ble_spi_send_command(uint8_t opcode, const uint8_t *data, uint16_t len);
int  ble_spi_read_response(ble_packet_t *pkt, uint32_t timeout_ms);
void ble_spi_register_callback(void (*cb)(const ble_packet_t *));
int  ble_spi_start_advertising(const char *name);
int  ble_spi_connect(const uint8_t *addr);
int  ble_send_can_frame(const can_frame_t *frame);

#endif // BLE_SPI_DRIVER_H
```

```c
// ble_spi.c — SPI BLE driver implementation
#include "ble_spi.h"
#include "registers.h"
#include "board.h"
#include <string.h>

#define SPI3_BASE     0x40003C00
#define SPI3_CR1      *(volatile uint32_t*)(SPI3_BASE + 0x00)
#define SPI3_CR2      *(volatile uint32_t*)(SPI3_BASE + 0x04)
#define SPI3_SR       *(volatile uint32_t*)(SPI3_BASE + 0x08)
#define SPI3_DR       *(volatile uint32_t*)(SPI3_BASE + 0x0C)

#define SPI_CR1_SPE    (1 << 6)
#define SPI_CR1_MSTR   (1 << 2)
#define SPI_CR1_BR(x)  ((x) << 3)
#define SPI_CR1_CPHA   (1 << 0)
#define SPI_CR1_CPOL   (1 << 1)
#define SPI_SR_TXE     (1 << 1)
#define SPI_SR_RXNE    (1 << 0)
#define SPI_SR_BSY     (1 << 7)

#define NSS_PIN        7  // PD7
#define NSS_PORT        GPIOD

static void (*rx_callback)(const ble_packet_t *) = NULL;
static volatile uint8_t ble_seq = 0;

static void spi_select(void) {
    NSS_PORT->ODR &= ~(1 << NSS_PIN);  // CS low
    for (volatile int i = 0; i < 10; i++); // ~0.5 µs delay
}

static void spi_deselect(void) {
    NSS_PORT->ODR |= (1 << NSS_PIN);   // CS high
}

static uint8_t spi_xfer(uint8_t tx_byte) {
    while (!(SPI3_SR & SPI_SR_TXE));
    SPI3_DR = tx_byte;
    while (!(SPI3_SR & SPI_SR_RXNE));
    return (uint8_t)SPI3_DR;
}

int ble_spi_init(void) {
    // Enable SPI3 clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

    // Configure SPI3: Master, CPOL=0, CPHA=0, 8-bit, MSB first
    SPI3_CR1 = SPI_CR1_MSTR | SPI_CR1_BR(2) |  // fPCLK/8 = 5.25 MHz
               (0 << 0) | (0 << 1);  // CPHA=0, CPOL=0
    SPI3_CR2 = (1 << 2);  // SSOE disabled, software NSS management

    SPI3_CR1 |= SPI_CR1_SPE;

    // Configure NSS pin as GPIO output
    GPIOD->MODER &= ~(3 << (NSS_PIN * 2));
    GPIOD->MODER |= (1 << (NSS_PIN * 2));  // Output mode
    NSS_PORT->ODR |= (1 << NSS_PIN);  // CS high (idle)

    return 0;
}

int ble_spi_send_command(uint8_t opcode, const uint8_t *data, uint16_t len) {
    spi_select();

    // Header: [0xAA] [opcode] [seq] [len_hi] [len_lo]
    spi_xfer(0xAA);                // Sync byte
    spi_xfer(opcode);
    spi_xfer(ble_seq++);
    spi_xfer((len >> 8) & 0xFF);
    spi_xfer(len & 0xFF);

    // Payload
    for (uint16_t i = 0; i < len; i++) {
        spi_xfer(data[i]);
    }

    // Checksum (XOR of all bytes)
    spi_deselect();
    return 0;
}

int ble_spi_read_response(ble_packet_t *pkt, uint32_t timeout_ms) {
    spi_select();

    // Wait for sync byte
    uint8_t sync;
    uint32_t timeout = timeout_ms * 1680; // Approximate loop count
    do {
        sync = spi_xfer(0x00);
        if (sync == 0xAA) break;
    } while (--timeout);

    if (timeout == 0) { spi_deselect(); return -1; }

    pkt->opcode = spi_xfer(0x00);
    pkt->seq    = spi_xfer(0x00);
    pkt->length = (spi_xfer(0x00) << 8) | spi_xfer(0x00);

    if (pkt->length > BLE_PKT_MAX) { spi_deselect(); return -2; }

    for (uint16_t i = 0; i < pkt->length; i++) {
        pkt->data[i] = spi_xfer(0x00);
    }

    spi_deselect();
    return 0;
}
```

### 5.3 SD Card Driver (sdcard.c / sdcard.h)

```c
// sdcard.h — SDIO SD card driver
#ifndef SDCARD_DRIVER_H
#define SDCARD_DRIVER_H

#include <stdint.h>

#define SDCARD_BLOCK_SIZE  512

typedef enum {
    SD_OK = 0,
    SD_ERROR_CMD,
    SD_ERROR_TIMEOUT,
    SD_ERROR_CRC,
    SD_ERROR_WRITE_PROTECTED,
    SD_ERROR_NO_CARD
} sd_result_t;

typedef struct {
    uint32_t card_type;     // 0=unknown, 1=SDv1, 2=SDv2, 3=SDHC
    uint32_t block_count;   // Total number of blocks
    uint32_t block_size;    // 512 bytes
    uint8_t  cid[16];       // Card ID register
    uint8_t  csd[16];       // Card specific data
} sd_card_info_t;

sd_result_t sd_init(void);
sd_result_t sd_read_blocks(uint32_t block_addr, uint8_t *buf, uint32_t count);
sd_result_t sd_write_blocks(uint32_t block_addr, const uint8_t *buf, uint32_t count);
sd_result_t sd_get_info(sd_card_info_t *info);
sd_result_t sd_erase_blocks(uint32_t start, uint32_t end);

#endif // SDCARD_DRIVER_H
```

```c
// sdcard.c — SDIO SD card driver implementation (excerpt)
#include "sdcard.h"
#include "registers.h"
#include "board.h"
#include <string.h>

// SDIO register base
#define SDIO_BASE      0x40012C00
#define SDIO_POWER     *(volatile uint32_t*)(SDIO_BASE + 0x00)
#define SDIO_CLKCR     *(volatile uint32_t*)(SDIO_BASE + 0x04)
#define SDIO_ARG       *(volatile uint32_t*)(SDIO_BASE + 0x08)
#define SDIO_CMD       *(volatile uint32_t*)(SDIO_BASE + 0x0C)
#define SDIO_RESPCMD   *(volatile uint32_t*)(SDIO_BASE + 0x10)
#define SDIO_RESP1     *(volatile uint32_t*)(SDIO_BASE + 0x14)
#define SDIO_RESP2     *(volatile uint32_t*)(SDIO_BASE + 0x18)
#define SDIO_RESP3     *(volatile uint32_t*)(SDIO_BASE + 0x1C)
#define SDIO_RESP4     *(volatile uint32_t*)(SDIO_BASE + 0x20)
#define SDIO_DTIMER    *(volatile uint32_t*)(SDIO_BASE + 0x24)
#define SDIO_DLEN      *(volatile uint32_t*)(SDIO_BASE + 0x28)
#define SDIO_DCTRL     *(volatile uint32_t*)(SDIO_BASE + 0x2C)
#define SDIO_DCOUNT    *(volatile uint32_t*)(SDIO_BASE + 0x30)
#define SDIO_STA       *(volatile uint32_t*)(SDIO_BASE + 0x34)
#define SDIO_ICR       *(volatile uint32_t*)(SDIO_BASE + 0x38)
#define SDIO_MASK      *(volatile uint32_t*)(SDIO_BASE + 0x3C)
#define SDIO_FIFOCNT   *(volatile uint32_t*)(SDIO_BASE + 0x48)
#define SDIO_FIFO      *(volatile uint32_t*)(SDIO_BASE + 0x80)

#define SDIO_POWER_PWRCTRL  (3 << 0)    // Power on
#define SDIO_CLKCR_CLKDIV(x) ((x) << 0) // Clock divider
#define SDIO_CLKCR_CLKEN     (1 << 8)   // Clock enable
#define SDIO_CLKCR_PWRSAV    (1 << 9)   // Power saving
#define SDIO_CLKCR_WIDBUS_4  (1 << 11)  // 4-bit bus
#define SDIO_DCTRL_DTEN      (1 << 0)   // Data transfer enable
#define SDIO_DCTRL_DTDIR     (1 << 1)   // Data direction (1=card→host)
#define SDIO_DCTRL_DTMODE    (1 << 2)   // Data transfer mode (0=block)
#define SDIO_DCTRL_DBLOCKSIZE(x) ((x) << 4) // Block size (9=512)

static sd_card_info_t sd_info;

sd_result_t sd_init(void) {
    // Enable SDIO clock
    RCC->APB2ENR |= (1 << 11);  // SDIOEN
    RCC->AHB1ENR |= (1 << 8);   // DMA2EN

    // Power off, then on
    SDIO_POWER = 0;
    for (volatile int i = 0; i < 10000; i++);
    SDIO_POWER = SDIO_POWER_PWRCTRL;

    // Clock: 400 kHz for identification
    SDIO_CLKCR = SDIO_CLKCR_CLKDIV(118) | SDIO_CLKCR_CLKEN;

    // Send CMD0 (GO_IDLE_STATE)
    SDIO_ARG = 0;
    SDIO_CMD = (0 << 10) | (0 << 6) | 0; // CMD0, no response
    // ... (full card identification sequence omitted for brevity)

    return SD_OK;
}

sd_result_t sd_read_blocks(uint32_t block_addr, uint8_t *buf, uint32_t count) {
    // Configure DMA2 Stream 3 Channel 4 for SDIO
    DMA2_Stream3->CR = (4 << 25) |   // Channel 4 (SDIO)
                       (2 << 16) |    // 32-bit memory size
                       (2 << 13) |    // 32-bit peripheral size
                       (1 << 10) |    // Memory increment
                       (1 << 8)  |    // Circular mode
                       (0 << 6);      // Peripheral-to-memory
    DMA2_Stream3->PAR = (uint32_t)&SDIO_FIFO;
    DMA2_Stream3->M0AR = (uint32_t)buf;
    DMA2_Stream3->NDTR = count * (512 / 4); // Number of 32-bit transfers
    DMA2_Stream3->CR |= (1 << 0); // Enable DMA

    // Set block size and data length
    SDIO_DCTRL = SDIO_DCTRL_DBLOCKSIZE(9); // 512 bytes
    SDIO_DLEN = count * 512;

    // Send CMD17 (READ_SINGLE_BLOCK) or CMD18 (READ_MULTIPLE_BLOCK)
    SDIO_ARG = (sd_info.card_type >= 3) ? block_addr : block_addr * 512;
    if (count == 1) {
        SDIO_CMD = (1 << 10) | (1 << 6) | 17; // CMD17, short response
    } else {
        SDIO_CMD = (1 << 10) | (1 << 6) | 18; // CMD18, short response
    }

    // Enable data transfer
    SDIO_DCTRL |= SDIO_DCTRL_DTEN | SDIO_DCTRL_DTDIR;

    // Wait for DMA completion (handled by DMA interrupt)
    // In production: use DMA interrupt with timeout

    return SD_OK;
}
```

## 6. Device Tree (Linux-style, for reference)

```
/dts-v1/;

/ {
    model = "CAN Bus Infiltrator v1.0";
    compatible = "hackdev,can-infiltrator";

    chosen {
        stdout-path = &usbc_serial;
    };

    cpus {
        cpu@0 {
            compatible = "arm,cortex-m4f";
            clock-frequency = <168000000>;
        };
    };

    memory@20000000 {
        device_type = "memory";
        reg = <0x20000000 0x20000>; /* 128 KB SRAM */
    };

    memory@10000000 {
        device_type = "memory";
        reg = <0x10000000 0x10000>; /* 64 KB CCM */
    };

    clocks {
        clk_hse: clock@0 { compatible = "fixed-clock"; clock-frequency = <8000000>; };
        clk_lse: clock@1 { compatible = "fixed-clock"; clock-frequency = <32768>; };
    };

    can1: can@40006400 {
        compatible = "st,stm32-can";
        reg = <0x40006400 0x400>;
        interrupts = <19 0>, <20 0>, <21 0>, <22 0>;
        clocks = <&rcc 25>; /* APB1 */
        status = "okay";
    };

    can2: can@40006800 {
        compatible = "st,stm32-can";
        reg = <0x40006800 0x400>;
        interrupts = <63 0>;
        clocks = <&rcc 26>; /* APB1 */
        status = "okay";
    };

    spi3: spi@40003C00 {
        compatible = "st,stm32-spi";
        reg = <0x40003C00 0x400>;
        interrupts = <51 0>;
        clocks = <&rcc 15>; /* APB1 */
        #address-cells = <1>;
        #size-cells = <0>;
        status = "okay";

        nrf52: ble@0 {
            compatible = "nordic,nrf52840-spi-slave";
            reg = <0>;
            spi-max-frequency = <8000000>;
            interrupt-parent = <&gpioe>;
            interrupts = <1 2>; /* PE1, falling edge */
        };
    };

    i2c1: i2c@40005400 {
        compatible = "st,stm32-i2c";
        reg = <0x40005400 0x400>;
        clocks = <&rcc 21>;
        status = "okay";

        eeprom: eeprom@50 {
            compatible = "atmel,at24c02";
            reg = <0x50>;
            pagesize = <8>;
        };
    };

    sdio: sdio@40012C00 {
        compatible = "st,stm32-sdio";
        reg = <0x40012C00 0x400>;
        interrupts = <49 0>;
        clocks = <&rcc 11>; /* APB2 */
        bus-width = <4>;
        status = "okay";
    };

    usbc_serial: usbc@50000000 {
        compatible = "st,stm32-otg-fs";
        reg = <0x50000000 0x40000>;
        interrupts = <67 0>;
        dr_mode = "peripheral";
        status = "okay";
    };

    led_strip: leds@PE7 {
        compatible = "worldsemi,ws2812b";
        gpios = <&gpioe 7 0>;
        led-count = <4>;
    };
};
```

## 7. Main Application

```c
// main.c — CAN Bus Infiltrator main application
#include "board.h"
#include "registers.h"
#include "can.h"
#include "ble_spi.h"
#include "sdcard.h"
#include <string.h>

// ---- Global State ----
static volatile uint32_t frame_count[2] = {0, 0};
static volatile uint32_t error_count[2] = {0, 0};
static volatile uint32_t inject_count[2] = {0, 0};

#define RING_BUF_SIZE  4096
static can_frame_t ring_buf[2][RING_BUF_SIZE];
static volatile uint32_t ring_head[2] = {0, 0};
static volatile uint32_t ring_tail[2] = {0, 0};

// ---- Fuzzer State ----
typedef enum {
    FUZZ_IDLE = 0,
    FUZZ_RANDOM,
    FUZZ_BITFLIP,
    FUZZ_BYTE_MUTATE,
    FUZZ_ARITHMETIC,
    FUZZ_FIELD_AWARE
} fuzz_mode_t;

static fuzz_mode_t fuzz_mode = FUZZ_IDLE;
static uint32_t fuzz_target_id = 0;
static uint32_t fuzz_count = 0;
static uint32_t fuzz_max_count = 1000;

static void system_init(void) {
    // Enable FPU
    SCB->CPACR |= (0xF << 20);

    // Enable all GPIO clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB |
                    RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD |
                    RCC_AHB1ENR_GPIOE;

    // Configure PLL and clocks (see Phase 4 §2)
    // HSE = 8 MHz → PLL → 168 MHz SYSCLK
    RCC->CR |= (1 << 16);  // HSEON
    while (!(RCC->CR & (1 << 17)));  // HSERDY

    RCC->PLLCFGR = (8 << 0) |    // PLLM = 8
                    (336 << 6) |  // PLLN = 336
                    (0 << 16) |   // PLLP = 2
                    (7 << 24) |   // PLLQ = 7
                    (1 << 22);    // PLLSRC = HSE

    RCC->CR |= (1 << 24);  // PLLON
    while (!(RCC->CR & (1 << 25)));  // PLLRDY

    // Flash latency (5 wait states at 168 MHz)
    FLASH->ACR = (5 << 0) | (1 << 8) | (1 << 9); // LATENCY, PRFTEN, ICEN

    // Switch system clock to PLL
    RCC->CFGR = (2 << 0);  // SW = PLL
    while ((RCC->CFGR & (3 << 2)) != (2 << 2));

    // APB1 prescaler = 4 (42 MHz), APB2 prescaler = 2 (84 MHz)
    RCC->CFGR |= (0b100 << 10) | (0b010 << 13); // PPRE1, PPRE2

    // Enable backup SRAM
    RCC->AHB1ENR |= RCC_AHB1ENR_BKPSRAMEN;

    // Initialize TIM2 as microsecond counter for CAN timestamps
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = (42000000 / 1000000) - 1;  // 1 µs tick @ 42 MHz APB1 × 2
    TIM2->ARR = 0xFFFFFFFF;  // 32-bit auto-reload (free-running)
    TIM2->CR1 = (1 << 0);  // CEN = enable
}

static void can_rx_handler(const can_frame_t *frame, uint8_t channel) {
    uint32_t next_head = (ring_head[channel] + 1) % RING_BUF_SIZE;

    // Store in ring buffer (discard if full)
    if (next_head != ring_tail[channel]) {
        ring_buf[channel][ring_head[channel]] = *frame;
        ring_head[channel] = next_head;
    }

    frame_count[channel]++;

    // Forward to BLE if connected
    ble_send_can_frame(frame);
}

static uint8_t prng_next(void) {
    // XORshift PRNG
    static uint32_t state = 0xDEADBEEF;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (uint8_t)(state & 0xFF);
}

static void fuzzer_generate_frame(can_frame_t *frame) {
    frame->id = fuzz_target_id;
    frame->id_type = (fuzz_target_id > 0x7FF) ? CAN_EXT_ID : CAN_STD_ID;
    frame->rtr = 0;
    frame->dlc = (prng_next() % 8) + 1;

    switch (fuzz_mode) {
        case FUZZ_RANDOM:
            for (int i = 0; i < frame->dlc; i++)
                frame->data[i] = prng_next();
            break;

        case FUZZ_BITFLIP:
            for (int i = 0; i < frame->dlc; i++) {
                frame->data[i] = prng_next();
                uint8_t bit = prng_next() & 7;
                frame->data[i] ^= (1 << bit);
            }
            break;

        case FUZZ_ARITHMETIC:
            for (int i = 0; i < frame->dlc; i++)
                frame->data[i] = prng_next();
            // Mutate random byte with ±1, ±2, ±4, ±8...
            {
                uint8_t idx = prng_next() % frame->dlc;
                int8_t delta = (int8_t)(prng_next());
                frame->data[idx] += delta;
            }
            break;

        default:
            for (int i = 0; i < 8; i++)
                frame->data[i] = prng_next();
            break;
    }
}

static void fuzzer_task(void) {
    if (fuzz_mode == FUZZ_IDLE) return;

    can_frame_t frame;
    uint8_t channel = 0; // Default to CAN1

    for (uint32_t i = 0; i < fuzz_max_count; i++) {
        fuzzer_generate_frame(&frame);
        if (can_transmit(channel, &frame) == 0) {
            inject_count[channel]++;
        }
        fuzz_count++;

        // Inter-frame delay: 1-5 ms (random)
        uint32_t delay = (prng_next() % 5) + 1;
        for (volatile uint32_t d = 0; d < delay * 168000; d++);
    }
}

int main(void) {
    system_init();

    // Initialize peripherals
    can_init(0, CAN_BAUD_500K, CAN_MODE_LISTEN);  // CAN1: listen-only sniffer
    can_init(1, CAN_BAUD_500K, CAN_MODE_NORMAL);  // CAN2: normal (inject-capable)

    can_register_rx_callback(0, can_rx_handler);
    can_register_rx_callback(1, can_rx_handler);

    ble_spi_init();
    sd_init();

    // Enable interrupts
    can_enable_interrupts(0);
    can_enable_interrupts(1);
    __enable_irq();

    // Main loop
    while (1) {
        // Process BLE commands
        ble_packet_t cmd;
        if (ble_spi_read_response(&cmd, 10) == 0) {
            switch (cmd.opcode) {
                case BLE_CMD_SEND_DATA: {
                    // Parse CAN frame from BLE data and inject
                    if (cmd.length >= 5) {
                        can_frame_t frame;
                        frame.id = cmd.data[0] | (cmd.data[1] << 8);
                        if (cmd.data[2] & 0x01) frame.id |= (cmd.data[3] << 16) | (cmd.data[4] << 24);
                        frame.id_type = (cmd.data[2] & 0x01) ? CAN_EXT_ID : CAN_STD_ID;
                        frame.dlc = cmd.data[2] >> 4;
                        frame.rtr = (cmd.data[2] >> 3) & 1;
                        memcpy(frame.data, &cmd.data[5], frame.dlc);
                        can_transmit(1, &frame);
                        inject_count[1]++;
                    }
                    break;
                }
                case BLE_CMD_SET_ADV_DATA:
                    ble_spi_start_advertising("CAN-Infiltrator");
                    break;
                default:
                    break;
            }
        }

        // Run fuzzer if active
        fuzzer_task();

        // Feed watchdog (if enabled)
        // IWDG->KR = 0xAAAA;
    }
}
```

## 8. Build Instructions

### 8.1 Toolchain

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Verify
arm-none-eabi-gcc --version
# arm-none-eabi-gcc (GNU Arm Embedded Toolchain) 12.3 or later
```

### 8.2 Build Firmware

```bash
cd firmware
make clean
make
# Output: can_infiltrator.hex, can_infiltrator.bin
```

### 8.3 Flash via SWD

```bash
# Using OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4.cfg \
  -c "program can_infiltrator.bin verify reset exit 0x08004000"

# Using stlink
st-flash --reset write can_infiltrator.bin 0x08004000
```

### 8.4 Flash via USB DFU

```bash
# Enter DFU mode (set flag via USB-CDC command)
dfu-util -d 0483:df11 -a 0 -s 0x08004000 -D can_infiltrator.bin
```

### 8.5 Build Companion App

```bash
cd app
npm install
npx react-native run-android    # Android
npx react-native run-ios        # iOS
```

## 9. USB Descriptors

```c
// usb_descriptors.h — USB CDC device descriptors
#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

#define USBD_VID        0x1209  // pid.codes
#define USBD_PID        0xC470  // CAN Bus Infiltrator
#define USBD_LANGID     0x0409  // English (US)

#define USBD_CONFIG_NUM 1
#define USBD_ITF_NUM    1
#define USBD_STR_IDX_NUM 4

// Device descriptor
static const uint8_t usbd_device_desc[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType = Device
    0x00, 0x02, // bcdUSB = 2.00
    0x02,       // bDeviceClass = CDC
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x40,       // bMaxPacketSize0 = 64
    0x09, 0x12, // idVendor = 0x1209
    0x70, 0xC4, // idProduct = 0xC470
    0x00, 0x01, // bcdDevice = 1.00
    0x01,       // iManufacturer
    0x02,       // iProduct
    0x03,       // iSerialNumber
    0x01        // bNumConfigurations
};

// Configuration descriptor (CDC)
static const uint8_t usbd_config_desc[] = {
    // Configuration descriptor
    0x09,       // bLength
    0x02,       // bDescriptorType = Configuration
    0x43, 0x00, // wTotalLength = 67 bytes
    0x02,       // bNumInterfaces = 2 (CDC requires 2)
    0x01,       // bConfigurationValue
    0x00,       // iConfiguration
    0x80,       // bmAttributes = bus-powered
    0xFA,       // bMaxPower = 500 mA

    // CDC Interface Association Descriptor
    0x08,       // bLength
    0x0B,       // bDescriptorType = IAD
    0x00,       // bFirstInterface
    0x02,       // bInterfaceCount
    0x02,       // bFunctionClass = CDC
    0x02,       // bFunctionSubClass = ACM
    0x01,       // bFunctionProtocol
    0x00,       // iFunction

    // CDC Control Interface
    0x09,       // bLength
    0x04,       // bDescriptorType = Interface
    0x00,       // bInterfaceNumber = 0
    0x00,       // bAlternateSetting
    0x01,       // bNumEndpoints = 1
    0x02,       // bInterfaceClass = CDC
    0x02,       // bInterfaceSubClass = ACM
    0x01,       // bInterfaceProtocol
    0x00,       // iInterface

    // CDC Header Functional Descriptor
    0x05, 0x24, 0x00, 0x10, 0x01,
    // CDC ACM Functional Descriptor
    0x04, 0x24, 0x02, 0x02,
    // CDC Union Functional Descriptor
    0x05, 0x24, 0x06, 0x00, 0x01,
    // CDC Call Management Functional Descriptor
    0x05, 0x24, 0x01, 0x00, 0x01,

    // CDC Notification Endpoint (Interrupt IN)
    0x07,       // bLength
    0x05,       // bDescriptorType = Endpoint
    0x81,       // bEndpointAddress = EP1 IN
    0x03,       // bmAttributes = Interrupt
    0x08, 0x00, // wMaxPacketSize = 8
    0x10,       // bInterval = 16 ms

    // CDC Data Interface
    0x09,       // bLength
    0x04,       // bDescriptorType = Interface
    0x01,       // bInterfaceNumber = 1
    0x00,       // bAlternateSetting
    0x02,       // bNumEndpoints = 2
    0x0A,       // bInterfaceClass = CDC Data
    0x00,       // bInterfaceSubClass
    0x00,       // bInterfaceProtocol
    0x00,       // iInterface

    // CDC Data OUT Endpoint
    0x07,       // bLength
    0x05,       // bDescriptorType = Endpoint
    0x02,       // bEndpointAddress = EP2 OUT
    0x02,       // bmAttributes = Bulk
    0x40, 0x00, // wMaxPacketSize = 64
    0x00,       // bInterval

    // CDC Data IN Endpoint
    0x07,       // bLength
    0x05,       // bDescriptorType = Endpoint
    0x82,       // bEndpointAddress = EP2 IN
    0x02,       // bmAttributes = Bulk
    0x40, 0x00, // wMaxPacketSize = 64
    0x00        // bInterval
};

// String descriptors
static const uint8_t usbd_str_lang[]   = { 0x04, 0x03, 0x09, 0x04 };
static const uint8_t usbd_str_mfr[]   = { 24, 0x03, 'h','a','c','k','d','e','v','.','i','o' };
static const uint8_t usbd_str_prod[]  = { 36, 0x03, 'C','A','N',' ','I','n','f','i','l','t','r','a','t','o','r' };
static const uint8_t usbd_str_serial[]= { 20, 0x03, 'C','4','7','0','-','0','0','0','0','1' };

#endif // USB_DESCRIPTORS_H
```

## 10. Command Protocol (USB-CDC / BLE)

The device speaks a simple binary protocol over both USB-CDC and BLE SPI:

### Frame Format (little-endian)

| Byte | Field | Description |
|---|---|---|
| 0 | Sync | 0xAA (start of frame) |
| 1 | Opcode | Command/response code |
| 2 | Seq | Sequence number |
| 3–4 | Length | Payload length (0–256) |
| 5–n | Data | Payload |
| n+1 | Checksum | XOR of bytes 1–n |

### Opcodes (Host → Device)

| Opcode | Name | Data | Description |
|---|---|---|---|
| 0x01 | CAN_SNIFF_START | [ch, baud] | Start sniffing on channel |
| 0x02 | CAN_SNIFF_STOP | [ch] | Stop sniffing on channel |
| 0x03 | CAN_INJECT | [ch, id_lo, id_hi, dlc, data...] | Inject a CAN frame |
| 0x04 | CAN_FUZZ_START | [ch, mode, target_id_lo, target_id_hi, count_lo, count_hi] | Start fuzzer |
| 0x05 | CAN_FUZZ_STOP | [ch] | Stop fuzzer |
| 0x06 | CAN_SET_FILTER | [ch, bank, id_lo, id_hi, mask_lo, mask_hi] | Set hardware filter |
| 0x07 | CAN_SET_BAUD | [ch, baud_code] | Change baud rate |
| 0x08 | SD_RECORD_START | [filename...] | Start recording to SD |
| 0x09 | SD_RECORD_STOP | — | Stop recording |
| 0x0A | SD_REPLAY_START | [filename...] | Replay from SD |
| 0x0B | SD_REPLAY_STOP | — | Stop replay |
| 0x0C | SD_LIST_FILES | — | List SD card files |
| 0x0D | DEVICE_INFO | — | Get device info |
| 0x0E | DFU_ENTER | — | Enter DFU bootloader mode |
| 0x0F | LED_SET | [led_idx, r, g, b] | Set LED color |

### Opcodes (Device → Host)

| Opcode | Name | Data | Description |
|---|---|---|---|
| 0x80 | CAN_FRAME | [ch, ts_lo, ts_hi, id_lo, id_hi, dlc, data...] | Received CAN frame |
| 0x81 | CAN_ERROR | [ch, err_code, tec, rec] | Bus error notification |
| 0x82 | FUZZ_STATUS | [ch, count_lo, count_hi] | Fuzzer progress update |
| 0x83 | SD_FILE_LIST | [name_len, name..., ...] | File listing response |
| 0x84 | DEVICE_INFO_RSP | [ver, channels, features...] | Device info response |
| 0xFF | NACK | [error_code] | Command not understood |