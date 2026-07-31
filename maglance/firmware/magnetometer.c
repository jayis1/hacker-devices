/*
 * magnetometer.c — RM3100 3-axis magnetometer driver implementation
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This driver communicates with three PNI RM3100 3-axis magnetometers
 * via SPI2. The sensors are placed at 0 mm, 15 mm, and 30 mm from the
 * coil face to enable gradient-based magnetic source localization.
 *
 * RM3100 key specs:
 *   - Resolution: ~1 µT (with default cycle count of 200)
 *   - Range: ±800 µT
 *   - Interface: SPI (mode 0, max 4 MHz)
 *   - Data format: 24-bit signed integer per axis
 *   - Conversion: µT = raw * 1.0 / cycle_count (approximate)
 *
 * The RM3100 uses a SPI protocol where the first byte is the register
 * address (bit 7 = R/W: 1=read, 0=write), followed by data bytes.
 */

#include "magnetometer.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ---- Private state ---- */
static mag_array_t s_latest_reading;
static uint16_t s_cycle_count[3] = {200, 200, 200};  /* Default cycle count */
static uint8_t  s_continuous = 0;

/* ---- CS pin mapping ---- */
static GPIO_TypeDef *cs_port[3] = {MAG_CS0_PORT, MAG_CS1_PORT, MAG_CS2_PORT};
static uint8_t       cs_pin[3]  = {MAG_CS0_PIN, MAG_CS1_PIN, MAG_CS2_PIN};

/* ---- SPI helper functions ---- */

static void spi_delay(void)
{
    /* Small delay for SPI timing (min CS high time ~50 ns) */
    for (volatile int i = 0; i < 5; i++) { }
}

static void cs_low(uint8_t sensor_idx)
{
    cs_port[sensor_idx]->BSRR = (1U << (cs_pin[sensor_idx] + 16));
}

static void cs_high(uint8_t sensor_idx)
{
    cs_port[sensor_idx]->BSRR = (1U << cs_pin[sensor_idx]);
}

static void spi_write_byte(uint8_t data)
{
    /* Wait for TXE (transmit buffer empty) */
    while (!(SPI2->SR & SPI_SR_TXE)) { }
    /* Write data (8-bit mode) */
    *(volatile uint8_t *)&SPI2->DR = data;
    /* Wait for RXNE (transfer complete) */
    while (!(SPI2->SR & SPI_SR_RXNE)) { }
    /* Read to clear RXNE */
    (void)SPI2->DR;
}

static uint8_t spi_read_byte(void)
{
    /* Send dummy byte to generate clock */
    while (!(SPI2->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI2->DR = 0xFF;
    /* Wait for response */
    while (!(SPI2->SR & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&SPI2->DR;
}

static void rm3100_write_reg(uint8_t sensor_idx, uint8_t reg, uint8_t value)
{
    cs_low(sensor_idx);
    spi_delay();

    /* Write: address with bit 7 = 0 */
    spi_write_byte(reg & 0x7F);
    spi_write_byte(value);

    spi_delay();
    cs_high(sensor_idx);
}

static uint8_t rm3100_read_reg(uint8_t sensor_idx, uint8_t reg)
{
    cs_low(sensor_idx);
    spi_delay();

    /* Read: address with bit 7 = 1 */
    spi_write_byte(reg | 0x80);
    uint8_t value = spi_read_byte();

    spi_delay();
    cs_high(sensor_idx);
    return value;
}

static int32_t rm3100_read_24bit(uint8_t sensor_idx, uint8_t reg)
{
    cs_low(sensor_idx);
    spi_delay();

    /* Read 3 bytes starting at reg */
    spi_write_byte(reg | 0x80);
    uint8_t b0 = spi_read_byte();
    uint8_t b1 = spi_read_byte();
    uint8_t b2 = spi_read_byte();

    spi_delay();
    cs_high(sensor_idx);

    /* Combine into 24-bit signed value */
    int32_t value = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;
    /* Sign extend from 24-bit to 32-bit */
    if (value & 0x800000) {
        value |= 0xFF000000;
    }
    return value;
}

/* ---- GPIO configuration ---- */

static void gpio_config_spi_pins(void)
{
    /* Configure SCK, MISO, MOSI as AF5 (SPI2) */
    uint32_t moder, afrl;

    /* SCK (PB13) */
    moder = MAG_SPI_PORT->MODER;
    moder &= ~(0x03U << (MAG_SCK_PIN * 2));
    moder |= (GPIO_MODE_AF << (MAG_SCK_PIN * 2));
    MAG_SPI_PORT->MODER = moder;

    afrl = MAG_SPI_PORT->AFRH;  /* Pin 13 → AFRH[5] */
    afrl &= ~(0x0FU << ((MAG_SCK_PIN - 8) * 4));
    afrl |= (0x05U << ((MAG_SCK_PIN - 8) * 4));  /* AF5 = SPI2 */
    MAG_SPI_PORT->AFRH = afrl;

    /* MISO (PB14) */
    moder = MAG_SPI_PORT->MODER;
    moder &= ~(0x03U << (MAG_MISO_PIN * 2));
    moder |= (GPIO_MODE_AF << (MAG_MISO_PIN * 2));
    MAG_SPI_PORT->MODER = moder;

    afrl = MAG_SPI_PORT->AFRH;
    afrl &= ~(0x0FU << ((MAG_MISO_PIN - 8) * 4));
    afrl |= (0x05U << ((MAG_MISO_PIN - 8) * 4));
    MAG_SPI_PORT->AFRH = afrl;

    /* MOSI (PB15) */
    moder = MAG_SPI_PORT->MODER;
    moder &= ~(0x03U << (MAG_MOSI_PIN * 2));
    moder |= (GPIO_MODE_AF << (MAG_MOSI_PIN * 2));
    MAG_SPI_PORT->MODER = moder;

    afrl = MAG_SPI_PORT->AFRH;
    afrl &= ~(0x0FU << ((MAG_MOSI_PIN - 8) * 4));
    afrl |= (0x05U << ((MAG_MOSI_PIN - 8) * 4));
    MAG_SPI_PORT->AFRH = afrl;

    /* Set high speed for SPI pins */
    uint32_t ospeedr = MAG_SPI_PORT->OSPEEDR;
    ospeedr |= (GPIO_SPEED_VHIGH << (MAG_SCK_PIN * 2));
    ospeedr |= (GPIO_SPEED_VHIGH << (MAG_MISO_PIN * 2));
    ospeedr |= (GPIO_SPEED_VHIGH << (MAG_MOSI_PIN * 2));
    MAG_SPI_PORT->OSPEEDR = ospeedr;
}

static void gpio_config_cs_pins(void)
{
    /* Configure CS pins as output, default high (deselected) */
    for (int i = 0; i < 3; i++) {
        uint32_t moder = cs_port[i]->MODER;
        moder &= ~(0x03U << (cs_pin[i] * 2));
        moder |= (GPIO_MODE_OUTPUT << (cs_pin[i] * 2));
        cs_port[i]->MODER = moder;

        /* Push-pull, high speed */
        cs_port[i]->OTYPER &= ~(1U << cs_pin[i]);
        cs_port[i]->OSPEEDR |= (GPIO_SPEED_VHIGH << (cs_pin[i] * 2));

        /* No pull-up/pull-down */
        cs_port[i]->PUPDR &= ~(0x03U << (cs_pin[i] * 2));

        /* Default high (deselected) */
        cs_high(i);
    }
}

/* ---- SPI2 initialization ---- */

static void spi2_init(void)
{
    /* Enable SPI2 clock */
    SET_BIT(RCC->APB1ENR1, RCC_APB1ENR1_SPI2EN);

    /* Configure GPIO pins */
    gpio_config_spi_pins();
    gpio_config_cs_pins();

    /* Disable SPI before configuration */
    CLEAR_BIT(SPI2->CR1, SPI_CR1_SPE);

    /* Configure SPI2:
     * - Master mode
     * - CPOL=0, CPHA=0 (mode 0)
     * - 8-bit data size
     * - Baud rate: PCLK1/16 = 170MHz/16 = 10.625 MHz
     *   (RM3100 max is 4 MHz, so we use /32 = 5.3 MHz — still too fast)
     *   Use /64 = 2.66 MHz — safe for RM3100
     */
    uint32_t cr1 = 0;
    cr1 |= SPI_CR1_MSTR;           /* Master mode */
    cr1 |= SPI_CR1_SSM;             /* Software slave management */
    cr1 |= SPI_CR1_SSI;             /* Internal slave select high */
    cr1 |= SPI_CR1_CPOL;            /* Actually need CPOL=0 — clear this */
    cr1 &= ~SPI_CR1_CPOL;
    cr1 &= ~SPI_CR1_CPHA;
    cr1 |= SPI_CR1_BR_DIV64;       /* /64 = 2.66 MHz */
    cr1 &= ~SPI_CR1_LSBFIRST;      /* MSB first */
    cr1 &= ~SPI_CR1_BIDIMODE;      /* Full duplex */
    SPI2->CR1 = cr1;

    /* CR2: 8-bit data, FRXTH, NSSP */
    uint32_t cr2 = 0;
    cr2 |= SPI_CR2_DS_8BIT;        /* 8-bit data size */
    cr2 |= SPI_CR2_FRXTH;          /* RXNE threshold = 8-bit */
    cr2 |= SPI_CR2_SSOE;           /* SS output enable */
    SPI2->CR2 = cr2;

    /* Enable SPI */
    SET_BIT(SPI2->CR1, SPI_CR1_SPE);
}

/* ---- Public API ---- */

int magnetometer_init(void)
{
    /* Initialize SPI2 */
    spi2_init();

    /* Small delay for sensors to stabilize */
    for (volatile int i = 0; i < 10000; i++) { }

    /* Verify each sensor's revision ID */
    for (int i = 0; i < 3; i++) {
        uint8_t revid = rm3100_read_reg(i, RM3100_REVID);
        if (revid != RM3100_REVID_EXPECTED) {
            /* Sensor not responding or wrong revision */
            return -1;
        }

        /* Set default cycle count (200 → ~1 µT resolution) */
        rm3100_write_reg(i, RM3100_CCX, (s_cycle_count[i] >> 8) & 0xFF);
        rm3100_write_reg(i, RM3100_CCX + 1, s_cycle_count[i] & 0xFF);
        rm3100_write_reg(i, RM3100_CCY, (s_cycle_count[i] >> 8) & 0xFF);
        rm3100_write_reg(i, RM3100_CCY + 1, s_cycle_count[i] & 0xFF);
        rm3100_write_reg(i, RM3100_CCZ, (s_cycle_count[i] >> 8) & 0xFF);
        rm3100_write_reg(i, RM3100_CCZ + 1, s_cycle_count[i] & 0xFF);

        /* Set update rate to 150 Hz (good balance of speed and noise) */
        rm3100_write_reg(i, RM3100_TMRC, RM3100_TMRC_150HZ);

        /* Start continuous measurement on all axes */
        uint8_t cmm = RM3100_CMM_START | RM3100_CMM_ALL_AXES;
        rm3100_write_reg(i, RM3100_CMM, cmm);
    }

    s_continuous = 1;
    memset(&s_latest_reading, 0, sizeof(s_latest_reading));
    return 0;
}

int magnetometer_read(uint8_t sensor_idx, mag_reading_t *reading)
{
    if (sensor_idx > 2 || reading == NULL) {
        return -1;
    }

    /* If not in continuous mode, trigger a single measurement */
    if (!s_continuous) {
        rm3100_write_reg(sensor_idx, RM3100_POLL, RM3100_POLL_ALL);
        /* Wait for data ready */
        uint32_t timeout = 100000;
        while (!(rm3100_read_reg(sensor_idx, RM3100_STATUS) &
                 RM3100_STATUS_DRDY)) {
            if (--timeout == 0) return -1;
        }
    }

    /* Read 24-bit raw values for each axis */
    int32_t raw_x = rm3100_read_24bit(sensor_idx, RM3100_MX);
    int32_t raw_y = rm3100_read_24bit(sensor_idx, RM3100_MY);
    int32_t raw_z = rm3100_read_24bit(sensor_idx, RM3100_MZ);

    /* Convert to centi-µT (µT × 100) */
    reading->x = magnetometer_raw_to_centiut(raw_x, s_cycle_count[sensor_idx]);
    reading->y = magnetometer_raw_to_centiut(raw_y, s_cycle_count[sensor_idx]);
    reading->z = magnetometer_raw_to_centiut(raw_z, s_cycle_count[sensor_idx]);

    return 0;
}

int magnetometer_read_all(mag_array_t *reading)
{
    if (reading == NULL) return -1;

    for (int i = 0; i < 3; i++) {
        if (magnetometer_read(i, &reading->sensor[i]) != 0) {
            return -1;
        }
    }

    /* Update cached reading */
    reading->timestamp_ms = 0;  /* TODO: use SysTick */
    s_latest_reading = *reading;

    /* Also update global status */
    for (int i = 0; i < 3; i++) {
        g_status.field_x[i] = reading->sensor[i].x;
        g_status.field_y[i] = reading->sensor[i].y;
        g_status.field_z[i] = reading->sensor[i].z;
    }

    return 0;
}

int magnetometer_start_continuous(uint8_t tmrc_rate)
{
    for (int i = 0; i < 3; i++) {
        rm3100_write_reg(i, RM3100_TMRC, tmrc_rate);
        uint8_t cmm = RM3100_CMM_START | RM3100_CMM_ALL_AXES;
        rm3100_write_reg(i, RM3100_CMM, cmm);
    }
    s_continuous = 1;
    return 0;
}

void magnetometer_stop_continuous(void)
{
    for (int i = 0; i < 3; i++) {
        rm3100_write_reg(i, RM3100_CMM, 0x00);
    }
    s_continuous = 0;
}

int magnetometer_data_ready(uint8_t sensor_idx)
{
    if (sensor_idx > 2) return 0;
    return (rm3100_read_reg(sensor_idx, RM3100_STATUS) &
            RM3100_STATUS_DRDY) ? 1 : 0;
}

int16_t magnetometer_raw_to_centiut(int32_t raw, uint16_t cycle_count)
{
    /*
     * RM3100 conversion:
     * The raw 24-bit value is in units of LSB.
     * The sensitivity is: 1 LSB = 1 / cycle_count µT (approximately)
     * More precisely: gain = cycle_count * 0.366 nT/LSB
     * So: µT = raw * 0.366 / cycle_count
     *
     * For centi-µT: centi_µT = raw * 36.6 / cycle_count
     *
     * With cycle_count = 200:
     *   1 LSB = 0.366 * 200 / 200 = 0.366 nT = 0.0366 centi-µT
     *   So 1 µT ≈ 27.3 LSB
     *
     * Simplified: centi_µT = raw * 3660 / (cycle_count * 100)
     *             = raw * 366 / (cycle_count * 10)
     *             = raw * 3660 / cycle_count / 10
     */
    if (cycle_count == 0) return 0;

    /* Convert to centi-µT (µT × 100) */
    int32_t centi_ut = (raw * 3660) / ((int32_t)cycle_count * 10);

    /* Clamp to int16 range (±327.68 µT) */
    if (centi_ut > 32767) centi_ut = 32767;
    if (centi_ut < -32768) centi_ut = -32768;

    return (int16_t)centi_ut;
}

void magnetometer_calculate_gradient(const mag_array_t *reading,
                                      float *grad_x,
                                      float *grad_y,
                                      float *grad_z)
{
    /*
     * Calculate the magnetic field gradient between sensors.
     * Sensors are at 0 mm, 15 mm, and 30 mm from the coil face.
     *
     * Gradient (in µT/mm) between each pair:
     *   grad_01 = (field[0] - field[1]) / 15 mm
     *   grad_12 = (field[1] - field[2]) / 15 mm
     *
     * Average gradient is used for source localization.
     *
     * The gradient direction indicates the direction to the
     * magnetic source — useful for passive target location.
     */

    if (reading == NULL || grad_x == NULL ||
        grad_y == NULL || grad_z == NULL) {
        return;
    }

    /* Convert centi-µT to µT for gradient calculation */
    float f0x = reading->sensor[0].x / 100.0f;
    float f0y = reading->sensor[0].y / 100.0f;
    float f0z = reading->sensor[0].z / 100.0f;
    float f1x = reading->sensor[1].x / 100.0f;
    float f1y = reading->sensor[1].y / 100.0f;
    float f1z = reading->sensor[1].z / 100.0f;
    float f2x = reading->sensor[2].x / 100.0f;
    float f2y = reading->sensor[2].y / 100.0f;
    float f2z = reading->sensor[2].z / 100.0f;

    /* Average gradient across the two intervals (0→15, 15→30 mm) */
    *grad_x = ((f0x - f1x) + (f1x - f2x)) / (2.0f * 15.0f);
    *grad_y = ((f0y - f1y) + (f1y - f2y)) / (2.0f * 15.0f);
    *grad_z = ((f0z - f1z) + (f1z - f2z)) / (2.0f * 15.0f);
}

int magnetometer_self_test(uint8_t sensor_idx)
{
    if (sensor_idx > 2) return -1;

    /* Start self-test for all axes */
    uint8_t bist = RM3100_BIST_START | RM3100_BIST_X |
                   RM3100_BIST_Y | RM3100_BIST_Z;
    rm3100_write_reg(sensor_idx, RM3100_BIST, bist);

    /* Wait for self-test to complete */
    uint32_t timeout = 100000;
    while (rm3100_read_reg(sensor_idx, RM3100_BIST) & RM3100_BIST_START) {
        if (--timeout == 0) return -1;
    }

    /* Read results — if all axes passed, BIST register should be 0 */
    uint8_t result = rm3100_read_reg(sensor_idx, RM3100_BIST);
    return (result == 0) ? 0 : -1;
}

int magnetometer_set_cycle_count(uint8_t sensor_idx, uint16_t count)
{
    if (sensor_idx > 2 || count < 20 || count > 65535) {
        return -1;
    }

    s_cycle_count[sensor_idx] = count;

    /* Write cycle count registers (16-bit, MSB first) */
    rm3100_write_reg(sensor_idx, RM3100_CCX, (count >> 8) & 0xFF);
    rm3100_write_reg(sensor_idx, RM3100_CCX + 1, count & 0xFF);
    rm3100_write_reg(sensor_idx, RM3100_CCY, (count >> 8) & 0xFF);
    rm3100_write_reg(sensor_idx, RM3100_CCY + 1, count & 0xFF);
    rm3100_write_reg(sensor_idx, RM3100_CCZ, (count >> 8) & 0xFF);
    rm3100_write_reg(sensor_idx, RM3100_CCZ + 1, count & 0xFF);

    return 0;
}

const mag_array_t *magnetometer_get_latest(void)
{
    return &s_latest_reading;
}