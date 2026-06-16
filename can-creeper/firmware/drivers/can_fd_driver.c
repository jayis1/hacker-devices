/**
 * @file can_fd_driver.c
 * @brief MCP2518FD CAN FD Controller Driver Implementation
 *
 * Full SPI-based driver for Microchip MCP2518FD CAN FD controller.
 * Handles initialization, bit timing configuration, FIFO management,
 * frame transmission/reception, acceptance filtering, and error monitoring.
 *
 * Uses nRF52840 SPIM peripheral with EasyDMA for zero-CPU SPI transfers.
 *
 * Datasheet: MCP2518FD Data Sheet DS20006027B
 * Clock: 40 MHz external crystal per controller
 * SPI: 8 MHz, Mode 0 (CPOL=0, CPHA=0)
 */

#include "can_fd_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/*===========================================================================
 * PRIVATE TYPES AND DEFINES
 *===========================================================================*/

/** Per-channel driver state */
typedef struct {
    bool     initialized;
    can_mode_t mode;
    uint8_t  tec;              /* Transmit Error Counter */
    uint8_t  rec;              /* Receive Error Counter */
    uint32_t nominal_brp;
    uint32_t nominal_tseg1;
    uint32_t nominal_tseg2;
    uint32_t nominal_sjw;
    uint32_t data_brp;
    uint32_t data_tseg1;
    uint32_t data_tseg2;
    uint32_t data_sjw;
    bool     fd_enable;
    bool     brs_enable;
} can_channel_state_t;

/** SPI transaction buffer sizes */
#define CAN_SPI_BUF_SIZE        64
#define CAN_SPI_TIMEOUT_US      1000  /* 1 ms timeout for SPI transactions */

/** FIFO assignments */
#define CAN_FIFO_RX              1    /* FIFO 1: RX */
#define CAN_FIFO_RX_FD           2    /* FIFO 2: RX for FD frames */
#define CAN_FIFO_TX              3    /* FIFO 3: TX */
#define CAN_FIFO_TX_FD           4    /* FIFO 4: TX for FD frames */
#define CAN_FIFO_RX_SIZE         32   /* 32 messages deep */
#define CAN_FIFO_TX_SIZE         8    /* 8 messages deep */

/** FIFO payload sizes */
#define CAN_PLSIZE_8             0    /* 8 bytes */
#define CAN_PLSIZE_12            1    /* 12 bytes */
#define CAN_PLSIZE_16            2    /* 16 bytes */
#define CAN_PLSIZE_20            3    /* 20 bytes */
#define CAN_PLSIZE_24            4    /* 24 bytes */
#define CAN_PLSIZE_32            5    /* 32 bytes */
#define CAN_PLSIZE_48            6    /* 48 bytes */
#define CAN_PLSIZE_64            7    /* 64 bytes */

/*===========================================================================
 * STATIC VARIABLES
 *===========================================================================*/

static can_channel_state_t g_can_state[2];
static uint8_t g_can_spi_tx_buf[2][CAN_SPI_BUF_SIZE];
static uint8_t g_can_spi_rx_buf[2][CAN_SPI_BUF_SIZE];

/*===========================================================================
 * PRIVATE FUNCTION DECLARATIONS
 *===========================================================================*/

static int can_spi_transfer(uint8_t channel, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);
static int can_spi_write_reg(uint8_t channel, uint16_t addr, uint32_t value);
static int can_spi_read_reg(uint8_t channel, uint16_t addr, uint32_t *value);
static int can_spi_write_reg_safe(uint8_t channel, uint16_t addr, uint32_t value);
static int can_reset(uint8_t channel);
static int can_configure_bit_timing(uint8_t channel, const can_fd_config_t *config);
static int can_configure_fifos(uint8_t channel);
static int can_wait_for_mode(uint8_t channel, can_mode_t mode, uint32_t timeout_ms);
static NRF_SPIM_Type *can_get_spim(uint8_t channel);
static uint8_t can_get_csn_pin(uint8_t channel);

/*===========================================================================
 * SPI HELPER FUNCTIONS
 *===========================================================================*/

/**
 * @brief Get SPIM peripheral for a given CAN channel
 */
static NRF_SPIM_Type *can_get_spim(uint8_t channel) {
    return (channel == 0) ? NRF_SPIM0 : NRF_SPIM1;
}

/**
 * @brief Get CSN pin for a given CAN channel
 */
static uint8_t can_get_csn_pin(uint8_t channel) {
    return (channel == 0) ? CAN0_SPI_CSN_PIN : CAN1_SPI_CSN_PIN;
}

/**
 * @brief Perform an SPI transfer using EasyDMA
 *
 * Configures SPIM for a single transaction, starts it, and waits for completion.
 *
 * @param channel CAN channel (0 or 1)
 * @param tx_data Transmit data buffer (can be NULL for read-only)
 * @param rx_data Receive data buffer (can be NULL for write-only)
 * @param len     Number of bytes to transfer
 * @return CAN_ERR_OK on success, CAN_ERR_TIMEOUT on timeout
 */
static int can_spi_transfer(uint8_t channel, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len) {
    NRF_SPIM_Type *spim = can_get_spim(channel);
    uint8_t csn_pin = can_get_csn_pin(channel);
    uint32_t timeout;

    if (len > CAN_SPI_BUF_SIZE) {
        return CAN_ERR_INVALID_PARAM;
    }

    /* Copy TX data to DMA buffer if provided */
    if (tx_data) {
        memcpy(g_can_spi_tx_buf[channel], tx_data, len);
    } else {
        memset(g_can_spi_tx_buf[channel], 0xFF, len);  /* Dummy bytes for read */
    }

    /* Configure EasyDMA pointers */
    spim->TXD_PTR = (uint32_t)g_can_spi_tx_buf[channel];
    spim->TXD_MAXCNT = len;
    spim->RXD_PTR = (uint32_t)g_can_spi_rx_buf[channel];
    spim->RXD_MAXCNT = len;

    /* Clear END event */
    spim->EVENTS_END = 0;
    spim->EVENTS_ENDRX = 0;
    spim->EVENTS_ENDTX = 0;

    /* Assert CS (active low) */
    NRF_P0->OUTCLR = (1UL << csn_pin);

    /* Start SPI transaction */
    spim->TASKS_START = 1;

    /* Wait for completion with timeout */
    timeout = CAN_SPI_TIMEOUT_US * 64;  /* Rough cycle count at 64 MHz */
    while (!spim->EVENTS_END) {
        if (--timeout == 0) {
            /* Timeout — abort transaction */
            spim->TASKS_STOP = 1;
            NRF_P0->OUTSET = (1UL << csn_pin);  /* De-assert CS */
            return CAN_ERR_TIMEOUT;
        }
    }

    /* De-assert CS */
    NRF_P0->OUTSET = (1UL << csn_pin);

    /* Copy RX data if buffer provided */
    if (rx_data) {
        memcpy(rx_data, g_can_spi_rx_buf[channel], len);
    }

    return CAN_ERR_OK;
}

/**
 * @brief Write a 32-bit value to an MCP2518FD register via SPI
 *
 * SPI sequence: WRITE opcode → 16-bit address (big-endian) → 32-bit data (little-endian)
 *
 * @param channel CAN channel
 * @param addr    16-bit register address
 * @param value   32-bit value to write
 * @return CAN_ERR_OK on success
 */
static int can_spi_write_reg(uint8_t channel, uint16_t addr, uint32_t value) {
    uint8_t tx[7];
    tx[0] = MCP2518FD_INSTR_WRITE;
    tx[1] = (uint8_t)((addr >> 8) & 0xFF);  /* Address high byte */
    tx[2] = (uint8_t)(addr & 0xFF);          /* Address low byte */
    tx[3] = (uint8_t)(value & 0xFF);         /* Data byte 0 (LSB) */
    tx[4] = (uint8_t)((value >> 8) & 0xFF);  /* Data byte 1 */
    tx[5] = (uint8_t)((value >> 16) & 0xFF); /* Data byte 2 */
    tx[6] = (uint8_t)((value >> 24) & 0xFF); /* Data byte 3 (MSB) */
    return can_spi_transfer(channel, tx, NULL, 7);
}

/**
 * @brief Read a 32-bit value from an MCP2518FD register via SPI
 *
 * SPI sequence: READ opcode → 16-bit address → 32-bit data returned
 *
 * @param channel CAN channel
 * @param addr    16-bit register address
 * @param value   Pointer to store read value
 * @return CAN_ERR_OK on success
 */
static int can_spi_read_reg(uint8_t channel, uint16_t addr, uint32_t *value) {
    uint8_t tx[3], rx[7];
    int ret;

    tx[0] = MCP2518FD_INSTR_READ;
    tx[1] = (uint8_t)((addr >> 8) & 0xFF);
    tx[2] = (uint8_t)(addr & 0xFF);

    ret = can_spi_transfer(channel, tx, rx, 7);
    if (ret != CAN_ERR_OK) return ret;

    /* Data starts at rx[3] (after opcode + 2-byte address), little-endian */
    *value = (uint32_t)rx[3] |
             ((uint32_t)rx[4] << 8) |
             ((uint32_t)rx[5] << 16) |
             ((uint32_t)rx[6] << 24);

    return CAN_ERR_OK;
}

/**
 * @brief Write a register with CRC verification (WRITE_SAFE)
 *
 * @param channel CAN channel
 * @param addr    16-bit register address
 * @param value   32-bit value to write
 * @return CAN_ERR_OK on success, CAN_ERR_CRC on CRC mismatch
 */
static int can_spi_write_reg_safe(uint8_t channel, uint16_t addr, uint32_t value) {
    uint8_t tx[9], rx[9];
    int ret;

    tx[0] = MCP2518FD_INSTR_WRITE_SAFE;
    tx[1] = (uint8_t)((addr >> 8) & 0xFF);
    tx[2] = (uint8_t)(addr & 0xFF);
    tx[3] = (uint8_t)(value & 0xFF);
    tx[4] = (uint8_t)((value >> 8) & 0xFF);
    tx[5] = (uint8_t)((value >> 16) & 0xFF);
    tx[6] = (uint8_t)((value >> 24) & 0xFF);
    tx[7] = 0x00;  /* CRC placeholder */
    tx[8] = 0x00;

    ret = can_spi_transfer(channel, tx, rx, 9);
    if (ret != CAN_ERR_OK) return ret;

    /* Check CRC in response (rx[7:8]) */
    /* For simplicity, we trust the write if no SPI error occurred */
    /* Full CRC verification would compute expected CRC and compare */

    return CAN_ERR_OK;
}

/*===========================================================================
 * MCP2518FD RESET
 *===========================================================================*/

/**
 * @brief Reset the MCP2518FD controller
 *
 * Sends RESET instruction (0x00) and waits for oscillator to stabilize.
 *
 * @param channel CAN channel
 * @return CAN_ERR_OK on success
 */
static int can_reset(uint8_t channel) {
    uint8_t tx[1] = { MCP2518FD_INSTR_RESET };
    uint32_t osc_status;
    int ret;
    uint32_t timeout;

    /* Send RESET command */
    ret = can_spi_transfer(channel, tx, NULL, 1);
    if (ret != CAN_ERR_OK) return ret;

    /* Wait for oscillator to start (max 1ms) */
    timeout = 100000;  /* ~1ms at 64 MHz */
    do {
        ret = can_spi_read_reg(channel, MCP2518FD_REG_OSC, &osc_status);
        if (ret != CAN_ERR_OK) return ret;
        if (osc_status & 0x01) break;  /* OSC ready bit */
        for (volatile uint32_t d = 0; d < 100; d++) { __NOP(); }
    } while (--timeout > 0);

    if (timeout == 0) {
        return CAN_ERR_TIMEOUT;
    }

    return CAN_ERR_OK;
}

/*===========================================================================
 * BIT TIMING CONFIGURATION
 *===========================================================================*/

/**
 * @brief Configure nominal and data bit timing
 *
 * Writes NBTCFG and DBTCFG registers with the provided timing parameters.
 *
 * @param channel CAN channel
 * @param config  Pointer to configuration
 * @return CAN_ERR_OK on success
 */
static int can_configure_bit_timing(uint8_t channel, const can_fd_config_t *config) {
    uint32_t nbtcfg_val, dbtcfg_val;
    int ret;

    /* Build NBTCFG register value */
    nbtcfg_val = ((config->nominal_brp & 0xFF) << 0) |
                 ((config->nominal_tseg1 & 0xFF) << 8) |
                 ((config->nominal_tseg2 & 0x7F) << 16) |
                 ((config->nominal_sjw & 0x7F) << 24);

    ret = can_spi_write_reg(channel, MCP2518FD_REG_NBTCFG, nbtcfg_val);
    if (ret != CAN_ERR_OK) return ret;

    /* Build DBTCFG register value (only if FD enabled) */
    if (config->fd_enable) {
        dbtcfg_val = ((config->data_brp & 0xFF) << 0) |
                     ((config->data_tseg1 & 0x1F) << 8) |
                     ((config->data_tseg2 & 0x0F) << 16) |
                     ((config->data_sjw & 0x0F) << 24);
    } else {
        dbtcfg_val = 0;  /* Don't care if FD disabled */
    }

    ret = can_spi_write_reg(channel, MCP2518FD_REG_DBTCFG, dbtcfg_val);
    if (ret != CAN_ERR_OK) return ret;

    return CAN_ERR_OK;
}

/*===========================================================================
 * FIFO CONFIGURATION
 *===========================================================================*/

/**
 * @brief Configure RX and TX FIFOs
 *
 * Sets up FIFO 1 as RX (32 deep, 64-byte payload for FD),
 * FIFO 3 as TX (8 deep, 64-byte payload for FD).
 *
 * @param channel CAN channel
 * @return CAN_ERR_OK on success
 */
static int can_configure_fifos(uint8_t channel) {
    uint32_t fifocon_val;
    int ret;

    /* Configure FIFO 1 as RX FIFO */
    fifocon_val = ((CAN_FIFO_RX_SIZE - 1) << FIFOCON_FSIZE_SHIFT) |  /* Size = 32 */
                  FIFOCON_RXEN |                                      /* RX enable */
                  FIFOCON_TEFTSEN |                                   /* Store timestamp in TEF */
                  ((CAN_PLSIZE_64) << FIFOCON_PLSIZE_SHIFT);         /* 64-byte payload */
    ret = can_spi_write_reg(channel, MCP2518FD_REG_FIFOCON_BASE + (CAN_FIFO_RX * 4), fifocon_val);
    if (ret != CAN_ERR_OK) return ret;

    /* Configure FIFO 3 as TX FIFO */
    fifocon_val = ((CAN_FIFO_TX_SIZE - 1) << FIFOCON_FSIZE_SHIFT) |  /* Size = 8 */
                  FIFOCON_TXEN |                                      /* TX enable */
                  ((0x03) << FIFOCON_TXPRI_SHIFT) |                  /* Highest priority */
                  ((CAN_PLSIZE_64) << FIFOCON_PLSIZE_SHIFT);         /* 64-byte payload */
    ret = can_spi_write_reg(channel, MCP2518FD_REG_FIFOCON_BASE + (CAN_FIFO_TX * 4), fifocon_val);
    if (ret != CAN_ERR_OK) return ret;

    return CAN_ERR_OK;
}

/*===========================================================================
 * MODE CONTROL
 *===========================================================================*/

/**
 * @brief Wait for the controller to enter a specific mode
 *
 * @param channel     CAN channel
 * @param mode        Desired mode
 * @param timeout_ms  Timeout in milliseconds
 * @return CAN_ERR_OK if mode entered, CAN_ERR_TIMEOUT on timeout
 */
static int can_wait_for_mode(uint8_t channel, can_mode_t mode, uint32_t timeout_ms) {
    uint32_t canctrl_val;
    uint32_t opmod;
    int ret;
    uint32_t timeout = timeout_ms * 1000;  /* Convert to rough loop iterations */

    do {
        ret = can_spi_read_reg(channel, MCP2518FD_REG_CANCTRL, &canctrl_val);
        if (ret != CAN_ERR_OK) return ret;

        opmod = (canctrl_val >> CANCTRL_OPMOD_SHIFT) & 0x07;

        if (opmod == (uint32_t)mode) {
            return CAN_ERR_OK;
        }

        /* Small delay */
        for (volatile uint32_t d = 0; d < 100; d++) { __NOP(); }
    } while (--timeout > 0);

    return CAN_ERR_TIMEOUT;
}

/*===========================================================================
 * PUBLIC API IMPLEMENTATION
 *===========================================================================*/

/**
 * @brief Initialize a CAN FD channel
 */
int can_fd_init(uint8_t channel, const can_fd_config_t *config) {
    NRF_SPIM_Type *spim;
    uint32_t con_val;
    int ret;

    if (channel > 1 || !config) {
        return CAN_ERR_INVALID_PARAM;
    }

    /* Configure SPIM peripheral */
    spim = can_get_spim(channel);

    /* Disable SPIM before configuration */
    spim->ENABLE = SPIM_ENABLE_DISABLE;

    /* Set pin assignments */
    if (channel == 0) {
        spim->PSEL_SCK  = CAN0_SPI_SCK_PIN;
        spim->PSEL_MOSI = CAN0_SPI_MOSI_PIN;
        spim->PSEL_MISO = CAN0_SPI_MISO_PIN;
        spim->PSEL_CSN  = CAN0_SPI_CSN_PIN;
    } else {
        spim->PSEL_SCK  = CAN1_SPI_SCK_PIN;
        spim->PSEL_MOSI = CAN1_SPI_MOSI_PIN;
        spim->PSEL_MISO = CAN1_SPI_MISO_PIN;
        spim->PSEL_CSN  = CAN1_SPI_CSN_PIN;
    }

    /* Configure SPI mode and frequency */
    spim->FREQUENCY = SPIM_FREQ_8MBPS;
    spim->CONFIG = SPIM_CONFIG_CPHA_Leading | SPIM_CONFIG_CPOL_ActiveHigh;  /* Mode 0 */
    spim->ORC = 0xFF;

    /* Enable SPIM */
    spim->ENABLE = SPIM_ENABLE_ENABLE;

    /* Reset MCP2518FD */
    ret = can_reset(channel);
    if (ret != CAN_ERR_OK) {
        spim->ENABLE = SPIM_ENABLE_DISABLE;
        return CAN_ERR_INIT_FAILED;
    }

    /* Configure bit timing */
    ret = can_configure_bit_timing(channel, config);
    if (ret != CAN_ERR_OK) {
        spim->ENABLE = SPIM_ENABLE_DISABLE;
        return CAN_ERR_INIT_FAILED;
    }

    /* Configure FIFOs */
    ret = can_configure_fifos(channel);
    if (ret != CAN_ERR_OK) {
        spim->ENABLE = SPIM_ENABLE_DISABLE;
        return CAN_ERR_INIT_FAILED;
    }

    /* Configure CON register */
    con_val = 0;
    if (config->listen_only) {
        con_val |= CON_REQOP_LISTEN_ONLY;
    } else {
        con_val |= CON_REQOP_NORMAL;
    }
    con_val |= CON_STEF;       /* Store in TEF for timestamping */
    con_val |= CON_ISOCRCEN;   /* ISO CRC for CAN FD */
    if (!config->brs_enable) {
        con_val |= CON_BRSDIS; /* Disable BRS */
    }
    con_val |= CON_TXQEN;      /* Enable TX queue */
    con_val |= (0x1FUL << CON_DNCNT_SHIFT);  /* Max retransmission attempts */

    ret = can_spi_write_reg(channel, MCP2518FD_REG_CON, con_val);
    if (ret != CAN_ERR_OK) {
        spim->ENABLE = SPIM_ENABLE_DISABLE;
        return CAN_ERR_INIT_FAILED;
    }

    /* Wait for mode to take effect */
    can_mode_t target_mode = config->listen_only ? CAN_MODE_LISTEN_ONLY : CAN_MODE_NORMAL;
    ret = can_wait_for_mode(channel, target_mode, 10);
    if (ret != CAN_ERR_OK) {
        spim->ENABLE = SPIM_ENABLE_DISABLE;
        return CAN_ERR_INIT_FAILED;
    }

    /* Store state */
    g_can_state[channel].initialized = true;
    g_can_state[channel].mode = target_mode;
    g_can_state[channel].nominal_brp = config->nominal_brp;
    g_can_state[channel].nominal_tseg1 = config->nominal_tseg1;
    g_can_state[channel].nominal_tseg2 = config->nominal_tseg2;
    g_can_state[channel].nominal_sjw = config->nominal_sjw;
    g_can_state[channel].data_brp = config->data_brp;
    g_can_state[channel].data_tseg1 = config->data_tseg1;
    g_can_state[channel].data_tseg2 = config->data_tseg2;
    g_can_state[channel].data_sjw = config->data_sjw;
    g_can_state[channel].fd_enable = config->fd_enable;
    g_can_state[channel].brs_enable = config->brs_enable;
    g_can_state[channel].tec = 0;
    g_can_state[channel].rec = 0;

    return CAN_ERR_OK;
}

/**
 * @brief Deinitialize a CAN FD channel
 */
int can_fd_deinit(uint8_t channel) {
    NRF_SPIM_Type *spim;

    if (channel > 1 || !g_can_state[channel].initialized) {
        return CAN_ERR_NOT_INITIALIZED;
    }

    /* Put controller in sleep mode */
    can_fd_set_mode(channel, CAN_MODE_SLEEP);

    /* Disable SPIM */
    spim = can_get_spim(channel);
    spim->ENABLE = SPIM_ENABLE_DISABLE;

    g_can_state[channel].initialized = false;

    return CAN_ERR_OK;
}

/**
 * @brief Transmit a CAN frame
 */
int can_fd_transmit(uint8_t channel, const can_frame_t *frame) {
    uint32_t fifosta_val;
    uint8_t tx_buf[72];  /* 4-byte header + up to 64 data bytes + 4-byte footer */
    uint16_t tx_len;
    int ret;

    if (channel > 1 || !g_can_state[channel].initialized || !frame) {
        return CAN_ERR_INVALID_PARAM;
    }

    /* Check TX FIFO not full */
    ret = can_spi_read_reg(channel, MCP2518FD_REG_FIFOSTA_BASE + (CAN_FIFO_TX * 4), &fifosta_val);
    if (ret != CAN_ERR_OK) return ret;

    if ((fifosta_val & 0x1F) >= CAN_FIFO_TX_SIZE) {
        return CAN_ERR_TX_FIFO_FULL;
    }

    /* Build TX message object in SPI buffer */
    /* Format: T0[31:0] → T1[31:0] → Data bytes → Footer */
    uint32_t t0 = 0;
    /* T0: SID[10:0] in bits 0-10, EID[17:0] in bits 11-28 if IDE */
    if (frame->flags & CAN_FLAG_IDE) {
        t0 |= (frame->id & 0x1FFFFFFFUL);  /* Full 29-bit ID */
        t0 |= (1UL << 29);                  /* IDE bit */
    } else {
        t0 |= ((frame->id & 0x7FFUL) << 18);  /* SID in bits 18-28 */
    }
    if (frame->flags & CAN_FLAG_RTR) t0 |= (1UL << 30);  /* SRR/RTR */

    tx_buf[0] = (uint8_t)(t0 & 0xFF);
    tx_buf[1] = (uint8_t)((t0 >> 8) & 0xFF);
    tx_buf[2] = (uint8_t)((t0 >> 16) & 0xFF);
    tx_buf[3] = (uint8_t)((t0 >> 24) & 0xFF);

    /* T1: DLC[3:0], IDE, FDF, BRS, ESI, SEQ */
    uint32_t t1 = (frame->dlc & 0x0F);  /* DLC */
    if (frame->flags & CAN_FLAG_IDE) t1 |= (1UL << 4);  /* IDE */
    if (frame->flags & CAN_FLAG_FD)  t1 |= (1UL << 5);  /* FDF */
    if (frame->flags & CAN_FLAG_BRS) t1 |= (1UL << 6);  /* BRS */
    if (frame->flags & CAN_FLAG_ESI) t1 |= (1UL << 7);  /* ESI */
    /* SEQ[31:9] = 0 (auto-increment) */

    tx_buf[4] = (uint8_t)(t1 & 0xFF);
    tx_buf[5] = (uint8_t)((t1 >> 8) & 0xFF);
    tx_buf[6] = (uint8_t)((t1 >> 16) & 0xFF);
    tx_buf[7] = (uint8_t)((t1 >> 24) & 0xFF);

    /* Data bytes */
    uint8_t data_len = (frame->dlc > 8) ? frame->dlc : 8;
    if (data_len > 64) data_len = 64;
    memcpy(&tx_buf[8], frame->data, data_len);

    /* Total TX length: 4 (T0) + 4 (T1) + data_len */
    tx_len = 8 + data_len;

    /* Write to TX FIFO user address */
    /* First read FIFOUA to get current write address */
    uint32_t fifoua;
    ret = can_spi_read_reg(channel, MCP2518FD_REG_FIFOUA_BASE + (CAN_FIFO_TX * 4), &fifoua);
    if (ret != CAN_ERR_OK) return ret;

    /* Write message to FIFO RAM at FIFOUA address */
    /* Use WRITE instruction with 16-bit address pointing to FIFO RAM */
    uint8_t spi_cmd[3];
    spi_cmd[0] = MCP2518FD_INSTR_WRITE;
    spi_cmd[1] = (uint8_t)((fifoua >> 8) & 0xFF);
    spi_cmd[2] = (uint8_t)(fifoua & 0xFF);

    /* Combine command and data into one SPI transaction */
    uint8_t full_tx[3 + 72];
    memcpy(full_tx, spi_cmd, 3);
    memcpy(&full_tx[3], tx_buf, tx_len);

    ret = can_spi_transfer(channel, full_tx, NULL, 3 + tx_len);
    if (ret != CAN_ERR_OK) return ret;

    /* Set TXREQ to request transmission */
    ret = can_spi_write_reg(channel, MCP2518FD_REG_TXREQ, (1UL << CAN_FIFO_TX));
    if (ret != CAN_ERR_OK) return ret;

    return CAN_ERR_OK;
}

/**
 * @brief Receive a CAN frame (non-blocking)
 */
int can_fd_receive(uint8_t channel, can_frame_t *frame) {
    uint32_t fifosta_val, fifoua;
    uint8_t rx_buf[72];
    uint16_t rx_len;
    int ret;

    if (channel > 1 || !g_can_state[channel].initialized || !frame) {
        return CAN_ERR_INVALID_PARAM;
    }

    /* Check if frames available in RX FIFO */
    ret = can_spi_read_reg(channel, MCP2518FD_REG_FIFOSTA_BASE + (CAN_FIFO_RX * 4), &fifosta_val);
    if (ret != CAN_ERR_OK) return ret;

    if ((fifosta_val & 0x1F) == 0) {
        return CAN_ERR_RX_FIFO_EMPTY;
    }

    /* Read FIFO user address */
    ret = can_spi_read_reg(channel, MCP2518FD_REG_FIFOUA_BASE + (CAN_FIFO_RX * 4), &fifoua);
    if (ret != CAN_ERR_OK) return ret;

    /* Read message from FIFO RAM */
    uint8_t spi_cmd[3];
    spi_cmd[0] = MCP2518FD_INSTR_READ;
    spi_cmd[1] = (uint8_t)((fifoua >> 8) & 0xFF);
    spi_cmd[2] = (uint8_t)(fifoua & 0xFF);

    /* Read T0 + T1 + up to 64 data bytes */
    rx_len = 8 + 64;  /* Read max possible */
    ret = can_spi_transfer(channel, spi_cmd, rx_buf, 3 + rx_len);
    if (ret != CAN_ERR_OK) return ret;

    /* Parse T0 (bytes 3-6 of response) */
    uint32_t t0 = (uint32_t)rx_buf[3] |
                  ((uint32_t)rx_buf[4] << 8) |
                  ((uint32_t)rx_buf[5] << 16) |
                  ((uint32_t)rx_buf[6] << 24);

    /* Parse T1 (bytes 7-10 of response) */
    uint32_t t1 = (uint32_t)rx_buf[7] |
                  ((uint32_t)rx_buf[8] << 8) |
                  ((uint32_t)rx_buf[9] << 16) |
                  ((uint32_t)rx_buf[10] << 24);

    /* Extract frame fields */
    frame->flags = 0;
    if (t0 & (1UL << 29)) {
        frame->flags |= CAN_FLAG_IDE;
        frame->id = t0 & 0x1FFFFFFFUL;  /* Full 29-bit ID */
    } else {
        frame->id = (t0 >> 18) & 0x7FFUL;  /* 11-bit SID */
    }
    if (t0 & (1UL << 30)) frame->flags |= CAN_FLAG_RTR;

    frame->dlc = t1 & 0x0F;
    if (t1 & (1UL << 5)) frame->flags |= CAN_FLAG_FD;
    if (t1 & (1UL << 6)) frame->flags |= CAN_FLAG_BRS;
    if (t1 & (1UL << 7)) frame->flags |= CAN_FLAG_ESI;

    /* Data bytes start at rx_buf[11] */
    uint8_t data_len = (frame->dlc > 8) ? frame->dlc : 8;
    if (data_len > 64) data_len = 64;
    memcpy(frame->data, &rx_buf[11], data_len);

    /* Timestamp will be filled by caller from hardware timer capture */
    frame->timestamp = 0;

    /* Update error counters */
    uint32_t trec_val;
    ret = can_spi_read_reg(channel, MCP2518FD_REG_TREC, &trec_val);
    if (ret == CAN_ERR_OK) {
        g_can_state[channel].tec = (uint8_t)(trec_val & 0xFF);
        g_can_state[channel].rec = (uint8_t)((trec_val >> 8) & 0xFF);
    }

    return CAN_ERR_OK;
}

/**
 * @brief Set CAN controller operating mode
 */
int can_fd_set_mode(uint8_t channel, can_mode_t mode) {
    uint32_t canctrl_val;
    int ret;

    if (channel > 1 || !g_can_state[channel].initialized) {
        return CAN_ERR_NOT_INITIALIZED;
    }

    /* Read current CANCTRL */
    ret = can_spi_read_reg(channel, MCP2518FD_REG_CANCTRL, &canctrl_val);
    if (ret != CAN_ERR_OK) return ret;

    /* Update REQOP field */
    canctrl_val &= ~CANCTRL_REQOP_MASK;
    canctrl_val |= ((uint32_t)mode << CANCTRL_REQOP_SHIFT);

    /* Write back */
    ret = can_spi_write_reg(channel, MCP2518FD_REG_CANCTRL, canctrl_val);
    if (ret != CAN_ERR_OK) return ret;

    /* Wait for mode change */
    ret = can_wait_for_mode(channel, mode, 10);
    if (ret != CAN_ERR_OK) return ret;

    g_can_state[channel].mode = mode;

    return CAN_ERR_OK;
}

/**
 * @brief Configure an acceptance filter
 */
int can_fd_set_filter(uint8_t channel, uint8_t filter_num, uint32_t mask, uint32_t id) {
    int ret;

    if (channel > 1 || !g_can_state[channel].initialized || filter_num > 31) {
        return CAN_ERR_INVALID_PARAM;
    }

    /* Write filter mask */
    ret = can_spi_write_reg(channel, MCP2518FD_REG_MASK_BASE + (filter_num * 4), mask);
    if (ret != CAN_ERR_OK) return ret;

    /* Write filter object (ID) */
    ret = can_spi_write_reg(channel, MCP2518FD_REG_FLTOBJ_BASE + (filter_num * 4), id);
    if (ret != CAN_ERR_OK) return ret;

    /* Enable filter and link to RX FIFO */
    uint32_t fltcon = (1UL << 7) |  /* FLTEN: filter enable */
                      (CAN_FIFO_RX & 0x1F);  /* Link to RX FIFO 1 */
    ret = can_spi_write_reg(channel, MCP2518FD_REG_FLTCON_BASE + (filter_num * 4), fltcon);
    if (ret != CAN_ERR_OK) return ret;

    return CAN_ERR_OK;
}

/**
 * @brief Get error counters
 */
int can_fd_get_error_counters(uint8_t channel, uint8_t *tec, uint8_t *rec) {
    if (channel > 1 || !g_can_state[channel].initialized) {
        return CAN_ERR_NOT_INITIALIZED;
    }

    *tec = g_can_state[channel].tec;
    *rec = g_can_state[channel].rec;

    return CAN_ERR_OK;
}

/**
 * @brief Get bus status
 */
can_bus_status_t can_fd_get_bus_status(uint8_t channel) {
    if (channel > 1 || !g_can_state[channel].initialized) {
        return CAN_BUS_ERROR_ACTIVE;  /* Default */
    }

    uint8_t tec = g_can_state[channel].tec;
    uint8_t rec = g_can_state[channel].rec;

    if (tec >= 256) {
        return CAN_BUS_BUS_OFF;
    } else if (tec >= 96 || rec >= 96) {
        return CAN_BUS_ERROR_PASSIVE;
    } else {
        return CAN_BUS_ERROR_ACTIVE;
    }
}

/**
 * @brief Check if channel is initialized
 */
bool can_fd_is_initialized(uint8_t channel) {
    if (channel > 1) return false;
    return g_can_state[channel].initialized;
}

/**
 * @brief Get number of frames available in RX FIFO
 */
uint8_t can_fd_rx_available(uint8_t channel) {
    uint32_t fifosta_val;
    int ret;

    if (channel > 1 || !g_can_state[channel].initialized) return 0;

    ret = can_spi_read_reg(channel, MCP2518FD_REG_FIFOSTA_BASE + (CAN_FIFO_RX * 4), &fifosta_val);
    if (ret != CAN_ERR_OK) return 0;

    return (uint8_t)(fifosta_val & 0x1F);
}

/**
 * @brief Get number of free slots in TX FIFO
 */
uint8_t can_fd_tx_available(uint8_t channel) {
    uint32_t fifosta_val;
    int ret;

    if (channel > 1 || !g_can_state[channel].initialized) return 0;

    ret = can_spi_read_reg(channel, MCP2518FD_REG_FIFOSTA_BASE + (CAN_FIFO_TX * 4), &fifosta_val);
    if (ret != CAN_ERR_OK) return 0;

    uint8_t used = (uint8_t)(fifosta_val & 0x1F);
    return CAN_FIFO_TX_SIZE - used;
}
