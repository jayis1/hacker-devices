/**
 * drivers/tlp_injector.c — TLP Injector Driver Implementation
 *
 * Complete driver for the PCIe Screamer active TLP injection engine.
 * Allows crafting and injecting arbitrary PCIe Transaction Layer Packets
 * onto the host-facing PCIe link for DMA attack simulation and
 * security research.
 *
 * The injector operates through the PCIe Hard IP TX FIFO, building
 * properly formatted TLPs with correct Fmt/Type encoding, requester ID,
 * tag, address, and data payload. For non-posted requests (MRd, CfgRd),
 * the injector waits for and captures the completion TLP.
 *
 * Security: Injection is gated by the INJECT_ENABLE and INJECT_ARMED
 * control bits. Both must be set before a TLP can be injected.
 *
 * Author: jayis1
 * Date: 2026-06-16
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"
#include "pcie.h"
#include "tlp_injector.h"

/*============================================================================
 * INTERNAL CONSTANTS
 *============================================================================*/

#define INJECT_TIMEOUT_US        100000UL  /* 100 ms for completion */
#define INJECT_MAX_DATA_DW       1024      /* Max payload in DWORDs */
#define INJECT_COMPLETION_BUF_SIZE 4096    /* Completion data buffer */

/*============================================================================
 * INTERNAL STATE
 *============================================================================*/

static uint32_t g_inject_count = 0;
static uint32_t g_inject_error_count = 0;
static uint8_t g_completion_buf[INJECT_COMPLETION_BUF_SIZE];

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int tlp_injector_init(void) {
    /* Reset injector registers */
    REG_WRITE(INJECTOR_CTRL_REG, 0);
    REG_WRITE(INJECTOR_STATUS_REG, 0);
    REG_WRITE(INJECTOR_ADDR_LO_REG, 0);
    REG_WRITE(INJECTOR_ADDR_HI_REG, 0);
    REG_WRITE(INJECTOR_TLP_TYPE_REG, 0);
    REG_WRITE(INJECTOR_LENGTH_REG, 0);
    REG_WRITE(INJECTOR_TAG_REG, 0);
    REG_WRITE(INJECTOR_REQID_REG, 0);
    REG_WRITE(INJECTOR_RESULT_REG, 0);
    REG_WRITE(INJECTOR_COUNT_REG, 0);
    REG_WRITE(INJECTOR_ERROR_COUNT_REG, 0);

    g_inject_count = 0;
    g_inject_error_count = 0;

    return 0;
}

/*============================================================================
 * STATUS
 *============================================================================*/

int tlp_injector_is_busy(void) {
    return (REG_READ(INJECTOR_STATUS_REG) & 0x1) ? 1 : 0;
}

uint32_t tlp_injector_get_count(void) {
    return REG_READ(INJECTOR_COUNT_REG);
}

uint32_t tlp_injector_get_error_count(void) {
    return REG_READ(INJECTOR_ERROR_COUNT_REG);
}

/*============================================================================
 * MEMORY READ INJECTION (DMA Read)
 *============================================================================*/

int tlp_injector_inject_mrd(uint16_t requester_id, uint8_t tag,
                            uint64_t address, uint16_t length_dw,
                            uint8_t *completion_data, uint16_t *cpl_len) {
    int ret;
    uint32_t result;

    /* Validate parameters */
    if (length_dw == 0 || length_dw > 1024) {
        return -1;
    }
    if (!completion_data || !cpl_len) {
        return -1;
    }

    /* Check if injector is busy */
    if (tlp_injector_is_busy()) {
        return -2;  /* Busy */
    }

    /* Configure injector */
    REG_WRITE(INJECTOR_TLP_TYPE_REG, INJECT_MRD);
    REG_WRITE(INJECTOR_ADDR_LO_REG, (uint32_t)(address & 0xFFFFFFFF));
    REG_WRITE(INJECTOR_ADDR_HI_REG, (uint32_t)(address >> 32));
    REG_WRITE(INJECTOR_LENGTH_REG, length_dw);
    REG_WRITE(INJECTOR_TAG_REG, tag);
    REG_WRITE(INJECTOR_REQID_REG, requester_id);

    /* Trigger injection */
    REG_WRITE(INJECTOR_TRIGGER_REG, 1);

    /* Wait for completion */
    ret = REG_POLL_UNTIL(INJECTOR_STATUS_REG, 0x1, 0x0, INJECT_TIMEOUT_US);
    if (ret != 0) {
        g_inject_error_count++;
        REG_WRITE(INJECTOR_ERROR_COUNT_REG, g_inject_error_count);
        return -3;  /* Timeout */
    }

    /* Read result */
    result = REG_READ(INJECTOR_RESULT_REG);

    if (result == INJECT_OK) {
        /* Read completion data from injector completion buffer */
        uint32_t cpl_dw_count = REG_READ(INJECTOR_LENGTH_REG);  /* Completion length */
        *cpl_len = cpl_dw_count * 4;

        if (*cpl_len > INJECT_COMPLETION_BUF_SIZE) {
            *cpl_len = INJECT_COMPLETION_BUF_SIZE;
        }

        for (uint32_t i = 0; i < cpl_dw_count; i++) {
            uint32_t dw = REG_READ(INJECTOR_DATA_FIFO_REG);
            memcpy(completion_data + i * 4, &dw, 4);
        }

        g_inject_count++;
        REG_WRITE(INJECTOR_COUNT_REG, g_inject_count);
        return 0;
    } else {
        g_inject_error_count++;
        REG_WRITE(INJECTOR_ERROR_COUNT_REG, g_inject_error_count);
        *cpl_len = 0;
        return result;  /* Return error code (UR, CA, CRS) */
    }
}

/*============================================================================
 * MEMORY WRITE INJECTION (DMA Write)
 *============================================================================*/

int tlp_injector_inject_mwr(uint16_t requester_id, uint64_t address,
                            const uint8_t *data, uint16_t length_dw) {
    int ret;

    /* Validate */
    if (length_dw == 0 || length_dw > INJECT_MAX_DATA_DW) {
        return -1;
    }
    if (!data && length_dw > 0) {
        return -1;
    }

    /* Check if busy */
    if (tlp_injector_is_busy()) {
        return -2;
    }

    /* Configure injector */
    REG_WRITE(INJECTOR_TLP_TYPE_REG, INJECT_MWR);
    REG_WRITE(INJECTOR_ADDR_LO_REG, (uint32_t)(address & 0xFFFFFFFF));
    REG_WRITE(INJECTOR_ADDR_HI_REG, (uint32_t)(address >> 32));
    REG_WRITE(INJECTOR_LENGTH_REG, length_dw);
    REG_WRITE(INJECTOR_REQID_REG, requester_id);

    /* Write data payload to injector FIFO */
    for (uint16_t i = 0; i < length_dw; i++) {
        uint32_t dw;
        memcpy(&dw, data + i * 4, 4);
        REG_WRITE(INJECTOR_DATA_FIFO_REG, dw);
    }

    /* Trigger */
    REG_WRITE(INJECTOR_TRIGGER_REG, 1);

    /* MWr is a posted transaction — no completion expected.
     * Just wait for injector to finish transmitting. */
    ret = REG_POLL_UNTIL(INJECTOR_STATUS_REG, 0x1, 0x0, INJECT_TIMEOUT_US);
    if (ret != 0) {
        g_inject_error_count++;
        REG_WRITE(INJECTOR_ERROR_COUNT_REG, g_inject_error_count);
        return -3;
    }

    g_inject_count++;
    REG_WRITE(INJECTOR_COUNT_REG, g_inject_count);

    return 0;
}

/*============================================================================
 * CONFIGURATION READ INJECTION
 *============================================================================*/

int tlp_injector_inject_cfgrd(uint8_t bus, uint8_t device, uint8_t function,
                              uint16_t reg_offset, uint32_t *value) {
    int ret;
    uint32_t result;
    uint16_t requester_id;
    uint64_t config_address;

    if (!value) return -1;
    if (bus > 255 || device > 31 || function > 7) return -1;
    if (reg_offset > 0xFF || (reg_offset & 0x3)) return -1;

    /* Build configuration address
     * For Type 0: address[31:2] = {0, reg[7:2], function[2:0], device[4:0], bus[7:0], 0, 0}
     * For Type 1: address[31:2] = {0, reg[7:2], function[2:0], device[4:0], bus[7:0], 0, 1}
     *
     * Simplified: we use the requester ID to encode bus/device/function
     * and the address for the register number. */
    requester_id = ((uint16_t)bus << 8) | ((uint16_t)device << 3) | function;
    config_address = (uint64_t)(reg_offset & 0xFC);

    /* Check if busy */
    if (tlp_injector_is_busy()) {
        return -2;
    }

    /* Configure injector for Type 0 Configuration Read */
    REG_WRITE(INJECTOR_TLP_TYPE_REG, INJECT_CFGRD0);
    REG_WRITE(INJECTOR_ADDR_LO_REG, (uint32_t)(config_address & 0xFFFFFFFF));
    REG_WRITE(INJECTOR_ADDR_HI_REG, 0);
    REG_WRITE(INJECTOR_LENGTH_REG, 1);  /* 1 DW */
    REG_WRITE(INJECTOR_TAG_REG, 0);
    REG_WRITE(INJECTOR_REQID_REG, requester_id);

    /* Trigger */
    REG_WRITE(INJECTOR_TRIGGER_REG, 1);

    /* Wait for completion */
    ret = REG_POLL_UNTIL(INJECTOR_STATUS_REG, 0x1, 0x0, INJECT_TIMEOUT_US);
    if (ret != 0) {
        g_inject_error_count++;
        REG_WRITE(INJECTOR_ERROR_COUNT_REG, g_inject_error_count);
        return -3;
    }

    result = REG_READ(INJECTOR_RESULT_REG);

    if (result == INJECT_OK) {
        /* Read completion data (1 DW) */
        *value = REG_READ(INJECTOR_DATA_FIFO_REG);
        g_inject_count++;
        REG_WRITE(INJECTOR_COUNT_REG, g_inject_count);
        return 0;
    } else {
        g_inject_error_count++;
        REG_WRITE(INJECTOR_ERROR_COUNT_REG, g_inject_error_count);
        *value = 0xFFFFFFFF;
        return result;
    }
}

/*============================================================================
 * CONFIGURATION WRITE INJECTION
 *============================================================================*/

int tlp_injector_inject_cfgwr(uint8_t bus, uint8_t device, uint8_t function,
                              uint16_t reg_offset, uint32_t value) {
    int ret;
    uint16_t requester_id;
    uint64_t config_address;

    if (bus > 255 || device > 31 || function > 7) return -1;
    if (reg_offset > 0xFF || (reg_offset & 0x3)) return -1;

    requester_id = ((uint16_t)bus << 8) | ((uint16_t)device << 3) | function;
    config_address = (uint64_t)(reg_offset & 0xFC);

    /* Check if busy */
    if (tlp_injector_is_busy()) {
        return -2;
    }

    /* Configure injector for Type 0 Configuration Write */
    REG_WRITE(INJECTOR_TLP_TYPE_REG, INJECT_CFGWR0);
    REG_WRITE(INJECTOR_ADDR_LO_REG, (uint32_t)(config_address & 0xFFFFFFFF));
    REG_WRITE(INJECTOR_ADDR_HI_REG, 0);
    REG_WRITE(INJECTOR_LENGTH_REG, 1);
    REG_WRITE(INJECTOR_REQID_REG, requester_id);

    /* Write data */
    REG_WRITE(INJECTOR_DATA_FIFO_REG, value);

    /* Trigger */
    REG_WRITE(INJECTOR_TRIGGER_REG, 1);

    /* CfgWr is non-posted — wait for completion */
    ret = REG_POLL_UNTIL(INJECTOR_STATUS_REG, 0x1, 0x0, INJECT_TIMEOUT_US);
    if (ret != 0) {
        g_inject_error_count++;
        REG_WRITE(INJECTOR_ERROR_COUNT_REG, g_inject_error_count);
        return -3;
    }

    uint32_t result = REG_READ(INJECTOR_RESULT_REG);

    if (result == INJECT_OK) {
        g_inject_count++;
        REG_WRITE(INJECTOR_COUNT_REG, g_inject_count);
        return 0;
    } else {
        g_inject_error_count++;
        REG_WRITE(INJECTOR_ERROR_COUNT_REG, g_inject_error_count);
        return result;
    }
}
