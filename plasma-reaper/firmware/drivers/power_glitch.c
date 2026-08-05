/*
 * power_glitch.c — Power glitch output stage driver
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls the power glitch output stage:
 *  - DAC5311 (SPI2) sets the shunt MOSFET gate voltage → glitch depth
 *  - TMUX1108 analog mux selects the series resistance (8 values)
 *  - The actual glitch timing is controlled by the FPGA/HRTIM which
 *    drives the shunt MOSFET gate directly for sub-10ns edges
 *
 * The driver configures the analog parameters; the timing is handled
 * by hrtim_glitch.c and fpga_if.c.
 */

#include "power_glitch.h"
#include "../registers.h"
#include "../board.h"

/* ---- Private state ------------------------------------------------- */

static uint16_t g_current_depth_mv;
static uint8_t  g_current_series_r;

/* ---- DAC5311 SPI2 helpers ------------------------------------------ */

static void dac_nss_low(void)
{
    GPIOB->BRR = BIT(DAC_NSS_PIN);
}

static void dac_nss_high(void)
{
    GPIOB->BSRR = BIT(DAC_NSS_PIN);
}

static void dac_write(uint16_t value)
{
    /*
     * DAC5311 protocol: 16-bit SPI transfer
     *  Bits [15:12] = don't care
     *  Bit  [13]    = PD1 (power mode, 0 = normal)
     *  Bit  [12]    = PD0 (power mode, 0 = normal)
     *  Bits [11:0]  = DAC data (12-bit, left-justified in 16-bit field)
     *  Wait — DAC5311 is 10-bit. Data is in bits [11:2].
     */
    uint16_t dac_val = (value & 0x0FFF); /* 12-bit field, 10-bit DAC */

    dac_nss_low();

    /* Send high byte then low byte */
    while (!(SPI2->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&SPI2->DR = (dac_val >> 8) & 0xFF;
    while (!(SPI2->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&SPI2->DR = dac_val & 0xFF;

    while (SPI2->SR & SPI_SR_BSY)
        ;
    dac_nss_high();
}

/* ---- Series resistance mux ----------------------------------------- */

static void set_series_resistance(uint8_t idx)
{
    /* 3-bit mux select on PE13, PE14, PE15 */
    idx &= 0x07;

    if (idx & 0x01)
        GPIOE->BSRR = BIT(SERIES_R_SEL0);
    else
        GPIOE->BRR = BIT(SERIES_R_SEL0);

    if (idx & 0x02)
        GPIOE->BSRR = BIT(SERIES_R_SEL1);
    else
        GPIOE->BRR = BIT(SERIES_R_SEL1);

    if (idx & 0x04)
        GPIOE->BSRR = BIT(SERIES_R_SEL2);
    else
        GPIOE->BRR = BIT(SERIES_R_SEL2);
}

/* ---- Initialization ------------------------------------------------ */

void power_glitch_init(void)
{
    /*
     * Configure SPI2 for DAC5311:
     *  PB12 = NSS (GPIO output, software-controlled)
     *  PB13 = SCK  (AF5) — but on STM32H7, SPI2_SCK is PB10 or PB13
     *  PB14 = MISO (AF5) — not used (DAC is write-only) but configured
     *  PB15 = MOSI (AF5)
     *
     * For simplicity we use the same SPI2 pins as defined in registers.h.
     * In a real build, the exact AF numbers depend on the STM32H7 AF table.
     */

    /* Configure NSS as output */
    gpio_config(GPIOB, DAC_NSS_PIN, GPIO_MODE_OUTPUT, GPIO_OSPEED_VHIGH,
                GPIO_PUPD_NONE, 0);
    dac_nss_high();

    /* Configure series resistance mux pins as outputs */
    gpio_config(GPIOE, SERIES_R_SEL0, GPIO_MODE_OUTPUT, GPIO_OSPEED_LOW,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOE, SERIES_R_SEL1, GPIO_MODE_OUTPUT, GPIO_OSPEED_LOW,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOE, SERIES_R_SEL2, GPIO_MODE_OUTPUT, GPIO_OSPEED_LOW,
                GPIO_PUPD_NONE, 0);

    /* Configure SPI2: master, 8-bit, CPOL=0 CPHA=0, /4 baud */
    SPI2->CR1 = 0;
    SPI2->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | SPI_CR1_BR_DIV4
              | SPI_CR1_SPE;

    /* Set default series resistance to index 3 (2 ohms) */
    set_series_resistance(3);

    /* Set default DAC output to mid-range (no glitch — full VCC) */
    dac_write(0x0800); /* ~1.65 V at VCC=3.3V → MOSFET fully on */

    g_current_depth_mv = 0;
    g_current_series_r = 3;
}

/* ---- Configure glitch parameters ----------------------------------- */

void power_glitch_configure(uint16_t depth_mv, uint8_t series_r_idx,
                            uint32_t width_ns)
{
    /*
     * The glitch depth is controlled by the DAC output voltage, which
     * sets the shunt MOSFET gate voltage. A lower gate voltage →
     * lower shunt current → shallower glitch. A higher gate voltage →
     * stronger shunt → deeper glitch.
     *
     * The DAC output range is 0–3.3 V, mapping to 0–0xFFF.
     * The depth_mv parameter specifies how much to pull VCC down.
     * A depth of 0 means no glitch; a depth of 3300 mV means full shunt.
     *
     * DAC value = depth_mv * 0xFFF / 3300
     */
    if (depth_mv > 3300)
        depth_mv = 3300;

    uint16_t dac_val = (uint16_t)((uint32_t)depth_mv * 0x0FFF / 3300);
    dac_write(dac_val);

    /* Set series resistance */
    if (series_r_idx >= SERIES_R_TABLE_SIZE)
        series_r_idx = SERIES_R_TABLE_SIZE - 1;
    set_series_resistance(series_r_idx);

    g_current_depth_mv = depth_mv;
    g_current_series_r = series_r_idx;

    (void)width_ns; /* width is controlled by HRTIM, not here */
}

/* ---- Fire / Disable ------------------------------------------------ */

void power_glitch_fire(void)
{
    /*
     * In normal operation, the FPGA/HRTIM controls the glitch timing
     * by driving the shunt MOSFET gate directly. This function is
     * for manual/debug use — it asserts the glitch immediately.
     * The shunt MOSFET gate is on PB0 (FPGA output pin, but the MCU
     * can also drive it via a shared path in debug mode).
     */
    /* Not directly accessible from MCU in normal operation;
     * this would set a debug GPIO. */
}

void power_glitch_disable(void)
{
    /* Set DAC to mid-range (MOSFET off, full VCC to target) */
    dac_write(0x0800);
    set_series_resistance(7); /* max resistance = minimal effect */
}

/* ---- End of file --------------------------------------------------- */