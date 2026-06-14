/*
 * ws2812b.h — WS2812B RGB LED driver for STM32F407
 */

#ifndef WS2812B_DRIVER_H
#define WS2812B_DRIVER_H

#include <stdint.h>
#include <stddef.h>

#define LED_COUNT  4

typedef struct {
    uint8_t g;  /* WS2812B sends G first */
    uint8_t r;
    uint8_t b;
} ws2812b_color_t;

void ws2812b_init(void);
void ws2812b_send(const ws2812b_color_t *colors, uint16_t count);
void ws2812b_clear(uint16_t count);

#endif /* WS2812B_DRIVER_H */