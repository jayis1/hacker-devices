/**
 * @file    battery.h
 * @brief   Battery monitoring — header
 * @author  jayis1
 */

#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize battery ADC monitoring
 */
void battery_init(void);

/**
 * @brief Read battery voltage in millivolts
 * @return Voltage in mV (~3300-4200)
 */
uint32_t battery_read_mv(void);

/**
 * @brief Read battery level as percentage (0-100)
 * @return Percentage
 */
uint32_t battery_read_percent(void);

/**
 * @brief Check if battery is in low warning state
 * @return true if below LOW threshold
 */
bool battery_is_low(void);

/**
 * @brief Check if battery is critically low
 * @return true if below CRITICAL threshold
 */
bool battery_is_critical(void);

/**
 * @brief FreeRTOS task for battery monitoring
 */
void battery_task(void *pvParameters);

#endif /* BATTERY_H */
