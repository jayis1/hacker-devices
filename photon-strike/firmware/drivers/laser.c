/**
 * drivers/laser.c — 1064 nm pulsed laser driver control
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls:
 *   - The driver-enable line (PB0) — gates the whole driver.
 *   - The TIM1_CH1 PWM gate (PA8) — sets the pulse envelope width.
 *   - The laser diode current via an external DAC (MCP4921 on SPI2,
 *     not on the FPGA SPI). The DAC sets the driver current sink
 *     reference; the FPGA pulse gate then switches that current into
 *     the diode for the programmed number of nanoseconds.
 *   - The TEC PWM (PA9, TIM1_CH2) for diode temperature stabilization.
 *   - Reads the diode thermistor (PC0, ADC1_IN0) for the M7-side
 *     temperature display and software cutoff.
 *
 * The M4 independently monitors safety and holds the shutter (PB1).
 * This driver only controls the enable + current + TEC; the M4's
 * shutter is the hard cut.
 */

#include "laser.h"
#include "gpio.h"
#include "../registers.h"

#define LASER_DAC_CS_PORT  ((volatile uint32_t *)GPIOB_BASE)
#define LASER_DAC_CS_PIN   6u    /* PB6 (re-used as a generic GPIO; SPI2 CS) */

static uint16_t s_current_counts = 0;
static bool     s_enabled = false;

/* ─── DAC (MCP4921) write ─────────────────────────────────────────────── */
/* The MCP4921 is a 12-bit single-channel DAC with an SPI interface.
 * We bit-bang it on PB6 (CS), PB7 (SCK), PB8 (SDI) — these are NOT the
 * I2C pins; PB6/7/8 here are a separate bit-bang GPIO bus. (On the
 * production board these route to an isolated SPI2; the bit-bang is
 * a firmware fallback for the prototype.) */
static void dac_write(uint16_t value)
{
    /* 16-bit command word: 0x3XXX = write, Vref buffered, gain 1x, /SHDN */
    uint16_t cmd = 0x3000u | (value & 0x0FFFu);

    gpio_clr_pin(LASER_DAC_CS_PORT, LASER_DAC_CS_PIN);
    for (int i = 15; i >= 0; i--) {
        if ((cmd >> i) & 1u)
            gpio_set_pin((volatile uint32_t *)GPIOB_BASE, 7);
        else
            gpio_clr_pin((volatile uint32_t *)GPIOB_BASE, 7);
        gpio_set_pin((volatile uint32_t *)GPIOB_BASE, 8);  /* SCK rising */
        gpio_clr_pin((volatile uint32_t *)GPIOB_BASE, 8);  /* SCK falling */
    }
    gpio_set_pin(LASER_DAC_CS_PORT, LASER_DAC_CS_PIN);
}

/* ─── ADC1 (laser thermistor on PC0) ──────────────────────────────────── */
static uint16_t adc1_read(void)
{
    volatile uint32_t *adc = (volatile uint32_t *)ADC1_BASE;

    /* Ensure ADC is enabled. */
    if (!(adc[ADC_CR / 4u] & ADC_CR_ADEN)) {
        /* Calibrate then enable. */
        adc[ADC_CR / 4u] |= ADC_CR_ADCAL;
        while (adc[ADC_CR / 4u] & ADC_CR_ADCAL) ;
        adc[ADC_CR / 4u] |= ADC_CR_ADEN;
        while (!(adc[ADC_ISR / 4u] & ADC_ISR_ADRDY)) ;
    }

    /* Single conversion on channel 0 (PC0), software triggered. */
    adc[ADC_SQR1 / 4u] = (1u << 0);   /* 1 conversion, channel 0 */
    adc[ADC_CR / 4u]  |= (1u << 2);   /* ADSTART */
    while (!(adc[ADC_ISR / 4u] & ADC_ISR_EOC)) ;
    return (uint16_t)adc[ADC_DR / 4u];
}

/* Convert the 12-bit ADC count to degrees C. The thermistor is a 10k
 * NTC (B=3977) in a divider with a 10k pull-up to 3.3 V. */
static uint16_t ntc_to_c(uint16_t raw)
{
    /* raw = 4095 * R_ntc / (R_pull + R_ntc)  → R_ntc = R_pull * raw / (4095 - raw)
     * Steinhart-Hart (simplified Beta model):
     *   1/T = 1/T0 + (1/B) * ln(R/R0)
     * with T0 = 298.15 K (25 C), R0 = 10k, B = 3977.
     * Returns degrees C as uint16_t. */
    if (raw >= 4090u) return 0;        /* open / cold */
    if (raw == 0)     return 150;      /* short / hot */

    /* Avoid floating point (Cortex-M7 has FPU, but keep it integer-safe
     * for the M4 build path that may reuse this code). Use a lookup
     * table approximation: 16 points spanning 0..80 C. */
    static const uint16_t lut_raw[17] = {
        3740, 3670, 3580, 3470, 3340, 3190, 3010, 2810,
        2590, 2360, 2130, 1900, 1690, 1490, 1310, 1150, 1010
    };
    static const uint16_t lut_c[17] = {
         0,   5,  10,  15,  20,  25,  30,  35,
        40,  45,  50,  55,  60,  65,  70,  75, 80
    };
    for (int i = 0; i < 16; i++) {
        if (raw >= lut_raw[i + 1]) {
            /* linear interp between i and i+1 */
            uint32_t span = lut_raw[i] - lut_raw[i + 1];
            uint32_t off  = lut_raw[i] - raw;
            return (uint16_t)(lut_c[i] + (5u * off) / span);
        }
    }
    return 80;
}

/* ─── Public API ──────────────────────────────────────────────────────── */
void laser_init(void)
{
    /* Configure the DAC bit-bang pins as outputs (PB6/7/8 alt use). */
    /* NOTE: PB8/PB9 are I2C1 SCL/SDA. The production board routes the
     * DAC CS/SCK/SDI to PA15/PB13/PB14. For the prototype we use the
     * bit-bang on PA15/PB13/PB14 (set here). */
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 15, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 13, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 14, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_set_pin  ((volatile uint32_t *)GPIOA_BASE, 15);  /* CS idle high */

    /* TIM1 for the TEC PWM (PA9, CH2) — 100 kHz, 50% duty. */
    volatile uint32_t *tim1 = (volatile uint32_t *)TIM1_BASE;
    tim1[TIM_PSC / 4u]  = 0;             /* 480 MHz / 1 */
    tim1[TIM_ARR / 4u]  = 4800 - 1;      /* 100 kHz */
    tim1[TIM_CCMR1 / 4u] = (6u << 12);   /* PWM mode 1 on CH2 */
    tim1[TIM_CCER / 4u]  |= (1u << 4);   /* CC2E */
    tim1[TIM_CCR1 / 4u]  = 2400;         /* 50% */
    tim1[TIM_BDTR / 4u]  |= TIM_BDTR_MOE;
    tim1[TIM_CR1 / 4u]   |= TIM_CR1_CEN;

    /* Start with the laser disabled and current zero. */
    laser_disable();
    dac_write(0);
}

void laser_enable(void)
{
    /* Software enable only; the M4 still holds the shutter. */
    gpio_set_pin((volatile uint32_t *)GPIOB_BASE, 0);
    s_enabled = true;
}

void laser_disable(void)
{
    gpio_clr_pin((volatile uint32_t *)GPIOB_BASE, 0);
    s_enabled = false;
}

void laser_set_current(uint16_t dac_counts)
{
    if (dac_counts > 4095u) dac_counts = 4095u;
    /* Enforce the compiled-in energy ceiling by clamping the DAC. */
    uint16_t ceiling = (uint16_t)(PS_MAX_ENERGY_NJ * 200u);
    if (dac_counts > ceiling) dac_counts = ceiling;
    s_current_counts = dac_counts;
    dac_write(dac_counts);
}

uint16_t laser_read_temp(void)
{
    return ntc_to_c(adc1_read());
}

bool laser_driver_fault(void)
{
    /* PB2 is the driver fault input (active low). */
    return !gpio_read_pin((volatile uint32_t *)GPIOB_BASE, 2);
}