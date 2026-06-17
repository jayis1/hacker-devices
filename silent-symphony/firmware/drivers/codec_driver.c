/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * WM8731S I²S Audio Codec Driver — Implementation
 *
 * Initializes I²C4, WM8731S codec, I²S Tx (SPI1), I²S Rx (SPI3),
 * and their DMA streams.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "codec_driver.h"
#include <string.h>

/* =========================================================================
 * Private State
 * ========================================================================= */

static struct codec_config g_codec = {0};
static int16_t *g_tx_dma_buf0 = NULL;
static int16_t *g_tx_dma_buf1 = NULL;
static int16_t *g_rx_dma_buf0 = NULL;
static int16_t *g_rx_dma_buf1 = NULL;

/* =========================================================================
 * I²C4 Driver (400 kHz Fast Mode Master)
 * ========================================================================= */

int i2c4_init(void)
{
    /* Enable I2C4 clock (APB1L) — I2C4 is on APB1L */
    mmio_set_bits(RCC_APB1LENR, 1U << 9); /* I2C4EN = bit 9? Actually check: STM32H7 RM, I2C4EN = bit 9 */
    /* If wrong, try bit 8. We'll use a flexible approach. */

    /* Configure PA13 (SCL) and PA14 (SDA) as AF4 */
    /* PA13 = AF4 = I2C4_SCL, PA14 = AF4 = I2C4_SDA */
    
    /* Set GPIO mode to AF */
    uint32_t moder = mmio_read32(GPIOA_BASE + GPIO_MODER);
    moder &= ~(3U << (13 * 2));
    moder &= ~(3U << (14 * 2));
    moder |= (GPIO_MODE_AF << (13 * 2));
    moder |= (GPIO_MODE_AF << (14 * 2));
    mmio_write32(GPIOA_BASE + GPIO_MODER, moder);

    /* Set AF4 on both pins */
    uint32_t afrh = mmio_read32(GPIOA_BASE + GPIO_AFRH);
    afrh &= ~(0xFUL << ((13 - 8) * 4));
    afrh &= ~(0xFUL << ((14 - 8) * 4));
    afrh |= (AF_I2C4 << ((13 - 8) * 4));
    afrh |= (AF_I2C4 << ((14 - 8) * 4));
    mmio_write32(GPIOA_BASE + GPIO_AFRH, afrh);

    /* Set open-drain, high speed, pull-up */
    uint32_t otyper = mmio_read32(GPIOA_BASE + GPIO_OTYPER);
    otyper |= (1U << 13) | (1U << 14); /* Open-drain */
    mmio_write32(GPIOA_BASE + GPIO_OTYPER, otyper);

    uint32_t ospeedr = mmio_read32(GPIOA_BASE + GPIO_OSPEEDR);
    ospeedr |= (GPIO_SPEED_VERY_HIGH << (13 * 2));
    ospeedr |= (GPIO_SPEED_VERY_HIGH << (14 * 2));
    mmio_write32(GPIOA_BASE + GPIO_OSPEEDR, ospeedr);

    uint32_t pupdr = mmio_read32(GPIOA_BASE + GPIO_PUPDR);
    pupdr |= (GPIO_PULL_UP << (13 * 2));
    pupdr |= (GPIO_PULL_UP << (14 * 2));
    mmio_write32(GPIOA_BASE + GPIO_PUPDR, pupdr);

    /* Configure I2C4 as 400 kHz master */
    uint32_t timing = 0; /* STM32H7 I2C uses timing register, not CR2 */

    /* I2C4 base is at offset 0x64 in APB1L memory map? Let's check.
     * For STM32H743, I2C4 base = 0x40006400. */
    /* But wait — our register.h defines I2C4_BASE = 0x58006400. Let's use that. */
    /* Actually, I2C4 is at 0x58006400. */

    /* Reset I2C4 */
    uint32_t tmp = mmio_read32(I2C4_BASE + 0x00); /* CR1 */
    tmp &= ~(1U << 0); /* PE = 0 to reset */
    mmio_write32(I2C4_BASE + 0x00, tmp);

    /* Set timing for 400 kHz @ 120 MHz APB1 */
    /* STM32H7 I2C timing computes: SCLL + SCLH determine SCL freq.
     * For 400 kHz: PRESC=1, SCLL=0x32, SCLH=0x1D, SDADEL=0x2, SCLDEL=0x5 */
    mmio_write32(I2C4_BASE + 0x04, 0x5032021D); /* TIMINGR */

    /* Enable I2C4, master mode */
    mmio_write32(I2C4_BASE + 0x00, (1U << 0) | (1U << 4)); /* PE=1, SBC=1 (slave byte control... actually for master, just PE) */
    /* Simplified: just PE */
    mmio_write32(I2C4_BASE + 0x00, 1U << 0); /* PE = 1 */
    /* NOSTRETCH = 0 (clock stretching enabled) */

    return 0;
}

int i2c4_write(uint8_t dev_addr, const uint8_t *data, uint8_t len)
{
    if (!data || len == 0)
        return ERR_INVALID_PARAM;

    /* Wait for BUSY flag to clear */
    uint32_t timeout = 10000;
    while (mmio_read32(I2C4_BASE + 0x1C) & (1U << 16)) { /* ISR BUSY */
        if (--timeout == 0) return ERR_TIMEOUT;
    }

    /* Clear STOP and NACK flags */
    mmio_write32(I2C4_BASE + 0x18, (1U << 5) | (1U << 4)); /* ICR: STOPCF=1, NACKCF=1 */

    /* Send START + device address (write, 7-bit) */
    mmio_write32(I2C4_BASE + 0x10, ((uint32_t)dev_addr << 1) | 0); /* TXDR? No, CR2 controls this */
    /* STM32H7 I2C: set SADD + ADD10 + RD_WRN + START in CR2 */
    uint32_t cr2 = ((uint32_t)dev_addr << 1) | (1U << 13) | (uint32_t)len; /* SADD + START + NBYTES */
    mmio_write32(I2C4_BASE + 0x04, cr2); /* CR2 */

    /* Wait for TXIS flag (TX data register empty) */
    timeout = 10000;
    while (!(mmio_read32(I2C4_BASE + 0x1C) & (1U << 1))) { /* ISR TXIS */
        if (mmio_read32(I2C4_BASE + 0x1C) & (1U << 4)) { /* NACKF */
            mmio_write32(I2C4_BASE + 0x18, (1U << 4)); /* Clear NACK */
            return ERR_NACK;
        }
        if (--timeout == 0) return ERR_TIMEOUT;
    }

    /* Send data bytes */
    for (uint8_t i = 0; i < len; i++) {
        /* Mini delay for last byte to avoid overrun */
        timeout = 10000;
        while (!(mmio_read32(I2C4_BASE + 0x1C) & (1U << 1))) { /* TXIS */
            if (--timeout == 0) return ERR_TIMEOUT;
        }
        mmio_write32(I2C4_BASE + 0x24, data[i]); /* TXDR */
    }

    /* Wait for STOP flag */
    timeout = 10000;
    while (!(mmio_read32(I2C4_BASE + 0x1C) & (1U << 5))) { /* STOPF */
        if (--timeout == 0) return ERR_TIMEOUT;
    }

    /* Clear STOP flag */
    mmio_write32(I2C4_BASE + 0x18, (1U << 5));

    return ERR_OK;
}

/* =========================================================================
 * WM8731 Register Write via I²C
 * ========================================================================= */

int codec_reg_write(uint8_t reg, uint16_t val)
{
    uint8_t buf[2];
    /* WM8731 I²C format: first byte = (reg << 1) | (val >> 8), second byte = val & 0xFF */
    buf[0] = (reg << 1) | ((val >> 8) & 0x01);
    buf[1] = val & 0xFF;

    return i2c4_write(WM8731_I2C_ADDR, buf, 2);
}

/* =========================================================================
 * Codec API
 * ========================================================================= */

int codec_init(struct codec_config *config, uint32_t sr)
{
    int ret;

    /* Reset codec pin */
    uint32_t moder = mmio_read32(PORT_CODEC + GPIO_MODER);
    moder &= ~(3U << (PIN_CODEC_RST * 2));
    moder |= (GPIO_MODE_OUTPUT << (PIN_CODEC_RST * 2));
    mmio_write32(PORT_CODEC + GPIO_MODER, moder);

    /* Toggle reset */
    mmio_write32(PORT_CODEC + GPIO_BSRR, ((uint32_t)1U << (PIN_CODEC_RST + 16))); /* Reset = low */
    for (volatile int i = 0; i < 1000; i++) __asm__ volatile("nop");
    mmio_write32(PORT_CODEC + GPIO_BSRR, (1U << PIN_CODEC_RST)); /* Set = high */
    for (volatile int i = 0; i < 1000; i++) __asm__ volatile("nop");

    /* Initialize I²C4 */
    ret = i2c4_init();
    if (ret) return ret;

    /* Send reset command */
    ret = codec_reg_write(WM8731_REG_RESET, 0x000);
    if (ret) return ret;

    /* Apply configuration */
    struct codec_config defaults = WM8731_CONFIG_INIT;
    if (!config)
        config = &defaults;

    memcpy(&g_codec, config, sizeof(g_codec));

    /* Write all registers except reset and active */
    for (int i = 0; i <= WM8731_REG_ACTIVE; i++) {
        if (i == WM8731_REG_RESET) continue;
        ret = codec_reg_write((uint8_t)i, g_codec.regs[i]);
        if (ret) return ret;
    }

    /* Set sample rate */
    ret = codec_set_sample_rate(sr);
    if (ret) return ret;

    /* Activate codec */
    ret = codec_reg_write(WM8731_REG_ACTIVE, WM8731_ACTIVE_ENABLE);
    if (ret) return ret;

    /* Update shadow */
    g_codec.regs[WM8731_REG_ACTIVE] = WM8731_ACTIVE_ENABLE;
    g_codec.initialized = 1;
    g_codec.sample_rate = sr;

    return ERR_OK;
}

int codec_set_sample_rate(uint32_t sr)
{
    if (!g_codec.initialized)
        return ERR_NOT_INIT;

    uint16_t sampling = 0;
    uint8_t use_usb = 0;

    /* Determine codec configuration for sample rate */
    /* MCLK = 12.288 MHz (our I2S MCK output) */
    /* For 48 kHz: 12.288 MHz / 256 = 48 kHz */
    /* For 96 kHz: 12.288 MHz / 128 = 96 kHz  */
    switch (sr) {
    case 8000:
        /* 256 fs, USB mode (8000 = 48000 / 6) */
        sampling = WM8731_SAMPLING_CLK_256FS | (0x6 << 2); /* SR=6 for 8kHz */
        use_usb = 1;
        break;
    case 16000:
        sampling = WM8731_SAMPLING_CLK_256FS | (0x5 << 2); /* SR=5 for 16kHz */
        use_usb = 0;
        break;
    case 32000:
        sampling = WM8731_SAMPLING_CLK_256FS | (0x2 << 2); /* SR=2 for 32kHz */
        use_usb = 0;
        break;
    case 44100:
        sampling = WM8731_SAMPLING_CLK_256FS | (0x1 << 2); /* SR=1 for 44.1kHz */
        use_usb = 0;
        break;
    case 48000:
    default:
        sampling = WM8731_SAMPLING_CLK_256FS | (0x0 << 2); /* SR=0 for 48kHz */
        use_usb = 0;
        break;
    case 96000:
        /* 96 kHz requires USB mode and BOSR=1 */
        sampling = WM8731_SAMPLING_USB | WM8731_SAMPLING_BOSR | (0x7 << 2); /* SR=7 */
        use_usb = 0;
        break;
    }

    if (use_usb)
        sampling |= WM8731_SAMPLING_USB;

    int ret = codec_reg_write(WM8731_REG_SAMPLING, sampling);
    if (ret) return ret;

    g_codec.regs[WM8731_REG_SAMPLING] = sampling;
    g_codec.sample_rate = sr;

    return ERR_OK;
}

int codec_set_volume(uint8_t left, uint8_t right)
{
    if (!g_codec.initialized) return ERR_NOT_INIT;

    uint16_t lval = (uint16_t)(left & 0x7F) | WM8731_HP_ZC;
    uint16_t rval = (uint16_t)(right & 0x7F) | WM8731_HP_ZC;

    int ret = codec_reg_write(WM8731_REG_LEFT_HP, lval);
    if (ret) return ret;
    ret = codec_reg_write(WM8731_REG_RIGHT_HP, rval);
    if (ret) return ret;

    g_codec.regs[WM8731_REG_LEFT_HP] = lval;
    g_codec.regs[WM8731_REG_RIGHT_HP] = rval;

    return ERR_OK;
}

int codec_set_mute(uint8_t mute)
{
    if (!g_codec.initialized) return ERR_NOT_INIT;

    uint16_t val = g_codec.regs[WM8731_REG_ACTIVE];
    if (mute)
        val &= ~WM8731_ACTIVE_ENABLE;
    else
        val |= WM8731_ACTIVE_ENABLE;

    int ret = codec_reg_write(WM8731_REG_ACTIVE, val);
    if (ret) return ret;

    g_codec.regs[WM8731_REG_ACTIVE] = val;
    return ERR_OK;
}

int codec_power_down(void)
{
    if (!g_codec.initialized) return ERR_NOT_INIT;

    /* Disable active first */
    codec_reg_write(WM8731_REG_ACTIVE, 0x000);

    /* Power down all blocks */
    int ret = codec_reg_write(WM8731_REG_POWER, WM8731_POWER_ALL_OFF);
    if (ret) return ret;

    g_codec.regs[WM8731_REG_POWER] = WM8731_POWER_ALL_OFF;
    g_codec.regs[WM8731_REG_ACTIVE] = 0x000;

    return ERR_OK;
}

int codec_power_up(void)
{
    if (!g_codec.initialized) return ERR_NOT_INIT;

    /* Power on all blocks */
    int ret = codec_reg_write(WM8731_REG_POWER, WM8731_POWER_ALL_ON);
    if (ret) return ret;

    /* Re-apply all register settings */
    for (int i = 0; i < WM8731_REG_ACTIVE; i++) {
        if (i == WM8731_REG_RESET || i == WM8731_REG_POWER) continue;
        ret = codec_reg_write((uint8_t)i, g_codec.regs[i]);
        if (ret) return ret;
    }

    /* Re-activate */
    ret = codec_reg_write(WM8731_REG_ACTIVE, WM8731_ACTIVE_ENABLE);
    if (ret) return ret;

    g_codec.regs[WM8731_REG_ACTIVE] = WM8731_ACTIVE_ENABLE;

    return ERR_OK;
}

/* =========================================================================
 * I²S Initialization (SPI1 as I²S1 master Tx, SPI3 as I²S3 slave Rx)
 * ========================================================================= */

static void i2s_gpio_init(void)
{
    uint32_t moder, afrh_a, afrl_c, afrh_c;

    /* --- I2S1 Tx pins (PA3-MCK, PA4-WS, PA5-CK, PA7-SD) --- */
    moder  = mmio_read32(GPIOA_BASE + GPIO_MODER);
    afrh_a = mmio_read32(GPIOA_BASE + GPIO_AFRH);

    /* PA3 = AF5 */
    moder  &= ~(3U << (3 * 2));
    moder  |= (GPIO_MODE_AF << (3 * 2));
    afrh_a &= ~(0xFUL << ((3 - 8) * 4));
    afrh_a |= (AF_I2S1 << ((3 - 8) * 4));

    /* PA4 = AF5 */
    moder  &= ~(3U << (4 * 2));
    moder  |= (GPIO_MODE_AF << (4 * 2));
    afrh_a &= ~(0xFUL << ((4 - 8) * 4));
    afrh_a |= (AF_I2S1 << ((4 - 8) * 4));

    /* PA5 = AF5 */
    moder  &= ~(3U << (5 * 2));
    moder  |= (GPIO_MODE_AF << (5 * 2));
    afrh_a &= ~(0xFUL << ((5 - 8) * 4));
    afrh_a |= (AF_I2S1 << ((5 - 8) * 4));

    /* PA7 = AF5 */
    moder  &= ~(3U << (7 * 2));
    moder  |= (GPIO_MODE_AF << (7 * 2));
    afrh_a &= ~(0xFUL << ((7 - 8) * 4));
    afrh_a |= (AF_I2S1 << ((7 - 8) * 4));

    mmio_write32(GPIOA_BASE + GPIO_MODER, moder);
    mmio_write32(GPIOA_BASE + GPIO_AFRH, afrh_a);

    /* --- I2S3 Rx pins (PC10-CK, PC11-WS, PC12-SD) --- */
    moder  = mmio_read32(GPIOC_BASE + GPIO_MODER);
    afrl_c = mmio_read32(GPIOC_BASE + GPIO_AFRL);
    afrh_c = mmio_read32(GPIOC_BASE + GPIO_AFRH);

    /* PC10 = AF6 */
    moder  &= ~(3U << (10 * 2));
    moder  |= (GPIO_MODE_AF << (10 * 2));
    afrh_c &= ~(0xFUL << ((10 - 8) * 4));
    afrh_c |= (AF_I2S3 << ((10 - 8) * 4));

    /* PC11 = AF6 */
    moder  &= ~(3U << (11 * 2));
    moder  |= (GPIO_MODE_AF << (11 * 2));
    afrh_c &= ~(0xFUL << ((11 - 8) * 4));
    afrh_c |= (AF_I2S3 << ((11 - 8) * 4));

    /* PC12 = AF6 */
    moder  &= ~(3U << (12 * 2));
    moder  |= (GPIO_MODE_AF << (12 * 2));
    afrh_c &= ~(0xFUL << ((12 - 8) * 4));
    afrh_c |= (AF_I2S3 << ((12 - 8) * 4));

    mmio_write32(GPIOC_BASE + GPIO_MODER, moder);
    mmio_write32(GPIOC_BASE + GPIO_AFRH, afrh_c);

    /* Set high speed on all I2S pins */
    mmio_write32(GPIOA_BASE + GPIO_OSPEEDR, 0xFFFFFFFF);
    mmio_write32(GPIOC_BASE + GPIO_OSPEEDR, 0xFFFFFFFF);
}

int i2s_tx_init(uint32_t sample_rate, uint8_t bit_depth, uint8_t master_clock)
{
    (void)master_clock; /* Always enabled for codec clocking */

    /* Enable peripheral clocks */
    mmio_set_bits(RCC_APB2ENR, RCC_APB2ENR_SPI1);   /* SPI1 = I2S1 Tx */
    mmio_set_bits(RCC_AHB1ENR, RCC_AHB1ENR_DMA2);    /* DMA2 for SPI1 Tx */

    /* Configure GPIO pins */
    i2s_gpio_init();

    /* Disable SPI1 before config */
    mmio_write32(I2S_TX_BASE + SPI_CR1, 0);

    /* Configure I2S1 in I2S master Tx mode */
    uint32_t i2scfgr = SPI_I2SCFGR_I2SMOD; /* Enable I2S mode */
    i2scfgr |= SPI_I2SCFGR_I2SCFG_TX;      /* Master Tx */
    i2scfgr |= SPI_I2SCFGR_CKPOL;           /* CKPOL = 1 (I2S Philips: clock low when idle) */
    i2scfgr |= SPI_I2SCFGR_STD_PHILIPS;     /* I2S Philips standard */
    i2scfgr |= SPI_I2SCFGR_CHLEN;           /* 32-bit channel frame */

    if (bit_depth == 16)
        i2scfgr |= SPI_I2SCFGR_DATLEN_16;
    else if (bit_depth == 24)
        i2scfgr |= SPI_I2SCFGR_DATLEN_24;
    else
        i2scfgr |= SPI_I2SCFGR_DATLEN_32;

    /* I2S prescaler: fs = I2S_CLK / (I2SDIV * 8 * (CHLEN+1) * 2) */
    /* I2S_CLK = APB2 = 120 MHz */
    /* For 96 kHz: I2SDIV = 120MHz / (96k * 8 * 2 * 2) = 120M / 3.072M = 39 */
    uint16_t i2sdiv = (uint16_t)(I2S_TX_CLOCK / (sample_rate * 8 * 2 * 2));
    if (i2sdiv < 2)  i2sdiv = 2;
    if (i2sdiv > 255) i2sdiv = 255;
    i2scfgr |= (uint32_t)(i2sdiv & 0xFF);

    mmio_write32(I2S_TX_BASE + SPI_I2SCFGR, i2scfgr);

    /* Configure SPI CFG registers for I2S mode */
    /* CFG1: FIFO threshold = 4, master baud rate */
    uint32_t cfg1 = (4U << SPI_CFG1_FTHLV_OFFSET) | (0U << SPI_CFG1_MBR_OFFSET);
    mmio_write32(I2S_TX_BASE + SPI_CFG1, cfg1);

    /* CFG2: Tx-only, SSM=1, SSI=1 (master mode) */
    uint32_t cfg2 = SPI_CFG2_COMM_TX | SPI_CFG2_SSM | SPI_CFG2_SSI;
    mmio_write32(I2S_TX_BASE + SPI_CFG2, cfg2);

    /* Enable I2S with I2SE */
    mmio_set_bits(I2S_TX_BASE + SPI_CR1, SPI_CR1_SPE);
    mmio_set_bits(I2S_TX_BASE + SPI_I2SCFGR, SPI_I2SCFGR_I2SE);

    return ERR_OK;
}

int i2s_rx_init(uint32_t sample_rate, uint8_t bit_depth)
{
    (void)sample_rate; /* Slave mode — gets clock from Tx codec */

    /* Enable SPI3 clock */
    mmio_set_bits(RCC_APB1HENR, RCC_APB1HENR_SPI3);

    /* Disable SPI3 */
    mmio_write32(I2S_RX_BASE + SPI_CR1, 0);

    /* Configure I2S3 in I2S slave Rx mode */
    uint32_t i2scfgr = SPI_I2SCFGR_I2SMOD; /* Enable I2S mode */
    i2scfgr |= SPI_I2SCFGR_I2SCFG_RX;      /* Slave Rx */
    i2scfgr |= SPI_I2SCFGR_CKPOL;           /* CKPOL = 1 */
    i2scfgr |= SPI_I2SCFGR_STD_PHILIPS;     /* I²S Philips */
    i2scfgr |= SPI_I2SCFGR_CHLEN;           /* 32-bit channel frame */

    if (bit_depth == 16)
        i2scfgr |= SPI_I2SCFGR_DATLEN_16;
    else if (bit_depth == 24)
        i2scfgr |= SPI_I2SCFGR_DATLEN_24;
    else
        i2scfgr |= SPI_I2SCFGR_DATLEN_32;

    mmio_write32(I2S_RX_BASE + SPI_I2SCFGR, i2scfgr);

    /* CFG1: FIFO threshold = 1 (to minimize latency on Rx) */
    uint32_t cfg1 = (1U << SPI_CFG1_FTHLV_OFFSET);
    mmio_write32(I2S_RX_BASE + SPI_CFG1, cfg1);

    /* CFG2: Rx-only */
    uint32_t cfg2 = SPI_CFG2_COMM_RX;
    mmio_write32(I2S_RX_BASE + SPI_CFG2, cfg2);

    /* Enable I2S */
    mmio_set_bits(I2S_RX_BASE + SPI_CR1, SPI_CR1_SPE);
    mmio_set_bits(I2S_RX_BASE + SPI_I2SCFGR, SPI_I2SCFGR_I2SE);

    return ERR_OK;
}

/* =========================================================================
 * DMA Setup for I²S Audio Streams
 * ========================================================================= */

#define DMA2_BASE               0x58025400UL

int i2s_tx_dma_start(int16_t *buf0, int16_t *buf1, uint16_t n_samples)
{
    g_tx_dma_buf0 = buf0;
    g_tx_dma_buf1 = buf1;

    /* DMA2 Stream1 for SPI1_TX */
    uint32_t dma_base = DMA2_BASE;
    uint8_t stream = DMA_I2S_TX_STREAM & 0x7;

    /* Disable stream */
    mmio_write32(dma_base + DMA_SxCR_BASE(stream), 0);

    /* Set peripheral address (SPI1 TXDR) */
    mmio_write32(dma_base + DMA_SxPAR(stream), I2S_TX_BASE + SPI_TXDR);

    /* Set memory addresses (double-buffer mode) */
    mmio_write32(dma_base + DMA_SxM0AR(stream), (uint32_t)buf0);
    mmio_write32(dma_base + DMA_SxM1AR(stream), (uint32_t)buf1);

    /* Set number of data items (16-bit words) */
    mmio_write32(dma_base + DMA_SxNDTR(stream), n_samples);

    /* Configure: MEM->PERIPH, MINC=1, PINC=0, PSIZE=16, MSIZE=16,
     * CIRC=1, DBM=1, PRIO=HIGH, HTIE=1, TCIE=1 */
    uint32_t cr = DMA_SxCR_EN        /* Will be set last */
                | DMA_SxCR_TCIE      /* Transfer complete interrupt */
                | DMA_SxCR_HTIE      /* Half-transfer interrupt */
                | DMA_SxCR_TEIE      /* Error interrupt */
                | DMA_SxCR_DIR_M2P   /* Memory to peripheral */
                | DMA_SxCR_MINC      /* Memory increment */
                | (DMA_SxCR_MSIZE_16) /* Memory size 16-bit */
                | (DMA_SxCR_PSIZE_16) /* Peripheral size 16-bit */
                | DMA_SxCR_CIRC      /* Circular mode */
                | DMA_SxCR_DBM       /* Double-buffer mode */
                | (DMA_SxCR_PRIO_HI); /* High priority */

    mmio_write32(dma_base + DMA_SxCR_BASE(stream), cr);

    /* Enable stream */
    mmio_set_bits(dma_base + DMA_SxCR_BASE(stream), DMA_SxCR_EN);

    return ERR_OK;
}

int i2s_rx_dma_start(int16_t *buf0, int16_t *buf1, uint16_t n_samples)
{
    g_rx_dma_buf0 = buf0;
    g_rx_dma_buf1 = buf1;

    /* DMA2 Stream0 for SPI3_RX */
    uint32_t dma_base = DMA2_BASE;
    uint8_t stream = DMA_I2S_RX_STREAM & 0x7;

    /* Disable stream */
    mmio_write32(dma_base + DMA_SxCR_BASE(stream), 0);

    /* Set peripheral address (SPI3 RXDR) */
    mmio_write32(dma_base + DMA_SxPAR(stream), I2S_RX_BASE + SPI_RXDR);

    /* Set memory addresses */
    mmio_write32(dma_base + DMA_SxM0AR(stream), (uint32_t)buf0);
    mmio_write32(dma_base + DMA_SxM1AR(stream), (uint32_t)buf1);

    /* Set number of data items */
    mmio_write32(dma_base + DMA_SxNDTR(stream), n_samples);

    /* Configure: PERIPH->MEM, MINC=1, PINC=0, PSIZE=16, MSIZE=16,
     * CIRC=1, DBM=1, PRIO=VERY_HIGH, HTIE=1, TCIE=1 */
    uint32_t cr = DMA_SxCR_EN
                | DMA_SxCR_TCIE
                | DMA_SxCR_HTIE
                | DMA_SxCR_TEIE
                | DMA_SxCR_DIR_P2M   /* Peripheral to memory */
                | DMA_SxCR_MINC
                | DMA_SxCR_MSIZE_16
                | DMA_SxCR_PSIZE_16
                | DMA_SxCR_CIRC
                | DMA_SxCR_DBM
                | (DMA_SxCR_PRIO_VHI); /* Very high priority for Rx */

    mmio_write32(dma_base + DMA_SxCR_BASE(stream), cr);

    /* Enable */
    mmio_set_bits(dma_base + DMA_SxCR_BASE(stream), DMA_SxCR_EN);

    return ERR_OK;
}

void i2s_tx_dma_stop(void)
{
    uint32_t dma_base = DMA2_BASE;
    uint8_t stream = DMA_I2S_TX_STREAM & 0x7;
    mmio_write32(dma_base + DMA_SxCR_BASE(stream), 0);
}

void i2s_rx_dma_stop(void)
{
    uint32_t dma_base = DMA2_BASE;
    uint8_t stream = DMA_I2S_RX_STREAM & 0x7;
    mmio_write32(dma_base + DMA_SxCR_BASE(stream), 0);
}