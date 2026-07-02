/*
 * optics_driver.c — LC bandpass + input select
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * Drives the MCP4725-class DAC on I2C1 that sets the liquid-crystal tunable
 * bandpass center wavelength (450-950 nm maps to 0-3V3). Also toggles a
 * GPIO that switches between the 30 mm collector lens and the 5 mm fiber
 * pigtail input via a small mechanical shutter / mirror.
 */
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "optics_driver.h"

static uint16_t g_wavelength = 550;
static bool     g_fiber = false;

/* Weak I2C write stub; real HAL in BSP. */
__attribute__((weak)) int i2c_write(uint8_t port, uint8_t addr,
                                    const uint8_t *data, uint32_t len)
{ (void)port; (void)addr; (void)data; (void)len; return 0; }

__attribute__((weak)) void gpio_set(uint8_t pin, int val) { (void)pin; (void)val; }

static void dac_set_mv(uint16_t mv)
{
    /* MCP4725: 12-bit, Vdd=3V3 -> code = mv*4095/3300. */
    uint16_t code = (uint16_t)((uint32_t)mv * 4095u / 3300u);
    uint8_t pkt[3] = { 0x40, (uint8_t)(code >> 4), (uint8_t)(code & 0x0F) << 4 };
    i2c_write(LC_DAC_I2C_PORT, LC_DAC_ADDR, pkt, 3);
}

int optics_init(void)
{
    g_wavelength = 550;
    g_fiber = false;
    optics_set_wavelength(g_wavelength);
    gpio_set(45 /* fiber-select GPIO */, 0);
    return 0;
}

void optics_set_wavelength(uint16_t nm)
{
    if (nm < LC_WAVELENGTH_MIN) nm = LC_WAVELENGTH_MIN;
    if (nm > LC_WAVELENGTH_MAX) nm = LC_WAVELENGTH_MAX;
    g_wavelength = nm;
    /* Linear map nm -> mV over 450..950. */
    uint32_t mv = (uint32_t)(nm - LC_WAVELENGTH_MIN) * 3300u
                  / (LC_WAVELENGTH_MAX - LC_WAVELENGTH_MIN);
    dac_set_mv((uint16_t)mv);
}

uint16_t optics_get_wavelength(void) { return g_wavelength; }

void optics_select_fiber_input(bool on)
{
    g_fiber = on;
    gpio_set(45, on ? 1 : 0);
}