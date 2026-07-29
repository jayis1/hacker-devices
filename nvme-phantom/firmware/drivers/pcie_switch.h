/*
 * drivers/pcie_switch.h — Broadcom PEX8606 PCIe switch driver header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_PCIE_SWITCH_H
#define NVME_PHANTOM_PCIE_SWITCH_H

#include "../board.h"

int pcie_switch_init(void);
int pcie_switch_enable_modify(int enable);
int pcie_switch_inject_hotplug(uint8_t event);
link_state_t pcie_switch_link_state(void);
uint8_t pcie_switch_link_width(void);
void pcie_switch_safe_mode(void);

int pex_write(uint16_t reg, uint8_t val);
int pex_write32(uint16_t reg, uint32_t val);
int pex_read32(uint16_t reg, uint32_t *out);

#endif /* NVME_PHANTOM_PCIE_SWITCH_H */