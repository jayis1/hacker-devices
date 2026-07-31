/*
 * coil_driver.c — H-bridge coil driver implementation for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This module controls the bipolar H-bridge that drives the coil assembly.
 * It uses the STM32G474's HRTIM peripheral for sub-nanosecond pulse timing
 * and implements closed-loop current control via a PI controller.
 *
 * H-bridge topology:
 *
 *         +5V ─┬───┬───┬─────┬───┬───┐
 *              │   │   │     │   │   │
 *             Q1  Q2  │    Q3  Q4  │
 *              │   │  ┌─┐   │   │  ┌─┐
 *              │   └──┤ ├───┘   ├──┤ │
 *              │      └─┘       │  └─┘
 *              │   H-bridge     │
 *              └─── COIL ───────┘
 *              │               │
 *         ┌────┴────┐    ┌────┴────┐
 *         │UCC21520#1│    │UCC21520#2│
 *         └─────────┘    └─────────┘
 *              │               │
 *         HRTIM A,B        HRTIM C,D
 *
 * Forward (NORTH): Q1 + Q4 ON, Q2 + Q3 OFF → current flows L→R
 * Reverse (SOUTH): Q2 + Q3 ON, Q1 + Q4 OFF → current flows R→L
 * Brake:           Q2 + Q4 ON → coil shorted for fast current decay
 */

#include "coil_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ---- Globals ---- */
bridge_state_t  g_bridge_state = BRIDGE_IDLE;
pi_controller_t g_pi = {0};
sweep_state_t  g_sweep = {0};

/* Private: current target for PI controller */
static uint32_t s_target_current_ma = 0;
static polarity_t s_target_polarity = POLARITY_NORTH;
static uint32_t s_pulse_remaining = 0;
static uint32_t s_pulse_delay_counter = 0;
static uint32_t s_pulse_width_ns = 0;

/* ---- Private helpers ---- */

static void gpio_config_output(GPIO_TypeDef *port, uint8_t pin,
                                uint8_t speed, uint8_t otype)
{
    /* Set MODER to output (01) */
    uint32_t moder = port->MODER;
    moder &= ~(0x03U << (pin * 2));
    moder |= (GPIO_MODE_OUTPUT << (pin * 2));
    port->MODER = moder;

    /* Set OTYPER */
    uint32_t otyper = port->OTYPER;
    otyper &= ~(1U << pin);
    otyper |= (otype << pin);
    port->OTYPER = otyper;

    /* Set OSPEEDR */
    uint32_t ospeedr = port->OSPEEDR;
    ospeedr &= ~(0x03U << (pin * 2));
    ospeedr |= (speed << (pin * 2));
    port->OSPEEDR = ospeedr;

    /* No pull-up/pull-down */
    uint32_t pupdr = port->PUPDR;
    pupdr &= ~(0x03U << (pin * 2));
    port->PUPDR = pupdr;
}

static void gpio_config_af(GPIO_TypeDef *port, uint8_t pin,
                            uint8_t af, uint8_t speed)
{
    /* Set MODER to AF mode (10) */
    uint32_t moder = port->MODER;
    moder &= ~(0x03U << (pin * 2));
    moder |= (GPIO_MODE_AF << (pin * 2));
    port->MODER = moder;

    /* Set AFR */
    if (pin < 8) {
        uint32_t afrl = port->AFRL;
        afrl &= ~(0x0FU << (pin * 4));
        afrl |= (af << (pin * 4));
        port->AFRL = afrl;
    } else {
        uint32_t afrh = port->AFRH;
        afrh &= ~(0x0FU << ((pin - 8) * 4));
        afrh |= (af << ((pin - 8) * 4));
        port->AFRH = afrh;
    }

    /* Set OSPEEDR */
    uint32_t ospeedr = port->OSPEEDR;
    ospeedr &= ~(0x03U << (pin * 2));
    ospeedr |= (speed << (pin * 2));
    port->OSPEEDR = ospeedr;
}

static void gpio_set(GPIO_TypeDef *port, uint8_t pin)
{
    port->BSRR = (1U << pin);
}

static void gpio_clear(GPIO_TypeDef *port, uint8_t pin)
{
    port->BSRR = (1U << (pin + 16));
}

static uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR >> pin) & 1U;
}

/* ---- HRTIM configuration ---- */

static void hrtim_enable_clock(void)
{
    /* Enable HRTIM clock in RCC */
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_HRTIMEN);
    /* Small delay for clock to stabilize */
    for (volatile int i = 0; i < 10; i++) { }
}

static void hrtim_config_timing_unit_a(void)
{
    /*
     * Timing Unit A controls the left high-side MOSFET (Q1).
     * We use it in one-shot mode for pulse generation.
     *
     * HRTIM clock: 170 MHz × 32 = 5.44 GHz (virtual)
     * 1 tick = 184 ps
     * For a 1 µs pulse: period = 1e-6 / 184e-12 = 5435 ticks
     * For a 100 ns pulse: period = 100e-9 / 184e-12 = 543 ticks
     * For a 100 ms pulse: period = 100e-3 / 184e-12 = 543,478,260 ticks
     *   (exceeds 16-bit → use prescaler for long pulses)
     *
     * For pulses > ~3 ms, we switch to the prescaled HRTIM clock (CKPSC).
     */

    /* Configure Timer A in continuous mode for PWM (DC mode)
     * or one-shot mode for pulses.
     * Initially set to continuous for flexibility. */

    /* Set period to maximum (65535) — will be adjusted per pulse */
    HRTIM1->TIMA_PER = 0xFFD0;  /* Leave some margin */

    /* CMP1 = set point (start of pulse) */
    HRTIM1->TIMA_CMP1 = 0;

    /* CMP2 = reset point (end of pulse) */
    HRTIM1->TIMA_CMP2 = 0;

    /* Output: Set on period, reset on CMP2 */
    HRTIM1->SETA1R = HRTIM_RST_PER;  /* Set at period */
    HRTIM1->RSTA1R = 0x02;           /* Reset at CMP2 */

    /* Enable timing unit A */
    HRTIM1->TIMA_CR = 0;  /* Start disabled */
}

static void hrtim_config_timing_unit_b(void)
{
    /* Timing Unit B controls the left low-side MOSFET (Q2).
     * Q2 is the complement of Q1 with deadtime insertion. */
    HRTIM1->TIMB_PER = 0xFFD0;
    HRTIM1->TIMB_CMP1 = 0;
    HRTIM1->TIMB_CMP2 = 0;
    HRTIM1->SETB1R = 0x02;  /* Set at CMP2 (complementary) */
    HRTIM1->RSTB1R = 0x01;  /* Reset at period */
    HRTIM1->TIMB_CR = 0;
}

static void hrtim_config_timing_unit_c(void)
{
    /* Timing Unit C controls the right high-side MOSFET (Q3).
     * In forward mode, Q3 is OFF when Q1 is ON.
     * In reverse mode, Q3 is ON when Q1 is OFF. */
    HRTIM1->TIMC_PER = 0xFFD0;
    HRTIM1->TIMC_CMP1 = 0;
    HRTIM1->TIMC_CMP2 = 0;
    HRTIM1->SETC1R = 0x02;
    HRTIM1->RSTC1R = 0x01;
    HRTIM1->TIMC_CR = 0;
}

static void hrtim_config_timing_unit_d(void)
{
    /* Timing Unit D controls the right low-side MOSFET (Q4).
     * Q4 is ON when Q1 is ON (forward mode). */
    HRTIM1->TIMD_PER = 0xFFD0;
    HRTIM1->TIMD_CMP1 = 0;
    HRTIM1->TIMD_CMP2 = 0;
    HRTIM1->SETD1R = HRTIM_RST_PER;
    HRTIM1->RSTD1R = 0x02;
    HRTIM1->TIMD_CR = 0;
}

static void hrtim_config_timing_unit_e(void)
{
    /* Timing Unit E is used for sweep mode frequency generation.
     * It generates a variable-frequency carrier that modulates
     * the H-bridge output via the external event system. */
    HRTIM1->TIME_PER = 0xFFD0;
    HRTIM1->TIME_CMP1 = 0;
    HRTIM1->TIME_CMP2 = 0x8000;  /* 50% duty */
    HRTIM1->SETE1R = HRTIM_RST_PER;
    HRTIM1->RSTE1R = 0x02;
    HRTIM1->TIME_CR = 0;
}

static void hrtim_enable_outputs(void)
{
    /* Enable all four H-bridge outputs */
    HRTIM1->OENR = 0x000000FF;  /* Enable TA1, TA2, TB1, TB2, TC1, TC2, TD1, TD2 */
}

static void hrtim_disable_outputs(void)
{
    /* Disable all outputs — immediate cutoff */
    HRTIM1->OENR2 = 0x000000FF;  /* Write to disable register */
}

/* ---- ADC configuration for current sensing ---- */

static void adc_config_current_sense(void)
{
    /* Enable ADC clock */
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_ADC12EN);

    /* Exit deep power-down */
    CLEAR_BIT(ADC1->CR, ADC_CR_DEEPPWD);
    /* Enable voltage regulator */
    SET_BIT(ADC1->CR, ADC_CR_ADVREGEN);
    /* Wait for regulator startup (~1 µs) */
    for (volatile int i = 0; i < 170; i++) { }

    /* Calibrate ADC */
    SET_BIT(ADC1->CR, ADC_CR_ADCAL);
    while (READ_REG(ADC1->CR) & ADC_CR_ADCAL) { }

    /* Enable ADC */
    SET_BIT(ADC1->CR, ADC_CR_ADEN);
    while (!(READ_REG(ADC1->ISR) & ADC_ISR_ADRDY)) { }

    /* Configure channel 5 (PA0/CURRENT_SENSE) */
    /* Set sample time to 7.5 ADC cycles (fast enough for 500 kHz) */
    uint32_t smpr1 = ADC1->SMPR1;
    smpr1 &= ~(0x07U << (5 * 3));  /* Clear channel 5 sample time */
    smpr1 |= (0x02U << (5 * 3));   /* 7.5 cycles */
    ADC1->SMPR1 = smpr1;

    /* Configure sequence: channel 5 only */
    ADC1->SQR1 = (1U << 0) | (5U << 6);  /* 1 conversion, channel 5 */

    /* 12-bit resolution, right-aligned */
    uint32_t cfgr = ADC1->CFGR;
    cfgr &= ~(0x03U << 3);  /* RES = 00 (12-bit) */
    cfgr &= ~(1U << 5);     /* ALIGN = 0 (right) */
    cfgr |= (1U << 13);     /* DMACFG = 1 (continuous DMA) */
    ADC1->CFGR = cfgr;
}

static uint16_t adc_read_current(void)
{
    /* Start conversion */
    SET_BIT(ADC1->CR, ADC_CR_ADSTART);
    /* Wait for end of conversion */
    while (!(READ_REG(ADC1->ISR) & ADC_ISR_EOC)) { }
    /* Read result */
    return (uint16_t)(READ_REG(ADC1->DR) & 0xFFFF);
}

/* ---- Current to HRTIM compare conversion ---- */

uint32_t coil_driver_current_to_cmp(uint32_t current_ma)
{
    /*
     * Convert desired current (mA) to HRTIM compare value.
     *
     * The H-bridge is driven by a 5V supply through a coil with
     * resistance R_coil and inductance L_coil.
     *
     * For pulsed mode (short pulses), the current ramps linearly:
     *   I(t) = V * t / L
     * So the peak current is determined by the pulse width, not duty cycle.
     *
     * For DC mode, the steady-state current is:
     *   I = V / R_coil
     * And we control it via PWM duty cycle:
     *   I_avg = D * V / R_coil
     *
     * The ACS724 gives us real-time feedback, so the PI controller
     * adjusts the duty cycle to hit the target.
     *
     * For the HRTIM compare value:
     *   CMP = (duty_cycle * PERIOD) / 1000
     * where duty_cycle is in per-mille (0-1000).
     *
     * Initial estimate: duty = I_target / I_max * 1000
     * I_max at 100% duty ≈ 5V / 0.1Ω = 50A (conservative)
     */
    uint32_t duty_permille;
    if (current_ma >= 50000) {
        duty_permille = 1000;  /* Max duty */
    } else {
        duty_permille = (current_ma * 1000UL) / 50000UL;
    }

    /* Convert to HRTIM ticks (period = 0xFFD0 = 65488) */
    uint32_t cmp = (duty_permille * 65488UL) / 1000UL;
    if (cmp > 65400) cmp = 65400;  /* Safety margin */
    return cmp;
}

uint32_t coil_driver_width_to_period(uint32_t width_ns)
{
    /*
     * Convert pulse width (ns) to HRTIM period value.
     * HRTIM tick = 184 ps = 0.184 ns
     * period = width_ns / 0.184 ns
     *
     * For long pulses (> 12 µs), we need the prescaler.
     * HRTIM prescaler options: 1, 2, 4, 8, 16, 32, 64, 128
     * With prescaler 32: tick = 5.888 ns
     * With prescaler 128: tick = 23.552 ns
     *
     * Max period with prescaler 1: 65535 * 0.184 ns = 12.05 µs
     * Max period with prescaler 32: 65535 * 5.888 ns = 386 µs
     * Max period with prescaler 128: 65535 * 23.552 ns = 1.54 ms
     *
     * For pulses > 1.54 ms, we use a software timer with the master
     * timer generating an interrupt at the pulse end.
     */

    if (width_ns < 12000) {
        /* Prescaler 1: 184 ps per tick */
        return (width_ns * 1000UL) / 184UL;
    } else if (width_ns < 386000) {
        /* Prescaler 32: 5.888 ns per tick */
        return width_ns / 5888UL;  /* Approximate — will set prescaler */
    } else if (width_ns < 1540000) {
        /* Prescaler 128: 23.552 ns per tick */
        return width_ns / 23552UL;
    } else {
        /* Use software timer for very long pulses */
        return 0;  /* Signal to use software timer */
    }
}

/* ---- Public API implementation ---- */

void coil_driver_init(void)
{
    /* Enable HRTIM clock */
    hrtim_enable_clock();

    /* Configure H-bridge GPIO pins as HRTIM AF outputs */
    /* PA8, PA9 → HRTIM TA1, TA2 (AF13) */
    gpio_config_af(HRTIM_HSL_PORT, HRTIM_HSL_PIN, 13, GPIO_SPEED_VHIGH);
    gpio_config_af(HRTIM_LSL_PORT, HRTIM_LSL_PIN, 13, GPIO_SPEED_VHIGH);
    /* PA10, PA11 → HRTIM TC1, TC2 (AF13) */
    gpio_config_af(HRTIM_HSR_PORT, HRTIM_HSR_PIN, 13, GPIO_SPEED_VHIGH);
    gpio_config_af(HRTIM_LSR_PORT, HRTIM_LSR_PIN, 13, GPIO_SPEED_VHIGH);

    /* Configure safety enable pin as input with pull-down */
    {
        uint32_t moder = SAFETY_ENABLE_PORT->MODER;
        moder &= ~(0x03U << (SAFETY_ENABLE_PIN * 2));
        SAFETY_ENABLE_PORT->MODER = moder;  /* Input mode */

        uint32_t pupdr = SAFETY_ENABLE_PORT->PUPDR;
        pupdr &= ~(0x03U << (SAFETY_ENABLE_PIN * 2));
        pupdr |= (GPIO_PUPD_PD << (SAFETY_ENABLE_PIN * 2));
        SAFETY_ENABLE_PORT->PUPDR = pupdr;
    }

    /* Configure HRTIM timing units */
    hrtim_config_timing_unit_a();
    hrtim_config_timing_unit_b();
    hrtim_config_timing_unit_c();
    hrtim_config_timing_unit_d();
    hrtim_config_timing_unit_e();

    /* Configure ADC for current sensing */
    adc_config_current_sense();

    /* Initialize PI controller */
    g_pi.kp = 3200;    /* Proportional gain (Q15) */
    g_pi.ki = 800;     /* Integral gain (Q15) */
    g_pi.integral = 0;
    g_pi.out_min = 0;
    g_pi.out_max = 65400;

    /* Initialize state */
    g_bridge_state = BRIDGE_IDLE;
    s_target_current_ma = 0;
    s_pulse_remaining = 0;
    s_pulse_delay_counter = 0;

    /* Do NOT enable HRTIM yet — requires arm() */
    g_status.safety_armed = 0;
}

int coil_driver_arm(void)
{
    /* Check safety switch */
    if (!gpio_read(SAFETY_ENABLE_PORT, SAFETY_ENABLE_PIN)) {
        g_status.error_code = ERR_SAFETY_NOT_ARMED;
        return -1;
    }

    /* Check for fault condition */
    if (g_bridge_state == BRIDGE_FAULT) {
        return -1;
    }

    /* Enable HRTIM master */
    SET_BIT(HRTIM1->CR1, HRTIM_CR1_HRTIMEN);

    /* Enable outputs */
    hrtim_enable_outputs();

    g_status.safety_armed = 1;
    g_bridge_state = BRIDGE_IDLE;
    return 0;
}

void coil_driver_disarm(void)
{
    /* Stop all timing units */
    HRTIM1->TIMA_CR = 0;
    HRTIM1->TIMB_CR = 0;
    HRTIM1->TIMC_CR = 0;
    HRTIM1->TIMD_CR = 0;
    HRTIM1->TIME_CR = 0;

    /* Disable outputs */
    hrtim_disable_outputs();

    /* Disable HRTIM master */
    CLEAR_BIT(HRTIM1->CR1, HRTIM_CR1_HRTIMEN);

    g_status.safety_armed = 0;
    g_bridge_state = BRIDGE_IDLE;
    s_target_current_ma = 0;
    s_pulse_remaining = 0;
}

static void hrtim_set_forward(void)
{
    /*
     * Forward (NORTH) polarity:
     * Q1 (HSL) + Q4 (LSR) ON
     * Q2 (LSL) + Q3 (HSR) OFF
     *
     * Timer A: Q1 high-side, active during pulse
     * Timer D: Q4 low-side, active during pulse (same timing as A)
     * Timer B: Q2 low-side, inactive (complement of A)
     * Timer C: Q3 high-side, inactive (complement of D)
     */
    HRTIM1->SETA1R = HRTIM_RST_PER;  /* Q1 set at period start */
    HRTIM1->RSTA1R = 0x02;           /* Q1 reset at CMP2 (pulse end) */
    HRTIM1->SETD1R = HRTIM_RST_PER;  /* Q4 set at period start */
    HRTIM1->RSTD1R = 0x02;           /* Q4 reset at CMP2 */
    HRTIM1->SETB1R = 0x00;           /* Q2 never set */
    HRTIM1->RSTB1R = 0x01;           /* Q2 always reset */
    HRTIM1->SETC1R = 0x00;           /* Q3 never set */
    HRTIM1->RSTC1R = 0x01;           /* Q3 always reset */
}

static void hrtim_set_reverse(void)
{
    /*
     * Reverse (SOUTH) polarity:
     * Q2 (LSL) + Q3 (HSR) ON
     * Q1 (HSL) + Q4 (LSR) OFF
     */
    HRTIM1->SETB1R = HRTIM_RST_PER;  /* Q2 set at period start */
    HRTIM1->RSTB1R = 0x02;           /* Q2 reset at CMP2 */
    HRTIM1->SETC1R = HRTIM_RST_PER;  /* Q3 set at period start */
    HRTIM1->RSTC1R = 0x02;           /* Q3 reset at CMP2 */
    HRTIM1->SETA1R = 0x00;           /* Q1 never set */
    HRTIM1->RSTA1R = 0x01;           /* Q1 always reset */
    HRTIM1->SETD1R = 0x00;           /* Q4 never set */
    HRTIM1->RSTD1R = 0x01;           /* Q4 always reset */
}

static void hrtim_set_brake(void)
{
    /*
     * Brake mode: Q2 + Q4 ON → coil shorted for fast current decay.
     * This recirculates the coil energy and prevents inductive kickback.
     */
    HRTIM1->SETB1R = HRTIM_RST_PER;
    HRTIM1->RSTB1R = 0x00;           /* Q2 stays on */
    HRTIM1->SETD1R = HRTIM_RST_PER;
    HRTIM1->RSTD1R = 0x00;           /* Q4 stays on */
    HRTIM1->SETA1R = 0x00;
    HRTIM1->RSTA1R = 0x01;
    HRTIM1->SETC1R = 0x00;
    HRTIM1->RSTC1R = 0x01;
}

int coil_driver_pulse(uint32_t width_ns, uint32_t current_ma,
                      polarity_t polarity)
{
    /* Validate parameters */
    if (width_ns < COIL_MIN_PULSE_NS || width_ns > COIL_MAX_PULSE_NS) {
        g_status.error_code = ERR_INVALID_PARAM;
        return -1;
    }
    if (current_ma > COIL_MAX_CURRENT_MA) {
        g_status.error_code = ERR_INVALID_PARAM;
        return -1;
    }
    if (!g_status.safety_armed) {
        g_status.error_code = ERR_SAFETY_NOT_ARMED;
        return -1;
    }
    if (g_bridge_state == BRIDGE_FAULT) {
        return -1;
    }

    /* Check coil tip is connected */
    if (g_status.tip_id == 0) {
        g_status.error_code = ERR_NO_TIP;
        return -1;
    }

    /* Set polarity */
    if (polarity == POLARITY_NORTH) {
        hrtim_set_forward();
        g_bridge_state = BRIDGE_FORWARD;
    } else {
        hrtim_set_reverse();
        g_bridge_state = BRIDGE_REVERSE;
    }

    /* Calculate HRTIM compare values */
    uint32_t cmp = coil_driver_current_to_cmp(current_ma);
    uint32_t period = coil_driver_width_to_period(width_ns);

    if (period == 0) {
        /* Long pulse — use master timer interrupt */
        /* For now, use max period and software counter */
        period = 0xFFD0;
        /* TODO: implement software-extended pulse for > 1.54 ms */
    }

    /* Set period and compare */
    HRTIM1->TIMA_PER = period;
    HRTIM1->TIMA_CMP2 = cmp;  /* Pulse end */
    HRTIM1->TIMD_PER = period;
    HRTIM1->TIMD_CMP2 = cmp;

    /* Start timing units A and D simultaneously */
    SET_BIT(HRTIM1->TIMA_CR, HRTIM_TIM_CR_RETRIG);
    SET_BIT(HRTIM1->TIMD_CR, HRTIM_TIM_CR_RETRIG);

    /* Wait for pulse to complete (polling for simplicity) */
    /* In production, this would use interrupt-driven completion */
    uint32_t timeout = (width_ns / 1000) + 10;  /* ms timeout */
    volatile uint32_t wait = 0;
    while ((HRTIM1->TIMA_ISR & 0x01) == 0) {  /* Wait for period end */
        if (++wait > (timeout * 1000)) break;  /* Timeout */
    }

    /* Stop timing units */
    HRTIM1->TIMA_CR = 0;
    HRTIM1->TIMD_CR = 0;

    /* Brake the coil for fast current decay */
    hrtim_set_brake();
    SET_BIT(HRTIM1->TIMA_CR, HRTIM_TIM_CR_RETRIG);
    /* Brief brake period (~10 µs) */
    HRTIM1->TIMA_PER = 54350;  /* ~10 µs */
    for (volatile int i = 0; i < 100; i++) { }
    HRTIM1->TIMA_CR = 0;

    g_bridge_state = BRIDGE_IDLE;
    g_status.pulse_fired++;
    return 0;
}

int coil_driver_pulse_burst(uint32_t width_ns, uint32_t current_ma,
                             polarity_t polarity,
                             uint32_t count, uint32_t delay_us)
{
    if (count == 0 || count > 1000) {
        g_status.error_code = ERR_INVALID_PARAM;
        return -1;
    }

    g_status.pulse_count = count;
    g_status.pulse_fired = 0;

    for (uint32_t i = 0; i < count; i++) {
        int rc = coil_driver_pulse(width_ns, current_ma, polarity);
        if (rc != 0) return rc;

        /* Inter-pulse delay */
        if (i < count - 1 && delay_us > 0) {
            /* Simple delay loop: ~1 µs per 170 iterations at 170 MHz */
            for (volatile uint32_t d = 0; d < delay_us * 170; d++) { }
        }
    }

    return 0;
}

int coil_driver_dc_start(uint32_t current_ma, polarity_t polarity)
{
    if (current_ma > COIL_MAX_DC_CURRENT_MA) {
        g_status.error_code = ERR_INVALID_PARAM;
        return -1;
    }
    if (!g_status.safety_armed) {
        g_status.error_code = ERR_SAFETY_NOT_ARMED;
        return -1;
    }

    s_target_current_ma = current_ma;
    s_target_polarity = polarity;

    /* Set polarity */
    if (polarity == POLARITY_NORTH) {
        hrtim_set_forward();
        g_bridge_state = BRIDGE_FORWARD;
    } else {
        hrtim_set_reverse();
        g_bridge_state = BRIDGE_REVERSE;
    }

    /* Set up continuous PWM mode */
    uint32_t cmp = coil_driver_current_to_cmp(current_ma);
    HRTIM1->TIMA_PER = 0xFFD0;  /* Maximum period for smooth PWM */
    HRTIM1->TIMA_CMP2 = cmp;
    HRTIM1->TIMD_PER = 0xFFD0;
    HRTIM1->TIMD_CMP2 = cmp;

    /* Start in continuous mode */
    SET_BIT(HRTIM1->TIMA_CR, HRTIM_TIM_CR_CONT);
    SET_BIT(HRTIM1->TIMD_CR, HRTIM_TIM_CR_CONT);

    return 0;
}

void coil_driver_dc_stop(void)
{
    HRTIM1->TIMA_CR = 0;
    HRTIM1->TIMD_CR = 0;

    /* Brake */
    hrtim_set_brake();
    SET_BIT(HRTIM1->TIMA_CR, HRTIM_TIM_CR_RETRIG);
    HRTIM1->TIMA_PER = 54350;
    for (volatile int i = 0; i < 100; i++) { }
    HRTIM1->TIMA_CR = 0;

    g_bridge_state = BRIDGE_IDLE;
    s_target_current_ma = 0;
}

int coil_driver_sweep_start(uint32_t start_hz, uint32_t stop_hz,
                            uint16_t steps, uint16_t dwell_ms,
                            uint32_t current_ma)
{
    if (start_hz == 0 || stop_hz < start_hz || steps == 0) {
        g_status.error_code = ERR_INVALID_PARAM;
        return -1;
    }
    if (current_ma > COIL_MAX_DC_CURRENT_MA) {
        g_status.error_code = ERR_INVALID_PARAM;
        return -1;
    }
    if (!g_status.safety_armed) {
        g_status.error_code = ERR_SAFETY_NOT_ARMED;
        return -1;
    }

    g_sweep.start_hz = start_hz;
    g_sweep.stop_hz = stop_hz;
    g_sweep.steps = steps;
    g_sweep.dwell_ms = dwell_ms;
    g_sweep.current_step = 0;
    g_sweep.step_timer = 0;
    g_sweep.active = 1;

    s_target_current_ma = current_ma;

    /* Start with first frequency step */
    /* Frequency → HRTIM period:
     * f = HRTIM_CLK / (period * prescaler)
     * For Timer E: period = HRTIM_FREQ / f
     * HRTIM_FREQ = 5.44 GHz
     * For 1 kHz: period = 5.44e9 / 1e3 = 5,440,000 → too large for 16-bit
     * Need prescaler: with prescaler 32, effective clock = 170 MHz
     * For 1 kHz: period = 170e6 / 1e3 = 170,000 → still too large
     * With prescaler 128: effective clock = 42.5 MHz
     * For 1 kHz: period = 42,500 → fits!
     * For 100 kHz: period = 425 → fits!
     */

    /* Calculate period for current frequency */
    uint32_t freq = start_hz;
    uint32_t period;
    if (freq >= 650) {
        /* Prescaler 32: 170 MHz / 32 = 5.3125 MHz */
        period = 5312500UL / freq;
    } else {
        /* Prescaler 128: 42.5 MHz / 128 */
        period = 42500000UL / freq;
        if (period > 65535) period = 65535;
    }

    /* Configure Timer E for sweep carrier */
    HRTIM1->TIME_PER = period;
    HRTIM1->TIME_CMP2 = period / 2;  /* 50% duty */

    /* Use Timer E to modulate the H-bridge via external event */
    /* For simplicity, we drive the H-bridge with Timer A gated by Timer E */
    hrtim_set_forward();
    g_bridge_state = BRIDGE_FORWARD;

    /* Start Timer E */
    SET_BIT(HRTIM1->TIME_CR, HRTIM_TIM_CR_CONT);

    /* Start Timer A (gated by Timer E frequency) */
    uint32_t cmp = coil_driver_current_to_cmp(current_ma);
    HRTIM1->TIMA_PER = 0xFFD0;
    HRTIM1->TIMA_CMP2 = cmp;
    SET_BIT(HRTIM1->TIMA_CR, HRTIM_TIM_CR_CONT);

    return 0;
}

void coil_driver_sweep_stop(void)
{
    g_sweep.active = 0;
    HRTIM1->TIME_CR = 0;
    HRTIM1->TIMA_CR = 0;
    HRTIM1->TIMD_CR = 0;

    hrtim_set_brake();
    SET_BIT(HRTIM1->TIMA_CR, HRTIM_TIM_CR_RETRIG);
    HRTIM1->TIMA_PER = 54350;
    for (volatile int i = 0; i < 100; i++) { }
    HRTIM1->TIMA_CR = 0;

    g_bridge_state = BRIDGE_IDLE;
    s_target_current_ma = 0;
}

void coil_driver_sweep_tick(void)
{
    if (!g_sweep.active) return;

    g_sweep.step_timer++;
    if (g_sweep.step_timer < g_sweep.dwell_ms) return;

    g_sweep.step_timer = 0;
    g_sweep.current_step++;

    if (g_sweep.current_step >= g_sweep.steps) {
        /* Sweep complete */
        coil_driver_sweep_stop();
        return;
    }

    /* Calculate next frequency (linear sweep) */
    uint32_t range = g_sweep.stop_hz - g_sweep.start_hz;
    uint32_t freq = g_sweep.start_hz +
                    (range * g_sweep.current_step) / g_sweep.steps;

    /* Update Timer E period */
    uint32_t period;
    if (freq >= 650) {
        period = 5312500UL / freq;
    } else {
        period = 42500000UL / freq;
        if (period > 65535) period = 65535;
    }

    HRTIM1->TIME_PER = period;
    HRTIM1->TIME_CMP2 = period / 2;
}

void coil_driver_pi_update(void)
{
    if (g_bridge_state != BRIDGE_FORWARD && g_bridge_state != BRIDGE_REVERSE) {
        return;
    }
    if (s_target_current_ma == 0) return;

    /* Read current from ADC */
    uint16_t adc_val = adc_read_current();

    /* Convert ADC to milliamps:
     * ACS724ELCTR-30AB: ±30A range, sensitivity = 66 mV/A
     * ADC: 12-bit, 3.3V reference → 0.806 mV per count
     * Current (mA) = (adc_val - offset) * 3300 / 66 / 4095 * 1000
     * Simplified: current_ma = (adc_val - 2048) * 3300000 / (66 * 4095)
     *           = (adc_val - 2048) * 12.2
     */
    int32_t current_ma = (int32_t)adc_val - 2048;
    current_ma = current_ma * 12200 / 1000;  /* mA */
    if (current_ma < 0) current_ma = 0;
    g_status.current_ma = (uint32_t)current_ma;

    /* PI controller: error = target - measured */
    int32_t error = (int32_t)s_target_current_ma - current_ma;

    /* Integral term with anti-windup */
    g_pi.integral += (error * g_pi.ki) / 32768;
    if (g_pi.integral > g_pi.out_max) g_pi.integral = g_pi.out_max;
    if (g_pi.integral < g_pi.out_min) g_pi.integral = g_pi.out_min;

    /* Output = KP * error + integral */
    int32_t output = (error * g_pi.kp) / 32768 + g_pi.integral;
    if (output > g_pi.out_max) output = g_pi.out_max;
    if (output < g_pi.out_min) output = g_pi.out_min;

    /* Update HRTIM compare for duty cycle adjustment */
    HRTIM1->TIMA_CMP2 = (uint32_t)output;
    HRTIM1->TIMD_CMP2 = (uint32_t)output;
}

bridge_state_t coil_driver_get_state(void)
{
    return g_bridge_state;
}

uint32_t coil_driver_get_current_ma(void)
{
    uint16_t adc_val = adc_read_current();
    int32_t current_ma = (int32_t)adc_val - 2048;
    current_ma = current_ma * 12200 / 1000;
    if (current_ma < 0) current_ma = 0;
    return (uint32_t)current_ma;
}

void coil_driver_emergency_stop(void)
{
    /* Immediate hardware cutoff */
    HRTIM1->TIMA_CR = 0;
    HRTIM1->TIMB_CR = 0;
    HRTIM1->TIMC_CR = 0;
    HRTIM1->TIMD_CR = 0;
    HRTIM1->TIME_CR = 0;

    hrtim_disable_outputs();
    CLEAR_BIT(HRTIM1->CR1, HRTIM_CR1_HRTIMEN);

    g_bridge_state = BRIDGE_FAULT;
    s_target_current_ma = 0;
    g_sweep.active = 0;
}

void coil_driver_set_pi_gains(int32_t kp, int32_t ki)
{
    g_pi.kp = kp;
    g_pi.ki = ki;
    g_pi.integral = 0;  /* Reset integral on gain change */
}

void coil_driver_get_pi_gains(int32_t *kp, int32_t *ki)
{
    if (kp) *kp = g_pi.kp;
    if (ki) *ki = g_pi.ki;
}