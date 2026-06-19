/**
 * drivers/gpio.c — GPIO abstraction for PhotonStrike
 * Author: jayis1
 * License: GPL-2.0
 *
 * Thin wrapper around the STM32H7 GPIO registers. Used by every other
 * driver to avoid duplicating MODER/PUPDR/AFR bit math.
 */

#include "gpio.h"

void gpio_config_pin(volatile uint32_t *port, uint8_t pin,
                     uint8_t mode, uint8_t pupd, uint8_t speed)
{
    uint32_t m = port[GPIO_MODER / 4u];
    m &= ~(3u << (pin * 2u));
    m |=  ((uint32_t)mode << (pin * 2u));
    port[GPIO_MODER / 4u] = m;

    uint32_t p = port[GPIO_PUPDR / 4u];
    p &= ~(3u << (pin * 2u));
    p |=  ((uint32_t)pupd << (pin * 2u));
    port[GPIO_PUPDR / 4u] = p;

    uint32_t s = port[GPIO_OSPEEDR / 4u];
    s &= ~(3u << (pin * 2u));
    s |=  ((uint32_t)speed << (pin * 2u));
    port[GPIO_OSPEEDR / 4u] = s;
}

void gpio_set_pin(volatile uint32_t *port, uint8_t pin)
{
    port[GPIO_BSRR / 4u] = (1u << pin);
}

void gpio_clr_pin(volatile uint32_t *port, uint8_t pin)
{
    port[GPIO_BSRR / 4u] = (1u << (pin + 16));
}

bool gpio_read_pin(volatile uint32_t *port, uint8_t pin)
{
    return (port[GPIO_IDR / 4u] >> pin) & 1u;
}

void gpio_af_pin(volatile uint32_t *port, uint8_t pin, uint8_t af)
{
    if (pin < 8) {
        uint32_t v = port[GPIO_AFRL / 4u];
        v &= ~(0xFu << (pin * 4u));
        v |=  ((uint32_t)af << (pin * 4u));
        port[GPIO_AFRL / 4u] = v;
    } else {
        uint8_t p = pin - 8;
        uint32_t v = port[GPIO_AFRH / 4u];
        v &= ~(0xFu << (p * 4u));
        v |=  ((uint32_t)af << (p * 4u));
        port[GPIO_AFRH / 4u] = v;
    }
}

void gpio_init_all(void)
{
    /* FPGA SPI1: PA5/6/7 AF5, PA4 output (CS), PB3 input (CDONE),
     * PB4 output (CRESET), PB5 input (IRQ). */
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 5, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOA_BASE, 5, 5);
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 6, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOA_BASE, 6, 5);
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 7, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOA_BASE, 7, 5);
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 4, GPIO_MODE_OUT, GPIO_PUPD_PU, GPIO_SPEED_HIGH);
    gpio_set_pin   ((volatile uint32_t *)GPIOA_BASE, 4);   /* CS idle high */

    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 3, GPIO_MODE_IN,  GPIO_PUPD_NONE, GPIO_SPEED_LOW);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 4, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_LOW);
    gpio_clr_pin   ((volatile uint32_t *)GPIOB_BASE, 4);   /* CRESET low  */
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 5, GPIO_MODE_IN,  GPIO_PUPD_PD,  GPIO_SPEED_LOW);

    /* Laser: PA8 AF1 (TIM1_CH1), PB0 out (enable), PB1 out (shutter, M4 owns
     * it but we set it low here for determinism), PA9 AF1 (TIM1_CH2 TEC). */
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 8, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOA_BASE, 8, 1);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 0, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_LOW);
    gpio_clr_pin   ((volatile uint32_t *)GPIOB_BASE, 0);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 1, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_LOW);
    gpio_clr_pin   ((volatile uint32_t *)GPIOB_BASE, 1);

    /* Trigger SMAs: PC6/PC7 input, PC8 input (pattern), PC9 input (arm btn),
     * PC10 input (target clk). */
    for (uint8_t p = 6; p <= 10; p++)
        gpio_config_pin((volatile uint32_t *)GPIOC_BASE, p, GPIO_MODE_IN, GPIO_PUPD_PD, GPIO_SPEED_HIGH);

    /* Safety: PB14/15 input pull-up (key, interlock), PH0 input pull-up (estop). */
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 14, GPIO_MODE_IN, GPIO_PUPD_PU, GPIO_SPEED_LOW);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 15, GPIO_MODE_IN, GPIO_PUPD_PU, GPIO_SPEED_LOW);
    gpio_config_pin((volatile uint32_t *)GPIOH_BASE, 0,  GPIO_MODE_IN, GPIO_PUPD_PU, GPIO_SPEED_LOW);

    /* LEDs: PE0/1/2 output. */
    for (uint8_t p = 0; p <= 2; p++)
        gpio_config_pin((volatile uint32_t *)GPIOE_BASE, p, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_LOW);

    /* BLE UART3: PB10 AF7 (TX), PB11 AF7 (RX), PB12 out (reset), PB13 out (boot). */
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 10, GPIO_MODE_AF, GPIO_PUPD_PU, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOB_BASE, 10, 7);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 11, GPIO_MODE_AF, GPIO_PUPD_PU, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOB_BASE, 11, 7);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 12, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_LOW);
    gpio_set_pin   ((volatile uint32_t *)GPIOB_BASE, 12);  /* reset idle high */
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 13, GPIO_MODE_OUT, GPIO_PUPD_NONE, GPIO_SPEED_LOW);
    gpio_clr_pin   ((volatile uint32_t *)GPIOB_BASE, 13);  /* boot pin low = normal */

    /* I2C1 for MEMS + fuel gauge: PB8 AF4, PB9 AF4. */
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 8, GPIO_MODE_AF, GPIO_PUPD_PU, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOB_BASE, 8, 4);
    gpio_config_pin((volatile uint32_t *)GPIOB_BASE, 9, GPIO_MODE_AF, GPIO_PUPD_PU, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOB_BASE, 9, 4);

    /* USB: PA11/PA12 AF10. */
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 11, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOA_BASE, 11, 10);
    gpio_config_pin((volatile uint32_t *)GPIOA_BASE, 12, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOA_BASE, 12, 10);

    /* ADC pins: PC0 (laser temp), PC1 (power trigger) analog. */
    gpio_config_pin((volatile uint32_t *)GPIOC_BASE, 0, GPIO_MODE_AN, GPIO_PUPD_NONE, GPIO_SPEED_LOW);
    gpio_config_pin((volatile uint32_t *)GPIOC_BASE, 1, GPIO_MODE_AN, GPIO_PUPD_NONE, GPIO_SPEED_LOW);

    /* SDMMC1: PC8..PC12 AF12, PD2 AF12. */
    for (uint8_t p = 8; p <= 12; p++) {
        gpio_config_pin((volatile uint32_t *)GPIOC_BASE, p, GPIO_MODE_AF, GPIO_PUPD_PU, GPIO_SPEED_HIGH);
        gpio_af_pin   ((volatile uint32_t *)GPIOC_BASE, p, 12);
    }
    gpio_config_pin((volatile uint32_t *)GPIOD_BASE, 2, GPIO_MODE_AF, GPIO_PUPD_PU, GPIO_SPEED_HIGH);
    gpio_af_pin   ((volatile uint32_t *)GPIOD_BASE, 2, 12);
    gpio_config_pin((volatile uint32_t *)GPIOC_BASE, 13, GPIO_MODE_IN, GPIO_PUPD_PU, GPIO_SPEED_LOW);  /* CD */
}