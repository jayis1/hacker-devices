/* OneWire Cartographer dual-port PHY API. Author: jayis1. MIT. */
#ifndef OWC_ONEWIRE_PHY_H
#define OWC_ONEWIRE_PHY_H
#include "../board.h"

typedef enum { OWC_PORT_UPSTREAM = 0, OWC_PORT_DOWNSTREAM = 1 } owc_port_t;
typedef enum { OWC_EDGE_FALLING = 0, OWC_EDGE_RISING = 1 } owc_edge_kind_t;

typedef struct {
    uint32_t timestamp_us;
    uint16_t voltage_mv;
    uint8_t port;
    uint8_t kind;
} owc_edge_t;

typedef struct {
    owc_edge_t edges[OWC_CAPTURE_DEPTH];
    volatile uint16_t write_index;
    volatile uint16_t read_index;
    uint32_t dropped;
    uint16_t low_threshold_mv;
    uint16_t high_threshold_mv;
    bool capturing;
    bool faulted;
} owc_phy_t;

void owc_phy_init(owc_phy_t *phy);
void owc_phy_start(owc_phy_t *phy);
void owc_phy_stop(owc_phy_t *phy);
void owc_phy_record_edge(owc_phy_t *phy, owc_port_t port, owc_edge_kind_t kind, uint16_t mv, uint32_t timestamp);
bool owc_phy_next_edge(owc_phy_t *phy, owc_edge_t *edge);
bool owc_phy_set_thresholds(owc_phy_t *phy, uint16_t low_mv, uint16_t high_mv);
bool owc_phy_bus_safe(owc_port_t port);
bool owc_phy_drive_reset(owc_phy_t *phy, owc_port_t port);
bool owc_phy_write_bit(owc_phy_t *phy, owc_port_t port, bool bit);
bool owc_phy_write_byte(owc_phy_t *phy, owc_port_t port, uint8_t value);
void owc_phy_release_all(void);
uint8_t owc_crc8(const uint8_t *data, size_t length);

#endif
