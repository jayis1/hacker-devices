/*
 * drivers/mipi_bridge.c — MIPI Bridge Chip Control for Prism-Tap
 *
 * Implements I2C-based configuration of the four MIPI bridge chips:
 *   - TC358746XBG  (CSI-2 Rx: sensor → parallel)
 *   - TC358748XBG  (CSI-2 Tx: parallel → CSI-2 to AP)
 *   - ADV7480      (DSI Rx: AP DSI → parallel)
 *   - TC358762XBG  (DSI Tx: parallel → DSI to panel)
 *
 * The I2C bus is shared (I2C1, PB8/PB9). Each chip has a unique 7-bit address.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "mipi_bridge.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- I2C timing values for 400 kHz fast-mode on STM32H7 (I2C1) ---- */
#define I2C1_TIMING_400K  0x20B15A55U

/* ---- TC358746/748 register addresses (16-bit addressed via I2C) ---- */
#define TC358746_REG_CHIP_ID     0x0000
#define TC358746_REG_RESET      0x0002
#define TC358746_REG_PLL         0x0004
#define TC358746_REG_CLK_CTRL    0x0006
#define TC358746_REG_LANE_EN     0x0008
#define TC358746_REG_DPHY_CFG    0x000A
#define TC358746_REG_CSITX_CTRL  0x000C  /* TC358748 uses this for Tx config  */
#define TC358746_REG_FIFO_CTRL   0x000E
#define TC358746_REG_DATA_FMT    0x0010
#define TC358746_REG_WIDTH       0x0012
#define TC358746_REG_HEIGHT      0x0014
#define TC358746_REG_HSYNC       0x0016
#define TC358746_REG_VSYNC       0x0018
#define TC358746_REG_INT_STATUS  0x001A
#define TC358746_REG_INT_MASK    0x001C
#define TC358746_REG_STATUS      0x001E

/* TC358746 chip ID values */
#define TC358746_CHIP_ID_VAL  0x4600
#define TC358748_CHIP_ID_VAL  0x4800

/* ---- ADV7480 register addresses (8-bit addressed) ---- */
#define ADV7480_REG_CHIP_ID_HI  0x00
#define ADV7480_REG_CHIP_ID_LO  0x01
#define ADV7480_REG_PWR_CTRL    0x0E
#define ADV7480_REG_DSI_RESET   0x10
#define ADV7480_REG_DSI_LANES   0x12
#define ADV7480_REG_DSI_FMT     0x14
#define ADV7480_REG_DSI_WIDTH_H 0x16
#define ADV7480_REG_DSI_WIDTH_L 0x17
#define ADV7480_REG_DSI_HEIGHT_H 0x18
#define ADV7480_REG_DSI_HEIGHT_L 0x19
#define ADV7480_REG_STATUS      0x20

#define ADV7480_CHIP_ID_HI_VAL  0x48
#define ADV7480_CHIP_ID_LO_VAL  0x80

/* ---- TC358762 register addresses (16-bit addressed) ---- */
#define TC358762_REG_CHIP_ID    0x0000
#define TC358762_REG_RESET      0x0004
#define TC358762_REG_DSI_CTRL   0x0008
#define TC358762_REG_DSI_LANES  0x000A
#define TC358762_REG_DSI_CLK    0x000C
#define TC358762_REG_PARALLEL   0x0010
#define TC358762_REG_DATA_FMT   0x0012
#define TC358762_REG_WIDTH      0x0014
#define TC358762_REG_HEIGHT     0x0016
#define TC358762_REG_STATUS     0x0018

#define TC358762_CHIP_ID_VAL    0x7620

/* ---- I2C low-level implementation ---- */

int bridge_i2c_init(void)
{
    /* Enable GPIOB clock */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    /* Enable I2C1 clock */
    RCC_APB1LENR |= RCC_APB1LENR_I2C1EN;

    /* Configure PB8 (SCL) and PB9 (SDA) as AF4 (I2C1) */
    uint32_t moder = GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (BRIDGE_I2C_PIN_SCL * 2));
    moder &= ~(3U << (BRIDGE_I2C_PIN_SDA * 2));
    moder |= (GPIO_MODE_AF << (BRIDGE_I2C_PIN_SCL * 2));
    moder |= (GPIO_MODE_AF << (BRIDGE_I2C_PIN_SDA * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) = moder;

    /* Set AF4 for I2C1 */
    uint32_t afrl = GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET);
    afrl &= ~(0xFU << (BRIDGE_I2C_PIN_SCL * 4));
    afrl |= (AF_I2C1_SCL_PB8 << (BRIDGE_I2C_PIN_SCL * 4));
    afrl &= ~(0xFU << (BRIDGE_I2C_PIN_SDA * 4));
    afrl |= (AF_I2C1_SDA_PB9 << (BRIDGE_I2C_PIN_SDA * 4));
    GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET) = afrl;

    /* Open-drain output type for I2C */
    uint32_t otyper = GPIO_REG(GPIOB_BASE, GPIO_OTYPER_OFFSET);
    otyper |= (GPIO_OTYPE_OD << BRIDGE_I2C_PIN_SCL);
    otyper |= (GPIO_OTYPE_OD << BRIDGE_I2C_PIN_SDA);
    GPIO_REG(GPIOB_BASE, GPIO_OTYPER_OFFSET) = otyper;

    /* High speed */
    uint32_t speed = GPIO_REG(GPIOB_BASE, GPIO_OSPEEDR_OFFSET);
    speed |= (GPIO_SPEED_VHIGH << (BRIDGE_I2C_PIN_SCL * 2));
    speed |= (GPIO_SPEED_VHIGH << (BRIDGE_I2C_PIN_SDA * 2));
    GPIO_REG(GPIOB_BASE, GPIO_OSPEEDR_OFFSET) = speed;

    /* Pull-up for I2C (external 4.7k pull-ups on board) */
    uint32_t pupdr = GPIO_REG(GPIOB_BASE, GPIO_PUPDR_OFFSET);
    pupdr &= ~(3U << (BRIDGE_I2C_PIN_SCL * 2));
    pupdr &= ~(3U << (BRIDGE_I2C_PIN_SDA * 2));
    pupdr |= (GPIO_PUPD_UP << (BRIDGE_I2C_PIN_SCL * 2));
    pupdr |= (GPIO_PUPD_UP << (BRIDGE_I2C_PIN_SDA * 2));
    GPIO_REG(GPIOB_BASE, GPIO_PUPDR_OFFSET) = pupdr;

    /* Configure I2C1 timing for 400 kHz */
    I2C_CR1(I2C1_BASE) = 0;  /* Disable */
    I2C_TIMING(I2C1_BASE) = I2C1_TIMING_400K;
    I2C_CR1(I2C1_BASE) = I2C_CR1_PE;  /* Enable peripheral */

    return 0;
}

/* Write data to I2C device (7-bit addr, 8-bit register addr, N data bytes) */
int bridge_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    uint8_t addr7 = (dev_addr << 1);

    /* Wait until bus not busy */
    uint32_t timeout = 100000;
    while ((I2C_ISR(I2C1_BASE) & I2C_ISR_BUSY) && timeout--)
        ;

    /* Start + write address */
    I2C_CR2(I2C1_BASE) = ((uint32_t)addr7) |
                         ((uint32_t)(1 + len) << 16) |
                         I2C_CR2_START;

    /* Wait for TXIS (TX empty) or NACK */
    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXE) && timeout--) {
        if (I2C_ISR(I2C1_BASE) & I2C_ISR_NACKF)
            return -1;
    }
    if (timeout == 0)
        return -2;

    /* Send register address */
    I2C_TXDR(I2C1_BASE) = reg_addr;

    /* Send data bytes */
    for (uint8_t i = 0; i < len; i++) {
        timeout = 10000;
        while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXE) && timeout--)
            ;
        if (timeout == 0)
            return -3;
        I2C_TXDR(I2C1_BASE) = data[i];
    }

    /* Wait for transfer complete */
    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TC) && timeout--)
        ;
    if (timeout == 0)
        return -4;

    /* Generate STOP */
    I2C_CR2(I2C1_BASE) |= I2C_CR2_STOP;

    return 0;
}

/* Read data from I2C device (7-bit addr, 8-bit register addr, N data bytes) */
int bridge_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    uint8_t addr7_w = (dev_addr << 1);
    uint8_t addr7_r = (dev_addr << 1) | 1;

    /* Wait until bus not busy */
    uint32_t timeout = 100000;
    while ((I2C_ISR(I2C1_BASE) & I2C_ISR_BUSY) && timeout--)
        ;

    /* Write phase: send register address */
    I2C_CR2(I2C1_BASE) = ((uint32_t)addr7_w) |
                         ((uint32_t)1 << 16) |
                         I2C_CR2_START;

    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXE) && timeout--)
        ;
    if (timeout == 0)
        return -2;

    I2C_TXDR(I2C1_BASE) = reg_addr;

    /* Wait for write TC */
    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TC) && timeout--)
        ;
    if (timeout == 0)
        return -3;

    /* Read phase: restart with read address */
    I2C_CR2(I2C1_BASE) = ((uint32_t)addr7_r) |
                         ((uint32_t)len << 16) |
                         I2C_CR2_START | I2C_CR2_RD_WRN;

    for (uint8_t i = 0; i < len; i++) {
        timeout = 10000;
        while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_RXNE) && timeout--)
            ;
        if (timeout == 0)
            return -4;
        data[i] = (uint8_t)I2C_RXDR(I2C1_BASE);
    }

    /* NACK + STOP (last byte) */
    I2C_CR2(I2C1_BASE) |= I2C_CR2_NACK | I2C_CR2_STOP;

    return 0;
}

/* ---- TC358746/748 16-bit register access ----
 * These chips use 16-bit register addresses, so we send 2 address bytes.
 */

static int tc358746_write16(uint8_t dev_addr, uint16_t reg, uint16_t val)
{
    uint8_t buf[4];
    buf[0] = (uint8_t)(reg >> 8);
    buf[1] = (uint8_t)(reg & 0xFF);
    buf[2] = (uint8_t)(val >> 8);
    buf[3] = (uint8_t)(val & 0xFF);

    /* Use raw I2C: start, write 4 bytes, stop */
    uint8_t addr7 = (dev_addr << 1);
    uint32_t timeout = 100000;
    while ((I2C_ISR(I2C1_BASE) & I2C_ISR_BUSY) && timeout--)
        ;

    I2C_CR2(I2C1_BASE) = ((uint32_t)addr7) | ((uint32_t)4 << 16) | I2C_CR2_START;

    for (int i = 0; i < 4; i++) {
        timeout = 10000;
        while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXE) && timeout--) {
            if (I2C_ISR(I2C1_BASE) & I2C_ISR_NACKF)
                return -1;
        }
        if (timeout == 0)
            return -2;
        I2C_TXDR(I2C1_BASE) = buf[i];
    }

    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TC) && timeout--)
        ;
    I2C_CR2(I2C1_BASE) |= I2C_CR2_STOP;
    return 0;
}

static uint16_t tc358746_read16(uint8_t dev_addr, uint16_t reg)
{
    uint8_t addr7_w = (dev_addr << 1);
    uint8_t addr7_r = (dev_addr << 1) | 1;
    uint8_t reg_hi = (uint8_t)(reg >> 8);
    uint8_t reg_lo = (uint8_t)(reg & 0xFF);
    uint32_t timeout;

    /* Write register address */
    timeout = 100000;
    while ((I2C_ISR(I2C1_BASE) & I2C_ISR_BUSY) && timeout--)
        ;

    I2C_CR2(I2C1_BASE) = ((uint32_t)addr7_w) | ((uint32_t)2 << 16) | I2C_CR2_START;

    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXE) && timeout--)
        ;
    if (timeout == 0)
        return 0xFFFF;
    I2C_TXDR(I2C1_BASE) = reg_hi;

    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXE) && timeout--)
        ;
    if (timeout == 0)
        return 0xFFFF;
    I2C_TXDR(I2C1_BASE) = reg_lo;

    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TC) && timeout--)
        ;

    /* Read 2 bytes */
    I2C_CR2(I2C1_BASE) = ((uint32_t)addr7_r) | ((uint32_t)2 << 16) |
                         I2C_CR2_START | I2C_CR2_RD_WRN;

    uint8_t hi = 0, lo = 0;
    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_RXNE) && timeout--)
        ;
    if (timeout == 0)
        return 0xFFFF;
    hi = (uint8_t)I2C_RXDR(I2C1_BASE);

    timeout = 10000;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_RXNE) && timeout--)
        ;
    lo = (uint8_t)I2C_RXDR(I2C1_BASE);

    I2C_CR2(I2C1_BASE) |= I2C_CR2_NACK | I2C_CR2_STOP;

    return ((uint16_t)hi << 8) | lo;
}

/* ---- CSI-2 Rx (TC358746) initialization ---- */

int bridge_init_csi_rx(const csi_config_t *cfg)
{
    if (!cfg)
        return -1;

    /* Verify chip ID */
    uint16_t id = tc358746_read16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_CHIP_ID);
    if (id != TC358746_CHIP_ID_VAL)
        return -2;

    /* Soft reset */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_RESET, 0x0001);
    for (volatile int i = 0; i < 10000; i++)
        ;
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_RESET, 0x0000);

    /* Configure PLL for desired lane speed */
    /* Simplified: actual calculation depends on input pixel clock and
       desired MIPI output lane speed. We use a lookup for common rates. */
    uint16_t pll_val = 0;
    if (cfg->lane_speed_mbps <= 500)
        pll_val = 0x0020;  /* divider config for <= 500 Mbps/lane */
    else if (cfg->lane_speed_mbps <= 1000)
        pll_val = 0x0040;  /* for <= 1000 Mbps/lane */
    else
        pll_val = 0x0060;  /* for <= 1500 Mbps/lane */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_PLL, pll_val);

    /* Enable lanes */
    uint16_t lane_en = (cfg->num_lanes == 2) ? 0x0003 : 0x0001;
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_LANE_EN, lane_en);

    /* D-PHY configuration */
    uint16_t dphy = (uint16_t)(cfg->lane_speed_mbps / 100);  /* encode HS freq */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_DPHY_CFG, dphy);

    /* Set frame dimensions */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_WIDTH, cfg->width);
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_HEIGHT, cfg->height);

    /* Data format */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_DATA_FMT,
                     (uint16_t)cfg->pixel_format);

    /* Clock control — enable CSI-2 output */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_CLK_CTRL, 0x0001);

    /* Enable interrupts for link status */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_INT_MASK, 0x000F);

    return 0;
}

/* ---- CSI-2 Tx (TC358748) initialization ---- */

int bridge_init_csi_tx(const csi_config_t *cfg)
{
    if (!cfg)
        return -1;

    /* Verify chip ID */
    uint16_t id = tc358746_read16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_CHIP_ID);
    if (id != TC358748_CHIP_ID_VAL)
        return -2;

    /* Soft reset */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_RESET, 0x0001);
    for (volatile int i = 0; i < 10000; i++)
        ;
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_RESET, 0x0000);

    /* PLL for output lane speed — same encoding as Rx */
    uint16_t pll_val = 0;
    if (cfg->lane_speed_mbps <= 500)
        pll_val = 0x0020;
    else if (cfg->lane_speed_mbps <= 1000)
        pll_val = 0x0040;
    else
        pll_val = 0x0060;
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_PLL, pll_val);

    /* Lane enable */
    uint16_t lane_en = (cfg->num_lanes == 2) ? 0x0003 : 0x0001;
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_LANE_EN, lane_en);

    /* D-PHY */
    uint16_t dphy = (uint16_t)(cfg->lane_speed_mbps / 100);
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_DPHY_CFG, dphy);

    /* Dimensions */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_WIDTH, cfg->width);
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_HEIGHT, cfg->height);

    /* Data format */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_DATA_FMT,
                     (uint16_t)cfg->pixel_format);

    /* CSI-Tx control — enable continuous clock, HS mode */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_CSITX_CTRL, 0x0003);

    /* Enable clock output */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_CLK_CTRL, 0x0001);

    return 0;
}

/* ---- DSI Rx (ADV7480) initialization ---- */

int bridge_init_dsi_rx(const dsi_config_t *cfg)
{
    if (!cfg)
        return -1;

    /* Verify chip ID (8-bit registers) */
    uint8_t id_hi, id_lo;
    if (bridge_i2c_read(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_CHIP_ID_HI, &id_hi, 1))
        return -2;
    if (bridge_i2c_read(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_CHIP_ID_LO, &id_lo, 1))
        return -3;
    if (id_hi != ADV7480_CHIP_ID_HI_VAL || id_lo != ADV7480_CHIP_ID_LO_VAL)
        return -4;

    /* Power control — power up DSI section */
    uint8_t pwr = 0x00;  /* all sections powered */
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_PWR_CTRL, &pwr, 1);

    /* DSI reset */
    uint8_t reset_val = 0x01;
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_RESET, &reset_val, 1);
    for (volatile int i = 0; i < 5000; i++)
        ;
    reset_val = 0x00;
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_RESET, &reset_val, 1);

    /* Configure DSI lanes */
    uint8_t lanes = cfg->num_lanes - 1;  /* 0=1-lane, 1=2-lane */
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_LANES, &lanes, 1);

    /* Format */
    uint8_t fmt = cfg->pixel_format;
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_FMT, &fmt, 1);

    /* Width (16-bit, split into two registers) */
    uint8_t w_h = (uint8_t)(cfg->width >> 8);
    uint8_t w_l = (uint8_t)(cfg->width & 0xFF);
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_WIDTH_H, &w_h, 1);
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_WIDTH_L, &w_l, 1);

    /* Height */
    uint8_t h_h = (uint8_t)(cfg->height >> 8);
    uint8_t h_l = (uint8_t)(cfg->height & 0xFF);
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_HEIGHT_H, &h_h, 1);
    bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_DSI_HEIGHT_L, &h_l, 1);

    return 0;
}

/* ---- DSI Tx (TC358762) initialization ---- */

int bridge_init_dsi_tx(const dsi_config_t *cfg)
{
    if (!cfg)
        return -1;

    /* Verify chip ID */
    uint16_t id = tc358746_read16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_CHIP_ID);
    if (id != TC358762_CHIP_ID_VAL)
        return -2;

    /* Soft reset */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_RESET, 0x0001);
    for (volatile int i = 0; i < 10000; i++)
        ;
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_RESET, 0x0000);

    /* DSI control: video mode, continuous clock */
    uint16_t dsi_ctrl = 0x0001;  /* video mode enable */
    if (cfg->video_mode)
        dsi_ctrl |= 0x0002;      /* burst mode */
    if (cfg->dcs_enabled)
        dsi_ctrl |= 0x0004;      /* DCS commands */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_DSI_CTRL, dsi_ctrl);

    /* Lanes */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_DSI_LANES,
                     (uint16_t)(cfg->num_lanes - 1));

    /* DSI clock frequency */
    uint16_t dsi_clk = (uint16_t)(cfg->lane_speed_mbps / 10);
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_DSI_CLK, dsi_clk);

    /* Parallel interface configuration */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_PARALLEL, 0x0001);

    /* Data format */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_DATA_FMT,
                     (uint16_t)cfg->pixel_format);

    /* Dimensions */
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_WIDTH, cfg->width);
    tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_HEIGHT, cfg->height);

    return 0;
}

/* ---- Link status ---- */

uint8_t bridge_csi_rx_link_up(void)
{
    uint16_t status = tc358746_read16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_STATUS);
    return (status & 0x0001) ? 1 : 0;
}

uint8_t bridge_csi_tx_link_up(void)
{
    uint16_t status = tc358746_read16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_STATUS);
    return (status & 0x0001) ? 1 : 0;
}

uint8_t bridge_dsi_rx_link_up(void)
{
    uint8_t status;
    if (bridge_i2c_read(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_STATUS, &status, 1))
        return 0;
    return (status & 0x01) ? 1 : 0;
}

uint8_t bridge_dsi_tx_link_up(void)
{
    uint16_t status = tc358746_read16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_STATUS);
    return (status & 0x0001) ? 1 : 0;
}

/* ---- Power control ---- */

void bridge_power_down(bridge_id_t id)
{
    switch (id) {
    case BRIDGE_CSI_RX:
        tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_CLK_CTRL, 0x0000);
        tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_RESET, 0x0002);
        break;
    case BRIDGE_CSI_TX:
        tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_CLK_CTRL, 0x0000);
        tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_RESET, 0x0002);
        break;
    case BRIDGE_DSI_RX: {
        uint8_t pwr = 0x03;  /* power down DSI section */
        bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_PWR_CTRL, &pwr, 1);
        break;
    }
    case BRIDGE_DSI_TX:
        tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_RESET, 0x0002);
        break;
    }
}

void bridge_power_up(bridge_id_t id)
{
    switch (id) {
    case BRIDGE_CSI_RX:
        tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_RESET, 0x0000);
        tc358746_write16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_CLK_CTRL, 0x0001);
        break;
    case BRIDGE_CSI_TX:
        tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_RESET, 0x0000);
        tc358746_write16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_CLK_CTRL, 0x0001);
        break;
    case BRIDGE_DSI_RX: {
        uint8_t pwr = 0x00;
        bridge_i2c_write(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_PWR_CTRL, &pwr, 1);
        break;
    }
    case BRIDGE_DSI_TX:
        tc358746_write16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_RESET, 0x0000);
        break;
    }
}

/* ---- Auto-detect ----
 * Try common CSI-2 configurations and see which one establishes a link.
 */

int bridge_auto_detect_csi(csi_config_t *cfg)
{
    if (!cfg)
        return -1;

    /* Common configurations to try */
    static const struct {
        uint8_t lanes;
        uint32_t speed;
        uint16_t width;
        uint16_t height;
        uint8_t  fmt;
    } presets[] = {
        { 2, 1000, 1920, 1080, FMT_RAW10 },  /* Full HD camera */
        { 2,  800, 1280,  720, FMT_RAW10 },  /* 720p camera     */
        { 1,  500,  640,  480, FMT_YUV422 }, /* VGA YUV         */
        { 1,  400,  320,  240, FMT_RAW8 },   /* QVGA raw        */
        { 2, 1500, 3840, 2160, FMT_RAW10 },  /* 4K camera       */
        { 1,  800, 1600, 1200, FMT_RAW10 },  /* 2MP camera      */
        { 2,  600, 1640, 1232, FMT_RAW10 },  /* RPi camera v2    */
        { 1,  300,  640,  480, FMT_RAW8 },   /* low-res raw     */
    };

    for (int i = 0; i < (int)(sizeof(presets) / sizeof(presets[0])); i++) {
        cfg->num_lanes = presets[i].lanes;
        cfg->lane_speed_mbps = presets[i].speed;
        cfg->width = presets[i].width;
        cfg->height = presets[i].height;
        cfg->pixel_format = presets[i].fmt;
        cfg->pixel_clock_hz = (uint32_t)presets[i].width * presets[i].height * 60;

        if (bridge_init_csi_rx(cfg) == 0) {
            /* Wait a bit for link to establish */
            for (volatile int j = 0; j < 100000; j++)
                ;
            if (bridge_csi_rx_link_up())
                return 0;  /* detected! */
        }
    }

    return -2;  /* no configuration worked */
}

int bridge_auto_detect_dsi(dsi_config_t *cfg)
{
    if (!cfg)
        return -1;

    static const struct {
        uint8_t lanes;
        uint32_t speed;
        uint16_t width;
        uint16_t height;
        uint8_t  fmt;
        uint8_t  video_mode;
    } presets[] = {
        { 2, 1000, 1080, 1920, FMT_RGB888, 1 },  /* phone display portrait */
        { 2,  800, 1080, 2400, FMT_RGB888, 1 },  /* tall phone display     */
        { 4, 1500, 2560, 1440, FMT_RGB888, 1 },  /* QHD tablet             */
        { 2,  500,  720, 1280, FMT_RGB888, 1 },  /* mid-range display      */
        { 1,  400,  480,  854, FMT_RGB888, 0 },  /* small display, cmd mode */
        { 2,  800, 1440, 2560, FMT_RGB888, 1 },  /* tablet portrait        */
    };

    for (int i = 0; i < (int)(sizeof(presets) / sizeof(presets[0])); i++) {
        cfg->num_lanes = presets[i].lanes;
        cfg->lane_speed_mbps = presets[i].speed;
        cfg->width = presets[i].width;
        cfg->height = presets[i].height;
        cfg->pixel_format = presets[i].fmt;
        cfg->video_mode = presets[i].video_mode;
        cfg->dcs_enabled = 1;

        if (bridge_init_dsi_rx(cfg) == 0) {
            for (volatile int j = 0; j < 100000; j++)
                ;
            if (bridge_dsi_rx_link_up())
                return 0;
        }
    }

    return -2;
}

/* ---- Diagnostic register dump ---- */

void bridge_dump_regs(bridge_id_t id)
{
    /* In a real implementation, read all registers and send to debug UART.
       For now, just read the status register. */
    uint16_t status = 0;
    switch (id) {
    case BRIDGE_CSI_RX:
        status = tc358746_read16(BRIDGE_I2C_ADDR_TC358746, TC358746_REG_STATUS);
        break;
    case BRIDGE_CSI_TX:
        status = tc358746_read16(BRIDGE_I2C_ADDR_TC358748, TC358746_REG_STATUS);
        break;
    case BRIDGE_DSI_RX: {
        uint8_t s;
        bridge_i2c_read(BRIDGE_I2C_ADDR_ADV7480, ADV7480_REG_STATUS, &s, 1);
        status = s;
        break;
    }
    case BRIDGE_DSI_TX:
        status = tc358746_read16(BRIDGE_I2C_ADDR_TC358762, TC358762_REG_STATUS);
        break;
    }
    (void)status;  /* would send via debug UART */
}

/* ---- End of mipi_bridge.c ----
 * Author: jayis1
 */