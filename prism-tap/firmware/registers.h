/*
 * registers.h — Prism-Tap MCU Register Definitions
 * STM32H730VB Cortex-M7 Register Base Addresses & Bit Definitions
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_REGISTERS_H
#define PRISM_TAP_REGISTERS_H

#include <stdint.h>

/* ============================================================
 *  System & Bus Matrices
 * ============================================================ */
#define PERIPH_BASE       0x58000000U
#define D1_APB1_BASE      0x58000000U
#define D1_APB2_BASE      0x58010000U

/* ---- System Control (RCC) ---- */
#define RCC_BASE          0x58024400U
#define RCC_CR            (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLL1CFGR      (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_PLL2CFGR      (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_D1CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_D2CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_D3CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x20))
#define RCC_AHB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB3ENR       (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR       (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB1LENR      (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0xF0))

/* RCC bit definitions */
#define RCC_CR_HSION      (1U << 11)
#define RCC_CR_HSIRDY     (1U << 12)
#define RCC_CR_HSEON      (1U << 16)
#define RCC_CR_HSERDY     (1U << 17)
#define RCC_CR_HSEBYP     (1U << 18)
#define RCC_CR_PLL1ON     (1U << 24)
#define RCC_CR_PLL1RDY    (1U << 25)

/* RCC AHB4ENR bits (GPIO enable) */
#define RCC_AHB4ENR_GPIOAEN  (1U << 0)
#define RCC_AHB4ENR_GPIOBEN  (1U << 1)
#define RCC_AHB4ENR_GPIOCEN  (1U << 2)
#define RCC_AHB4ENR_GPIODEN  (1U << 3)
#define RCC_AHB4ENR_GPIOEEN  (1U << 4)
#define RCC_AHB4ENR_GPIOHEN  (1U << 7)

/* RCC APB1LENR bits */
#define RCC_APB1LENR_SPI2EN   (1U << 14)
#define RCC_APB1LENR_UART4EN  (1U << 18)
#define RCC_APB1LENR_I2C1EN   (1U << 21)
#define RCC_APB1LENR_I2C3EN   (1U << 23)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_SPI1EN  (1U << 12)
#define RCC_APB2ENR_SPI3EN  (1U << 15)
#define RCC_APB2ENR_USART1EN (1U << 14)

/* ---- Power Controller (PWR) ---- */
#define PWR_BASE          0x58024800U
#define PWR_CR1           (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR2           (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_CR3           (*(volatile uint32_t *)(PWR_BASE + 0x08))
#define PWR_SR1           (*(volatile uint32_t *)(PWR_BASE + 0x10))
#define PWR_SR2           (*(volatile uint32_t *)(PWR_BASE + 0x14))

#define PWR_CR1_VOS_SHIFT  3
#define PWR_CR1_VOS_MASK   (0x3U << PWR_CR1_VOS_SHIFT)
#define PWR_CR3_SCUEN      (1U << 0)

/* ============================================================
 *  GPIO Registers
 * ============================================================ */
#define GPIO_MODER_OFFSET   0x00
#define GPIO_OTYPER_OFFSET  0x04
#define GPIO_OSPEEDR_OFFSET 0x08
#define GPIO_PUPDR_OFFSET   0x0C
#define GPIO_IDR_OFFSET     0x10
#define GPIO_ODR_OFFSET     0x14
#define GPIO_BSRR_OFFSET    0x18
#define GPIO_AFRL_OFFSET    0x20
#define GPIO_AFRH_OFFSET    0x24

#define GPIO_REG(port, offset) (*(volatile uint32_t *)((port) + (offset)))

#define GPIO_MODE_INPUT   0
#define GPIO_MODE_OUTPUT   1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG   3

#define GPIO_OTYPE_PP      0
#define GPIO_OTYPE_OD      1

#define GPIO_SPEED_LOW     0
#define GPIO_SPEED_MID     1
#define GPIO_SPEED_HIGH   2
#define GPIO_SPEED_VHIGH  3

#define GPIO_PUPD_NONE     0
#define GPIO_PUPD_UP       1
#define GPIO_PUPD_DOWN     2

/* GPIO BSRR: bits 0-15 set, bits 16-31 reset */
#define GPIO_SET(port, pin)   GPIO_REG(port, GPIO_BSRR_OFFSET) = (1U << (pin))
#define GPIO_RESET(port, pin) GPIO_REG(port, GPIO_BSRR_OFFSET) = (1U << ((pin) + 16))
#define GPIO_READ(port, pin)  ((GPIO_REG(port, GPIO_IDR_OFFSET) >> (pin)) & 1U)

/* ---- Alternate function mappings ---- */
#define AF_USART1_TX_PA9     7
#define AF_USART1_RX_PA10    7
#define AF_SPI3_SCK_PB3      6
#define AF_SPI3_MISO_PB4    6
#define AF_SPI3_MOSI_PB5    6
#define AF_I2C1_SCL_PB8      4
#define AF_I2C1_SDA_PB9      4
#define AF_UART4_TX_PB10     5
#define AF_UART4_RX_PB11     5
#define AF_SPI2_SCK_PB13     5
#define AF_SPI2_MISO_PB14    5
#define AF_SPI2_MOSI_PB15    5
#define AF_SPI1_SCK_PC6      5
#define AF_SPI1_MISO_PC7     5
#define AF_SPI1_MOSI_PC8     5
#define AF_I2C3_SCL_PC10    4
#define AF_I2C3_SDA_PC11    4

/* ============================================================
 *  SPI Registers
 * ============================================================ */
#define SPI1_BASE   0x58013000U  /* APB2: SPI1 for NOR flash */
#define SPI2_BASE   0x58003800U  /* APB1: SPI2 for SD card */
#define SPI3_BASE   0x58003C00U  /* APB1: SPI3 for FPGA */

#define SPI_CR1(offset_base)  (*(volatile uint32_t *)((offset_base) + 0x00))
#define SPI_CR2(offset_base)  (*(volatile uint32_t *)((offset_base) + 0x04))
#define SPI_SR(offset_base)   (*(volatile uint32_t *)((offset_base) + 0x08))
#define SPI_DR(offset_base)   (*(volatile uint32_t *)((offset_base) + 0x0C))

#define SPI_CR1_CPHA     (1U << 0)
#define SPI_CR1_CPOL     (1U << 1)
#define SPI_CR1_MSTR     (1U << 2)
#define SPI_CR1_SSI      (1U << 8)
#define SPI_CR1_SSM      (1U << 9)
#define SPI_CR1_RXONLY   (1U << 10)
#define SPI_CR1_BR_DIV2  (0)
#define SPI_CR1_BR_DIV4  (1)
#define SPI_CR1_BR_DIV8  (2)
#define SPI_CR1_BR_DIV16 (3)
#define SPI_CR1_BR_DIV32 (4)
#define SPI_CR1_BR_DIV64 (5)
#define SPI_CR1_BR_DIV128 (6)
#define SPI_CR1_BR_DIV256 (7)
#define SPI_CR1_BR_SHIFT  3
#define SPI_CR1_LSBFIRST (1U << 7)
#define SPI_CR1_SPE      (1U << 6)

#define SPI_CR2_DS_8BIT  (7U << 8)
#define SPI_CR2_DS_16BIT (0xFU << 8)
#define SPI_CR2_FRF      (1U << 4)
#define SPI_CR2_SSOE     (1U << 2)
#define SPI_CR2_RXDMAEN  (1U << 0)
#define SPI_CR2_TXDMAEN  (1U << 1)

#define SPI_SR_RXNE      (1U << 0)
#define SPI_SR_TXE       (1U << 1)
#define SPI_SR_BSY       (1U << 7)
#define SPI_SR_FRE       (1U << 8)

/* ============================================================
 *  I2C Registers (simplified — I2C1/I2C3 on STM32H7)
 * ============================================================ */
#define I2C1_BASE  0x58005400U
#define I2C3_BASE  0x58005C00U

#define I2C_CR1(base)  (*(volatile uint32_t *)((base) + 0x00))
#define I2C_CR2(base)  (*(volatile uint32_t *)((base) + 0x04))
#define I2C_ISR(base)  (*(volatile uint32_t *)((base) + 0x10))
#define I2C_TXDR(base) (*(volatile uint32_t *)((base) + 0x28))
#define I2C_RXDR(base) (*(volatile uint32_t *)((base) + 0x24))
#define I2C_TIMING(base) (*(volatile uint32_t *)((base) + 0x08))

#define I2C_CR1_PE      (1U << 0)
#define I2C_CR2_START   (1U << 13)
#define I2C_CR2_STOP    (1U << 14)
#define I2C_CR2_NACK   (1U << 15)
#define I2C_CR2_RD_WRN  (1U << 10)
#define I2C_ISR_TXE     (1U << 0)
#define I2C_ISR_RXNE    (1U << 2)
#define I2C_ISR_TCR     (1U << 5)
#define I2C_ISR_TC      (1U << 6)
#define I2C_ISR_BUSY    (1U << 15)
#define I2C_ISR_NACKF   (1U << 4)

/* ============================================================
 *  USART Registers
 * ============================================================ */
#define USART1_BASE 0x58006400U  /* Debug console */
#define UART4_BASE  0x58004400U   /* BLE module */

#define USART_CR1(base)  (*(volatile uint32_t *)((base) + 0x00))
#define USART_CR2(base)  (*(volatile uint32_t *)((base) + 0x04))
#define USART_CR3(base)  (*(volatile uint32_t *)((base) + 0x08))
#define USART_BRR(base)  (*(volatile uint32_t *)((base) + 0x0C))
#define USART_RDR(base)  (*(volatile uint32_t *)((base) + 0x24))
#define USART_TDR(base)  (*(volatile uint32_t *)((base) + 0x28))
#define USART_ISR(base)  (*(volatile uint32_t *)((base) + 0x1C))

#define USART_CR1_UE    (1U << 0)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TXEIE  (1U << 7)
#define USART_ISR_RXNE  (1U << 5)
#define USART_ISR_TXE   (1U << 7)
#define USART_ISR_TC    (1U << 6)

/* ============================================================
 *  DMA Controller (simplified)
 * ============================================================ */
#define DMA1_BASE         0x58000000U
#define DMA1_STREAM0_BASE  (DMA1_BASE + 0x010)
#define DMA1_STREAM1_BASE  (DMA1_BASE + 0x028)
#define DMA1_STREAM2_BASE  (DMA1_BASE + 0x040)
#define DMA1_STREAM3_BASE  (DMA1_BASE + 0x058)

#define DMA_SCR(stream)   (*(volatile uint32_t *)((stream) + 0x00))
#define DMA_PAR(stream)   (*(volatile uint32_t *)((stream) + 0x04))
#define DMA_M0AR(stream)  (*(volatile uint32_t *)((stream) + 0x08))
#define DMA_NDTR(stream)  (*(volatile uint32_t *)((stream) + 0x0C))
#define DMA_ISR           (*(volatile uint32_t *)(DMA1_BASE + 0x000))
#define DMA_IFCR          (*(volatile uint32_t *)(DMA1_BASE + 0x004))

#define DMA_SCR_EN        (1U << 0)
#define DMA_SCR_TCIE      (1U << 4)
#define DMA_SCR_MINC      (1U << 10)
#define DMA_SCR_PINC      (1U << 9)
#define DMA_SCR_DIR_P2M   (0x2U << 6)
#define DMA_SCR_DIR_M2P   (0x1U << 6)
#define DMA_SCR_CHSEL_SHIFT 25

/* ============================================================
 *  Flash Controller
 * ============================================================ */
#define FLASH_BASE        0x58022000U
#define FLASH_ACR         (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_KEYR        (*(volatile uint32_t *)(FLASH_BASE + 0x08))
#define FLASH_SR          (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_CR          (*(volatile uint32_t *)(FLASH_BASE + 0x14))

#define FLASH_ACR_LATENCY_MASK  (0xFU << 0)
#define FLASH_ACR_PRFTEN        (1U << 8)
#define FLASH_ACR_ICEN          (1U << 9)
#define FLASH_ACR_DCEN          (1U << 10)

/* ---- Flash memory map ---- */
#define FLASH_BASE_ADDR   0x08000000U
#define FLASH_SIZE        (1U * 1024 * 1024)  /* 1 MB */
#define RAM_DTCM_BASE      0x20000000U
#define RAM_DTCM_SIZE      (128U * 1024)
#define RAM_SRAM1_BASE     0x24000000U
#define RAM_SRAM1_SIZE     (128U * 1024)

/* ============================================================
 *  CRC Engine
 * ============================================================ */
#define CRC_BASE       0x58026000U
#define CRC_DR          (*(volatile uint32_t *)(CRC_BASE + 0x00))
#define CRC_CR          (*(volatile uint32_t *)(CRC_BASE + 0x08))
#define CRC_INIT        (*(volatile uint32_t *)(CRC_BASE + 0x10))
#define CRC_POL          (*(volatile uint32_t *)(CRC_BASE + 0x14))

#define CRC_CR_RESET    (1U << 0)
#define CRC_CR_REV_IN_MASK  (0x3U << 5)
#define CRC_CR_REV_OUT     (1U << 7)

/* ============================================================
 *  SysTick & NVIC
 * ============================================================ */
#define SYSTICK_BASE   0xE000E010U
#define SYSTICK_CSR    (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_RVR    (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_CVR    (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CALIB  (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

#define SYSTICK_CSR_ENABLE  (1U << 0)
#define SYSTICK_CSR_CLKSRC  (1U << 2)
#define SYSTICK_CSR_TICKINT (1U << 1)

/* NVIC */
#define NVIC_ISER0   (*(volatile uint32_t *)(0xE000E100U))
#define NVIC_ICPR0   (*(volatile uint32_t *)(0xE000E280U))
#define NVIC_IP_BASE (0xE000E400U)

#define IRQ_USART1   37
#define IRQ_UART4    52
#define IRQ_SPI1     42
#define IRQ_SPI2     43
#define IRQ_SPI3     44
#define IRQ_I2C1_EV  31
#define IRQ_I2C1_ER  32
#define IRQ_DMA1_S0  11
#define IRQ_DMA1_S1  12

/* ---- RCC peripheral reset bits (for deinit) ---- */
#define RCC_AHB1RSTR_OFFSET  0xD0
#define RCC_AHB2RSTR_OFFSET  0xD4
#define RCC_AHB3RSTR_OFFSET  0xD8
#define RCC_AHB4RSTR_OFFSET  0xDC
#define RCC_APB1LRSTR_OFFSET 0xE0
#define RCC_APB2RSTR_OFFSET  0xE8

/* ---- Watchdog ---- */
#define IWDG_BASE    0x58004800U
#define IWDG_KR      (*(volatile uint32_t *)(IWDG_BASE + 0x00))
#define IWDG_PR      (*(volatile uint32_t *)(IWDG_BASE + 0x04))
#define IWDG_RLR     (*(volatile uint32_t *)(IWDG_BASE + 0x08))
#define IWDG_SR      (*(volatile uint32_t *)(IWDG_BASE + 0x0C))

#define IWDG_KR_ENABLE  0xCCCC
#define IWDG_KR_RELOAD  0xAAAA
#define IWDG_KR_UNLOCK  0x5555

/* ============================================================
 *  FPGA Register Map (via SPI)
 * ============================================================ */
#define FPGA_REG_VERSION      0x0000  /* FPGA bitstream version (RO)        */
#define FPGA_REG_STATUS       0x0002  /* Status: links, buffer state (RO)  */
#define FPGA_REG_CONTROL      0x0004  /* Control: start/stop tap, modes    */
#define FPGA_REG_IRQ_EN       0x0006  /* Interrupt enable mask             */
#define FPGA_REG_IRQ_STATUS   0x0008  /* Interrupt status (read to clear) */
#define FPGA_REG_FRAME_IDX    0x000A  /* Current frame index counter (RO)  */
#define FPGA_REG_FRAME_W     0x000C  /* Frame width from bridge (RO)      */
#define FPGA_REG_FRAME_H     0x000E  /* Frame height from bridge (RO)     */
#define FPGA_REG_FRAME_FMT   0x0010  /* Frame format (RO)                 */
#define FPGA_REG_BUFFER_A    0x0020  /* Buffer A DMA base address          */
#define FPGA_REG_BUFFER_B    0x0022  /* Buffer B DMA base address          */
#define FPGA_REG_BUFFER_SZ   0x0024  /* Buffer size in 32-bit words        */
#define FPGA_REG_ACTIVE_BUF   0x0026  /* 0=A, 1=B — currently filling      */
#define FPGA_REG_INJECT_CTRL  0x0030  /* Injection control register         */
#define FPGA_REG_INJECT_MODE  0x0032  /* INJECT_FULL/OVERLAY/SELECTIVE     */
#define FPGA_REG_INJECT_ADDR  0x0034  /* Injection frame base in FPGA RAM   */
#define FPGA_REG_INJECT_W     0x0036  /* Injection frame width             */
#define FPGA_REG_INJECT_H     0x0038  /* Injection frame height             */
#define FPGA_REG_OVERLAY_X   0x003A  /* Overlay X position                */
#define FPGA_REG_OVERLAY_Y   0x003C  /* Overlay Y position                */
#define FPGA_REG_OVERLAY_W   0x003E  /* Overlay width                     */
#define FPGA_REG_OVERLAY_H   0x0040  /* Overlay height                    */
#define FPGA_REG_TIMING_CTRL  0x0050  /* Timing manipulation control      */
#define FPGA_REG_TIMING_DELAY 0x0052  /* Delay in ns (signed)              */
#define FPGA_REG_TIMING_JITTER 0x0054 /* Jitter percentage 0-100           */
#define FPGA_REG_TIMING_DROP  0x0056  /* Drop pattern bitmask (cycling)    */
#define FPGA_REG_TIMING_DROPPED 0x0058/* Count of frames dropped (RO)     */
#define FPGA_REG_CSI_ERR     0x0060  /* CSI-2 error counters (RO)        */
#define FPGA_REG_DSI_ERR     0x0062  /* DSI error counters (RO)           */

/* FPGA status register bits */
#define FPGA_STATUS_CSI_LINK     (1U << 0)
#define FPGA_STATUS_DSI_LINK     (1U << 1)
#define FPGA_STATUS_BUF_A_FULL   (1U << 2)
#define FPGA_STATUS_BUF_B_FULL   (1U << 3)
#define FPGA_STATUS_INJECT_ACTIVE (1U << 4)
#define FPGA_STATUS_OVERFLOW      (1U << 5)
#define FPGA_STATUS_CRC_ERR       (1U << 6)

/* FPGA control register bits */
#define FPGA_CTRL_TAP_ENABLE     (1U << 0)
#define FPGA_CTRL_CAPTURE_ENABLE (1U << 1)
#define FPGA_CTRL_INJECT_ENABLE  (1U << 2)
#define FPGA_CTRL_TIMING_ENABLE  (1U << 3)
#define FPGA_CTRL_RESET          (1U << 7)

/* FPGA interrupt bits */
#define FPGA_IRQ_FRAME_READY     (1U << 0)
#define FPGA_IRQ_LINK_CHANGE     (1U << 1)
#define FPGA_IRQ_ERROR           (1U << 2)
#define FPGA_IRQ_OVERFLOW        (1U << 3)

#endif /* PRISM_TAP_REGISTERS_H */

/* ---- End of registers.h ----
 * Author: jayis1
 */