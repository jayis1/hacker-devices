/*
 * aperture-phantom / firmware / registers.h
 *
 * Memory map and register definitions for the AperturePhantom control MCU
 * (STM32H743). Defines peripheral base addresses, FPGA gateware register
 * file offsets, BLE/USB command opcodes, script-engine opcodes, and the
 * capture/inject frame header layout.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef APERTURE_PHANTOM_REGISTERS_H
#define APERTURE_PHANTOM_REGISTERS_H

#include <stdint.h>
#include <stddef.h>

/* ---- Cortex-M4 peripheral base (referenced by board.h too) ---- */
#define PERIPH_BASE         0x40000000u
#define PERIPH_BASE_AHB     0x48000000u
#define PERIPH_BASE_APB1    0x40000000u
#define PERIPH_BASE_APB2    0x40010000u

/* ---- RCC ---- */
#define RCC_BASE            0x58024400u
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD4)) /* FMC */
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x100))

/* RCC enable bits */
#define RCC_AHB1ENR_DMA1EN  (1u << 0)
#define RCC_AHB1ENR_DMA2EN  (1u << 1)
#define RCC_AHB1ENR_MDMAEN  (1u << 16)
#define RCC_AHB3ENR_FMCEN   (1u << 0)
#define RCC_APB1LENR_USART3EN (1u << 18)
#define RCC_APB1LENR_I2C1EN (1u << 21)
#define RCC_APB1LENR_I2C2EN (1u << 22)
#define RCC_APB2ENR_SPI1EN  (1u << 12)

/* ---- PWR ---- */
#define PWR_BASE            0x48000000u
#define PWR_CR1             (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR3             (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_CPUCR           (*(volatile uint32_t *)(PWR_BASE + 0x10))

/* ---- GPIO ---- */
#define GPIOA_BASE          0x58020000u
#define GPIOB_BASE          0x58020400u
#define GPIOC_BASE          0x58020800u
#define GPIOD_BASE          0x58020C00u
#define GPIOE_BASE          0x58021000u
#define GPIOH_BASE          0x58021C00u

#define GPIO_MODER(g)       (*(volatile uint32_t *)((g) + 0x00))
#define GPIO_OTYPER(g)      (*(volatile uint32_t *)((g) + 0x04))
#define GPIO_OSPEEDR(g)     (*(volatile uint32_t *)((g) + 0x08))
#define GPIO_PUPDR(g)       (*(volatile uint32_t *)((g) + 0x0C))
#define GPIO_IDR(g)         (*(volatile uint32_t *)((g) + 0x10))
#define GPIO_ODR(g)         (*(volatile uint32_t *)((g) + 0x14))
#define GPIO_BSRR(g)        (*(volatile uint32_t *)((g) + 0x18))
#define GPIO_AFRL(g)        (*(volatile uint32_t *)((g) + 0x20))
#define GPIO_AFRH(g)        (*(volatile uint32_t *)((g) + 0x24))

/* ---- USART3 (BLE module console) ---- */
#define USART3_BASE         0x40004800u
#define USART3_CR1          (*(volatile uint32_t *)(USART3_BASE + 0x00))
#define USART3_CR2          (*(volatile uint32_t *)(USART3_BASE + 0x04))
#define USART3_CR3          (*(volatile uint32_t *)(USART3_BASE + 0x08))
#define USART3_BRR          (*(volatile uint32_t *)(USART3_BASE + 0x0C))
#define USART3_ISR          (*(volatile uint32_t *)(USART3_BASE + 0x1C))
#define USART3_RDR          (*(volatile uint32_t *)(USART3_BASE + 0x24))
#define USART3_TDR          (*(volatile uint32_t *)(USART3_BASE + 0x28))
#define USART3_ISR_TXE      (1u << 7)
#define USART3_ISR_RXNE     (1u << 5)
#define USART3_ISR_TC       (1u << 6)

/* ---- I2C1 / I2C2 (CCI sensor control, two sides of link) ---- */
#define I2C1_BASE           0x40005400u
#define I2C2_BASE           0x40005800u
#define I2C_CR1(i)          (*(volatile uint32_t *)((i) + 0x00))
#define I2C_CR2(i)          (*(volatile uint32_t *)((i) + 0x04))
#define I2C_OAR1(i)         (*(volatile uint32_t *)((i) + 0x08))
#define I2C_TIMINGR(i)     (*(volatile uint32_t *)((i) + 0x10))
#define I2C_ISR(i)          (*(volatile uint32_t *)((i) + 0x18))
#define I2C_RXDR(i)         (*(volatile uint32_t *)((i) + 0x24))
#define I2C_TXDR(i)         (*(volatile uint32_t *)((i) + 0x28))
#define I2C_ISR_TXE        (1u << 0)
#define I2C_ISR_RXNE       (1u << 2)
#define I2C_ISR_TC         (1u << 6)
#define I2C_ISR_NACKF      (1u << 2)
#define I2C_ISR_BUSY       (1u << 15)

/* ---- SPI1 (to FPGA bridge) ---- */
#define SPI1_BASE           0x40013000u
#define SPI1_CR1            (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_CR2            (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI1_CFG1           (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_CFG2           (*(volatile uint32_t *)(SPI1_BASE + 0x0C))
#define SPI1_SR             (*(volatile uint32_t *)(SPI1_BASE + 0x14))
#define SPI1_DR             (*(volatile uint32_t *)(SPI1_BASE + 0x18))
#define SPI1_SR_TXP         (1u << 1)
#define SPI1_SR_RXP         (1u << 0)
#define SPI1_SR_EOT        (1u << 3)

/* ---- SDMMC1 (microSD capture/replay storage) ---- */
#define SDMMC1_BASE         0x48002000u
#define SDMMC1_POWER        (*(volatile uint32_t *)(SDMMC1_BASE + 0x00))
#define SDMMC1_CLKCR        (*(volatile uint32_t *)(SDMMC1_BASE + 0x04))
#define SDMMC1_ARG          (*(volatile uint32_t *)(SDMMC1_BASE + 0x08))
#define SDMMC1_CMD          (*(volatile uint32_t *)(SDMMC1_BASE + 0x0C))
#define SDMMC1_RESP1        (*(volatile uint32_t *)(SDMMC1_BASE + 0x14))
#define SDMMC1_DTIMER       (*(volatile uint32_t *)(SDMMC1_BASE + 0x40))
#define SDMMC1_DLENR        (*(volatile uint32_t *)(SDMMC1_BASE + 0x44))
#define SDMMC1_DCTRL        (*(volatile uint32_t *)(SDMMC1_BASE + 0x48))
#define SDMMC1_DCOUNT       (*(volatile uint32_t *)(SDMMC1_BASE + 0x4C))
#define SDMMC1_STA          (*(volatile uint32_t *)(SDMMC1_BASE + 0x50))
#define SDMMC1_ICR          (*(volatile uint32_t *)(SDMMC1_BASE + 0x58))

/* ---- FMC (SDRAM frame buffer) ---- */
#define FMC_BASE            0x52004000u
#define FMC_Bank5_SDCR1     (*(volatile uint32_t *)(FMC_BASE + 0x240))
#define FMC_Bank5_SDCR2     (*(volatile uint32_t *)(FMC_BASE + 0x244))
#define FMC_Bank5_SDTR1     (*(volatile uint32_t *)(FMC_BASE + 0x248))
#define FMC_Bank5_SDCMR     (*(volatile uint32_t *)(FMC_BASE + 0x250))
#define FMC_Bank5_SDSR      (*(volatile uint32_t *)(FMC_BASE + 0x258))
#define SDRAM_BANK_ADDR    0xC0000000u
#define SDRAM_SIZE_BYTES    (16u * 1024u * 1024u)

/* ---- DMA / MDMA ---- */
#define DMA1_BASE           0x48000000u
#define DMA1_STREAM0        (DMA1_BASE + 0x10)
#define MDMA_BASE           0x52000000u

/* ---- USB OTG-HS (CDC backhaul) ---- */
#define USB_OTG_HS_BASE     0x40040000u

/* ---- NVIC ---- */
#define NVIC_ISER0          (*(volatile uint32_t *)(0xE000E100u))
#define NVIC_ICPR0          (*(volatile uint32_t *)(0xE000E280u))

/* ---- External interrupts ---- */
#define EXTI_BASE           0x58000000u
#define EXTI_IMR1           (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_RTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x20))
#define EXTI_FTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x24))
#define EXTI_PR1            (*(volatile uint32_t *)(EXTI_BASE + 0x3C))

/* ---- SysTick / SCB ---- */
#define SCB_VTOR            (*(volatile uint32_t *)(0xE000ED08u))
#define SCB_CCR             (*(volatile uint32_t *)(0xE000ED14u))
#define SCB_SHPR3           (*(volatile uint32_t *)(0xE000ED20u))
#define SYST_CSR            (*(volatile uint32_t *)(0xE000E010u))
#define SYST_RVR            (*(volatile uint32_t *)(0xE000E014u))
#define SYST_CVR            (*(volatile uint32_t *)(0xE000E018u))

/* ===================================================================== *
 * FPGA gateware register file (memory-mapped via SPI command 0x00..0x3F) *
 * The CrossLink-NX presents a 64-entry 32-bit register file.            *
 * ===================================================================== */
#define FPGA_REG_VERSION        0x00u   /* RO: gateware version (0xA5A5xxxx) */
#define FPGA_REG_STATUS        0x04u   /* RO: link/lane state */
#define FPGA_REG_CONTROL        0x08u   /* RW: mode bits */
#define FPGA_REG_LANE_MASK      0x0Cu   /* RW: enabled lanes (0x01..0x0F) */
#define FPGA_REG_DPHY_RATE      0x10u   /* RW: desired Mbps/lane x10 (e.g. 8000) */
#define FPGA_REG_VC_MASK        0x14u   /* RW: virtual-channel mask */
#define FPGA_REG_DT_FILTER      0x18u   /* RW: data-type filter bitmap */
#define FPGA_REG_CAP_FIFO_LVL   0x1Cu   /* RO: capture FIFO fill level (bytes) */
#define FPGA_REG_INJ_FIFO_LVL   0x20u   /* RO: inject FIFO free level (bytes) */
#define FPGA_REG_LINE_COUNT     0x24u   /* RO: lines in current frame */
#define FPGA_REG_WORD_COUNT     0x28u   /* RO: bytes captured in current frame */
#define FPGA_REG_CRC_ERR_COUNT  0x2Cu   /* RO: CSI-2 header CRC errors */
#define FPGA_REG_ECC_ERR_COUNT  0x30u   /* RO: CSI-2 ECC one-bit errors */
#define FPGA_REG_FRAME_COUNT   0x34u   /* RO: total frames captured since reset */
#define FPGA_REG_FRAME_LEN      0x38u   /* RO: last complete frame length (bytes) */
#define FPGA_REG_TX_DTYPE       0x3Cu   /* RW: TX data type to emit (inject) */
#define FPGA_REG_TX_WIDTH       0x40u   /* RW: TX frame width  (pixels) */
#define FPGA_REG_TX_HEIGHT      0x44u   /* RW: TX frame height (pixels) */
#define FPGA_REG_TX_VC          0x48u   /* RW: TX virtual channel */
#define FPGA_REG_TX_TRIGGER     0x4Cu   /* WO: strobe to send one frame from inject FIFO */

/* STATUS bits */
#define FPGA_STATUS_RX_LOCK      (1u << 0)
#define FPGA_STATUS_TX_LOCK      (1u << 1)
#define FPGA_STATUS_CAP_FIFO_OVF (1u << 2)
#define FPGA_STATUS_INJ_FIFO_UDF (1u << 3)
#define FPGA_STATUS_CAP_DONE     (1u << 4)
#define FPGA_STATUS_LINK_ERR     (1u << 8)
#define FPGA_STATUS_SENSOR_CLK   (1u << 9)   /* sensor clock present */
#define FPGA_STATUS_PASSTHRU     (1u << 16)
#define FPGA_STATUS_CAPTURING    (1u << 17)
#define FPGA_STATUS_INJECTING    (1u << 18)
#define FPGA_STATUS_REPLAYING    (1u << 19)
#define FPGA_STATUS_FUZZING      (1u << 20)

/* CONTROL bits */
#define FPGA_CTRL_PASSTHROUGH     (1u << 0)
#define FPGA_CTRL_CAP_ENABLE      (1u << 1)
#define FPGA_CTRL_INJECT_ENABLE   (1u << 2)
#define FPGA_CTRL_MUTE_SENSOR     (1u << 3)
#define FPGA_CTRL_RESET           (1u << 4)
#define FPGA_CTRL_CRC_CHECK       (1u << 5)
#define FPGA_CTRL_ECC_CHECK       (1u << 6)
#define FPGA_CTRL_TX_CRC_BAD      (1u << 7)  /* for fuzzing: corrupt TX CRC */
#define FPGA_CTRL_TX_SHORT_LINE   (1u << 8)  /* for fuzzing: short line */
#define FPGA_CTRL_TX_BAD_DT       (1u << 9)  /* for fuzzing: bad data type */
#define FPGA_CTRL_TX_OVERSIZE     (1u << 10) /* for fuzzing: oversize payload */

/* ===================================================================== *
 * BLE / USB binary command protocol (framed over GATT notify / CDC)     *
 * Format: [STX=0xAA][LEN][OP][PAYLOAD..][CRC8][ETX=0x55]                *
 * ===================================================================== */
#define CMD_STX                0xAAu
#define CMD_ETX                0x55u

typedef enum {
    CMD_NOP = 0x00,
    CMD_PING = 0x01,
    CMD_GET_STATUS = 0x02,        /* → status struct */
    CMD_GET_LINK_INFO = 0x03,     /* → lanes, rate, dt */
    CMD_SET_MODE = 0x04,         /* PASSTHROUGH/CAPTURE/INJECT/REPLAY/FUZZ */
    CMD_ARM = 0x05,              /* arm/disarm */
    CMD_CAP_START = 0x06,
    CMD_CAP_STOP = 0x07,
    CMD_CAP_SNAPSHOT = 0x08,     /* one frame */
    CMD_CAP_STREAM = 0x09,       /* stream downsampled frames over BLE */
    CMD_INJECT_LOAD = 0x0A,      /* payload follows, slot id in payload[0] */
    CMD_INJECT_NOW = 0x0B,      /* inject slot id now */
    CMD_INJECT_LOOP = 0x0C,     /* inject slot id repeatedly with ms period */
    CMD_INJECT_STOP = 0x0D,
    CMD_REPLAY_LIST = 0x0E,    /* list SD replay library */
    CMD_REPLAY_START = 0x0F,   /* replay file by index */
    CMD_REPLAY_STOP = 0x10,
    CMD_SCRIPT_LOAD = 0x11,     /* upload bytecode */
    CMD_SCRIPT_RUN = 0x12,
    CMD_SCRIPT_STOP = 0x13,
    CMD_SCRIPT_STATUS = 0x14,
    CMD_SENSOR_SCAN = 0x15,    /* I2C bus scan on side-A CCI */
    CMD_SENSOR_READ = 0x16,   /* addr, reg, len */
    CMD_SENSOR_WRITE = 0x17,  /* addr, reg, val */
    CMD_SENSOR_AUTOCONFIG = 0x18, /* detect common sensors, load defaults */
    CMD_FUZZ_START = 0x19,   /* strategy mask, count, delay */
    CMD_FUZZ_STOP = 0x1A,
    CMD_TRIGGER_SET = 0x1B,  /* set reference frame for trigger */
    CMD_TRIGGER_ENABLE = 0x1C,
    CMD_TRIGGER_DISABLE = 0x1D,
    CMD_TOUCH_EVENT = 0x1E,  /* from device: capacitive pad event */
    CMD_AUTH_CHALLENGE = 0x1F, /* DS28E07 challenge/response */
    CMD_LOG = 0x20,         /* device → app log line */
    CMD_FRAME = 0x21,       /* device → app downscaled frame chunk */
    CMD_ERROR = 0x7F
} cmd_opcode_t;

/* ===================================================================== *
 * Script engine bytecode (executed on-device, phone-less autonomous)    *
 * ===================================================================== */
typedef enum {
    OP_NOP = 0x00,
    OP_WAIT_MS = 0x01,       /* u16 ms */
    OP_WAIT_FRAMES = 0x02,   /* u16 count (waits for that many RX frames) */
    OP_CAPTURE = 0x03,       /* u8 slot, u16 count */
    OP_INJECT = 0x04,        /* u8 slot */
    OP_REPLAY = 0x05,        /* u8 file index, u16 loop count */
    OP_SENSOR_WRITE = 0x06,  /* u8 reg, u16 val */
    OP_SENSOR_READ = 0x07,   /* u8 reg, u8 slot to store into */
    OP_SET_MODE = 0x08,      /* u8 mode */
    OP_IF_TRIGGER = 0x09,    /* u8 slot; skip next if no trigger */
    OP_TRIGGER_ARM = 0x0A,   /* arm trigger on current frame */
    OP_TRIGGER_DISARM = 0x0B,
    OP_BLE_NOTIFY = 0x0C,    /* notify app with u8 code */
    OP_LOG = 0x0D,           /* u8 string id */
    OP_LED = 0x0E,           /* u8 color */
    OP_MOTOR_PULSE = 0x0F,   /* u8 ms */
    OP_LOOP = 0x10,          /* u16 count (0 = infinite) */
    OP_END_LOOP = 0x11,
    OP_JUMP = 0x12,          /* u16 addr */
    OP_HALT = 0x13
} script_op_t;

/* ===================================================================== *
 * On-disk frame format (.apf)                                          *
 * ===================================================================== */
#define APF_MAGIC0  'A'
#define APF_MAGIC1  'P'
#define APF_MAGIC2  'F'
#define APF_VERSION 1u

typedef struct __attribute__((packed)) {
    uint8_t  magic[3];         /* 'A','P','F' */
    uint8_t  version;
    uint16_t width;
    uint16_t height;
    uint8_t  data_type;        /* CSI-2 data type byte */
    uint8_t  virtual_channel;
    uint16_t reserved;
    uint32_t ts_ms;            /* capture timestamp */
    uint32_t payload_len;
    /* payload_len bytes follow: pixel data */
} apf_header_t;

/* CSI-2 data types we care about */
#define CSI_DT_RAW8         0x2Au
#define CSI_DT_RAW10        0x2Bu
#define CSI_DT_RAW12        0x2Cu
#define CSI_DT_RAW14        0x2Du
#define CSI_DT_YUV422_8     0x1Eu
#define CSI_DT_YUV422_10    0x1Fu
#define CSI_DT_RGB565       0x22u
#define CSI_DT_RGB888       0x24u
#define CSI_DT_USER_DEF_1   0x30u
#define CSI_DT_USER_DEF_2   0x31u
#define CSI_DT_USER_DEF_3   0x32u
#define CSI_DT_USER_DEF_4   0x33u
#define CSI_DT_YUV420_8     0x18u
#define CSI_DT_YUV420_10    0x19u

/* CSI-2 short packet codes (the byte stuffed into the data-type field
 * of a short packet) */
#define CSI_SP_FS           0x00u   /* frame start */
#define CSI_SP_FE           0x01u   /* frame end */
#define CSI_SP_LS           0x02u   /* line start */
#define CSI_SP_LE           0x03u   /* line end */
#define CSI_SP_GEN_SHORT_1  0x08u
#define CSI_SP_GEN_SHORT_2  0x09u
#define CSI_SP_GEN_SHORT_3  0x0Au
#define CSI_SP_GEN_SHORT_4  0x0Bu

/* ---- Operating modes ---- */
typedef enum {
    MODE_PASSTHROUGH = 0,
    MODE_CAPTURE = 1,
    MODE_INJECT = 2,
    MODE_REPLAY = 3,
    MODE_FUZZ = 4
} op_mode_t;

/* ---- Inject slots ---- */
#define INJECT_SLOTS 4
#define MAX_INJECT_FRAME_BYTES (1920u * 1080u) /* worst case */

/* ---- Replay library ---- */
#define MAX_REPLAY_FILES 64

/* ---- Misc helpers ---- */
#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)       (((a) < (b)) ? (a) : (b))
#define MAX(a,b)       (((a) > (b)) ? (a) : (b))

/* CRC8 (poly 0x07, init 0x00) for command framing */
static inline uint8_t crc8(const uint8_t *p, size_t n)
{
    uint8_t c = 0x00;
    while (n--) {
        c ^= *p++;
        for (int i = 0; i < 8; i++)
            c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
    }
    return c;
}

#endif /* APERTURE_PHANTOM_REGISTERS_H */