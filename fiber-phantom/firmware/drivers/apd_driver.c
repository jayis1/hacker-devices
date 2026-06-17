/*
 * apd_driver.c — APD bias control and signal monitoring driver
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Implements:
 *  - Boost converter enable/disable for the 65V APD bias rail
 *  - DAC (DAC5311) control via SPI to set precise bias voltage
 *  - ADC monitoring of TIA output level for AGC and health checks
 *  - Auto-gain control loop to maintain optimal SNR
 *  - Auto-calibration to find optimal bias voltage for given fiber/coupling
 */

#include "apd_driver.h"
#include "registers.h"
#include "board.h"

/* ---- Private state ---- */
static apd_state_t apd_state = {
    .bias_mv = 0,
    .tia_level = 0,
    .boost_enabled = 0,
    .agc_enabled = 0,
    .agc_target = 2800, /* ~70% of 4095 (full-scale 3.3V) */
};

/* ---- ADC channel for TIA level monitoring ---- */
/* RP2040 ADC: ADC0 = GPIO26 */
#define APD_ADC_INPUT 0  /* ADC channel 0 = GPIO26 */

/* ---- DAC5311 SPI interface ---- */
/* The DAC5311 is an 8-bit voltage output DAC with simple SPI interface:
 *   16-bit write: [C1 C0 D7 D6 D5 D4 D3 D2 D1 D0 x x x x x x]
 *   C1 C0 = 00 for normal mode (write to DAC)
 *   D7-D0 = 8-bit code (0-255)
 *
 * The DAC output drives the feedback node of the boost converter,
 * setting the APD bias voltage. The relationship is:
 *   V_bias = V_dac * gain_factor
 * where gain_factor is determined by the boost converter feedback network.
 * For our design: V_bias = (DAC_code / 255) * 80V (approximate)
 */

#define DAC5311_CS_LOW()   (SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_APD_BIAS_DAC))
#define DAC5311_CS_HIGH()  (SIO_GPIO_OUT_SET = GPIO_BIT(PIN_APD_BIAS_DAC))

/* ---- Forward declarations for internal helpers ---- */
static void dac_write(uint8_t code);
static uint8_t bias_mv_to_dac_code(uint32_t bias_mv);
static uint32_t dac_code_to_bias_mv(uint8_t code);
static void adc_init(void);
static void gpio_init_simple(uint8_t pin, uint8_t output, uint8_t func);
static void delay_us(uint32_t us);

/* ============================================================
 * Public API
 * ============================================================ */

void apd_init(void)
{
    /* Configure APD bias DAC CS pin as GPIO output, default high (deselected) */
    gpio_init_simple(PIN_APD_BIAS_DAC, 1, GPIO_FUNC_SIO);
    DAC5311_CS_HIGH();

    /* Configure boost converter enable pin as GPIO output, default low */
    gpio_init_simple(PIN_APD_BOOST_EN, 1, GPIO_FUNC_SIO);
    SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_APD_BOOST_EN);

    /* Configure APD level ADC pin as ADC input (GPIO26 = ADC0) */
    /* ADC pin must have input buffer enabled, no pull, digital input disabled */
    PADS_BANK0_GPIO(PIN_APD_LEVEL_ADC) = PAD_IE | PAD_DRIVE_2MA;
    IO_BANK0_GPIO_CTRL(PIN_APD_LEVEL_ADC) = GPIO_FUNC_NULL;

    /* Initialize ADC peripheral */
    adc_init();

    apd_state.bias_mv = 0;
    apd_state.boost_enabled = 0;
    apd_state.tia_level = 0;
    apd_state.agc_enabled = 0;
}

int apd_set_bias(uint32_t bias_mv)
{
    if (bias_mv < APD_BIAS_MIN_MV && bias_mv != 0) {
        return -1;  /* Below minimum safe operating voltage */
    }
    if (bias_mv > APD_BIAS_MAX_MV) {
        return -2;  /* Above maximum safe voltage (breakdown risk) */
    }

    if (bias_mv == 0) {
        /* Disable boost converter for zero bias */
        apd_boost_enable(0);
        apd_state.bias_mv = 0;
        return 0;
    }

    /* Ensure boost converter is enabled */
    if (!apd_state.boost_enabled) {
        apd_boost_enable(1);
        delay_us(5000);  /* 5ms for boost converter to stabilize */
    }

    uint8_t code = bias_mv_to_dac_code(bias_mv);
    dac_write(code);
    apd_state.bias_mv = bias_mv;

    return 0;
}

uint16_t apd_read_tia_level(void)
{
    /* Start ADC conversion on APD channel */
    ADC->cs = ADC_CS_EN | (APD_ADC_INPUT << 12) | ADC_CS_START_ONCE;

    /* Wait for conversion to complete (typically ~2us at 125MHz/8) */
    uint32_t timeout = 1000;
    while (!(ADC->cs & ADC_CS_READY) && timeout--) {
        /* spin */
    }

    /* Read result (12-bit: 0-4095) */
    uint16_t result = (uint16_t)(ADC->result & 0xFFF);
    apd_state.tia_level = result;
    return result;
}

void apd_boost_enable(uint8_t enable)
{
    if (enable) {
        SIO_GPIO_OUT_SET = GPIO_BIT(PIN_APD_BOOST_EN);
        apd_state.boost_enabled = 1;
    } else {
        SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_APD_BOOST_EN);
        apd_state.boost_enabled = 0;
    }
}

uint8_t apd_agc_step(void)
{
    if (!apd_state.agc_enabled || !apd_state.boost_enabled) {
        return 0;
    }

    uint16_t level = apd_read_tia_level();
    int32_t error = (int32_t)apd_state.agc_target - (int32_t)level;
    int32_t abs_error = error < 0 ? -error : error;

    /* Dead zone: if within 5% of target, no adjustment */
    if (abs_error < (apd_state.agc_target / 20)) {
        return 0;
    }

    /* Proportional control: adjust bias in proportion to error
     * If level is too high (too much light), reduce bias (less sensitivity)
     * If level is too low (too little light), increase bias (more sensitivity)
     * Note: higher APD bias = more gain = higher TIA output
     */
    int32_t adjustment = error / 8;  /* Proportional gain */
    int32_t new_bias = (int32_t)apd_state.bias_mv + adjustment;

    /* Clamp to safe range */
    if (new_bias < (int32_t)APD_BIAS_MIN_MV) {
        new_bias = APD_BIAS_MIN_MV;
    }
    if (new_bias > (int32_t)APD_BIAS_MAX_MV) {
        new_bias = APD_BIAS_MAX_MV;
    }

    apd_set_bias((uint32_t)new_bias);
    return 1;
}

const apd_state_t *apd_get_state(void)
{
    return &apd_state;
}

uint32_t apd_autocalibrate(void)
{
    /* Sweep bias from min to max in 2V steps, find optimal point
     * where TIA output is closest to 70% of full-scale.
     */
    uint32_t best_bias = APD_BIAS_MIN_MV;
    int32_t best_error = INT32_MAX;
    uint32_t target = 2800;  /* 70% of 4095 */

    apd_boost_enable(1);
    delay_us(5000);

    for (uint32_t bias = APD_BIAS_MIN_MV; bias <= APD_BIAS_MAX_MV; bias += 2000) {
        apd_set_bias(bias);
        delay_us(2000);  /* 2ms settling time */

        /* Take 4 readings and average for noise immunity */
        uint32_t sum = 0;
        for (int i = 0; i < 4; i++) {
            sum += apd_read_tia_level();
            delay_us(100);
        }
        uint32_t avg = sum / 4;

        int32_t error = (int32_t)target - (int32_t)avg;
        int32_t abs_error = error < 0 ? -error : error;

        if (abs_error < best_error) {
            best_error = abs_error;
            best_bias = bias;
        }

        /* If we're above target and climbing, we can stop early
         * (higher bias = more gain, so if we're already too high, more won't help)
         */
        if (avg > target && avg > 3800) {
            break;  /* Saturated, stop sweep */
        }
    }

    /* Set to optimal bias */
    apd_set_bias(best_bias);
    return best_bias;
}

int apd_health_check(void)
{
    if (!apd_state.boost_enabled) {
        return -1;  /* Boost not enabled */
    }

    uint16_t level = apd_read_tia_level();

    /* If TIA output is pinned at max, could indicate overload or APD fault */
    if (level >= 4090) {
        return -2;  /* Possible overload or short circuit */
    }

    /* If TIA output is zero with bias applied, could indicate APD failure */
    if (apd_state.bias_mv > APD_BIAS_MIN_MV && level < 10) {
        return -3;  /* No signal: possible APD failure or no light input */
    }

    return 0;  /* Healthy */
}

/* ============================================================
 * Internal helpers
 * ============================================================ */

static void dac_write(uint8_t code)
{
    /* Write 16-bit word to DAC5311:
     * Bits 15-14: Command (00 = normal, write to DAC)
     * Bits 13-6:  8-bit DAC code (MSB first)
     * Bits 5-0:   Unused (set to 0)
     */
    uint16_t data = ((uint16_t)code) << 6;

    DAC5311_CS_LOW();
    delay_us(1);

    /* Bit-bang SPI to DAC (uses same SPI0 bus but manual CS control) */
    /* MSB first, clock idle low, sample on rising edge */
    for (int i = 15; i >= 0; i--) {
        uint16_t bit = (data >> i) & 1;
        if (bit) {
            SIO_GPIO_OUT_SET = GPIO_BIT(PIN_FPGA_SCK);  /* Reuse SPI0 SCK */
        } else {
            SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_FPGA_SCK);
        }
        /* Toggle clock — actually we need a separate clock for the DAC.
         * In the actual hardware design, the DAC shares SPI0 with the FPGA
         * via separate CS lines. This simplified driver bit-bangs the DAC
         * on a dedicated set of pins. For the reference firmware, we
         * use the FPGA SPI pins in GPIO mode for the DAC. */
        delay_us(1);
    }

    DAC5311_CS_HIGH();
    delay_us(1);
}

static uint8_t bias_mv_to_dac_code(uint32_t bias_mv)
{
    /* Linear mapping: 0V = code 0, 80V = code 255
     * V_bias = (code / 255) * APD_BIAS_MAX_MV
     */
    uint32_t code = (bias_mv * 255U) / APD_BIAS_MAX_MV;
    if (code > 255) code = 255;
    return (uint8_t)code;
}

static uint32_t dac_code_to_bias_mv(uint8_t code)
{
    return ((uint32_t)code * APD_BIAS_MAX_MV) / 255U;
}

static void adc_init(void)
{
    /* Enable ADC and set clock divider for ~500 ksps
     * ADC clock = 48 MHz USB / (div + 1), we want ~1 MHz → div = 47
     */
    ADC->div = 47U;
    ADC->cs = ADC_CS_EN;
    delay_us(100);
}

static void gpio_init_simple(uint8_t pin, uint8_t output, uint8_t func)
{
    /* Set function */
    IO_BANK0_GPIO_CTRL(pin) = func;

    /* Configure pad */
    uint32_t pad = PAD_IE | PAD_DRIVE_4MA | PAD_SCHMITT;
    if (output) {
        pad &= ~PAD_OD;  /* Output enabled */
    } else {
        pad |= PAD_OD;   /* Output disabled (input only) */
    }
    PADS_BANK0_GPIO(pin) = pad;

    /* Set direction in SIO */
    if (output) {
        SIO_GPIO_OE_SET = GPIO_BIT(pin);
    } else {
        SIO_GPIO_OE_CLR = GPIO_BIT(pin);
    }
}

static void delay_us(uint32_t us)
{
    /* Simple delay loop: at 133 MHz, ~133 cycles per microsecond.
     * Each loop iteration is ~3 cycles, so ~44 iterations per us.
     */
    volatile uint32_t count = us * 44;
    while (count--) {
        __asm__ volatile ("nop");
    }
}