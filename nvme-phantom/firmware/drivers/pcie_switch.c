/*
 * drivers/pcie_switch.c — Broadcom PEX8606 PCIe switch configuration driver
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * The PEX8606 is a 6-port PCIe Gen3 switch.  In NVMe-Phantom it is wired as:
 *   USP  (port 0)  <- host M.2 slot
 *   DSP0 (port 1)  -> target SSD M.2 socket
 *   DSP1 (port 2)  -> Lattice ECP5 FPGA (capture / modify / inject port)
 *
 * The switch's "port mirror / capture-port" feature copies all traffic between
 * USP and DSP0 to DSP1, so the FPGA can snoop without being in the critical
 * path.  When the FPGA wants to modify a TLP, it asserts a "modify override"
 * (vendor-specific register 0x0D40) which the switch honours on DSP0.
 *
 * This driver configures the switch over I2C1 at power-up and exposes helpers
 * to enable/disable port mirroring, inline-modify, and hot-plug event
 * injection.
 */

#include "../board.h"
#include "../registers.h"

/* ---- I2C1 low-level (bit-banged-ish via peripheral registers) ---------- */

static void i2c1_init(void)
{
    /* Enable I2C1 clock */
    RCC_APB1LENR |= RCC_APB1LENR_I2C1;
    /* PB6/PB7 AF4 */
    volatile uint32_t *gpiob_moder = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpiob_afrl  = (volatile uint32_t *)(GPIOB_BASE + GPIO_AFRL_OFF);
    *gpiob_moder &= ~((3U << (6*2)) | (3U << (7*2)));
    *gpiob_moder |=  ((2U << (6*2)) | (2U << (7*2)));   /* AF mode */
    *gpiob_afrl  &= ~((0xFU << (6*4)) | (0xFU << (7*4)));
    *gpiob_afrl  |=  ((4U  << (6*4)) | (4U  << (7*4))); /* AF4 = I2C1 */
    /* Configure I2C timing for 400 kHz from 125 MHz PCLK (simplified) */
    volatile uint32_t *i2c_cr1  = (volatile uint32_t *)(I2C1_BASE + I2C_CR1);
    volatile uint32_t *i2c_timingr = (volatile uint32_t *)(I2C1_BASE + 0x10);
    *i2c_cr1 = 0;                          /* disable during config */
    *i2c_timingr = 0x10B17DB5U;            /* 400 kHz, analog filter on */
    *i2c_cr1 = I2C_CR1_PE;                 /* enable peripheral */
}

static int i2c1_write(uint8_t addr, uint16_t reg, const uint8_t *data, uint16_t len)
{
    volatile uint32_t *cr1  = (volatile uint32_t *)(I2C1_BASE + I2C_CR1);
    volatile uint32_t *cr2  = (volatile uint32_t *)(I2C1_BASE + I2C_CR2);
    volatile uint32_t *isr  = (volatile uint32_t *)(I2C1_BASE + I2C_ISR);
    volatile uint32_t *txdr = (volatile uint32_t *)(I2C1_BASE + I2C_TXDR);
    volatile uint32_t *icr  = (volatile uint32_t *)(I2C1_BASE + I2C_ICR);
    uint32_t timeout = 0xFFFF;

    /* 2-byte register address + data bytes = NBYTES */
    uint16_t nbytes = 2 + len;
    *cr2 = ((uint32_t)addr << 1) | (nbytes << 16) | I2C_CR2_START;
    /* send register high, low, then data */
    while (!(*isr & I2C_ISR_TXE) && --timeout) { }
    *txdr = (reg >> 8) & 0xFF;
    while (!(*isr & I2C_ISR_TXE) && --timeout) { }
    *txdr = reg & 0xFF;
    for (uint16_t i = 0; i < len; i++) {
        while (!(*isr & I2C_ISR_TXE) && --timeout) { }
        *txdr = data[i];
    }
    while (!(*isr & I2C_ISR_TC) && --timeout) { }
    *cr2 |= I2C_CR2_STOP;
    *icr = 0x3FFF;                         /* clear all flags */
    return timeout ? 0 : -1;
}

static int i2c1_read(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len)
{
    volatile uint32_t *cr1  = (volatile uint32_t *)(I2C1_BASE + I2C_CR1);
    volatile uint32_t *cr2  = (volatile uint32_t *)(I2C1_BASE + I2C_CR2);
    volatile uint32_t *isr  = (volatile uint32_t *)(I2C1_BASE + I2C_ISR);
    volatile uint32_t *txdr = (volatile uint32_t *)(I2C1_BASE + I2C_TXDR);
    volatile uint32_t *rxdr = (volatile uint32_t *)(I2C1_BASE + I2C_RXDR);
    volatile uint32_t *icr  = (volatile uint32_t *)(I2C1_BASE + I2C_ICR);
    uint32_t timeout = 0xFFFF;

    /* write 2-byte reg addr (no stop) */
    *cr2 = ((uint32_t)addr << 1) | (2U << 16) | I2C_CR2_START;
    while (!(*isr & I2C_ISR_TXE) && --timeout) { }
    *txdr = (reg >> 8) & 0xFF;
    while (!(*isr & I2C_ISR_TXE) && --timeout) { }
    *txdr = reg & 0xFF;
    while (!(*isr & I2C_ISR_TC) && --timeout) { }

    /* repeated start, read N bytes */
    *cr2 = ((uint32_t)addr << 1) | 1U | (len << 16) | I2C_CR2_START | I2C_CR2_NACK | I2C_CR2_STOP;
    for (uint16_t i = 0; i < len; i++) {
        timeout = 0xFFFF;
        while (!(*isr & I2C_ISR_RXNE) && --timeout) { }
        buf[i] = (uint8_t)*rxdr;
    }
    *icr = 0x3FFF;
    return timeout ? 0 : -1;
}

/* ---- PEX8606 register helpers ------------------------------------------ */

int pex_write(uint16_t reg, uint8_t val)
{
    return i2c1_write(PEX8606_I2C_ADDR, reg, &val, 1);
}

int pex_write32(uint16_t reg, uint32_t val)
{
    uint8_t buf[4] = {
        (uint8_t)(val & 0xFF),
        (uint8_t)((val >> 8) & 0xFF),
        (uint8_t)((val >> 16) & 0xFF),
        (uint8_t)((val >> 24) & 0xFF),
    };
    return i2c1_write(PEX8606_I2C_ADDR, reg, buf, 4);
}

int pex_read32(uint16_t reg, uint32_t *out)
{
    uint8_t buf[4];
    int rc = i2c1_read(PEX8606_I2C_ADDR, reg, buf, 4);
    if (rc == 0) {
        *out = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
               ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    }
    return rc;
}

/* ---- Public API -------------------------------------------------------- */

int pcie_switch_init(void)
{
    i2c1_init();
    board_delay_ms(5);

    /* Verify the switch is present by reading the vendor ID register */
    uint32_t vid;
    if (pex_read32(PEX8606_REG_VID0, &vid) != 0) {
        return -1;                     /* I2C timeout — switch not responding */
    }
    if ((vid & 0xFFFF) != 0x11F8) {    /* Broadcom / PLX vendor ID */
        return -2;                     /* wrong device */
    }

    /* --- Configure lane widths --- */
    /* USP = x4 (Gen3), DSP0 = x4 (Gen3), DSP1 = x4 (Gen2, FPGA hard IP) */
    pex_write32(PEX8606_REG_LANESEL,
                (4U << 0)  |           /* USP width = 4 */
                (4U << 8)  |           /* DSP0 width = 4 */
                (4U << 16));           /* DSP1 width = 4 */

    /* --- Enable port mirror: copy USP<->DSP0 traffic to DSP1 --- */
    /* Bit 0 = enable mirror, bits 8:10 = source port pair (USP<->DSP0),
       bits 16:18 = destination port (DSP1). */
    pex_write32(PEX8606_REG_MIRROR,
                (1U << 0)  |           /* enable mirror */
                (0U << 8)  |           /* source pair = USP/DSP0 */
                (2U << 16));           /* dest = DSP1 (FPGA) */

    /* --- Disable inline TLP modify by default (safe mode) --- */
    pex_write32(PEX8606_REG_MODIFY, 0x00000000);

    /* --- Enable standard error reporting on all ports --- */
    pex_write32(PEX8606_REG_ERRCTL, 0x0000FFFF);

    g_state.switch_ready = 1;
    return 0;
}

int pcie_switch_enable_modify(int enable)
{
    /* When set, the FPGA can assert a modify-override on DSP0 to rewrite
     * a TLP in flight.  Only enable in MITM / Spoof / DMA modes. */
    return pex_write32(PEX8606_REG_MODIFY, enable ? 0x00000001 : 0x00000000);
}

int pcie_switch_inject_hotplug(uint8_t event)
{
    /* Emulate surprise-removal (event=0) or replug (event=1) on DSP0.
     * This toggles the PEX8606's per-port link-disable bit, which causes
     * the host to see a surprise-removal or a hot-plug re-enumeration. */
    if (event == 0) {
        /* surprise-removal: disable DSP0 link, set surprise-remove flag */
        pex_write32(PEX8606_REG_PORTCTRL + 0x10 * PEX8606_DSP0_SSD, 0x00000004);
    } else {
        /* replug: re-enable link, force retrain */
        pex_write32(PEX8606_REG_PORTCTRL + 0x10 * PEX8606_DSP0_SSD, 0x00000001);
        board_delay_ms(20);
        pex_write32(PEX8606_REG_PORTCTRL + 0x10 * PEX8606_DSP0_SSD, 0x00000000);
    }
    return 0;
}

link_state_t pcie_switch_link_state(void)
{
    uint32_t portctrl;
    if (pex_read32(PEX8606_REG_PORTCTRL, &portctrl) != 0) {
        return LINK_DOWN;
    }
    if (!(portctrl & 0x00000080)) {        /* LINKUP bit */
        return LINK_DOWN;
    }
    uint32_t gen = (portctrl >> 8) & 0x3;  /* link gen field */
    return (link_state_t)gen;
}

uint8_t pcie_switch_link_width(void)
{
    uint32_t lanesel;
    if (pex_read32(PEX8606_REG_LANESEL, &lanesel) != 0) {
        return 0;
    }
    return (uint8_t)(lanesel & 0xF);       /* USP width */
}

/* ---- Safe-mode: disable all injection / modify ------------------------- */

void pcie_switch_safe_mode(void)
{
    pex_write32(PEX8606_REG_MODIFY, 0x00000000);   /* disable modify override */
    g_state.mode = MODE_SAFE;
}