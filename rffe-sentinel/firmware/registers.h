/* RFFE Sentinel STM32G474 and FPGA register definitions
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef RFFE_SENTINEL_REGISTERS_H
#define RFFE_SENTINEL_REGISTERS_H

#include <stdint.h>

#define RS_BIT(n) (1u << (n))
#define RS_FPGA_ID_EXPECTED       0x52464645u
#define RS_FPGA_REG_ID            0x00u
#define RS_FPGA_REG_CONTROL       0x04u
#define RS_FPGA_REG_STATUS        0x08u
#define RS_FPGA_REG_FIFO_LEVEL    0x0cu
#define RS_FPGA_REG_EVENT_DATA    0x10u
#define RS_FPGA_REG_RULE_INDEX    0x20u
#define RS_FPGA_REG_RULE_MATCH    0x24u
#define RS_FPGA_REG_RULE_VALUE    0x28u
#define RS_FPGA_REG_RULE_ACTION   0x2cu
#define RS_FPGA_REG_DELAY_NS      0x30u
#define RS_FPGA_REG_WATCHDOG      0x34u

#define RS_FPGA_CTL_CAPTURE       RS_BIT(0)
#define RS_FPGA_CTL_FORWARD       RS_BIT(1)
#define RS_FPGA_CTL_INTERVENE     RS_BIT(2)
#define RS_FPGA_CTL_FIFO_RESET    RS_BIT(3)
#define RS_FPGA_CTL_RULE_COMMIT   RS_BIT(4)

#define RS_FPGA_ST_PLL_LOCK       RS_BIT(0)
#define RS_FPGA_ST_BYPASS_SENSE   RS_BIT(1)
#define RS_FPGA_ST_FIFO_OVERFLOW  RS_BIT(2)
#define RS_FPGA_ST_CONTENTION     RS_BIT(3)
#define RS_FPGA_ST_WATCHDOG       RS_BIT(4)
#define RS_FPGA_ST_VIO_VALID      RS_BIT(5)

#define RS_RFFE_FLAG_PARITY_OK    RS_BIT(0)
#define RS_RFFE_FLAG_BUS_PARK_OK  RS_BIT(1)
#define RS_RFFE_FLAG_CONTENTION   RS_BIT(2)
#define RS_RFFE_FLAG_TRUNCATED    RS_BIT(3)
#define RS_RFFE_FLAG_READ         RS_BIT(4)
#define RS_RFFE_FLAG_EXTENDED     RS_BIT(5)

#define RS_FLASH_PAGE_SIZE        256u
#define RS_FLASH_SECTOR_SIZE      4096u
#define RS_FLASH_EVENT_BASE       0x001000u
#define RS_FLASH_EVENT_LIMIT      0xF00000u
#define RS_FLASH_POLICY_BASE      0x000000u
#define RS_FLASH_MANIFEST_BASE    0x000800u

struct rs_mmio {
    uint32_t id;
    uint32_t control;
    uint32_t status;
    uint32_t fifo_level;
    uint32_t event_data;
    uint32_t rule_index;
    uint32_t rule_match;
    uint32_t rule_value;
    uint32_t rule_action;
    uint32_t delay_ns;
    uint32_t watchdog;
};

uint32_t fpga_reg_read(uint32_t offset);
void fpga_reg_write(uint32_t offset, uint32_t value);
void fpga_sim_set_status(uint32_t value);

#endif
