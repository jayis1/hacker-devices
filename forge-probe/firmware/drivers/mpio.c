/**
 * mpio.c — Multi-Protocol IO (MPIO) Mux Manager
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Manages the 16-channel universal IO pins that can be dynamically
 * reconfigured for UART, SPI, I2C, 1-Wire, PWM, or GPIO modes.
 * Each channel has a level shifter for 1.8V–5.0V target compatibility.
 */

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* MPIO channel configuration table */
const mpio_pin_t mpio_pins[MPIO_COUNT] = {
    /* Ch,   Port,  Pin  AF 5V-tol */
    {  0,  GPIOB,  7,  7,  true  },   /* MPIO0: UART1_TX / SPI3_CS / GPIO */
    {  1,  GPIOB,  6,  7,  true  },   /* MPIO1: UART1_RX / SPI3_SCK / GPIO */
    {  2,  GPIOB,  5,  7,  true  },   /* MPIO2: SPI3_MOSI / TIM3_CH2 */
    {  3,  GPIOB,  4,  7,  true  },   /* MPIO3: SPI3_MISO / TIM3_CH1 */
    {  4,  GPIOA,  15, 7,  true  },   /* MPIO4: SPI3_CS / TIM2_CH1 */
    {  5,  GPIOA,  14, 7,  true  },   /* MPIO5: SPI3_SCK / TIM2_CH2 */
    {  6,  GPIOA,  13, 7,  true  },   /* MPIO6: I2C1_SCL / GPIO */
    {  7,  GPIOA,  12, 7,  true  },   /* MPIO7: I2C1_SDA / GPIO */
    {  8,  GPIOE,  8,  7,  true  },   /* MPIO8: 1-Wire / UART5_TX / GPIO */
    {  9,  GPIOE,  9,  7,  true  },   /* MPIO9: GPIO / PWM / UART5_RX */
    { 10,  GPIOE,  10, 7,  true  },   /* MPIO10: GPIO / PWM */
    { 11,  GPIOE,  11, 7,  true  },   /* MPIO11: GPIO / PWM */
    { 12,  GPIOE,  12, 7,  true  },   /* MPIO12: GPIO */
    { 13,  GPIOE,  13, 7,  true  },   /* MPIO13: GPIO */
    { 14,  GPIOE,  14, 7,  true  },   /* MPIO14: GPIO */
    { 15,  GPIOE,  15, 7,  true  }    /* MPIO15: GPIO */
};

/* Current mode of each MPIO channel */
static uint8_t g_mpio_modes[MPIO_COUNT];

/**
 * mpio_channel_init: Initialize a single MPIO channel in GPIO input mode.
 */
void mpio_channel_init(uint8_t channel)
{
    if (channel >= MPIO_COUNT) return;

    const mpio_pin_t *pin = &mpio_pins[channel];

    /* Configure port clock (already done in mpio_init) */
    mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_INPUT);
    mpio_set_pull(pin->gpio_port, pin->gpio_pin, 0);  /* No pull */

    g_mpio_modes[channel] = MPIO_MODE_GPIO_IN;
}

/**
 * mpio_channel_set_mode: Reconfigure an MPIO channel for a new protocol.
 */
bool mpio_channel_set_mode(uint8_t channel, uint8_t mode)
{
    if (channel >= MPIO_COUNT) return false;

    const mpio_pin_t *pin = &mpio_pins[channel];

    switch (mode)
    {
        case MPIO_MODE_GPIO_IN:
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_INPUT);
            break;

        case MPIO_MODE_GPIO_OUT:
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_OUTPUT_PP);
            break;

        case MPIO_MODE_UART:
            mpio_set_af(pin->gpio_port, pin->gpio_pin, pin->af_number);
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_AF_PP);
            break;

        case MPIO_MODE_SPI:
            mpio_set_af(pin->gpio_port, pin->gpio_pin, pin->af_number);
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_AF_PP);
            break;

        case MPIO_MODE_I2C:
            mpio_set_af(pin->gpio_port, pin->gpio_pin, 4);  /* I2C AF */
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_AF_OD);
            break;

        case MPIO_MODE_ONEWIRE:
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_OUTPUT_OD);
            break;

        case MPIO_MODE_PWM_IN:
            mpio_set_af(pin->gpio_port, pin->gpio_pin, 2);  /* TIM AF */
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_AF_PP);
            break;

        case MPIO_MODE_PWM_OUT:
            mpio_set_af(pin->gpio_port, pin->gpio_pin, 2);
            mpio_set_mode(pin->gpio_port, pin->gpio_pin, GPIO_MODE_AF_PP);
            break;

        default:
            return false;
    }

    g_mpio_modes[channel] = mode;
    return true;
}

/**
 * mpio_channel_write: Write a GPIO value on an MPIO channel.
 */
void mpio_channel_write(uint8_t channel, uint8_t val)
{
    if (channel >= MPIO_COUNT) return;
    const mpio_pin_t *pin = &mpio_pins[channel];
    mpio_write(pin->gpio_port, pin->gpio_pin, val);
}

/**
 * mpio_channel_read: Read a GPIO value from an MPIO channel.
 */
uint8_t mpio_channel_read(uint8_t channel)
{
    if (channel >= MPIO_COUNT) return 0;
    const mpio_pin_t *pin = &mpio_pins[channel];
    return mpio_read(pin->gpio_port, pin->gpio_pin);
}

/**
 * mpio_configure_protocol: Set up MPIO channels for a specific protocol.
 */
void mpio_configure_protocol(uint8_t protocol)
{
    switch (protocol)
    {
        case PROTOCOL_JTAG:
            /* JTAG uses dedicated pins (not MPIO) — configure via mpio_configure_dbg_pins */
            break;

        case PROTOCOL_SWD:
            mpio_configure_dbg_pins(PROTOCOL_SWD);
            break;

        case PROTOCOL_CJTAG:
            mpio_configure_dbg_pins(PROTOCOL_CJTAG);
            break;

        case PROTOCOL_UART_AUX:
            mpio_channel_set_mode(MPIO_UART_TX, MPIO_MODE_UART);
            mpio_channel_set_mode(MPIO_UART_RX, MPIO_MODE_UART);
            break;

        case PROTOCOL_SPI_AUX:
            mpio_channel_set_mode(MPIO_SPI_SCK, MPIO_MODE_SPI);
            mpio_channel_set_mode(MPIO_SPI_MOSI, MPIO_MODE_SPI);
            mpio_channel_set_mode(MPIO_SPI_MISO, MPIO_MODE_SPI);
            mpio_channel_set_mode(MPIO_SPI_CS, MPIO_MODE_SPI);
            break;

        case PROTOCOL_I2C_AUX:
            mpio_channel_set_mode(MPIO_I2C_SCL, MPIO_MODE_I2C);
            mpio_channel_set_mode(MPIO_I2C_SDA, MPIO_MODE_I2C);
            break;
    }
}