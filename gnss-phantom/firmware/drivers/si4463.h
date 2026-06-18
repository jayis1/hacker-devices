/*
 * drivers/si4463.h — Si4463 RF transceiver driver for GNSS-Phantom
 *
 * The Si4463 is a 142-1050 MHz + 2.4 GHz ISM transceiver used here as the
 * direct-synthesis RF source for GPS/Galileo/GLONASS/BeiDou L1 spoofing.
 * We operate it in direct TX mode with continuous phase modulation to
 * synthesize the BPSK-modulated C/A code + NAV data signal at L1.
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#ifndef GNSS_PHANTOM_SI4463_H
#define GNSS_PHANTOM_SI4463_H

#include <stdint.h>
#include "../board.h"

/* Si4463 register addresses (API config commands) */
#define SI4463_CMD_PART_INFO        0x01
#define SI4463_CMD_FUNC_INFO        0x10
#define SI4463_CMD_SET_PROPERTY     0x11
#define SI4463_CMD_GET_PROPERTY     0x12
#define SI4463_CMD_GPIO_PIN_CFG     0x13
#define SI4463_CMD_FIFO_INFO        0x15
#define SI4463_CMD_RX_FIFO_RESET    0x15
#define SI4463_CMD_TX_FIFO_RESET    0x15
#define SI4463_CMD_PACKET_INFO      0x16
#define SI4463_CMD_READ_CMD_BUFF    0x44
#define SI4463_CMD_READ_RX_FIFO     0x77
#define SI4463_CMD_WRITE_TX_FIFO    0x66
#define SI4463_CMD_CHANGE_STATE     0x34
#define SI4463_CMD_POWER_UP         0x02
#define SI4463_CMD_POWER_DOWN        0x01
#define SI4463_CMD_GET_INT_STATUS   0x20
#define SI4463_CMD_GET_PH_STATUS     0x21
#define SI4463_CMD_GET_MODEM_STATUS 0x22
#define SI4463_CMD_GET_ADC_VALUE    0x14
#define SI4463_CMD_START_TX         0x31
#define SI4463_CMD_START_RX         0x32
#define SI4463_CMD_REQUEST_DEVICE_STATE 0x33
#define SI4463_CMD_FAST_WAKEUP      0x56

/* Property groups */
#define SI4463_PROP_GROUP_MODEM     0x20
#define SI4463_PROP_GROUP_PREAMBLE  0x10
#define SI4463_PROP_GROUP_PKT       0x12
#define SI4463_PROP_GROUP_FREQ      0x40
#define SI4463_PROP_GROUP_TX        0x22
#define SI4463_PROP_GROUP_RX        0x21
#define SI4463_PROP_GROUP_PA        0x22
#define SI4463_PROP_GROUP_SYNTH     0x40
#define SI4463_PROP_GROUP_GLOBAL    0x00

/* Modem properties */
#define SI4463_PROP_MODEM_MOD_TYPE  0x01
#define SI4463_PROP_MODEM_TX_NCO_MODE 0x06
#define SI4463_PROP_MODEM_FREQ_DEV  0x0A
#define SI4463_PROP_MODEM_CLKGEN_BAND 0x20
#define SI4463_PROP_MODEM_BCR_OSR  0x12
#define SI4463_PROP_MODEM_TX_RATE   0x03

/* Radio state machine states */
#define SI4463_STATE_NO_CHANGE      0x00
#define SI4463_STATE_SLEEP          0x01
#define SI4463_STATE_SPI_ACTIVE     0x02
#define SI4463_STATE_READY          0x03
#define SI4463_STATE_TUNE_TX        0x05
#define SI4463_STATE_TX             0x07
#define SI4463_STATE_TUNE_RX        0x08
#define SI4463_STATE_RX             0x09

/* TX complete / FIFO definitions */
#define SI4463_FIFO_TX_COMPLETE     0x01
#define SI4463_FIFO_TX_ALMOST_EMPTY 0x20

/* TX power levels (in dBm, via PA config property) */
#define SI4463_PA_MAX_DBM           20
#define SI4463_PA_MIN_DBM           -30

/* Device instances */
typedef enum {
    SI4463_GPS = 0,     /* GPS L1 / Galileo E1 */
    SI4463_GLO = 1,      /* GLONASS L1 / BeiDou B1 */
} si4463_inst_t;

/* Status / CTS polling */
#define SI4463_CTS_READY             0xFF

typedef struct {
    si4463_inst_t inst;
    uint32_t freq_hz;
    int8_t tx_power_dbm;
    uint8_t state;
    uint8_t initialized;
} si4463_t;

/* API */
int si4463_init(si4463_t *dev, si4463_inst_t inst);
int si4463_set_frequency(si4463_t *dev, uint32_t freq_hz);
int si4463_set_tx_power(si4463_t *dev, int8_t dbm);
int si4463_start_tx(si4463_t *dev, const uint8_t *data, uint16_t len);
int si4463_stop_tx(si4463_t *dev);
int si4463_tx_fifo_write(si4463_t *dev, const uint8_t *data, uint16_t len);
int si4463_tx_fifo_fill(si4463_t *dev, const uint8_t *data, uint16_t len);
uint8_t si4463_get_state(si4463_t *dev);
uint8_t si4463_get_part_info(si4463_t *dev);
void si4463_shutdown(si4463_t *dev);
void si4463_reset(si4463_t *dev);

/* Low-level SPI helpers (in .c) */
static inline void si4463_select(si4463_inst_t inst) {
    if (inst == SI4463_GPS) {
        gpio_write(SI4463_PORT, SI4463_SEL_PIN, 0);
    } else {
        gpio_write(SI4463B_PORT, SI4463B_SEL_PIN, 0);
    }
}

static inline void si4463_deselect(si4463_inst_t inst) {
    if (inst == SI4463_GPS) {
        gpio_write(SI4463_PORT, SI4463_SEL_PIN, 1);
    } else {
        gpio_write(SI4463B_PORT, SI4463B_SEL_PIN, 1);
    }
}

#endif /* GNSS_PHANTOM_SI4463_H */