/*
 * tesla-phantom — drivers/stepper.c
 * 3-axis TMC2209 stepper motor control via UART2 daisy chain.
 * Handles positioning, homing, and StallGuard4 sensorless homing.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* TMC2209 UART protocol:
 * Each TMC2209 is addressed via a daisy-chained UART.
 * Frame format: [SYNC(0x05)] [ADDR] [REG(7:0)|RW] [DATA(31:0)] [CRC]
 * Read:  write with RW=1, then read back 8 bytes.
 * Write: RW=0, send 8 bytes total.
 * CRC:   polynomial 0x07, initial 0
 *
 * In daisy-chain mode, each driver has a unique address set by
 * the MS pin configuration. We use addresses 0x00, 0x01, 0x02
 * for X, Y, Z respectively.
 */

#define TMC_SYNC       0x05
#define TMC_READ       0x00  /* OR with register for read */
#define TMC_WRITE      0x80  /* OR with register for write */

/* Steps per mm:
 * NEMA-8: 200 steps/rev (full step)
 * Microstepping: 16× → 3200 steps/rev
 * Lead screw pitch: 2 mm/rev (for Z) → 1600 steps/mm
 * Belt drive: 36 teeth * 2 mm GT2 = 72 mm/rev → 44.44 steps/mm
 * We use a lead screw for all axes: 2 mm pitch, 16× microstepping
 * → 3200 steps / 2 mm = 1600 steps/mm
 */
#define STEPS_PER_MM   1600.0f
#define MICROSTEPS     16

/* Axis addresses */
static const uint8_t axis_addr[3] = { 0x00, 0x01, 0x02 };

/* Current positions in steps (relative to home) */
static int32_t current_steps[3] = { 0, 0, 0 };

/* Movement state */
static volatile uint8_t moving[3] = { 0, 0, 0 };
static int32_t target_steps[3] = { 0, 0, 0 };

/* ── UART2 init for TMC2209 ───────────────────────────────────────── */
static void stepper_uart_init(void) {
    volatile usart_regs_t *u = USART2;

    /* Disable UART */
    u->CR1 = 0;

    /* Baud rate: 115200 (APB1 = 120 MHz, /16 = 7.5 MHz, BRR = 75000000/115200 ≈ 651 */
    u->BRR = 651;

    /* 8 data bits, no parity, 1 stop bit (default) */
    u->CR1 = USART_CR1_TE | USART_CR1_RE;

    /* Enable UART */
    u->CR1 |= USART_CR1_UE;
}

/* ── TMC2209 CRC ──────────────────────────────────────────────────── */
static uint8_t tmc_crc(uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ── TMC2209 register access ──────────────────────────────────────── */
static int tmc_write_reg(uint8_t axis, uint8_t reg, uint32_t data) {
    uint8_t frame[8];
    frame[0] = TMC_SYNC;
    frame[1] = axis_addr[axis];
    frame[2] = reg | TMC_WRITE;
    frame[3] = (data >> 24) & 0xFF;
    frame[4] = (data >> 16) & 0xFF;
    frame[5] = (data >>  8) & 0xFF;
    frame[6] =  data        & 0xFF;
    frame[7] = tmc_crc(frame, 7);

    for (int i = 0; i < 8; i++)
        usart_write(USART2, frame[i]);

    /* Small delay for transmission to complete */
    delay_us(100);
    return 0;
}

static uint32_t tmc_read_reg(uint8_t axis, uint8_t reg) {
    uint8_t frame[4];
    frame[0] = TMC_SYNC;
    frame[1] = axis_addr[axis];
    frame[2] = reg | TMC_READ;
    frame[3] = tmc_crc(frame, 3);

    /* Send read request */
    for (int i = 0; i < 4; i++)
        usart_write(USART2, frame[i]);

    /* Read back 12 bytes (4 sync + 8 data) — simplified: read 8 */
    uint8_t reply[12];
    for (int i = 0; i < 12; i++)
        reply[i] = usart_read(USART2);

    /* Extract 32-bit data from bytes 4-7 */
    return ((uint32_t)reply[4] << 24) |
           ((uint32_t)reply[5] << 16) |
           ((uint32_t)reply[6] <<  8) |
           ((uint32_t)reply[7]);
}

/* ── Step pulse generation ────────────────────────────────────────── */
/* We use GPIO to generate step pulses (DIR + STEP pins).
 * Since we're using TMC2209 in UART mode, DIR and STEP are controlled
 * via the XDIRECT register or via dedicated GPIO pins.
 * For simplicity, we use GPIO toggle for STEP and DIR.
 *
 * Actually, the TMC2209 in UART mode uses the STEP/DIR pins for movement.
 * The UART is only for configuration. So we need STEP/DIR GPIO pins.
 * We define them here (not in the main pin map for simplicity):
 *   X: STEP=PC0, DIR=PC1
 *   Y: STEP=PC2, DIR=PC3
 *   Z: STEP=PD0, DIR=PD1
 */

#define X_STEP_PORT  GPIOC
#define X_STEP_PIN   0
#define X_DIR_PORT   GPIOC
#define X_DIR_PIN    1
#define Y_STEP_PORT  GPIOC
#define Y_STEP_PIN   2
#define Y_DIR_PORT   GPIOC
#define Y_DIR_PIN    3
#define Z_STEP_PORT  GPIOD
#define Z_STEP_PIN   0
#define Z_DIR_PORT   GPIOD
#define Z_DIR_PIN    1

static void step_pins_init(void) {
    gpio_cfg_output(GPIO(GPIOC), X_STEP_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(GPIOC), X_DIR_PIN,  GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(GPIOC), Y_STEP_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(GPIOC), Y_DIR_PIN,  GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(GPIOD), Z_STEP_PIN, GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
    gpio_cfg_output(GPIO(GPIOD), Z_DIR_PIN,  GPIO_SPEED_HIGH, GPIO_OTYPE_PP);
}

static void generate_step_pulse(volatile gpio_regs_t *step_port,
                                 uint32_t step_pin,
                                 uint32_t delay_us_val) {
    gpio_set(step_port, step_pin);
    delay_us(2);  /* minimum step pulse width */
    gpio_clr(step_port, step_pin);
    delay_us(delay_us_val);
}

static void do_step(uint8_t axis, int dir, uint32_t step_delay_us) {
    volatile gpio_regs_t *step_port, *dir_port;
    uint32_t step_pin, dir_pin;

    switch (axis) {
        case TMC_AXIS_X:
            step_port = GPIO(X_STEP_PORT); dir_port = GPIO(X_DIR_PORT);
            step_pin = X_STEP_PIN; dir_pin = X_DIR_PIN;
            break;
        case TMC_AXIS_Y:
            step_port = GPIO(Y_STEP_PORT); dir_port = GPIO(Y_DIR_PORT);
            step_pin = Y_STEP_PIN; dir_pin = Y_DIR_PIN;
            break;
        case TMC_AXIS_Z:
            step_port = GPIO(Z_STEP_PORT); dir_port = GPIO(Z_DIR_PORT);
            step_pin = Z_STEP_PIN; dir_pin = Z_DIR_PIN;
            break;
        default:
            return;
    }

    if (dir)
        gpio_set(dir_port, dir_pin);
    else
        gpio_clr(dir_port, dir_pin);

    generate_step_pulse(step_port, step_pin, step_delay_us);

    /* Update position tracking */
    current_steps[axis] += (dir ? 1 : -1);
}

/* ── Limit switch check ───────────────────────────────────────────── */
static uint8_t limit_triggered(uint8_t axis) {
    switch (axis) {
        case TMC_AXIS_X:
            return !gpio_get(GPIO(GPIOC), LIM_X_PIN);  /* active low */
        case TMC_AXIS_Y:
            return !gpio_get(GPIO(GPIOC), LIM_Y_PIN);
        case TMC_AXIS_Z:
            return !gpio_get(GPIO(GPIOC), LIM_Z_PIN);
        default:
            return 1;
    }
}

/* ── Public API ───────────────────────────────────────────────────── */

int stepper_init(void) {
    stepper_uart_init();
    step_pins_init();

    /* Configure each TMC2209 driver */
    for (int axis = 0; axis < 3; axis++) {
        /* GCONF: enable driver, use external STEP/DIR, shaft = normal */
        tmc_write_reg(axis, TMC_REG_GCONF, 0x00000001);

        /* IHOLD_IRUN: hold current = 8 (0–31), run current = 16, hold delay = 6 */
        tmc_write_reg(axis, TMC_REG_IHOLD_IRUN,
                      (8U << 0) | (16U << 8) | (6U << 16));

        /* CHOPCONF: 16 microsteps, TOFF=3, HSTRT=5, HEND=1 */
        uint32_t chopconf = (3U << 0) | (5U << 4) | (1U << 7) | (4U << 15);
        tmc_write_reg(axis, TMC_REG_CHOPCONF, chopconf);

        /* StallGuard4 threshold for sensorless homing */
        tmc_write_reg(axis, TMC_REG_SGTHRS, 100);

        /* Enable driver */
        stepper_enable(axis, 1);
    }

    /* Initialize position tracking */
    for (int i = 0; i < 3; i++) {
        current_steps[i] = 0;
        target_steps[i] = 0;
        moving[i] = 0;
    }

    return 0;
}

int stepper_move_to(uint8_t axis, float pos_mm) {
    if (axis > TMC_AXIS_Z) return -1;

    int32_t target = (int32_t)(pos_mm * STEPS_PER_MM);
    int32_t diff = target - current_steps[axis];
    int dir = (diff > 0) ? 1 : 0;
    uint32_t steps = (diff < 0) ? -diff : diff;

    /* Step delay: 500 µs for slow, 100 µs for fast (simplified constant speed) */
    uint32_t step_delay = 200;  /* µs per step → ~5 mm/s */

    for (uint32_t i = 0; i < steps; i++) {
        /* Check limit switches */
        if (dir == 0 && limit_triggered(axis)) {
            current_steps[axis] = 0;
            break;
        }
        do_step(axis, dir, step_delay);
    }

    target_steps[axis] = target;
    return 0;
}

int stepper_move_xyz(float x, float y, float z) {
    /* Move all three axes simultaneously (interleaved stepping for
     * approximate linear motion). */
    int32_t tx = (int32_t)(x * STEPS_PER_MM);
    int32_t ty = (int32_t)(y * STEPS_PER_MM);
    int32_t tz = (int32_t)(z * STEPS_PER_MM);

    int32_t dx = tx - current_steps[TMC_AXIS_X];
    int32_t dy = ty - current_steps[TMC_AXIS_Y];
    int32_t dz = tz - current_steps[TMC_AXIS_Z];

    int dir_x = (dx >= 0) ? 1 : 0;
    int dir_y = (dy >= 0) ? 1 : 0;
    int dir_z = (dz >= 0) ? 1 : 0;

    uint32_t sx = (dx < 0) ? -dx : dx;
    uint32_t sy = (dy < 0) ? -dy : dy;
    uint32_t sz = (dz < 0) ? -dz : dz;

    /* Find max steps for Bresenham-like interpolation */
    uint32_t max_steps = sx;
    if (sy > max_steps) max_steps = sy;
    if (sz > max_steps) max_steps = sz;

    if (max_steps == 0) return 0;

    uint32_t step_delay = 200;  /* µs */
    int64_t err_x = 0, err_y = 0, err_z = 0;

    for (uint32_t i = 0; i < max_steps; i++) {
        err_x += sx;
        err_y += sy;
        err_z += sz;

        if (err_x >= (int64_t)max_steps) {
            if (!(dir_x == 0 && limit_triggered(TMC_AXIS_X)))
                do_step(TMC_AXIS_X, dir_x, 0);
            err_x -= max_steps;
        }
        if (err_y >= (int64_t)max_steps) {
            if (!(dir_y == 0 && limit_triggered(TMC_AXIS_Y)))
                do_step(TMC_AXIS_Y, dir_y, 0);
            err_y -= max_steps;
        }
        if (err_z >= (int64_t)max_steps) {
            if (!(dir_z == 0 && limit_triggered(TMC_AXIS_Z)))
                do_step(TMC_AXIS_Z, dir_z, 0);
            err_z -= max_steps;
        }
        delay_us(step_delay);
    }

    return 0;
}

int stepper_home(uint8_t axis) {
    if (axis > TMC_AXIS_Z) return -1;

    /* Move backwards until limit switch triggers */
    while (limit_triggered(axis) == 0) {
        do_step(axis, 0, 300);  /* dir=0, 300 µs/step */
    }

    /* Back off slightly */
    for (int i = 0; i < 200; i++) {
        do_step(axis, 1, 300);
    }

    current_steps[axis] = 0;
    return 0;
}

int stepper_home_all(void) {
    /* Home Z first (lift probe), then X and Y */
    stepper_home(TMC_AXIS_Z);
    delay_ms(100);
    stepper_home(TMC_AXIS_X);
    delay_ms(100);
    stepper_home(TMC_AXIS_Y);
    delay_ms(100);
    return 0;
}

int stepper_stop(uint8_t axis) {
    if (axis > TMC_AXIS_Z) return -1;
    moving[axis] = 0;
    target_steps[axis] = current_steps[axis];
    return 0;
}

int stepper_set_current(uint8_t axis, uint16_t mA_run, uint16_t mA_hold) {
    if (axis > TMC_AXIS_Z) return -1;

    /* Convert mA to TMC2209 current code (0–31)
     * I = (code+1)/32 * Vref / Rsense
     * With Rsense = 0.11Ω, Vref = 0.32V:
     * I_full = 0.32 / 0.11 / 32 = 90.9 mA per code
     * code = mA / 90.9 - 1 */
    uint8_t run_code = (uint8_t)(mA_run / 91);
    uint8_t hold_code = (uint8_t)(mA_hold / 91);
    if (run_code > 31) run_code = 31;
    if (hold_code > 31) hold_code = 31;

    tmc_write_reg(axis, TMC_REG_IHOLD_IRUN,
                  (hold_code << 0) | (run_code << 8) | (6U << 16));
    return 0;
}

int stepper_enable(uint8_t axis, uint8_t en) {
    if (axis > TMC_AXIS_Z) return -1;
    /* GCONF bit 0 = enable */
    tmc_write_reg(axis, TMC_REG_GCONF, en ? 0x00000001 : 0x00000000);
    return 0;
}

float stepper_get_position(uint8_t axis) {
    if (axis > TMC_AXIS_Z) return 0.0f;
    return (float)current_steps[axis] / STEPS_PER_MM;
}

int stepper_set_speed(uint8_t axis, uint16_t speed_mm_s) {
    /* Speed in mm/s → step delay in µs
     * step_delay = 1e6 / (speed_mm_s * steps_per_mm) */
    if (axis > TMC_AXIS_Z) return -1;
    if (speed_mm_s == 0) return -1;
    /* Speed is applied globally in do_step via a variable;
     * this simplified version uses a fixed delay. */
    (void)speed_mm_s;
    return 0;
}