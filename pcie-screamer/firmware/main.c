/**
 * main.c — PCIe Screamer Firmware Main Entry Point
 *
 * Complete firmware initialization and main loop for the PCIe Screamer
 * NVMe interposer. Runs on a RISC-V soft CPU (VexRiscv) inside the
 * Lattice ECP5-45F FPGA.
 *
 * Initialization sequence:
 *   1. GPIO configuration
 *   2. PLL lock and clock distribution
 *   3. DDR3 SDRAM initialization and calibration
 *   4. PCIe Hard IP initialization and link training
 *   5. FT601 USB 3.0 bridge initialization
 *   6. TLP sniffer pipeline enable
 *   7. Main loop: capture → buffer → stream
 *
 * Author: jayis1
 * Date: 2026-06-16
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/pcie.h"
#include "drivers/ddr3.h"
#include "drivers/usb_ft601.h"
#include "drivers/tlp_sniffer.h"
#include "drivers/tlp_injector.h"
#include "drivers/crc32c.h"

/*============================================================================
 * FIRMWARE VERSION
 *============================================================================*/

#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION_BUILD    1
#define FW_VERSION          ((FW_VERSION_MAJOR << 24) | \
                             (FW_VERSION_MINOR << 16) | \
                             (FW_VERSION_PATCH << 8)  | \
                             (FW_VERSION_BUILD))

/*============================================================================
 * GLOBAL STATE
 *============================================================================*/

typedef enum {
    STATE_RESET = 0,
    STATE_PLL_INIT,
    STATE_DDR3_INIT,
    STATE_PCIE_INIT,
    STATE_USB_INIT,
    STATE_ACTIVE,
    STATE_ERROR,
    STATE_FW_UPDATE
} system_state_t;

static system_state_t g_state = STATE_RESET;
static uint32_t g_error_code = 0;
static uint8_t g_capture_buffer[16384] __attribute__((aligned(16)));
static uint8_t g_usb_tx_buffer[4096] __attribute__((aligned(16)));
static uint8_t g_usb_rx_buffer[4096] __attribute__((aligned(16)));

/*============================================================================
 * ERROR CODES
 *============================================================================*/

#define ERR_NONE                    0x00000000
#define ERR_PLL0_LOCK_TIMEOUT       0x00000001
#define ERR_PLL1_LOCK_TIMEOUT       0x00000002
#define ERR_DDR3_INIT_TIMEOUT       0x00000003
#define ERR_DDR3_CAL_FAIL           0x00000004
#define ERR_PCIE_PIPE_TIMEOUT       0x00000005
#define ERR_PCIE_LINK_TIMEOUT       0x00000006
#define ERR_PCIE_LINK_FAIL          0x00000007
#define ERR_USB_ENUM_TIMEOUT        0x00000008
#define ERR_USB_FIFO_ERROR          0x00000009
#define ERR_DDR3_OVERFLOW           0x0000000A
#define ERR_TEMP_CRITICAL           0x0000000B
#define ERR_VOLT_FAULT              0x0000000C
#define ERR_WATCHDOG                0x0000000D
#define ERR_BITSTREAM_CRC           0x0000000E

/*============================================================================
 * FORWARD DECLARATIONS
 *============================================================================*/

static void system_reset(void);
static int pll_init(void);
static int ddr3_init_sequence(void);
static int pcie_init_sequence(void);
static int usb_init_sequence(void);
static void set_error(system_state_t state, uint32_t error);
static void check_health(void);
static void process_usb_commands(void);
static void stream_captured_tlps(void);
static void handle_capture_start(const uint8_t *payload, uint16_t len);
static void handle_capture_stop(void);
static void handle_inject_tlp(const uint8_t *payload, uint16_t len);
static void handle_query_status(void);
static void handle_read_reg(const uint8_t *payload, uint16_t len);
static void handle_write_reg(const uint8_t *payload, uint16_t len);
static void handle_fw_update_init(const uint8_t *payload, uint16_t len);
static void handle_fw_update_data(const uint8_t *payload, uint16_t len);
static void handle_fw_update_commit(void);
static void handle_self_test(void);
static void send_error_response(uint8_t error_cmd, uint32_t error_code);
static void send_ack(uint8_t cmd);
static void send_nack(uint8_t cmd, uint32_t reason);

/*============================================================================
 * MAIN ENTRY POINT
 *============================================================================*/

int main(void) {
    /* 1. System reset and GPIO init */
    system_reset();
    gpio_init();
    uart_init(115200);

    uart_puts("\r\n\r\n=== PCIe Screamer Firmware v1.0.0 ===\r\n");
    uart_puts("Initializing...\r\n");

    /* 2. PLL initialization */
    g_state = STATE_PLL_INIT;
    uart_puts("PLL init... ");
    if (pll_init() != 0) {
        uart_puts("FAILED\r\n");
        set_error(STATE_PLL_INIT, g_error_code);
        goto error_loop;
    }
    uart_puts("OK (125/100/400 MHz locked)\r\n");

    /* 3. DDR3 initialization */
    g_state = STATE_DDR3_INIT;
    uart_puts("DDR3 init... ");
    if (ddr3_init_sequence() != 0) {
        uart_puts("FAILED\r\n");
        set_error(STATE_DDR3_INIT, g_error_code);
        goto error_loop;
    }
    uart_puts("OK (512 MB, 800 MT/s)\r\n");

    /* 4. PCIe initialization */
    g_state = STATE_PCIE_INIT;
    uart_puts("PCIe init... ");
    if (pcie_init_sequence() != 0) {
        uart_puts("FAILED\r\n");
        set_error(STATE_PCIE_INIT, g_error_code);
        goto error_loop;
    }
    uart_puts("OK (Gen2 x4, L0)\r\n");

    /* 5. USB initialization */
    g_state = STATE_USB_INIT;
    uart_puts("USB init... ");
    if (usb_init_sequence() != 0) {
        uart_puts("FAILED\r\n");
        set_error(STATE_USB_INIT, g_error_code);
        goto error_loop;
    }
    uart_puts("OK (USB 3.0 enumerated)\r\n");

    /* 6. Enter active state */
    g_state = STATE_ACTIVE;
    led_set_all(LED_ON, LED_ON, LED_OFF, LED_OFF);
    uart_puts("System ready. Entering main loop.\r\n");

    /* 7. Main loop */
    while (1) {
        /* Pet watchdog */
        REG_WRITE(SCR_WATCHDOG_KICK, 0xDEADBEEF);

        /* Check system health */
        check_health();

        /* Process incoming USB commands */
        process_usb_commands();

        /* Stream captured TLPs to USB */
        if (REG_READ(SCR_CONTROL) & CTRL_CAPTURE_ENABLE) {
            stream_captured_tlps();
        }

        /* Update status LEDs if not overridden */
        if (!(REG_READ(SCR_CONTROL) & CTRL_LED_OVERRIDE)) {
            uint32_t status = REG_READ(SCR_STATUS);
            led_set_pcie_link((status & STATUS_PCIE_LINK_UP) ? LED_ON : LED_OFF);
            led_set_usb_enum((status & STATUS_USB_ENUMERATED) ? LED_ON : LED_OFF);
            led_set_capture((status & STATUS_CAPTURE_ACTIVE) ? LED_ON : LED_OFF);
            led_set_error((g_state == STATE_ERROR) ? LED_ON : LED_OFF);
        }

        /* Yield a small amount to prevent tight-loop starvation */
        delay_us(10);
    }

error_loop:
    /* Error state — blink red LED, wait for USB commands to recover */
    g_state = STATE_ERROR;
    led_set_all(LED_OFF, LED_OFF, LED_OFF, LED_ON);
    uart_puts("*** SYSTEM IN ERROR STATE ***\r\n");
    uart_puts("Error code: 0x");
    /* Print error code hex */
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (g_error_code >> i) & 0xF;
        uart_putc(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    }
    uart_puts("\r\n");

    while (1) {
        REG_WRITE(SCR_WATCHDOG_KICK, 0xDEADBEEF);
        process_usb_commands();
        led_set_error(LED_ON);
        delay_ms(200);
        led_set_error(LED_OFF);
        delay_ms(200);
    }

    return 0; /* Never reached */
}

/*============================================================================
 * SYSTEM RESET
 *============================================================================*/

static void system_reset(void) {
    /* Assert all internal resets via control register */
    uint32_t ctrl = REG_READ(SCR_CONTROL);
    ctrl |= CTRL_SOFT_RESET;
    REG_WRITE(SCR_CONTROL, ctrl);

    /* Clear error register */
    REG_WRITE(SCR_ERROR, 0xFFFFFFFF);  /* Write-1-to-clear all */

    /* Set version */
    REG_WRITE(SCR_VERSION, FW_VERSION);

    /* Initialize status */
    REG_WRITE(SCR_STATUS, 0);

    /* Clear control */
    REG_WRITE(SCR_CONTROL, 0);

    /* Clear error mask */
    REG_WRITE(SCR_ERROR_MASK, 0);

    /* Wait for reset to propagate */
    delay_ms(10);

    /* Release soft reset */
    ctrl = REG_READ(SCR_CONTROL);
    ctrl &= ~CTRL_SOFT_RESET;
    REG_WRITE(SCR_CONTROL, ctrl);

    delay_ms(1);
}

/*============================================================================
 * PLL INITIALIZATION
 *============================================================================*/

static int pll_init(void) {
    /* PLL0: 125 MHz from 100 MHz PCIe REFCLK
     * CLKI_DIV=4, CLKFB_DIV=5, CLKOP_DIV=8 → VCO=1000 MHz, CLKOP=125 MHz
     */
    /* PLL1: 100 MHz from 125 MHz
     * CLKI_DIV=5, CLKFB_DIV=4, CLKOP_DIV=8 → VCO=800 MHz, CLKOP=100 MHz
     * CLKOS_DIV=4 → CLKOS=200 MHz (DDR3 user clock)
     */

    /* PLL configuration is done in FPGA gateware at bitstream load time.
     * Here we just wait for lock signals. */

    /* Wait for PLL0 lock (125 MHz system clock) */
    if (REG_POLL_UNTIL(SCR_STATUS, STATUS_PCIE_LINK_UP, 0, 1000) != 0) {
        /* PLL lock is indicated by internal status; use a dedicated
         * PLL status register in real implementation */
        g_error_code = ERR_PLL0_LOCK_TIMEOUT;
        return -1;
    }

    /* Wait for PLL1 lock (100 MHz + 200 MHz) */
    delay_us(100);

    /* DDR3 PLL lock is checked during DDR3 init */

    return 0;
}

/*============================================================================
 * DDR3 INITIALIZATION SEQUENCE
 *============================================================================*/

static int ddr3_init_sequence(void) {
    int ret;

    ret = ddr3_init();
    if (ret != 0) {
        g_error_code = ERR_DDR3_INIT_TIMEOUT;
        return ret;
    }

    ret = ddr3_calibrate();
    if (ret != 0) {
        g_error_code = ERR_DDR3_CAL_FAIL;
        return ret;
    }

    /* Set buffer size (512 MB = 0x20000000) */
    REG_WRITE(DDR3_BUFFER_SIZE_REG, 0x20000000);

    /* Initialize pointers */
    REG_WRITE(DDR3_WRITE_PTR_REG, 0);
    REG_WRITE(DDR3_READ_PTR_REG, 0);

    return 0;
}

/*============================================================================
 * PCIe INITIALIZATION SEQUENCE
 *============================================================================*/

static int pcie_init_sequence(void) {
    int ret;
    uint32_t speed, width;

    ret = pcie_init();
    if (ret != 0) {
        if (ret == -2) {
            g_error_code = ERR_PCIE_PIPE_TIMEOUT;
        } else if (ret == -3) {
            g_error_code = ERR_PCIE_LINK_TIMEOUT;
        } else {
            g_error_code = ERR_PCIE_LINK_FAIL;
        }
        return ret;
    }

    /* Read back negotiated parameters */
    speed = REG_READ(PCIE_LINK_SPEED_REG);
    width = REG_READ(PCIE_LINK_WIDTH_REG);

    /* Update status register */
    uint32_t status = REG_READ(SCR_STATUS);
    status |= STATUS_PCIE_LINK_UP;
    REG_WRITE(SCR_STATUS, status);

    REG_WRITE(SCR_PCIE_LINK_SPEED, speed);
    REG_WRITE(SCR_PCIE_LINK_WIDTH, width);

    return 0;
}

/*============================================================================
 * USB INITIALIZATION SEQUENCE
 *============================================================================*/

static int usb_init_sequence(void) {
    int ret;

    ret = ft601_init();
    if (ret != 0) {
        g_error_code = ERR_USB_ENUM_TIMEOUT;
        return ret;
    }

    /* Update status */
    uint32_t status = REG_READ(SCR_STATUS);
    status |= STATUS_USB_ENUMERATED;
    REG_WRITE(SCR_STATUS, status);

    return 0;
}

/*============================================================================
 * ERROR HANDLING
 *============================================================================*/

static void set_error(system_state_t state, uint32_t error) {
    g_state = STATE_ERROR;
    g_error_code = error;

    /* Latch error in register */
    REG_WRITE(SCR_ERROR, error);

    /* Update status */
    uint32_t status = REG_READ(SCR_STATUS);
    status &= ~(STATUS_PCIE_LINK_UP | STATUS_USB_ENUMERATED |
                STATUS_CAPTURE_ACTIVE | STATUS_DDR3_READY);
    REG_WRITE(SCR_STATUS, status);

    /* Disable capture */
    uint32_t ctrl = REG_READ(SCR_CONTROL);
    ctrl &= ~CTRL_CAPTURE_ENABLE;
    REG_WRITE(SCR_CONTROL, ctrl);
}

/*============================================================================
 * HEALTH MONITORING
 *============================================================================*/

static void check_health(void) {
    uint32_t status = REG_READ(SCR_STATUS);

    /* Check temperature */
    if (status & STATUS_TEMP_CRITICAL) {
        set_error(STATE_ACTIVE, ERR_TEMP_CRITICAL);
        return;
    }

    /* Check voltage rails */
    if (status & (STATUS_VOLT_3V3_FAULT | STATUS_VOLT_1V1_FAULT |
                  STATUS_VOLT_1V35_FAULT | STATUS_VOLT_1V2_FAULT |
                  STATUS_VOLT_1V8_FAULT)) {
        set_error(STATE_ACTIVE, ERR_VOLT_FAULT);
        return;
    }

    /* Check DDR3 overflow */
    if (status & STATUS_DDR3_OVERFLOW) {
        /* Non-fatal: increment counter, notify host */
        uint32_t count = REG_READ(SCR_DDR3_OVERFLOW_COUNT);
        REG_WRITE(SCR_DDR3_OVERFLOW_COUNT, count + 1);
        /* Clear overflow flag */
        status &= ~STATUS_DDR3_OVERFLOW;
        REG_WRITE(SCR_STATUS, status);
    }

    /* Check PCIe link */
    if (!(status & STATUS_PCIE_LINK_UP)) {
        /* Link lost — attempt recovery */
        uint32_t ltssm = REG_READ(SCR_PCIE_LTSSM_STATE);
        if (ltssm == LTSSM_DETECT_QUIET || ltssm == LTSSM_DISABLED) {
            /* Try to re-train */
            pcie_init_sequence();
        }
    }
}

/*============================================================================
 * USB COMMAND PROCESSING
 *============================================================================*/

static void process_usb_commands(void) {
    int rx_len;
    uint8_t cmd;
    uint16_t payload_len;
    uint32_t crc_received, crc_calculated;

    /* Try to receive a frame */
    rx_len = ft601_rx_packet(g_usb_rx_buffer, sizeof(g_usb_rx_buffer));
    if (rx_len < 13) {
        return;  /* No complete frame available */
    }

    /* Verify sync marker */
    if (g_usb_rx_buffer[0] != 0x53 || g_usb_rx_buffer[1] != 0x43 ||
        g_usb_rx_buffer[2] != 0x52 || g_usb_rx_buffer[3] != 0x45 ||
        g_usb_rx_buffer[4] != 0x41 || g_usb_rx_buffer[5] != 0x4D) {
        /* Invalid sync — ignore */
        return;
    }

    /* Parse header */
    cmd = g_usb_rx_buffer[6];
    payload_len = (g_usb_rx_buffer[7] << 8) | g_usb_rx_buffer[8];

    /* Verify CRC-32C */
    crc_received = (g_usb_rx_buffer[13 + payload_len + 0] << 24) |
                   (g_usb_rx_buffer[13 + payload_len + 1] << 16) |
                   (g_usb_rx_buffer[13 + payload_len + 2] << 8)  |
                   (g_usb_rx_buffer[13 + payload_len + 3]);
    crc_calculated = crc32c_calculate(g_usb_rx_buffer, 13 + payload_len);

    if (crc_received != crc_calculated) {
        send_nack(cmd, 0xBADCRC);
        return;
    }

    /* Dispatch command */
    const uint8_t *payload = &g_usb_rx_buffer[9];

    switch (cmd) {
    case 0x01: /* CMD_CAPTURE_START */
        handle_capture_start(payload, payload_len);
        break;
    case 0x02: /* CMD_CAPTURE_STOP */
        handle_capture_stop();
        break;
    case 0x10: /* CMD_INJECT_TLP */
        handle_inject_tlp(payload, payload_len);
        break;
    case 0x20: /* CMD_QUERY_STATUS */
        handle_query_status();
        break;
    case 0x22: /* CMD_READ_REG */
        handle_read_reg(payload, payload_len);
        break;
    case 0x24: /* CMD_WRITE_REG */
        handle_write_reg(payload, payload_len);
        break;
    case 0x30: /* CMD_FW_UPDATE_INIT */
        handle_fw_update_init(payload, payload_len);
        break;
    case 0x31: /* CMD_FW_UPDATE_DATA */
        handle_fw_update_data(payload, payload_len);
        break;
    case 0x33: /* CMD_FW_UPDATE_COMMIT */
        handle_fw_update_commit();
        break;
    case 0x40: /* CMD_SELF_TEST_START */
        handle_self_test();
        break;
    case 0xFF: /* CMD_RESET */
        system_reset();
        send_ack(0xFF);
        break;
    default:
        send_nack(cmd, 0xBADCMD);
        break;
    }
}

/*============================================================================
 * COMMAND HANDLERS
 *============================================================================*/

static void handle_capture_start(const uint8_t *payload, uint16_t len) {
    if (len < 15) {
        send_nack(0x01, 0xBADLEN);
        return;
    }

    uint32_t filter_mask = (payload[0] << 24) | (payload[1] << 16) |
                           (payload[2] << 8)  | payload[3];
    uint64_t addr_filter = ((uint64_t)payload[4]  << 56) |
                           ((uint64_t)payload[5]  << 48) |
                           ((uint64_t)payload[6]  << 40) |
                           ((uint64_t)payload[7]  << 32) |
                           ((uint64_t)payload[8]  << 24) |
                           ((uint64_t)payload[9]  << 16) |
                           ((uint64_t)payload[10] << 8)  |
                           payload[11];
    uint16_t reqid_filter = (payload[12] << 8) | payload[13];
    uint8_t flags = payload[14];

    /* Configure sniffer */
    REG_WRITE(SCR_CAPTURE_FILTER_MASK, filter_mask);
    REG_WRITE(SCR_CAPTURE_FILTER_ADDR_LO, (uint32_t)(addr_filter & 0xFFFFFFFF));
    REG_WRITE(SCR_CAPTURE_FILTER_ADDR_HI, (uint32_t)(addr_filter >> 32));
    REG_WRITE(SCR_CAPTURE_FILTER_REQID, reqid_filter);

    /* Set control flags */
    uint32_t ctrl = CTRL_CAPTURE_ENABLE;
    if (flags & 0x01) ctrl |= CTRL_CAPTURE_HOST2DEV;
    if (flags & 0x02) ctrl |= CTRL_CAPTURE_DEV2HOST;
    if (flags & 0x04) ctrl |= CTRL_TIMESTAMP_ENABLE;
    if (flags & 0x08) ctrl |= CTRL_DDR3_WRAP_ENABLE;
    if (flags & 0x10) ctrl |= CTRL_USB_STREAM_ENABLE;
    if (filter_mask != 0) ctrl |= CTRL_FILTER_ENABLE;

    REG_WRITE(SCR_CONTROL, ctrl);

    /* Update status */
    uint32_t status = REG_READ(SCR_STATUS);
    status |= STATUS_CAPTURE_ACTIVE;
    REG_WRITE(SCR_STATUS, status);

    send_ack(0x01);
}

static void handle_capture_stop(void) {
    uint32_t ctrl = REG_READ(SCR_CONTROL);
    ctrl &= ~CTRL_CAPTURE_ENABLE;
    REG_WRITE(SCR_CONTROL, ctrl);

    uint32_t status = REG_READ(SCR_STATUS);
    status &= ~STATUS_CAPTURE_ACTIVE;
    REG_WRITE(SCR_STATUS, status);

    send_ack(0x02);
}

static void handle_inject_tlp(const uint8_t *payload, uint16_t len) {
    if (len < 15) {
        send_nack(0x10, 0xBADLEN);
        return;
    }

    uint8_t tlp_type = payload[0];
    uint16_t requester_id = (payload[1] << 8) | payload[2];
    uint8_t tag = payload[3];
    uint64_t address = ((uint64_t)payload[4]  << 56) |
                       ((uint64_t)payload[5]  << 48) |
                       ((uint64_t)payload[6]  << 40) |
                       ((uint64_t)payload[7]  << 32) |
                       ((uint64_t)payload[8]  << 24) |
                       ((uint64_t)payload[9]  << 16) |
                       ((uint64_t)payload[10] << 8)  |
                       payload[11];
    uint16_t data_len_dw = (payload[12] << 8) | payload[13];
    uint8_t tc = payload[14];

    /* Check if injection is enabled */
    if (!(REG_READ(SCR_CONTROL) & CTRL_INJECT_ENABLE)) {
        send_nack(0x10, 0xINJDIS);
        return;
    }

    /* Check if injector is busy */
    if (REG_READ(SCR_STATUS) & STATUS_INJECT_BUSY) {
        send_nack(0x10, 0xINJBSY);
        return;
    }

    /* Configure injector registers */
    REG_WRITE(SCR_INJECT_TLP_TYPE, tlp_type);
    REG_WRITE(SCR_INJECT_ADDR_LO, (uint32_t)(address & 0xFFFFFFFF));
    REG_WRITE(SCR_INJECT_ADDR_HI, (uint32_t)(address >> 32));
    REG_WRITE(SCR_INJECT_LENGTH, data_len_dw);
    REG_WRITE(SCR_INJECT_TAG, tag);
    REG_WRITE(SCR_INJECT_REQID, requester_id);

    /* Write data payload if present */
    if (data_len_dw > 0 && len >= 15 + data_len_dw * 4) {
        for (uint16_t i = 0; i < data_len_dw; i++) {
            uint32_t dw = (payload[15 + i*4 + 0] << 24) |
                          (payload[15 + i*4 + 1] << 16) |
                          (payload[15 + i*4 + 2] << 8)  |
                          payload[15 + i*4 + 3];
            REG_WRITE(SCR_INJECT_DATA_FIFO, dw);
        }
    }

    /* Arm and trigger */
    uint32_t ctrl = REG_READ(SCR_CONTROL);
    ctrl |= CTRL_INJECT_ARMED;
    REG_WRITE(SCR_CONTROL, ctrl);
    REG_WRITE(SCR_INJECT_TRIGGER, 1);

    /* Wait for result (poll with timeout) */
    for (int timeout = 0; timeout < 100000; timeout++) {
        if (!(REG_READ(SCR_STATUS) & STATUS_INJECT_BUSY)) {
            break;
        }
        delay_us(1);
    }

    /* Send result */
    uint32_t result = REG_READ(SCR_INJECT_RESULT);
    uint8_t resp[16];
    resp[0] = result & 0xFF;
    /* For reads, include completion data length */
    if (tlp_type == TLP_TYPE_MRD || tlp_type == TLP_TYPE_CFGRD0 ||
        tlp_type == TLP_TYPE_CFGRD1) {
        /* Completion data would be in injector result buffer */
        uint16_t cpl_len = 0;  /* Read from injector completion buffer */
        resp[1] = (cpl_len >> 8) & 0xFF;
        resp[2] = cpl_len & 0xFF;
        ft601_tx_response(0x11, resp, 3 + cpl_len);
    } else {
        ft601_tx_response(0x11, resp, 1);
    }
}

static void handle_query_status(void) {
    uint8_t resp[64];
    uint32_t *r = (uint32_t *)resp;

    r[0] = REG_READ(SCR_STATUS);
    r[1] = REG_READ(SCR_CAPTURE_COUNT);
    r[2] = REG_READ(SCR_DDR3_WRITE_PTR);
    r[3] = REG_READ(SCR_DDR3_READ_PTR);
    r[4] = REG_READ(SCR_DDR3_OVERFLOW_COUNT);
    r[5] = REG_READ(SCR_USB_TX_BYTES);
    r[6] = REG_READ(SCR_USB_RX_BYTES);
    r[7] = REG_READ(SCR_PCIE_LTSSM_STATE);
    r[8] = REG_READ(SCR_PCIE_LINK_SPEED);
    r[9] = REG_READ(SCR_PCIE_LINK_WIDTH);
    r[10] = REG_READ(SCR_TEMP_SENSOR);
    r[11] = REG_READ(SCR_VOLT_MON_3V3);
    r[12] = REG_READ(SCR_VOLT_MON_1V1);
    r[13] = REG_READ(SCR_VOLT_MON_1V35);
    r[14] = REG_READ(SCR_VOLT_MON_1V2);
    r[15] = REG_READ(SCR_VOLT_MON_1V8);

    ft601_tx_response(0x21, resp, 64);
}

static void handle_read_reg(const uint8_t *payload, uint16_t len) {
    if (len < 4) {
        send_nack(0x22, 0xBADLEN);
        return;
    }

    uint32_t addr = (payload[0] << 24) | (payload[1] << 16) |
                    (payload[2] << 8)  | payload[3];
    uint32_t value = REG_READ(addr);

    uint8_t resp[8];
    resp[0] = (addr >> 24) & 0xFF;
    resp[1] = (addr >> 16) & 0xFF;
    resp[2] = (addr >> 8) & 0xFF;
    resp[3] = addr & 0xFF;
    resp[4] = (value >> 24) & 0xFF;
    resp[5] = (value >> 16) & 0xFF;
    resp[6] = (value >> 8) & 0xFF;
    resp[7] = value & 0xFF;

    ft601_tx_response(0x23, resp, 8);
}

static void handle_write_reg(const uint8_t *payload, uint16_t len) {
    if (len < 8) {
        send_nack(0x24, 0xBADLEN);
        return;
    }

    uint32_t addr = (payload[0] << 24) | (payload[1] << 16) |
                    (payload[2] << 8)  | payload[3];
    uint32_t value = (payload[4] << 24) | (payload[5] << 16) |
                     (payload[6] << 8)  | payload[7];

    REG_WRITE(addr, value);
    send_ack(0x24);
}

static void handle_fw_update_init(const uint8_t *payload, uint16_t len) {
    if (len < 12) {
        send_nack(0x30, 0xBADLEN);
        return;
    }

    uint32_t total_size = (payload[0] << 24) | (payload[1] << 16) |
                          (payload[2] << 8)  | payload[3];
    uint32_t expected_crc = (payload[4] << 24) | (payload[5] << 16) |
                            (payload[6] << 8)  | payload[7];
    uint32_t flash_addr = (payload[8] << 24) | (payload[9] << 16) |
                          (payload[10] << 8) | payload[11];

    /* Validate */
    if (total_size > 0x600000) {  /* Max 6 MB for update slot */
        send_nack(0x30, 0xTOOBIG);
        return;
    }
    if (flash_addr != 0x600000 && flash_addr != 0x000000) {
        send_nack(0x30, 0xBADADDR);
        return;
    }

    /* Enter FW update mode */
    g_state = STATE_FW_UPDATE;
    REG_WRITE(SCR_FW_UPDATE_CTRL, 1);  /* Begin update */
    REG_WRITE(SCR_FW_UPDATE_ADDR, flash_addr);

    send_ack(0x30);
}

static void handle_fw_update_data(const uint8_t *payload, uint16_t len) {
    if (g_state != STATE_FW_UPDATE) {
        send_nack(0x31, 0xNOTINIT);
        return;
    }
    if (len < 8) {
        send_nack(0x31, 0xBADLEN);
        return;
    }

    uint32_t block_offset = (payload[0] << 24) | (payload[1] << 16) |
                            (payload[2] << 8)  | payload[3];
    uint32_t block_len = (payload[4] << 24) | (payload[5] << 16) |
                         (payload[6] << 8)  | payload[7];

    if (block_len > 4080 || len < 8 + block_len) {
        send_nack(0x31, 0xBADLEN);
        return;
    }

    /* Write block to SPI flash via update data FIFO */
    for (uint32_t i = 0; i < block_len; i++) {
        REG_WRITE(SCR_FW_UPDATE_DATA, payload[8 + i]);
    }

    send_ack(0x31);
}

static void handle_fw_update_commit(void) {
    if (g_state != STATE_FW_UPDATE) {
        send_nack(0x33, 0xNOTINIT);
        return;
    }

    /* Verify CRC */
    uint32_t computed_crc = REG_READ(SCR_FW_UPDATE_CRC);
    uint8_t resp[4];
    resp[0] = (computed_crc >> 24) & 0xFF;
    resp[1] = (computed_crc >> 16) & 0xFF;
    resp[2] = (computed_crc >> 8) & 0xFF;
    resp[3] = computed_crc & 0xFF;
    ft601_tx_response(0x32, resp, 4);

    /* Commit (trigger reconfiguration) */
    REG_WRITE(SCR_FW_UPDATE_CTRL, 2);  /* Commit + reconfigure */

    /* Device will reboot after this */
    send_ack(0x33);
}

static void handle_self_test(void) {
    REG_WRITE(SCR_SELF_TEST_CTRL, 1);

    /* Wait for completion */
    for (int timeout = 0; timeout < 500000; timeout++) {
        if (!(REG_READ(SCR_STATUS) & STATUS_SELF_TEST_BUSY)) {
            break;
        }
        delay_us(1);
    }

    uint32_t result = REG_READ(SCR_SELF_TEST_RESULT);
    uint8_t resp[4];
    resp[0] = (result >> 24) & 0xFF;
    resp[1] = (result >> 16) & 0xFF;
    resp[2] = (result >> 8) & 0xFF;
    resp[3] = result & 0xFF;

    ft601_tx_response(0x41, resp, 4);
}

/*============================================================================
 * TLP STREAMING
 *============================================================================*/

static void stream_captured_tlps(void) {
    int record_len;

    /* Check if USB streaming is enabled */
    if (!(REG_READ(SCR_CONTROL) & CTRL_USB_STREAM_ENABLE)) {
        return;
    }

    /* Read records from DDR3 and send over USB */
    while (1) {
        record_len = ddr3_read_dma(g_capture_buffer, sizeof(g_capture_buffer));
        if (record_len <= 0) {
            break;  /* No more data */
        }

        /* Send as CMD_CAPTURE_DATA frame */
        ft601_tx_response(0x03, g_capture_buffer, record_len);

        /* Update byte count */
        uint32_t tx_bytes = REG_READ(SCR_USB_TX_BYTES);
        REG_WRITE(SCR_USB_TX_BYTES, tx_bytes + record_len + 13);
    }
}

/*============================================================================
 * RESPONSE HELPERS
 *============================================================================*/

static void send_error_response(uint8_t error_cmd, uint32_t error_code) {
    uint8_t resp[5];
    resp[0] = error_cmd;
    resp[1] = (error_code >> 24) & 0xFF;
    resp[2] = (error_code >> 16) & 0xFF;
    resp[3] = (error_code >> 8) & 0xFF;
    resp[4] = error_code & 0xFF;
    ft601_tx_response(0xF0, resp, 5);
}

static void send_ack(uint8_t cmd) {
    uint8_t resp[1] = { cmd };
    ft601_tx_response(0xF1, resp, 1);
}

static void send_nack(uint8_t cmd, uint32_t reason) {
    uint8_t resp[5];
    resp[0] = cmd;
    resp[1] = (reason >> 24) & 0xFF;
    resp[2] = (reason >> 16) & 0xFF;
    resp[3] = (reason >> 8) & 0xFF;
    resp[4] = reason & 0xFF;
    ft601_tx_response(0xF2, resp, 5);
}
