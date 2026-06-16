/**
 * @file can_fd_driver.h
 * @brief MCP2518FD CAN FD Controller Driver API
 *
 * Provides SPI-based interface to Microchip MCP2518FD CAN FD controller.
 * Supports dual-channel operation with configurable bit timing,
 * FIFO management, acceptance filtering, and error monitoring.
 *
 * Datasheet: MCP2518FD Data Sheet DS20006027B
 */

#ifndef CAN_FD_DRIVER_H
#define CAN_FD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * CAN FRAME STRUCTURES
 *===========================================================================*/

/** CAN frame flags */
#define CAN_FLAG_IDE    0x01  /* Extended ID (29-bit) */
#define CAN_FLAG_RTR    0x02  /* Remote Transmission Request */
#define CAN_FLAG_FD     0x04  /* CAN FD frame */
#define CAN_FLAG_BRS    0x08  /* Bit Rate Switch */
#define CAN_FLAG_ESI    0x10  /* Error State Indicator */

/** CAN frame representation */
typedef struct {
    uint32_t id;            /* CAN ID (11-bit or 29-bit, in bits 28:0) */
    uint8_t  dlc;           /* Data Length Code (0-15 for CAN FD, 0-8 for classical) */
    uint8_t  data[64];      /* Payload data (up to 64 bytes for CAN FD) */
    uint32_t timestamp;     /* Hardware timestamp in microseconds */
    uint8_t  flags;         /* IDE, RTR, FD, BRS, ESI flags */
} can_frame_t;

/*===========================================================================
 * CAN CONFIGURATION
 *===========================================================================*/

/** CAN FD controller configuration */
typedef struct {
    uint8_t  channel;          /* CAN channel (0 or 1) */
    uint32_t nominal_brp;      /* Nominal Baud Rate Prescaler (0-255) */
    uint32_t nominal_tseg1;    /* Nominal Time Segment 1 (PropSeg + PhaseSeg1 - 1) */
    uint32_t nominal_tseg2;    /* Nominal Time Segment 2 (PhaseSeg2 - 1) */
    uint32_t nominal_sjw;      /* Nominal Synchronization Jump Width (SJW - 1) */
    uint32_t data_brp;         /* Data Baud Rate Prescaler (0-255) */
    uint32_t data_tseg1;       /* Data Time Segment 1 */
    uint32_t data_tseg2;       /* Data Time Segment 2 */
    uint32_t data_sjw;         /* Data Synchronization Jump Width */
    bool     fd_enable;        /* Enable CAN FD mode */
    bool     brs_enable;       /* Enable Bit Rate Switch */
    bool     listen_only;      /* Passive listen-only mode (no ACK, no error flags) */
} can_fd_config_t;

/*===========================================================================
 * CAN MODES
 *===========================================================================*/

typedef enum {
    CAN_MODE_NORMAL       = 0,  /* Normal operation (ACK, error frames, arbitration) */
    CAN_MODE_LISTEN_ONLY  = 1,  /* Passive monitor (no ACK, no error flags) */
    CAN_MODE_LOOPBACK     = 2,  /* Internal loopback (TX → RX internally) */
    CAN_MODE_BUS_MONITOR  = 3,  /* Bus monitor (no ACK, no error flags, BRS disabled) */
    CAN_MODE_SLEEP        = 4,  /* Low-power sleep */
    CAN_MODE_STANDBY      = 5   /* Standby (wake on bus activity) */
} can_mode_t;

/*===========================================================================
 * BUS STATUS
 *===========================================================================*/

typedef enum {
    CAN_BUS_ERROR_ACTIVE   = 0,  /* TEC < 96 and REC < 96 */
    CAN_BUS_ERROR_PASSIVE  = 1,  /* TEC >= 96 or REC >= 96 */
    CAN_BUS_BUS_OFF        = 2   /* TEC >= 256 */
} can_bus_status_t;

/*===========================================================================
 * ERROR CODES
 *===========================================================================*/

#define CAN_ERR_OK                0
#define CAN_ERR_TIMEOUT          -1
#define CAN_ERR_SPI              -2
#define CAN_ERR_CRC              -3
#define CAN_ERR_BUS_OFF          -4
#define CAN_ERR_TX_FIFO_FULL     -5
#define CAN_ERR_RX_FIFO_EMPTY    -6
#define CAN_ERR_INVALID_PARAM    -7
#define CAN_ERR_NOT_INITIALIZED  -8
#define CAN_ERR_INIT_FAILED      -9

/*===========================================================================
 * MCP2518FD REGISTER ADDRESSES (SPI Address Space)
 *===========================================================================*/

/* Configuration registers */
#define MCP2518FD_REG_CON         0x000
#define MCP2518FD_REG_NBTCFG      0x004
#define MCP2518FD_REG_DBTCFG      0x008
#define MCP2518FD_REG_TDC         0x00C
#define MCP2518FD_REG_TBC         0x010
#define MCP2518FD_REG_TSCON       0x014
#define MCP2518FD_REG_VEC         0x018
#define MCP2518FD_REG_INT         0x01C
#define MCP2518FD_REG_RXIF        0x020
#define MCP2518FD_REG_TXIF        0x024
#define MCP2518FD_REG_RXOVIF      0x028
#define MCP2518FD_REG_TXATIF      0x02C
#define MCP2518FD_REG_TXREQ       0x030
#define MCP2518FD_REG_TREC        0x034
#define MCP2518FD_REG_BDIA        0x038
#define MCP2518FD_REG_TEFCON      0x040
#define MCP2518FD_REG_TEFSTA      0x044
#define MCP2518FD_REG_TEFUA       0x048
#define MCP2518FD_REG_TXQCON      0x050
#define MCP2518FD_REG_TXQSTA      0x054
#define MCP2518FD_REG_TXQUA       0x058
#define MCP2518FD_REG_FIFOCON_BASE  0x060  /* FIFOCON[1..31] */
#define MCP2518FD_REG_FIFOSTA_BASE  0x0A0  /* FIFOSTA[1..31] */
#define MCP2518FD_REG_FIFOUA_BASE   0x0E0  /* FIFOUA[1..31] */
#define MCP2518FD_REG_FLTCON_BASE   0x200  /* FLTCON[0..31] */
#define MCP2518FD_REG_FLTOBJ_BASE   0x280  /* FLTOBJ[0..31] */
#define MCP2518FD_REG_MASK_BASE     0x300  /* MASK[0..31] */
#define MCP2518FD_REG_OSC           0xE00
#define MCP2518FD_REG_IOCON         0xE08
#define MCP2518FD_REG_CANCTRL       0xE0C
#define MCP2518FD_REG_CRC           0xE10

/* SPI instruction opcodes */
#define MCP2518FD_INSTR_RESET       0x00
#define MCP2518FD_INSTR_READ        0x03
#define MCP2518FD_INSTR_WRITE       0x02
#define MCP2518FD_INSTR_READ_CRC    0x0B
#define MCP2518FD_INSTR_WRITE_CRC   0x0A
#define MCP2518FD_INSTR_WRITE_SAFE  0x0C

/* CON register bit fields */
#define CON_REQOP_NORMAL            0x00
#define CON_REQOP_LISTEN_ONLY       0x01
#define CON_REQOP_LOOPBACK          0x02
#define CON_REQOP_BUS_MONITOR       0x03
#define CON_REQOP_SLEEP             0x04
#define CON_ABAT                    (1UL << 4)   /* Abort All Transmissions */
#define CON_TXQEN                   (1UL << 5)   /* TX Queue Enable */
#define CON_WAKFIL                  (1UL << 6)   /* Wake-up Filter Enable */
#define CON_ESIGM                   (1UL << 7)   /* ESI Gateway Mode */
#define CON_STEF                    (1UL << 8)   /* Store in TEF */
#define CON_ISOCRCEN                (1UL << 9)   /* ISO CRC Enable (CAN FD) */
#define CON_PXEDIS                  (1UL << 10)  /* Protocol Exception Disable */
#define CON_BRSDIS                  (1UL << 11)  /* BRS Disable */
#define CON_RTXAT                   (1UL << 12)  /* Restrict Retransmission Attempts */
#define CON_DNCNT_SHIFT             16
#define CON_DNCNT_MASK              (0x1FUL << CON_DNCNT_SHIFT)

/* FIFOCON register bit fields */
#define FIFOCON_FSIZE_SHIFT         0
#define FIFOCON_FSIZE_MASK          (0x1FUL << FIFOCON_FSIZE_SHIFT)
#define FIFOCON_TXEN                (1UL << 7)   /* TX Enable */
#define FIFOCON_RXEN                (1UL << 8)   /* RX Enable */
#define FIFOCON_RTSOV               (1UL << 9)   /* RTS Overflow */
#define FIFOCON_TXPRI_SHIFT         10
#define FIFOCON_TXPRI_MASK          (0x3UL << FIFOCON_TXPRI_SHIFT)
#define FIFOCON_PLSIZE_SHIFT        12
#define FIFOCON_PLSIZE_MASK         (0x7UL << FIFOCON_PLSIZE_SHIFT)
#define FIFOCON_FRESET              (1UL << 15)  /* FIFO Reset */
#define FIFOCON_TEFTSEN             (1UL << 16)  /* TEF Timestamp Enable */

/* CANCTRL register bit fields */
#define CANCTRL_REQOP_SHIFT         0
#define CANCTRL_REQOP_MASK          (0x7UL << CANCTRL_REQOP_SHIFT)
#define CANCTRL_OPMOD_SHIFT         4
#define CANCTRL_OPMOD_MASK          (0x7UL << CANCTRL_OPMOD_SHIFT)

/*===========================================================================
 * BIT TIMING PRESETS
 *===========================================================================*/

/* 500 kbps nominal (40 MHz clock, TQ=25ns, 80 TQ per bit) */
#define NBTCFG_500KBPS_BRP          0
#define NBTCFG_500KBPS_TSEG1        55
#define NBTCFG_500KBPS_TSEG2        23
#define NBTCFG_500KBPS_SJW          7

/* 250 kbps nominal */
#define NBTCFG_250KBPS_BRP          0
#define NBTCFG_250KBPS_TSEG1        119
#define NBTCFG_250KBPS_TSEG2        39
#define NBTCFG_250KBPS_SJW          15

/* 1 Mbps nominal */
#define NBTCFG_1MBPS_BRP            0
#define NBTCFG_1MBPS_TSEG1          23
#define NBTCFG_1MBPS_TSEG2          15
#define NBTCFG_1MBPS_SJW            3

/* 4 Mbps data (CAN FD, 10 TQ per bit) */
#define DBTCFG_4MBPS_BRP            0
#define DBTCFG_4MBPS_TSEG1          5
#define DBTCFG_4MBPS_TSEG2          2
#define DBTCFG_4MBPS_SJW            1

/* 8 Mbps data (CAN FD, 5 TQ per bit) */
#define DBTCFG_8MBPS_BRP            0
#define DBTCFG_8MBPS_TSEG1          2
#define DBTCFG_8MBPS_TSEG2          1
#define DBTCFG_8MBPS_SJW            0

/*===========================================================================
 * DRIVER API
 *===========================================================================*/

/**
 * @brief Initialize a CAN FD channel
 *
 * Resets the MCP2518FD, configures bit timing, sets up FIFOs,
 * and puts the controller in the requested mode.
 *
 * @param channel CAN channel (0 or 1)
 * @param config  Pointer to configuration structure
 * @return CAN_ERR_OK on success, negative error code on failure
 */
int can_fd_init(uint8_t channel, const can_fd_config_t *config);

/**
 * @brief Deinitialize a CAN FD channel
 *
 * Puts the controller in sleep mode and releases SPI resources.
 *
 * @param channel CAN channel (0 or 1)
 * @return CAN_ERR_OK on success
 */
int can_fd_deinit(uint8_t channel);

/**
 * @brief Transmit a CAN frame
 *
 * Writes the frame to the TX FIFO. The controller handles arbitration
 * and automatic retransmission on loss.
 *
 * @param channel CAN channel (0 or 1)
 * @param frame   Pointer to frame to transmit
 * @return CAN_ERR_OK on success, negative error on failure
 */
int can_fd_transmit(uint8_t channel, const can_frame_t *frame);

/**
 * @brief Receive a CAN frame (non-blocking)
 *
 * Reads the next available frame from the RX FIFO.
 * Returns immediately if no frame is available.
 *
 * @param channel CAN channel (0 or 1)
 * @param frame   Pointer to store received frame
 * @return CAN_ERR_OK if frame received, CAN_ERR_RX_FIFO_EMPTY if none available
 */
int can_fd_receive(uint8_t channel, can_frame_t *frame);

/**
 * @brief Set CAN controller operating mode
 *
 * @param channel CAN channel (0 or 1)
 * @param mode    Desired operating mode
 * @return CAN_ERR_OK on success
 */
int can_fd_set_mode(uint8_t channel, can_mode_t mode);

/**
 * @brief Configure an acceptance filter
 *
 * Sets up a filter to accept only frames matching the given mask and ID.
 *
 * @param channel    CAN channel (0 or 1)
 * @param filter_num Filter number (0-31)
 * @param mask       Acceptance mask (bits: 0=match, 1=don't care)
 * @param id         Acceptance ID to match against
 * @return CAN_ERR_OK on success
 */
int can_fd_set_filter(uint8_t channel, uint8_t filter_num, uint32_t mask, uint32_t id);

/**
 * @brief Get transmit and receive error counters
 *
 * @param channel CAN channel (0 or 1)
 * @param tec     Pointer to store Transmit Error Counter
 * @param rec     Pointer to store Receive Error Counter
 * @return CAN_ERR_OK on success
 */
int can_fd_get_error_counters(uint8_t channel, uint8_t *tec, uint8_t *rec);

/**
 * @brief Get current bus status
 *
 * @param channel CAN channel (0 or 1)
 * @return Current bus status
 */
can_bus_status_t can_fd_get_bus_status(uint8_t channel);

/**
 * @brief Check if a channel is initialized
 *
 * @param channel CAN channel (0 or 1)
 * @return true if initialized, false otherwise
 */
bool can_fd_is_initialized(uint8_t channel);

/**
 * @brief Get the number of frames available in RX FIFO
 *
 * @param channel CAN channel (0 or 1)
 * @return Number of frames available (0 if none)
 */
uint8_t can_fd_rx_available(uint8_t channel);

/**
 * @brief Get the number of free slots in TX FIFO
 *
 * @param channel CAN channel (0 or 1)
 * @return Number of free TX slots
 */
uint8_t can_fd_tx_available(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* CAN_FD_DRIVER_H */
