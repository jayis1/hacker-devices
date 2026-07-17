/*
 * pd_phy.c — USB-C Power Delivery PHY (FUSB302B) driver for CC-Stiletto.
 *
 * Implements I2C register access and the BMC FIFO RX/TX path for the FUSB302B
 * USB Type-C PD transceiver. Two instances are created in main.c — one for the
 * source side and one for the sink side — to enable true in-line MITM.
 *
 * The I2C routines use a small bit-banged-style poll loop against the STM32 I2C
 * registers defined in registers.h. No interrupts are used in this path so that
 * glitch timing stays deterministic.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#include "pd_phy.h"
#include "board.h"
#include <string.h>

/* ---- I2C primitives ------------------------------------------------------ */

static void i2c_delay_us(uint32_t us)
{
    /* crude cycle counter delay at 170 MHz; assumes DWT cycle counter is running */
    extern volatile uint32_t g_cycles;     /* incremented by SysTick */
    uint32_t start = g_cycles;
    uint32_t ticks = us * (SYSCLK_FREQ_HZ / 1000000u);
    while ((g_cycles - start) < ticks) { /* spin */ }
}

/* Wait while BUSY flag is set on a given I2C port */
static int i2c_wait_busy(i2c_reg_t *i2c)
{
    uint32_t to = 100000u;
    while (i2c->ISR & I2C_ISR_BUSY) {
        if (--to == 0) return -1;
    }
    return 0;
}

/* Configure the I2C peripheral: standard mode 100 kHz, 7-bit addressing.
 * The CC PHY only needs 100 kHz; we don't push faster to avoid signal-integrity
 * glitches on the isolator. */
static void i2c_setup(i2c_reg_t *i2c)
{
    i2c->CR1 = 0;
    /* TIMINGR: PRESC=3, SCLL=0x3F, SCLH=0x3F, SDADEL=0x2, SCLDEL=0x4 — ~100 kHz */
    i2c->TIMINGR = (3u << 28) | (0x3Fu << 8) | (0x3Fu << 0) | (0x2u << 20) | (0x4u << 16);
    i2c->OAR1 = 0;
    i2c->CR1 = I2C_CR1_PE;          /* peripheral enable */
    i2c_delay_us(50);
}

/* Issue START + addr<<1 + STOP after n bytes; used for both reads and writes. */
static int i2c_xfer(i2c_reg_t *i2c, uint8_t addr8, bool write,
                    uint8_t *buf, uint8_t n)
{
    if (i2c_wait_busy(i2c)) return -1;

    uint32_t cr2 = ((uint32_t)addr8 << 1) | ((uint32_t)n << 16);
    if (!write) cr2 |= I2C_CR2_NACK;        /* reads NACK the last byte */
    cr2 |= I2C_CR2_AUTOEND;                 /* auto STOP when NBYTES done */
    cr2 |= I2C_CR2_START;
    i2c->CR2 = cr2;

    uint32_t to;
    if (write) {
        for (uint8_t i = 0; i < n; i++) {
            to = 50000u;
            while (!(i2c->ISR & I2C_ISR_TXIS)) {
                if (i2c->ISR & I2C_ISR_NACKF) { i2c->ICR = I2C_ISR_NACKF; return -2; }
                if (--to == 0) return -3;
            }
            i2c->TXDR = buf[i];
        }
        to = 50000u;
        while (!(i2c->ISR & I2C_ISR_TC) && !(i2c->ISR & I2C_ISR_STOPF)) {
            if (--to == 0) return -4;
        }
    } else {
        for (uint8_t i = 0; i < n; i++) {
            to = 50000u;
            while (!(i2c->ISR & I2C_ISR_RXNE)) {
                if (--to == 0) return -5;
            }
            buf[i] = (uint8_t)i2c->RXDR;
        }
        to = 50000u;
        while (!(i2c->ISR & I2C_ISR_STOPF)) { if (--to == 0) break; }
    }
    i2c->ICR = I2C_ISR_STOPF | I2C_ISR_NACKF;
    return n;
}

/* ---- FUSB302B register helpers ------------------------------------------- */

int fusb_write(pd_phy_t *p, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_xfer(p->i2c, p->addr_w, true, buf, 2) == 2 ? 0 : -1;
}

int fusb_writes(pd_phy_t *p, uint8_t reg, const uint8_t *buf, uint8_t n)
{
    /* FUSB302B auto-increments the register pointer for consecutive writes */
    uint8_t tmp[16];
    if (n > 15) return -1;
    tmp[0] = reg;
    memcpy(&tmp[1], buf, n);
    return i2c_xfer(p->i2c, p->addr_w, true, tmp, n + 1) == (n + 1) ? 0 : -1;
}

int fusb_read(pd_phy_t *p, uint8_t reg, uint8_t *val)
{
    uint8_t cmd = reg;
    if (i2c_xfer(p->i2c, p->addr_w, true, &cmd, 1) != 1) return -1;
    if (i2c_xfer(p->i2c, p->addr_r, false, val, 1) != 1) return -1;
    return 0;
}

int fusb_reads(pd_phy_t *p, uint8_t reg, uint8_t *buf, uint8_t n)
{
    uint8_t cmd = reg;
    if (i2c_xfer(p->i2c, p->addr_w, true, &cmd, 1) != 1) return -1;
    if (i2c_xfer(p->i2c, p->addr_r, false, buf, n) != (int)n) return -1;
    return 0;
}

/* ---- Driver entry points ------------------------------------------------- */

void pd_phy_init(pd_phy_t *p)
{
    i2c_setup(p->i2c);

    /* Read device ID to confirm the FUSB302B is alive */
    if (!pd_phy_detect(p)) return;

    /* Power on + reset the PHY */
    fusb_write(p, 0x00u, 0x01u);            /* SWRESET */
    i2c_delay_us(2000);

    /* Default: power on, no auto GoodCRC yet (set per role) */
    fusb_write(p, FUSB_CONTROL0, 0x40u);     /* enable PD, host mode */
    fusb_write(p, FUSB_CONTROL1, 0x01u);     /* default spec rev 2.0 */
    fusb_write(p, FUSB_CONTROL3, 0x06u);     /* auto-retry 3, flush FIFOs */
    fusb_write(p, FUSB_CONTROL4, 0x00u);

    /* Start measuring CC to detect orientation */
    pd_phy_detect_cc(p);
}

bool pd_phy_detect(pd_phy_t *p)
{
    uint8_t id = 0;
    if (fusb_read(p, FUSB_DEVICE_ID, &id)) return false;
    /* FUSB302B returns 0x80 in upper nibble (product id), 0x0x in lower (rev) */
    return (id & 0x80u) != 0;
}

int pd_phy_detect_cc(pd_phy_t *p)
{
    /* Toggle MEASURE on CC1 then CC2; whichever returns a higher BC_LVL indicates
     * the active CC line. */
    fusb_write(p, FUSB_SWITCHES0, SW0_CC1_EN);     /* enable CC1 measurement */
    fusb_write(p, FUSB_MEASURE,  0x50u | 0x01u);   /* MDAC=0x50, meas CC1 */
    i2c_delay_us(250);
    uint8_t s0a = 0; fusb_read(p, FUSB_STATUS0, &s0a);

    fusb_write(p, FUSB_SWITCHES0, SW0_CC2_EN);
    fusb_write(p, FUSB_MEASURE,  0x50u | 0x02u);   /* meas CC2 */
    i2c_delay_us(250);
    uint8_t s0b = 0; fusb_read(p, FUSB_STATUS0, &s0b);

    bool cc1 = (s0a & 0x04u) != 0;   /* BC_LVL bit for CC1 */
    bool cc2 = (s0b & 0x04u) != 0;   /* BC_LVL bit for CC2 */

    if (cc1 && !cc2) {
        p->orient = CC1_ACTIVE;
        fusb_write(p, FUSB_SWITCHES0, SW0_CC1_EN);
        return CC1_ACTIVE;
    } else if (cc2 && !cc1) {
        p->orient = CC2_ACTIVE;
        fusb_write(p, FUSB_SWITCHES0, SW0_CC2_EN);
        return CC2_ACTIVE;
    }
    p->orient = CC_NONE;
    return CC_NONE;
}

int pd_phy_set_role(pd_phy_t *p, pd_power_role_t pr, pd_data_role_t dr)
{
    p->power_role = pr;
    p->data_role  = dr;

    uint8_t sw1 = 0;
    /* Spec rev 2.0 — bits 0..1 */
    sw1 |= SW1_SPECREV1;           /* spec rev = 2 -> bits = 0b10 */
    sw1 |= SW1_AUTO_CRC;           /* auto-send GoodCRC on every RX */

    /* Transmit on whichever CC is active */
    if (p->orient == CC1_ACTIVE) sw1 |= SW1_TXCC1;
    else if (p->orient == CC2_ACTIVE) sw1 |= SW1_TXCC2;

    fusb_write(p, FUSB_SWITCHES1, sw1);

    /* CONTROL2 sets power role for SOP framing */
    uint8_t c2 = 0;
    if (pr == PD_POWER_SOURCE) c2 |= 0x20u;   /* SRC bit */
    if (dr == PD_ROLE_SOURCE)  c2 |= 0x40u;   /* data role DFP */
    fusb_write(p, FUSB_CONTROL2, c2);
    return 0;
}

int pd_phy_enable_vconn(pd_phy_t *p, bool en)
{
    uint8_t sw0 = 0;
    if (p->orient == CC1_ACTIVE) sw0 |= SW0_CC1_EN;
    else if (p->orient == CC2_ACTIVE) sw0 |= SW0_CC2_EN;
    if (en) {
        if (p->orient == CC1_ACTIVE) sw0 |= SW0_VCONN_CC2; /* VCONN on non-active CC */
        else if (p->orient == CC2_ACTIVE) sw0 |= SW0_VCONN_CC1;
    }
    return fusb_write(p, FUSB_SWITCHES0, sw0);
}

void pd_phy_flush_rx(pd_phy_t *p)
{
    fusb_write(p, FUSB_CONTROL1, 0x06u);     /* FLUSH_TX/RX */
    i2c_delay_us(50);
    fusb_write(p, FUSB_CONTROL1, 0x01u);
}

int pd_phy_send_hard_reset(pd_phy_t *p)
{
    return fusb_write(p, FUSB_CONTROL0, 0x20u);   /* SEND_HARD_RESET bit */
}

int pd_phy_send_bist(pd_phy_t *p, uint8_t bist_mode)
{
    /* Build a BIST message: header with type=BIST, cnt=1, plus 1 obj with mode */
    uint16_t hdr = 0x0040u;   /* BIST message type */
    uint32_t obj = (uint32_t)bist_mode & 0x0Fu;
    return pd_phy_send(p, SOP_SOP, hdr, &obj, 1);
}

/* Build the 16-bit PD header. msg_id is tracked per-SOP in the stack module. */
static uint16_t build_header(pd_phy_t *p, uint8_t type, uint8_t nobj,
                             bool is_control)
{
    /* Header layout (rev 2.0/3.0):
     *   bits 0..3   message type
     *   bit  4      rsvd (0)
     *   bit  5      data role (0=sink/UFP, 1=source/DFP)
     *   bits 6..7   spec rev (1=1.0, 2=2.0, 3=3.0)
     *   bit  8      power role / cable plug (for SOP': 0=SOP'/1=SOP'')
     *   bits 9..13  message id
     *   bits 14..15 object count (0 for control messages)
     */
    uint16_t h = (uint16_t)(type & 0x0Fu);
    h |= (p->data_role == PD_ROLE_SOURCE) ? (1u << 5) : 0u;
    h |= (2u << 6);                       /* spec rev 2.0 */
    h |= (p->power_role == PD_POWER_SOURCE) ? (1u << 8) : 0u;
    if (!is_control) h |= (uint16_t)(nobj & 0x07u) << 14;
    /* msg_id is filled in by the stack; leave 0 here, stack ORs it in */
    return h;
}

int pd_phy_send(pd_phy_t *p, uint8_t sop, const uint16_t hdr,
                const uint32_t *obj, uint8_t nobj)
{
    if (nobj > 7) return -1;
    uint8_t buf[32];
    uint8_t idx = 0;

    /* TX FIFO write sequence: SOP token, then header (LE), then each object (LE). */
    buf[idx++] = sop;
    buf[idx++] = FUSB_TXON;
    buf[idx++] = (uint8_t)(hdr & 0xFFu);
    buf[idx++] = (uint8_t)(hdr >> 8);
    for (uint8_t i = 0; i < nobj; i++) {
        buf[idx++] = (uint8_t)(obj[i]       & 0xFFu);
        buf[idx++] = (uint8_t)(obj[i] >> 8  & 0xFFu);
        buf[idx++] = (uint8_t)(obj[i] >> 16 & 0xFFu);
        buf[idx++] = (uint8_t)(obj[i] >> 24 & 0xFFu);
    }
    /* Append CRC placeholder — FUSB302B auto-appends CRC when AUTO_CRC is set. */
    int rc = fusb_writes(p, FUSB_FIFOS, buf, idx);
    if (rc) return rc;

    /* Trigger transmission: set TXON in CONTROL0 (bit 0). */
    fusb_write(p, FUSB_CONTROL0, 0x40u | 0x01u);
    i2c_delay_us(20);
    return 0;
}

int pd_phy_recv(pd_phy_t *p, pd_msg_t *out)
{
    uint8_t st0 = 0, st1 = 0;
    fusb_read(p, FUSB_STATUS0, &st0);
    fusb_read(p, FUSB_STATUS1, &st1);

    if (!(st1 & 0x20u)) return 0;        /* RX_EMPTY bit -> no message */

    /* Read the first few bytes of the RX FIFO: order set marker + SOP type */
    uint8_t rxb[32];
    uint8_t n = 3;
    if (fusb_reads(p, FUSB_FIFOS, rxb, n)) return -1;

    /* First byte after order set indicates SOP type */
    uint8_t tok = rxb[0];
    out->sop_type = (tok == FUSB_RXSOPX) ? 0 : (tok == FUSB_RXSOPXP) ? 1 : 2;

    /* Following two bytes are the header (LE) */
    uint16_t hdr = (uint16_t)rxb[1] | ((uint16_t)rxb[2] << 8);
    out->header = hdr;
    out->msg_id = (uint8_t)((hdr >> 9) & 0x1Fu);
    out->num_obj = (uint8_t)((hdr >> 14) & 0x07u);

    if (out->num_obj > 7) { pd_phy_flush_rx(p); return -2; }
    for (uint8_t i = 0; i < out->num_obj; i++) {
        uint8_t ob[4];
        if (fusb_reads(p, FUSB_FIFOS, ob, 4)) return -3;
        out->obj[i] = (uint32_t)ob[0] | ((uint32_t)ob[1] << 8)
                   | ((uint32_t)ob[2] << 16) | ((uint32_t)ob[3] << 24);
    }
    /* CRC bytes are appended by the PHY and consumed automatically when
     * AUTO_CRC is enabled — read+discard them. */
    uint8_t crc[4];
    if (out->num_obj == 0) fusb_reads(p, FUSB_FIFOS, crc, 4);

    /* Clear interrupt flags */
    fusb_write(p, 0x44u, 0xFFu);
    return 1;
}

/* Convenience: build + send using role-appropriate header */
int pd_phy_send_typed(pd_phy_t *p, uint8_t sop, uint8_t type,
                      const uint32_t *obj, uint8_t nobj, bool is_control)
{
    uint16_t h = build_header(p, type, nobj, is_control);
    return pd_phy_send(p, sop, h, obj, nobj);
}