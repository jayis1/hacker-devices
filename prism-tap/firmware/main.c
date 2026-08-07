/*
 * main.c — Prism-Tap Main Firmware
 * MIPI CSI-2 / DSI Camera & Display Interface Tap & Injection Implant
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This is the main entry point for the Prism-Tap firmware. It initializes
 * all hardware peripherals, loads the FPGA bitstream, configures MIPI bridges,
 * and runs the main cooperative scheduler loop that services BLE commands,
 * polls the capture engine, updates the OLED display, and monitors power.
 */

#include "board.h"
#include "registers.h"
#include "drivers/fpga_if.h"
#include "drivers/mipi_bridge.h"
#include "drivers/frame_capture.h"
#include "drivers/frame_inject.h"
#include "drivers/ble_if.h"
#include "drivers/sdcard.h"
#include "drivers/usb_cdc.h"
#include "drivers/oled.h"
#include "drivers/protocol.h"
#include "drivers/power.h"

/* ---- Global status ---- */
device_status_t g_status;
boot_stage_t g_boot_stage;

/* ---- SysTick counter (1ms tick) ---- */
static volatile uint32_t systick_ms = 0;

/* ---- Forward declarations ---- */
static void clock_init(void);
static void gpio_init(void);
static void leds_init(void);
static void debug_uart_init(void);
static void systick_init(void);
static void wdt_init(void);
static void update_status(void);
static void update_oled(void);

/* ---- UART4 ISR (BLE module) ---- */
/* IRQ 52: UART4 global interrupt */
void UART4_IRQHandler(void)
{
    extern void ble_uart_rx_isr(void);
    ble_uart_rx_isr();
}

/* ---- SysTick ISR ---- */
void SysTick_Handler(void)
{
    systick_ms++;
    g_status.uptime_ms = systick_ms;
}

/* ---- Clock initialization ----
 * Configure PLL1 for 550 MHz system clock from 25 MHz HSE.
 * PLL1: VCO = HSE * (N / M), SYSCLK = VCO / P
 * HSE = 25 MHz, M = 5, N = 110, P = 1 → VCO = 550 MHz, SYSCLK = 550 MHz
 */
static void clock_init(void)
{
    g_boot_stage = BOOT_STAGE_CLOCKS;

    /* Enable HSE */
    RCC_CR |= RCC_CR_HSEON;
    uint32_t timeout = 10000;
    while (!(RCC_CR & RCC_CR_HSERDY) && timeout--)
        ;

    if (timeout == 0) {
        /* HSE failed — fall back to HSI (64 MHz) */
        RCC_CR |= RCC_CR_HSION;
        while (!(RCC_CR & RCC_CR_HSIRDY))
            ;
        /* Use HSI as system clock (simplified) */
        return;
    }

    /* Configure voltage scaling for high speed (VOS1) */
    PWR_CR1 &= ~PWR_CR1_VOS_MASK;
    PWR_CR1 |= (3U << PWR_CR1_VOS_SHIFT);  /* VOS1 = highest performance */

    /* Wait for voltage scaling to be ready */
    timeout = 1000;
    while (!(PWR_SR1 & (1U << 13)) && timeout--)  /* VOSRDY bit */
        ;

    /* Configure PLL1: M=5, N=110, P=1, Q=2, R=2 */
    RCC_PLL1CFGR = (5U << 0) | (110U << 8) | (1U << 24) | (2U << 16) | (2U << 20);

    /* Enable PLL1 */
    RCC_CR |= RCC_CR_PLL1ON;
    timeout = 10000;
    while (!(RCC_CR & RCC_CR_PLL1RDY) && timeout--)
        ;

    /* Configure flash latency for 550 MHz (5 wait states) */
    FLASH_ACR = (FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN |
                 (5U << 0));

    /* Set PLL1 as system clock source */
    RCC_CFGR = (3U << 0);  /* SW = PLL1 */
    timeout = 10000;
    while (((RCC_CFGR >> 3) & 3) != 3 && timeout--)  /* SWS = PLL1 */
        ;

    /* Configure AHB/APB dividers */
    RCC_D1CFGR = (1U << 0) | (0U << 4) | (0U << 8);  /* HCLK = SYSCLK/2, APB2 = HCLK/1 */
    RCC_D2CFGR = (1U << 0) | (0U << 4) | (0U << 8);  /* APB1 = HCLK/2 */
    RCC_D3CFGR = 0;
}

/* ---- GPIO initialization ---- */
static void gpio_init(void)
{
    g_boot_stage = BOOT_STAGE_GPIO;

    /* Enable all GPIO port clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                   RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                   RCC_AHB4ENR_GPIOHEN;
}

/* ---- LED initialization ---- */
static void leds_init(void)
{
    g_boot_stage = BOOT_STAGE_GPIO;

    /* Configure PC13, PC14, PC15 as output (LEDs, active low) */
    uint32_t moder = GPIO_REG(GPIOC_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (LED_STATUS_PIN * 2));
    moder &= ~(3U << (LED_CAPTURE_PIN * 2));
    moder &= ~(3U << (LED_INJECT_PIN * 2));
    moder |= (GPIO_MODE_OUTPUT << (LED_STATUS_PIN * 2));
    moder |= (GPIO_MODE_OUTPUT << (LED_CAPTURE_PIN * 2));
    moder |= (GPIO_MODE_OUTPUT << (LED_INJECT_PIN * 2));
    GPIO_REG(GPIOC_BASE, GPIO_MODER_OFFSET) = moder;

    /* All LEDs off (active low → set high) */
    GPIO_SET(LED_STATUS_PORT, LED_STATUS_PIN);
    GPIO_SET(LED_CAPTURE_PORT, LED_CAPTURE_PIN);
    GPIO_SET(LED_INJECT_PORT, LED_INJECT_PIN);
}

/* ---- Debug UART (USART1, PA9/PA10, 115200 baud) ---- */
static void debug_uart_init(void)
{
    g_boot_stage = BOOT_STAGE_GPIO;

    /* Enable GPIOA and USART1 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    /* PA9 (TX) and PA10 (RX) as AF7 (USART1) */
    uint32_t moder = GPIO_REG(GPIOA_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (9 * 2));
    moder &= ~(3U << (10 * 2));
    moder |= (GPIO_MODE_AF << (9 * 2));
    moder |= (GPIO_MODE_AF << (10 * 2));
    GPIO_REG(GPIOA_BASE, GPIO_MODER_OFFSET) = moder;

    uint32_t afrl = GPIO_REG(GPIOA_BASE, GPIO_AFRL_OFFSET);
    afrl &= ~(0xFU << (9 * 4));
    afrl |= (AF_USART1_TX_PA9 << (9 * 4));
    afrl &= ~(0xFU << (10 * 4));
    afrl |= (AF_USART1_RX_PA10 << (10 * 4));
    GPIO_REG(GPIOA_BASE, GPIO_AFRL_OFFSET) = afrl;

    /* Configure USART1: 115200 baud, 8N1 */
    USART_CR1(USART1_BASE) = 0;
    USART_BRR(USART1_BASE) = (APB2_FREQ / 115200);
    USART_CR1(USART1_BASE) = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;
}

/* ---- Debug print ---- */
static void debug_print(const char *str)
{
    if (!str)
        return;
    while (*str) {
        while (!(USART_ISR(USART1_BASE) & USART_ISR_TXE))
            ;
        USART_TDR(USART1_BASE) = (uint8_t)*str++;
    }
}

static void debug_print_uint(uint32_t val)
{
    char buf[12];
    int len = 0;
    if (val == 0) {
        buf[len++] = '0';
    } else {
        while (val > 0 && len < 11) {
            buf[len++] = '0' + (val % 10);
            val /= 10;
        }
    }
    for (int i = len - 1; i >= 0; i--) {
        while (!(USART_ISR(USART1_BASE) & USART_ISR_TXE))
            ;
        USART_TDR(USART1_BASE) = (uint8_t)buf[i];
    }
}

/* ---- SysTick initialization (1ms tick) ---- */
static void systick_init(void)
{
    g_boot_stage = BOOT_STAGE_INIT;

    /* Configure SysTick for 1ms interrupt at 275 MHz AHB */
    SYSTICK_RVR = (AHB_FREQ / 1000) - 1;
    SYSTICK_CVR = 0;
    SYSTICK_CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_CLKSRC | SYSTICK_CSR_TICKINT;
}

/* ---- Independent Watchdog (IWDG) ---- */
static void wdt_init(void)
{
    /* Enable IWDG with 4-second timeout */
    IWDG_KR = IWDG_KR_ENABLE;
    IWDG_KR = IWDG_KR_UNLOCK;
    IWDG_PR = 6;  /* Prescaler /256 → 32768/256 = 128 Hz */
    IWDG_RLR = 512;  /* 512 / 128 = 4 seconds */
    IWDG_KR = IWDG_KR_RELOAD;
}

static void wdt_reload(void)
{
    IWDG_KR = IWDG_KR_RELOAD;
}

/* ---- Status update ---- */
static void update_status(void)
{
    /* Read FPGA link status */
    g_status.csi_link_up = fpga_csi_link_up();
    g_status.dsi_link_up = fpga_dsi_link_up();
    g_status.fpga_ready = fpga_is_ready();
    g_status.sd_present = sdcard_present();
    g_status.ble_connected = ble_is_connected();
    g_status.usb_connected = usb_is_connected();

    /* Update capture/inject counts */
    if (g_status.capture.active)
        g_status.capture.frames_captured = capture_get_frame_count();

    if (g_status.inject.active)
        inject_get_count();

    /* Update LED states */
    if (g_status.fpga_ready)
        GPIO_RESET(LED_STATUS_PORT, LED_STATUS_PIN);  /* LED on */
    else
        GPIO_SET(LED_STATUS_PORT, LED_STATUS_PIN);   /* LED off */

    if (g_status.capture.active)
        GPIO_RESET(LED_CAPTURE_PORT, LED_CAPTURE_PIN);
    else
        GPIO_SET(LED_CAPTURE_PORT, LED_CAPTURE_PIN);

    if (g_status.inject.active)
        GPIO_RESET(LED_INJECT_PORT, LED_INJECT_PIN);
    else
        GPIO_SET(LED_INJECT_PORT, LED_INJECT_PIN);
}

/* ---- OLED update (throttled to ~2 fps) ---- */
static uint32_t last_oled_update = 0;
static void update_oled(void)
{
    uint32_t now = systick_ms;
    if ((now - last_oled_update) < 500 && last_oled_update != 0)
        return;
    last_oled_update = now;

    const char *mode_str = "IDLE";
    switch (g_status.mode) {
    case MODE_IDLE:             mode_str = "IDLE"; break;
    case MODE_PASSTHROUGH:      mode_str = "PASS"; break;
    case MODE_CAPTURE_ONLY:     mode_str = "CAPT"; break;
    case MODE_INJECT_ONLY:      mode_str = "INJ "; break;
    case MODE_FULL_MITM:        mode_str = "MITM"; break;
    case MODE_CAPTURE_LOW_POWER: mode_str = "LO-P"; break;
    case MODE_DIAGNOSTIC:       mode_str = "DIAG"; break;
    }

    char link_str[8];
    link_str[0] = g_status.csi_link_up ? 'C' : '-';
    link_str[1] = g_status.dsi_link_up ? 'D' : '-';
    link_str[2] = g_status.ble_connected ? 'B' : '-';
    link_str[3] = g_status.usb_connected ? 'U' : '-';
    link_str[4] = '\0';

    oled_draw_status(mode_str, link_str, g_status.battery_pct,
                     g_status.capture.frames_captured, g_status.sd_present);
}

/* ---- Main entry point ---- */
int main(void)
{
    /* Initialize globals */
    g_boot_stage = BOOT_STAGE_INIT;
    g_status.mode = MODE_IDLE;
    g_status.battery_pct = 100;
    g_status.battery_mv = 4200;
    g_status.sd_present = 0;
    g_status.ble_connected = 0;
    g_status.usb_connected = 0;
    g_status.fpga_ready = 0;
    g_status.csi_link_up = 0;
    g_status.dsi_link_up = 0;
    g_status.uptime_ms = 0;
    g_status.capture.active = 0;
    g_status.capture.format = FMT_RAW10;
    g_status.capture.width = 1920;
    g_status.capture.height = 1080;
    g_status.capture.interval_ms = 0;
    g_status.capture.max_frames = 0;
    g_status.capture.frames_captured = 0;
    g_status.capture.bytes_written = 0;
    g_status.capture.jpeg_compress = 0;
    g_status.inject.active = 0;
    g_status.inject.mode = INJECT_FULL_REPLACE;
    g_status.inject.frame_count = 0;
    g_status.timing.enabled = 0;
    g_status.timing.delay_ns = 0;
    g_status.timing.jitter_pct = 0;
    g_status.timing.drop_pattern = 0;
    g_status.timing.frames_dropped = 0;

    /* ---- Boot sequence ---- */

    clock_init();
    gpio_init();
    leds_init();
    debug_uart_init();
    systick_init();

    debug_print("\r\n=== Prism-Tap Boot ===\r\n");
    debug_print("FW: v");
    debug_print_uint(FW_VERSION_MAJOR);
    debug_print(".");
    debug_print_uint(FW_VERSION_MINOR);
    debug_print(".");
    debug_print_uint(FW_VERSION_PATCH);
    debug_print("\r\n");

    /* Initialize I2C bus for MIPI bridges */
    g_boot_stage = BOOT_STAGE_BRIDGES;
    debug_print("I2C init...\r\n");
    bridge_i2c_init();

    /* Initialize FPGA SPI and load bitstream */
    g_boot_stage = BOOT_STAGE_FPGA_LOAD;
    debug_print("FPGA init...\r\n");
    if (fpga_init()) {
        debug_print("FPGA init FAILED!\r\n");
        /* Continue anyway — some features may work without FPGA */
    } else {
        debug_print("FPGA OK, loading bitstream from NOR...\r\n");
        if (fpga_load_from_nor()) {
            debug_print("FPGA bitstream load FAILED!\r\n");
        } else {
            debug_print("FPGA bitstream loaded\r\n");
            g_status.fpga_ready = 1;
        }
    }

    /* Initialize MIPI bridges with default config */
    debug_print("MIPI bridges init...\r\n");
    csi_config_t csi_default;
    csi_default.num_lanes = 2;
    csi_default.lane_speed_mbps = 1000;
    csi_default.width = 1920;
    csi_default.height = 1080;
    csi_default.pixel_format = FMT_RAW10;
    csi_default.pixel_clock_hz = 124416000;

    if (bridge_init_csi_rx(&csi_default) == 0)
        debug_print("CSI Rx OK\r\n");
    else
        debug_print("CSI Rx FAIL\r\n");

    if (bridge_init_csi_tx(&csi_default) == 0)
        debug_print("CSI Tx OK\r\n");
    else
        debug_print("CSI Tx FAIL\r\n");

    dsi_config_t dsi_default;
    dsi_default.num_lanes = 2;
    dsi_default.lane_speed_mbps = 800;
    dsi_default.width = 1080;
    dsi_default.height = 1920;
    dsi_default.pixel_format = FMT_RGB888;
    dsi_default.video_mode = 1;
    dsi_default.dcs_enabled = 1;

    if (bridge_init_dsi_rx(&dsi_default) == 0)
        debug_print("DSI Rx OK\r\n");
    else
        debug_print("DSI Rx FAIL\r\n");

    if (bridge_init_dsi_tx(&dsi_default) == 0)
        debug_print("DSI Tx OK\r\n");
    else
        debug_print("DSI Tx FAIL\r\n");

    /* Initialize SD card */
    g_boot_stage = BOOT_STAGE_SD;
    debug_print("SD card init...\r\n");
    if (sdcard_init() == 0 && sdcard_present()) {
        g_status.sd_present = 1;
        debug_print("SD OK, present\r\n");
    } else {
        debug_print("SD not present or init failed\r\n");
    }

    /* Initialize capture and injection engines */
    capture_init();
    inject_init();

    /* Initialize BLE C2 */
    g_boot_stage = BOOT_STAGE_BLE;
    debug_print("BLE init...\r\n");
    ble_init();

    /* Initialize command protocol */
    protocol_init();
    debug_print("Protocol registered\r\n");

    /* Initialize USB CDC */
    g_boot_stage = BOOT_STAGE_USB;
    debug_print("USB init...\r\n");
    usb_init();

    /* Initialize OLED display */
    g_boot_stage = BOOT_STAGE_OLED;
    debug_print("OLED init...\r\n");
    oled_init();
    oled_draw_status("BOOT", "----", 100, 0, 0);

    /* Initialize power management */
    g_boot_stage = BOOT_STAGE_INIT;
    debug_print("Power init...\r\n");
    power_init();

    /* Enable FPGA tap in passthrough mode */
    fpga_enable_tap(1);
    g_status.mode = MODE_PASSTHROUGH;

    /* Enable watchdog */
    wdt_init();

    g_boot_stage = BOOT_STAGE_DONE;
    debug_print("Boot complete!\r\n");

    /* ---- Main loop ---- */
    uint32_t loop_count = 0;
    while (1) {
        loop_count++;

        /* Service BLE commands */
        ble_poll();

        /* Service USB */
        usb_poll();

        /* Poll capture engine (reads frames from FPGA, writes to SD) */
        if (g_status.capture.active)
            capture_poll();

        /* Poll power management (battery, charging) */
        power_poll();

        /* Update status and OLED display */
        update_status();
        update_oled();

        /* Reload watchdog */
        if (loop_count % 100 == 0)
            wdt_reload();
    }

    return 0;  /* never reached */
}

/* ---- End of main.c ----
 * Author: jayis1
 */