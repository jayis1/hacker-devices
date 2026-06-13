# Phase 4: Software Stack — RF Transceiver Tool

## 1. MMIO Register Definitions

### 1.1 STM32F401xx Register Map (Base Addresses)

| Peripheral | Base Address | Bus | Clock Enable Bit |
|---|---|---|---|
| GPIOA | 0x40020000 | AHB1 | RCC_AHB1ENR[0] (GPIOAEN) |
| GPIOB | 0x40020400 | AHB1 | RCC_AHB1ENR[1] (GPIOBEN) |
| SPI1 | 0x40013000 | APB2 | RCC_APB2ENR[12] (SPI1EN) |
| SPI2 | 0x40003800 | APB1 | RCC_APB1ENR[14] (SPI2EN) |
| I2C1 | 0x40005400 | APB1 | RCC_APB1ENR[21] (I2C1EN) |
| USART1 | 0x40011000 | APB2 | RCC_APB2ENR[4] (USART1EN) |
| USB_OTG_FS | 0x50000000 | AHB2 | RCC_AHB2ENR[7] (OTGFSEN) |
| DMA2 | 0x40026400 | AHB1 | RCC_AHB1ENR[22] (DMA2EN) |
| SYSCFG | 0x40013800 | APB2 | RCC_APB2ENR[14] (SYSCFGEN) |
| NVIC | 0xE000E100 | Core | — |
| SCB | 0xE000ED00 | Core | — |
| FLASH | 0x40023C00 | AHB1 | — |
| PWR | 0x40007000 | APB1 | RCC_APB1ENR[28] (PWREN) |
| RCC | 0x40023800 | — | — |

### 1.2 Key Register Definitions

```c
/* RCC registers */
#define RCC_CR              (*((volatile uint32_t *)0x40023800))
#define RCC_PLLCFGR         (*((volatile uint32_t *)0x40023804))
#define RCC_CFGR            (*((volatile uint32_t *)0x40023808))
#define RCC_AHB1ENR         (*((volatile uint32_t *)0x40023830))
#define RCC_APB1ENR         (*((volatile uint32_t *)0x40023840))
#define RCC_APB2ENR         (*((volatile uint32_t *)0x40023844))

/* GPIO registers (per port, offset 0x400) */
#define GPIOx_MODER(b)      (*((volatile uint32_t *)(b + 0x00)))
#define GPIOx_OTYPER(b)     (*((volatile uint32_t *)(b + 0x04)))
#define GPIOx_OSPEEDR(b)    (*((volatile uint32_t *)(b + 0x08)))
#define GPIOx_PUPDR(b)      (*((volatile uint32_t *)(b + 0x0C)))
#define GPIOx_IDR(b)        (*((volatile uint32_t *)(b + 0x10)))
#define GPIOx_ODR(b)        (*((volatile uint32_t *)(b + 0x14)))
#define GPIOx_BSRR(b)       (*((volatile uint32_t *)(b + 0x18)))
#define GPIOx_AFRH(b)       (*((volatile uint32_t *)(b + 0x24)))
#define GPIOx_AFRL(b)       (*((volatile uint32_t *)(b + 0x20)))

/* SPI registers (offset from base) */
#define SPIx_CR1(b)         (*((volatile uint32_t *)(b + 0x00)))
#define SPIx_CR2(b)         (*((volatile uint32_t *)(b + 0x04)))
#define SPIx_SR(b)          (*((volatile uint32_t *)(b + 0x08)))
#define SPIx_DR(b)          (*((volatile uint32_t *)(b + 0x0C)))
#define SPIx_CRCPR(b)       (*((volatile uint32_t *)(b + 0x10)))

/* I2C registers (offset from base) */
#define I2Cx_CR1(b)         (*((volatile uint32_t *)(b + 0x00)))
#define I2Cx_CR2(b)         (*((volatile uint32_t *)(b + 0x04)))
#define I2Cx_SR1(b)         (*((volatile uint32_t *)(b + 0x14)))
#define I2Cx_SR2(b)         (*((volatile uint32_t *)(b + 0x18)))
#define I2Cx_CCR(b)         (*((volatile uint32_t *)(b + 0x1C)))
#define I2Cx_TRISE(b)       (*((volatile uint32_t *)(b + 0x20)))

/* DMA registers (DMA2, offset from base) */
#define DMAx_LISR(b)        (*((volatile uint32_t *)(b + 0x00)))
#define DMAx_HISR(b)        (*((volatile uint32_t *)(b + 0x04)))
#define DMAx_LIFCR(b)       (*((volatile uint32_t *)(b + 0x08)))
#define DMAx_HIFCR(b)       (*((volatile uint32_t *)(b + 0x0C)))
#define DMAx_SxCR(b,s)      (*((volatile uint32_t *)(b + 0x10 + 0x18*(s))))
#define DMAx_SxNDTR(b,s)    (*((volatile uint32_t *)(b + 0x14 + 0x18*(s))))
#define DMAx_SxPAR(b,s)     (*((volatile uint32_t *)(b + 0x18 + 0x18*(s))))
#define DMAx_SxM0AR(b,s)    (*((volatile uint32_t *)(b + 0x1C + 0x18*(s))))

/* USB OTG FS registers */
#define OTG_FS_GOTGCTL      (*((volatile uint32_t *)0x50000000))
#define OTG_FS_GOTGINT      (*((volatile uint32_t *)0x50000004))
#define OTG_FS_GAHBCFG      (*((volatile uint32_t *)0x50000008))
#define OTG_FS_GUSBCFG      (*((volatile uint32_t *)0x5000000C))
#define OTG_FS_GRSTCTL      (*((volatile uint32_t *)0x50000010))
#define OTG_FS_GINTSTS      (*((volatile uint32_t *)0x50000014))
#define OTG_FS_GINTMSK      (*((volatile uint32_t *)0x50000018))
#define OTG_FS_GRXSTSD      (*((volatile uint32_t *)0x5000001C))
#define OTG_FS_DCFG         (*((volatile uint32_t *)0x50000804))
#define OTG_FS_DAINT        (*((volatile uint32_t *)0x50000818))
#define OTG_FS_DAINTMSK     (*((volatile uint32_t *)0x5000081C))

/* Flash option bytes */
#define FLASH_ACR            (*((volatile uint32_t *)0x40023C00))
#define FLASH_OPTCR         (*((volatile uint32_t *)0x40023C14))
```

### 1.3 CC1101 Register Map (SPI-accessible)

| Addr | Register | Default | Description |
|---|---|---|---|
| 0x00 | IOCFG2 | 0x29 | GDO2 output configuration |
| 0x01 | IOCFG1 | 0x2E | GDO1 output configuration |
| 0x02 | IOCFG0 | 0x3F | GDO0 output configuration |
| 0x03 | FIFOTHR | 0x07 | RX/TX FIFO threshold |
| 0x04 | SYNC1 | 0xD3 | Sync word, high byte |
| 0x05 | SYNC0 | 0x91 | Sync word, low byte |
| 0x06 | PKTLEN | 0xFF | Packet length |
| 0x07 | PKTCTRL1 | 0x04 | Packet automation control |
| 0x08 | PKTCTRL0 | 0x45 | Packet automation control |
| 0x09 | ADDR | 0x00 | Device address |
| 0x0A | CHANNR | 0x00 | Channel number |
| 0x0B | FSCTRL1 | 0x0F | Frequency synthesizer control |
| 0x0C | FSCTRL0 | 0x00 | Frequency offset |
| 0x0D | FREQ2 | 0x1D | Frequency control, high byte |
| 0x0E | FREQ1 | 0x89 | Frequency control, middle byte |
| 0x0F | FREQ0 | 0x31 | Frequency control, low byte |
| 0x10 | MDMCFG4 | 0x87 | Modem configuration |
| 0x11 | MDMCFG3 | 0x83 | Modem configuration |
| 0x12 | MDMCFG2 | 0x93 | Modem configuration (modulation format) |
| 0x13 | MDMCFG1 | 0x22 | Modem configuration |
| 0x14 | MDMCFG0 | 0xF8 | Modem configuration |
| 0x15 | DEVIATN | 0x47 | Deviation setting |
| 0x1D | MCSM0 | 0x18 | Main state machine config |
| 0x1E | MCSM1 | 0x1C | Main state machine config |
| 0x1F | MCSM2 | 0x07 | Main state machine config |
| 0x20 | WOREVT1 | 0x87 | Wake-on-radio event timeout |
| 0x21 | WOREVT0 | 0x6B | Wake-on-radio event timeout |
| 0x22 | WORCTRL | 0xF8 | Wake-on-radio control |
| 0x23 | FREND1 | 0xB6 | Front-end RX config |
| 0x24 | FREND0 | 0x10 | Front-end TX config |
| 0x25 | FSCAL3 | 0xE9 | Frequency calibration |
| 0x26 | FSCAL2 | 0x2A | Frequency calibration |
| 0x27 | FSCAL1 | 0x00 | Frequency calibration |
| 0x28 | FSCAL0 | 0x1F | Frequency calibration |
| 0x2D | TEST2 | 0x81 | Test configuration |
| 0x2E | TEST1 | 0x35 | Test configuration |
| 0x2F | TEST0 | 0x09 | Test configuration |
| 0x30 | PA_TABLE0 | 0x00 | PA power setting 0 |
| 0x3E | PATABLE | — | PA table (burst access) |

### 1.4 nRF24L01+ Register Map (SPI-accessible)

| Addr | Register | Default | Description |
|---|---|---|---|
| 0x00 | CONFIG | 0x08 | Configuration |
| 0x01 | EN_AA | 0x3F | Enable auto-ack |
| 0x02 | EN_RXADDR | 0x03 | Enable RX addresses |
| 0x03 | SETUP_AW | 0x03 | Address width |
| 0x04 | SETUP_RETR | 0x03 | Setup retransmit |
| 0x05 | RF_CH | 0x02 | RF channel |
| 0x06 | RF_SETUP | 0x0F | RF setup (rate, power) |
| 0x07 | STATUS | 0x0E | Status |
| 0x08 | OBSERVE_TX | 0x00 | Transmit observer |
| 0x09 | RPD | 0x00 | Received power detector |
| 0x0A | RX_ADDR_P0 | 0xE7E7E7E7E7 | RX address pipe 0 |
| 0x0B | RX_ADDR_P1 | 0xE7E7E7E7E7 | RX address pipe 1 |
| 0x10 | TX_ADDR | 0xE7E7E7E7E7 | TX address |
| 0x11 | RX_PW_P0 | 0x00 | RX payload width pipe 0 |
| 0x12 | RX_PW_P1 | 0x00 | RX payload width pipe 1 |
| 0x17 | FIFO_STATUS | 0x11 | FIFO status |

**nRF24L01+ SPI Commands:**

| Command | Byte | Description |
|---|---|---|
| R_REGISTER | 0x00+addr | Read register |
| W_REGISTER | 0x20+addr | Write register |
| R_RX_PAYLOAD | 0x61 | Read RX payload |
| W_TX_PAYLOAD | 0xA0 | Write TX payload |
| FLUSH_TX | 0xE1 | Flush TX FIFO |
| FLUSH_RX | 0xE2 | Flush RX FIFO |
| REUSE_TX_PL | 0xE3 | Reuse TX payload |
| NOP | 0xFF | No operation (read STATUS) |

## 2. Clock Configuration

### 2.1 Clock Tree

```
                    ┌───────────┐
                    │  HSE 8MHz │
                    │  (Y1 XTAL)│
                    └─────┬─────┘
                          │
                    ┌─────┴──────┐
                    │   PLL       │
                    │  M=8        │  → PLL input: 1 MHz
                    │  N=336      │  → PLL VCO: 336 MHz
                    │  P=4        │  → PLL 84 MHz
                    │  Q=7        │  → PLL48: 48 MHz
                    └─────┬──────┘
                          │
                 ┌────────┴────────┐
                 │                 │
           SYSCLK = 84 MHz    USB 48 MHz
                 │
           AHB = 84 MHz (HCLK)
           APB1 = 42 MHz (PCLK1) — SPI2, I2C1
           APB2 = 84 MHz (PCLK2) — SPI1, USART1
```

### 2.2 Clock Init Code (Production C)

```c
/* See firmware/main.c for the complete implementation */
static void system_clock_config(void)
{
    /* Enable HSE and wait for ready */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY))
        ;

    /* Configure voltage regulator: power scale 2 for 84 MHz */
    RCC_APB1ENR |= (1 << 28);  /* PWREN */
    PWR_CR &= ~PWR_CR_VOS;
    PWR_CR |= PWR_CR_VOS_SCALE_2;
    while (!(PWR_CSR & PWR_CSR_VOSRDY))
        ;

    /* Configure flash: 2 wait states for 84 MHz at VOS scale 2 */
    FLASH_ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | 2;

    /* PLL configuration: HSE/8 * 336 / 4 = 84 MHz, 48 MHz USB */
    RCC_PLLCFGR = (8 << 0)       /* PLLM = 8 */
                | (336 << 6)     /* PLLN = 336 */
                | (0 << 16)      /* PLLP = 4 (0=4, 1=8, 2=6, 3=2) */
                | (1 << 22)     /* PLLSRC = HSE */
                | (7 << 24);    /* PLLQ = 7 */

    /* Enable PLL and wait */
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY))
        ;

    /* Switch SYSCLK to PLL */
    RCC_CFGR &= ~RCC_CFGR_SW_MASK;
    RCC_CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL)
        ;

    /* AHB prescaler = 1, APB1 = /2, APB2 = /1 */
    RCC_CFGR &= ~RCC_CFGR_HPRE_MASK;
    RCC_CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC_CFGR &= ~RCC_CFGR_PPRE1_MASK;
    RCC_CFGR |= RCC_CFGR_PPRE1_DIV2;  /* APB1 = 42 MHz */
    RCC_CFGR &= ~RCC_CFGR_PPRE2_MASK;
    RCC_CFGR |= RCC_CFGR_PPRE2_DIV1;  /* APB2 = 84 MHz */
}
```

## 3. GPIO Initialization

```c
static void gpio_init(void)
{
    /* Enable GPIO clocks */
    RCC_AHB1ENR |= (RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB | RCC_AHB1ENR_GPIOC);

    /* PA0: BTN1 input, pull-up */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (0 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (0 << (0 * 2));   /* Input */
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3 << (0 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1 << (0 * 2));    /* Pull-up */

    /* PA1: BTN2 input, pull-up */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (1 * 2));
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3 << (1 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1 << (1 * 2));

    /* PA3: CC1101 RESETn output, push-pull */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (3 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (1 << (3 * 2));    /* Output */
    GPIOx_OTYPER(GPIOA_BASE) &= ~(1 << 3);         /* Push-pull */
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3 << (3 * 2));   /* High speed */

    /* PA4: SPI1_NSS (CC1101 CSN) — managed by SPI1 hardware, but we
       bit-bang for multi-register access; initially high */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (4 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (1 << (4 * 2));    /* Output */
    GPIOx_ODR(GPIOA_BASE) |= (1 << 4);              /* High (deselected) */
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3 << (4 * 2));   /* High speed */

    /* PA5: SPI1_SCK — AF5 */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (5 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2 << (5 * 2));    /* Alternate function */
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xF << (5 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (5 << (5 * 4));      /* AF5 = SPI1_SCK */
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3 << (5 * 2));   /* High speed */

    /* PA6: SPI1_MISO — AF5 */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (6 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2 << (6 * 2));
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xF << (6 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (5 << (6 * 4));
    GPIOx_PUPDR(GPIOA_BASE) &= ~(3 << (6 * 2));
    GPIOx_PUPDR(GPIOA_BASE) |= (1 << (6 * 2));    /* Pull-up */

    /* PA7: SPI1_MOSI — AF5 */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (7 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2 << (7 * 2));
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xF << (7 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (5 << (7 * 4));
    GPIOx_OSPEEDR(GPIOA_BASE) |= (3 << (7 * 2));

    /* PA8: CC1101 GDO0 — input (default) or output */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (8 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (0 << (8 * 2));    /* Input initially */

    /* PA9: CC1101 GDO2 — input */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (9 * 2));

    /* PA11: USB_DM — AF10 */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (11 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2 << (11 * 2));
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xF << (11 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (10 << (11 * 4));

    /* PA12: USB_DP — AF10 */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (12 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (2 << (12 * 2));
    GPIOx_AFRL(GPIOA_BASE) &= ~(0xF << (12 * 4));
    GPIOx_AFRL(GPIOA_BASE) |= (10 << (12 * 4));

    /* PA15: nRF24 CE — output, push-pull */
    GPIOx_MODER(GPIOA_BASE) &= ~(3 << (15 * 2));
    GPIOx_MODER(GPIOA_BASE) |= (1 << (15 * 2));
    GPIOx_ODR(GPIOA_BASE) &= ~(1 << 15);           /* Low (disabled) */

    /* PB0: LED1 output */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (0 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (1 << (0 * 2));
    GPIOx_OTYPER(GPIOB_BASE) &= ~(1 << 0);
    GPIOx_ODR(GPIOB_BASE) &= ~(1 << 0);             /* Off */

    /* PB1: LED2 output */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (1 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (1 << (1 * 2));
    GPIOx_OTYPER(GPIOB_BASE) &= ~(1 << 1);
    GPIOx_ODR(GPIOB_BASE) &= ~(1 << 1);

    /* PB3: nRF24 IRQ — input, pull-up */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (3 * 2));
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3 << (3 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1 << (3 * 2));

    /* PB6: I2C1_SCL — AF4 */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (6 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2 << (6 * 2));
    GPIOx_AFRL(GPIOB_BASE) &= ~(0xF << (6 * 4));
    GPIOx_AFRL(GPIOB_BASE) |= (4 << (6 * 4));
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3 << (6 * 2));
    GPIOx_OTYPER(GPIOB_BASE) |= (1 << 6);          /* Open-drain */
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3 << (6 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1 << (6 * 2));

    /* PB7: I2C1_SDA — AF4 */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (7 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2 << (7 * 2));
    GPIOx_AFRL(GPIOB_BASE) &= ~(0xF << (7 * 4));
    GPIOx_AFRL(GPIOB_BASE) |= (4 << (7 * 4));
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3 << (7 * 2));
    GPIOx_OTYPER(GPIOB_BASE) |= (1 << 7);
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3 << (7 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1 << (7 * 2));

    /* PB12: SPI2_NSS (nRF24 CSN) — output, initially high */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (12 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (1 << (12 * 2));
    GPIOx_ODR(GPIOB_BASE) |= (1 << 12);
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3 << (12 * 2));

    /* PB13: SPI2_SCK — AF5 */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (13 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2 << (13 * 2));
    GPIOx_AFRL(GPIOB_BASE) &= ~(0xF << (13 * 4 - 32));
    GPIOx_AFRH(GPIOB_BASE) &= ~(0xF << ((13 - 8) * 4));
    GPIOx_AFRH(GPIOB_BASE) |= (5 << ((13 - 8) * 4));
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3 << (13 * 2));

    /* PB14: SPI2_MISO — AF5 */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (14 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2 << (14 * 2));
    GPIOx_AFRH(GPIOB_BASE) &= ~(0xF << ((14 - 8) * 4));
    GPIOx_AFRH(GPIOB_BASE) |= (5 << ((14 - 8) * 4));
    GPIOx_PUPDR(GPIOB_BASE) &= ~(3 << (14 * 2));
    GPIOx_PUPDR(GPIOB_BASE) |= (1 << (14 * 2));

    /* PB15: SPI2_MOSI — AF5 */
    GPIOx_MODER(GPIOB_BASE) &= ~(3 << (15 * 2));
    GPIOx_MODER(GPIOB_BASE) |= (2 << (15 * 2));
    GPIOx_AFRH(GPIOB_BASE) &= ~(0xF << ((15 - 8) * 4));
    GPIOx_AFRH(GPIOB_BASE) |= (5 << ((15 - 8) * 4));
    GPIOx_OSPEEDR(GPIOB_BASE) |= (3 << (15 * 2));
}
```

## 4. SPI Configuration

### 4.1 SPI1 (CC1101) — Mode 0, 10 MHz, 8-bit, full-duplex, MSB-first

```c
static void spi1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* SPI1 CR1: BR=0 (PCLK2/2=42MHz→prescale /4=10.5MHz), CPOL=0, CPHA=0, MSB, 8-bit, full-duplex, NSS software */
    SPIx_CR1(SPI1_BASE) = SPI_CR1_MSTR      /* Master mode */
                         | SPI_CR1_SSM       /* Software NSS */
                         | SPI_CR1_SSI       /* Internal NSS high */
                         | (0 << 3);         /* BR: 000 = fPCLK/2 = 42 MHz (we'll prescale) */
    /* Actually use BR=001 = /4 → 84/4 = 21 MHz, still within CC1101 10 MHz max...
       Let's use BR=010 = /8 = 10.5 MHz */
    SPIx_CR1(SPI1_BASE) = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
                         | (2 << 3);         /* BR=010 = /8 = 10.5 MHz */

    SPIx_CR2(SPI1_BASE) = 0;  /* No DMA yet, no NSS pulse */

    SPIx_CR1(SPI1_BASE) |= SPI_CR1_SPE;  /* Enable SPI1 */
}
```

### 4.2 SPI2 (nRF24L01+) — Mode 0, 10 MHz, 8-bit, full-duplex, MSB-first

```c
static void spi2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_SPI2EN;

    /* SPI2 on APB1 (42 MHz): BR=001 = /4 = 10.5 MHz */
    SPIx_CR1(SPI2_BASE) = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
                         | (1 << 3);         /* BR=001 = /4 = 10.5 MHz */

    SPIx_CR2(SPI2_BASE) = 0;
    SPIx_CR1(SPI2_BASE) |= SPI_CR1_SPE;
}
```

### 4.3 I2C1 (SSD1306) — 400 kHz, 7-bit addressing

```c
static void i2c1_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* I2C1 on APB1 (42 MHz): CCR for 400 kHz fast mode */
    I2Cx_CR1(I2C1_BASE) &= ~I2C_CR1_PE;  /* Disable to configure */

    I2Cx_CR2(I2C1_BASE) = 42;  /* FREQ = 42 MHz APB1 clock */
    I2Cx_CCR(I2C1_BASE) = 35;  /* CCR = 42MHz / (400kHz * 2) ≈ 52.5, use 35 for slight margin */
    I2Cx_TRISE(I2C1_BASE) = 43; /* TRISE = (1000ns / (1/42MHz)) + 1 = 43 */

    I2Cx_CR1(I2C1_BASE) |= I2C_CR1_PE | I2C_CR1_ACK;
}
```

## 5. Critical Device Driver: CC1101 SPI with DMA

See `firmware/drivers/cc1101.c` for the complete production driver with DMA support.

**Key design decisions:**
- SPI CSN is bit-banged (GPIO output) because CC1101 requires multi-byte burst transactions with CSN held low
- GDO0/GDO2 are polled for FIFO status (no EXTI interrupts in initial version)
- DMA is used for bulk FIFO reads/writes (SPI1 TX on DMA2 Stream3 Channel3, SPI1 RX on DMA2 Stream0 Channel3)

## 6. Device Tree Overlay

```
/* /sys/firmware/devicetree/ overlay for Linux USB gadget mode */
/dts-v1/;
/plugin/;

/ {
    compatible = "stm32f401ccu6,rf-transceiver-tool";
    
    fragment@0 {
        target = <&spi1>;
        __overlay__ {
            status = "okay";
            cc1101@0 {
                compatible = "ti,cc1101";
                reg = <0>;
                spi-max-frequency = <10000000>;
                gdo0-gpio = <&gpioa 8 0>;
                gdo2-gpio = <&gpioa 9 0>;
                reset-gpio = <&gpioa 3 GPIO_ACTIVE_LOW>;
            };
        };
    };
    
    fragment@1 {
        target = <&spi2>;
        __overlay__ {
            status = "okay";
            nrf24l01@0 {
                compatible = "nordic,nrf24l01+";
                reg = <0>;
                spi-max-frequency = <10000000>;
                ce-gpio = <&gpioa 15 0>;
                irq-gpio = <&gpiob 3 GPIO_ACTIVE_LOW>;
            };
        };
    };
    
    fragment@2 {
        target = <&i2c1>;
        __overlay__ {
            status = "okay";
            ssd1306@3c {
                compatible = "solomon,ssd1306fb-i2c";
                reg = <0x3c>;
                solomon,width = <128>;
                solomon,height = <64>;
                solomon,page-offset = <0>;
                solomon,com-invdir;
            };
        };
    };
};
```

## 7. Bootloader Strategy

The device uses the **STM32 built-in bootloader** (BOOT0 pin) for initial programming, then switches to a custom **USB DFU** bootloader in the first 16 KB of flash for subsequent field updates.

| Boot Mode | BOOT0 | BOOT1 | Vector |
|---|---|---|---|
| Flash (normal) | 0 | X | 0x08000000 |
| System bootloader (UART/SPI) | 1 | 0 | 0x00000000 (system memory) |

**Bootloader partition layout:**

| Region | Address | Size | Content |
|---|---|---|---|
| Bootloader | 0x08000000 | 16 KB | USB DFU bootloader (always present) |
| Application | 0x08004000 | 240 KB | Main firmware |
| Config | 0x0803C000 | 8 KB | RF config store (calibration, profiles) |
| Option bytes | 0x1FFFC000 | — | BOOT0 pull-down, SWD enabled |

**Field update flow:**
1. Hold BTN1 during reset → bootloader enters DFU mode
2. Host sends new firmware via USB DFU (STM32 DFU protocol)
3. Bootloader writes to application region (0x08004000)
4. On next reset without BTN1, bootloader jumps to application

**SWD debug connector (J4):**

| Pin | Signal |
|---|---|
| 1 | VCC (3.3V) |
| 2 | SWDIO (PA13) |
| 3 | GND |
| 4 | SWCLK (PA14) |
| 5 | GND |
| 6 | NRST |