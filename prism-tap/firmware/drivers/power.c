/*
 * drivers/power.c — Power Management for Prism-Tap
 *
 * Battery monitoring via ADC, charger status, and low-power mode control.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "power.h"
#include "board.h"
#include "registers.h"

/* ---- ADC registers (simplified — ADC1 on STM32H7) ---- */
#define ADC1_BASE  0x58026000U  /* Note: overlaps with CRC; real base is 0x58026000 for ADC1 common, 0x58024000 for ADC1 */
#define ADC1_CR    (*(volatile uint32_t *)(0x58024000U + 0x08))
#define ADC1_ISR   (*(volatile uint32_t *)(0x58024000U + 0x00))
#define ADC1_CFGR   (*(volatile uint32_t *)(0x58024000U + 0x0C))
#define ADC1_SMPR1  (*(volatile uint32_t *)(0x58024000U + 0x14))
#define ADC1_SQR1   (*(volatile uint32_t *)(0x58024000U + 0x30))
#define ADC1_DR     (*(volatile uint32_t *)(0x58024000U + 0x40))

#define ADC_CR_ADEN    (1U << 0)
#define ADC_CR_ADCAL   (1U << 1)
#define ADC_ISR_ADRDY  (1U << 0)
#define ADC_ISR_EOC    (1U << 2)

/* Battery parameters */
#define BATT_MAX_MV   4200   /* full charge  */
#define BATT_MIN_MV   3300   /* empty         */
#define BATT_DIVIDER  2      /* voltage divider: ADC pin = Vbatt / 2 */
#define VREF_MV       3300   /* ADC reference voltage */

static uint8_t low_power = 0;
static uint32_t last_adc_read = 0;

int power_init(void)
{
    /* Enable ADC1 clock */
    RCC_AHB2ENR |= (1U << 24);  /* ADC12EN (simplified bit position) */

    /* Configure ADC1 for single conversion, 12-bit, channel 5 */
    ADC1_CR = 0;

    /* Enable ADC */
    uint32_t timeout = 10000;
    ADC1_CR |= ADC_CR_ADEN;
    while (!(ADC1_ISR & ADC_ISR_ADRDY) && timeout--)
        ;

    /* Configure sample time for channel 5 (640.5 cycles) */
    ADC1_SMPR1 |= (0x7U << (5 * 3));  /* channel 5 = 640.5 cycles */

    return 0;
}

static uint16_t power_read_adc(void)
{
    /* Configure sequence: 1 conversion, channel 5 */
    ADC1_SQR1 = (BATT_ADC_CHANNEL << 6) | (0 << 0);  /* 1 conversion, ch5 */

    /* Start conversion */
    ADC1_CR |= (1U << 2);  /* ADSTART */

    /* Wait for end of conversion */
    uint32_t timeout = 10000;
    while (!(ADC1_ISR & ADC_ISR_EOC) && timeout--)
        ;

    return (uint16_t)(ADC1_DR & 0xFFF);
}

void power_poll(void)
{
    /* Read ADC every 1 second */
    uint32_t now = g_status.uptime_ms;
    if ((now - last_adc_read) < 1000 && last_adc_read != 0)
        return;

    last_adc_read = now;

    uint16_t adc_val = power_read_adc();

    /* Convert ADC value to millivolts:
     * Vpin = (adc_val / 4095) * VREF
     * Vbatt = Vpin * BATT_DIVIDER
     */
    uint32_t pin_mv = ((uint32_t)adc_val * VREF_MV) / 4095;
    uint16_t batt_mv = (uint16_t)(pin_mv * BATT_DIVIDER);

    g_status.battery_mv = batt_mv;

    /* Calculate percentage (linear approximation) */
    if (batt_mv >= BATT_MAX_MV)
        g_status.battery_pct = 100;
    else if (batt_mv <= BATT_MIN_MV)
        g_status.battery_pct = 0;
    else
        g_status.battery_pct = (uint8_t)(((batt_mv - BATT_MIN_MV) * 100) /
                                          (BATT_MAX_MV - BATT_MIN_MV));

    /* Low battery protection — enter standby if critical */
    if (batt_mv < 3400 && !low_power) {
        power_set_low_power_mode(1);
    }
}

uint8_t power_get_battery_pct(void)
{
    return g_status.battery_pct;
}

uint16_t power_get_battery_mv(void)
{
    return g_status.battery_mv;
}

uint8_t power_is_charging(void)
{
    /* Would read charger status pin from bq24074
       For now, check if USB is connected */
    return g_status.usb_connected ? 1 : 0;
}

uint8_t power_is_usb_powered(void)
{
    return g_status.usb_connected;
}

void power_set_low_power_mode(uint8_t enable)
{
    low_power = enable;
    if (enable) {
        /* Reduce system clock to save power */
        /* Disable FPGA capture if active */
        if (g_status.mode == MODE_FULL_MITM)
            g_status.mode = MODE_CAPTURE_LOW_POWER;

        /* Turn off status LED */
        GPIO_SET(LED_STATUS_PORT, LED_STATUS_PIN);
    } else {
        /* Restore normal operation */
        GPIO_RESET(LED_STATUS_PORT, LED_STATUS_PIN);
    }
}

void power_enter_standby(void)
{
    /* Power down all peripherals */
    bridge_power_down(BRIDGE_CSI_RX);
    bridge_power_down(BRIDGE_CSI_TX);
    bridge_power_down(BRIDGE_DSI_RX);
    bridge_power_down(BRIDGE_DSI_TX);

    /* Enable wakeup on USB-C connect or BLE event */
    /* Configure EXTI for USB-C VBUS detect pin */

    /* Enter standby mode */
    PWR_CR1 |= (3U << 0);  /* PDDS: standby mode */
    /* Set SLEEPDEEP and clear SLEEPONEXIT */
    __asm volatile ("wfi");
}

void power_wakeup(void)
{
    /* Called after waking from standby — reinitialize hardware */
    /* The main init sequence will run again */
}

/* ---- End of power.c ----
 * Author: jayis1
 */