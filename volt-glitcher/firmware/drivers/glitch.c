/*
 * drivers/glitch.c — Glitch engine driver
 * Voltage/EM/Clock glitch generation with FPGA-assisted sub-ns timing
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#include "glitch.h"
#include "fpga.h"
#include "../board.h"
#include "../registers.h"

/* ========================================================================
 * Internal State
 * ======================================================================== */

static glitch_config_t  g_cfg;
static glitch_stats_t   g_stats;
static volatile uint8_t g_armed = 0;
static volatile uint8_t g_glitch_active = 0;
static int32_t g_vcc_offset_mv = 0;
static int32_t g_current_offset_ma = 0;
static uint8_t  g_calibrated = 0;

/* I2C EEPROM helpers */
static void i2c_start(void) {
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
}

static void i2c_stop(void) {
    I2C1->CR1 |= I2C_CR1_STOP;
    while (I2C1->SR2 & I2C_SR1_BUSY);
}

static void i2c_send_addr(uint8_t addr, uint8_t dir) {
    I2C1->DR = (addr << 1) | dir;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR1;
    (void)I2C1->SR2;  /* Clear ADDR flag */
}

static void i2c_write_byte(uint8_t data) {
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = data;
    while (!(I2C1->SR1 & I2C_SR1_BTF));
}

static uint8_t i2c_read_byte(int last) {
    if (last) {
        I2C1->CR1 &= ~I2C_CR1_ACK;  /* NACK for last byte */
        i2c_stop();
    } else {
        I2C1->CR1 |= I2C_CR1_ACK;
    }
    while (!(I2C1->SR1 & I2C_SR1_RXNE));
    return (uint8_t)(I2C1->DR & 0xFF);
}

/* ========================================================================
 * Configuration Conversion
 * ======================================================================== */

void glitch_config_from_usb(const usb_glitch_config_t *usb_cfg,
                             glitch_config_t *cfg) {
    cfg->mode             = (glitch_mode_t)usb_cfg->mode;
    cfg->shape            = (glitch_shape_t)usb_cfg->shape;
    cfg->trigger_mode     = (trigger_mode_t)usb_cfg->trigger_mode;
    cfg->trigger_edge     = usb_cfg->trigger_edge;
    cfg->delay_ns         = usb_cfg->delay_ns;
    cfg->width_ns         = usb_cfg->width_ns;
    cfg->repeat_count     = usb_cfg->repeat_count;
    cfg->repeat_delay_ns  = usb_cfg->repeat_delay_ns;
    cfg->em_amplitude     = usb_cfg->em_amplitude;
    cfg->clk_phase_offset = usb_cfg->clk_phase_offset;
    cfg->vcc_target_mv    = usb_cfg->vcc_target_mv;
    cfg->max_current_ma   = usb_cfg->max_current_ma;
    cfg->auto_arm         = usb_cfg->auto_arm;
    cfg->safety_limit     = usb_cfg->safety_limit;

    for (int i = 0; i < 4; i++) {
        cfg->uart_pattern[i] = usb_cfg->uart_pattern[i];
    }
    cfg->uart_mask       = usb_cfg->uart_mask;
    cfg->jtag_state       = usb_cfg->jtag_state;
    cfg->gpio_trigger_pin = usb_cfg->gpio_trigger_pin;
}

void glitch_config_to_usb(const glitch_config_t *cfg,
                            usb_glitch_config_t *usb_cfg) {
    usb_cfg->mode             = (uint8_t)cfg->mode;
    usb_cfg->shape            = (uint8_t)cfg->shape;
    usb_cfg->trigger_mode     = (uint8_t)cfg->trigger_mode;
    usb_cfg->trigger_edge     = cfg->trigger_edge;
    usb_cfg->delay_ns         = (uint16_t)cfg->delay_ns;
    usb_cfg->width_ns         = (uint16_t)cfg->width_ns;
    usb_cfg->repeat_count     = cfg->repeat_count;
    usb_cfg->repeat_delay_ns  = (uint16_t)cfg->repeat_delay_ns;
    usb_cfg->em_amplitude     = cfg->em_amplitude;
    usb_cfg->clk_phase_offset = (uint16_t)cfg->clk_phase_offset;
    usb_cfg->vcc_target_mv    = cfg->vcc_target_mv;
    usb_cfg->max_current_ma   = cfg->max_current_ma;
    usb_cfg->auto_arm         = cfg->auto_arm;
    usb_cfg->safety_limit     = cfg->safety_limit;

    for (int i = 0; i < 4; i++) {
        usb_cfg->uart_pattern[i] = cfg->uart_pattern[i];
    }
    usb_cfg->uart_mask       = cfg->uart_mask;
    usb_cfg->jtag_state      = cfg->jtag_state;
    usb_cfg->gpio_trigger_pin = cfg->gpio_trigger_pin;
}

/* ========================================================================
 * Glitch Engine Core
 * ======================================================================== */

void glitch_init(glitch_config_t *cfg) {
    /* Default configuration */
    cfg->mode             = MODE_IDLE;
    cfg->shape            = GLITCH_SHAPE_RECTANGLE;
    cfg->trigger_mode     = TRIG_MANUAL;
    cfg->trigger_edge     = 0;
    cfg->delay_ns         = 0;
    cfg->width_ns         = 100;        /* 100 ns default width */
    cfg->repeat_count     = 1;
    cfg->repeat_delay_ns  = 1000;       /* 1 µs between bursts */
    cfg->em_amplitude     = 512;        /* Half power */
    cfg->clk_phase_offset = 0;
    cfg->vcc_target_mv    = 3300;       /* 3.3V target default */
    cfg->max_current_ma   = 500;
    cfg->auto_arm         = 0;
    cfg->safety_limit     = 1;          /* Safety on by default */

    for (int i = 0; i < 4; i++) {
        cfg->uart_pattern[i] = 0;
    }
    cfg->uart_mask       = 0xFF;
    cfg->jtag_state       = 0;
    cfg->gpio_trigger_pin = 0;

    /* Clear statistics */
    g_stats.total_glitches = 0;
    g_stats.total_triggers = 0;
    g_stats.successful = 0;
    g_stats.faults = 0;
    g_stats.last_vcc_mv = 0;
    g_stats.last_current_ma = 0;
    g_stats.last_delay_ns = 0;
    g_stats.last_width_ns = 0;

    g_armed = 0;
    g_glitch_active = 0;
}

void glitch_arm(const glitch_config_t *cfg) {
    /* Copy config */
    g_cfg = *cfg;

    /* Configure FPGA for the selected mode */
    switch (g_cfg.mode) {
    case MODE_VOLTAGE_GLITCH:
        glitch_setup_voltage(&g_cfg);
        break;
    case MODE_EM_GLITCH:
        glitch_setup_em(&g_cfg);
        break;
    case MODE_CLOCK_GLITCH:
        glitch_setup_clock(&g_cfg);
        break;
    default:
        return;
    }

    /* Configure trigger */
    glitch_setup_trigger(&g_cfg);

    /* Program FPGA timing registers */
    uint32_t delay_units = g_cfg.delay_ns * 10;  /* Convert ns to 100ps units */
    fpga_write_reg(FPGA_REG_TRIG_DELAY_LO, (uint16_t)(delay_units & 0xFFFF));
    fpga_write_reg(FPGA_REG_TRIG_DELAY_HI, (uint16_t)(delay_units >> 16));

    uint32_t width_units = g_cfg.width_ns * 10;
    fpga_write_reg(FPGA_REG_GLITCH_WIDTH_LO, (uint16_t)(width_units & 0xFFFF));
    fpga_write_reg(FPGA_REG_GLITCH_WIDTH_HI, (uint16_t)(width_units >> 16));

    fpga_write_reg(FPGA_REG_GLITCH_SHAPE, (uint16_t)g_cfg.shape);
    fpga_write_reg(FPGA_REG_GLITCH_MODE, (uint16_t)g_cfg.mode);
    fpga_write_reg(FPGA_REG_REPEAT_COUNT, g_cfg.repeat_count);

    uint32_t rep_delay_units = g_cfg.repeat_delay_ns * 10;
    fpga_write_reg(FPGA_REG_REPEAT_DELAY, (uint16_t)(rep_delay_units & 0xFFFF));

    /* Set up ADC watchdog for safety cutoff */
    if (g_cfg.safety_limit && g_cfg.max_current_ma > 0) {
        /* Configure analog watchdog threshold
         * ADC reads target VCC via voltage divider.
         * Overcurrent shows as voltage drop across shunt. */
        uint16_t threshold = (uint16_t)((g_cfg.vcc_target_mv * ADC_FULL_SCALE) / (ADC_VREF * 2));
        ADC1->HTR = threshold;
        ADC1->LTR = 0;
    }

    /* Arm the FPGA */
    fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_ENABLE | FPGA_CTRL_ARM);

    g_armed = 1;
}

void glitch_disarm(void) {
    g_armed = 0;
    g_glitch_active = 0;

    /* Disarm FPGA */
    if (fpga_is_configured()) {
        uint16_t ctrl = fpga_read_reg(FPGA_REG_CTRL);
        ctrl &= ~(FPGA_CTRL_ARM | FPGA_CTRL_ENABLE);
        fpga_write_reg(FPGA_REG_CTRL, ctrl);
    }

    /* Ensure all glitch outputs are off */
    GPIOC->ODR &= ~((1UL << 6) | (1UL << 7) | (1UL << 9) | (1UL << 10));
    GPIOB->ODR &= ~((1UL << 14) | (1UL << 15));

    /* Disable trigger interrupt */
    EXTI->IMR &= ~(1UL << 0);
}

void glitch_fire_manual(void) {
    if (!g_armed) return;

    /* Tell FPGA to fire immediately (no delay) */
    fpga_write_reg(FPGA_REG_CTRL,
        FPGA_CTRL_ENABLE | FPGA_CTRL_ARM | FPGA_CTRL_FIRE_NOW);

    /* Also fire MCU-level glitch output as backup/reinforcement */
    glitch_fire_hardware();
}

void glitch_fire_on_trigger(void) {
    if (!g_armed) return;

    g_stats.total_triggers++;

    /* The FPGA handles the precise delay, but we also set up
     * the MCU timer as a secondary timing mechanism for
     * the case where the FPGA path has more latency than expected. */

    /* For short delays (< 1 µs), use FPGA path exclusively */
    if (g_cfg.delay_ns < 1000) {
        /* FPGA already armed and will handle delay internally */
        /* Just pulse the FIRE_NOW bit to tell FPGA a trigger occurred */
        fpga_write_reg(FPGA_REG_CTRL,
            FPGA_CTRL_ENABLE | FPGA_CTRL_ARM | FPGA_CTRL_FIRE_NOW);
    } else {
        /* For longer delays, use TIM2 for coarse delay then FPGA for fine */
        uint32_t delay_ticks = (g_cfg.delay_ns / 1000UL) * 84UL;  /* 84 MHz = 84 ticks/µs */

        /* Configure TIM2 for one-shot delay */
        TIM2->ARR = delay_ticks;
        TIM2->CNT = 0;
        TIM2->CR1 = TIM_CR1_OPM | TIM_CR1_CEN;  /* One-pulse mode */
    }

    glitch_fire_hardware();
}

/* ========================================================================
 * Hardware Glitch Output
 * ======================================================================== */

static void glitch_assert_output(void) {
    switch (g_cfg.mode) {
    case MODE_VOLTAGE_GLITCH:
        /* Enable gate driver U6 (UCC27511) */
        GPIOC->ODR |= (1UL << 7);   /* PC7 high = driver enable */
        __asm__ volatile ("nop; nop;");  /* Gate driver propagation: ~25ns */
        /* Drive Q1 gate high (SiS426DN MOSFET ON) */
        GPIOC->ODR |= (1UL << 6);   /* PC6 high = shunt VCC to GND */
        break;

    case MODE_EM_GLITCH:
        /* Enable EM coil driver */
        GPIOC->ODR |= (1UL << 10);  /* PC10 high = EM enable */
        __asm__ volatile ("nop; nop;");
        /* Drive Q2 gate (IRLML6344) */
        GPIOC->ODR |= (1UL << 9);   /* PC9 high = EM pulse */
        break;

    case MODE_CLOCK_GLITCH:
        /* Enable clock glitch path */
        GPIOB->ODR |= (1UL << 15);  /* PB15 high = clock glitch enable */
        __asm__ volatile ("nop; nop;");
        /* Drive Q3 gate (BSS84P, active-low for P-ch) */
        GPIOB->ODR &= ~(1UL << 14);  /* PB14 low = Q3 ON (clock stretch) */
        break;

    default:
        break;
    }
}

static void glitch_deassert_output(void) {
    switch (g_cfg.mode) {
    case MODE_VOLTAGE_GLITCH:
        GPIOC->ODR &= ~(1UL << 6);   /* PC6 low = Q1 gate off */
        __asm__ volatile ("nop; nop; nop; nop;");  /* MOSFET turn-off delay */
        GPIOC->ODR &= ~(1UL << 7);   /* PC7 low = gate driver disable */
        break;

    case MODE_EM_GLITCH:
        GPIOC->ODR &= ~(1UL << 9);   /* PC9 low = Q2 gate off */
        __asm__ volatile ("nop; nop;");
        GPIOC->ODR &= ~(1UL << 10);  /* PC10 low = EM disable */
        break;

    case MODE_CLOCK_GLITCH:
        GPIOB->ODR |= (1UL << 14);   /* PB14 high = Q3 OFF (clock released) */
        __asm__ volatile ("nop; nop;");
        GPIOB->ODR &= ~(1UL << 15);  /* PB15 low = clock glitch disable */
        break;

    default:
        break;
    }
}

void glitch_fire_hardware(void) {
    g_glitch_active = 1;

    /* Record pre-glitch measurements */
    g_stats.last_vcc_mv = adc_read_target_vcc_mv();
    g_stats.last_current_ma = adc_read_shunt_current_ma();

    /* If repeat count > 1, fire burst */
    for (uint16_t i = 0; i < g_cfg.repeat_count; i++) {
        glitch_assert_output();

        /* Glitch pulse width timing.
         * For widths < 100 ns: use cycle-accurate NOP loop
         * For widths 100 ns - 65 µs: use TIM2 one-shot
         * For widths > 65 µs: use TIM2 with prescaler */

        if (g_cfg.width_ns < 100) {
            /* Cycle-accurate delay: 168 MHz = ~6 ns/cycle, ~4 cycles per loop iter */
            volatile uint32_t count = g_cfg.width_ns / 6;
            while (count--) {
                __asm__ volatile ("nop");
            }
        } else if (g_cfg.width_ns < 65000) {
            /* Use TIM2 for µs-range pulse width.
             * TIM2 at 84 MHz: 1 tick = ~12 ns
             * Max ARR = 65535 → max ~786 µs */
            uint32_t ticks = (g_cfg.width_ns * 84UL) / 1000UL;
            TIM2->ARR = (ticks > 0xFFFF) ? 0xFFFF : (uint16_t)ticks;
            TIM2->CNT = 0;
            TIM2->CR1 = TIM_CR1_OPM | TIM_CR1_CEN;
            while (TIM2->CR1 & TIM_CR1_CEN);  /* Wait for pulse complete */
        } else {
            /* Long pulse: use TIM2 with /84 prescaler → 1 MHz */
            TIM2->PSC = 83;  /* 84 MHz / 84 = 1 MHz */
            TIM2->ARR = (uint16_t)(g_cfg.width_ns / 1000UL);
            TIM2->CNT = 0;
            TIM2->CR1 = TIM_CR1_OPM | TIM_CR1_CEN;
            while (TIM2->CR1 & TIM_CR1_CEN);
            TIM2->PSC = 0;  /* Restore no prescaler */
        }

        glitch_deassert_output();
        g_stats.total_glitches++;

        /* Inter-pulse delay (if more pulses in burst) */
        if (i < g_cfg.repeat_count - 1 && g_cfg.repeat_delay_ns > 0) {
            if (g_cfg.repeat_delay_ns < 1000) {
                volatile uint32_t d = g_cfg.repeat_delay_ns / 6;
                while (d--) __asm__ volatile ("nop");
            } else {
                delay_us(g_cfg.repeat_delay_ns / 1000);
            }
        }
    }

    g_glitch_active = 0;

    /* Record post-glitch measurements */
    int32_t post_vcc = adc_read_target_vcc_mv();
    int32_t post_current = adc_read_shunt_current_ma();
    (void)post_vcc;
    (void)post_current;

    g_stats.last_width_ns = g_cfg.width_ns;
    g_stats.last_delay_ns = g_cfg.delay_ns;
}

void glitch_emergency_cutoff(void) {
    g_armed = 0;
    g_glitch_active = 0;

    /* Immediately disable all MOSFET gates */
    GPIOC->ODR &= ~((1UL << 6) | (1UL << 7) | (1UL << 9) | (1UL << 10));
    GPIOB->ODR &= ~((1UL << 14) | (1UL << 15));

    /* Disarm FPGA */
    if (fpga_is_configured()) {
        fpga_write_reg(FPGA_REG_CTRL, 0);
    }

    /* Disable all timer outputs */
    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM1->CR1 &= ~TIM_CR1_CEN;

    g_stats.faults++;
    led_error_set(1);
}

/* ========================================================================
 * Mode-Specific Setup
 * ======================================================================== */

void glitch_setup_voltage(const glitch_config_t *cfg) {
    /* Voltage glitch mode: shunt target VCC to GND via Q1 MOSFET
     *
     * Circuit:  VCC_TARGET ──┬── R_shunt ── VCC MCU
     *                       │
     *                      Q1 (SiS426DN, N-ch)
     *                       │
     *                      GND
     *
     * When Q1 gate is driven high, VCC_TARGET is pulled toward GND.
     * The UCC27511 gate driver provides 4A peak gate drive for
     * fast turn-on (typical 15ns rise time into Q1 gate).
     *
     * The FPGA generates the precise timing for when to assert Q1.
     */

    /* Ensure glitch outputs start in safe state */
    GPIOC->ODR &= ~((1UL << 6) | (1UL << 7));

    /* Configure gate driver enable as push-pull output */
    GPIOC->OTYPER &= ~(1UL << 7);

    /* Set very high speed on gate drive pin for minimal latency */
    GPIOC->OSPEEDR &= ~(0x3UL << 12);
    GPIOC->OSPEEDR |= (0x3UL << 12);  /* Very high speed on PC6,7 */

    /* Configure ADC watchdog for target VCC monitoring.
     * We set the low threshold to detect when VCC drops too far. */
    if (cfg->safety_limit) {
        uint16_t vcc_min_adc = (uint16_t)((cfg->vcc_target_mv - 500UL)
            * ADC_FULL_SCALE / (ADC_VREF * 2));
        ADC1->LTR = vcc_min_adc;
    }

    /* Set FPGA to voltage glitch mode */
    fpga_write_reg(FPGA_REG_GLITCH_MODE, (uint16_t)MODE_VOLTAGE_GLITCH);
}

void glitch_setup_em(const glitch_config_t *cfg) {
    /* EM glitch mode: drive current pulse through external EM coil
     *
     * Circuit:  VCC_5V ── EM_COIL ── Q2 (IRLML6344) ── GND
     *
     * The coil generates a brief electromagnetic pulse that can
     * induce faults in nearby ICs. The pulse amplitude is controlled
     * by the em_amplitude DAC value (10-bit, 0-1023).
     *
     * Typical EM coil: 10-20 turns of 0.5mm magnet wire,
     * positioned directly over target IC die.
     */

    /* Ensure EM outputs start off */
    GPIOC->ODR &= ~((1UL << 9) | (1UL << 10));

    /* Set high speed on EM gate pins */
    GPIOC->OSPEEDR &= ~(0xFUL << 18);
    GPIOC->OSPEEDR |= (0xFUL << 18);  /* Very high speed on PC9,10 */

    /* Set DAC value for EM amplitude in FPGA */
    fpga_write_reg(FPGA_REG_DAC_VAL, cfg->em_amplitude);

    /* Set FPGA to EM glitch mode */
    fpga_write_reg(FPGA_REG_GLITCH_MODE, (uint16_t)MODE_EM_GLITCH);
}

void glitch_setup_clock(const glitch_config_t *cfg) {
    /* Clock glitch mode: momentarily stretch/insert clock edges
     *
     * Circuit:  TARGET_CLK ──┬── Q3 (BSS84P, P-ch) ── TARGET_CLK_OUT
     *                        │
     *                   CLK_STRETCH
     *
     * When Q3 gate is driven low (P-ch ON), the clock line is
     * momentarily stretched to GND, causing the target to see
     * an extended clock low period. This can cause skip conditions
     * or pipeline faults in the target MCU.
     *
     * The phase_offset parameter controls which clock edge to glitch.
     * The FPGA synchronizes to the target clock and inserts the
     * stretch at the precise phase angle.
     */

    /* Ensure clock glitch outputs start off */
    GPIOB->ODR |= (1UL << 14);   /* PB14 high = Q3 OFF (P-ch) */
    GPIOB->ODR &= ~(1UL << 15);  /* PB15 low = clock glitch disable */

    /* Set high speed on clock glitch pins */
    GPIOB->OSPEEDR &= ~(0xFUL << 28);
    GPIOB->OSPEEDR |= (0xFUL << 28);

    /* Set phase offset in FPGA */
    fpga_write_reg(FPGA_REG_PHASE_OFFSET,
        (uint16_t)(cfg->clk_phase_offset & 0xFFFF));

    /* Set FPGA to clock glitch mode */
    fpga_write_reg(FPGA_REG_GLITCH_MODE, (uint16_t)MODE_CLOCK_GLITCH);
}

/* ========================================================================
 * Trigger Configuration
 * ======================================================================== */

void glitch_setup_trigger(const glitch_config_t *cfg) {
    /* Disable all trigger sources first */
    EXTI->IMR &= ~(1UL << 0);     /* Disable GPIO EXTI */
    USART1->CR1 &= ~USART_CR1_RXNEIE;  /* Disable UART RX IRQ */

    switch (cfg->trigger_mode) {
    case TRIG_GPIO:
        /* Enable GPIO EXTI on PB0 (TRIG0) */
        if (cfg->trigger_edge == 0) {
            EXTI->RTSR |= (1UL << 0);   /* Rising edge only */
            EXTI->FTSR &= ~(1UL << 0);
        } else if (cfg->trigger_edge == 1) {
            EXTI->RTSR &= ~(1UL << 0);
            EXTI->FTSR |= (1UL << 0);   /* Falling edge only */
        } else {
            EXTI->RTSR |= (1UL << 0);   /* Both edges */
            EXTI->FTSR |= (1UL << 0);
        }
        EXTI->IMR |= (1UL << 0);       /* Enable EXTI0 */
        break;

    case TRIG_UART_PATTERN:
        /* Configure UART trigger pattern matching.
         * The FPGA receives UART bytes via TRIG1 and compares
         * against the configured 4-byte pattern with mask.
         * The MCU also runs a software matcher as backup. */
        fpga_set_uart_pattern(cfg->uart_pattern, cfg->uart_mask);

        /* Enable USART1 RX interrupt for software matcher */
        USART1->CR1 |= USART_CR1_RXNEIE;
        break;

    case TRIG_JTAG_STATE:
        /* Configure JTAG TAP state trigger.
         * The FPGA monitors TCK/TMS/TDI/TDO and tracks the
         * TAP state machine. When the target enters the
         * configured state, it fires the trigger.
         *
         * Common JTAG TAP states:
         * 0: Test-Logic-Reset
         * 1: Run-Test/Idle
         * 2: Select-DR-Scan
         * ... (see IEEE 1149.1) */
        fpga_set_jtag_trigger(cfg->jtag_state);
        break;

    case TRIG_MANUAL:
        /* Manual trigger only — no automatic trigger source.
         * Glitch fires on button press or USB FIRE command. */
        break;

    case TRIG_FPGA_PATTERN:
        /* Complex pattern trigger handled entirely by FPGA.
         * FPGA monitors all trigger inputs simultaneously and
         * applies user-defined boolean logic (AND/OR/SEQ). */
        fpga_set_gpio_trigger(FPGA_GPIO_TRIG_RISING | FPGA_GPIO_TRIG_FALLING);
        break;

    default:
        break;
    }

    /* Set trigger mode in FPGA */
    fpga_write_reg(FPGA_REG_TRIG_MODE, (uint16_t)cfg->trigger_mode);
}

void glitch_enable_trigger(trigger_mode_t mode) {
    switch (mode) {
    case TRIG_GPIO:
        EXTI->IMR |= (1UL << 0);
        break;
    case TRIG_UART_PATTERN:
        USART1->CR1 |= USART_CR1_RXNEIE;
        break;
    default:
        break;
    }
}

void glitch_disable_trigger(trigger_mode_t mode) {
    switch (mode) {
    case TRIG_GPIO:
        EXTI->IMR &= ~(1UL << 0);
        break;
    case TRIG_UART_PATTERN:
        USART1->CR1 &= ~USART_CR1_RXNEIE;
        break;
    default:
        break;
    }
}

/* ========================================================================
 * EEPROM Profile Storage
 * ======================================================================== */

static void eeprom_wait_ready(void) {
    /* Wait for EEPROM write cycle (typical 5ms, max 10ms for CAT24C256) */
    delay_ms(10);
}

static void eeprom_write_bytes(uint16_t addr, const uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        i2c_start();
        i2c_send_addr(EEPROM_I2C_ADDR, 0);  /* Write */
        i2c_write_byte((uint8_t)((addr + i) >> 8));   /* Address high */
        i2c_write_byte((uint8_t)((addr + i) & 0xFF)); /* Address low */
        i2c_write_byte(data[i]);
        i2c_stop();
        eeprom_wait_ready();
    }
}

static void eeprom_read_bytes(uint16_t addr, uint8_t *data, uint16_t len) {
    i2c_start();
    i2c_send_addr(EEPROM_I2C_ADDR, 0);  /* Write (set address) */
    i2c_write_byte((uint8_t)(addr >> 8));
    i2c_write_byte((uint8_t)(addr & 0xFF));

    i2c_start();  /* Repeated start */
    i2c_send_addr(EEPROM_I2C_ADDR, 1);  /* Read */

    for (uint16_t i = 0; i < len; i++) {
        data[i] = i2c_read_byte(i == len - 1);
    }
}

int glitch_save_profile(uint8_t profile_id, const glitch_config_t *cfg,
                         const char *name) {
    if (profile_id >= EEPROM_MAX_PROFILES) return -1;

    eeprom_profile_t prof;
    prof.magic = EEPROM_PROFILE_MAGIC;
    prof.profile_id = profile_id;

    /* Copy name (max 16 chars) */
    int name_len = 0;
    if (name) {
        while (name[name_len] && name_len < 16) {
            prof.name[name_len] = name[name_len];
            name_len++;
        }
    }
    prof.name_len = (uint8_t)name_len;
    while (name_len < 16) {
        prof.name[name_len++] = 0;
    }

    /* Convert config to USB format for storage */
    glitch_config_to_usb(cfg, &prof.config);

    /* Calculate checksum */
    prof.checksum = 0;
    const uint8_t *p = (const uint8_t *)&prof;
    for (size_t i = 0; i < sizeof(prof) - 1; i++) {
        prof.checksum ^= p[i];
    }

    /* Write to EEPROM */
    uint16_t addr = profile_id * EEPROM_PROFILE_SIZE;
    eeprom_write_bytes(addr, (const uint8_t *)&prof, sizeof(prof));

    return 0;
}

int glitch_load_profile(uint8_t profile_id, glitch_config_t *cfg) {
    if (profile_id >= EEPROM_MAX_PROFILES) return -1;

    eeprom_profile_t prof;
    uint16_t addr = profile_id * EEPROM_PROFILE_SIZE;
    eeprom_read_bytes(addr, (uint8_t *)&prof, sizeof(prof));

    /* Verify magic */
    if (prof.magic != EEPROM_PROFILE_MAGIC) return -2;

    /* Verify checksum */
    uint8_t csum = 0;
    const uint8_t *p = (const uint8_t *)&prof;
    for (size_t i = 0; i < sizeof(prof) - 1; i++) {
        csum ^= p[i];
    }
    if (csum != prof.checksum) return -3;

    /* Convert from USB format */
    glitch_config_from_usb(&prof.config, cfg);
    return 0;
}

int glitch_delete_profile(uint8_t profile_id) {
    if (profile_id >= EEPROM_MAX_PROFILES) return -1;

    /* Overwrite with zeros (invalidate magic) */
    uint16_t addr = profile_id * EEPROM_PROFILE_SIZE;
    uint8_t zero = 0;
    eeprom_write_bytes(addr, &zero, 1);
    return 0;
}

int glitch_list_profiles(uint8_t *ids, char names[][16], uint8_t max) {
    uint8_t count = 0;

    for (uint8_t i = 0; i < EEPROM_MAX_PROFILES && count < max; i++) {
        uint16_t addr = i * EEPROM_PROFILE_SIZE;
        uint16_t magic = 0;
        eeprom_read_bytes(addr, (uint8_t *)&magic, 2);

        if (magic == EEPROM_PROFILE_MAGIC) {
            ids[count] = i;
            /* Read name from profile */
            eeprom_profile_t prof;
            eeprom_read_bytes(addr, (uint8_t *)&prof, sizeof(prof));
            for (int j = 0; j < 16; j++) {
                names[count][j] = prof.name[j];
            }
            count++;
        }
    }

    return (int)count;
}

/* ========================================================================
 * Calibration
 * ======================================================================== */

void glitch_calibrate(void) {
    /* Step 1: Measure baseline target VCC (no glitch active) */
    int32_t vcc_sum = 0;
    int32_t curr_sum = 0;
    const int samples = 64;

    for (int i = 0; i < samples; i++) {
        vcc_sum += adc_read_target_vcc_mv();
        curr_sum += adc_read_shunt_current_ma();
        delay_us(100);
    }

    int32_t avg_vcc = vcc_sum / samples;
    int32_t avg_curr = curr_sum / samples;

    /* Step 2: Calculate offsets (expected VCC from config) */
    g_vcc_offset_mv = avg_vcc - (int32_t)g_cfg.vcc_target_mv;
    g_current_offset_ma = avg_curr;

    /* Step 3: Calibrate FPGA delay chain.
     * The FPGA has a delay-locked loop (DLL) that measures
     * the actual delay per tap. We trigger a calibration
     * sequence that measures this and stores correction
     * factors in FPGA registers. */
    if (fpga_is_configured()) {
        /* Trigger FPGA delay chain calibration */
        uint16_t ctrl = fpga_read_reg(FPGA_REG_CTRL);
        ctrl |= 0x0100;  /* Calibration bit (custom) */
        fpga_write_reg(FPGA_REG_CTRL, ctrl);

        /* Wait for calibration complete (FPGA signals via status) */
        uint32_t start = g_tick_ms;
        while (!(fpga_read_reg(FPGA_REG_STATUS) & 0x0100)) {
            if ((g_tick_ms - start) > 2000) break;
        }
    }

    /* Step 4: Set ADC watchdog thresholds based on calibration */
    uint16_t vcc_nominal_adc = (uint16_t)((avg_vcc * ADC_FULL_SCALE) / (ADC_VREF * 2));
    ADC1->HTR = vcc_nominal_adc + 200;  /* High threshold: +200 ADC counts */
    ADC1->LTR = vcc_nominal_adc - 200;  /* Low threshold: -200 ADC counts */

    g_calibrated = 1;
}

int glitch_is_calibrated(void) {
    return g_calibrated;
}

int32_t glitch_get_vcc_offset(void) {
    return g_vcc_offset_mv;
}

int32_t glitch_get_current_offset(void) {
    return g_current_offset_ma;
}

/* ========================================================================
 * Statistics
 * ======================================================================== */

void glitch_get_stats(glitch_stats_t *stats) {
    *stats = g_stats;
}

void glitch_reset_stats(void) {
    g_stats.total_glitches = 0;
    g_stats.total_triggers = 0;
    g_stats.successful = 0;
    g_stats.faults = 0;
}

void glitch_mark_success(void) {
    g_stats.successful++;
}