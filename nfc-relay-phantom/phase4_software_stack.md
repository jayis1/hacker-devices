# NFC Relay Phantom — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On / Reset
    │
    ├── BOOT0 = LOW (normal boot from Flash)
    │   └── STM32L4 starts from 0x0800_0000 (Main FW)
    │
    ├── BOOT0 = HIGH (DFU mode via SW2 held at reset)
    │   └── Enter USB DFU bootloader (built-in ROM)
    │
    └── Dual-bank layout:
        ├── Bank A (0x0800_0000): Application firmware (1.5 MB)
        ├── Bank B (0x0806_0000): OTA update staging (512 KB)
        └── EEPROM emul (0x080E_0000): Settings, keys, profiles
```

### 1.2 NRF52832 Boot

```
NRF52832 boot:
    │
    ├── STM32L4 drives NRF_RST HIGH after 10ms
    │   └── NRF52 starts from 0x0000_0000 (internal Flash)
    │       └── SoftDevice S132 v7.x initializes BLE stack
    │           └── Application starts, UART cmd interface ready
    │
    └── DFU mode: STM32L4 holds NRF_RST + P0.19 (DFU pin) LOW at boot
        └── NRF52 enters Bootloader DFU mode (BLE OTA)
```

### 1.3 Boot Timing

| Phase | Duration | Activity |
|-------|----------|----------|
| T+0 ms | 0-2 ms | Power ramp, LDO stabilization |
| T+2 ms | 2-5 ms | STM32L4 HSE lock, PLL ramp |
| T+5 ms | 5-10 ms | Clock tree config, GPIO init, peripheral clocks |
| T+10 ms | 10-15 ms | Release NRF52 reset, NRF52 SoftDevice init |
| T+15 ms | 15-30 ms | SPI flash probe, PN5180 init, EM4095 init |
| T+30 ms | 30-100 ms | OLED splash, BLE advertising start |
| T+100 ms | 100-500 ms | Ready for commands |
| T+500 ms | 500 ms | Fully operational |

---

## 2. Memory Map & MMIO Registers

### 2.1 STM32L4S5 Memory Map

| Region | Address Range | Size | Description |
|--------|---------------|------|-------------|
| Flash Bank A | 0x0800_0000 - 0x0805_FFFF | 384 KB | Main application |
| Flash Bank B | 0x0806_0000 - 0x080F_FFFF | 384 KB | OTA staging + FS |
| Flash Bank C | 0x0810_0000 - 0x081F_FFFF | 1 MB | Capture storage |
| SRAM1 | 0x2000_0000 - 0x2002_FFFF | 192 KB | Main heap/stack |
| SRAM2 | 0x2003_0000 - 0x2003_3FFF | 16 KB | BLE shared buffer |
| SRAM3 | 0x2003_4000 - 0x2004_FFFF | 112 KB | NFC frame buffer |
| Periph | 0x4000_0000 - 0x4FFF_FFFF | — | APB1/2, AHB1/2 peripherals |
| Cortex-M4 | 0xE000_0000 - 0xE00F_FFFF | — | NVIC, SCB, SysTick, MPU |

### 2.2 Key MMIO Registers (STM32L4S5)

#### RCC (Reset and Clock Control) — Base: 0x4002_1000

| Register | Offset | Description |
|-----------|--------|-------------|
| RCC_CR | 0x00 | Clock control (HSE, HSI, PLL) |
| RCC_CFGR | 0x08 | Clock configuration (SYSCLK, AHB, APB prescalers) |
| RCC_AHB1ENR | 0x48 | AHB1 peripheral clock enable |
| RCC_AHB2ENR | 0x4C | AHB2 peripheral clock enable (GPIOA-GPIOE) |
| RCC_APB1ENR1 | 0x58 | APB1 peripheral clock enable (SPI2, UART4) |
| RCC_APB2ENR | 0x60 | APB2 peripheral clock enable (SPI1, SYSCFG) |

#### GPIOA — Base: 0x4002_0000

| Register | Offset | Description |
|-----------|--------|-------------|
| GPIOA_MODER | 0x00 | Port mode (00: input, 01: output, 10: AF, 11: analog) |
| GPIOA_OTYPER | 0x04 | Output type (push-pull / open-drain) |
| GPIOA_OSPEEDR | 0x08 | Output speed (low/medium/high/very-high) |
| GPIOA_PUPDR | 0x0C | Pull-up / pull-down |
| GPIOA_IDR | 0x10 | Input data register |
| GPIOA_ODR | 0x14 | Output data register |
| GPIOA_BSRR | 0x18 | Bit set/reset register |
| GPIOA_AFRL | 0x20 | Alternate function low (pins 0-7) |
| GPIOA_AFRH | 0x24 | Alternate function high (pins 8-15) |

#### SPI1 — Base: 0x4001_3000

| Register | Offset | Description |
|-----------|--------|-------------|
| SPI1_CR1 | 0x00 | Control register 1 (BR, CPOL, CPHA, MSTR) |
| SPI1_CR2 | 0x04 | Control register 2 (DS, TXEIE, RXNEIE, DMA) |
| SPI1_SR | 0x08 | Status register (BSY, TXE, RXNE, OVR) |
| SPI1_DR | 0x0C | Data register |
| SPI1_CRCPR | 0x10 | CRC polynomial |

#### SPI2 — Base: 0x4001_3800

Same register layout as SPI1.

#### USART1 — Base: 0x4001_1000

| Register | Offset | Description |
|-----------|--------|-------------|
| USART1_CR1 | 0x00 | Control 1 (UE, TE, RE, RXNEIE, TCIE) |
| USART1_CR2 | 0x04 | Control 2 (STOP, CLKEN) |
| USART1_CR3 | 0x08 | Control 3 (DMAT, DMAR) |
| USART1_BRR | 0x0C | Baud rate register |
| USART1_ISR | 0x1C | Interrupt status register |
| USART1_TDR | 0x28 | Transmit data register |
| USART1_RDR | 0x24 | Receive data register |

#### USART4 — Base: 0x4000_4C00

Same layout, different base. Used for NRF52 UART link.

#### DMA1 — Base: 0x4002_0000

| Register | Offset | Description |
|-----------|--------|-------------|
| DMA1_ISR | 0x00 | Interrupt status |
| DMA1_IFCR | 0x04 | Interrupt flag clear |
| DMA1_CCR1 | 0x08 | Channel 1 configuration (EN, TCIE, DIR, CIRC) |
| DMA1_CNDTR1 | 0x0C | Channel 1 number of data |
| DMA1_CPAR1 | 0x10 | Channel 1 peripheral address |
| DMA1_CMAR1 | 0x14 | Channel 1 memory address |

#### I2C1 — Base: 0x4000_5400

| Register | Offset | Description |
|-----------|--------|-------------|
| I2C1_CR1 | 0x00 | Control 1 (PE, TXIE, RXIE, NACKIE) |
| I2C1_CR2 | 0x04 | Control 2 (SADD, RD_WRN, START, STOP) |
| I2C1_ISR | 0x14 | Interrupt status (TXIS, RXNE, STOPF, NACKF) |
| I2C1_TXDR | 0x28 | Transmit data |
| I2C1_RXDR | 0x24 | Receive data |

#### USB (OTG FS) — Base: 0x5000_0000

| Register | Offset | Description |
|-----------|--------|-------------|
| OTG_FS_GOTGCTL | 0x000 | OTG control |
| OTG_FS_GINTSTS | 0x018 | Global interrupt status |
| OTG_FS_GINTMSK | 0x01C | Global interrupt mask |
| OTG_FS_DAINT | 0x218 | Device all endpoint interrupt |
| OTG_FS_DIEPCTL0 | 0x400 | Device IN EP0 control |
| OTG_FS_DOEPCTL0 | 0x600 | Device OUT EP0 control |

---

## 3. Clock & GPIO Initialization

### 3.1 Clock Tree

```
HSE (8 MHz crystal) ──► PLL ──► SYSCLK (120 MHz)
                           │
                           ├── AHB (120 MHz) → GPIO, DMA, Flash
                           ├── APB1 (120 MHz) → SPI2, UART1, UART4, I2C1
                           └── APB2 (120 MHz) → SPI1, USB, ADC

LSE (32.768 kHz) ──► RTC
HSI16 (16 MHz) ──► Backup (PLL failover)

NRF52832:
HFCLK (32 MHz crystal) ──► CPU @ 64 MHz
LFCLK (32.768 kHz internal RC) ──► BLE timing
```

### 3.2 Clock Configuration (C Code)

```c
// System clock: HSE → PLL → 120 MHz
// PLL Configuration:
//   PLLSRC = HSE (8 MHz)
//   PLLM = 1  → 8 MHz
//   PLLN = 60  → 480 MHz VCO
//   PLLP = 7  → 120 MHz (PLLP divider = P+1 = 8, but L4 uses PLLN/PLLQ)
//   PLLQ = 10 → 48 MHz (USB)
//   PLLR = 2  → 120 MHz (SYSCLK)

#define RCC_CR_HSEON      (1 << 16)
#define RCC_CR_HSIRDY     (1 << 10)
#define RCC_CR_HSERDY     (1 << 17)
#define RCC_CR_PLLON      (1 << 24)
#define RCC_CR_PLLRDY     (1 << 25)

#define RCC_PLLCFGR_PLLSRC_HSE  (1 << 0)
#define RCC_PLLCFGR_PLLM_Pos    4
#define RCC_PLLCFGR_PLLN_Pos    8
#define RCC_PLLCFGR_PLLQ_Pos    21
#define RCC_PLLCFGR_PLLR_Pos    25

static void clock_init(void) {
    // Enable HSE
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Configure PLL: HSE/1 × 60 /2 = 120 MHz, Q=5 for 48 MHz USB
    RCC->PLLCFGR = (1 << RCC_PLLCFGR_PLLM_Pos)   // PLLM = 1
                  | (60 << RCC_PLLCFGR_PLLN_Pos)   // PLLN = 60
                  | (5 << RCC_PLLCFGR_PLLQ_Pos)    // PLLQ = 5 → 48MHz
                  | (0 << RCC_PLLCFGR_PLLR_Pos)    // PLLR = 2
                  | RCC_PLLCFGR_PLLSRC_HSE;

    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Enable PLL system clock output
    RCC->CR |= RCC_CR_PLLSYSON;

    // Flash latency (5 wait states @ 120 MHz, VDD range 1)
    FLASH->ACR |= (5 << FLASH_ACR_LATENCY_Pos);

    // Switch SYSCLK to PLL
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | (3 << RCC_CFGR_SW_Pos);
    while ((RCC->CFGR & RCC_CFGR_SWS) != (3 << RCC_CFGR_SWS_Pos));

    // Enable peripheral clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                     RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN |
                     RCC_AHB2ENR_GPIOEEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN | RCC_APB1ENR1_USART1EN |
                      RCC_APB1ENR1_USART4EN | RCC_APB1ENR1_I2C1EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SYSCFGEN;
}
```

### 3.3 GPIO Initialization

```c
static void gpio_init(void) {
    // --- SPI1 (PN5180): PA4(NSS), PA5(SCK), PA6(MISO), PA7(MOSI) ---
    // PA4: Output, push-pull, high speed (manual NSS)
    GPIOA->MODER = (GPIOA->MODER & ~(3 << (4*2))) | (1 << (4*2));
    GPIOA->OSPEEDR |= (3 << (4*2));  // Very high speed
    GPIOA->ODR |= (1 << 4);  // NSS high (deselected)

    // PA5: AF5 (SPI1_SCK), PA6: AF5 (SPI1_MISO), PA7: AF5 (SPI1_MOSI)
    GPIOA->MODER = (GPIOA->MODER & ~(0xF << (5*2))) | (0x2A << (5*2)); // AF mode
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFF << (5*4))) | (0x55 << (5*4));  // AF5
    GPIOA->OSPEEDR |= (3 << (5*2)) | (3 << (6*2)) | (3 << (7*2));

    // --- SPI2 (W25Q128): PB12(NSS), PB13(SCK), PB14(MISO), PB15(MOSI) ---
    GPIOB->MODER = (GPIOB->MODER & ~(3 << (12*2))) | (1 << (12*2));
    GPIOB->ODR |= (1 << 12);  // NSS high
    // PB13-PB15: AF5 (SPI2)
    GPIOB->MODER = (GPIOB->MODER & ~(0x3F << (13*2))) | (0x2A << (13*2));
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xFFF << ((13-8)*4))) | (0x555 << ((13-8)*4));

    // --- I2C1 (SSD1306 + BQ25896): PB6(SCL), PB7(SDA) ---
    GPIOB->MODER = (GPIOB->MODER & ~(0xF << (6*2))) | (0xA << (6*2));  // AF4
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFF << (6*4))) | (0x44 << (6*4));  // AF4
    GPIOB->OTYPER |= (3 << 6);  // Open-drain
    GPIOB->PUPDR |= (5 << (6*2));  // Pull-up

    // --- UART4 (NRF52): PA0(TX), PA1(RX) ---
    GPIOA->MODER = (GPIOA->MODER & ~(0xF << (0*2))) | (0xA << (0*2));  // AF8
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFF << (0*4))) | (0x88 << (0*4));  // AF8
    GPIOA->OSPEEDR |= (3 << (0*2)) | (3 << (1*2));

    // --- UART1 (EM4095): PA9(TX), PA10(RX) ---
    GPIOA->MODER = (GPIOA->MODER & ~(0xF << (9*2))) | (0xA << (9*2));  // AF7
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFF << ((9-8)*4))) | (0x77 << ((9-8)*4));  // AF7

    // --- USB: PA11(DM), PA12(DP) ---
    GPIOA->MODER = (GPIOA->MODER & ~(0xF << (11*2))) | (0xA << (11*2));  // AF10
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFF << ((11-8)*4))) | (0xAA << ((11-8)*4));

    // --- Control GPIOs ---
    // PC0: NRF_IRQ (input, pull-up)
    GPIOC->MODER &= ~(3 << (0*2));  // Input
    GPIOC->PUPDR |= (1 << (0*2));   // Pull-up

    // PC1: EM4095_SHD (output, push-pull)
    GPIOC->MODER = (GPIOC->MODER & ~(3 << (1*2))) | (1 << (1*2));

    // PC2: EM4095_MOD (output, push-pull)
    GPIOC->MODER = (GPIOC->MODER & ~(3 << (2*2))) | (1 << (2*2));

    // PC5: PN5180_RST (output, push-pull)
    GPIOC->MODER = (GPIOC->MODER & ~(3 << (5*2))) | (1 << (5*2));

    // PC8: NRF_RST (output, push-pull)
    GPIOC->MODER = (GPIOC->MODER & ~(3 << (8*2))) | (1 << (8*2));

    // LEDs: PB0(Green), PB1(Red), PB9(Blue) — output, push-pull
    GPIOB->MODER = (GPIOB->MODER & ~(3 << (0*2))) | (1 << (0*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3 << (1*2))) | (1 << (1*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3 << (9*2))) | (1 << (9*2));

    // Buttons: PC13(SW1), PB8(SW2) — input, pull-up
    GPIOC->MODER &= ~(3 << (13*2));
    GPIOC->PUPDR |= (1 << (13*2));
    GPIOB->MODER &= ~(3 << (8*2));
    GPIOB->PUPDR |= (1 << (8*2));
}
```

---

## 4. Device Drivers

### 4.1 SPI Driver (with DMA)

**SPI1** — PN5180 communication, 20 MHz, DMA-based transfers.

```c
// drivers/spi_dma.h
#ifndef SPI_DMA_H
#define SPI_DMA_H

#include <stdint.h>
#include <stddef.h>

#define SPI1_NS_LOW()   GPIOA->ODR &= ~(1 << 4)
#define SPI1_NS_HIGH()  GPIOA->ODR |= (1 << 4)
#define SPI2_NS_LOW()   GPIOB->ODR &= ~(1 << 12)
#define SPI2_NS_HIGH()  GPIOB->ODR |= (1 << 12)

typedef enum {
    SPI_BUS_1 = 0,  // PN5180
    SPI_BUS_2 = 1,  // W25Q128
} spi_bus_t;

void spi_init(void);
void spi_transfer_blocking(spi_bus_t bus, const uint8_t *tx, uint8_t *rx, size_t len);
void spi_transfer_dma(spi_bus_t bus, const uint8_t *tx, uint8_t *rx, size_t len);
bool spi_dma_busy(spi_bus_t bus);
void spi_dma_wait(spi_bus_t bus);

#endif // SPI_DMA_H
```

### 4.2 PN5180 NFC Controller Driver

```c
// drivers/pn5180.h
#ifndef PN5180_H
#define PN5180_H

#include <stdint.h>
#include <stdbool.h>

// PN5180 Register Map
#define PN5180_REG_SYSTEM_CONFIG        0x00
#define PN5180_REG_IRQ_STATUS           0x04
#define PN5180_REG_IRQ_ENABLE           0x08
#define PN5180_REG_TX_DATA_CFG          0x14
#define PN5180_REG_TX_MOD_DEPTH         0x18
#define PN5180_REG_RX_CFG               0x1C
#define PN5180_REG_RX_STATUS            0x20
#define PN5180_REG_FIELD_STATUS         0x24
#define PN5180_REG_FIELD_ON_TIME         0x28
#define PN5180_REG_FIELD_OFF_TIME        0x2C
#define PN5180_REG_CRC_CFG              0x30
#define PN5180_REG_MISC_CFG             0x34
#define PN5180_REG_ANAT_CONFIG          0x38
#define PN5180_REG_RF_STATUS            0x3C

// PN5180 Commands
#define PN5180_CMD_ACTIVATE_TX          0x01
#define PN5180_CMD_DEACTIVATE_TX        0x02
#define PN5180_CMD_ACTIVATE_RX          0x03
#define PN5180_CMD_DEACTIVATE_RX        0x04
#define PN5180_CMD_TRANSCEIVE           0x05
#define PN5180_CMD_WRITE_DATA           0x08
#define PN5180_CMD_READ_DATA            0x09
#define PN5180_CMD_LOAD_RF_CONFIG       0x0A

// NFC Protocol Modes
typedef enum {
    NFC_MODE_A_106 = 0,    // ISO 14443A @ 106 kbps
    NFC_MODE_A_212 = 1,    // FeliCa @ 212 kbps
    NFC_MODE_A_424 = 2,    // FeliCa @ 424 kbps
    NFC_MODE_B_106 = 3,    // ISO 14443B @ 106 kbps
    NFC_MODE_F_212 = 4,    // NFC-F @ 212 kbps
    NFC_MODE_F_424 = 5,    // NFC-F @ 424 kbps
    NFC_MODE_V           = 6,    // ISO 15693 (NFC-V)
} nfc_protocol_t;

// Operation modes
typedef enum {
    NFC_OPS_READER = 0,
    NFC_OPS_CARD_EMUL = 1,
    NFC_OPS_SNIFFER = 2,
} nfc_op_mode_t;

typedef struct {
    nfc_protocol_t protocol;
    nfc_op_mode_t mode;
    uint8_t uid[10];
    uint8_t uid_len;
    uint8_t atqa[2];
    uint8_t sak;
    bool field_on;
} nfc_context_t;

void pn5180_init(void);
void pn5180_reset(void);
void pn5180_load_protocol(nfc_protocol_t proto);
void pn5180_field_on(void);
void pn5180_field_off(void);
bool pn5180_detect_card(nfc_context_t *ctx);
bool pn5180_send_frame(const uint8_t *data, size_t len);
bool pn5180_recv_frame(uint8_t *buf, size_t *len, uint32_t timeout_ms);
void pn5180_set_card_emulation(const uint8_t *uid, size_t uid_len,
                                const uint8_t *atqa, uint8_t sak);
void pn5180_set_sniffer_mode(void);
uint32_t pn5180_read_irq(void);
void pn5180_clear_irq(uint32_t mask);

#endif // PN5180_H
```

### 4.3 EM4095 125 kHz RFID Driver

```c
// drivers/em4095.h
#ifndef EM4095_H
#define EM4095_H

#include <stdint.h>
#include <stdbool.h>

// EM4100 format: 64 bits = 9 header + 32 data + 14 parity + 9 stop
#define EM4100_BIT_LEN    64
#define EM4100_BYTE_LEN   8

// HID Prox II format: variable length, PSK modulation
#define HID_PROX_MAX_BITS  200

typedef enum {
    RFID_PROTO_EM4100 = 0,
    RFID_PROTO_HID_PROX = 1,
    RFID_PROTO_AWID = 2,
    RFID_PROTO_IO_PROX = 3,
} rfid_125_protocol_t;

typedef struct {
    rfid_125_protocol_t protocol;
    uint8_t data[25];       // Up to 200 bits
    uint8_t bit_count;
    uint32_t facility_code;
    uint32_t card_number;
} rfid_125_context_t;

void em4095_init(void);
void em4095_power_on(void);
void em4095_power_off(void);
bool em4095_read_em4100(rfid_125_context_t *ctx);
bool em4095_read_hid_prox(rfid_125_context_t *ctx);
bool em4095_write_t5577(const rfid_125_context_t *ctx, uint8_t *key);
void em4095_set_modulation(bool on);
void em4095_start_continuous_read(void (*callback)(const rfid_125_context_t *));
void em4095_stop_continuous_read(void);

// Manchester decoding
int8_t em4095_manchester_decode(const uint8_t *raw, size_t raw_bits,
                                 uint8_t *decoded, size_t *decoded_bits);

#endif // EM4095_H
```

---

## 5. DMA Configuration

### 5.1 DMA Channel Assignment

| Channel | Stream | Direction | Peripheral | Use |
|---------|--------|-----------|-----------|-----|
| DMA1 | Ch2 | Peripheral→Memory | SPI1_RX | PN5180 DMA RX |
| DMA1 | Ch3 | Memory→Peripheral | SPI1_TX | PN5180 DMA TX |
| DMA1 | Ch4 | Peripheral→Memory | SPI2_RX | W25Q128 DMA RX |
| DMA1 | Ch5 | Memory→Peripheral | SPI2_TX | W25Q128 DMA TX |
| DMA1 | Ch6 | Peripheral→Memory | USART1_RX | EM4095 RX DMA |
| DMA1 | Ch7 | Peripheral→Memory | USART4_RX | NRF52 UART RX DMA |

### 5.2 DMA Configuration Code

```c
// SPI1 RX DMA (Channel 2, Stream 0)
DMA1_Channel2->CCR = DMA_CCR_MINC |    // Memory increment
                      DMA_CCR_CIRC |    // Circular mode for continuous capture
                      DMA_CCR_TCIE;    // Transfer complete interrupt
DMA1_Channel2->CPAR = (uint32_t)&SPI1->DR;
DMA1_Channel2->CMAR = (uint32_t)spi1_rx_buffer;
DMA1_Channel2->CNDTR = SPI1_DMA_BUF_SIZE;
```

---

## 6. Device Tree (STM32L4)

```
/dts-v1/;
/include/ "st/l4/stm32l4s5vi.dtsi"

/ {
    model = "NFC Relay Phantom v1";
    compatible = "phantom,nfc-relay-v1";

    chosen {
        zephyr,console = &usart4;
        zephyr,shell-uart = &usart4;
        zephyr,flash = &flash0;
    };

    leds {
        compatible = "gpio-leds";
        led_green: led_0 { gpios = <&gpiob 0 GPIO_ACTIVE_HIGH>; };
        led_red: led_1 { gpios = <&gpiob 1 GPIO_ACTIVE_HIGH>; };
        led_blue: led_2 { gpios = <&gpiob 9 GPIO_ACTIVE_HIGH>; };
    };

    gpio_keys {
        compatible = "gpio-keys";
        btn_sw1: btn_0 { gpios = <&gpioc 13 GPIO_ACTIVE_LOW>; };
        btn_sw2: btn_1 { gpios = <&gpiob 8 GPIO_ACTIVE_LOW>; };
    };

    aliases {
        spi-nfc = &spi1;
        spi-flash = &spi2;
        i2c-display = &i2c1;
        uart-em4095 = &usart1;
        uart-nrf52 = &usart4;
    };
};

&spi1 {
    status = "okay";
    cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>;
    speed-hz = <20000000>;
    pn5180: pn5180@0 {
        compatible = "nxp,pn5180";
        reg = <0>;
        busy-gpios = <&gpioc 4 GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpioc 5 GPIO_ACTIVE_LOW>;
        irq-gpios = <&gpioc 0 GPIO_ACTIVE_LOW>;
    };
};

&spi2 {
    status = "okay";
    cs-gpios = <&gpiob 12 GPIO_ACTIVE_LOW>;
    speed-hz = <40000000>;
    w25q128: flash@0 {
        compatible = "winbond,w25q128";
        reg = <0>;
        size = <16777216>;
        wp-gpios = <&gpiod 0 GPIO_ACTIVE_LOW>;
        hold-gpios = <&gpiod 1 GPIO_ACTIVE_LOW>;
    };
};

&i2c1 {
    status = "okay";
    clock-frequency = <I2C_BIT_RATE_FAST>;  // 400 kHz
    ssd1306: oled@3c {
        compatible = "solomon,ssd1306";
        reg = <0x3c>;
        width = <128>;
        height = <64>;
        reset-gpios = <&gpioe 0 GPIO_ACTIVE_LOW>;
    };
    bq25896: charger@6a {
        compatible = "ti,bq25896";
        reg = <0x6a>;
    };
};

&usart1 {
    status = "okay";
    current-speed = <9600>;
    em4095 {
        compatible = "emmicro,em4095";
        shutdown-gpios = <&gpioc 1 GPIO_ACTIVE_LOW>;
        mod-gpios = <&gpioc 2 GPIO_ACTIVE_HIGH>;
    };
};

&usart4 {
    status = "okay";
    current-speed = <115200>;
};

&usb {
    status = "okay";
    dr_mode = "peripheral";
};
```

---

## 7. Firmware Architecture

### 7.1 RTOS Task Model (FreeRTOS)

| Task | Priority | Stack | Period | Description |
|------|----------|-------|--------|-------------|
| NFC_Main | 5 (high) | 2048 | Event-driven | NFC protocol state machine |
| RFID_125 | 4 | 1024 | 10 ms poll | 125 kHz read/clone |
| BLE_Cmd | 3 | 1024 | Event-driven | UART command parser from NRF52 |
| USB_CDC | 3 | 1024 | Event-driven | USB serial command interface |
| Display | 1 (low) | 512 | 100 ms update | OLED status rendering |
| Capture | 2 | 4096 | Event-driven | Frame capture → SPI flash write |
| Relay | 5 (high) | 2048 | Event-driven | Relay tunnel between devices |

### 7.2 State Machine

```
IDLE ──[detect NFC field]──> SNIFFING
  │                              │
  ├──[button press]──> MENU
  │                      │
  │                      ├── EMULATE (card mode)
  │                      ├── CLONE (read + write T5577)
  │                      ├── RELAY (BLE tunnel)
  │                      └── SETTINGS
  │
  ├──[BLE connect]──> BLE_COMMAND
  │                      │
  │                      └── (same as MENU options)
  │
  └──[USB connect]──> USB_COMMAND
```

---

## 8. Build Instructions

### 8.1 Toolchain

```bash
# Install ARM GCC toolchain
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# Verify
arm-none-eabi-gcc --version
# arm-none-eabi-gcc (GNU Arm Embedded Toolchain) 12.x or newer

# Clone and build
cd nfc-relay-phantom/firmware
make clean
make -j$(nproc)

# Output: build/nfc_phantom.bin (flashable binary)
#         build/nfc_phantom.elf (debug symbols)
#         build/nfc_phantom.hex (Intel HEX)
```

### 8.2 Flash Methods

**Method 1: OpenOCD via SWD (J2 Tag-Connect)**
```bash
openocd -f interface/cmsis-dap.cfg -f target/stm32l4s5.cfg \
    -c "program build/nfc_phantom.elf verify reset exit"
```

**Method 2: USB DFU (SW2 held during reset)**
```bash
dfu-util -a 0 -s 0x08000000:leave -D build/nfc_phantom.bin
```

**Method 3: STM32CubeProgrammer (GUI)**
```
Select: ST-LINK or USB (DFU)
Connect → Download → Verify → Start
```

### 8.3 NRF52832 Firmware

```bash
# Install nRF Connect SDK / nRF5 SDK
# Build with CMake or Make

cd nfc-relay-phantom/firmware/nrf52_ble
make

# Flash via SWD (from STM32L4 GPIO or external probe)
nrfjprog --program build/nrf52_ble.hex --sectorerase --reset

# Or OTA DFU via BLE
nrfutil dfu ble -s build/nrf52_ble.zip
```

---

## 9. USB Descriptors

See `usb_descriptors.h` in firmware directory. Device presents as composite:
- Interface 0: CDC-ACM (serial console for commands)
- Interface 1: Mass Storage (capture file access, read-only FAT)

VID: 0x1209 (pid.codes) | PID: 0xNF52

---

## 10. BLE GATT Service

| Service | UUID | Characteristics |
|---------|------|----------------|
| NFC Control | 0xFFF0 | 0xFFF1: Mode select (R/W), 0xFFF2: Protocol select (R/W), 0xFFF3: Start/Stop (R/W) |
| NFC Data | 0xFFF4 | 0xFFF5: UID read (Notify), 0xFFF6: Frame data (Notify), 0xFFF7: Send frame (Write) |
| RFID 125 | 0xFFF8 | 0xFFF9: Card ID read (Notify), 0xFFFA: Write T5577 (Write) |
| Relay | 0xFFFB | 0xFFFC: Relay data (Notify + Write), 0xFFFD: Relay control (R/W) |
| Device Info | 0x180A | Standard device info chars |
| Battery | 0x180F | 0x2A19: Battery level |

---

## 11. Security Considerations

1. **Firmware read protection:** Option Bytes set to RDP Level 1 (debug read disabled, no SWD access while locked)
2. **BLE pairing:** AES-CCM authenticated pairing, MITM protection via passkey
3. **USB CDC:** No authentication required (physical access assumed)
4. **Key storage:** Mifare keys stored in SRAM2 (battery-backed domain), wiped on tamper detect
5. **OTA updates:** Signed firmware images (Ed25519), verified before Bank B write
6. **Relay encryption:** AES-128-GCM tunnel between paired Phantom devices