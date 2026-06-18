/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * SWD Engine — ARM Serial Wire Debug bit-bang driver
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GHOSTBUS_SWD_DRIVER_H
#define GHOSTBUS_SWD_DRIVER_H

#include "registers.h"
#include "board.h"

/* SWD debug port register addresses */
#define SWD_DP_IDCODE     0x00
#define SWD_DP_ABORT      0x00
#define SWD_DP_CTRLSTAT   0x04
#define SWD_DP_SELECT     0x08
#define SWD_DP_RDBUFF     0x0C

#define SWD_AP_IDR        0x0FC
#define SWD_AP_BASE       0x0F8
#define SWD_AP_CSW        0x000
#define SWD_AP_TAR        0x004
#define SWD_AP_DRW        0x00C
#define SWD_AP_BD0        0x010
#define SWD_AP_BD1        0x014
#define SWD_AP_BD2        0x018
#define SWD_AP_BD3        0x01C

/* CTRL/STAT bits */
#define SWD_CDBGPWRUPREQ  (1U << 28)
#define SWD_CDBGPWRUPACK  (1U << 29)
#define SWD_CSYSPWRUPREQ  (1U << 30)
#define SWD_CSYSPWRUPACK  (1U << 31)
#define SWD_ORUNERRCLR    (1U << 4)
#define SWD_STICKYERRCLR  (1U << 5)

/* CSW bits */
#define SWD_CSW_DBGSWEN   (1U << 0)
#define SWD_CSW_SPIDEN    (1U << 3)
#define SWD_CSW_SIZE_8    (0x0U << 0)
#define SWD_CSW_SIZE_16   (0x1U << 2)
#define SWD_CSW_SIZE_32   (0x2U << 2)
#define SWD_CSW_ADDR_INC  (0x2U << 4)

/* Core debug registers (via AHB-AP memory access) */
#define SWD_DHCSR         0xE000EDF0UL
#define SWD_DCRSR         0xE000EDF4UL
#define SWD_DCRDR         0xE000EDF8UL
#define SWD_DEMCR         0xE000EDFCUL
#define SWD_AI          0xE000E000UL  /* arm-info */

#define SWD_DHCSR_DBGKEY  (0xA05FUL << 16)
#define SWD_DHCSR_C_HALT  (1U << 1)
#define SWD_DHCSR_C_STEP  (1U << 4)
#define SWD_DHCSR_C_RESUME (1U << 0)
#define SWD_DHCSR_S_HALT  (1U << 17)
#define SWD_DHCSR_S_REGRDY (1U << 16)

typedef enum {
    SWD_READ = 0,
    SWD_WRITE = 1
} swd_dir_t;

typedef struct {
    uint32_t idcode;
    uint32_t ap_count;
    uint32_t ap_idr[8];
    uint8_t  ap_type[8];
    uint8_t  rdp_level;
    uint32_t sram_base;
    uint32_t flash_base;
    uint8_t  exploit_available;
} swd_target_info_t;

/* Public API */
void          swd_init(probe_channel_t swdio_ch, probe_channel_t swclk_ch);
gb_status_t   swd_line_reset(void);
gb_status_t   swd_read_dp(uint8_t addr, uint32_t *val);
gb_status_t   swd_write_dp(uint8_t addr, uint32_t val);
gb_status_t   swd_read_ap(uint8_t apsel, uint8_t addr, uint32_t *val);
gb_status_t   swd_write_ap(uint8_t apsel, uint8_t addr, uint32_t val);
gb_status_t   swd_connect(swd_target_info_t *info);
gb_status_t   swd_mem_read32(uint32_t addr, uint32_t *buf, uint32_t count);
gb_status_t   swd_mem_write32(uint32_t addr, const uint32_t *buf, uint32_t count);
gb_status_t   swd_mem_read8(uint32_t addr, uint8_t *buf, uint32_t count);
gb_status_t   swd_flash_extract(uint32_t addr, uint8_t *buf, uint32_t len,
                                void (*progress)(uint32_t done, uint32_t total));
gb_status_t   swd_halt(void);
gb_status_t   swd_resume(void);
gb_status_t   swd_step(void);
gb_status_t   swd_read_core_reg(uint8_t reg_num, uint32_t *val);
gb_status_t   swd_write_core_reg(uint8_t reg_num, uint32_t val);
gb_status_t   swd_get_rdp_level(uint8_t *level);
void          swd_set_clock(uint32_t hz);
uint32_t      swd_get_clock(void);

#endif /* GHOSTBUS_SWD_DRIVER_H */