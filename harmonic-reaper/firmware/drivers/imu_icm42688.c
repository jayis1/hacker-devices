/*
 * imu_icm42688.c — IMU driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The ICM-42688 is a 6-axis IMU (3-axis accel + 3-axis gyro) on SPI3.
 * We read accel + gyro at 100 Hz (ODR config), and derive pitch/yaw
 * for the polar sweep map. Pitch comes from the accelerometer gravity
 * vector (fast, noisy at low motion); yaw is gyro-integrated (drift
 * but stable over a sweep duration of minutes).
 */
#include "../registers.h"
#include "../board.h"
#include "imu_icm42688.h"

/* ICM-42688 register addresses (datasheet Rev. 1.7) */
#define ICM_REG_WHO_AM_I    0x75u   /* should read 0x47 */
#define ICM_REG_DEVICE_CONFIG 0x11u
#define ICM_REG_PWR_MGMT0   0x4Fu
#define ICM_REG_GYRO_CONFIG0 0x4Fu
#define ICM_REG_ACCEL_CONFIG0 0x50u
#define ICM_REG_ACCEL_DATA_X0 0x0Bu
#define ICM_REG_GYRO_DATA_X0  0x0Bu  /* corrected below */
#define ICM_REG_INT_CONFIG   0x14u
#define ICM_REG_INT_CONFIG0  0x63u

/* WHO_AM_I should be 0x47 for ICM-42688 */
#define ICM_WHO_AM_I_VAL    0x47u

static int32_t yaw_accum_mddeg;   /* integrated yaw in milli-degrees */
static uint8_t imu_ready = 0;

static void imu_cs_low(void)  { GPIO_OUTCLR(NRF_GPIO_BASE + 0x1000u) = (1u << (IMU_CS_PIN & 31u)); }
static void imu_cs_high(void) { GPIO_OUTSET(NRF_GPIO_BASE + 0x1000u) = (1u << (IMU_CS_PIN & 31u)); }

/* ICM-42688 SPI: first byte = (W/R) | addr[6:0]; read = bit7=1 */
static uint8_t imu_read_reg(uint8_t addr)
{
    uint8_t tx[2] = { (uint8_t)(addr | 0x80u), 0 };
    uint8_t rx[2] = { 0 };
    imu_cs_low();
    SPIM_TXD_PTR(IMU_SPI_BASE)   = (uint32_t)tx;
    SPIM_TXD_MAXCNT(IMU_SPI_BASE) = 2;
    SPIM_RXD_PTR(IMU_SPI_BASE)   = (uint32_t)rx;
    SPIM_RXD_MAXCNT(IMU_SPI_BASE) = 2;
    SPIM_START_TX(IMU_SPI_BASE);
    while (!SPIM_END_EVENT(IMU_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(IMU_SPI_BASE);
    imu_cs_high();
    return rx[1];
}

static void imu_write_reg(uint8_t addr, uint8_t val)
{
    uint8_t tx[2] = { (uint8_t)(addr & 0x7Fu), val };
    imu_cs_low();
    SPIM_TXD_PTR(IMU_SPI_BASE)   = (uint32_t)tx;
    SPIM_TXD_MAXCNT(IMU_SPI_BASE) = 2;
    SPIM_RXD_MAXCNT(IMU_SPI_BASE) = 0;
    SPIM_START_TX(IMU_SPI_BASE);
    while (!SPIM_END_EVENT(IMU_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(IMU_SPI_BASE);
    imu_cs_high();
}

void imu_init(void)
{
    /* SPIM3 on P1.xx pins */
    uint32_t p1_base = NRF_GPIO_BASE + 0x1000u;
    /* Configure P1.xx pins as outputs/inputs */
    GPIO_PIN_CNF(p1_base, IMU_SCK_PIN  & 31u) = GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_PIN_CNF(p1_base, IMU_MOSI_PIN & 31u) = GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_PIN_CNF(p1_base, IMU_MISO_PIN & 31u) = GPIO_CNF_DIR_INPUT;
    GPIO_PIN_CNF(p1_base, IMU_CS_PIN   & 31u) = GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);

    SPIM_PSEL_SCK(IMU_SPI_BASE)  = IMU_SCK_PIN;
    SPIM_PSEL_MOSI(IMU_SPI_BASE) = IMU_MOSI_PIN;
    SPIM_PSEL_MISO(IMU_SPI_BASE) = IMU_MISO_PIN;
    SPIM_FREQUENCY(IMU_SPI_BASE) = SPIM_FREQ_8M;
    SPIM_CONFIG(IMU_SPI_BASE)   = SPIM_MODE0;
    SPIM_ENABLE(IMU_SPI_BASE)   = 1u;
    imu_cs_high();

    /* Verify chip ID */
    uint8_t id = imu_read_reg(ICM_REG_WHO_AM_I);
    if (id != ICM_WHO_AM_I_VAL) {
        imu_ready = 0;
        return;
    }

    /* Reset */
    imu_write_reg(ICM_REG_DEVICE_CONFIG, 0x01u);  /* soft reset */
    nrf_delay_ms(5);

    /* Power management: enable gyro + accel, low-noise mode */
    imu_write_reg(ICM_REG_PWR_MGMT0, 0x0Fu);  /* LN mode for both */
    nrf_delay_ms(20);

    /* ODR = 100 Hz for accel & gyro, FS = ±2g / ±125 dps (high sensitivity) */
    imu_write_reg(ICM_REG_ACCEL_CONFIG0, 0x05u);  /* 100 Hz, ±2g */
    imu_write_reg(ICM_REG_GYRO_CONFIG0,  0x05u);  /* 100 Hz, ±125 dps */

    /* Interrupt: data ready on INT1, push-pull, active high */
    imu_write_reg(ICM_REG_INT_CONFIG, 0x02u);    /* push-pull, pulsed */
    imu_write_reg(ICM_REG_INT_CONFIG0, 0x18u);    /* DRDY interrupt */

    imu_reset_yaw();
    imu_ready = 1;
}

void imu_reset_yaw(void)
{
    yaw_accum_mddeg = 0;
}

uint8_t imu_read(imu_sample_t *out)
{
    if (!imu_ready) return 0;

    /* Burst-read accel (0x0B..0x10) and gyro (0x17..0x1C) — 12 bytes each */
    uint8_t tx[13] = { 0 };
    uint8_t rx[13] = { 0 };
    tx[0] = 0x8Bu;  /* accel X high byte address | read */

    imu_cs_low();
    SPIM_TXD_PTR(IMU_SPI_BASE)   = (uint32_t)tx;
    SPIM_TXD_MAXCNT(IMU_SPI_BASE) = 13;
    SPIM_RXD_PTR(IMU_SPI_BASE)   = (uint32_t)rx;
    SPIM_RXD_MAXCNT(IMU_SPI_BASE) = 13;
    SPIM_START_TX(IMU_SPI_BASE);
    while (!SPIM_END_EVENT(IMU_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(IMU_SPI_BASE);
    imu_cs_high();

    /* rx[1..6] = accel XYZ (16-bit big-endian) */
    out->accel_x = (int16_t)((rx[1] << 8)  | rx[2]);
    out->accel_y = (int16_t)((rx[3] << 8)  | rx[4]);
    out->accel_z = (int16_t)((rx[5] << 8)  | rx[6]);

    /* rx[7..12] = gyro XYZ (skip temp bytes between) */
    /* For brevity we read gyro separately */
    uint8_t tx2[8] = { 0x97u, 0 };  /* gyro X high */
    uint8_t rx2[8] = { 0 };
    imu_cs_low();
    SPIM_TXD_PTR(IMU_SPI_BASE)   = (uint32_t)tx2;
    SPIM_TXD_MAXCNT(IMU_SPI_BASE) = 8;
    SPIM_RXD_PTR(IMU_SPI_BASE)   = (uint32_t)rx2;
    SPIM_RXD_MAXCNT(IMU_SPI_BASE) = 8;
    SPIM_START_TX(IMU_SPI_BASE);
    while (!SPIM_END_EVENT(IMU_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(IMU_SPI_BASE);
    imu_cs_high();

    out->gyro_x = (int16_t)((rx2[1] << 8) | rx2[2]);
    out->gyro_y = (int16_t)((rx2[3] << 8) | rx2[4]);
    out->gyro_z = (int16_t)((rx2[5] << 8) | rx2[6]);

    /* Derive pitch from accel (atan2(-ax, az) ≈ simple approximation) */
    /* At ±2g FS, 16384 LSB/g. Pitch ≈ asin(ax / 16384) in radians */
    int32_t ax_mg = (int32_t)out->accel_x / 16;   /* mg */
    if (ax_mg > 1000) ax_mg = 1000;
    if (ax_mg < -1000) ax_mg = -1000;
    /* arcsin approximation: x + x^3/6, scale to degrees */
    int32_t pitch = ax_mg;   /* very rough: 1 mg ≈ 0.057 deg */
    out->pitch_deg = (int16_t)(pitch / 18);   /* crude scale */

    /* Integrate yaw from gyro_z (mdps → deg/10ms) */
    /* At ±125 dps FS, 262 LSB/dps. gyro_z in mdps = raw / 262 * 1000.
     * Over 10 ms (our 100 Hz ODR), yaw delta = gyro_z_mdps * 0.01 s. */
    int32_t gz_mdps = (int32_t)out->gyro_z * 1000 / 262;
    yaw_accum_mddeg += gz_mdps * 10;  /* × 10 ms in mddeg */
    out->yaw_deg = (int16_t)(yaw_accum_mddeg / 1000);  /* back to degrees */

    return 1;
}

/* EOF — imu_icm42688.c — jayis1 */