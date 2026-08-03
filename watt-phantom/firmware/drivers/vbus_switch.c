/*
 * vbus_switch.c — VBUS power path control (eFuse, MOSFET, boost converter)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls the TPS25982 eFuses and SISS740DN MOSFETs that manage the
 * VBUS power path between the source and sink USB-C ports. Also controls
 * the internal boost converter for overvoltage attacks.
 *
 * The TPS25982 eFuse provides:
 *  - Hardware overvoltage protection (OVP) — programmable via I²C
 *  - Hardware overcurrent protection (OCP) — programmable via I²C
 *  - Slew rate control for VBUS ramp
 *  - Reverse current blocking
 *  - Thermal shutdown
 *
 * The boost converter can generate up to 48V from a 5V input for
 * overvoltage attack scenarios (attack mode only).
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* ---- GPIO pin definitions ---- */
#define EFUSE_SRC_EN_PIN    0   /* PB0 */
#define EFUSE_SNK_EN_PIN    1   /* PB1 */
#define MOSFET1_EN_PIN      2   /* PB2 */
#define MOSFET2_EN_PIN      3   /* PB3 */
#define BOOST_EN_PIN        8   /* PB8 */

/* ---- TPS25982 I²C addresses ---- */
#define TPS25982_SRC_ADDR   0x42u  /* 7-bit: 0x21 → 8-bit W: 0x42 */
#define TPS25982_SNK_ADDR   0x44u  /* 7-bit: 0x22 → 8-bit W: 0x44 */

/* TPS25982 register map (key registers) */
#define TPS25982_REG_CONTROL    0x00u
#define TPS25982_REG_PROTECT    0x01u
#define TPS25982_REG_OVP        0x02u
#define TPS25982_REG_OCP        0x03u
#define TPS25982_REG_SLEW       0x04u
#define TPS25982_REG_STATUS     0x05u
#define TPS25982_REG_FAULT      0x06u
#define TPS25982_REG_VIN        0x08u
#define TPS25982_REG_IOUT       0x0Au

/* OVP threshold encoding (mV): register value = (threshold - 2000) / 100 */
static uint8_t ovp_to_reg(uint16_t ovp_mv) {
    if (ovp_mv < 2000) return 0;
    if (ovp_mv > 23000) return 0xFF;
    return (uint8_t)((ovp_mv - 2000u) / 100u);
}

/* OCP threshold encoding (mA): register value = (threshold - 500) / 50 */
static uint8_t ocp_to_reg(uint16_t ocp_ma) {
    if (ocp_ma < 500) return 0;
    if (ocp_ma > 13000) return 0xFF;
    return (uint8_t)((ocp_ma - 500u) / 50u);
}

/* ---- I²C bus for each port's eFuse ---- */
static uint32_t s_efuse_bus[PD_PORT_COUNT] = { I2C1_BUS, I2C2_BUS };
static uint8_t s_efuse_addr[PD_PORT_COUNT] = { TPS25982_SRC_ADDR, TPS25982_SNK_ADDR };

/* ---- GPIO write helper ---- */
static void gpio_write_pb(uint8_t pin, uint8_t val) {
    volatile uint32_t *bsrr = (volatile uint32_t *)(GPIOB_BASE + GPIO_BSRR_OFFSET);
    if (val) {
        *bsrr = (1u << pin);
    } else {
        *bsrr = (1u << (pin + 16));
    }
}

/* ---- VBUS init ---- */
void vbus_init(void) {
    /* Disable both eFuses and MOSFETs initially */
    gpio_write_pb(EFUSE_SRC_EN_PIN, 0);
    gpio_write_pb(EFUSE_SNK_EN_PIN, 0);
    gpio_write_pb(MOSFET1_EN_PIN, 0);
    gpio_write_pb(MOSFET2_EN_PIN, 0);
    gpio_write_pb(BOOST_EN_PIN, 0);

    /* Configure eFuse default OVP/OCP via I²C */
    for (int p = 0; p < PD_PORT_COUNT; p++) {
        uint8_t ovp = ovp_to_reg(DEFAULT_OVP_MV);
        uint8_t ocp = ocp_to_reg(DEFAULT_OCP_MA);

        /* Set OVP */
        i2c_write_reg(s_efuse_bus[p], s_efuse_addr[p],
                      TPS25982_REG_OVP, &ovp, 1);
        /* Set OCP */
        i2c_write_reg(s_efuse_bus[p], s_efuse_addr[p],
                      TPS25982_REG_OCP, &ocp, 1);

        /* Set slew rate to 2.5 V/ms (safe default) */
        uint8_t slew = 0x04u;
        i2c_write_reg(s_efuse_bus[p], s_efuse_addr[p],
                      TPS25982_REG_SLEW, &slew, 1);

        /* Enable continuous monitoring */
        uint8_t ctrl = 0x01u; /* MONITOR_EN */
        i2c_write_reg(s_efuse_bus[p], s_efuse_addr[p],
                      TPS25982_REG_CONTROL, &ctrl, 1);
    }
}

/* ---- Set eFuse OVP/OCP limits ---- */
void vbus_set_efuse_limits(pd_port_t port, uint16_t ovp_mv, uint16_t ocp_ma) {
    uint8_t ovp = ovp_to_reg(ovp_mv);
    uint8_t ocp = ocp_to_reg(ocp_ma);

    i2c_write_reg(s_efuse_bus[port], s_efuse_addr[port],
                  TPS25982_REG_OVP, &ovp, 1);
    i2c_write_reg(s_efuse_bus[port], s_efuse_addr[port],
                  TPS25982_REG_OCP, &ocp, 1);
}

/* ---- Enable/disable VBUS path for a port ---- */
void vbus_enable_path(pd_port_t port, uint8_t enable) {
    if (port == PD_PORT_SOURCE) {
        gpio_write_pb(EFUSE_SRC_EN_PIN, enable);
        gpio_write_pb(MOSFET1_EN_PIN, enable);
    } else {
        gpio_write_pb(EFUSE_SNK_EN_PIN, enable);
        gpio_write_pb(MOSFET2_EN_PIN, enable);
    }
}

/* ---- Disconnect VBUS for a port ---- */
void vbus_disconnect(pd_port_t port) {
    vbus_enable_path(port, 0);
}

/* ---- Set VBUS voltage (via boost converter or PD negotiation) ---- */
int vbus_set_voltage(pd_port_t port, uint16_t target_mv) {
    /* Safety: check OVP limit */
    extern device_state_t g_state;
    if (!g_state.safety.attack_mode_enabled &&
        target_mv > g_state.safety.ovp_threshold_mv) {
        return -1;
    }

    if (target_mv <= 5000) {
        /* 5V: just pass through from source, disable boost */
        boost_enable(0);
        vbus_enable_path(port, 1);
        return 0;
    }

    if (target_mv <= 20000) {
        /* 5-20V: request from source via PD, or use boost converter */
        /* Try PD first: request the closest PDO from the source */
        uint32_t pdo = MAKE_FIXED_PDO(target_mv, 3000);
        /* Send a request for this voltage */
        /* In a real implementation, we'd negotiate with the source.
         * For the attack scenario, we use the boost converter. */
        boost_enable(1);
        boost_set_voltage(target_mv);
        vbus_enable_path(port, 1);
        return 0;
    }

    /* >20V: must use boost converter (PD 3.1 EPR) */
    if (!g_state.safety.attack_mode_enabled) {
        return -1; /* Not allowed without attack mode */
    }

    boost_enable(1);
    boost_set_voltage(target_mv);
    vbus_enable_path(port, 1);
    return 0;
}

/* ---- Boost converter init ---- */
void boost_init(void) {
    /* PB8 = EN (GPIO output), PB9 = PWM (TIM4 CH4) */
    /* TIM4 is on APB1. Configure for 200 kHz PWM. */
    /* For now, just disable the boost converter */
    gpio_write_pb(BOOST_EN_PIN, 0);
}

/* ---- Boost converter set voltage ---- */
/* The boost converter output voltage is controlled by the PWM duty cycle
 * on PB9 (TIM4_CH4). The feedback network maps duty cycle to output voltage:
 * Vout = Vin / (1 - D) * (Rfb2 / (Rfb1 + Rfb2))
 * For our design: Vin=5V, Rfb1=10k, Rfb2=200k → max Vout = 5 * 200/10 / (1-D)
 * At D=0.9: Vout = 5 * 20 / 0.1 = 1000V (theoretical max, limited by components)
 * We limit to 48V for safety.
 */
int boost_set_voltage(uint16_t target_mv) {
    /* Calculate duty cycle for target voltage */
    /* D = 1 - (Vin * Rfb_ratio / Vout) = 1 - (5 * 20 / Vout) = 1 - 100/Vout */
    if (target_mv < 6000) {
        boost_enable(0);
        return 0;
    }
    if (target_mv > 48000) target_mv = 48000;

    /* duty = 1 - 100 / (target_mv / 1000) = 1 - 100000 / target_mv */
    uint32_t duty = (170000000u / 200000u); /* TIM4 ARR for 200kHz at 170MHz */
    uint32_t pulse = duty - (100000u * duty / target_mv);

    /* Configure TIM4 CH4 (PB9, AF11) */
    volatile uint32_t *tim4_cr1 = (volatile uint32_t *)(0x40000800u); /* TIM4 base */
    volatile uint32_t *tim4_arr = (volatile uint32_t *)(0x40000800u + 0x2Cu);
    volatile uint32_t *tim4_ccr4 = (volatile uint32_t *)(0x40000800u + 0x40u);
    volatile uint32_t *tim4_ccmr2 = (volatile uint32_t *)(0x40000800u + 0x3Cu);
    volatile uint32_t *tim4_ccer = (volatile uint32_t *)(0x40000800u + 0x20u);

    *tim4_arr = duty;
    *tim4_ccr4 = pulse;
    /* PWM mode 1 on CH4, preload enabled */
    *tim4_ccmr2 = (6u << 12u) | (1u << 15u); /* OC4M=PWM1, OC4PE */
    *tim4_ccer = (1u << 12u); /* CC4E (enable CH4 output) */
    *tim4_cr1 = 1u; /* CEN (enable timer) */

    return 0;
}

/* ---- Boost converter enable/disable ---- */
void boost_enable(uint8_t enable) {
    gpio_write_pb(BOOST_EN_PIN, enable);
}