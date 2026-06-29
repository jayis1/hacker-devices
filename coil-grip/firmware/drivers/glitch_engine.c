/*
 * glitch_engine.c — Contactless power-glitch injection for CoilGrip
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * The iCE40-UP5K FPGA holds the glitch sequencer so timing is cycle-
 * accurate to the 12 MHz Qi-derived clock. The MCU arms the FPGA over
 * SPI1 with a parameter block, then either auto-fires on a chosen
 * trigger or the MCU fires manually.
 *
 * The glitch itself is produced by gating the H-bridge enable pin
 * (PA1): when deasserted, the Qi TX stops transferring power and the
 * downstream target's rail droops; when re-asserted, power resumes.
 * The width and delay of the drop are what we tune.
 *
 * For the reference firmware (FPGA bitstream loaded externally) we
 * also implement a software fallback that bit-bangs PA1 with TIM2
 * providing the timing. The two paths share the same parameter struct.
 */

#include "board.h"
#include "registers.h"

/* FPGA SPI command opcodes (protocol defined by the CoilGrip bitstream) */
#define FPGA_CMD_ARM            0x01
#define FPGA_CMD_FIRE           0x02
#define FPGA_CMD_DISARM         0x03
#define FPGA_CMD_STATUS         0x04
#define FPGA_CMD_SET_PARAMS     0x10

/* FPGA status bits */
#define FPGA_ST_ARMED           0x01
#define FPGA_ST_FIRED            0x02
#define FPGA_ST_READY            0x04

static glitch_params_t g_params;
static bool g_armed = false;
static uint32_t g_fire_count = 0;
static bool g_use_fpga = false;  /* false = software fallback via TIM2 */

/* ---- SPI1 to iCE40 ----------------------------------------------------- */

static void spi1_cs_low(void)  { GPIOE->BSRR = BIT(PIN_PE1 + 16); }
static void spi1_cs_high(void) { GPIOE->BSRR = BIT(PIN_PE1); }

static uint8_t spi1_xfer(uint8_t tx)
{
    SPI1->CR1 = 0;
    SPI1->CFG1 = SPI_CFG1_DSIZE_8BIT;
    SPI1->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSOE;
    SPI1->CR1 |= SPI_CR1_SPE;
    while (!(SPI1->SR & SPI_SR_TXP)) { }
    SPI1->TXDR = tx;
    SPI1->CR1 |= SPI_CR1_CSTART;
    while (!(SPI1->SR & SPI_SR_RXP)) { }
    uint8_t rx = (uint8_t)SPI1->RXDR;
    while (!(SPI1->SR & SPI_SR_EOT)) { }
    SPI1->IFCR = SPI_IFCR_EOTC;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    return rx;
}

static bool fpga_present(void)
{
    /* Check the FPGA_DONE line (PE0 reads low when done is asserted in
     * our wiring; high when unconfigured). We treat a configured FPGA
     * as "use hardware path". For the reference build we always fall
     * back to the software path unless the bitstream has been loaded. */
    return !(GPIOE->IDR & BIT(PIN_PE0));
}

/* ---- FPGA path --------------------------------------------------------- */

static int fpga_set_params(const glitch_params_t *p)
{
    spi1_cs_low();
    spi1_xfer(FPGA_CMD_SET_PARAMS);
    /* 16-byte parameter block: trigger(1) + pad + delay(4) + width(4) +
     * repeat(4) + ramp(4 signed) — the FPGA packs the rest internally. */
    spi1_xfer((uint8_t)p->trigger);
    spi1_xfer(0);
    spi1_xfer((uint8_t)(p->delay_us));
    spi1_xfer((uint8_t)(p->delay_us >> 8));
    spi1_xfer((uint8_t)(p->delay_us >> 16));
    spi1_xfer((uint8_t)(p->width_us));
    spi1_xfer((uint8_t)(p->width_us >> 8));
    spi1_xfer((uint8_t)(p->width_us >> 16));
    spi1_xfer((uint8_t)(p->repeat));
    spi1_xfer((uint8_t)(p->repeat >> 8));
    spi1_xfer((uint8_t)(p->repeat >> 16));
    spi1_xfer((uint8_t)(p->repeat >> 24));
    spi1_xfer((uint8_t)(p->ramp_us));
    spi1_xfer((uint8_t)(p->ramp_us >> 8));
    spi1_xfer((uint8_t)(p->ramp_us >> 16));
    spi1_xfer((uint8_t)(p->ramp_us >> 24));
    spi1_cs_high();
    return 0;
}

static int fpga_arm(void)
{
    spi1_cs_low();
    spi1_xfer(FPGA_CMD_ARM);
    spi1_xfer(0x00);
    spi1_cs_high();
    return 0;
}

static int fpga_fire(void)
{
    spi1_cs_low();
    spi1_xfer(FPGA_CMD_FIRE);
    spi1_xfer(0x00);
    spi1_cs_high();
    return 0;
}

static int fpga_disarm(void)
{
    spi1_cs_low();
    spi1_xfer(FPGA_CMD_DISARM);
    spi1_xfer(0x00);
    spi1_cs_high();
    return 0;
}

static uint8_t fpga_status(void)
{
    spi1_cs_low();
    spi1_xfer(FPGA_CMD_STATUS);
    uint8_t st = spi1_xfer(0x00);
    spi1_cs_high();
    return st;
}

/* ---- Software fallback (TIM2 + PA1 bit-bang) --------------------------- */
/* Used when no FPGA bitstream is loaded. Precision is bounded by the
 * Cortex-M7 instruction timing (~2 ns/cycle at 480 MHz) and the TIM2
 * input clock (120 MHz → 8.33 ns/tick). Good enough for ≥1 µs glitches.
 *
 * The glitch is a low pulse on PA1 (H-bridge enable): low = power off. */

static void sw_glitch_once(uint32_t delay_us, uint32_t width_us)
{
    /* Delay: busy-wait scaled by HCLK. Width: same. */
    uint32_t delay_loops = (uint32_t)((float)delay_us * (float)HCLK_HZ / 12.0f);
    uint32_t width_loops = (uint32_t)((float)width_us * (float)HCLK_HZ / 12.0f);
    for (volatile uint32_t i = 0; i < delay_loops; i++) { __asm__ volatile("nop"); }
    GPIOA->BSRR = BIT(PIN_PA1 + 16);  /* PA1 low → power off */
    for (volatile uint32_t i = 0; i < width_loops; i++) { __asm__ volatile("nop"); }
    GPIOA->BSRR = BIT(PIN_PA1);        /* PA1 high → power on */
}

static int sw_fire(const glitch_params_t *p)
{
    /* Validate trigger source for the software path: only TIMER, QI_PKT
     * (coarsely), and EXT_GPIO are supported without the FPGA. */
    uint32_t count = p->repeat ? p->repeat : 1;
    uint32_t width = p->width_us;
    for (uint32_t i = 0; i < count; i++) {
        /* For TRIG_EXT_GPIO we wait for the chosen pin to go high. */
        if (p->trigger == TRIG_EXT_GPIO) {
            uint8_t pin = p->trig_pin;
            gpio_t *port = (pin < 16) ? GPIOA :
                          (pin < 32) ? GPIOB :
                          (pin < 48) ? GPIOC :
                          (pin < 64) ? GPIOD : GPIOE;
            uint8_t bit = pin & 15;
            while (!(port->IDR & BIT(bit))) { }
        }
        /* For TRIG_TIMER we just delay then fire. */
        sw_glitch_once(p->delay_us, width);
        width = (uint32_t)((int32_t)width + p->ramp_us);
        if (width < GLITCH_MIN_WIDTH_US) width = GLITCH_MIN_WIDTH_US;
        if (width > GLITCH_MAX_WIDTH_US) width = GLITCH_MAX_WIDTH_US;
        /* Inter-glitch gap (period) */
        if (p->period_ms) {
            for (volatile uint32_t d = 0; d < p->period_ms * 1000U * (HCLK_HZ / 12U) / 1000U; d++) { __asm__ volatile("nop"); }
        }
    }
    return 0;
}

/* ---- Public API -------------------------------------------------------- */

int glitch_engine_arm(const glitch_params_t *p)
{
    if (!p) return -1;
    if (p->width_us < GLITCH_MIN_WIDTH_US || p->width_us > GLITCH_MAX_WIDTH_US)
        return -1;
    g_params = *p;
    g_use_fpga = fpga_present();
    if (g_use_fpga) {
        fpga_set_params(p);
        fpga_arm();
    }
    g_armed = true;
    return 0;
}

int glitch_engine_fire(void)
{
    if (!g_armed) return -1;
    if (g_use_fpga) {
        fpga_fire();
        /* Wait until the FPGA reports fired */
        for (uint32_t i = 0; i < 1000000; i++) {
            if (fpga_status() & FPGA_ST_FIRED) break;
        }
    } else {
        sw_fire(&g_params);
    }
    g_fire_count++;
    return 0;
}

void glitch_engine_disarm(void)
{
    if (g_use_fpga) fpga_disarm();
    g_armed = false;
}

uint32_t glitch_engine_get_count(void) { return g_fire_count; }
bool glitch_engine_is_armed(void) { return g_armed; }
bool glitch_engine_uses_fpga(void) { return g_use_fpga; }