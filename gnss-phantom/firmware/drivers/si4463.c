/*
 * drivers/si4463.c — Si4463 RF transceiver driver implementation
 *
 * Provides direct-synthesis RF transmission for GNSS L1 spoofing via the
 * Si4463 ISM-band transceiver.  The MCU streams BPSK-modulated C/A code +
 * navigation data over SPI at high speed; the Si4463 upconverts to L1.
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#include "si4463.h"
#include "../registers.h"
#include <string.h>

/* ----- SPI2 low-level primitives -----
 * SPI2 is shared between both Si4463 transceivers (separate CS).
 * Clock: APB1/16 = 7.5 MHz (sufficient for FIFO refill).
 */

static void spi2_wait_txe(void) {
    while (!(SPI2->SR & SPI_SR_TXP)) { }
}

static void spi2_wait_eot(void) {
    while (!(SPI2->SR & SPI_SR_EOT)) { }
    SPI2->IFCR = SPI_SR_EOT | SPI_SR_OVR | SPI_SR_MODF;
}

static void spi2_wait_rxp(void) {
    while (!(SPI2->SR & SPI_SR_RXP)) { }
}

static uint8_t spi2_xfer(uint8_t tx) {
    spi2_wait_txe();
    SPI2->TXDR = tx;
    spi2_wait_eot();
    spi2_wait_rxp();
    return (uint8_t)SPI2->RXDR;
}

static uint8_t spi2_xfer_buf(si4463_inst_t inst, const uint8_t *tx,
                             uint8_t *rx, uint16_t len) {
    uint8_t last;
    si4463_select(inst);
    /* Set TSIZE for end-of-transfer detection */
    SPI2->CR2 = (SPI2->CR2 & ~0xFFFFU) | len;
    SPI2->CR1 |= SPI_CR1_SPE;
    last = 0;
    for (uint16_t i = 0; i < len; i++) {
        spi2_wait_txe();
        SPI2->TXDR = tx ? tx[i] : 0xFF;
        spi2_wait_eot();
        spi2_wait_rxp();
        uint8_t r = (uint8_t)SPI2->RXDR;
        if (rx) rx[i] = r;
        if (i == len - 1) last = r;
    }
    SPI2->CR1 &= ~SPI_CR1_SPE;
    si4463_deselect(inst);
    return last;
}

/* Poll CTS (clear-to-send) after sending a command */
static int si4463_wait_cts(si4463_inst_t inst) {
    uint8_t cmd = SI4463_CMD_READ_CMD_BUFF;
    uint8_t resp[2];
    for (int timeout = 0; timeout < 1000; timeout++) {
        resp[0] = spi2_xfer_buf(inst, &cmd, resp, 2);
        if (resp[1] == SI4463_CTS_READY)
            return 0;
    }
    return -1;
}

/* Send a command with no parameters and wait for CTS */
static int si4463_cmd(si4463_inst_t inst, const uint8_t *cmd, uint16_t len) {
    si4463_select(inst);
    SPI2->CR2 = (SPI2->CR2 & ~0xFFFFU) | len;
    SPI2->CR1 |= SPI_CR1_SPE;
    for (uint16_t i = 0; i < len; i++) {
        spi2_wait_txe();
        SPI2->TXDR = cmd[i];
        spi2_wait_eot();
        spi2_wait_rxp();
        (void)SPI2->RXDR;
    }
    SPI2->CR1 &= ~SPI_CR1_SPE;
    si4463_deselect(inst);
    return si4463_wait_cts(inst);
}

/* Send a command and read response */
static int si4463_cmd_read(si4463_inst_t inst, const uint8_t *cmd,
                           uint16_t cmd_len, uint8_t *resp, uint16_t resp_len) {
    int rc = si4463_cmd(inst, cmd, cmd_len);
    if (rc) return rc;
    uint8_t read_cmd = SI4463_CMD_READ_CMD_BUFF;
    si4463_select(inst);
    SPI2->CR2 = (SPI2->CR2 & ~0xFFFFU) | (resp_len + 1);
    SPI2->CR1 |= SPI_CR1_SPE;
    spi2_wait_txe();
    SPI2->TXDR = read_cmd;
    spi2_wait_eot();
    spi2_wait_rxp();
    (void)SPI2->RXDR; /* discard CTS byte (should be 0xFF) */
    for (uint16_t i = 0; i < resp_len; i++) {
        spi2_wait_txe();
        SPI2->TXDR = 0xFF;
        spi2_wait_eot();
        spi2_wait_rxp();
        resp[i] = (uint8_t)SPI2->RXDR;
    }
    SPI2->CR1 &= ~SPI_CR1_SPE;
    si4463_deselect(inst);
    return 0;
}

/* Set a property (group, prop, value(s)) */
static int si4463_set_property(si4463_inst_t inst, uint8_t group,
                                uint8_t prop, const uint8_t *val,
                                uint8_t len) {
    uint8_t cmd[16];
    cmd[0] = SI4463_CMD_SET_PROPERTY;
    cmd[1] = group;
    cmd[2] = len;
    cmd[3] = prop;
    for (uint8_t i = 0; i < len && i < 12; i++)
        cmd[4 + i] = val[i];
    return si4463_cmd(inst, cmd, 4 + len);
}

static int si4463_set_property_byte(si4463_inst_t inst, uint8_t group,
                                    uint8_t prop, uint8_t val) {
    return si4463_set_property(inst, group, prop, &val, 1);
}

/* ----- Public API ----- */

void si4463_reset(si4463_t *dev) {
    GPIO_TypeDef *port = (dev->inst == SI4463_GPS) ? SI4463_PORT : SI4463B_PORT;
    int sdn_pin = (dev->inst == SI4463_GPS) ? SI4463_SDN_PIN : SI4463B_SDN_PIN;
    int sel_pin = (dev->inst == SI4463_GPS) ? SI4463_SEL_PIN : SI4463B_SEL_PIN;

    /* Assert shutdown (SDN high) */
    gpio_write(port, sdn_pin, 1);
    gpio_write(port, sel_pin, 1);
    /* Delay ~10 us */
    for (volatile int i = 0; i < 1000; i++) { }
    /* Release shutdown (SDN low) — chip boots from internal ROM */
    gpio_write(port, sdn_pin, 0);
    /* Wait for chip to be ready (~3 ms) */
    for (volatile int i = 0; i < 300000; i++) { }
    dev->state = SI4463_STATE_READY;
}

void si4463_shutdown(si4463_t *dev) {
    GPIO_TypeDef *port = (dev->inst == SI4463_GPS) ? SI4463_PORT : SI4463B_PORT;
    int sdn_pin = (dev->inst == SI4463_GPS) ? SI4463_SDN_PIN : SI4463B_SDN_PIN;
    gpio_write(port, sdn_pin, 1);
    dev->state = SI4463_STATE_SLEEP;
}

int si4463_init(si4463_t *dev, si4463_inst_t inst) {
    dev->inst = inst;
    dev->freq_hz = 1575420000;  /* default GPS L1 */
    dev->tx_power_dbm = 10;
    dev->state = SI4463_STATE_SLEEP;
    dev->initialized = 0;

    /* GPIO config for this instance */
    GPIO_TypeDef *port = (inst == SI4463_GPS) ? SI4463_PORT : SI4463B_PORT;
    int sdn_pin = (inst == SI4463_GPS) ? SI4463_SDN_PIN : SI4463B_SDN_PIN;
    int sel_pin = (inst == SI4463_GPS) ? SI4463_SEL_PIN : SI4463B_SEL_PIN;
    int irq_pin = (inst == SI4463_GPS) ? SI4463_IRQ_PIN : SI4463B_IRQ_PIN;

    gpio_set_mode(port, sdn_pin, GPIO_MODE_OUTPUT);
    gpio_set_otype(port, sdn_pin, GPIO_OTYPE_PP);
    gpio_set_speed(port, sdn_pin, GPIO_OSPEED_LOW);
    gpio_write(port, sdn_pin, 1);

    gpio_set_mode(port, sel_pin, GPIO_MODE_OUTPUT);
    gpio_set_otype(port, sel_pin, GPIO_OTYPE_PP);
    gpio_set_speed(port, sel_pin, GPIO_OSPEED_HIGH);
    gpio_write(port, sel_pin, 1);  /* idle high */

    gpio_set_mode(port, irq_pin, GPIO_MODE_INPUT);
    gpio_set_pupd(port, irq_pin, GPIO_PUPD_PU);

    /* Hard reset the chip */
    si4463_reset(dev);

    /* Power-up command: use 26 MHz TCXO, API mode */
    uint8_t pwr_cmd[8] = {
        SI4463_CMD_POWER_UP,
        0x01, 0x00,  /* boot patch 0, no patch */
        0x01, 0xC9, 0x83, 0x02, /* 26 MHz TCXO in 0x01C98302 */
        0x00,
    };
    /* Calculate correct xtal value: 26 MHz = 0x00C98302 in SiLabs format */
    pwr_cmd[3] = 0x01; /* XDIV2=0, XDIV4=0, etc. — simplified */
    pwr_cmd[4] = 0x52;
    pwr_cmd[5] = 0x00;
    pwr_cmd[6] = 0x26;
    if (si4463_cmd(inst, pwr_cmd, 7)) {
        return -1;
    }

    /* Verify part info */
    uint8_t part_cmd[1] = { SI4463_CMD_PART_INFO };
    uint8_t part_resp[8];
    if (si4463_cmd_read(inst, part_cmd, 1, part_resp, 8)) {
        return -2;
    }
    /* Expected chip rev: 0xB1 for Si4463 */
    if (part_resp[0] == 0x00 && part_resp[1] == 0x00)
        return -3;  /* no chip present */

    /* Configure GPIO pins on Si4463 */
    uint8_t gpio_cmd[7] = {
        SI4463_CMD_GPIO_PIN_CFG,
        0x00, 0x00, 0x20, 0x00, 0x00, 0x00
    };
    if (si4463_cmd(inst, gpio_cmd, 7)) {
        return -4;
    }

    /* Set modem to BPSK-like / direct TX mode */
    si4463_set_property_byte(inst, SI4463_PROP_GROUP_MODEM,
                             SI4463_PROP_MODEM_MOD_TYPE, 0x07); /* direct mode */
    /* TX data rate = 1.023 MHz (C/A code rate) */
    uint8_t rate_bytes[4] = { 0x00, 0x0F, 0xA5, 0x00 };
    si4463_set_property(inst, SI4463_PROP_GROUP_MODEM,
                        SI4463_PROP_MODEM_TX_RATE, rate_bytes, 4);

    /* Configure synth band for L1 (high band) */
    si4463_set_property_byte(inst, SI4463_PROP_GROUP_MODEM,
                             SI4463_PROP_MODEM_CLKGEN_BAND, 0x0B);

    /* Configure PA: max power, no ramp */
    uint8_t pa_cfg[6] = { 0x18, 0x00, 0x00, 0x00, 0x7F, 0x7F };
    si4463_set_property(inst, SI4463_PROP_GROUP_TX, 0x00, pa_cfg, 6);

    /* Set frequency (L1 GPS default) */
    si4463_set_frequency(dev, dev->freq_hz);

    /* Set default TX power */
    si4463_set_tx_power(dev, dev->tx_power_dbm);

    dev->initialized = 1;
    return 0;
}

/* Frequency synthesis: write 4-byte RF frequency property.
 * Si4463 freq = (outdiv * xtal / 2^19) * (rf_freq_word)
 * For high band (L1), outdiv = 8, so word = freq * 2^19 / (8 * xtal)
 * 1575.42e6 * 2^19 / (8 * 26e6) ≈ 3.97e9 — too large for 32 bits.
 * Use fractional mode with 4-byte property at FREQ_CONTROL_INTE / FRAC.
 */
int si4463_set_frequency(si4463_t *dev, uint32_t freq_hz) {
    if (!dev->initialized) return -1;

    /* Calculate integer/fractional words.
     * VCO frequency = freq_hz * outdiv
     * For high band (>850 MHz), outdiv = 8
     * N_int = floor(VCO / (2 * xtal))
     * We use 26 MHz TCXO.
     */
    uint32_t vco = freq_hz * 8;  /* outdiv = 8 for high band */
    uint32_t denom = 2 * 26000000U;
    uint32_t n_int = vco / denom;
    uint32_t remainder = vco - (n_int * denom);
    /* Fractional part scaled to 19 bits */
    uint32_t n_frac = (uint32_t)((uint64_t)remainder * 524288U / denom);

    uint8_t freq_cmd[6] = {
        SI4463_CMD_SET_PROPERTY,
        SI4463_PROP_GROUP_FREQ, 0x04,
        0x00, /* property start: INTE */
        (uint8_t)(n_int & 0xFF),
        (uint8_t)((n_frac >> 16) & 0xFF),
    };
    if (si4463_cmd(dev->inst, freq_cmd, 6)) return -1;

    uint8_t frac_lo[4] = {
        SI4463_CMD_SET_PROPERTY,
        SI4463_PROP_GROUP_FREQ, 0x04,
        0x01, /* FRAC high */
        (uint8_t)((n_frac >> 8) & 0xFF),
        (uint8_t)(n_frac & 0xFF),
    };
    if (si4463_cmd(dev->inst, frac_lo, 6)) return -1;

    dev->freq_hz = freq_hz;
    return 0;
}

int si4463_set_tx_power(si4463_t *dev, int8_t dbm) {
    if (!dev->initialized) return -1;
    if (dbm > SI4463_PA_MAX_DBM) dbm = SI4463_PA_MAX_DBM;
    if (dbm < SI4463_PA_MIN_DBM) dbm = SI4463_PA_MIN_DBM;

    /* PA power level property (0x2201) — 1 byte mapped to dBm */
    uint8_t level = (uint8_t)((dbm + 30) * 2 + 8);
    si4463_set_property_byte(dev->inst, SI4463_PROP_GROUP_TX,
                              0x01, level);
    dev->tx_power_dbm = dbm;
    return 0;
}

int si4463_start_tx(si4463_t *dev, const uint8_t *data, uint16_t len) {
    if (!dev->initialized) return -1;

    /* Reset TX FIFO */
    uint8_t fifo_reset[3] = { SI4463_CMD_FIFO_INFO, 0x80, 0x00 };
    if (si4463_cmd(dev->inst, fifo_reset, 3)) return -1;

    /* Pre-fill TX FIFO with initial data */
    si4463_tx_fifo_fill(dev, data, len);

    /* Start TX: channel 0, no retransmit, no len field (direct mode) */
    uint8_t start_tx[5] = {
        SI4463_CMD_START_TX,
        0x00, 0x30, 0x00, 0x00
    };
    if (si4463_cmd(dev->inst, start_tx, 5)) return -1;
    dev->state = SI4463_STATE_TX;
    return 0;
}

int si4463_stop_tx(si4463_t *dev) {
    /* Change state to READY */
    uint8_t state_cmd[2] = { SI4463_CMD_CHANGE_STATE,
                             SI4463_STATE_READY };
    si4463_cmd(dev->inst, state_cmd, 2);
    dev->state = SI4463_STATE_READY;
    return 0;
}

int si4463_tx_fifo_write(si4463_t *dev, const uint8_t *data, uint16_t len) {
    si4463_select(dev->inst);
    SPI2->CR2 = (SPI2->CR2 & ~0xFFFFU) | (len + 1);
    SPI2->CR1 |= SPI_CR1_SPE;
    spi2_wait_txe();
    SPI2->TXDR = SI4463_CMD_WRITE_TX_FIFO;
    spi2_wait_eot();
    spi2_wait_rxp();
    (void)SPI2->RXDR;
    for (uint16_t i = 0; i < len; i++) {
        spi2_wait_txe();
        SPI2->TXDR = data[i];
        spi2_wait_eot();
        spi2_wait_rxp();
        (void)SPI2->RXDR;
    }
    SPI2->CR1 &= ~SPI_CR1_SPE;
    si4463_deselect(dev->inst);
    return 0;
}

int si4463_tx_fifo_fill(si4463_t *dev, const uint8_t *data, uint16_t len) {
    /* Write to TX FIFO via WRITE_TX_FIFO command */
    uint8_t cmd = SI4463_CMD_WRITE_TX_FIFO;
    si4463_select(dev->inst);
    SPI2->CR2 = (SPI2->CR2 & ~0xFFFFU) | (len + 1);
    SPI2->CR1 |= SPI_CR1_SPE;
    spi2_wait_txe();
    SPI2->TXDR = cmd;
    spi2_wait_eot();
    spi2_wait_rxp();
    (void)SPI2->RXDR;
    for (uint16_t i = 0; i < len; i++) {
        spi2_wait_txe();
        SPI2->TXDR = data[i];
        spi2_wait_eot();
        spi2_wait_rxp();
        (void)SPI2->RXDR;
    }
    SPI2->CR1 &= ~SPI_CR1_SPE;
    si4463_deselect(dev->inst);
    return len;
}

uint8_t si4463_get_state(si4463_t *dev) {
    uint8_t cmd = SI4463_CMD_REQUEST_DEVICE_STATE;
    uint8_t resp[2];
    if (si4463_cmd_read(dev->inst, &cmd, 1, resp, 2))
        return 0xFF;
    dev->state = resp[0];
    return resp[0];
}

uint8_t si4463_get_part_info(si4463_t *dev) {
    uint8_t cmd = SI4463_CMD_PART_INFO;
    uint8_t resp[8];
    if (si4463_cmd_read(dev->inst, &cmd, 1, resp, 8))
        return 0;
    return resp[0];
}