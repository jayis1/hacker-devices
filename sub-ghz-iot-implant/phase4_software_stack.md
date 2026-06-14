# Sub-GHz IoT Gateway Implant — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On / Reset
  │
  ├── CC1352P Boot ROM (TI-built, 0x0000_0000)
  │     ├── Check BOOTCFG register for boot source
  │     ├── If BOOTCFG = FLASH: Jump to internal flash
  │     └── If BOOTCFG = DEBUG: Enter SWD debug mode
  │
  ├── Stage 1: Internal Flash Bootloader (0x0000_0000 – 0x0000_1FFF)
  │     ├── Initialize watchdog (disable for now)
  │     ├── Configure XOSC_HF (24 MHz crystal)
  │     ├── Switch system clock to 48 MHz (PLL)
  │     ├── Initialize SRAM (128 KB)
  │     ├── Verify firmware integrity (CRC32 + ATECC608A ECDSA)
  │     │     ├── Load firmware CRC from flash header
  │     │     ├── Compute CRC over firmware region
  │     │     ├── If CRC fails: halt, blink LED RED
  │     │     └── If CRC passes: continue
  │     ├── ATECC608A secure boot verification (optional)
  │     │     ├── Send firmware hash to ATECC608A
  │     │     ├── ATECC608A returns ECDSA signature
  │     │     ├── Verify against stored public key
  │     │     └── If signature invalid: halt, blink LED RED
  │     └── Jump to Stage 2
  │
  ├── Stage 2: Application Firmware (0x0000_2000 – 0x000F_FFFF)
  │     ├── Initialize hardware clocks and GPIO
  │     ├── Initialize power domains
  │     ├── Initialize SDRAM controller (xSPI)
  │     ├── Initialize SPI Flash
  │     ├── Initialize ATECC608A (I²C)
  │     ├── Initialize USB CDC-ECM
  │     ├── Initialize BLE stack
  │     ├── Initialize Sub-GHz radio
  │     ├── Load configuration from SPI Flash
  │     ├── Start FreeRTOS scheduler
  │     └── Enter main application loop
  │
  └── Application Tasks (FreeRTOS)
        ├── Radio RX Task (highest priority)
        ├── Radio TX Task
        ├── Packet Processing Task
        ├── BLE Communication Task
        ├── USB Communication Task
        ├── SDRAM Ring Buffer Manager
        ├── MicroSD Logger Task
        ├── LED Status Task (lowest priority)
        └── Watchdog Feed Task
```

### 1.2 Boot Configuration Register (BOOTCFG)

Address: `0x5000_1000` (AON_PMCTL:BOOTCFG)

| Bit | Field | Reset | Description |
|-----|-------|-------|-------------|
| [31:24] | BOOT_SRC | 0x00 | 0x00 = Internal Flash, 0x01 = External SPI, 0x02 = DEBUG |
| [23:16] | BOOT_VER | 0x01 | Bootloader version |
| [15:8] | APP_VER | — | Application version (set by firmware) |
| [7:0] | FLAGS | 0x00 | Bit 0: Secure boot enable, Bit 1: USB boot enable |

### 1.3 Flash Memory Map

| Region | Start Address | Size | Content |
|--------|--------------|------|---------|
| Bootloader | 0x0000_0000 | 8 KB | Stage 1 bootloader |
| App Firmware | 0x0000_2000 | 960 KB | Application code + const data |
| CCFG | 0x000F_F000 | 256 B | Customer Configuration (boot pins, security) |
| ATECC Slot Config | 0x000F_FC00 | 256 B | ATECC608A provisioning data |
| NV Snapshot | 0x000F_FE00 | 512 B | FreeRTOS NV state (for OTA) |

## 2. MMIO Registers (CC1352P)

### 2.1 Clock Control Registers

| Register | Address | Width | Description |
|----------|---------|-------|-------------|
| SCLK_HF_MCLK | 0x400C_0000 | 32 | System HF clock control |
| SCLK_MF_CTL | 0x400C_0004 | 32 | MF clock control |
| SCLK_LF_CTL | 0x400C_0008 | 32 | LF clock control |
| DPLL_MCLK_CFG0 | 0x400C_0010 | 32 | Main PLL config (MCLK) |
| DPLL_MCLK_CFG1 | 0x400C_0014 | 32 | Main PLL config 1 |
| DPLL_MCLK_STATUS | 0x400C_0018 | 32 | Main PLL status |
| XOSC_HF_CTRL | 0x400C_0020 | 32 | 24 MHz crystal control |
| XOSC_LF_CTRL | 0x400C_0024 | 32 | 32.768 kHz crystal control |

**Clock initialization sequence:**
```c
// Step 1: Enable 24 MHz crystal
XOSC_HF_CTRL = 0x00000001;  // XOSC_HF_ENABLE = 1
while (!(XOSC_HF_CTRL & 0x00000002)); // Wait for XOSC_HF_RDY

// Step 2: Configure PLL for 48 MHz
DPLL_MCLK_CFG0 = 0x00000C00; // MCLK_DIV = 12, XOSC_HF / 12 = 2 MHz ref
DPLL_MCLK_CFG1 = 0x00000018; // MCLK_MULT = 24, 2 MHz * 24 = 48 MHz

// Step 3: Switch system clock to PLL
SCLK_HF_MCLK = 0x00000002; // MCLK_SRC = DPLL_MCLK
```

### 2.2 GPIO Control Registers

| Register | Address | Description |
|----------|---------|-------------|
| GPIO_DOE0 | 0x4002_2000 | Data Output Enable (DIO_0 – DIO_31) |
| GPIO_DOE1 | 0x4002_2004 | Data Output Enable (DIO_32 – DIO_47) |
| GPIO_DOUT0 | 0x4002_2008 | Data Out (DIO_0 – DIO_31) |
| GPIO_DOUT1 | 0x4002_200C | Data Out (DIO_32 – DIO_47) |
| GPIO_DIN0 | 0x4002_2010 | Data In (DIO_0 – DIO_31) |
| GPIO_DIN1 | 0x4002_2014 | Data In (DIO_32 – DIO_47) |
| GPIO_EV0 | 0x4002_2018 | Event flag register 0 |
| GPIO_EV1 | 0x4002_201C | Event flag register 1 |
| GPIO_CTRL0 | 0x4002_2080 | IO Control DIO_0 |
| ... | ... | ... |
| GPIO_CTRL47 | 0x4002_20BC | IO Control DIO_47 |

**GPIO Pin Mode Configuration (GPIO_CTRL[n]):**

| Bit | Field | Values |
|-----|-------|--------|
| [2:0] | MODE | 0=Input, 1=Output, 2=Peripheral |
| [3] | PULL | 0=No pull, 1=Pull-up |
| [4] | PULL_DOWN | 0=No pull, 1=Pull-down |
| [5] | SLEW | 0=Slow, 1=Fast |
| [6] | HYST | 0=No hysteresis, 1=Hysteresis |
| [7] | IO_MODE | 0=Push-pull, 1=Open-drain |

### 2.3 SSI (SPI) Registers

| Register | Address | Description |
|----------|---------|-------------|
| SSI0_BASE | 0x4000_8000 | SDRAM SPI (SSI0) |
| SSI1_BASE | 0x4000_A000 | Flash SPI (SSI1) |
| SSI2_BASE | 0x4000_C000 | I²C alt (SSI2) |
| SSI3_BASE | 0x4000_E000 | SD Card SPI (SSI3) |

**SSI Register Block (each SSI):**

| Offset | Name | Description |
|--------|------|-------------|
| 0x000 | SSI_CR0 | Control register 0 |
| 0x004 | SSI_CR1 | Control register 1 |
| 0x008 | SSI_DR | Data register |
| 0x00C | SSI_SR | Status register |
| 0x010 | SSI_CPSR | Clock prescale |
| 0x014 | SSI_IM | Interrupt mask |
| 0x018 | SSI_RIS | Raw interrupt status |
| 0x01C | SSI_MIS | Masked interrupt status |
| 0x020 | SSI_ICR | Interrupt clear |
| 0x024 | SSI_DMACR | DMA control |

### 2.4 RF Core Registers (Sub-GHz Radio)

| Register | Address | Description |
|----------|---------|-------------|
| RFC_DBELL_BASE | 0x4003_7000 | RF Core Doorbell |
| RFC_DBELL_CMDR | 0x4003_7004 | Command request |
| RFC_DBELL_CMDSTA | 0x4003_7008 | Command status |
| RFC_DBELL_CMDARG | 0x4003_700C | Command argument pointer |
| RFC_DBELL_RFCPEIFG | 0x4003_7014 | RF PE interrupt flags |
| RFC_PWR_BASE | 0x4003_7100 | RF Power control |
| RFC_PWR_PWMCLKG | 0x4003_7104 | RF Power clock gate |
| RFC_FSCA_BASE | 0x4003_7200 | Frequency Synthesizer Calibration |

**Radio Command Interface:**

The CC1352P radio is controlled via command-based API (TI proprietary). Commands are sent as opaque structures via the RF doorbell:

```c
typedef struct {
    uint16_t commandId;      // Command ID
    uint16_t status;         // Status (filled by radio core)
    uint8_t  *pNextOp;       // Pointer to next command
    uint32_t startTime;      // Start time (in RAT ticks)
    uint32_t startTrigger;   // Start trigger mode
} rfc_command_t;

// Key command IDs
#define RFC_CMD_RADIO_SETUP     0x0601  // Configure radio for Sub-GHz
#define RFC_CMD_RX              0x0801  // Start RX
#define RFC_CMD_TX              0x0802  // Start TX
#define RFC_CMD_IEEE_RX         0x2801  // IEEE 802.15.4 RX
#define RFC_CMD_IEEE_TX         0x2802  // IEEE 802.15.4 TX
#define RFC_CMD_ADV_RX          0x1801  // Advanced RX (raw capture)
#define RFC_CMD_PROP_RX         0x3801  // Proprietary RX
#define RFC_CMD_PROP_TX         0x3802  // Proprietary TX
```

### 2.5 USB Registers

| Register | Address | Description |
|----------|---------|-------------|
| USB_CTRL | 0x4000_0000 | USB Control |
| USB_FADDR | 0x4000_0004 | Function address |
| USB_DPS | 0x4000_0008 | Device Power Status |
| USB_EPIDX | 0x4000_000C | Endpoint index |
| USB_CTRL_EP | 0x4000_0100 | Control endpoint 0 |
| USB_EP1 | 0x4000_0120 | Endpoint 1 (CDC data IN) |
| USB_EP2 | 0x4000_0140 | Endpoint 2 (CDC data OUT) |
| USB_EP3 | 0x4000_0160 | Endpoint 3 (CDC interrupt IN) |
| USB_EP4 | 0x4000_0180 | Endpoint 4 (ECM data IN) |
| USB_EP5 | 0x4000_01A0 | Endpoint 5 (ECM data OUT) |

## 3. Clock & GPIO Initialization

### 3.1 Clock Initialization (board.c)

```c
// board.c — Hardware initialization for Sub-GHz IoT Gateway Implant

#include "board.h"
#include "registers.h"

void board_init_clocks(void) {
    // Step 1: Enable XOSC_HF (24 MHz crystal)
    XOSC_HF_CTRL = XOSC_HF_ENABLE | XOSC_HF_XOSC_HF_FAST_START;
    while (!(XOSC_HF_CTRL & XOSC_HF_RDY))
        ;  // Wait for crystal stable

    // Step 2: Configure DPLL for 48 MHz system clock
    // DPLL = (XOSC_HF / ref_div) * mult = (24MHz / 1) * 2 = 48 MHz
    DPLL_MCLK_CFG0 = DPLL_MCLK_REF_DIV(1) | DPLL_MCLK_MULT(2);
    DPLL_MCLK_CFG1 = DPLL_MCLK_ENABLE;
    while (!(DPLL_MCLK_STATUS & DPLL_MCLK_LOCKED))
        ;  // Wait for PLL lock

    // Step 3: Switch system clock to DPLL
    SCLK_HF_MCLK = SCLK_HF_SRC_DPLL;

    // Step 4: Configure SCLK_MF (2 MHz from XOSC_HF/12)
    SCLK_MF_CTL = SCLK_MF_SRC_XOSC_HF | SCLK_MF_DIV(12);

    // Step 5: Configure SCLK_LF (32.768 kHz from XOSC_LF)
    XOSC_LF_CTRL = XOSC_LF_ENABLE;
    while (!(XOSC_LF_CTRL & XOSC_LF_RDY))
        ;
    SCLK_LF_CTL = SCLK_LF_SRC_XOSC_LF;

    // Step 6: Enable peripheral clock gates
    RFC_PWR_PWMCLKG = RFC_PWR_CLKEN;
    GPIO_DOE0 = 0;  // Start with all GPIOs as input
    GPIO_DOE1 = 0;
}

void board_init_gpio(void) {
    // SDRAM interface (SSI0) — DIO_0 through DIO_7 + DIO_21-23 + DIO_30-35
    for (int i = 0; i < 8; i++) {
        GPIO_CTRL(i) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;
    }
    GPIO_CTRL(21) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;  // SDRAM_CLK
    GPIO_CTRL(22) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;  // SDRAM_CS
    GPIO_CTRL(23) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;  // SDRAM_DQS

    // SDRAM data high byte (U2) — DIO_30 through DIO_35
    for (int i = 30; i < 36; i++) {
        GPIO_CTRL(i) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;
    }

    // SPI Flash (SSI1) — DIO_8 through DIO_11
    for (int i = 8; i < 12; i++) {
        GPIO_CTRL(i) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;
    }
    GPIO_CTRL(12) = GPIO_MODE_OUTPUT;  // FLASH_CS (GPIO)

    // MicroSD (SSI3) — DIO_12 through DIO_15
    GPIO_CTRL(12) = GPIO_MODE_OUTPUT;  // SD_CS (GPIO)
    for (int i = 13; i < 16; i++) {
        GPIO_CTRL(i) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;
    }

    // I²C (ATECC608A) — DIO_19, DIO_20
    GPIO_CTRL(19) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST | GPIO_PULL_UP | GPIO_OPEN_DRAIN;
    GPIO_CTRL(20) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST | GPIO_PULL_UP | GPIO_OPEN_DRAIN;

    // UART0 (Debug) — DIO_24, DIO_25
    GPIO_CTRL(24) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;  // TX
    GPIO_CTRL(25) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;  // RX

    // UART1 (Expansion) — DIO_26, DIO_27
    GPIO_CTRL(26) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;  // TX
    GPIO_CTRL(27) = GPIO_MODE_PERIPHERAL | GPIO_SLEW_FAST;  // RX

    // USB — DIO_28, DIO_29 (configured by USB driver)

    // WS2812B LED — DIO_16
    GPIO_CTRL(16) = GPIO_MODE_OUTPUT;

    // Buttons — DIO_17, DIO_18 (inputs with pull-ups)
    GPIO_CTRL(17) = GPIO_MODE_INPUT | GPIO_PULL_UP;
    GPIO_CTRL(18) = GPIO_MODE_INPUT | GPIO_PULL_UP;

    // Set initial output states
    GPIO_DOUT0 = 0;
    GPIO_DOUT1 = 0;
    // Enable outputs for GPIO-mode pins
    GPIO_DOE0 |= (1 << 12) | (1 << 16);  // FLASH_CS, WS2812B
    GPIO_DOE1 |= (1 << (30 - 32));  // SDRAM upper byte enable
}
```

## 4. Device Drivers

### 4.1 Sub-GHz Radio Driver (drivers/radio_subghz.c)

```c
// drivers/radio_subghz.h
#ifndef RADIO_SUBGHZ_H
#define RADIO_SUBGHZ_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RADIO_MODE_IDLE = 0,
    RADIO_MODE_RX_SNIFF,
    RADIO_MODE_TX,
    RADIO_MODE_RX_ZIGBEE,
    RADIO_MODE_TX_ZIGBEE,
    RADIO_MODE_RX_ZWAVE,
    RADIO_MODE_TX_ZWAVE,
    RADIO_MODE_RX_SUBGHZ_OOK,
    RADIO_MODE_TX_SUBGHZ_OOK,
    RADIO_MODE_RX_SUBGHZ_FSK,
    RADIO_MODE_TX_SUBGHZ_FSK,
} radio_mode_t;

typedef enum {
    FREQ_315_MHZ = 315000000,
    FREQ_433_MHZ = 433920000,
    FREQ_868_MHZ = 868000000,
    FREQ_868_4_MHZ = 868400000,  // Z-Wave EU
    FREQ_908_4_MHZ = 908400000,  // Z-Wave US
    FREQ_915_MHZ = 915000000,
} radio_freq_t;

typedef struct {
    uint32_t frequency;       // Hz
    int8_t    tx_power;       // dBm (-20 to +20)
    uint32_t  baud_rate;      // bps
    uint8_t   modulation;    // 0=OOK, 1=FSK, 2=GFSK, 3=O-QPSK
    uint32_t  deviation;      // Hz (for FSK/GFSK)
    uint32_t  channel_bw;     // Hz
    uint8_t   sync_word[4];  // Sync word (up to 32 bits)
    uint8_t   sync_len;       // Sync word length in bits
} radio_config_t;

typedef struct __attribute__((packed)) {
    uint32_t timestamp;       // RAT timestamp (4 µs resolution)
    int8_t    rssi;           // dBm
    uint8_t   channel;        // Channel number
    uint8_t   length;         // Payload length
    uint8_t   payload[255];  // Payload data
    uint8_t   lqi;            // Link quality indicator
    uint8_t   crc_valid;      // CRC check result
} radio_packet_t;

typedef void (*radio_rx_cb_t)(const radio_packet_t *pkt);

int  radio_subghz_init(void);
int  radio_subghz_configure(const radio_config_t *cfg);
int  radio_subghz_set_mode(radio_mode_t mode);
int  radio_subghz_set_frequency(radio_freq_t freq);
int  radio_subghz_set_tx_power(int8_t dbm);
int  radio_subghz_start_rx(radio_rx_cb_t callback);
int  radio_subghz_stop_rx(void);
int  radio_subghz_tx_packet(const uint8_t *data, uint8_t len);
int  radio_subghz_tx_raw(const uint8_t *data, uint16_t len, uint32_t rate);
int  radio_subghz_set_channel(uint8_t channel);
void radio_subghz_irq_handler(void);

// Zigbee-specific API
int  radio_zigbee_start_sniff(uint16_t pan_id, uint8_t channel);
int  radio_zigbee_inject(uint16_t dst_pan, uint16_t dst_addr,
                         const uint8_t *payload, uint8_t len);
int  radio_zigbee_mitm_start(uint16_t target_pan, uint8_t channel);
int  radio_zigbee_mitm_stop(void);

// Z-Wave-specific API
int  radio_zwave_start_sniff(uint8_t channel);
int  radio_zwave_tx(uint8_t home_id, uint8_t node_id,
                    const uint8_t *payload, uint8_t len);

// Rolling-code analyzer
int  radio_rolling_capture_start(radio_freq_t freq, uint8_t modulation);
int  radio_rolling_analyze(uint16_t *codes, uint8_t count,
                           uint32_t *predicted_next);

#endif // RADIO_SUBGHZ_H
```

```c
// drivers/radio_subghz.c
#include "radio_subghz.h"
#include "board.h"
#include "registers.h"

static radio_config_t current_cfg;
static radio_mode_t current_mode = RADIO_MODE_IDLE;
static radio_rx_cb_t rx_callback = NULL;
static volatile uint32_t rat_offset = 0;  // Radio timer offset

// Internal packet buffer (ring buffer in SDRAM)
#define PKT_RING_SIZE  4096
static radio_packet_t *pkt_ring = (radio_packet_t *)SDRAM_BASE;

int radio_subghz_init(void) {
    // Power on RF core
    RFC_PWR_PWMCLKG = RFC_PWR_CLKEN;
    
    // Wait for RF core to be ready
    for (volatile int i = 0; i < 1000; i++)
        ;
    
    // Set up RF core doorbell for command interface
    RFC_DBELL_CMDR = 0;
    
    // Initialize RAT (Radio Timer)
    rat_offset = 0;
    
    current_mode = RADIO_MODE_IDLE;
    return 0;
}

int radio_subghz_configure(const radio_config_t *cfg) {
    // Validate configuration
    if (cfg->frequency < 281000000 || cfg->frequency > 1660000000) {
        return -1;  // Out of CC1352P range
    }
    if (cfg->tx_power > 20) {
        return -2;  // Max +20 dBm
    }
    
    current_cfg = *cfg;
    
    // Build and send RADIO_SETUP command
    rfc_CMD_RADIO_SETUP_t setup_cmd = {
        .commandNo = 0x0601,
        .status = 0,
        .pNextOp = NULL,
        .startTime = 0,
        .startTrigger = {
            .triggerType = 0,  // Immediate
        },
        .mode = 0,  // Sub-GHz mode
        .config = {
            .frontEndMode = 0x00,  // Differential
            .biasMode = 0x00,      // Internal bias
        },
        .loDivider = (cfg->frequency < 500000000) ? 6 :
                     (cfg->frequency < 900000000) ? 2 : 2,
    };
    
    // Send command via doorbell
    RFC_DBELL_CMDARG = (uint32_t)&setup_cmd;
    RFC_DBELL_CMDR = RFC_DBELL_CMD_TRIGGER;
    
    // Wait for command completion
    while (!(RFC_DBELL_RFCPEIFG & RFC_DBELL_CMD_DONE))
        ;
    RFC_DBELL_RFCPEIFG = RFC_DBELL_CMD_DONE;
    
    // Set TX power
    radio_subghz_set_tx_power(cfg->tx_power);
    
    // Set frequency
    radio_subghz_set_frequency(cfg->frequency);
    
    return 0;
}

int radio_subghz_set_frequency(radio_freq_t freq) {
    // Calculate synthesizer frequency word
    // FS_FREQ = (freq / 4000000) * 2^22 for CC1352P
    uint32_t freq_word = (uint32_t)((float)freq / 4000000.0f * (1 << 22));
    
    // Write to FS_FREQ register
    *(volatile uint32_t *)0x5000_2000 = freq_word;
    
    // Wait for synthesizer to lock
    while (!(*(volatile uint32_t *)0x5000_2010 & 0x01))
        ;
    
    current_cfg.frequency = freq;
    return 0;
}

int radio_subghz_set_tx_power(int8_t dbm) {
    if (dbm > 20) dbm = 20;
    if (dbm < -20) dbm = -20;
    
    // CC1352P PA power table (simplified)
    // Maps dBm to PA register value
    uint8_t pa_val;
    if (dbm <= 0) {
        pa_val = (uint8_t)((dbm + 20) * 127 / 40);
    } else {
        // +20 dBm requires high-power PA mode
        pa_val = 0x7F;
    }
    
    // Write to TX_POWER register
    *(volatile uint32_t *)0x5000_3000 = pa_val;
    current_cfg.tx_power = dbm;
    return 0;
}

int radio_subghz_start_rx(radio_rx_cb_t callback) {
    rx_callback = callback;
    
    // Build RX command based on current modulation
    rfc_CMD_PROP_RX_t rx_cmd = {
        .commandNo = 0x3801,  // PROP_RX
        .status = 0,
        .pNextOp = NULL,
        .startTime = 0,
        .startTrigger = {.triggerType = 0},
        .rule = 0x01,  // Stop on overrun
        .maxPktLen = 255,
        .pktLen = 0,    // Variable length
        .syncWord = *(uint32_t *)current_cfg.sync_word,
        .syncWordLen = current_cfg.sync_len,
    };
    
    // Enable RF core interrupts
    RFC_DBELL_RFCPEIFG = 0;
    NVIC_EnableIRQ(RFC_IRQ);
    
    // Start RX
    RFC_DBELL_CMDARG = (uint32_t)&rx_cmd;
    RFC_DBELL_CMDR = RFC_DBELL_CMD_TRIGGER;
    
    current_mode = RADIO_MODE_RX_SNIFF;
    return 0;
}

int radio_subghz_stop_rx(void) {
    // Send CMD_ABORT
    rfc_CMD_ABORT_t abort_cmd = {
        .commandNo = 0x0401,
    };
    RFC_DBELL_CMDARG = (uint32_t)&abort_cmd;
    RFC_DBELL_CMDR = RFC_DBELL_CMD_TRIGGER;
    
    current_mode = RADIO_MODE_IDLE;
    rx_callback = NULL;
    return 0;
}

int radio_subghz_tx_packet(const uint8_t *data, uint8_t len) {
    if (len > 255) return -1;
    
    // Copy data to TX buffer
    volatile uint8_t *tx_buf = (volatile uint8_t *)0x2000_8000;
    for (int i = 0; i < len; i++) {
        tx_buf[i] = data[i];
    }
    
    // Build TX command
    rfc_CMD_PROP_TX_t tx_cmd = {
        .commandNo = 0x3802,
        .status = 0,
        .pNextOp = NULL,
        .startTime = 0,
        .startTrigger = {.triggerType = 0},
        .pktLen = len,
        .pPkt = (uint8_t *)0x2000_8000,
    };
    
    RFC_DBELL_CMDARG = (uint32_t)&tx_cmd;
    RFC_DBELL_CMDR = RFC_DBELL_CMD_TRIGGER;
    
    while (!(RFC_DBELL_RFCPEIFG & RFC_DBELL_CMD_DONE))
        ;
    
    return 0;
}

int radio_subghz_tx_raw(const uint8_t *data, uint16_t len, uint32_t rate) {
    // Raw TX mode for OOK/ASK replay
    // Uses proprietary mode with Manchester or raw encoding
    
    volatile uint8_t *tx_buf = (volatile uint8_t *)0x2000_8000;
    for (int i = 0; i < len && i < 2048; i++) {
        tx_buf[i] = data[i];
    }
    
    rfc_CMD_PROP_TX_t tx_cmd = {
        .commandNo = 0x3802,
        .pktLen = len,
        .pPkt = (uint8_t *)0x2000_8000,
    };
    
    RFC_DBELL_CMDARG = (uint32_t)&tx_cmd;
    RFC_DBELL_CMDR = RFC_DBELL_CMD_TRIGGER;
    
    while (!(RFC_DBELL_RFCPEIFG & RFC_DBELL_CMD_DONE))
        ;
    
    return 0;
}

void radio_subghz_irq_handler(void) {
    uint32_t flags = RFC_DBELL_RFCPEIFG;
    
    if (flags & RFC_DBELL_RX_FIFO_DATA) {
        // Read packet from RX FIFO
        radio_packet_t pkt;
        volatile uint8_t *rx_fifo = (volatile uint8_t *)0x2100_0000;
        
        pkt.timestamp = *(volatile uint32_t *)(&rx_fifo[0]);
        pkt.rssi = *(volatile int8_t *)(&rx_fifo[4]);
        pkt.channel = rx_fifo[5];
        pkt.length = rx_fifo[6];
        for (int i = 0; i < pkt.length && i < 255; i++) {
            pkt.payload[i] = rx_fifo[7 + i];
        }
        pkt.lqi = rx_fifo[7 + pkt.length];
        pkt.crc_valid = rx_fifo[8 + pkt.length];
        
        if (rx_callback) {
            rx_callback(&pkt);
        }
    }
    
    // Clear all flags
    RFC_DBELL_RFCPEIFG = 0;
}

// Zigbee-specific functions
int radio_zigbee_start_sniff(uint16_t pan_id, uint8_t channel) {
    radio_config_t cfg = {
        .frequency = 2405000000 + (channel - 11) * 5000000,
        .modulation = 3,  // O-QPSK
        .baud_rate = 250000,
        .channel_bw = 2000000,
    };
    
    if (radio_subghz_configure(&cfg) != 0) return -1;
    return radio_subghz_start_rx(NULL);
}

int radio_zigbee_inject(uint16_t dst_pan, uint16_t dst_addr,
                         const uint8_t *payload, uint8_t len) {
    uint8_t frame[128];
    frame[0] = 0x41;  // Data frame, intra-PAN
    frame[1] = 0x88;  // Sequence number suppression
    frame[2] = 0x01;  // Sequence number
    frame[3] = dst_pan & 0xFF;
    frame[4] = (dst_pan >> 8) & 0xFF;
    frame[5] = dst_addr & 0xFF;
    frame[6] = (dst_addr >> 8) & 0xFF;
    for (int i = 0; i < len && i < 120; i++) {
        frame[7 + i] = payload[i];
    }
    return radio_subghz_tx_packet(frame, 7 + len);
}

int radio_zigbee_mitm_start(uint16_t target_pan, uint8_t channel) {
    // Configure to impersonate coordinator on target PAN
    // This implements a Zigbee network interposition attack
    (void)target_pan;
    (void)channel;
    return 0;  // Placeholder — full MITM requires Zigbee stack
}

int radio_zigbee_mitm_stop(void) {
    return radio_subghz_stop_rx();
}

// Z-Wave-specific functions
int radio_zwave_start_sniff(uint8_t channel) {
    radio_config_t cfg = {
        .frequency = (channel == 0) ? 868400000 : 908400000,
        .modulation = 2,  // GFSK
        .baud_rate = 100000,
        .deviation = 20000,
        .channel_bw = 300000,
    };
    if (radio_subghz_configure(&cfg) != 0) return -1;
    return radio_subghz_start_rx(NULL);
}

int radio_zwave_tx(uint8_t home_id, uint8_t node_id,
                    const uint8_t *payload, uint8_t len) {
    (void)home_id;
    (void)node_id;
    (void)payload;
    (void)len;
    return 0;  // Placeholder
}

// Rolling-code analyzer
int radio_rolling_capture_start(radio_freq_t freq, uint8_t modulation) {
    radio_config_t cfg = {
        .frequency = freq,
        .modulation = modulation,
        .baud_rate = 4000,  // Typical Sub-GHz OOK
        .channel_bw = 1000000,
    };
    if (radio_subghz_configure(&cfg) != 0) return -1;
    return radio_subghz_start_rx(NULL);
}

int radio_rolling_analyze(uint16_t *codes, uint8_t count,
                           uint32_t *predicted_next) {
    // Simple KEELOQ-like rolling code prediction
    // Analyzes captured codes for patterns
    if (count < 2) return -1;
    
    // Check for simple incrementing pattern
    int32_t delta = codes[1] - codes[0];
    bool constant_delta = true;
    for (int i = 2; i < count; i++) {
        if ((int32_t)(codes[i] - codes[i-1]) != delta) {
            constant_delta = false;
            break;
        }
    }
    
    if (constant_delta) {
        *predicted_next = codes[count - 1] + delta;
        return 0;
    }
    
    return -1;  // Cannot predict (cryptographic rolling code)
}
```

### 4.2 SDRAM Driver with DMA (drivers/sdram.c)

```c
// drivers/sdram.h
#ifndef SDRAM_H
#define SDRAM_H

#include <stdint.h>

#define SDRAM_BASE       0x6000_0000
#define SDRAM_SIZE       (64 * 1024 * 1024)  // 64 MB

// Ring buffer for packet captures
typedef struct {
    volatile uint32_t write_idx;   // Next write position
    volatile uint32_t read_idx;    // Next read position
    uint32_t capacity;             // Total capacity in bytes
    uint8_t *buffer;              // Pointer to SDRAM base
} sdram_ring_t;

int  sdram_init(void);
int  sdram_ring_init(sdram_ring_t *ring, uint32_t capacity);
int  sdram_ring_write(sdram_ring_t *ring, const uint8_t *data, uint32_t len);
int  sdram_ring_read(sdram_ring_t *ring, uint8_t *data, uint32_t len);
void sdram_dma_start(uint32_t src, uint32_t dst, uint32_t len);
bool sdram_dma_done(void);
void sdram_test(void);

#endif // SDRAM_H
```

```c
// drivers/sdram.c
#include "sdram.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static sdram_ring_t capture_ring;

int sdram_init(void) {
    // Configure SSI0 for xSPI mode (HyperBus/xSPI SRAM)
    // Step 1: Enable SSI0 peripheral clock
    SSI0_SSI_CR1 = 0;  // Disable SSI
    SSI0_SSI_CR0 = SSI_CR0_SPH | SSI_CR0_SPO  // Mode 3
                  | SSI_CR0_FRF_SPI
                  | SSI_CR0_DSS_8BIT;
    SSI0_SSI_CPSR = 2;  // Clock prescale = 2
    
    // Step 2: Configure for high-speed xSPI mode
    // CC1352P uses XSPI for external memory interface
    // Enable XSPI mode in SSI control register
    *(volatile uint32_t *)0x400080FC = 0x00000001;  // XSPI enable
    
    // Step 3: Configure timing for IS66WVS256 (104 MHz max)
    // At 48 MHz MCU clock, SSI0 clock = 48/2 = 24 MHz (safe)
    SSI0_SSI_CPSR = 2;
    
    // Step 4: Enable SSI0
    SSI0_SSI_CR1 = SSI_CR1_SSE;  // SSI enable
    
    // Step 5: Initialize both SDRAM chips
    // Send reset command to both chips
    uint8_t reset_cmd = 0xFF;  // Reset command for IS66WVS256
    SSI0_SSI_DR = reset_cmd;
    while (!(SSI0_SSI_SR & SSI_SR_TFE))
        ;
    
    // Wait for SDRAM ready
    for (volatile int i = 0; i < 10000; i++)
        ;
    
    return 0;
}

int sdram_ring_init(sdram_ring_t *ring, uint32_t capacity) {
    if (capacity > SDRAM_SIZE) return -1;
    
    ring->write_idx = 0;
    ring->read_idx = 0;
    ring->capacity = capacity;
    ring->buffer = (uint8_t *)SDRAM_BASE;
    
    return 0;
}

int sdram_ring_write(sdram_ring_t *ring, const uint8_t *data, uint32_t len) {
    // Check available space
    uint32_t used = (ring->write_idx - ring->read_idx) % ring->capacity;
    uint32_t free_space = ring->capacity - used;
    
    if (len > free_space) {
        // Overflow: advance read pointer (drop oldest data)
        uint32_t overflow = len - free_space;
        ring->read_idx = (ring->read_idx + overflow) % ring->capacity;
    }
    
    // Write data in one or two chunks (handle wrap-around)
    uint32_t first_chunk = (len <= ring->capacity - ring->write_idx) 
                           ? len 
                           : ring->capacity - ring->write_idx;
    uint32_t second_chunk = len - first_chunk;
    
    // Use DMA for first chunk
    if (first_chunk > 0) {
        memcpy(&ring->buffer[ring->write_idx], data, first_chunk);
    }
    
    // Handle wrap-around
    if (second_chunk > 0) {
        memcpy(&ring->buffer[0], data + first_chunk, second_chunk);
    }
    
    ring->write_idx = (ring->write_idx + len) % ring->capacity;
    
    return len;
}

int sdram_ring_read(sdram_ring_t *ring, uint8_t *data, uint32_t len) {
    uint32_t available = (ring->write_idx - ring->read_idx) % ring->capacity;
    
    if (len > available) {
        len = available;  // Read only what's available
    }
    
    if (len == 0) return 0;
    
    // Read data in one or two chunks
    uint32_t first_chunk = (len <= ring->capacity - ring->read_idx)
                           ? len
                           : ring->capacity - ring->read_idx;
    uint32_t second_chunk = len - first_chunk;
    
    if (first_chunk > 0) {
        memcpy(data, &ring->buffer[ring->read_idx], first_chunk);
    }
    
    if (second_chunk > 0) {
        memcpy(data + first_chunk, &ring->buffer[0], second_chunk);
    }
    
    ring->read_idx = (ring->read_idx + len) % ring->capacity;
    
    return len;
}

void sdram_dma_start(uint32_t src, uint32_t dst, uint32_t len) {
    // Configure DMA channel for SDRAM transfer
    // Using CC1352P built-in DMA controller (UDMA)
    
    // Channel assignment: Channel 8 = SSI0 RX (SDRAM read)
    //                     Channel 9 = SSI0 TX (SDRAM write)
    
    volatile uint32_t *udma_ctrl = (volatile uint32_t *)0x4000_F000;
    
    // Set source address
    udma_ctrl[0x10] = src;  // Channel 8 source
    udma_ctrl[0x14] = dst;  // Channel 8 destination
    udma_ctrl[0x18] = (len / 4 - 1) << 4;  // Transfer size
    
    // Enable channel
    udma_ctrl[0x00] |= (1 << 8);  // Enable channel 8
}

bool sdram_dma_done(void) {
    volatile uint32_t *udma_ctrl = (volatile uint32_t *)0x4000_F000;
    return !(udma_ctrl[0x00] & (1 << 8));  // Channel 8 not active
}

void sdram_test(void) {
    // Write test pattern to SDRAM and read back
    volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE;
    
    // Write pattern
    for (int i = 0; i < 256; i++) {
        sdram[i] = 0xDEADBEEF ^ i;
    }
    
    // Read back and verify
    for (int i = 0; i < 256; i++) {
        uint32_t expected = 0xDEADBEEF ^ i;
        if (sdram[i] != expected) {
            // SDRAM test failed — halt with error
            while (1)
                ;
        }
    }
}
```

### 4.3 BLE Communication Driver (drivers/ble_comms.c)

```c
// drivers/ble_comms.h
#ifndef BLE_COMMS_H
#define BLE_COMMS_H

#include <stdint.h>
#include <stdbool.h>

// BLE GATT service UUIDs (custom)
#define BLE_SVC_SUBSTATION_UUID   0x5355  // "SU" = Substation
#define BLE_CHAR_MODE_UUID        0x5356  // Mode control
#define BLE_CHAR_CAPTURE_UUID     0x5357  // Packet capture data
#define BLE_CHAR_CONFIG_UUID      0x5358  // Configuration
#define BLE_CHAR_STATUS_UUID      0x5359  // Device status

typedef enum {
    BLE_STATE_IDLE = 0,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTED,
    BLE_STATE_STREAMING,
} ble_state_t;

typedef struct {
    uint8_t  mode;           // Current radio mode
    uint32_t freq;           // Current frequency
    int8_t   rssi;           // Last RSSI
    uint32_t pkt_count;      // Packets captured
    uint32_t uptime_s;       // Uptime in seconds
    uint8_t  battery_pct;    // Battery percentage
} ble_status_t;

int  ble_comms_init(void);
int  ble_comms_start_advertising(const char *name);
int  ble_comms_stop_advertising(void);
int  ble_comms_send_packet(const uint8_t *data, uint16_t len);
int  ble_comms_send_status(const ble_status_t *status);
int  ble_comms_send_capture(const uint8_t *pcap_data, uint32_t len);
void ble_comms_set_mode(uint8_t mode);
void ble_comms_set_config(const uint8_t *config, uint16_t len);
ble_state_t ble_comms_get_state(void);
void ble_comms_task(void);  // Called from main loop

#endif // BLE_COMMS_H
```

```c
// drivers/ble_comms.c
#include "ble_comms.h"
#include "board.h"
#include "registers.h"

static ble_state_t ble_state = BLE_STATE_IDLE;
static uint16_t ble_conn_handle = 0;
static ble_status_t last_status;

// BLE advertising data (max 31 bytes)
static uint8_t adv_data[] = {
    0x02, 0x01, 0x06,              // Flags: LE General Discoverable, BR/EDR Not Supported
    0x07, 0x09, 'S', 'U', 'B', 'S', 'T', 'N',  // Name: "SUBSTN"
    0x03, 0x03, 0x55, 0x53,        // Service UUID: 0x5355
};

// GATT service definition (simplified)
// In production, this would use the TI BLE stack's GATT framework

int ble_comms_init(void) {
    // Initialize BLE radio (2.4 GHz) on CC1352P
    // The CC1352P shares the 2.4 GHz radio between BLE and Sub-GHz
    // BLE uses the BLE Core inside the CC1352P
    
    // Power on BLE core
    *(volatile uint32_t *)RFC_PWR_PWMCLKG |= RFC_PWR_BLE_CLKEN;
    
    // Wait for BLE core ready
    for (volatile int i = 0; i < 10000; i++)
        ;
    
    ble_state = BLE_STATE_IDLE;
    return 0;
}

int ble_comms_start_advertising(const char *name) {
    // Configure advertising parameters
    // Interval: 100ms (160 * 0.625ms)
    *(volatile uint32_t *)0x5000_5000 = 160;  // Adv interval
    *(volatile uint32_t *)0x5000_5004 = 0x00;  // Adv type: Connectable Undirected
    *(volatile uint32_t *)0x5000_5008 = (uint32_t)adv_data;  // Adv data pointer
    *(volatile uint32_t *)0x5000_500C = sizeof(adv_data);    // Adv data length
    
    // Start advertising
    *(volatile uint32_t *)0x5000_5010 = 0x01;  // Enable advertising
    
    ble_state = BLE_STATE_ADVERTISING;
    return 0;
}

int ble_comms_stop_advertising(void) {
    *(volatile uint32_t *)0x5000_5010 = 0x00;  // Disable advertising
    ble_state = BLE_STATE_IDLE;
    return 0;
}

int ble_comms_send_packet(const uint8_t *data, uint16_t len) {
    if (ble_state != BLE_STATE_CONNECTED && ble_state != BLE_STATE_STREAMING) {
        return -1;
    }
    
    // Send via BLE notification on capture characteristic
    // Max BLE notification payload = 20 bytes (BLE 4.0) or 244 (BLE 5.0 DLE)
    uint16_t mtu = 244;  // Assume BLE 5.0 with DLE
    
    uint16_t offset = 0;
    while (offset < len) {
        uint16_t chunk = (len - offset > mtu) ? mtu : (len - offset);
        
        // Write to BLE TX FIFO
        volatile uint8_t *ble_tx_fifo = (volatile uint8_t *)0x5000_6000;
        for (int i = 0; i < chunk; i++) {
            ble_tx_fifo[i] = data[offset + i];
        }
        
        // Trigger notification
        *(volatile uint32_t *)0x5000_6004 = chunk;
        
        // Wait for TX complete
        while (!(*(volatile uint32_t *)0x5000_6008 & 0x01))
            ;
        
        offset += chunk;
    }
    
    return len;
}

int ble_comms_send_status(const ble_status_t *status) {
    // Pack status into a 20-byte BLE packet
    uint8_t buf[20];
    buf[0] = status->mode;
    buf[1] = (status->freq >> 24) & 0xFF;
    buf[2] = (status->freq >> 16) & 0xFF;
    buf[3] = (status->freq >> 8) & 0xFF;
    buf[4] = status->freq & 0xFF;
    buf[5] = (uint8_t)status->rssi;
    buf[6] = (status->pkt_count >> 24) & 0xFF;
    buf[7] = (status->pkt_count >> 16) & 0xFF;
    buf[8] = (status->pkt_count >> 8) & 0xFF;
    buf[9] = status->pkt_count & 0xFF;
    buf[10] = (status->uptime_s >> 24) & 0xFF;
    buf[11] = (status->uptime_s >> 16) & 0xFF;
    buf[12] = (status->uptime_s >> 8) & 0xFF;
    buf[13] = status->uptime_s & 0xFF;
    buf[14] = status->battery_pct;
    buf[15] = 0;  // Reserved
    buf[16] = 0;
    buf[17] = 0;
    buf[18] = 0;
    buf[19] = 0;  // CRC placeholder
    
    return ble_comms_send_packet(buf, sizeof(buf));
}

int ble_comms_send_capture(const uint8_t *pcap_data, uint32_t len) {
    // Stream pcap data over BLE in chunks
    return ble_comms_send_packet(pcap_data, (uint16_t)len);
}

void ble_comms_set_mode(uint8_t mode) {
    // Update radio mode from BLE command
    // This is called from BLE write callback
    (void)mode;
}

void ble_comms_set_config(const uint8_t *config, uint16_t len) {
    // Parse configuration from BLE write
    (void)config;
    (void)len;
}

ble_state_t ble_comms_get_state(void) {
    return ble_state;
}

void ble_comms_task(void) {
    // Main BLE event loop — called from FreeRTOS task
    // Handles: connection events, notification scheduling, disconnect
    // In production, this integrates with TI BLE stack
}
```

## 5. Device Tree

```
// substation.dts — Device tree for Sub-GHz IoT Gateway Implant

/dts-v1/;

/ {
    model = "Substation Sub-GHz IoT Gateway Implant";
    compatible = "substation,sub-ghz-implant";
    
    chosen {
        stdout-path = &uart0;
    };
    
    cpus {
        cpu0: cpu@0 {
            compatible = "arm,cortex-m4f";
            clock-frequency = <48000000>;
        };
    };
    
    memory@20000000 {
        device_type = "memory";
        reg = <0x20000000 0x20000>;  // 128 KB internal SRAM
    };
    
    memory@60000000 {
        device_type = "memory";
        reg = <0x60000000 0x4000000>;  // 64 MB external SDRAM
    };
    
    memory@70000000 {
        device_type = "memory";
        reg = <0x70000000 0x1000000>;  // 16 MB external SPI Flash
    };
    
    soc {
        compatible = "ti,cc1352p";
        
        gpio: gpio@40022000 {
            compatible = "ti,cc26xx-gpio";
            reg = <0x40022000 0x1000>;
            interrupts = <0 0>;
            gpio-controller;
            #gpio-cells = <2>;
        };
        
        ssi0: spi@40008000 {
            compatible = "ti,cc26xx-ssi";
            reg = <0x40008000 0x1000>;
            interrupts = <24 0>;
            clock-frequency = <24000000>;
            #address-cells = <1>;
            #size-cells = <0>;
            
            sdram@0 {
                compatible = "issi,is66wvs256";
                reg = <0>;
                spi-max-frequency = <104000000>;
                memory-size = <33554432>;  // 32 MB
            };
        };
        
        ssi1: spi@4000A000 {
            compatible = "ti,cc26xx-ssi";
            reg = <0x4000A000 0x1000>;
            interrupts = <25 0>;
            
            flash@0 {
                compatible = "macronix,mx25lw1636";
                reg = <0>;
                spi-max-frequency = <80000000>;
            };
        };
        
        ssi3: spi@4000E000 {
            compatible = "ti,cc26xx-ssi";
            reg = <0x4000E000 0x1000>;
            interrupts = <27 0>;
        };
        
        i2c0: i2c@4000C000 {
            compatible = "ti,cc26xx-i2c";
            reg = <0x4000C000 0x1000>;
            interrupts = <26 0>;
            clock-frequency = <1000000>;  // 1 MHz fast mode+
            
            atecc@60 {
                compatible = "microchip,atecc608a";
                reg = <0x60>;
            };
        };
        
        uart0: serial@40023000 {
            compatible = "ti,cc26xx-uart";
            reg = <0x40023000 0x1000>;
            interrupts = <21 0>;
            current-speed = <115200>;
        };
        
        uart1: serial@40024000 {
            compatible = "ti,cc26xx-uart";
            reg = <0x40024000 0x1000>;
            interrupts = <22 0>;
            current-speed = <115200>;
        };
        
        usb: usb@40000000 {
            compatible = "ti,cc26xx-usb";
            reg = <0x40000000 0x1000>;
            interrupts = <35 0>;
            dr_mode = "peripheral";
            maximum-speed = "full-speed";
        };
        
        rfcore: rf@40037000 {
            compatible = "ti,cc1352p-rf";
            reg = <0x40037000 0x1000>;
            interrupts = <0 0>;
            frequencies = <281000000 1660000000  // Sub-GHz
                          2402000000 2480000000>; // 2.4 GHz
            max-tx-power = <20>;  // +20 dBm
        };
        
        crypto: crypto@0 {
            compatible = "microchip,atecc608a";
            reg = <0x60>;
            i2c-bus = <&i2c0>;
        };
    };
    
    leds {
        compatible = "gpio-leds";
        led0: led@16 {
            gpios = <&gpio 16 0>;
            label = "RGB Status";
            linux,default-trigger = "heartbeat";
        };
    };
    
    buttons {
        compatible = "gpio-keys";
        btn-mode {
            gpios = <&gpio 17 1>;  // Active low
            label = "Mode";
        };
        btn-action {
            gpios = <&gpio 18 1>;  // Active low
            label = "Action";
        };
    };
    
    sdmmc0: sd@0 {
        compatible = "ti,cc26xx-sd";
        spi-bus = <&ssi3>;
        cs-gpios = <&gpio 12 0>;
        cd-gpios = <&gpio 28 0>;
        voltage = <3300>;
    };
};
```

## 6. Build Instructions

### 6.1 Prerequisites

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Install TI SimpleLink SDK (for BLE stack)
# Download from: https://www.ti.com/tool/SIMPLELINK-CC13X2-26X2-SDK
# Install to: /opt/ti/simplelink_sdk

# Install Make, CMake
sudo apt-get install make cmake

# Install TI UniFlash (for programming)
# Download from: https://www.ti.com/tool/UNIFLASH

# Install KiCad (for PCB design)
sudo apt-get install kicad
```

### 6.2 Firmware Build

```bash
cd firmware/

# Build bootloader
make -f Makefile.boot TARGET=cc1352p BOOTLOADER=1

# Build application firmware
make TARGET=cc1352p SDK_PATH=/opt/ti/simplelink_sdk

# Build with secure boot enabled
make TARGET=cc1352p SECURE_BOOT=1 ATECC=1

# Generate binary for OTA
make TARGET=cc1352p OTA=1

# Flash via SWD (using OpenOCD)
openocd -f interface/cmsis-dap.cfg -f target/cc1352p.cfg \
    -c "program build/substation.elf verify reset exit"

# Flash via UniFlash
./uniflash.sh -cc1352p -program build/substation.hex
```

### 6.3 Companion App Build

```bash
cd app/

# Install dependencies
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios

# Build APK
cd android && ./gradlew assembleRelease

# Build IPA
xcodebuild -workspace Substation.xcworkspace -scheme Substation -configuration Release
```

### 6.4 KiCad Design

```bash
cd kicad/

# Generate netlist
kicad-cli netlist device.kicad_sch -o device.net

# Run DRC
kicad-cli drc device.kicad_pcb

# Generate Gerbers
kicad-cli pcb export gerbers device.kicad_pcb -o gerbers/

# Generate BOM
kicad-cli sch export bom device.kicad_sch -o bom.csv
```

## 7. FreeRTOS Task Architecture

| Task | Priority | Stack Size | Period | Function |
|------|----------|-----------|--------|----------|
| Radio RX | 7 (highest) | 1024 | Event-driven | Receive and queue packets |
| Radio TX | 6 | 512 | Event-driven | Transmit packets from queue |
| Packet Processor | 5 | 2048 | Event-driven | Parse, classify, store packets |
| SDRAM Manager | 4 | 512 | Event-driven | Ring buffer management |
| BLE Comms | 3 | 2048 | 10 ms | BLE notification streaming |
| USB Comms | 3 | 1024 | 10 ms | USB CDC-ECM data transfer |
| SD Logger | 2 | 1024 | 100 ms | Write pcap to MicroSD |
| LED Status | 1 (lowest) | 256 | 100 ms | WS2812B status updates |
| Watchdog | 0 | 128 | 1000 ms | Kick WDT, system health |

### Task Communication

```
┌─────────────┐     ┌──────────────────┐     ┌─────────────┐
│  Radio RX   │────►│  FreeRTOS Queue   │────►│  Packet     │
│  Task       │     │  (pkts, 1024)     │     │  Processor  │
└─────────────┘     └──────────────────┘     └──────┬──────┘
                                                       │
                                                       ▼
                                              ┌──────────────────┐
                                              │  SDRAM Ring      │
                                              │  Buffer (64 MB)  │
                                              └────────┬─────────┘
                                                       │
                              ┌─────────────────────────┼─────────────────┐
                              │                         │                  │
                              ▼                         ▼                  ▼
                       ┌─────────────┐         ┌──────────────┐    ┌────────────┐
                       │  BLE Comms  │         │  USB Comms   │    │  SD Logger │
                       │  Task       │         │  Task        │    │  Task       │
                       └─────────────┘         └──────────────┘    └────────────┘
```

## 8. USB Descriptors

### Device Descriptor
```
VID: 0x1209 (pid.codes)
PID: 0x5UB5 (SUBSTN)
Class: CDC-ECM (Communications + Ethernet Networking)
```

### Configuration
- Interface 0: CDC-ECM (Ethernet over USB)
  - Endpoint 0: Control (64 bytes)
  - Endpoint 1: CDC Data IN (64 bytes)
  - Endpoint 2: CDC Data OUT (64 bytes)
  - Endpoint 3: CDC Interrupt IN (8 bytes)
- Interface 1: CDC-ACM (Debug Console)
  - Endpoint 4: ACM Data IN (64 bytes)
  - Endpoint 5: ACM Data OUT (64 bytes)
  - Endpoint 6: ACM Interrupt IN (8 bytes)

The CDC-ECM interface presents the device as a network interface, allowing pcap download via TCP/IP over USB. The CDC-ACM interface provides a serial console for debugging.