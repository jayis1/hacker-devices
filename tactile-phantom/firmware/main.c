/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * main.c — Core 0: PIO/DMA initialization, bus attach, real-time loop
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Core 0 owns the real-time bus handling:
 *   - Configures PIO state machines for transparent through-path + tap
 *   - Sets up DMA channels to ring-buffer captured transactions
 *   - Runs the bus-attach state machine (detect, enumerate, capture)
 *   - Arbitrates injection bus access
 *   - Manages power, status LEDs, and the ESP32-C3 UART link
 *
 * Core 1 (core1.c) runs the decode pipeline and event reconstruction.
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "board.h"
#include "registers.h"

/* --- DMA ring buffer (32-bit words, 8 KB) ----------------------------- */
static uint32_t dma_ring_buf[TP_RING_BUF_WORDS] __attribute__((aligned(64)));

/* DMA channel handles */
static int dma_chan_tap;   /* PIO tap SM -> ring buffer */
static int dma_chan_ctrl;  /* chain control */

/* PIO state machine handles */
static uint pio0_sm_through;  /* through-path */
static uint pio0_sm_tap;      /* tap monitor */
static uint pio1_sm_inject;   /* injection drive */

/* Bus state (shared between cores via mutex) */
static tp_bus_state_t g_bus_state;
static mutex_t g_state_mutex;

/* Core 1 handshake: raw transaction queue */
static uint8_t  core1_raw_buf[512];
static uint16_t core1_raw_len;

/* --- Forward declarations -------------------------------------------- */
static void tp_pio_load_programs(void);
static void tp_dma_setup_ring(void);
static void tp_uart_esp_init(void);
static void tp_adc_init(void);
static void tp_gpio_init_all(void);
static void tp_oled_init(void);
static void tp_oled_status(const char *line1, const char *line2);
static void tp_target_power_wait(void);
static bool tp_bus_attach(tp_bus_state_t *state);
static void tp_capture_loop(tp_bus_state_t *state);
static void tp_inject_arbitrate(tp_bus_state_t *state);
static void tp_heartbeat_task(tp_bus_state_t *state);

/* --- Board initialization --------------------------------------------- */
void tp_board_init(void)
{
    /* Set system clock: 125 MHz (RP2040 default), boost to 133 MHz */
    set_sys_clock_khz(133000, true);

    /* Initialize stdio over USB CDC (for debug) */
    stdio_init_all();

    /* Initialize GPIO for all board functions */
    tp_gpio_init_all();

    /* Initialize ADC for power monitoring */
    tp_adc_init();

    /* Initialize UART to ESP32-C3 (BLE backhaul) */
    tp_uart_esp_init();

    /* Initialize OLED status display */
    tp_oled_init();

    /* Initialize mutex for shared bus state */
    mutex_init(&g_state_mutex);

    /* Initialize bus state */
    memset(&g_bus_state, 0, sizeof(g_bus_state));
    g_bus_state.mode = TP_BUS_AUTO;
    g_bus_state.vendor = TP_TC_UNKNOWN;

    /* Load PIO programs for bus tap and injection */
    tp_pio_load_programs();

    /* Set up DMA ring buffer for tap capture */
    tp_dma_setup_ring();

    tp_oled_status("TactilePhantom", "v1.0 jayis1");
    sleep_ms(500);

    printf("[TP] Tactile-Phantom v%d.%d.%d (%s)\n",
           TP_VERSION_MAJOR, TP_VERSION_MINOR, TP_VERSION_PATCH, TP_AUTHOR);
}

/* --- GPIO initialization --------------------------------------------- */
static void tp_gpio_init_all(void)
{
    /* Status LEDs */
    gpio_init(PIN_LED_STATUS);
    gpio_set_dir(PIN_LED_STATUS, GPIO_OUT);
    gpio_init(PIN_LED_INJECT);
    gpio_set_dir(PIN_LED_INJECT, GPIO_OUT);
    gpio_init(PIN_LED_USB);
    gpio_set_dir(PIN_LED_USB, GPIO_OUT);

    /* TXB0108 output enable — default disabled until bus attached */
    gpio_init(PIN_TXB_OE);
    gpio_set_dir(PIN_TXB_OE, GPIO_OUT);
    gpio_put(PIN_TXB_OE, 0);  /* disabled: level shifters off */

    /* Bus mode select (read with pull-up; float = auto) */
    gpio_init(PIN_BUS_MODE);
    gpio_set_dir(PIN_BUS_MODE, GPIO_IN);
    gpio_pull_up(PIN_BUS_MODE);

    /* Boot button */
    gpio_init(PIN_BOOT_BTN);
    gpio_set_dir(PIN_BOOT_BTN, GPIO_IN);
    gpio_pull_up(PIN_BOOT_BTN);

    /* Target VCC detect */
    gpio_init(PIN_SENSE_TARGET);
    gpio_set_dir(PIN_SENSE_TARGET, GPIO_IN);
    gpio_pull_down(PIN_SENSE_TARGET);

    /* Bus tap GPIOs will be claimed by PIO; set them to input first */
    for (uint p = PIN_TGT_SDA_MOSI; p <= PIN_INJ_SCL_SCK; p++) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
        gpio_pull_up(p);  /* I2C idle high; harmless for SPI */
    }

    /* SD card SPI pins claimed by SPI1 later */
    for (uint p = PIN_SD_MISO; p <= PIN_SD_MOSI; p++) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
    }
}

/* --- ADC initialization (power monitoring) ---------------------------- */
static void tp_adc_init(void)
{
    adc_init();
    adc_gpio_init(PIN_ADC_TARGET);  /* ADC0: target bus voltage */
    adc_gpio_init(PIN_ADC_BATT);    /* ADC1: LiPo battery voltage */
    adc_gpio_init(PIN_ADC_REF);     /* ADC2: spare */
}

/* Read target bus voltage in millivolts (ADC 0..3.3V, divider 2:1) */
static uint16_t tp_read_target_voltage_mv(void)
{
    adc_select_input(0);  /* ADC0 = target voltage */
    uint16_t raw = adc_read();  /* 0..4095 = 0..3.3V */
    /* voltage = raw * 3300 / 4095 * 2 (divider) */
    return (uint16_t)((uint32_t)raw * 6600 / 4095);
}

/* Read LiPo battery voltage in millivolts (ADC1, divider 2:1) */
static uint16_t tp_read_battery_mv(void)
{
    adc_select_input(1);
    uint16_t raw = adc_read();
    return (uint16_t)((uint32_t)raw * 6600 / 4095);
}

/* --- UART to ESP32-C3 (BLE module) ----------------------------------- */
static void tp_uart_esp_init(void)
{
    uart_init(uart1, PIN_ESP_UART_BAUD);
    gpio_set_function(PIN_ESP_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_ESP_UART_RX, GPIO_FUNC_UART);
    uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart1, true);
}

/* --- OLED (SSD1306 128x32) bit-banged I2C ----------------------------- */
/* Minimal I2C bit-bang for OLED status; no library needed. */
static void tp_i2c_bitbang_delay(void)
{
    /* ~100 kHz at 133 MHz: ~665 cycles -> ~5 us */
    busy_wait_us_32(5);
}

static void tp_i2c_sda(bool high)
{
    gpio_put(PIN_OLED_SDA, high);
    tp_i2c_bitbang_delay();
}

static void tp_i2c_scl_pulse(void)
{
    gpio_put(PIN_OLED_SCL, 1);
    tp_i2c_bitbang_delay();
    gpio_put(PIN_OLED_SCL, 0);
    tp_i2c_bitbang_delay();
}

static void tp_i2c_start(void)
{
    gpio_put(PIN_OLED_SDA, 1);
    gpio_put(PIN_OLED_SCL, 1);
    tp_i2c_bitbang_delay();
    gpio_put(PIN_OLED_SDA, 0);
    tp_i2c_bitbang_delay();
    gpio_put(PIN_OLED_SCL, 0);
}

static void tp_i2c_stop(void)
{
    gpio_put(PIN_OLED_SDA, 0);
    gpio_put(PIN_OLED_SCL, 1);
    tp_i2c_bitbang_delay();
    gpio_put(PIN_OLED_SDA, 1);
    tp_i2c_bitbang_delay();
}

static bool tp_i2c_write_byte(uint8_t byte)
{
    for (int i = 7; i >= 0; i--) {
        gpio_put(PIN_OLED_SDA, (byte >> i) & 1);
        tp_i2c_scl_pulse();
    }
    /* Read ACK */
    gpio_put(PIN_OLED_SDA, 1);
    gpio_put(PIN_OLED_SCL, 1);
    tp_i2c_bitbang_delay();
    bool ack = (gpio_get(PIN_OLED_SDA) == 0);
    gpio_put(PIN_OLED_SCL, 0);
    return ack;
}

static void tp_oled_cmd(uint8_t cmd)
{
    tp_i2c_start();
    tp_i2c_write_byte(TP_OLED_I2C_ADDR << 1);
    tp_i2c_write_byte(0x00);  /* control byte: command */
    tp_i2c_write_byte(cmd);
    tp_i2c_stop();
}

static void tp_oled_data(uint8_t data)
{
    tp_i2c_start();
    tp_i2c_write_byte(TP_OLED_I2C_ADDR << 1);
    tp_i2c_write_byte(0x40);  /* control byte: data */
    tp_i2c_write_byte(data);
    tp_i2c_stop();
}

static void tp_oled_init(void)
{
    gpio_init(PIN_OLED_SDA);
    gpio_set_dir(PIN_OLED_SDA, GPIO_OUT);
    gpio_init(PIN_OLED_SCL);
    gpio_set_dir(PIN_OLED_SCL, GPIO_OUT);
    gpio_put(PIN_OLED_SDA, 1);
    gpio_put(PIN_OLED_SCL, 1);
    sleep_ms(50);

    /* SSD1306 128x32 initialization sequence */
    tp_oled_cmd(0xAE);  /* display off */
    tp_oled_cmd(0xD5); tp_oled_cmd(0x80);  /* clock divide */
    tp_oled_cmd(0xA8); tp_oled_cmd(0x1F);  /* multiplex 1/32 */
    tp_oled_cmd(0xD3); tp_oled_cmd(0x00);  /* display offset */
    tp_oled_cmd(0x40);  /* start line 0 */
    tp_oled_cmd(0x8D); tp_oled_cmd(0x14);  /* charge pump on */
    tp_oled_cmd(0x20); tp_oled_cmd(0x00);  /* horizontal addressing */
    tp_oled_cmd(0xA1);  /* segment remap */
    tp_oled_cmd(0xC8);  /* COM scan direction */
    tp_oled_cmd(0xDA); tp_oled_cmd(0x02);  /* COM pins */
    tp_oled_cmd(0x81); tp_oled_cmd(0xCF);  /* contrast */
    tp_oled_cmd(0xD9); tp_oled_cmd(0xF1);  /* pre-charge */
    tp_oled_cmd(0xDB); tp_oled_cmd(0x40);  /* VCOM deselect */
    tp_oled_cmd(0xA4);  /* display follows RAM */
    tp_oled_cmd(0xA6);  /* normal display */
    tp_oled_cmd(0xAF);  /* display on */
}

/* 5x7 font for OLED status text (simplified) */
static const uint8_t tp_font5x7[][5] = {
    [0]  = {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    [1]  = {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    /* ... (full font omitted for brevity; space chars are blank) */
};
#define TP_FONT_HEIGHT 7
#define TP_FONT_WIDTH   5

static void tp_oled_draw_char(uint8_t col, uint8_t page, char c)
{
    if (c < ' ' || c > '~') c = ' ';
    uint8_t idx = (uint8_t)(c - ' ');
    if (idx >= 80) return;  /* not in our mini font */
    for (uint8_t i = 0; i < TP_FONT_WIDTH; i++) {
        uint8_t bits = 0;
        if (idx < (uint8_t)(sizeof(tp_font5x7)/sizeof(tp_font5x7[0])))
            bits = tp_font5x7[idx][i];
        /* Set column address */
        tp_oled_cmd(0x21); tp_oled_cmd(col + i); tp_oled_cmd(col + i);
        tp_oled_cmd(0x22); tp_oled_cmd(page); tp_oled_cmd(page);
        tp_oled_data(bits);
    }
}

static void tp_oled_status(const char *line1, const char *line2)
{
    /* Clear display (4 pages × 128 cols) */
    for (uint8_t page = 0; page < 4; page++) {
        tp_oled_cmd(0x22); tp_oled_cmd(page); tp_oled_cmd(page);
        tp_oled_cmd(0x21); tp_oled_cmd(0); tp_oled_cmd(127);
        for (uint8_t c = 0; c < 128; c++)
            tp_oled_data(0x00);
    }
    if (line1) {
        for (uint8_t i = 0; line1[i] && i < 21; i++)
            tp_oled_draw_char(i * 6, 0, line1[i]);
    }
    if (line2) {
        for (uint8_t i = 0; line2[i] && i < 21; i++)
            tp_oled_draw_char(i * 6, 1, line2[i]);
    }
}

/* --- PIO program loading --------------------------------------------- */
/* The PIO programs are assembled at runtime via the SDK's pio_add_program.
 * In a real build these would come from bus_tap.pio.h (generated by pioasm).
 * Here we use inline PIO assembly via the SDK's pio_add_program API.
 *
 * Through-path: copies input pin -> output pin with 1-cycle delay.
 *   I2C: forward SCL (bidirectional handled by TXB0108), SDA forwarded both ways.
 *   For simplicity in this design, the through-path uses GPIO bypass mode:
 *   the TXB0108 physically forwards the signal, and PIO only monitors.
 *   The "through-path" is thus the TXB0108 hardware; PIO does the tap.
 *
 * This simplifies the design: TXB0108 handles transparent through-path,
 * PIO state machines handle tap (read-only) and injection (write). */

static void tp_pio_load_programs(void)
{
    /* We use PIO0 for tap monitoring and PIO1 for injection.
     * The through-path is handled by TXB0108 hardware (OE controlled by GPIO),
     * so no PIO program is needed for forwarding — the signal passes through
     * the level shifter with <3 ns delay.
     *
     * Tap program (PIO0 SM0): sample SDA on SCL edges, shift into ISR, DMA.
     * Injection program (PIO1 SM0): drive SDA/SCL to write register data.
     *
     * For this firmware, the tap is implemented as an I2C/SPI sniffer
     * using PIO. The actual PIO assembly would be generated by pioasm from
     * bus_tap.pio. Here we define the pin mapping and clock config.
     */

    /* Configure PIO clock: 125 MHz (fast enough for 1 MHz I2C, 8 MHz SPI) */
    uint pio0_clk = clock_get_hz(clk_sys);
    (void)pio0_clk;

    /* SM allocation */
    pio0_sm_tap = 0;     /* PIO0 SM0: I2C/SPI tap monitor */
    pio1_sm_inject = 0;  /* PIO1 SM0: injection driver */

    /* In a full build, pio_add_program() would load the assembled .pio
     * program here. For this design, we stub the SM configuration and
     * use software-driven capture via GPIO polling for the tap, since
     * the PIO program depends on pioasm-generated headers.
     *
     * The production firmware would include bus_tap.pio.h with:
     *   - i2c_tap_program (PIO0): samples SDA on SCL rising edge
     *   - spi_tap_program (PIO0): samples MOSI/MISO on SCK edge
     *   - i2c_inject_program (PIO1): drives SDA/SCL for injection writes
     */
    pio_sm_set_consecutive_pindirs(pio0, pio0_sm_tap, PIN_TAP_SDA_MOSI, 2, false);
    pio_sm_set_consecutive_pindirs(pio1, pio1_sm_inject, PIN_INJ_SDA_MOSI, 2, false);

    printf("[TP] PIO programs configured (tap=PIO0_SM0, inject=PIO1_SM0)\n");
}

/* --- DMA ring buffer setup ------------------------------------------- */
static void tp_dma_setup_ring(void)
{
    /* Allocate two DMA channels: one for PIO tap data, one for chaining
     * (ring buffer wrap-around). */
    dma_chan_tap  = dma_claim_unused_channel(true);
    dma_chan_ctrl = dma_claim_unused_channel(true);

    /* Configure the tap DMA channel to read from PIO0 RX FIFO and write
     * to the ring buffer. Chain to ctrl channel for wrap-around. */
    dma_channel_config c = dma_channel_get_default_config(dma_chan_tap);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);   /* fixed: PIO RX FIFO */
    channel_config_set_write_increment(&c, true);    /* ring buffer */
    channel_config_set_ring(&c, true, 11);           /* ring at 2^11 = 2048 words */
    channel_config_set_chain_to(&c, dma_chan_ctrl);
    /* In production: channel_config_set_dreq(&c, pio_get_dreq(pio0, pio0_sm_tap, false)); */

    dma_channel_configure(dma_chan_tap, &c,
        dma_ring_buf,               /* write: ring buffer */
        NULL,                       /* read: set by PIO FIFO (placeholder) */
        TP_RING_BUF_WORDS,
        false);                     /* don't trigger yet */

    /* Control channel re-enables the tap channel for continuous ring */
    dma_channel_config c2 = dma_channel_get_default_config(dma_chan_ctrl);
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_32);
    channel_config_set_read_increment(&c2, false);
    channel_config_set_write_increment(&c2, false);
    dma_channel_configure(dma_chan_ctrl, &c2,
        &dma_hw->ch[dma_chan_tap].al1_count,  /* write: reload count */
        &dma_hw->ch[dma_chan_tap].al1_count,  /* read: same value */
        1,
        false);

    printf("[TP] DMA ring configured: %u words, ch=%d ctrl=%d\n",
           TP_RING_BUF_WORDS, dma_chan_tap, dma_chan_ctrl);
}

/* --- Target power wait ---------------------------------------------- */
static void tp_target_power_wait(void)
{
    tp_oled_status("Waiting for", "target power...");
    printf("[TP] Waiting for target bus power...\n");

    uint32_t timeout = 0;
    while (true) {
        uint16_t v = tp_read_target_voltage_mv();
        if (v > 1000) {  /* > 1.0 V: bus is powered */
            printf("[TP] Target bus voltage: %u mV\n", v);
            break;
        }
        sleep_ms(100);
        if (++timeout > 300) {  /* 30 s timeout */
            tp_oled_status("No target", "timeout");
            printf("[TP] Target power timeout\n");
            /* Continue anyway in parasitic mode (battery powered) */
            break;
        }
    }
}

/* --- Bus attach: enable TXB0108, run auto-detect --------------------- */
static bool tp_bus_attach(tp_bus_state_t *state)
{
    tp_oled_status("Attaching", "bus...");
    printf("[TP] Enabling TXB0108 level shifters...\n");

    /* Enable TXB0108 through-path (transparent forwarding enabled) */
    gpio_put(PIN_TXB_OE, 1);
    sleep_ms(10);  /* let TXB0108 settle */

    /* Read bus mode jumper */
    if (gpio_get(PIN_BUS_MODE) == 0) {
        state->mode = TP_BUS_I2C;
        printf("[TP] Mode jumper: I2C\n");
    } else {
        state->mode = TP_BUS_AUTO;
        printf("[TP] Mode jumper: auto-detect\n");
    }

    /* Run protocol auto-detect (implemented in protocol.c) */
    bool ok = tp_protocol_auto_detect(state);
    if (!ok) {
        tp_oled_status("Detect FAIL", "check bus");
        printf("[TP] Auto-detect failed\n");
        gpio_put(PIN_TXB_OE, 0);  /* disable through-path on failure */
        return false;
    }

    state->attached = true;
    state->capturing = true;

    /* Update OLED with detected info */
    char line2[22];
    snprintf(line2, sizeof(line2), "%s %s 0x%02X",
             state->mode == TP_BUS_I2C ? "I2C" : "SPI",
             tp_protocol_vendor_name(state->vendor),
             state->i2c_addr);
    tp_oled_status("CAPTURING", line2);

    printf("[TP] Bus attached: %s %s @0x%02X, %lu Hz, %ux%u\n",
           state->mode == TP_BUS_I2C ? "I2C" : "SPI",
           tp_protocol_vendor_name(state->vendor),
           state->i2c_addr, (unsigned long)state->clock_hz,
           state->x_resolution, state->y_resolution);

    /* Start DMA capture */
    dma_channel_start(dma_chan_tap);

    /* Launch core 1 for decode pipeline */
    multicore_launch_core1(core1_main);
    printf("[TP] Core 1 launched (decode pipeline)\n");

    return true;
}

/* --- Main capture loop (core 0) -------------------------------------- */
/* Core 0 monitors the DMA ring buffer, feeds raw transactions to core 1,
 * handles injection arbitration, and runs periodic tasks. */
static void tp_capture_loop(tp_bus_state_t *state)
{
    uint32_t last_heartbeat = 0;
    uint32_t ring_head = 0;  /* our read position in the ring */

    printf("[TP] Entering capture loop\n");

    while (true) {
        /* Check DMA ring for new data.
         * In production, this would read PIO RX FIFO via DMA and extract
         * I2C/SPI transactions. Here we poll the ring buffer write position
         * and feed complete transactions to core 1. */

        uint32_t dma_write_pos = dma_hw->ch[dma_chan_tap].write_addr;
        uint32_t buf_base = (uint32_t)dma_ring_buf;
        uint32_t buf_end  = buf_base + sizeof(dma_ring_buf);
        uint32_t write_idx = (dma_write_pos - buf_base) / sizeof(uint32_t);

        /* Process new words in the ring buffer */
        if (write_idx != ring_head) {
            uint32_t words_to_read;
            if (write_idx > ring_head) {
                words_to_read = write_idx - ring_head;
            } else {
                /* Wrap-around */
                words_to_read = (TP_RING_BUF_WORDS - ring_head) + write_idx;
            }

            /* Read up to 128 words (512 bytes) per iteration */
            uint32_t to_read = words_to_read > 128 ? 128 : words_to_read;
            uint8_t  raw_bytes[512];
            uint16_t raw_len = 0;

            for (uint32_t i = 0; i < to_read; i++) {
                uint32_t word = dma_ring_buf[ring_head];
                /* Each 32-bit word contains 4 bytes of bus data.
                 * For I2C: these are (SCL_state, SDA_state) pairs or
                 * raw byte values depending on PIO program design.
                 * For this design, we treat each word as 4 raw bytes. */
                raw_bytes[raw_len++] = (word >>  0) & 0xFF;
                raw_bytes[raw_len++] = (word >>  8) & 0xFF;
                raw_bytes[raw_len++] = (word >> 16) & 0xFF;
                raw_bytes[raw_len++] = (word >> 24) & 0xFF;

                ring_head = (ring_head + 1) % TP_RING_BUF_WORDS;
            }

            state->total_transactions += to_read;

            /* Submit raw data to core 1 for decode */
            if (raw_len > 0) {
                core1_submit_raw(raw_bytes, raw_len);
            }
        }

        /* Injection arbitration: if armed and commands pending, execute
         * during host inter-poll gap */
        if (state->inject_armed && tp_injection_pending() > 0) {
            tp_inject_arbitrate(state);
        }

        /* Heartbeat task (every 1 second) */
        uint32_t now = time_us_32();
        if (now - last_heartbeat >= TP_HEARTBEAT_MS * 1000) {
            last_heartbeat = now;
            tp_heartbeat_task(state);
        }

        /* Small yield to avoid busy-spinning 100% */
        tight_loop_contents();
    }
}

/* --- Injection arbitration ------------------------------------------- */
static void tp_inject_arbitrate(tp_bus_state_t *state)
{
    /* Wait for a gap in host polling (bus idle for >= 2 ms) */
    /* In production, this would monitor SCL/SCK for idle and then
     * trigger the injection PIO SM. For this design, we execute
     * the injection command via the injection module. */
    tp_inj_cmd_t cmd;
    if (!tp_ble_link_poll_command(&cmd))
        return;

    printf("[TP] Injecting: type=%d\n", cmd.type);
    tp_inject_led_set(true);

    bool ok = tp_injection_execute(&cmd, state);
    if (ok) {
        state->total_events++;
        printf("[TP] Injection complete\n");
    } else {
        printf("[TP] Injection failed\n");
    }

    tp_inject_led_set(false);
}

/* --- Heartbeat task ------------------------------------------------- */
static void tp_heartbeat_task(tp_bus_state_t *state)
{
    /* Read battery voltage */
    uint16_t batt_mv = tp_read_battery_mv();
    uint8_t  batt_pct = (uint8_t)((uint32_t)(batt_mv - 3300) * 100 / (4200 - 3300));
    if (batt_pct > 100) batt_pct = 100;

    /* Update OLED periodically */
    char line2[22];
    if (state->attached && state->capturing) {
        snprintf(line2, sizeof(line2), "EV:%lu B:%d%%",
                 (unsigned long)state->total_events, batt_pct);
        tp_oled_status("CAPTURING", line2);
    }

    /* Send heartbeat over BLE link */
    tp_ble_link_task();

    /* Check boot button for mode toggle */
    if (gpio_get(PIN_BOOT_BTN) == 0) {
        state->inject_armed = !state->inject_armed;
        tp_inject_led_set(state->inject_armed);
        sleep_ms(200);  /* debounce */
        printf("[TP] Inject armed: %s\n",
               state->inject_armed ? "YES" : "NO");
    }

    /* Check storage rotation */
    tp_storage_rotate();
}

/* --- Status LED helpers --------------------------------------------- */
void tp_status_led_set(bool on)   { gpio_put(PIN_LED_STATUS, on); }
void tp_inject_led_set(bool on)   { gpio_put(PIN_LED_INJECT, on); }

/* --- Main entry point ----------------------------------------------- */
int main(void)
{
    /* Initialize all board hardware */
    tp_board_init();

    /* Initialize storage (SD card) */
    if (!tp_storage_init()) {
        printf("[TP] WARNING: SD card init failed — logging to BLE only\n");
    }

    /* Initialize BLE link to ESP32-C3 */
    if (!tp_ble_link_init()) {
        printf("[TP] WARNING: BLE link init failed — capture-only mode\n");
    }

    /* Wait for target power */
    tp_target_power_wait();

    /* Attach to the bus */
    tp_bus_state_t *state = &g_bus_state;
    if (!tp_bus_attach(state)) {
        printf("[TP] Bus attach failed — entering standby\n");
        tp_oled_status("STANDBY", "no bus");
        /* In standby, wait for manual trigger via boot button */
        while (true) {
            if (gpio_get(PIN_BOOT_BTN) == 0) {
                sleep_ms(50);
                if (tp_bus_attach(state))
                    break;
            }
            sleep_ms(100);
        }
    }

    /* Main capture loop (runs on core 0 forever) */
    tp_capture_loop(state);

    return 0;  /* never reached */
}