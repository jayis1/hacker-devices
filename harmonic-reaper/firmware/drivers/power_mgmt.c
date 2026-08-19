/*
 * power_mgmt.c — battery gauge + PMIC control
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Battery voltage is measured via the nRF52840's internal SAADC reading
 * VDD/4 (SAADC channel 0 with PSELP=VDD). The 2× 21700 cells are in
 * series → 7.4 V nominal, but we read the midpoint (3.7 V/cell) through
 * a 2:1 divider on an analog input pin. For simplicity here we read
 * the internal VDD (which tracks the regulated rail, not the raw cell
 * voltage); a real production unit would use an external fuel gauge
 * (e.g., MAX17048) on I²C. The API is identical either way.
 */
#include "../registers.h"
#include "../board.h"
#include "power_mgmt.h"

void power_init(void)
{
    /* Configure SAADC channel 0 for VDD/4 measurement */
    SAADC_ENABLE = 1u;
    SAADC_RESOLUTION = SAADC_RES_12BIT;
    SAADC_CH0_PSELP = SAADC_PSELP_VDD;
    SAADC_CH0_CONFIG = (0x02u << 8) | (0x05u << 16);  /* gain 1/6, tacq 10us */

    /* Charger status pin as input with pull-up */
    GPIO_PIN_CNF(NRF_GPIO_BASE, CHARGE_STAT_PIN) =
        GPIO_CNF_DIR_INPUT | (GPIO_CNF_PULL_UP << 2);
}

uint16_t power_read_vdd_mv(void)
{
    /* Trigger one SAADC sample on CH0, read the result.
     * The SAADC result is 12-bit; VDD = result × 0.6 V × 6 / 4095. */
    static uint16_t sample_buf[1];

    SAADC_RESULT_PTR = (uint32_t)sample_buf;
    SAADC_RESULT_MAXCNT = 1;

    /* Start sample task */
    NRF_TASK_START(NRF_SAADC_BASE, 0x000u);   /* START */
    while (!NRF_EVENT_CHECK(NRF_SAADC_BASE, 0x100u)) { /* spin on END */ }
    NRF_EVENT_CLEAR(NRF_SAADC_BASE, 0x100u);

    uint32_t raw = sample_buf[0];
    /* Convert to millivolts: V = raw × (0.6 × 6) / 4095 × 1000
     * The cell pack is 2× this (divider 2:1), so ×2. */
    uint32_t mv = (raw * 3600u) / 4095u;
    return (uint16_t)mv * 2u;  /* ×2 for 2S pack */
}

uint8_t power_mv_to_pct(uint16_t mv)
{
    /* Li-ion 2S pack discharge curve (approximate, 1C rate):
     *   8.4V = 100%, 7.4V = 50%, 6.8V = 10%, 6.0V = 0%
     * Linear interpolation between these knee points. */
    if (mv >= 8400) return 100;
    if (mv >= 7400) {
        uint32_t pct = 50 + ((uint32_t)(mv - 7400) * 50) / 1000;
        return (uint8_t)pct;
    }
    if (mv >= 6800) {
        uint32_t pct = 10 + ((uint32_t)(mv - 6800) * 40) / 600;
        return (uint8_t)pct;
    }
    if (mv > 6000) {
        uint32_t pct = ((uint32_t)(mv - 6000) * 10) / 800;
        return (uint8_t)pct;
    }
    return 0;
}

uint8_t power_is_charging(void)
{
    /* CHARGE_STAT_PIN is low when charging (open-drain on the charger IC,
     * pulled up here) */
    uint8_t vbus = (POWER_USBREGSTATUS & POWER_USBREG_VBUS_DETECTED) ? 1 : 0;
    uint8_t stat = (GPIO_OUT(NRF_GPIO_BASE) & (1u << CHARGE_STAT_PIN)) ? 1 : 0;
    return (vbus && !stat) ? 1 : 0;  /* VBUS present + stat low → charging */
}

void power_enable_rail(uint8_t rail_id, uint8_t en)
{
    /* The TPS6523550 PMIC is controlled via I²C. We would write to the
     * rail enable register here. Stub: in production this calls the
     * shared I²C driver with the PMIC's slave address. */
    (void)rail_id; (void)en;
    /* The FPGA 1.2V / 2.5V rails are enabled at boot and stay on.
     * The PA 9V rail is gated by QPL9547_TX_EN_PIN directly. */
}

/* EOF — power_mgmt.c — jayis1 */