/*
 * clock_glitch.c — Clock glitch output stage driver
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls the clock glitch output stage via a TI TS3A5018 4:1 mux.
 * The mux selects between the target's normal clock, a forced high,
 * a forced low, and the onboard 100 MHz oscillator. By rapidly
 * switching the mux select lines, the FPGA can inject or suppress
 * individual clock edges.
 *
 * The MCU configures the mux default state and the onboard oscillator
 * frequency (via I2C). The actual edge-precise glitching is performed
 * by the FPGA which controls the mux select lines with sub-ns timing.
 */

#include "clock_glitch.h"
#include "../registers.h"
#include "../board.h"

/* ---- SiT9102 I2C oscillator address -------------------------------- */

#define SIT9102_I2C_ADDR  0x60  /* 7-bit address */

/* Onboard oscillator default frequency: 100 MHz */
#define DEFAULT_OSC_FREQ_HZ 100000000UL

static uint8_t  g_current_shape;
static uint32_t g_current_cycle_offset;
static uint8_t  g_current_src = CLOCK_SRC_PASSTHROUGH;

/* ---- I2C helper for SiT9102 oscillator ----------------------------- */

static void i2c_wait_idle(void)
{
    while (!(I2C1->ISR & BIT(6))) /* wait for TC (transfer complete) */
        ;
}

static void sit9102_write_reg(uint8_t reg, uint32_t value)
{
    /*
     * Write a 4-byte register to the SiT9102 oscillator.
     * This sets the output frequency.
     * In a real implementation, the register map and frequency
     * calculation would be per the SiT9102 datasheet.
     */
    I2C1->CR2 = (SIT9102_I2C_ADDR << 1)             /* address + write */
              | (5U << I2C_CR2_NBYTES_SHIFT)         /* 5 bytes: reg + 4 data */
              | I2C_CR2_START;

    /* Send register address */
    while (!(I2C1->ISR & I2C_ISR_TXE))
        ;
    I2C1->TXDR = reg;

    /* Send 4 data bytes */
    for (int i = 0; i < 4; i++) {
        while (!(I2C1->ISR & I2C_ISR_TXE))
            ;
        I2C1->TXDR = (value >> (i * 8)) & 0xFF;
    }

    i2c_wait_idle();
    I2C1->CR2 |= I2C_CR2_STOP;
}

/* ---- Mux control --------------------------------------------------- */

static void clock_mux_select(uint8_t src)
{
    /* TS3A5018 select pins: SEL0 = PB1, SEL1 = PB2 */
    if (src & 0x01)
        GPIOB->BSRR = BIT(CLOCK_MUX_SEL0);
    else
        GPIOB->BRR = BIT(CLOCK_MUX_SEL0);

    if (src & 0x02)
        GPIOB->BSRR = BIT(CLOCK_MUX_SEL1);
    else
        GPIOB->BRR = BIT(CLOCK_MUX_SEL1);

    g_current_src = src;
}

/* ---- Initialization ------------------------------------------------ */

void clock_glitch_init(void)
{
    /*
     * Configure clock mux select pins as outputs.
     * The mux select lines are shared with the FPGA — in normal
     * operation the FPGA drives them, but the MCU can override
     * for configuration.
     */
    gpio_config(GPIOB, CLOCK_MUX_SEL0, GPIO_MODE_OUTPUT, GPIO_OSPEED_VHIGH,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOB, CLOCK_MUX_SEL1, GPIO_MODE_OUTPUT, GPIO_OSPEED_VHIGH,
                GPIO_PUPD_NONE, 0);

    /* Default: passthrough (target's own clock) */
    clock_mux_select(CLOCK_SRC_PASSTHROUGH);

    /* Configure onboard oscillator to 100 MHz */
    /* (In practice, the SiT9102 is factory-programmed; this is for
     * runtime frequency adjustment if needed.) */
    /* sit9102_write_reg(0x00, DEFAULT_OSC_FREQ_HZ); */

    g_current_shape = CLOCK_GLITCH_EXTRA_EDGE;
    g_current_cycle_offset = 0;
}

/* ---- Configure glitch parameters ----------------------------------- */

void clock_glitch_configure(uint8_t shape, uint32_t cycle_offset,
                            uint32_t width_ns)
{
    g_current_shape = shape;
    g_current_cycle_offset = cycle_offset;

    /*
     * The clock glitch shape and timing are communicated to the FPGA
     * via the glitch control registers. The FPGA then drives the mux
     * select lines at the precise cycle to inject/suppress the edge.
     *
     * Register programming:
     *  - GLITCH_SHAPE: 0 = extra edge, 1 = suppress
     *  - GLITCH_WIDTH: width in ns (for the injected pulse)
     *  - CLOCK_CYCLE_OFFSET: position in clock cycles from trigger
     */
    uint8_t shape_data = shape;
    fpga_write_reg(FPGA_REG_GLITCH_SHAPE, &shape_data, 1);

    uint8_t width_bytes[4];
    width_bytes[0] = (width_ns >> 0) & 0xFF;
    width_bytes[1] = (width_ns >> 8) & 0xFF;
    width_bytes[2] = (width_ns >> 16) & 0xFF;
    width_bytes[3] = (width_ns >> 24) & 0xFF;
    fpga_write_reg(FPGA_REG_GLITCH_WIDTH, width_bytes, 4);

    (void)cycle_offset; /* also sent to FPGA as part of trigger offset */
}

/* ---- Manual mux control (for debugging) ---------------------------- */

void clock_glitch_passthrough(void)
{
    clock_mux_select(CLOCK_SRC_PASSTHROUGH);
}

void clock_glitch_inject_edge(void)
{
    /* Briefly switch to forced high then back to passthrough.
     * This injects an extra rising edge. In practice the FPGA does
     * this with sub-ns precision; this manual version is for testing. */
    clock_mux_select(CLOCK_SRC_FORCED_HIGH);
    /* tiny delay (~5 ns) */
    for (volatile int i = 0; i < 1; i++)
        ;
    clock_mux_select(CLOCK_SRC_PASSTHROUGH);
}

void clock_glitch_suppress_edge(void)
{
    /* Briefly switch to forced low to suppress a rising edge */
    clock_mux_select(CLOCK_SRC_FORCED_LOW);
    for (volatile int i = 0; i < 1; i++)
        ;
    clock_mux_select(CLOCK_SRC_PASSTHROUGH);
}

/* ---- End of file --------------------------------------------------- */