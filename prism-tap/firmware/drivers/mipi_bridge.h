/*
 * drivers/mipi_bridge.h — MIPI Bridge Chip Control for Prism-Tap
 *
 * I2C configuration of Toshiba TC358746 (CSI-2 Rx), TC358748 (CSI-2 Tx),
 * Analog Devices ADV7480 (DSI Rx), and Toshiba TC358762 (DSI Tx).
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_MIPI_BRIDGE_H
#define PRISM_TAP_MIPI_BRIDGE_H

#include <stdint.h>
#include "board.h"

/* ---- Bridge chip identifiers ---- */
typedef enum {
    BRIDGE_CSI_RX = 0,    /* TC358746 — CSI-2 input from camera/sensor  */
    BRIDGE_CSI_TX = 1,    /* TC358748 — CSI-2 output to AP              */
    BRIDGE_DSI_RX = 2,    /* ADV7480  — DSI input from AP              */
    BRIDGE_DSI_TX = 3,    /* TC358762 — DSI output to display panel     */
} bridge_id_t;

/* ---- CSI-2 configuration ---- */
typedef struct {
    uint8_t   num_lanes;       /* 1 or 2 lanes                              */
    uint32_t  lane_speed_mbps; /* per-lane data rate in Mbps                */
    uint16_t  width;           /* active width in pixels                    */
    uint16_t  height;          /* active height in pixels                   */
    uint8_t   pixel_format;    /* frame_format_t value                      */
    uint32_t  pixel_clock_hz;  /* parallel bus pixel clock in Hz            */
} csi_config_t;

/* ---- DSI configuration ---- */
typedef struct {
    uint8_t   num_lanes;       /* 1 or 2 lanes                              */
    uint32_t  lane_speed_mbps;
    uint16_t  width;
    uint16_t  height;
    uint8_t   pixel_format;    /* frame_format_t value                      */
    uint8_t   video_mode;      /* 0=command mode, 1=video mode (burst/non-burst) */
    uint8_t   dcs_enabled;     /* 1 = DCS commands supported               */
} dsi_config_t;

/* ---- I2C low-level ---- */
int  bridge_i2c_init(void);
int  bridge_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len);
int  bridge_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len);

/* ---- Bridge initialization ---- */
int  bridge_init_csi_rx(const csi_config_t *cfg);
int  bridge_init_csi_tx(const csi_config_t *cfg);
int  bridge_init_dsi_rx(const dsi_config_t *cfg);
int  bridge_init_dsi_tx(const dsi_config_t *cfg);

/* ---- Bridge status ---- */
uint8_t bridge_csi_rx_link_up(void);
uint8_t bridge_csi_tx_link_up(void);
uint8_t bridge_dsi_rx_link_up(void);
uint8_t bridge_dsi_tx_link_up(void);

/* ---- Bridge power control ---- */
void bridge_power_down(bridge_id_t id);
void bridge_power_up(bridge_id_t id);

/* ---- Auto-detect (try common configurations) ---- */
int  bridge_auto_detect_csi(csi_config_t *cfg);
int  bridge_auto_detect_dsi(dsi_config_t *cfg);

/* ---- Diagnostic ---- */
void bridge_dump_regs(bridge_id_t id);

#endif /* PRISM_TAP_MIPI_BRIDGE_H */

/* ---- End of mipi_bridge.h ----
 * Author: jayis1
 */