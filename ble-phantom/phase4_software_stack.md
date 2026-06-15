# BLE Phantom — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On Reset
    │
    ├─ BOOT0 = LOW → Boot from Flash (normal)
    │                    │
    │                    ├─ Init clocks (HSE 8 MHz → PLL 84 MHz)
    │                    ├─ Init SRAM, CCM
    │                    ├─ Copy .data from Flash → SRAM
    │                    ├─ Zero .bss in SRAM + CCM
    │                    ├─ Init stack pointer (MSP = 0x20010000)
    │                    ├─ Call SystemInit()
    │                    ├─ Call main()
    │                    │
    │                    ├─ main():
    │                    │    ├─ board_init()
    │                    │    ├─ usb_init()  → USB CDC device
    │                    │    ├─ radio_a_init() → SPI0 to nRF A
    │                    │    ├─ radio_b_init() → SPI1 to nRF B
    │                    │    ├─ battery_init() → I2C gauge
    │                    │    └─ scheduler_loop()
    │                    │
    │                    └─ DFU mode (BOOT0=HIGH via SWD trigger):
    │                         └─ USB DFU bootloader (STM32 built-in)
    │
    └─ BOOT0 = HIGH → System Memory Bootloader (STM32 ROM)
                         └─ USB DFU mode (for firmware updates)
```

### 1.2 Vector Table

| Offset | Handler | Address |
|--------|---------|---------|
| 0x0000 | Initial SP | 0x20010000 (end of SRAM) |
| 0x0004 | Reset | 0x08000000 (Flash start) |
| 0x0008 | NMI | flash_vector_nmi |
| 0x000C | HardFault | flash_vector_hardfault |
| 0x0040 | EXTI0 | radio_a_irq_handler |
| 0x0044 | EXTI1 | radio_b_irq_handler |
| 0x0058 | SPI1_IRQHandler | spi1_dma_handler |
| 0x0064 | SPI2_IRQHandler | spi2_dma_handler |
| 0x0094 | USB_IRQHandler | usb_isr_handler |

### 1.3 Linker Script Regions

```
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
    CCM (rwx)   : ORIGIN = 0x10000000, LENGTH = 16K
}

SECTIONS
{
    .isr_vector : { *(.isr_vector) } > FLASH
    .text       : { *(.text*) } > FLASH
    .rodata      : { *(.rodata*) } > FLASH
    .data        : { *(.data*) } > SRAM AT > FLASH
    .bss         : { *(.bss*) *(COMMON) } > SRAM
    .ccm_buffer  : { *(.ccm_buffer) } > CCM
    
    /* DMA buffers in SRAM for radio capture */
    .radio_a_buf (NOLOAD) : { . = ALIGN(4); __radio_a_buf_start = .; . += 32K; } > SRAM
    .radio_b_buf (NOLOAD) : { . = ALIGN(4); __radio_b_buf_start = .; . += 32K; } > SRAM
}
```

## 2. MMIO Registers

### 2.1 STM32F401 Key Registers

#### RCC (Reset and Clock Control)

| Register | Address | Purpose | Key Bits |
|----------|---------|---------|----------|
| RCC_CR | 0x40023800 | Clock control | HSEON, HSERDY, PLLON, PLLRDY |
| RCC_PLLCFGR | 0x40023804 | PLL config | PLLM=8, PLLN=336, PLLP=4, PLLQ=7 → 84 MHz |
| RCC_CFGR | 0x40023808 | Clock config | SW=PLL, AHB=/1, APB1=/4, APB2=/2 |
| RCC_AHB1ENR | 0x40023830 | AHB1 enable | GPIOA, GPIOB, GPIOC, DMA1, DMA2 |
| RCC_APB1ENR | 0x40023840 | APB1 enable | SPI2, I2C1, USART2, PWREN |
| RCC_APB2ENR | 0x40023844 | APB2 enable | SPI1, SYSCFG, OTGFS |

#### GPIO (GPIOA)

| Register | Address | Purpose |
|----------|---------|---------|
| GPIOA_MODER | 0x40020000 | Mode (00=input, 01=output, 10=alt, 03=analog) |
| GPIOA_OTYPER | 0x40020004 | Output type (0=PP, 1=OD) |
| GPIOA_OSPEEDR | 0x40020008 | Speed (00=low, 01=med, 10=fast, 11=high) |
| GPIOA_PUPDR | 0x4002000C | Pull-up/pull-down |
| GPIOA_IDR | 0x40020010 | Input data |
| GPIOA_ODR | 0x40020014 | Output data |
| GPIOA_BSRR | 0x40020018 | Bit set/reset |
| GPIOA_AFRL | 0x40020020 | Alternate function low |
| GPIOA_AFRH | 0x40020024 | Alternate function high |

#### SPI1 (Radio A)

| Register | Address | Purpose | Reset |
|----------|---------|---------|-------|
| SPI1_CR1 | 0x40013000 | Control 1 | 0x0000 |
| SPI1_CR2 | 0x40013004 | Control 2 | 0x0000 |
| SPI1_SR | 0x40013008 | Status | 0x0002 |
| SPI1_DR | 0x4001300C | Data | — |
| SPI1_CRCPR | 0x40013010 | CRC poly | 0x0007 |

#### SPI2 (Radio B)

| Register | Address | Purpose |
|----------|---------|---------|
| SPI2_CR1 | 0x40003800 | Control 1 |
| SPI2_CR2 | 0x40003804 | Control 2 |
| SPI2_SR | 0x40003808 | Status |
| SPI2_DR | 0x4000380C | Data |

#### DMA1

| Register | Address | Purpose |
|----------|---------|---------|
| DMA1_LISR | 0x40026008 | Low interrupt status |
| DMA1_HISR | 0x4002600C | High interrupt status |
| DMA1_S3CR | 0x40026030 | Stream 3 config (SPI1_RX) |
| DMA1_S5CR | 0x40026058 | Stream 5 config (SPI2_RX) |

#### USB OTG FS

| Register | Address | Purpose |
|----------|---------|---------|
| OTG_FS_GOTGCTL | 0x50000000 | OTG control |
| OTG_FS_GUSBCFG | 0x5000000C | USB config |
| OTG_FS_GINTSTS | 0x50000014 | Interrupt status |
| OTG_FS_GINTMSK | 0x50000018 | Interrupt mask |
| OTG_FS_DAINT | 0x50000018 | Device interrupt |
| OTG_FS_DIEPCTL0 | 0x50000100 | EP0 IN control |
| OTG_FS_DOEPCTL0 | 0x50000800 | EP0 OUT control |

#### EXTI

| Register | Address | Purpose |
|----------|---------|---------|
| EXTI_IMR | 0x40013C00 | Interrupt mask |
| EXTI_EMR | 0x40013C04 | Event mask |
| EXTI_RTSR | 0x40013C08 | Rising trigger |
| EXTI_FTSR | 0x40013C0C | Falling trigger |
| EXTI_PR | 0x40013C14 | Pending register |

## 3. Clock and GPIO Initialization

### 3.1 Clock Tree Configuration

```
HSE (8 MHz crystal)
  │
  ├─ PLL
  │    ├─ PLLM = 8    → 1 MHz input
  │    ├─ PLLN = 336  → 336 MHz VCO
  │    ├─ PLLP = 4    → 84 MHz SYSCLK
  │    └─ PLLQ = 7    → 48 MHz USB OTG
  │
  ├─ AHB  = /1  → 84 MHz  (DMA, GPIO, SRAM)
  ├─ APB1 = /4  → 21 MHz  (SPI2, I2C1, USART2)
  └─ APB2 = /2  → 42 MHz  (SPI1, SYSCFG, OTGFS)
```

### 3.2 Clock Init Code

```c
void system_clock_init(void) {
    // Enable HSE
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Configure PLL: HSE/8 * 336 / 4 = 84 MHz
    RCC->PLLCFGR = (8 << 0)    // PLLM = 8
                  | (336 << 6)  // PLLN = 336
                  | (0 << 16)   // PLLP = 2 (encoded as 0)
                  | (7 << 24)   // PLLQ = 7
                  | RCC_PLLCFGR_PLLSRC_HSE;

    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Flash: 2 wait states for 84 MHz
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_2WS;

    // Switch SYSCLK to PLL
    RCC->CFGR = (RCC_CFGR_SW_PLL << 0)   // SYSCLK = PLL
              | (0 << 4)                   // AHB = /1
              | (0b101 << 10)              // APB1 = /4 (21 MHz)
              | (0b100 << 13);             // APB2 = /2 (42 MHz)
    
    while ((RCC->CFGR & RCC_CFGR_SWS) != (RCC_CFGR_SW_PLL << 2));
}
```

### 3.3 GPIO Configuration

```c
void gpio_init(void) {
    // Enable all GPIO clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN 
                  | RCC_AHB1ENR_GPIOCEN;

    // --- PA0: Radio A IRQ (EXTI0, input, pull-up) ---
    GPIOA->MODER &= ~(3 << (0 * 2));    // Input
    GPIOA->PUPDR |= (1 << (0 * 2));     // Pull-up

    // --- PA1: Radio B IRQ (EXTI1, input, pull-up) ---
    GPIOA->MODER &= ~(3 << (1 * 2));    // Input
    GPIOA->PUPDR |= (1 << (1 * 2));     // Pull-up

    // --- PA4: Radio A RESET (output, push-pull) ---
    GPIOA->MODER |= (1 << (4 * 2));     // Output
    GPIOA->OTYPER &= ~(1 << 4);         // Push-pull
    GPIOA->OSPEEDR |= (2 << (4 * 2));   // Fast speed
    GPIOA->ODR |= (1 << 4);             // High = not reset

    // --- PA5/PA6/PA7: SPI1 (AF5) ---
    GPIOA->MODER |= (2 << (5 * 2)) | (2 << (6 * 2)) | (2 << (7 * 2));
    GPIOA->AFRL |= (5 << (5 * 4)) | (5 << (6 * 4)) | (5 << (7 * 4));
    GPIOA->OSPEEDR |= (2 << (5 * 2)) | (2 << (6 * 2)) | (2 << (7 * 2));

    // --- PA8: Radio A SPI CS (output, push-pull) ---
    GPIOA->MODER |= (1 << (8 * 2));
    GPIOA->OTYPER &= ~(1 << 8);
    GPIOA->OSPEEDR |= (2 << (8 * 2));
    GPIOA->ODR |= (1 << 8);             // CS high = deselected

    // --- PB2: Radio B RESET (output, push-pull) ---
    GPIOB->MODER |= (1 << (2 * 2));
    GPIOB->OTYPER &= ~(1 << 2);
    GPIOB->OSPEEDR |= (2 << (2 * 2));
    GPIOB->ODR |= (1 << 2);             // High = not reset

    // --- PB10/PB11/PB14/PB15: SPI2 (AF5) ---
    // PB10=SPI2_SCK, PB14=SPI2_MISO, PB15=SPI2_MOSI
    GPIOB->MODER |= (2 << (10 * 2)) | (2 << (14 * 2)) | (2 << (15 * 2));
    GPIOB->AFRH |= (5 << ((10 - 8) * 4)) | (5 << ((14 - 8) * 4)) | (5 << ((15 - 8) * 4));
    GPIOB->OSPEEDR |= (2 << (10 * 2)) | (2 << (14 * 2)) | (2 << (15 * 2));

    // --- PB12: Radio B SPI CS (output) ---
    GPIOB->MODER |= (1 << (12 * 2));
    GPIOB->OTYPER &= ~(1 << 12);
    GPIOB->OSPEEDR |= (2 << (12 * 2));
    GPIOB->ODR |= (1 << 12);            // CS high = deselected

    // --- PA11/PA12: USB (AF10 = OTG_FS) ---
    GPIOA->MODER |= (2 << (11 * 2)) | (2 << (12 * 2));
    GPIOA->AFRH |= (10 << ((11 - 8) * 4)) | (10 << ((12 - 8) * 4));
    GPIOA->OSPEEDR |= (3 << (11 * 2)) | (3 << (12 * 2)); // Very high speed

    // --- PB5/PB6: I2C1 (AF4) ---
    GPIOB->MODER |= (2 << (5 * 2)) | (2 << (6 * 2));
    GPIOB->AFRL |= (4 << (5 * 4)) | (4 << (6 * 4));
    GPIOB->OTYPER |= (1 << 5) | (1 << 6); // Open-drain
    GPIOB->OSPEEDR |= (1 << (5 * 2)) | (1 << (6 * 2));

    // --- LEDs: PB0, PB1, PC14, PC15 (output) ---
    GPIOB->MODER |= (1 << (0 * 2)) | (1 << (1 * 2));
    GPIOC->MODER |= (1 << (14 * 2)) | (1 << (15 * 2));

    // --- PC13: User button (input, pull-up) ---
    GPIOC->MODER &= ~(3 << (13 * 2));
    GPIOC->PUPDR |= (1 << (13 * 2));
}
```

## 4. Device Drivers

### 4.1 SPI Radio Driver (spi_radio.c / spi_radio.h)

```c
// spi_radio.h
#ifndef SPI_RADIO_H
#define SPI_RADIO_H

#include <stdint.h>
#include <stddef.h>

// Radio instance identifiers
typedef enum {
    RADIO_A = 0,
    RADIO_B = 1,
    RADIO_COUNT = 2
} radio_id_t;

// SPI command opcodes
#define CMD_RADIO_INIT       0x01
#define CMD_START_ADV        0x02
#define CMD_START_SCAN       0x03
#define CMD_CONNECT          0x04
#define CMD_TX_DATA          0x05
#define CMD_RX_DATA          0x06
#define CMD_SET_CHANNEL      0x07
#define CMD_SET_TX_POWER     0x08
#define CMD_GET_STATUS       0x10
#define CMD_RESET            0xFF

// SPI protocol constants
#define SPI_SYNC_WORD        0xAA55
#define SPI_TRAILER_WORD     0x55AA
#define SPI_HEADER_SIZE      5    // sync(2) + cmd(1) + len(2)
#define SPI_MAX_PAYLOAD      255
#define SPI_CRC16_INIT       0xFFFF

// Radio status flags
#define RADIO_STATUS_INITIALIZED  (1 << 0)
#define RADIO_STATUS_SCANNING     (1 << 1)
#define RADIO_STATUS_ADVERTISING  (1 << 2)
#define RADIO_STATUS_CONNECTED    (1 << 3)
#define RADIO_STATUS_RX_PENDING   (1 << 4)
#define RADIO_STATUS_TX_COMPLETE  (1 << 5)
#define RADIO_STATUS_ERROR        (1 << 7)

// Radio configuration
typedef struct {
    uint8_t channel;        // BLE channel (0-36 data, 37-39 adv)
    int8_t tx_power;         // -40 to +4 dBm
    uint16_t adv_interval;  // Advertising interval in 0.625 ms units
    uint16_t scan_window;   // Scan window in 0.625 ms units
    uint16_t scan_interval; // Scan interval in 0.625 ms units
    uint8_t phy;            // 1M, 2M, coded
} radio_config_t;

// Received packet
typedef struct {
    uint8_t channel;
    int8_t rssi;
    uint8_t length;
    uint8_t data[255];
} radio_packet_t;

// Initialize SPI radio driver
int spi_radio_init(radio_id_t radio);

// Send command to radio
int spi_radio_send_cmd(radio_id_t radio, uint8_t cmd, 
                       const uint8_t *payload, uint16_t len);

// Read response from radio (non-blocking, check IRQ first)
int spi_radio_read_response(radio_id_t radio, uint8_t *cmd,
                            uint8_t *payload, uint16_t *len);

// Configure radio parameters
int spi_radio_configure(radio_id_t radio, const radio_config_t *config);

// Start advertising
int spi_radio_start_adv(radio_id_t radio);

// Start scanning
int spi_radio_start_scan(radio_id_t radio);

// Connect to a device
int spi_radio_connect(radio_id_t radio, const uint8_t *addr, uint8_t addr_type);

// Send data on established connection
int spi_radio_send_data(radio_id_t radio, const uint8_t *data, uint16_t len);

// Get radio status
uint8_t spi_radio_get_status(radio_id_t radio);

// Reset radio
int spi_radio_reset(radio_id_t radio);

// IRQ handler (called from EXTI ISR)
void spi_radio_irq_handler(radio_id_t radio);

// CRC16-CCITT calculation
uint16_t spi_crc16(const uint8_t *data, size_t len);

#endif // SPI_RADIO_H
```

```c
// spi_radio.c
#include "spi_radio.h"
#include "registers.h"
#include "board.h"

// Radio instance state
typedef struct {
    volatile uint8_t status;
    volatile uint8_t rx_pending;
    radio_packet_t rx_packet;
    uint8_t cs_pin_port;
    uint16_t cs_pin;
    uint8_t reset_pin_port;
    uint16_t reset_pin;
    SPI_TypeDef *spi;
} radio_state_t;

static radio_state_t radios[RADIO_COUNT] = {
    {   // Radio A
        .cs_pin_port = 'A',
        .cs_pin = 8,
        .reset_pin_port = 'A',
        .reset_pin = 4,
        .spi = SPI1
    },
    {   // Radio B
        .cs_pin_port = 'B',
        .cs_pin = 12,
        .reset_pin_port = 'B',
        .reset_pin = 2,
        .spi = SPI2
    }
};

static inline void cs_select(radio_id_t radio) {
    radio_state_t *r = &radios[radio];
    if (r->cs_pin_port == 'A')
        GPIOA->ODR &= ~(1 << r->cs_pin);
    else
        GPIOB->ODR &= ~(1 << r->cs_pin);
}

static inline void cs_deselect(radio_id_t radio) {
    radio_state_t *r = &radios[radio];
    if (r->cs_pin_port == 'A')
        GPIOA->ODR |= (1 << r->cs_pin);
    else
        GPIOB->ODR |= (1 << r->cs_pin);
}

static inline void reset_assert(radio_id_t radio) {
    radio_state_t *r = &radios[radio];
    if (r->reset_pin_port == 'A')
        GPIOA->ODR &= ~(1 << r->reset_pin);
    else
        GPIOB->ODR &= ~(1 << r->reset_pin);
}

static inline void reset_deassert(radio_id_t radio) {
    radio_state_t *r = &radios[radio];
    if (r->reset_pin_port == 'A')
        GPIOA->ODR |= (1 << r->reset_pin);
    else
        GPIOB->ODR |= (1 << r->reset_pin);
}

static uint8_t spi_transfer_byte(SPI_TypeDef *spi, uint8_t tx) {
    while (!(spi->SR & SPI_SR_TXE));
    spi->DR = tx;
    while (!(spi->SR & SPI_SR_RXNE));
    return (uint8_t)spi->DR;
}

uint16_t spi_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = SPI_CRC16_INIT;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

int spi_radio_init(radio_id_t radio) {
    radio_state_t *r = &radios[radio];
    
    // Reset the radio
    reset_assert(radio);
    for (volatile int i = 0; i < 10000; i++); // ~100 µs
    reset_deassert(radio);
    for (volatile int i = 0; i < 100000; i++); // ~1 ms
    
    r->status = 0;
    r->rx_pending = 0;
    
    // Send INIT command
    return spi_radio_send_cmd(radio, CMD_RADIO_INIT, NULL, 0);
}

int spi_radio_send_cmd(radio_id_t radio, uint8_t cmd,
                       const uint8_t *payload, uint16_t len) {
    if (len > SPI_MAX_PAYLOAD) return -1;
    
    radio_state_t *r = &radios[radio];
    uint16_t total_len = len;
    
    // Build frame
    uint8_t frame[SPI_HEADER_SIZE + SPI_MAX_PAYLOAD + 4]; // +CRC+trailer
    frame[0] = (SPI_SYNC_WORD >> 8) & 0xFF; // 0xAA
    frame[1] = SPI_SYNC_WORD & 0xFF;        // 0x55
    frame[2] = cmd;
    frame[3] = (total_len >> 8) & 0xFF;
    frame[4] = total_len & 0xFF;
    
    if (len > 0 && payload) {
        for (uint16_t i = 0; i < len; i++)
            frame[5 + i] = payload[i];
    }
    
    // CRC over header + payload
    uint16_t crc = spi_crc16(frame, 5 + len);
    frame[5 + len] = (crc >> 8) & 0xFF;
    frame[5 + len + 1] = crc & 0xFF;
    frame[5 + len + 2] = (SPI_TRAILER_WORD >> 8) & 0xFF;
    frame[5 + len + 3] = SPI_TRAILER_WORD & 0xFF;
    
    // Transfer via SPI
    cs_select(radio);
    for (uint16_t i = 0; i < 5 + len + 4; i++) {
        spi_transfer_byte(r->spi, frame[i]);
    }
    cs_deselect(radio);
    
    return 0;
}

int spi_radio_read_response(radio_id_t radio, uint8_t *cmd,
                            uint8_t *payload, uint16_t *len) {
    radio_state_t *r = &radios[radio];
    
    if (!r->rx_pending) return -1;
    r->rx_pending = 0;
    
    // Read response frame from radio
    cs_select(radio);
    
    // Read sync
    uint8_t sync_hi = spi_transfer_byte(r->spi, 0xFF);
    uint8_t sync_lo = spi_transfer_byte(r->spi, 0xFF);
    if ((sync_hi << 8 | sync_lo) != SPI_SYNC_WORD) {
        cs_deselect(radio);
        return -2; // Sync error
    }
    
    // Read header
    *cmd = spi_transfer_byte(r->spi, 0xFF);
    uint8_t len_hi = spi_transfer_byte(r->spi, 0xFF);
    uint8_t len_lo = spi_transfer_byte(r->spi, 0xFF);
    *len = (len_hi << 8) | len_lo;
    
    // Read payload
    for (uint16_t i = 0; i < *len && i < SPI_MAX_PAYLOAD; i++) {
        payload[i] = spi_transfer_byte(r->spi, 0xFF);
    }
    
    // Read CRC and trailer (verify)
    spi_transfer_byte(r->spi, 0xFF); // CRC hi
    spi_transfer_byte(r->spi, 0xFF); // CRC lo
    spi_transfer_byte(r->spi, 0xFF); // Trailer hi
    spi_transfer_byte(r->spi, 0xFF); // Trailer lo
    
    cs_deselect(radio);
    return 0;
}

int spi_radio_configure(radio_id_t radio, const radio_config_t *config) {
    uint8_t buf[8];
    buf[0] = config->channel;
    buf[1] = (uint8_t)config->tx_power;
    buf[2] = (config->adv_interval >> 8) & 0xFF;
    buf[3] = config->adv_interval & 0xFF;
    buf[4] = (config->scan_window >> 8) & 0xFF;
    buf[5] = config->scan_window & 0xFF;
    buf[6] = (config->scan_interval >> 8) & 0xFF;
    buf[7] = config->scan_interval & 0xFF;
    return spi_radio_send_cmd(radio, CMD_SET_CHANNEL, buf, 8);
}

int spi_radio_start_adv(radio_id_t radio) {
    return spi_radio_send_cmd(radio, CMD_START_ADV, NULL, 0);
}

int spi_radio_start_scan(radio_id_t radio) {
    return spi_radio_send_cmd(radio, CMD_START_SCAN, NULL, 0);
}

int spi_radio_connect(radio_id_t radio, const uint8_t *addr, uint8_t addr_type) {
    uint8_t buf[7];
    buf[0] = addr_type;
    for (int i = 0; i < 6; i++) buf[1 + i] = addr[i];
    return spi_radio_send_cmd(radio, CMD_CONNECT, buf, 7);
}

int spi_radio_send_data(radio_id_t radio, const uint8_t *data, uint16_t len) {
    return spi_radio_send_cmd(radio, CMD_TX_DATA, data, len);
}

uint8_t spi_radio_get_status(radio_id_t radio) {
    return radios[radio].status;
}

int spi_radio_reset(radio_id_t radio) {
    reset_assert(radio);
    for (volatile int i = 0; i < 100000; i++);
    reset_deassert(radio);
    for (volatile int i = 0; i < 100000; i++);
    radios[radio].status = 0;
    return 0;
}

void spi_radio_irq_handler(radio_id_t radio) {
    radios[radio].rx_pending = 1;
    radios[radio].status |= RADIO_STATUS_RX_PENDING;
}
```

### 4.2 USB CDC Driver (usb_cdc.c / usb_cdc.h)

```c
// usb_cdc.h
#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>
#include <stddef.h>

// USB CDC packet sizes
#define USB_CDC_EP_SIZE     64
#define USB_CDC_BUF_SIZE    512

// Callback types
typedef void (*usb_rx_callback_t)(const uint8_t *data, uint16_t len);

// Initialize USB CDC device
int usb_cdc_init(void);

// Send data over CDC
int usb_cdc_send(const uint8_t *data, uint16_t len);

// Register receive callback
void usb_cdc_set_rx_callback(usb_rx_callback_t cb);

// Check if USB is configured (connected)
int usb_cdc_is_configured(void);

// USB ISR handler (call from OTG_FS_IRQHandler)
void usb_cdc_isr(void);

#endif // USB_CDC_H
```

```c
// usb_cdc.c — Minimal USB CDC ACM device driver for STM32F401
// Uses STM32 OTG FS peripheral in device mode

#include "usb_cdc.h"
#include "registers.h"
#include "board.h"
#include "usb_descriptors.h"

static usb_rx_callback_t rx_callback = NULL;
static volatile int usb_configured = 0;
static volatile int usb_suspended = 0;

// RX/TX buffers
static uint8_t rx_buf[USB_CDC_BUF_SIZE];
static uint16_t rx_len = 0;

// Endpoint states
static volatile uint8_t ep0_state = 0;
static volatile uint16_t ep0_rx_remaining = 0;

static void usb_write_reg(volatile uint32_t *reg, uint32_t val) {
    *reg = val;
}

static uint32_t usb_read_reg(volatile uint32_t *reg) {
    return *reg;
}

int usb_cdc_init(void) {
    // Enable OTG FS clock
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    
    // Configure USB pins (PA11=DM, PA12=DP) - done in gpio_init()
    
    // Reset OTG FS
    OTG_FS_GLOBAL->GRSTCTL = OTG_GRSTCTL_CSRST;
    while (OTG_FS_GLOBAL->GRSTCTL & OTG_GRSTCTL_CSRST);
    
    // Configure OTG FS for device mode, FS
    OTG_FS_GLOBAL->GUSBCFG = OTG_GUSBCFG_FDMOD 
                            | OTG_GUSBCFG_TRDT(5) 
                            | OTG_GUSBCFG_PHYSEL;
    
    // Device configuration
    OTG_FS_DEVICE->DCFG = OTG_DCFG_DSPD_FULL 
                         | OTG_DCFG_NZLSOHSK;
    
    // Set RX FIFO size (128 words = 512 bytes)
    OTG_FS_GLOBAL->GRXFSIZ = 128;
    // Set TX FIFO sizes: EP0 = 16 words, EP1 = 64 words (CDC data), EP2 = 16 words (CDC notify)
    OTG_FS_GLOBAL->DIEPTXF0 = (16 << 16) | 128;
    OTG_FS_GLOBAL->DIEPTXF1 = (64 << 16) | (128 + 16);
    OTG_FS_GLOBAL->DIEPTXF2 = (16 << 16) | (128 + 16 + 64);
    
    // Enable interrupts
    OTG_FS_GLOBAL->GINTMSK = OTG_GINTMSK_USBRST 
                            | OTG_GINTMSK_ENUMDNE 
                            | OTG_GINTMSK_RXFLVL 
                            | OTG_GINTMSK_IEPINT 
                            | OTG_GINTMSK_OEPINT;
    
    // Unmask global interrupt
    OTG_FS_GLOBAL->GAHBCFG = OTG_GAHBCFG_GINT;
    
    // Enable device mode
    OTG_FS_DEVICE->DCTL &= ~OTG_DCTL_SDIS;
    
    return 0;
}

int usb_cdc_send(const uint8_t *data, uint16_t len) {
    if (!usb_configured) return -1;
    if (len > USB_CDC_EP_SIZE) len = USB_CDC_EP_SIZE;
    
    // Wait for EP1 TX FIFO empty
    while (!(OTG_FS_DEVICE->DIEPINT1 & OTG_DIEPINT_TXFE));
    
    // Write data to FIFO
    volatile uint32_t *fifo = (volatile uint32_t *)OTG_FS_FIFO_BASE(1);
    uint16_t i;
    for (i = 0; i + 3 < len; i += 4) {
        uint32_t word = data[i] | (data[i+1] << 8) 
                      | (data[i+2] << 16) | (data[i+3] << 24);
        *fifo = word;
    }
    // Handle remaining bytes
    if (i < len) {
        uint32_t word = 0;
        for (uint16_t j = 0; i + j < len; j++) {
            word |= (data[i + j] << (j * 8));
        }
        *fifo = word;
    }
    
    // Set transfer size
    OTG_FS_DEVICE->DIEPTSIZ1 = (1 << 19) | len; // 1 packet, len bytes
    OTG_FS_DEVICE->DIEPCTL1 |= OTG_DIEPCTL_EPENA;
    
    return len;
}

void usb_cdc_set_rx_callback(usb_rx_callback_t cb) {
    rx_callback = cb;
}

int usb_cdc_is_configured(void) {
    return usb_configured;
}

void usb_cdc_isr(void) {
    uint32_t gintsts = OTG_FS_GLOBAL->GINTSTS;
    
    // USB Reset
    if (gintsts & OTG_GINTSTS_USBRST) {
        OTG_FS_GLOBAL->GINTSTS = OTG_GINTSTS_USBRST;
        usb_configured = 0;
        rx_len = 0;
        
        // Reset all endpoints
        OTG_FS_DEVICE->DCTL |= OTG_DCTL_RWUSIG;
    }
    
    // Enumeration done
    if (gintsts & OTG_GINTSTS_ENUMDNE) {
        OTG_FS_GLOBAL->GINTSTS = OTG_GINTSTS_ENUMDNE;
        usb_configured = 1;
    }
    
    // RX FIFO not empty
    if (gintsts & OTG_GINTSTS_RXFLVL) {
        uint32_t grxstsp = OTG_FS_GLOBAL->GRXSTSP;
        uint8_t ep_num = grxstsp & 0xF;
        uint8_t pkt_status = (grxstsp >> 17) & 0xF;
        uint16_t pkt_len = (grxstsp >> 4) & 0x7FF;
        
        if (pkt_status == 0x02 && ep_num == 1) {
            // OUT data on EP1 (CDC data)
            volatile uint32_t *fifo = (volatile uint32_t *)OTG_FS_FIFO_BASE(1);
            uint16_t words = (pkt_len + 3) / 4;
            uint16_t idx = 0;
            for (uint16_t w = 0; w < words; w++) {
                uint32_t word = *fifo;
                for (int b = 0; b < 4 && idx < pkt_len && idx < USB_CDC_BUF_SIZE; b++) {
                    rx_buf[idx++] = (word >> (b * 8)) & 0xFF;
                }
            }
            rx_len = idx;
            
            if (rx_callback && rx_len > 0) {
                rx_callback(rx_buf, rx_len);
            }
        }
    }
}
```

## 5. Device Tree (Conceptual)

Since this is a bare-metal Cortex-M system, there is no Linux device tree. Instead, we use a compile-time configuration in `board.h`:

```c
// board.h — Hardware description for BLE Phantom
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

// Clock configuration
#define CLOCK_HSE_FREQ      8000000U    // 8 MHz crystal
#define CLOCK_SYS_FREQ      84000000U    // 84 MHz SYSCLK
#define CLOCK_AHB_FREQ      84000000U    // 84 MHz AHB
#define CLOCK_APB1_FREQ     21000000U    // 21 MHz APB1
#define CLOCK_APB2_FREQ     42000000U    // 42 MHz APB2

// GPIO pin assignments
#define PIN_RADIO_A_IRQ      0    // PA0 - EXTI0
#define PIN_RADIO_B_IRQ      1    // PA1 - EXTI1
#define PIN_RADIO_A_RST      4    // PA4
#define PIN_RADIO_A_CS       8    // PA8
#define PIN_RADIO_B_RST      2    // PB2
#define PIN_RADIO_B_CS       12   // PB12
#define PIN_USER_BUTTON      13   // PC13
#define PIN_LED_RADIO_A      0    // PB0
#define PIN_LED_RADIO_B      1    // PB1
#define PIN_LED_USB          14   // PC14
#define PIN_LED_POWER        15   // PC15

// SPI configuration
#define SPI1_SPEED_HZ       8000000U    // 8 MHz SPI clock
#define SPI2_SPEED_HZ       8000000U    // 8 MHz SPI clock

// Radio A SPI instance
#define RADIO_A_SPI          SPI1
#define RADIO_A_SPI_CLK_EN   RCC_APB2ENR_SPI1EN

// Radio B SPI instance  
#define RADIO_B_SPI          SPI2
#define RADIO_B_SPI_CLK_EN   RCC_APB1ENR_SPI2EN

// DMA configuration
#define DMA_RADIO_A_STREAM   DMA1_Stream3   // SPI1_RX
#define DMA_RADIO_A_CHANNEL   3              // Channel 3 for SPI1_RX
#define DMA_RADIO_B_STREAM   DMA1_Stream5   // SPI2_RX
#define DMA_RADIO_B_CHANNEL  5              // Channel 5 for SPI2_RX

// Ring buffer sizes
#define RADIO_RING_BUF_SIZE  (32 * 1024)    // 32 KB per radio
#define USB_TX_BUF_SIZE      512
#define USB_RX_BUF_SIZE      512

// Battery / power
#define BATTERY_CAPACITY_MAH  600
#define VBUS_DETECT_PIN       9    // PA9

// LED macros
#define LED_RADIO_A_ON()     GPIOB->ODR &= ~(1 << PIN_LED_RADIO_A)
#define LED_RADIO_A_OFF()    GPIOB->ODR |= (1 << PIN_LED_RADIO_A)
#define LED_RADIO_B_ON()     GPIOB->ODR &= ~(1 << PIN_LED_RADIO_B)
#define LED_RADIO_B_OFF()    GPIOB->ODR |= (1 << PIN_LED_RADIO_B)
#define LED_USB_ON()         GPIOC->ODR &= ~(1 << PIN_LED_USB)
#define LED_USB_OFF()        GPIOC->ODR |= (1 << PIN_LED_USB)
#define LED_POWER_ON()       GPIOC->ODR &= ~(1 << PIN_LED_POWER)
#define LED_POWER_OFF()      GPIOC->ODR |= (1 << PIN_LED_POWER)

// Button
#define BUTTON_PRESSED()     !(GPIOC->IDR & (1 << PIN_USER_BUTTON))

#endif // BOARD_H
```

## 6. Main Firmware

The `main.c` implements the main event loop:

```c
// main.c — BLE Phantom main firmware
#include "board.h"
#include "registers.h"
#include "spi_radio.h"
#include "usb_cdc.h"
#include <string.h>

// Operating modes
typedef enum {
    MODE_IDLE = 0,
    MODE_SNIFF,
    MODE_SCAN,
    MODE_ADVERTISE,
    MODE_CONNECT,
    MODE_MITM,
    MODE_REPLAY
} device_mode_t;

static volatile device_mode_t current_mode = MODE_IDLE;
static radio_config_t config_a = {0};
static radio_config_t config_b = {0};

// Command protocol from host app
#define CMD_HOST_SET_MODE      0xA0
#define CMD_HOST_SET_CONFIG     0xA1
#define CMD_HOST_START          0xA2
#define CMD_HOST_STOP           0xA3
#define CMD_HOST_REPLAY         0xA4
#define CMD_HOST_STATUS         0xA5

static void handle_host_command(const uint8_t *data, uint16_t len);
static void process_radio_a(void);
static void process_radio_b(void);
static void mitm_relay(void);

int main(void) {
    // Initialize hardware
    system_clock_init();
    gpio_init();
    
    // Initialize USB CDC
    usb_cdc_init();
    usb_cdc_set_rx_callback(handle_host_command);
    
    // Initialize SPI radios
    spi_radio_init(RADIO_A);
    spi_radio_init(RADIO_B);
    
    // Default configuration
    config_a.channel = 37;  // Advertising channel 37
    config_a.tx_power = 4;   // +4 dBm
    config_a.adv_interval = 160; // 100 ms
    config_a.phy = 1;        // 1 Mbps
    
    config_b.channel = 38;   // Advertising channel 38
    config_b.tx_power = 4;
    config_b.scan_window = 16; // 10 ms
    config_b.scan_interval = 16;
    config_b.phy = 1;
    
    spi_radio_configure(RADIO_A, &config_a);
    spi_radio_configure(RADIO_B, &config_b);
    
    // Turn on power LED
    LED_POWER_ON();
    
    // Main loop
    while (1) {
        // Check for host commands (handled via USB CDC ISR)
        
        // Process radio interrupts
        if (radios[RADIO_A].rx_pending) {
            process_radio_a();
        }
        if (radios[RADIO_B].rx_pending) {
            process_radio_b();
        }
        
        // MITM relay mode
        if (current_mode == MODE_MITM) {
            mitm_relay();
        }
        
        // Check button press (mode cycle)
        if (BUTTON_PRESSED()) {
            // Debounce
            for (volatile int i = 0; i < 840000; i++);
            if (BUTTON_PRESSED()) {
                current_mode = (device_mode_t)((current_mode + 1) % 7);
                // Update LEDs based on mode
                LED_RADIO_A_OFF();
                LED_RADIO_B_OFF();
                LED_USB_OFF();
                switch (current_mode) {
                    case MODE_SNIFF:      LED_RADIO_A_ON(); break;
                    case MODE_SCAN:       LED_RADIO_B_ON(); break;
                    case MODE_ADVERTISE:  LED_RADIO_A_ON(); LED_RADIO_B_ON(); break;
                    case MODE_CONNECT:    LED_RADIO_A_ON(); LED_USB_ON(); break;
                    case MODE_MITM:       LED_RADIO_A_ON(); LED_RADIO_B_ON(); LED_USB_ON(); break;
                    default: break;
                }
            }
        }
    }
}

static void handle_host_command(const uint8_t *data, uint16_t len) {
    if (len < 1) return;
    
    uint8_t cmd = data[0];
    uint8_t response[64];
    
    switch (cmd) {
        case CMD_HOST_SET_MODE: {
            if (len < 2) break;
            current_mode = (device_mode_t)data[1];
            response[0] = 0xA0;
            response[1] = 0x00; // OK
            usb_cdc_send(response, 2);
            break;
        }
        case CMD_HOST_SET_CONFIG: {
            if (len < 3) break;
            radio_id_t radio = (radio_id_t)data[1];
            radio_config_t *cfg = (radio == RADIO_A) ? &config_a : &config_b;
            if (len >= 10) {
                cfg->channel = data[2];
                cfg->tx_power = (int8_t)data[3];
                cfg->adv_interval = (data[4] << 8) | data[5];
                cfg->scan_window = (data[6] << 8) | data[7];
                cfg->scan_interval = (data[8] << 8) | data[9];
                cfg->phy = data[10];
                spi_radio_configure(radio, cfg);
            }
            response[0] = 0xA1;
            response[1] = 0x00;
            usb_cdc_send(response, 2);
            break;
        }
        case CMD_HOST_START: {
            if (len < 2) break;
            radio_id_t radio = (radio_id_t)data[1];
            if (current_mode == MODE_SNIFF || current_mode == MODE_SCAN) {
                spi_radio_start_scan(radio);
            } else if (current_mode == MODE_ADVERTISE) {
                spi_radio_start_adv(radio);
            }
            response[0] = 0xA2;
            response[1] = 0x00;
            usb_cdc_send(response, 2);
            break;
        }
        case CMD_HOST_STATUS: {
            response[0] = 0xA5;
            response[1] = (uint8_t)current_mode;
            response[2] = spi_radio_get_status(RADIO_A);
            response[3] = spi_radio_get_status(RADIO_B);
            usb_cdc_send(response, 4);
            break;
        }
        default:
            break;
    }
}

static void process_radio_a(void) {
    uint8_t cmd;
    uint8_t payload[255];
    uint16_t len;
    
    int rc = spi_radio_read_response(RADIO_A, &cmd, payload, &len);
    if (rc == 0 && usb_cdc_is_configured()) {
        // Forward captured packet to host via USB CDC
        uint8_t pkt[260];
        pkt[0] = 0x01; // Radio A marker
        pkt[1] = cmd;
        pkt[2] = (len >> 8) & 0xFF;
        pkt[3] = len & 0xFF;
        memcpy(&pkt[4], payload, len);
        usb_cdc_send(pkt, 4 + len);
    }
}

static void process_radio_b(void) {
    uint8_t cmd;
    uint8_t payload[255];
    uint16_t len;
    
    int rc = spi_radio_read_response(RADIO_B, &cmd, payload, &len);
    if (rc == 0 && usb_cdc_is_configured()) {
        uint8_t pkt[260];
        pkt[0] = 0x02; // Radio B marker
        pkt[1] = cmd;
        pkt[2] = (len >> 8) & 0xFF;
        pkt[3] = len & 0xFF;
        memcpy(&pkt[4], payload, len);
        usb_cdc_send(pkt, 4 + len);
    }
}

static void mitm_relay(void) {
    // In MITM mode, packets from Radio A are forwarded to Radio B
    // and vice versa, with optional modification
    if (radios[RADIO_A].rx_pending) {
        uint8_t cmd;
        uint8_t payload[255];
        uint16_t len;
        if (spi_radio_read_response(RADIO_A, &cmd, payload, &len) == 0) {
            // Forward to Radio B (optionally modify payload here)
            spi_radio_send_data(RADIO_B, payload, len);
            LED_RADIO_A_ON();
        }
    }
    if (radios[RADIO_B].rx_pending) {
        uint8_t cmd;
        uint8_t payload[255];
        uint16_t len;
        if (spi_radio_read_response(RADIO_B, &cmd, payload, &len) == 0) {
            // Forward to Radio A
            spi_radio_send_data(RADIO_A, payload, len);
            LED_RADIO_B_ON();
        }
    }
}

// Interrupt handlers
void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1 << 0)) {
        EXTI->PR = (1 << 0);
        spi_radio_irq_handler(RADIO_A);
    }
}

void EXTI1_IRQHandler(void) {
    if (EXTI->PR & (1 << 1)) {
        EXTI->PR = (1 << 1);
        spi_radio_irq_handler(RADIO_B);
    }
}

void OTG_FS_IRQHandler(void) {
    usb_cdc_isr();
}
```

## 7. USB Descriptors

```c
// usb_descriptors.h
#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

// Device descriptor
static const uint8_t device_descriptor[] = {
    18,                   // bLength
    0x01,                 // bDescriptorType (DEVICE)
    0x00, 0x02,           // bcdUSB (2.00)
    0x02,                 // bDeviceClass (CDC)
    0x00,                 // bDeviceSubClass
    0x00,                 // bDeviceProtocol
    64,                   // bMaxPacketSize0
    0x83, 0x17,           // idVendor (0x1783 = Shenzhen Zoro)
    0x40, 0xF0,           // idProduct (0xF040 = BLE Phantom)
    0x01, 0x00,           // bcdDevice (1.00)
    1,                    // iManufacturer
    2,                    // iProduct
    3,                    // iSerialNumber
    1                     // bNumConfigurations
};

// Configuration descriptor (CDC ACM)
static const uint8_t config_descriptor[] = {
    // Configuration descriptor
    9,                    // bLength
    0x02,                 // bDescriptorType (CONFIGURATION)
    0x43, 0x00,           // wTotalLength (67 bytes)
    2,                    // bNumInterfaces (2: CDC data + CDC comm)
    1,                    // bConfigurationValue
    0,                    // iConfiguration
    0x80,                 // bmAttributes (bus-powered)
    250,                  // bMaxPower (500 mA)

    // Interface 0: CDC Communications
    9,                    // bLength
    0x04,                 // bDescriptorType (INTERFACE)
    0,                    // bInterfaceNumber
    0,                    // bAlternateSetting
    1,                    // bNumEndpoints (1 INT)
    0x02,                 // bInterfaceClass (CDC)
    0x02,                 // bInterfaceSubClass (ACM)
    0x01,                 // bInterfaceProtocol (V.250)
    0,                    // iInterface

    // CDC Header Functional Descriptor
    5, 0x24, 0x00, 0x10, 0x01,

    // CDC ACM Functional Descriptor
    4, 0x24, 0x02, 0x02,

    // CDC Union Functional Descriptor
    5, 0x24, 0x06, 0x00, 0x01,

    // CDC Call Management Functional Descriptor
    5, 0x24, 0x01, 0x00, 0x01,

    // Endpoint 2: INT IN (CDC notification)
    7,                    // bLength
    0x05,                 // bDescriptorType (ENDPOINT)
    0x82,                 // bEndpointAddress (EP2 IN)
    0x03,                 // bmAttributes (INT)
    8, 0x00,              // wMaxPacketSize (8)
    255,                  // bInterval (255 ms)

    // Interface 1: CDC Data
    9,                    // bLength
    0x04,                 // bDescriptorType (INTERFACE)
    1,                    // bInterfaceNumber
    0,                    // bAlternateSetting
    2,                    // bNumEndpoints (2 BULK)
    0x0A,                 // bInterfaceClass (CDC Data)
    0x00,                 // bInterfaceSubClass
    0x00,                 // bInterfaceProtocol
    0,                    // iInterface

    // Endpoint 1: BULK OUT
    7,                    // bLength
    0x05,                 // bDescriptorType (ENDPOINT)
    0x01,                 // bEndpointAddress (EP1 OUT)
    0x02,                 // bmAttributes (BULK)
    64, 0x00,             // wMaxPacketSize (64)
    0,                    // bInterval

    // Endpoint 3: BULK IN
    7,                    // bLength
    0x05,                 // bDescriptorType (ENDPOINT)
    0x83,                 // bEndpointAddress (EP3 IN)
    0x02,                 // bmAttributes (BULK)
    64, 0x00,             // wMaxPacketSize (64)
    0                     // bInterval
};

// String descriptors
static const uint8_t string_lang[] = {4, 0x03, 0x09, 0x04};  // English
static const uint8_t string_manufacturer[] = {
    22, 0x03, 'N', 0, 'o', 0, 'u', 0, 's', 0, ' ', 0, 
    'R', 0, 'e', 0, 's', 0, 'e', 0, 'a', 0, 'r', 0, 'c', 0, 'h', 0
};
static const uint8_t string_product[] = {
    24, 0x03, 'B', 0, 'L', 0, 'E', 0, ' ', 0, 'P', 0, 'h', 0, 'a', 0,
    'n', 0, 't', 0, 'o', 0, 'm', 0
};
static const uint8_t string_serial[] = {
    18, 0x03, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0
};

#endif // USB_DESCRIPTORS_H
```

## 8. Build Instructions

### 8.1 Prerequisites

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Install Make
sudo apt-get install make

# Clone and build
cd ble-phantom/firmware
make clean
make
```

### 8.2 Makefile

See `firmware/Makefile` for the build system. Key targets:

- `make` — Build firmware.elf
- `make hex` — Build firmware.hex (for flashing)
- `make bin` — Build firmware.bin (for DFU)
- `make flash` — Flash via OpenOCD + ST-Link
- `make dfu` — Package for USB DFU upgrade
- `make clean` — Remove build artifacts

### 8.3 Flashing

**Via SWD (ST-Link v2):**
```bash
make flash
# Or manually:
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg -c "program firmware.elf verify reset exit"
```

**Via USB DFU:**
```bash
make dfu
dfu-util -a 0 -s 0x08000000:leave -D firmware.dfu
```

### 8.4 nRF52832 Radio Firmware

Each nRF52832 requires custom SPI-slave firmware. Build using nRF5 SDK:

```bash
# Requires nRF5 SDK v17.x and Segger Embedded Studio
cd nrf_radio_fw/
make radio_a  # Build for Radio A (SPI slave, BLE peripheral)
make radio_b  # Build for Radio B (SPI slave, BLE central)
```

Flash nRF52832 via SWD using nRF Connect Programmer or OpenOCD.

## 9. Runtime Architecture

```
┌───────────────────────────────────────────────┐
│               Main Event Loop                  │
│                                               │
│  ┌─────────┐  ┌──────────┐  ┌───────────┐    │
│  │ USB CDC │  │ SPI Radio│  │ Button/   │    │
│  │ RX ISR  │  │ A IRQ    │  │ Mode ISR  │    │
│  └────┬────┘  └────┬─────┘  └─────┬─────┘    │
│       │            │              │            │
│       ▼            ▼              ▼            │
│  ┌─────────────────────────────────────┐       │
│  │         Command Dispatcher          │       │
│  │  SET_MODE | SET_CFG | START | STOP │       │
│  └──────────┬──────────────────────┬───┘       │
│             │                      │           │
│    ┌────────▼────────┐   ┌─────────▼──────┐   │
│    │  Radio A Task    │   │  Radio B Task   │   │
│    │  (Sniff/Adv/     │   │  (Sniff/Adv/   │   │
│    │   Connect)       │   │   Connect)     │   │
│    └────────┬────────┘   └────────┬───────┘   │
│             │                      │           │
│             └──────┬───────────────┘           │
│                    ▼                            │
│           ┌──────────────┐                    │
│           │  MITM Relay   │                    │
│           │  Engine       │                    │
│           └───────┬───────┘                    │
│                   │                             │
│                   ▼                             │
│           ┌──────────────┐                    │
│           │  USB CDC TX   │                    │
│           │  → Host App   │                    │
│           └──────────────┘                    │
└───────────────────────────────────────────────┘
```