/*
 * drivers/drfm.c — DRFM FPGA SPI protocol (iCE40UP5K)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements the control interface to the iCE40UP5K FPGA that holds the
 * digital RF memory: delay (range), Doppler NCO (velocity), amplitude
 * (RCS), and multi-tap target generation. Also handles FPGA configuration
 * (CRESET/CDONE handshake) and status polling.
 */
#include "../board.h"
#include "../registers.h"

/* ---- Physical constants -------------------------------------------- */
/* Range -> round-trip delay:  t = 2*R / c
 * With 5 ns delay resolution (DRFM clock = 200 MHz): 1 delay unit = 5 ns
 * Per unit range:  2*R / c = 2*R[m] / 3e8 = R * 6.667e-9 s
 * In 5 ns units:    delay_units = 2 * R[m] / (3e8 * 5e-9) = R * 4/3 / 100
 * Simplified:       delay_units = (4 * range_cm) / 6000  (approx R[m]*1.333)
 * We use:           delay_units = (range_cm * 6667) / 10000000
 */
#define SPEED_OF_LIGHT_CMPS   29979245800ULL
#define DRFM_CLK_HZ            200000000ULL
#define DRFM_DELAY_UNIT_PS     5000ULL     /* 5 ns = 5000 ps */

/* Doppler: f_d = 2*v/lambda, lambda @ 77 GHz = 3.896 mm
 * f_d (Hz) = 2 * v[mm/s] / 3.896mm = v * 0.5133
 * NCO stored in milli-Hz: dop_mHz = v_mmps * 513.3
 */
#define DOPPLER_HZ_PER_MPS    513.3       /* @77 GHz */
#define DOPPLER_MILLIHZ(v_mmps) (int32_t)((v_mmps) * 513.3)

/* ---- FPGA configuration ------------------------------------------- */
void drfm_reset(void)
{
    spi_cs_high(DRFM_NSS_PORT, DRFM_NSS_PIN);
    GPIO_BSRR(DRFM_CRESET_PORT) = GPIO_BR(DRFM_CRESET_PIN); /* low */
    board_delay_ms(5);
    GPIO_BSRR(DRFM_CRESET_PORT) = GPIO_BS(DRFM_CRESET_PIN); /* high */
    board_delay_ms(50);   /* let FPGA load from flash */
}

uint8_t drfm_is_ready(void)
{
    return (GPIO_IDR(DRFM_CDONE_PORT) & GPIO_PIN(DRFM_CDONE_PIN)) ? 1 : 0;
}

/* ---- Low-level register access ------------------------------------ */
static void drfm_write_raw(uint8_t addr, const uint8_t *data, uint8_t n)
{
    spi_cs_low(DRFM_NSS_PORT, DRFM_NSS_PIN);
    spi_xfer(SPI2_BASE, 0x02);          /* write command */
    spi_xfer(SPI2_BASE, addr);
    for (uint8_t i = 0; i < n; i++)
        spi_xfer(SPI2_BASE, data[i]);
    spi_cs_high(DRFM_NSS_PORT, DRFM_NSS_PIN);
}

static void drfm_read_raw(uint8_t addr, uint8_t *data, uint8_t n)
{
    spi_cs_low(DRFM_NSS_PORT, DRFM_NSS_PIN);
    spi_xfer(SPI2_BASE, 0x03);          /* read command */
    spi_xfer(SPI2_BASE, addr);
    for (uint8_t i = 0; i < n; i++) {
        spi_xfer(SPI2_BASE, 0x00);
        data[i] = SPI_DR(SPI2_BASE) & 0xFF;
    }
    spi_cs_high(DRFM_NSS_PORT, DRFM_NSS_PIN);
}

void drfm_write_u8(uint8_t addr, uint8_t val)
{
    drfm_write_raw(addr, &val, 1);
}

uint8_t drfm_read_u8(uint8_t addr)
{
    uint8_t v;
    drfm_read_raw(addr, &v, 1);
    return v;
}

void drfm_write_u24(uint8_t addr, uint32_t val)
{
    uint8_t buf[3] = {
        (uint8_t)(val & 0xFF),
        (uint8_t)((val >> 8) & 0xFF),
        (uint8_t)((val >> 16) & 0xFF)
    };
    drfm_write_raw(addr, buf, 3);
}

uint32_t drfm_read_u24(uint8_t addr)
{
    uint8_t buf[3] = {0, 0, 0};
    drfm_read_raw(addr, buf, 3);
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16);
}

/* ---- Status & control --------------------------------------------- */
uint8_t drfm_status(void)
{
    return drfm_read_u8(DRFM_REG_STATUS);
}

void drfm_arm(uint8_t armed)
{
    uint8_t ctrl = drfm_read_u8(DRFM_REG_CTRL);
    if (armed) ctrl |= 0x01;
    else       ctrl &= ~0x01;
    drfm_write_u8(DRFM_REG_CTRL, ctrl);
}

void drfm_clear(void)
{
    uint8_t ctrl = drfm_read_u8(DRFM_REG_CTRL);
    ctrl |= 0x02;            /* assert clear */
    drfm_write_u8(DRFM_REG_CTRL, ctrl);
    board_delay_ms(1);
    ctrl &= ~0x02;
    drfm_write_u8(DRFM_REG_CTRL, ctrl);
}

void drfm_noise_mode(uint8_t on)
{
    uint8_t ctrl = drfm_read_u8(DRFM_REG_CTRL);
    if (on) ctrl |= 0x04;
    else    ctrl &= ~0x04;
    drfm_write_u8(DRFM_REG_CTRL, ctrl);
}

/* ---- Range (delay) ------------------------------------------------- */
/* Convert range in cm to 24-bit delay value (5 ns units) */
static uint32_t range_cm_to_delay(uint32_t range_cm)
{
    /* t_ps = 2 * range_cm / c * 1e10 ps/m ... we compute:
     * delay_units = t_ps / 5000ps = 2*range_cm*1e10 / (c_cm_per_s * 5000)
     *            = 2*range_cm*1e10 / (3e10*5000) = range_cm * 2e10 / 1.5e14
     *            = range_cm / 7500
     */
    if (range_cm > RP_MAX_RANGE_CM) range_cm = RP_MAX_RANGE_CM;
    uint64_t d = (uint64_t)range_cm * 20000000000ULL
               / (SPEED_OF_LIGHT_CMPS * DRFM_DELAY_UNIT_PS);
    if (d > 0xFFFFFF) d = 0xFFFFFF;
    return (uint32_t)d;
}

void drfm_set_range_cm(uint32_t range_cm)
{
    uint32_t d = range_cm_to_delay(range_cm);
    drfm_write_u24(DRFM_REG_DELAY_LSB, d);
}

/* ---- Velocity (Doppler NCO) --------------------------------------- */
void drfm_set_velocity_mmps(int32_t vel_mmps)
{
    if (vel_mmps >  (int32_t)RP_MAX_VEL_MPS) vel_mmps =  (int32_t)RP_MAX_VEL_MPS;
    if (vel_mmps < -(int32_t)RP_MAX_VEL_MPS) vel_mmps = -(int32_t)RP_MAX_VEL_MPS;
    int32_t mhz = DOPPLER_MILLIHZ(vel_mmps);
    /* convert to unsigned 24-bit (two's complement) */
    uint32_t u = (uint32_t)mhz & 0xFFFFFF;
    drfm_write_u24(DRFM_REG_DOPP_LSB, u);
}

/* ---- RCS (amplitude) ---------------------------------------------- */
/* RCS in q0.8 dBsm: gain = 10^((rcs_db / 10)) mapped to 0..255 */
void drfm_set_rcs_qdb(int16_t rcs_qdb)
{
    if (rcs_qdb > RP_MAX_RCS_QDB)  rcs_qdb = RP_MAX_RCS_QDB;
    if (rcs_qdb < RP_MIN_RCS_QDB)  rcs_qdb = RP_MIN_RCS_QDB;
    /* gain_q8 = 10^((rcs_qdb/256)/10) * 128
     * Approximate using lookup; here a coarse linear mapping for robustness.
     * rcs -40..+20 dBsm -> gain 1..255
     */
    int32_t span = RP_MAX_RCS_QDB - RP_MIN_RCS_QDB;       /* 480 */
    int32_t idx = rcs_qdb - RP_MIN_RCS_QDB;               /* 0..480 */
    uint8_t gain = (uint8_t)((idx * 255) / span);
    if (gain == 0) gain = 1;   /* never zero — keep a return */
    drfm_write_u8(DRFM_REG_GAIN, gain);
}

/* ---- Multi-tap target generation ---------------------------------- */
typedef struct {
    uint32_t rel_delay;     /* in 5 ns units relative to base */
    int32_t  rel_doppler;   /* milli-Hz relative offset */
    uint8_t  gain;          /* 0..255 */
} drfm_tap_t;

void drfm_set_taps(uint8_t n_taps, const drfm_tap_t *taps)
{
    if (n_taps > 4) n_taps = 4;
    drfm_write_u8(DRFM_REG_NTAPS, n_taps);
    /* Tap register bases: each tap has delay(3)+gain(1)+doppler(3) = 7 regs */
    static const uint8_t tap_base[4] = {
        DRFM_REG_TAP0_DLY, DRFM_REG_TAP1_DLY,
        DRFM_REG_TAP2_DLY, DRFM_REG_TAP3_DLY
    };
    for (uint8_t i = 0; i < n_taps; i++) {
        uint8_t base = tap_base[i];
        drfm_write_u24(base,     taps[i].rel_delay);
        drfm_write_u8 (base + 3, taps[i].gain);
        drfm_write_u24(base + 4, (uint32_t)taps[i].rel_doppler & 0xFFFFFF);
    }
}

/* ---- Micro-Doppler modulation ------------------------------------- */
/* Pedestrian gait: leg-swing harmonics at ~1 Hz stride, ~3 Hz swing */
void drfm_set_microdoppler(uint8_t enable, uint8_t depth)
{
    drfm_write_u8(DRFM_REG_MD_SRC,  enable ? 1 : 0);
    drfm_write_u8(DRFM_REG_MD_DEPTH, depth);
}

/* ---- Bulk configure from a single phantom descriptor --------------- */
typedef struct {
    uint32_t range_cm;
    int32_t  vel_mmps;
    int16_t  rcs_qdb;
    uint8_t  n_taps;
    uint8_t  noise_mode;
    uint8_t  microdoppler;
} phantom_desc_t;

void drfm_configure(const phantom_desc_t *p)
{
    drfm_noise_mode(p->noise_mode);
    drfm_set_range_cm(p->range_cm);
    drfm_set_velocity_mmps(p->vel_mmps);
    drfm_set_rcs_qdb(p->rcs_qdb);
    drfm_set_microdoppler(p->microdoppler ? 1 : 0, p->microdoppler);
    /* taps configured separately via drfm_set_taps() for multi-target */
}

/* ---- Self-test ----------------------------------------------------- */
uint8_t drfm_selftest(void)
{
    drfm_reset();
    board_delay_ms(60);
    if (!drfm_is_ready()) return 0;
    /* write/readback a scratch via STATUS (read-only) — instead test CTRL */
    drfm_write_u8(DRFM_REG_GAIN, 0x5A);
    uint8_t rb = drfm_read_u8(DRFM_REG_GAIN);
    return (rb == 0x5A) ? 1 : 0;
}