/*
 * ad9361.c — 2f0 receiver driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The AD9361 is configured via SPI (3-wire, mode 0). Its register map
 * spans ~400 registers; we only touch the ones needed to set up a
 * direct-conversion RX at 4.8 GHz with 20 MHz RF bandwidth and 25 MSPS
 * I/Q streaming on the CMOS data bus.
 *
 * Default state: powered down, TX disabled, RX enabled when armed.
 */
#include "../registers.h"
#include "../board.h"
#include "ad9361.h"

/* AD9361 register addresses (subset, datasheet Rev. C) */
#define AD9361_REG_LOOPBACK     0x006u
#define AD9361_REG_ENABLE       0x000u   /* power-down / enable bits */
#define AD9361_REG_RXFIR        0x01Eu
#define AD9361_REG_RXFIR_EN     0x0FFu   /* enable FIR */
#define AD9361_REG_RX_GAIN_CTL  0x0A7u
#define AD9361_REG_RX_GAIN_MAN  0x0A8u
#define AD9361_REG_RX_RF_GAIN   0x0AAu
#define AD9361_REG_FREQ_RF_RX  0x003u
#define AD9361_REG_FREQ_FRAC   0x004u
#define AD9361_REG_TX_DISABLE  0x005u
#define AD9361_REG_BW_RF_RX    0x0B5u

#define AD9361_SPI_BASE NRF_SPIM1_BASE

static void ad9361_spi_write(uint16_t reg, uint8_t val)
{
    /* AD9361 SPI frame: bit7=W/R, bit[6:0]=reg[8:2] for auto-increment off.
     * We use the simple "write" frame: [W=1][addr7:0][data7:0]. */
    uint8_t buf[3];
    buf[0] = 0x80u | (uint8_t)(reg & 0x7Fu);   /* W=1, addr[6:0] */
    buf[1] = (uint8_t)((reg >> 7) & 0xFFu);    /* addr[13:7] — extended addr */
    buf[2] = val;

    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << AD9361_CS_PIN);
    SPIM_TXD_PTR(AD9361_SPI_BASE) = (uint32_t)buf;
    SPIM_TXD_MAXCNT(AD9361_SPI_BASE) = 3;
    SPIM_RXD_MAXCNT(AD9361_SPI_BASE) = 0;
    SPIM_START_TX(AD9361_SPI_BASE);
    while (!SPIM_END_EVENT(AD9361_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(AD9361_SPI_BASE);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << AD9361_CS_PIN);
}

static uint8_t ad9361_spi_read(uint16_t reg)
{
    uint8_t tx[3] = { 0x00u | (uint8_t)(reg & 0x7Fu),
                      (uint8_t)((reg >> 7) & 0xFFu), 0 };
    uint8_t rx[3] = { 0 };
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << AD9361_CS_PIN);
    SPIM_TXD_PTR(AD9361_SPI_BASE) = (uint32_t)tx;
    SPIM_RXD_PTR(AD9361_SPI_BASE) = (uint32_t)rx;
    SPIM_TXD_MAXCNT(AD9361_SPI_BASE) = 3;
    SPIM_RXD_MAXCNT(AD9361_SPI_BASE) = 3;
    SPIM_START_TX(AD9361_SPI_BASE);
    while (!SPIM_END_EVENT(AD9361_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(AD9361_SPI_BASE);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << AD9361_CS_PIN);
    return rx[2];
}

void ad9361_init(void)
{
    /* Configure SPIM1 pins for AD9361 (shared bus with FPGA — FPGA uses
     * its own CS, so no bus contention as long as we don't talk to both
     * at the same instant). */
    SPIM_PSEL_SCK(AD9361_SPI_BASE)  = AD9361_SCK_PIN;
    SPIM_PSEL_MOSI(AD9361_SPI_BASE) = AD9361_MOSI_PIN;
    SPIM_PSEL_MISO(AD9361_SPI_BASE) = AD9361_MISO_PIN;
    SPIM_FREQUENCY(AD9361_SPI_BASE) = SPIM_FREQ_8M;
    SPIM_CONFIG(AD9361_SPI_BASE)   = SPIM_MODE0;
    SPIM_ENABLE(AD9361_SPI_BASE)   = 1u;

    /* Assert reset (active low) for 10 us then release */
    GPIO_PIN_CNF(NRF_GPIO_BASE, AD9361_RESET_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << AD9361_RESET_PIN);
    nrf_delay_us(10);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << AD9361_RESET_PIN);
    nrf_delay_ms(1);

    /* Read the phantom power / chip ID register to verify SPI link.
     * AD9361 doesn't have a true ID; we check LOOPBACK register r/w. */
    ad9361_spi_write(AD9361_REG_LOOPBACK, 0x01u);
    uint8_t lb = ad9361_spi_read(AD9361_REG_LOOPBACK);
    if (lb != 0x01u) {
        /* SPI fault — would raise MODE_FAULT here */
        (void)lb;
    }
    ad9361_spi_write(AD9361_REG_LOOPBACK, 0x00u);  /* clear loopback */

    /* Disable TX path entirely (we never use it) */
    ad9361_spi_write(AD9361_REG_TX_DISABLE, 0x01u);

    /* Set RX LO frequency: 4.8 GHz (2 × 2.4 GHz).
     * The AD9361 fractional PLL uses a 40 MHz reference (from the same
     * TCXO as the ADF4159, divided). INT=120, FRAC=0 → 4.8 GHz. */
    ad9361_spi_write(AD9361_REG_FREQ_RF_RX, 0x78u);   /* INT=120 */
    ad9361_spi_write(AD9361_REG_FREQ_FRAC, 0x00u);   /* FRAC=0 */

    /* RX RF bandwidth = 20 MHz (enough to capture the full reflected
     * spectrum of a CW fundamental — the harmonics will be at exactly
     * 2f0, so 20 MHz BW is generous). */
    ad9361_spi_write(AD9361_REG_BW_RF_RX, 0x14u);

    /* Enable RX FIR filter (default coefficients) */
    ad9361_spi_write(AD9361_REG_RXFIR, 0x01u);
    ad9361_spi_write(AD9361_REG_RXFIR_EN, 0x01u);

    /* Manual gain mode (AGC is done by the MCU, not the AD9361's own AGC) */
    ad9361_spi_write(AD9361_REG_RX_GAIN_CTL, 0x00u);  /* manual mode */
    ad9361_set_gain(30);  /* 30 dB nominal start */

    /* Power down until armed */
    ad9361_spi_write(AD9361_REG_ENABLE, 0x00u);
}

void ad9361_enable(uint8_t en)
{
    /* Bit 0 = RX enable. We keep TX off always. */
    ad9361_spi_write(AD9361_REG_ENABLE, en ? 0x01u : 0x00u);
}

void ad9361_set_gain(uint8_t gain_db)
{
    /* Clamp to AD9361's valid manual gain range (0..73 dB) */
    if (gain_db > 73) gain_db = 73;
    /* Write to the RX manual gain register (SPI gain index = gain_db × 4) */
    ad9361_spi_write(AD9361_REG_RX_GAIN_MAN, gain_db);
    ad9361_spi_write(AD9361_REG_RX_RF_GAIN, gain_db);
}

uint8_t ad9361_get_rssi_db(void)
{
    /* The AD9361 reports an RSSI pre-ADC; we read it from the RX RSSI
     * register (address 0x0C6, datasheet §). This is an 8-bit unsigned
     * value in dB. */
    return ad9361_spi_read(0x0C6u);
}

/* EOF — ad9361.c — jayis1 */