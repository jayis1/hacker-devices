/**
 * @file main.c
 * @brief CAN Creeper main application — dual CAN FD sniffer/injector firmware
 *
 * Full system initialization, peripheral setup, main event loop,
 * and error handling for the CAN Creeper device.
 *
 * MCU: nRF52840-QIAA-R7 (Cortex-M4F @ 64 MHz)
 * SoftDevice: S140 v7.3.0 (BLE 5.2)
 * SDK: nRF5 SDK 17.1.0
 *
 * Architecture:
 *   - Dual MCP2518FD CAN FD controllers on SPI0/SPI1
 *   - APS6404L 8MB QSPI PSRAM for frame ring buffers
 *   - W25Q128JV 16MB QSPI NOR Flash for DBC/script storage
 *   - BLE 5.2 NUS for wireless control
 *   - USB CDC-ACM for wired control
 *   - SSD1306 128x64 OLED for local display
 *   - 1 MHz TIMER0 for µs-precision frame timestamps
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/can_fd_driver.h"
#include "drivers/psram_driver.h"
#include "drivers/ble_nus_driver.h"
#include "drivers/usb_cdc_driver.h"
#include "drivers/oled_driver.h"
#include "drivers/protocol_handler.h"
#include "drivers/injection_engine.h"

/*===========================================================================
 * GLOBAL STATE
 *===========================================================================*/

typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_SNIFFING,
    STATE_INJECTING,
    STATE_MITM,
    STATE_ERROR,
    STATE_SLEEP
} system_state_t;

static system_state_t g_system_state = STATE_INIT;
static volatile uint32_t g_uptime_seconds = 0;
static volatile bool g_ble_connected = false;
static volatile bool g_usb_connected = false;
static volatile uint32_t g_can0_frame_count = 0;
static volatile uint32_t g_can1_frame_count = 0;
static volatile uint32_t g_can0_overflow_count = 0;
static volatile uint32_t g_can1_overflow_count = 0;

/* Ring buffer instances */
static ring_buffer_t g_can0_ring;
static ring_buffer_t g_can1_ring;

/* CAN configuration */
static can_fd_config_t g_can0_config;
static can_fd_config_t g_can1_config;

/* Protocol handler state */
static protocol_state_t g_protocol_state;

/* Injection engine */
static injection_script_t g_active_script;

/* Error log (retained across soft resets) */
__attribute__((section(".noinit"))) static error_log_t g_error_log;

/*===========================================================================
 * FORWARD DECLARATIONS
 *===========================================================================*/

static void system_init(void);
static void peripherals_init(void);
static void can_init(void);
static void memory_init(void);
static void comms_init(void);
static void display_init(void);
static void timers_init(void);
static void interrupts_init(void);
static void main_loop(void);
static void process_can_frame(uint8_t channel, const can_frame_t *frame);
static void handle_ble_rx(const uint8_t *data, uint16_t len);
static void handle_usb_rx(const uint8_t *data, uint16_t len);
static void update_display(void);
static void check_power_state(void);
static void error_handler(uint16_t error_code, uint16_t error_data);
static void wdt_kick(void);

/*===========================================================================
 * INTERRUPT HANDLERS
 *===========================================================================*/

/**
 * @brief GPIOTE interrupt handler — CAN frame arrival detection
 *
 * Triggered by falling edge on MCP2518FD nINT pins.
 * Captures timestamp immediately, then defers frame reading to SWI1.
 */
void GPIOTE_IRQHandler(void) {
    /* Check CAN0 interrupt */
    if (NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_CAN0_INT]) {
        NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_CAN0_INT] = 0;
        /* Capture timestamp immediately for accuracy */
        NRF_TIMER0->TASKS_CAPTURE[0] = 1;
        /* Trigger deferred processing via SWI */
        NRF_SWI1->TASKS_TRIGGER = 1;
    }

    /* Check CAN1 interrupt */
    if (NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_CAN1_INT]) {
        NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_CAN1_INT] = 0;
        NRF_TIMER0->TASKS_CAPTURE[1] = 1;
        NRF_SWI1->TASKS_TRIGGER = 1;
    }

    /* Check user button */
    if (NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_BTN_USER]) {
        NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_BTN_USER] = 0;
        /* Debounce handled in main loop */
    }
}

/**
 * @brief SWI1 interrupt handler — deferred CAN frame processing
 *
 * Reads frames from MCP2518FD via SPI, stores to PSRAM ring buffer,
 * and queues for BLE/USB transmission.
 */
void SWI1_IRQHandler(void) {
    can_frame_t frame;
    uint32_t timestamp;

    /* Process CAN0 */
    if (NRF_TIMER0->EVENTS_COMPARE[0]) {
        NRF_TIMER0->EVENTS_COMPARE[0] = 0;
        timestamp = NRF_TIMER0->CC[0];  /* Captured timestamp */

        if (can_fd_receive(0, &frame) == 0) {
            frame.timestamp = timestamp;
            process_can_frame(0, &frame);
        }
    }

    /* Process CAN1 */
    if (NRF_TIMER0->EVENTS_COMPARE[1]) {
        NRF_TIMER0->EVENTS_COMPARE[1] = 0;
        timestamp = NRF_TIMER0->CC[1];

        if (can_fd_receive(1, &frame) == 0) {
            frame.timestamp = timestamp;
            process_can_frame(1, &frame);
        }
    }
}

/**
 * @brief TIMER0 interrupt handler — µs timestamp overflow
 */
void TIMER0_IRQHandler(void) {
    /* TIMER0 is free-running at 1 MHz, wraps every ~71 minutes.
     * We track wraps in software for extended timestamps. */
    NRF_TIMER0->EVENTS_COMPARE[0] = 0;
    /* No action needed — just acknowledge */
}

/**
 * @brief RTC0 interrupt handler — 1-second system tick
 */
void RTC0_IRQHandler(void) {
    NRF_RTC0->EVENTS_TICK = 0;
    g_uptime_seconds++;
}

/**
 * @brief WDT interrupt handler — watchdog timeout (should never fire)
 */
void WDT_IRQHandler(void) {
    /* If we get here, the system hung. Log the error and let WDT reset. */
    if (g_error_log.magic == ERROR_LOG_MAGIC) {
        g_error_log.entries[g_error_log.count % ERROR_LOG_MAX].timestamp = g_uptime_seconds;
        g_error_log.entries[g_error_log.count % ERROR_LOG_MAX].code = ERROR_WDT_TIMEOUT;
        g_error_log.entries[g_error_log.count % ERROR_LOG_MAX].data = 0;
        g_error_log.count++;
    }
    /* WDT will reset the system after this handler returns */
}

/*===========================================================================
 * SYSTEM INITIALIZATION
 *===========================================================================*/

/**
 * @brief Full system initialization sequence
 */
static void system_init(void) {
    /* 1. Configure power management */
    board_power_init();

    /* 2. Start clocks */
    board_clock_init();

    /* 3. Initialize GPIO */
    board_gpio_init();

    /* 4. Initialize error log */
    if (g_error_log.magic != ERROR_LOG_MAGIC) {
        memset(&g_error_log, 0, sizeof(g_error_log));
        g_error_log.magic = ERROR_LOG_MAGIC;
    }

    /* 5. Initialize watchdog */
    wdt_init();

    /* 6. Initialize timers */
    timers_init();

    /* 7. Initialize interrupts */
    interrupts_init();

    /* 8. Initialize memory (PSRAM + Flash) */
    memory_init();

    /* 9. Initialize CAN controllers */
    can_init();

    /* 10. Initialize display */
    display_init();

    /* 11. Initialize communication (BLE + USB) */
    comms_init();

    /* 12. Initialize protocol handler */
    protocol_init(&g_protocol_state);

    /* 13. Initialize injection engine */
    injection_init();

    g_system_state = STATE_IDLE;
}

/**
 * @brief Initialize clocks
 */
static void board_clock_init(void) {
    /* Start 32 MHz HF crystal */
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (!NRF_CLOCK->EVENTS_HFCLKSTARTED) {
        /* Wait for crystal to stabilize (~2ms) */
    }
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

    /* Start 32.768 kHz LF crystal */
    NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (!NRF_CLOCK->EVENTS_LFCLKSTARTED) {
        /* Wait for LF crystal (~500ms) */
    }
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

    /* Calibrate LFCLK every 4 seconds */
    NRF_CLOCK->CTIV = 16;  /* 16 × 0.25s = 4s */
    NRF_CLOCK->TASKS_CAL = 1;
}

/**
 * @brief Initialize power management
 */
static void board_power_init(void) {
    /* Enable DC/DC converter for power efficiency */
    NRF_POWER->DCDCEN = 1;
    /* Wait for DC/DC to stabilize */
    while (!NRF_POWER->DCDCEN0) {
        /* DCDCEN0 is set when DC/DC is ready */
    }

    /* Configure main regulator for 3.3V output */
    /* (Already set by UICR REGOUT0 = 0xFFFFFFF5 for 3.3V) */

    /* Enable power fail comparator (warn at ~2.8V) */
    NRF_POWER->POFCON = (POWER_POFCON_THRESHOLD_V28 << POWER_POFCON_THRESHOLD_Pos) |
                        (POWER_POFCON_POF_Enabled << POWER_POFCON_POF_Pos);
}

/**
 * @brief Initialize GPIO pins
 */
static void board_gpio_init(void) {
    /* CAN0 SPI pins */
    NRF_P0->PIN_CNF[BOARD_PIN_SPI0_CSN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P0->PIN_CNF[BOARD_PIN_SPI0_MISO] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                                             (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);
    NRF_P0->PIN_CNF[BOARD_PIN_SPI0_MOSI] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                             (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P0->PIN_CNF[BOARD_PIN_SPI0_SCK] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);

    /* CAN1 SPI pins */
    NRF_P0->PIN_CNF[BOARD_PIN_SPI1_MOSI] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                             (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P0->PIN_CNF[BOARD_PIN_SPI1_MISO] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                                             (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);
    NRF_P0->PIN_CNF[BOARD_PIN_SPI1_SCK] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P0->PIN_CNF[BOARD_PIN_SPI1_CSN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);

    /* CAN interrupt inputs (sense high-to-low for active-low nINT) */
    NRF_P0->PIN_CNF[BOARD_PIN_CAN0_INT] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                                            (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
    NRF_P0->PIN_CNF[BOARD_PIN_CAN1_INT] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                                            (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);

    /* CAN transceiver control */
    NRF_P1->PIN_CNF[CAN0_STB_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                      (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P1->PIN_CNF[CAN1_STB_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                      (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P1->PIN_CNF[CAN0_EN_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                     (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P1->PIN_CNF[CAN1_EN_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                     (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);

    /* Termination FET control */
    NRF_P1->PIN_CNF[CAN0_TERM_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                       (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P1->PIN_CNF[CAN1_TERM_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                       (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);

    /* LEDs */
    NRF_P0->PIN_CNF[LED_RED_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                    (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P1->PIN_CNF[LED_GRN_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                     (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P1->PIN_CNF[LED_BLU_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                     (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);

    /* Button */
    NRF_P1->PIN_CNF[BTN_USER_PIN] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                                      (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
                                      (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);

    /* Battery voltage sense (analog, disconnect input buffer) */
    NRF_P0->PIN_CNF[VBAT_SENSE_PIN] = (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
                                        (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

    /* QSPI CS pins (initial high = inactive) */
    NRF_P0->PIN_CNF[QSPI_CSN_PSRAM_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);
    NRF_P0->PIN_CNF[QSPI_CSN_FLASH_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                            (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos);

    /* Set initial output states */
    NRF_P0->OUTSET = (1UL << BOARD_PIN_SPI0_CSN) | (1UL << BOARD_PIN_SPI1_CSN) |
                     (1UL << QSPI_CSN_PSRAM_PIN) | (1UL << QSPI_CSN_FLASH_PIN);
    NRF_P1->OUTCLR = (1UL << CAN0_STB_PIN) | (1UL << CAN1_STB_PIN);   /* STB low = normal */
    NRF_P1->OUTSET = (1UL << CAN0_EN_PIN) | (1UL << CAN1_EN_PIN);     /* EN high = enabled */
    NRF_P1->OUTCLR = (1UL << CAN0_TERM_PIN) | (1UL << CAN1_TERM_PIN); /* Term off */
    LED_RED_OFF();
    LED_GRN_OFF();
    LED_BLU_OFF();
}

/**
 * @brief Initialize timers
 */
static void timers_init(void) {
    /* TIMER0: 1 MHz µs timestamp counter, 32-bit, free-running */
    NRF_TIMER0->MODE = TIMER_MODE_TIMER;
    NRF_TIMER0->BITMODE = TIMER_BITMODE_32BIT;
    NRF_TIMER0->PRESCALER = TIMER_PRESCALER_DIV16;  /* 16 MHz / 16 = 1 MHz */
    NRF_TIMER0->TASKS_CLEAR = 1;
    NRF_TIMER0->TASKS_START = 1;

    /* TIMER1: Injection scheduler (configured by injection engine) */
    NRF_TIMER1->MODE = TIMER_MODE_TIMER;
    NRF_TIMER1->BITMODE = TIMER_BITMODE_32BIT;
    NRF_TIMER1->PRESCALER = TIMER_PRESCALER_DIV16;  /* 1 MHz for µs precision */

    /* TIMER2: Watchdog kick timer (1 second interval) */
    NRF_TIMER2->MODE = TIMER_MODE_TIMER;
    NRF_TIMER2->BITMODE = TIMER_BITMODE_32BIT;
    NRF_TIMER2->PRESCALER = TIMER_PRESCALER_DIV16;
    NRF_TIMER2->CC[0] = 1000000;  /* 1 second at 1 MHz */
    NRF_TIMER2->INTENSET = (1UL << 16);  /* COMPARE0 interrupt */
    NRF_TIMER2->TASKS_START = 1;

    /* RTC0: System tick at 1 Hz */
    NRF_RTC0->PRESCALER = 32767;  /* 32768 / (32767+1) = 1 Hz */
    NRF_RTC0->EVTEN = (1UL << 0);  /* TICK event enabled */
    NRF_RTC0->INTENSET = (1UL << 0);  /* TICK interrupt */
    NRF_RTC0->TASKS_START = 1;
}

/**
 * @brief Initialize NVIC interrupts
 */
static void interrupts_init(void) {
    /* Set priorities (lower number = higher priority) */
    NVIC_SET_PRIORITY(NVIC_IRQ_RADIO, 0);       /* BLE SoftDevice — highest */
    NVIC_SET_PRIORITY(NVIC_IRQ_TIMER0, 1);      /* Timestamp capture */
    NVIC_SET_PRIORITY(NVIC_IRQ_GPIOTE, 2);      /* CAN INT detection */
    NVIC_SET_PRIORITY(NVIC_IRQ_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0, 3); /* SPI */
    NVIC_SET_PRIORITY(NVIC_IRQ_SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1, 3);
    NVIC_SET_PRIORITY(NVIC_IRQ_QSPI, 4);        /* QSPI */
    NVIC_SET_PRIORITY(NVIC_IRQ_USBD, 5);        /* USB */
    NVIC_SET_PRIORITY(NVIC_IRQ_RTC0, 6);        /* System tick */
    NVIC_SET_PRIORITY(NVIC_IRQ_SWI0, 2);        /* SD callback */
    NVIC_SET_PRIORITY(NVIC_IRQ_SWI1, 3);        /* Deferred CAN processing */

    /* Enable interrupts */
    NVIC_ENABLE_IRQ(NVIC_IRQ_GPIOTE);
    NVIC_ENABLE_IRQ(NVIC_IRQ_TIMER0);
    NVIC_ENABLE_IRQ(NVIC_IRQ_RTC0);
    NVIC_ENABLE_IRQ(NVIC_IRQ_SWI1);
    NVIC_ENABLE_IRQ(NVIC_IRQ_WDT);
}

/**
 * @brief Initialize CAN controllers
 */
static void can_init(void) {
    /* Default CAN0 config: 500 kbps nominal, 4 Mbps FD data, normal mode */
    g_can0_config.channel = 0;
    g_can0_config.nominal_brp = 0;
    g_can0_config.nominal_tseg1 = 55;   /* PropSeg + PhaseSeg1 - 1 */
    g_can0_config.nominal_tseg2 = 23;   /* PhaseSeg2 - 1 */
    g_can0_config.nominal_sjw = 7;      /* SJW - 1 */
    g_can0_config.data_brp = 0;
    g_can0_config.data_tseg1 = 5;
    g_can0_config.data_tseg2 = 2;
    g_can0_config.data_sjw = 1;
    g_can0_config.fd_enable = true;
    g_can0_config.brs_enable = true;
    g_can0_config.listen_only = false;

    /* Default CAN1 config: same as CAN0 */
    memcpy(&g_can1_config, &g_can0_config, sizeof(can_fd_config_t));
    g_can1_config.channel = 1;

    /* Initialize both CAN controllers */
    if (can_fd_init(0, &g_can0_config) != 0) {
        error_handler(ERROR_CAN0_INIT_FAILED, 0);
    }
    if (can_fd_init(1, &g_can1_config) != 0) {
        error_handler(ERROR_CAN1_INIT_FAILED, 0);
    }

    /* Set up GPIOTE for CAN interrupt detection */
    NRF_GPIOTE->CONFIG[GPIOTE_CH_CAN0_INT] =
        (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
        (CAN0_INT_PIN << GPIOTE_CONFIG_PSEL_Pos) |
        (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos);
    NRF_GPIOTE->CONFIG[GPIOTE_CH_CAN1_INT] =
        (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
        (CAN1_INT_PIN << GPIOTE_CONFIG_PSEL_Pos) |
        (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos);

    /* GPIOTE for user button */
    NRF_GPIOTE->CONFIG[GPIOTE_CH_BTN_USER] =
        (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
        (BTN_USER_PIN << GPIOTE_CONFIG_PSEL_Pos) |
        (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos);

    /* Enable GPIOTE interrupts */
    NRF_GPIOTE->INTENSET = (1UL << GPIOTE_CH_CAN0_INT) |
                           (1UL << GPIOTE_CH_CAN1_INT) |
                           (1UL << GPIOTE_CH_BTN_USER);

    /* Enable CAN transceivers */
    CAN0_TRX_NORMAL();
    CAN1_TRX_NORMAL();
}

/**
 * @brief Initialize memory (PSRAM ring buffers + NOR Flash)
 */
static void memory_init(void) {
    int ret;

    /* Initialize QSPI peripheral */
    ret = psram_init();
    if (ret != 0) {
        error_handler(ERROR_PSRAM_INIT_FAILED, (uint16_t)ret);
        return;
    }

    /* Initialize CAN0 ring buffer in PSRAM */
    ret = psram_ring_init(&g_can0_ring, PSRAM_CH0_BUFFER_OFFSET, PSRAM_CH0_BUFFER_SIZE);
    if (ret != 0) {
        error_handler(ERROR_PSRAM_RING_INIT_FAILED, 0);
    }

    /* Initialize CAN1 ring buffer in PSRAM */
    ret = psram_ring_init(&g_can1_ring, PSRAM_CH1_BUFFER_OFFSET, PSRAM_CH1_BUFFER_SIZE);
    if (ret != 0) {
        error_handler(ERROR_PSRAM_RING_INIT_FAILED, 1);
    }

    /* Initialize NOR Flash (verify presence, leave in standby) */
    ret = flash_init();
    if (ret != 0) {
        error_handler(ERROR_FLASH_INIT_FAILED, (uint16_t)ret);
    }
}

/**
 * @brief Initialize communication interfaces (BLE + USB)
 */
static void comms_init(void) {
    ble_nus_config_t ble_config = {
        .rx_callback = handle_ble_rx,
        .tx_complete_callback = NULL,
        .max_tx_size = BLE_NUS_MAX_DATA_LEN,
        .max_rx_size = BLE_NUS_MAX_DATA_LEN,
    };

    usb_cdc_config_t usb_config = {
        .rx_callback = handle_usb_rx,
        .tx_complete_callback = NULL,
        .state_callback = NULL,
    };

    /* Initialize BLE NUS */
    if (ble_nus_init(&ble_config) != 0) {
        error_handler(ERROR_BLE_INIT_FAILED, 0);
    }

    /* Start BLE advertising */
    ble_nus_start_advertising();

    /* Initialize USB CDC-ACM */
    if (usb_cdc_init(&usb_config) != 0) {
        error_handler(ERROR_USB_INIT_FAILED, 0);
    }
}

/**
 * @brief Initialize OLED display
 */
static void display_init(void) {
    if (oled_init() != 0) {
        /* Non-fatal — device works without display */
        return;
    }

    oled_clear();
    oled_draw_string(0, 0, "CAN Creeper", OLED_FONT_8X16);
    oled_draw_string(0, 2, "Rev A  v1.0", OLED_FONT_5X7);
    oled_draw_string(0, 4, "Initializing...", OLED_FONT_5X7);
    oled_display();
}

/*===========================================================================
 * MAIN ENTRY POINT
 *===========================================================================*/

/**
 * @brief Application entry point
 *
 * Called by the SoftDevice after BLE stack initialization.
 * Never returns — runs infinite main loop.
 */
int main(void) {
    /* Full system initialization */
    system_init();

    /* Show ready state on display */
    oled_clear();
    oled_draw_string(0, 0, "CAN Creeper", OLED_FONT_8X16);
    oled_draw_string(0, 2, "Ready", OLED_FONT_5X7);
    oled_draw_string(0, 4, "BLE: Advertising", OLED_FONT_5X7);
    oled_display();

    LED_GRN_ON();  /* System ready */

    /* Main event loop */
    main_loop();

    /* Should never reach here */
    return 0;
}

/**
 * @brief Main event loop — processes frames, BLE/USB events, and manages state
 */
static void main_loop(void) {
    uint32_t last_display_update = 0;
    uint32_t last_power_check = 0;
    uint32_t last_wdt_kick = 0;
    uint32_t now;

    while (1) {
        now = g_uptime_seconds;

        /* Kick watchdog */
        if ((now - last_wdt_kick) >= 1) {
            wdt_kick();
            last_wdt_kick = now;
        }

        /* Process BLE SoftDevice events */
        /* (Handled by SD event dispatcher → ble_nus_event_handler) */

        /* Process USB events */
        /* (Handled by USBD interrupt → app_usbd event handler) */

        /* Process injection engine */
        if (g_active_script.running) {
            injection_process(&g_active_script);
        }

        /* Check for pending frames to transmit over BLE */
        protocol_flush_ble(&g_protocol_state);

        /* Check for pending frames to transmit over USB */
        protocol_flush_usb(&g_protocol_state);

        /* Update display periodically (every 500ms) */
        if ((now - last_display_update) >= 500) {
            update_display();
            last_display_update = now;
        }

        /* Check power state periodically (every 5 seconds) */
        if ((now - last_power_check) >= 5) {
            check_power_state();
            last_power_check = now;
        }

        /* Check user button for mode changes */
        if (BTN_IS_PRESSED()) {
            /* Simple debounce: wait and check again */
            for (volatile uint32_t d = 0; d < 100000; d++) { __NOP(); }
            if (BTN_IS_PRESSED()) {
                /* Toggle between sniffing and idle */
                if (g_system_state == STATE_IDLE) {
                    g_system_state = STATE_SNIFFING;
                    LED_GRN_OFF();
                    LED_RED_ON();
                } else if (g_system_state == STATE_SNIFFING) {
                    g_system_state = STATE_IDLE;
                    LED_RED_OFF();
                    LED_GRN_ON();
                }
                /* Wait for button release */
                while (BTN_IS_PRESSED()) { __NOP(); }
            }
        }

        /* Sleep until next event (WFE wakes on any interrupt) */
        __WFE();
    }
}

/*===========================================================================
 * FRAME PROCESSING
 *===========================================================================*/

/**
 * @brief Process a received CAN frame
 *
 * Stores frame in PSRAM ring buffer, updates counters,
 * and queues for BLE/USB transmission.
 *
 * @param channel CAN channel (0 or 1)
 * @param frame   Received CAN frame with timestamp
 */
static void process_can_frame(uint8_t channel, const can_frame_t *frame) {
    ring_buffer_t *ring;
    uint8_t entry[CAN_FRAME_ENTRY_SIZE];
    int ret;

    /* Select ring buffer */
    if (channel == 0) {
        ring = &g_can0_ring;
        g_can0_frame_count++;
    } else {
        ring = &g_can1_ring;
        g_can1_frame_count++;
    }

    /* Serialize frame to ring buffer entry format */
    /* Entry format: [0-3] timestamp, [4-7] ID+flags, [8] DLC+flags, [9-15] reserved, [16-31] data */
    entry[0] = (uint8_t)(frame->timestamp & 0xFF);
    entry[1] = (uint8_t)((frame->timestamp >> 8) & 0xFF);
    entry[2] = (uint8_t)((frame->timestamp >> 16) & 0xFF);
    entry[3] = (uint8_t)((frame->timestamp >> 24) & 0xFF);

    /* CAN ID + flags */
    uint32_t id_flags = frame->id & 0x1FFFFFFFUL;
    if (frame->flags & CAN_FLAG_IDE) id_flags |= (1UL << 29);
    if (frame->flags & CAN_FLAG_RTR) id_flags |= (1UL << 30);
    if (frame->flags & CAN_FLAG_FD)  id_flags |= (1UL << 31);
    entry[4] = (uint8_t)(id_flags & 0xFF);
    entry[5] = (uint8_t)((id_flags >> 8) & 0xFF);
    entry[6] = (uint8_t)((id_flags >> 16) & 0xFF);
    entry[7] = (uint8_t)((id_flags >> 24) & 0xFF);

    /* DLC + extended flags */
    uint8_t dlc_flags = frame->dlc & 0x0F;
    if (frame->flags & CAN_FLAG_BRS) dlc_flags |= (1 << 4);
    if (frame->flags & CAN_FLAG_ESI) dlc_flags |= (1 << 5);
    entry[8] = dlc_flags;

    /* Reserved bytes */
    memset(&entry[9], 0, 7);

    /* Data bytes (up to 16 in ring entry; CAN FD with >16 bytes uses next slot) */
    uint8_t data_len = (frame->dlc > 8) ? frame->dlc : 8;
    if (data_len > 16) data_len = 16;
    memcpy(&entry[16], frame->data, data_len);

    /* Write to ring buffer */
    ret = psram_ring_write(ring, entry, CAN_FRAME_ENTRY_SIZE);
    if (ret != 0) {
        /* Buffer full — increment overflow counter */
        if (channel == 0) {
            g_can0_overflow_count++;
        } else {
            g_can1_overflow_count++;
        }
    }

    /* Queue for BLE/USB transmission */
    protocol_queue_frame(&g_protocol_state, channel, frame);
}

/*===========================================================================
 * COMMUNICATION HANDLERS
 *===========================================================================*/

/**
 * @brief Handle received data from BLE NUS (app → device)
 *
 * @param data Received data buffer
 * @param len  Data length in bytes
 */
static void handle_ble_rx(const uint8_t *data, uint16_t len) {
    protocol_parse(&g_protocol_state, data, len, PROTOCOL_TRANSPORT_BLE);
}

/**
 * @brief Handle received data from USB CDC (host → device)
 *
 * @param data Received data buffer
 * @param len  Data length in bytes
 */
static void handle_usb_rx(const uint8_t *data, uint16_t len) {
    protocol_parse(&g_protocol_state, data, len, PROTOCOL_TRANSPORT_USB);
}

/*===========================================================================
 * DISPLAY UPDATE
 *===========================================================================*/

/**
 * @brief Update OLED display with current status
 */
static void update_display(void) {
    char buf[32];

    oled_clear();

    /* Line 0: Device name and state */
    oled_draw_string(0, 0, "CAN Creeper", OLED_FONT_8X16);

    /* Line 2: CAN0 status */
    snprintf(buf, sizeof(buf), "CH0: %lu fr", (unsigned long)g_can0_frame_count);
    oled_draw_string(0, 2, buf, OLED_FONT_5X7);

    /* Line 3: CAN1 status */
    snprintf(buf, sizeof(buf), "CH1: %lu fr", (unsigned long)g_can1_frame_count);
    oled_draw_string(0, 3, buf, OLED_FONT_5X7);

    /* Line 4: Connection status */
    if (g_ble_connected && g_usb_connected) {
        oled_draw_string(0, 4, "BLE+USB", OLED_FONT_5X7);
    } else if (g_ble_connected) {
        oled_draw_string(0, 4, "BLE only", OLED_FONT_5X7);
    } else if (g_usb_connected) {
        oled_draw_string(0, 4, "USB only", OLED_FONT_5X7);
    } else {
        oled_draw_string(0, 4, "No link", OLED_FONT_5X7);
    }

    /* Line 5: Battery voltage */
    uint16_t batt_mv = board_battery_read_mv();
    snprintf(buf, sizeof(buf), "BAT: %u.%02uV", batt_mv / 1000, (batt_mv % 1000) / 10);
    oled_draw_string(0, 5, buf, OLED_FONT_5X7);

    /* Line 6: Uptime */
    snprintf(buf, sizeof(buf), "Up: %lus", (unsigned long)g_uptime_seconds);
    oled_draw_string(0, 6, buf, OLED_FONT_5X7);

    /* Line 7: Overflow warning if any */
    if (g_can0_overflow_count > 0 || g_can1_overflow_count > 0) {
        snprintf(buf, sizeof(buf), "OVF:%lu/%lu",
                 (unsigned long)g_can0_overflow_count,
                 (unsigned long)g_can1_overflow_count);
        oled_draw_string(0, 7, buf, OLED_FONT_5X7);
    }

    oled_display();
}

/*===========================================================================
 * POWER MANAGEMENT
 *===========================================================================*/

/**
 * @brief Check power state and adjust operating mode
 */
static void check_power_state(void) {
    uint16_t batt_mv = board_battery_read_mv();

    if (batt_mv < BATTERY_CRITICAL_MV) {
        /* Critical battery — enter sleep mode */
        g_system_state = STATE_SLEEP;
        LED_RED_ON();
        LED_GRN_OFF();
        LED_BLU_OFF();

        /* Save state to retention RAM */
        NRF_POWER->GPREGRET = (uint32_t)g_system_state;

        /* Disable CAN transceivers */
        CAN0_TRX_SLEEP();
        CAN1_TRX_SLEEP();

        /* Stop advertising */
        ble_nus_stop_advertising();

        /* Enter system ON sleep */
        NRF_POWER->SYSTEMOFF = 1;
        /* System will wake on button press or charger connection */
    } else if (batt_mv < BATTERY_LOW_MV) {
        /* Low battery — reduce BLE TX power */
        ble_nus_set_tx_power(-8);  /* -8 dBm */
        LED_RED_ON();  /* Blink handled by GPIOTE task */
    } else {
        /* Normal — full power */
        ble_nus_set_tx_power(0);  /* 0 dBm */
    }
}

/*===========================================================================
 * ERROR HANDLING
 *===========================================================================*/

/**
 * @brief Handle system errors
 *
 * Logs error to retention RAM, updates display, and attempts recovery.
 *
 * @param error_code Error category code
 * @param error_data Additional error data
 */
static void error_handler(uint16_t error_code, uint16_t error_data) {
    /* Log to retention RAM */
    if (g_error_log.magic == ERROR_LOG_MAGIC) {
        uint32_t idx = g_error_log.count % ERROR_LOG_MAX;
        g_error_log.entries[idx].timestamp = g_uptime_seconds;
        g_error_log.entries[idx].code = error_code;
        g_error_log.entries[idx].data = error_data;
        g_error_log.count++;
    }

    /* Update state */
    g_system_state = STATE_ERROR;

    /* Visual indication */
    LED_RED_ON();
    LED_GRN_OFF();
    LED_BLU_OFF();

    /* Display error */
    char buf[32];
    oled_clear();
    oled_draw_string(0, 0, "ERROR", OLED_FONT_8X16);
    snprintf(buf, sizeof(buf), "Code: 0x%04X", error_code);
    oled_draw_string(0, 2, buf, OLED_FONT_5X7);
    snprintf(buf, sizeof(buf), "Data: 0x%04X", error_data);
    oled_draw_string(0, 3, buf, OLED_FONT_5X7);
    oled_draw_string(0, 5, "Reset to recover", OLED_FONT_5X7);
    oled_display();

    /* Attempt recovery based on error type */
    switch (error_code) {
        case ERROR_CAN0_INIT_FAILED:
        case ERROR_CAN1_INIT_FAILED:
            /* Try re-initializing the CAN controller */
            can_fd_deinit(error_code == ERROR_CAN0_INIT_FAILED ? 0 : 1);
            /* Will retry on next watchdog reset if persistent */
            break;

        case ERROR_PSRAM_INIT_FAILED:
        case ERROR_PSRAM_RING_INIT_FAILED:
            /* PSRAM failure is critical — operate without buffering */
            break;

        case ERROR_FLASH_INIT_FAILED:
            /* Non-critical — operate without DBC/script storage */
            break;

        case ERROR_BLE_INIT_FAILED:
            /* Retry BLE init */
            ble_nus_deinit();
            break;

        case ERROR_USB_INIT_FAILED:
            /* Retry USB init */
            usb_cdc_deinit();
            break;

        default:
            break;
    }

    /* If critical error, let WDT reset the system */
    if (error_code >= ERROR_CRITICAL_THRESHOLD) {
        /* Stop kicking watchdog — system will reset in WDT_TIMEOUT_SEC */
        while (1) {
            __WFE();
        }
    }
}

/*===========================================================================
 * WATCHDOG
 *===========================================================================*/

/**
 * @brief Initialize watchdog timer
 */
static void wdt_init(void) {
    NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos);
    NRF_WDT->CRV = BOARD_LFCLK_FREQ_HZ * WDT_TIMEOUT_SEC;  /* 32768 * 8 = 262144 */
    NRF_WDT->RREN = (1UL << WDT_RELOAD_CHANNEL);
    NRF_WDT->TASKS_START = 1;
}

/**
 * @brief Kick the watchdog (reload timer)
 */
static void wdt_kick(void) {
    NRF_WDT->RR[WDT_RELOAD_CHANNEL] = WDT_RR_RR_Reload;
}

/*===========================================================================
 * BATTERY MONITORING
 *===========================================================================*/

/**
 * @brief Read battery voltage using SAADC
 *
 * @return Battery voltage in millivolts
 */
uint16_t board_battery_read_mv(void) {
    /* Configure SAADC for single sample on channel 2 (AIN2) */
    NRF_SAADC->ENABLE = 1;

    /* Configure channel 2: AIN2 single-ended, gain 1/6, internal reference */
    NRF_SAADC->CH[2].PSELP = (VBAT_SENSE_ADC_CHANNEL << SAADC_CH_PSELP_PSELP_Pos);
    NRF_SAADC->CH[2].PSELN = 0;  /* Not used in SE mode */
    NRF_SAADC->CH[2].CONFIG =
        (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos) |
        (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
        (SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos) |
        (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos) |
        (SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos);

    /* 12-bit resolution */
    NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_12bit;

    /* Single sample buffer */
    static int16_t sample_buffer[1];
    NRF_SAADC->RESULT.PTR = (uint32_t)sample_buffer;
    NRF_SAADC->RESULT.MAXCNT = 1;

    /* Start sampling */
    NRF_SAADC->TASKS_START = 1;
    NRF_SAADC->TASKS_SAMPLE = 1;

    /* Wait for result */
    while (!NRF_SAADC->EVENTS_END) {
        /* Poll — could use interrupt but simple poll is fine for infrequent reads */
    }
    NRF_SAADC->EVENTS_END = 0;

    /* Stop SAADC */
    NRF_SAADC->TASKS_STOP = 1;
    while (!NRF_SAADC->EVENTS_STOPPED) {}
    NRF_SAADC->EVENTS_STOPPED = 0;

    NRF_SAADC->ENABLE = 0;

    /* Convert sample to mV */
    /* With gain 1/6 and internal 0.6V ref, input range = 0 to 3.6V */
    /* 12-bit: 4096 steps → each step = 3.6V / 4096 = ~0.879 mV */
    /* VBAT = sample * (3600 / 4096) * VBAT_DIVIDER_RATIO */
    int32_t mv = (int32_t)sample_buffer[0] * 3600 / 4096 * VBAT_DIVIDER_RATIO;
    if (mv < 0) mv = 0;

    return (uint16_t)mv;
}

/*===========================================================================
 * DEVICE ID
 *===========================================================================*/

/**
 * @brief Get unique device ID from FICR
 *
 * @param id_out Buffer to store 8-byte device ID
 */
void board_get_device_id(uint8_t id_out[8]) {
    uint32_t id0 = NRF_FICR->DEVICEID[0];
    uint32_t id1 = NRF_FICR->DEVICEID[1];

    id_out[0] = (uint8_t)(id0 & 0xFF);
    id_out[1] = (uint8_t)((id0 >> 8) & 0xFF);
    id_out[2] = (uint8_t)((id0 >> 16) & 0xFF);
    id_out[3] = (uint8_t)((id0 >> 24) & 0xFF);
    id_out[4] = (uint8_t)(id1 & 0xFF);
    id_out[5] = (uint8_t)((id1 >> 8) & 0xFF);
    id_out[6] = (uint8_t)((id1 >> 16) & 0xFF);
    id_out[7] = (uint8_t)((id1 >> 24) & 0xFF);
}
