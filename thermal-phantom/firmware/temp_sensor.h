/*
 * temp_sensor.h - Temperature sensor abstraction header
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

void temp_sensor_init(void);

/* MLX90614 IR sensor - non-contact target surface temperature */
bool mlx_read_temp(float *temp_c);
float mlx_get_temp(void);
bool mlx_is_present(void);

/* DS18B20 - contact temperature on Peltier cold plate */
bool ds18b20_read_temp(float *temp_c);
float ds18b20_get_temp(void);
bool ds18b20_is_present(void);

/* Combined read for convenience */
typedef struct {
    float ir_temp;      /* MLX90614 target surface */
    float plate_temp;   /* DS18B20 cold plate */
    float internal;     /* MCU internal temp */
} temp_readings_t;

void temp_read_all(temp_readings_t *readings);

#endif /* TEMP_SENSOR_H */