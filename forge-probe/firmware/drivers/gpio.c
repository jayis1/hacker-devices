/**
 * gpio.c — GPIO Abstraction Layer for Forge-Probe
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Provides low-level GPIO control abstraction over the STM32H743 HAL.
 * Implements the mpio_* functions used by the firmware main loop.
 */

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* GPIO mode constants */
#define GPIO_MODE_INPUT         0x00
#define GPIO_MODE_OUTPUT_PP     0x01
#define GPIO_MODE_OUTPUT_OD     0x02
#define GPIO_MODE_AF_PP         0x03
#define GPIO_MODE_AF_OD         0x04
#define GPIO_MODE_ANALOG        0x05

/* ─── Pin Configuration ────────────────────────────────────────────────────── */

/**
 * mpio_init: Initialize all MPIO channels in safe default state.
 * Configures all debug pins as high-impedance inputs to avoid
 * accidental drive onto the target.
 */
void mpio_init(void)
{
    /* Enable all GPIO port clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOHEN |
                    RCC_AHB4ENR_GPIOIEN;

    /* Set all debug interface pins to input with pull-up */
    mpio_set_mode(MPIO_JTAG_TCK, GPIO_MODE_INPUT);
    mpio_set_pull(MPIO_JTAG_TCK, 1);  /* Pull-up */

    mpio_set_mode(MPIO_JTAG_TMS, GPIO_MODE_INPUT);
    mpio_set_pull(MPIO_JTAG_TMS, 1);

    mpio_set_mode(MPIO_JTAG_TDI, GPIO_MODE_INPUT);
    mpio_set_pull(MPIO_JTAG_TDI, 1);

    mpio_set_mode(MPIO_JTAG_TDO, GPIO_MODE_INPUT);
    mpio_set_pull(MPIO_JTAG_TDO, 1);

    /* Status LEDs off (active low) */
    mpio_set_mode(STATUS_LED_R, GPIO_MODE_OUTPUT_PP);
    mpio_set_mode(STATUS_LED_G, GPIO_MODE_OUTPUT_PP);
    mpio_set_mode(STATUS_LED_B, GPIO_MODE_OUTPUT_PP);
    mpio_write(STATUS_LED_R, 1);
    mpio_write(STATUS_LED_G, 1);
    mpio_write(STATUS_LED_B, 1);

    /* Enable target power rail default (3.3V, off initially) */
    mpio_set_mode(EN_3V3_PORT, EN_3V3_PIN, GPIO_MODE_OUTPUT_PP);
    mpio_write(EN_3V3_PORT, EN_3V3_PIN, 0);
}

/**
 * mpio_set_mode: Set GPIO pin mode for a port/pin pair.
 */
void mpio_set_mode(GPIO_TypeDef *port, uint16_t pin, uint8_t mode)
{
    uint32_t pos = (pin < 8) ? (pin * 4) : ((pin - 8) * 4);
    uint32_t *moder = (pin < 8) ? &port->MODER : &port->MODER;

    port->MODER &= ~(0x3UL << pos);
    port->MODER |= ((uint32_t)mode & 0x3) << pos;

    /* For alternate functions, set AF register */
    if (mode == GPIO_MODE_AF_PP || mode == GPIO_MODE_AF_OD)
    {
        uint32_t af_pos = (pin < 8) ? (pin * 4) : ((pin - 8) * 4);
        uint32_t *afr = (pin < 8) ? &port->AFR[0] : &port->AFR[1];
        *afr &= ~(0xFUL << af_pos);
        *afr |= (0x0UL << af_pos);  /* AF0 default */
    }
}

/**
 * mpio_set_pull: Set pull-up/pull-down configuration.
 * pull: 0=none, 1=pull-up, 2=pull-down
 */
void mpio_set_pull(GPIO_TypeDef *port, uint16_t pin, uint8_t pull)
{
    uint32_t pos = pin * 2;
    port->PUPDR &= ~(0x3UL << pos);
    port->PUPDR |= ((uint32_t)pull) << pos;
}

/**
 * mpio_write: Set GPIO output value.
 */
void mpio_write(GPIO_TypeDef *port, uint16_t pin, uint8_t val)
{
    if (val)
        port->BSRR = (1UL << pin);
    else
        port->BSRR = (1UL << (pin + 16));
}

/**
 * mpio_read: Read GPIO input value.
 * Returns 0 or 1.
 */
uint8_t mpio_read(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->IDR >> pin) & 1;
}

/**
 * mpio_toggle: Toggle GPIO output.
 */
void mpio_toggle(GPIO_TypeDef *port, uint16_t pin)
{
    port->ODR ^= (1UL << pin);
}

/**
 * mpio_configure_dbg_pins: Set up debug interface pin muxing.
 * Called when switching between JTAG/SWD/cJTAG protocols.
 */
void mpio_configure_dbg_pins(uint8_t protocol)
{
    if (protocol == PROTOCOL_SWD)
    {
        /* SWD: SWCLK (AF0 push-pull), SWDIO (AF0 open-drain) */
        mpio_set_mode(SWD_SWCLK_PORT, SWD_SWCLK_PIN, GPIO_MODE_AF_PP);
        mpio_set_pull(SWD_SWCLK_PORT, SWD_SWCLK_PIN, 0);
        mpio_set_mode(SWD_SWDIO_PORT, SWD_SWDIO_PIN, GPIO_MODE_AF_OD);
        mpio_set_pull(SWD_SWDIO_PORT, SWD_SWDIO_PIN, 1);  /* Pull-up */

        /* Tristate JTAG-only pins */
        mpio_set_mode(MPIO_JTAG_TDI, GPIO_MODE_INPUT);
        mpio_set_mode(MPIO_JTAG_TDO, GPIO_MODE_INPUT);
    }
    else if (protocol == PROTOCOL_JTAG)
    {
        /* JTAG: TCK/TMS/TDI push-pull, TDO input */
        mpio_set_mode(JTAG_TCK_PORT, JTAG_TCK_PIN, GPIO_MODE_AF_PP);
        mpio_set_mode(JTAG_TMS_PORT, JTAG_TMS_PIN, GPIO_MODE_AF_PP);
        mpio_set_mode(JTAG_TDI_PORT, JTAG_TDI_PIN, GPIO_MODE_AF_PP);
        mpio_set_mode(JTAG_TDO_PORT, JTAG_TDO_PIN, GPIO_MODE_INPUT);
        mpio_set_pull(JTAG_TDO_PORT, JTAG_TDO_PIN, 1);

        /* nTRST and nSRST as open-drain */
        mpio_set_mode(JTAG_nTRST_PORT, JTAG_nTRST_PIN, GPIO_MODE_OUTPUT_OD);
        mpio_write(JTAG_nTRST_PORT, JTAG_nTRST_PIN, 1);  /* Deassert */
        mpio_set_mode(JTAG_nSRST_PORT, JTAG_nSRST_PIN, GPIO_MODE_OUTPUT_OD);
        mpio_write(JTAG_nSRST_PORT, JTAG_nSRST_PIN, 1);  /* Deassert */
    }
    else if (protocol == PROTOCOL_CJTAG)
    {
        /* cJTAG: TCKC push-pull, TMSC open-drain */
        mpio_set_mode(CJTAG_TCKC_PORT, CJTAG_TCKC_PIN, GPIO_MODE_AF_PP);
        mpio_set_mode(CJTAG_TMSC_PORT, CJTAG_TMSC_PIN, GPIO_MODE_AF_OD);
        mpio_set_pull(CJTAG_TMSC_PORT, CJTAG_TMSC_PIN, 1);
    }
}

/**
 * mpio_set_af: Set alternate function number for a pin.
 * af_num: 0-15 (AF0-AF15)
 */
void mpio_set_af(GPIO_TypeDef *port, uint16_t pin, uint8_t af_num)
{
    uint32_t afr_idx = (pin < 8) ? 0 : 1;
    uint32_t pos = (pin < 8) ? (pin * 4) : ((pin - 8) * 4);

    port->AFR[afr_idx] &= ~(0xFUL << pos);
    port->AFR[afr_idx] |= ((uint32_t)af_num & 0xF) << pos;
}