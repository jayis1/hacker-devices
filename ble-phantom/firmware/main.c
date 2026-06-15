/*
 * main.c — BLE Phantom main firmware
 *
 * Dual-radio BLE security research platform.
 * STM32F401CCU6 orchestrates two nRF52832 radios via SPI
 * and exposes a USB CDC interface to the host application.
 */

#include "board.h"
#include "registers.h"
#include "spi_radio.h"
#include "usb_cdc.h"
#include <string.h>

/* ============================================================
 * Operating modes
 * ============================================================ */
typedef enum {
    MODE_IDLE = 0,
    MODE_SNIFF,
    MODE_SCAN,
    MODE_ADVERTISE,
    MODE_CONNECT,
    MODE_MITM,
    MODE_REPLAY
} device_mode_t;

/* ============================================================
 * Global state
 * ============================================================ */
static volatile device_mode_t current_mode = MODE_IDLE;
static radio_config_t config_a = {0};
static radio_config_t config_b = {0};
static volatile uint32_t systic_ms = 0;

/* ============================================================
 * Forward declarations
 * ============================================================ */
static void handle_host_command(const uint8_t *data, uint16_t len);
static void process_radio_a(void);
static void process_radio_b(void);
static void mitm_relay(void);
static void delay_ms(uint32_t ms);
static void update_leds(void);

/* ============================================================
 * System tick handler — 1 ms interval
 * ============================================================ */
void SysTick_Handler(void) {
    systic_ms++;
}

/* ============================================================
 * Simple delay using SysTick
 * ============================================================ */
static void delay_ms(uint32_t ms) {
    uint32_t start = systic_ms;
    while ((systic_ms - start) < ms);
}

/* ============================================================
 * Update status LEDs based on current mode
 * ============================================================ */
static void update_leds(void) {
    LED_RADIO_A_OFF();
    LED_RADIO_B_OFF();
    LED_USB_OFF();

    switch (current_mode) {
        case MODE_SNIFF:      LED_RADIO_A_ON(); break;
        case MODE_SCAN:       LED_RADIO_B_ON(); break;
        case MODE_ADVERTISE:  LED_RADIO_A_ON(); LED_RADIO_B_ON(); break;
        case MODE_CONNECT:    LED_RADIO_A_ON(); LED_USB_ON(); break;
        case MODE_MITM:       LED_RADIO_A_ON(); LED_RADIO_B_ON(); LED_USB_ON(); break;
        case MODE_REPLAY:     LED_USB_ON(); break;
        default: break;
    }
}

/* ============================================================
 * Host command handler (called from USB CDC ISR)
 * ============================================================ */
#define CMD_HOST_SET_MODE      0xA0
#define CMD_HOST_SET_CONFIG    0xA1
#define CMD_HOST_START         0xA2
#define CMD_HOST_STOP         0xA3
#define CMD_HOST_REPLAY       0xA4
#define CMD_HOST_STATUS       0xA5
#define CMD_HOST_GET_PACKET   0xA6
#define CMD_HOST_SET_ADV_DATA 0xA7
#define CMD_HOST_SET_CHAN      0xA8
#define CMD_HOST_SET_TX_PWR   0xA9

static void handle_host_command(const uint8_t *data, uint16_t len) {
    if (len < 1) return;

    uint8_t cmd = data[0];
    uint8_t response[64];
    uint16_t resp_len = 0;

    switch (cmd) {
        case CMD_HOST_SET_MODE: {
            if (len < 2) break;
            current_mode = (device_mode_t)data[1];
            response[0] = 0xA0;
            response[1] = 0x00;  /* OK */
            resp_len = 2;
            update_leds();
            break;
        }
        case CMD_HOST_SET_CONFIG: {
            if (len < 11) break;
            radio_id_t radio = (radio_id_t)data[1];
            radio_config_t *cfg = (radio == RADIO_A) ? &config_a : &config_b;
            cfg->channel = data[2];
            cfg->tx_power = (int8_t)data[3];
            cfg->adv_interval = (data[4] << 8) | data[5];
            cfg->scan_window = (data[6] << 8) | data[7];
            cfg->scan_interval = (data[8] << 8) | data[9];
            cfg->phy = data[10];
            spi_radio_configure(radio, cfg);
            response[0] = 0xA1;
            response[1] = 0x00;
            resp_len = 2;
            break;
        }
        case CMD_HOST_START: {
            if (len < 2) break;
            radio_id_t radio = (radio_id_t)data[1];
            if (current_mode == MODE_SNIFF || current_mode == MODE_SCAN) {
                spi_radio_start_scan(radio);
            } else if (current_mode == MODE_ADVERTISE) {
                spi_radio_start_adv(radio);
            }
            response[0] = 0xA2;
            response[1] = 0x00;
            resp_len = 2;
            break;
        }
        case CMD_HOST_STOP: {
            if (len < 2) break;
            radio_id_t radio = (radio_id_t)data[1];
            spi_radio_reset(radio);
            response[0] = 0xA3;
            response[1] = 0x00;
            resp_len = 2;
            break;
        }
        case CMD_HOST_REPLAY: {
            /* Replay stored packet — data[1] = radio, data[2..] = packet */
            if (len < 3) break;
            radio_id_t radio = (radio_id_t)data[1];
            spi_radio_send_data(radio, &data[2], len - 2);
            response[0] = 0xA4;
            response[1] = 0x00;
            resp_len = 2;
            break;
        }
        case CMD_HOST_STATUS: {
            response[0] = 0xA5;
            response[1] = (uint8_t)current_mode;
            response[2] = spi_radio_get_status(RADIO_A);
            response[3] = spi_radio_get_status(RADIO_B);
            response[4] = (systic_ms >> 0) & 0xFF;
            response[5] = (systic_ms >> 8) & 0xFF;
            response[6] = (systic_ms >> 16) & 0xFF;
            response[7] = (systic_ms >> 24) & 0xFF;
            resp_len = 8;
            break;
        }
        case CMD_HOST_SET_CHAN: {
            if (len < 3) break;
            radio_id_t radio = (radio_id_t)data[1];
            uint8_t chan = data[2];
            radio_config_t *cfg = (radio == RADIO_A) ? &config_a : &config_b;
            cfg->channel = chan;
            spi_radio_configure(radio, cfg);
            response[0] = 0xA8;
            response[1] = 0x00;
            resp_len = 2;
            break;
        }
        case CMD_HOST_SET_TX_PWR: {
            if (len < 3) break;
            radio_id_t radio = (radio_id_t)data[1];
            int8_t pwr = (int8_t)data[2];
            spi_radio_send_cmd(radio, CMD_SET_TX_POWER, (uint8_t *)&pwr, 1);
            response[0] = 0xA9;
            response[1] = 0x00;
            resp_len = 2;
            break;
        }
        default:
            response[0] = cmd;
            response[1] = 0xFF;  /* Unknown command */
            resp_len = 2;
            break;
    }

    if (resp_len > 0 && usb_cdc_is_configured()) {
        usb_cdc_send(response, resp_len);
    }
}

/* ============================================================
 * Process Radio A data (called from main loop)
 * ============================================================ */
static void process_radio_a(void) {
    uint8_t cmd;
    uint8_t payload[SPI_MAX_PAYLOAD];
    uint16_t len;

    int rc = spi_radio_read_response(RADIO_A, &cmd, payload, &len);
    if (rc == 0 && usb_cdc_is_configured()) {
        /* Forward to host with Radio A marker */
        uint8_t pkt[4 + SPI_MAX_PAYLOAD];
        pkt[0] = 0x01;  /* Radio A */
        pkt[1] = cmd;
        pkt[2] = (len >> 8) & 0xFF;
        pkt[3] = len & 0xFF;
        uint16_t copy_len = (len > SPI_MAX_PAYLOAD) ? SPI_MAX_PAYLOAD : len;
        memcpy(&pkt[4], payload, copy_len);
        usb_cdc_send(pkt, 4 + copy_len);
    }
}

/* ============================================================
 * Process Radio B data (called from main loop)
 * ============================================================ */
static void process_radio_b(void) {
    uint8_t cmd;
    uint8_t payload[SPI_MAX_PAYLOAD];
    uint16_t len;

    int rc = spi_radio_read_response(RADIO_B, &cmd, payload, &len);
    if (rc == 0 && usb_cdc_is_configured()) {
        uint8_t pkt[4 + SPI_MAX_PAYLOAD];
        pkt[0] = 0x02;  /* Radio B */
        pkt[1] = cmd;
        pkt[2] = (len >> 8) & 0xFF;
        pkt[3] = len & 0xFF;
        uint16_t copy_len = (len > SPI_MAX_PAYLOAD) ? SPI_MAX_PAYLOAD : len;
        memcpy(&pkt[4], payload, copy_len);
        usb_cdc_send(pkt, 4 + copy_len);
    }
}

/* ============================================================
 * MITM relay: forward packets between Radio A and Radio B
 * ============================================================ */
static void mitm_relay(void) {
    uint8_t cmd;
    uint8_t payload[SPI_MAX_PAYLOAD];
    uint16_t len;

    if (spi_radio_get_status(RADIO_A) & RADIO_STATUS_RX_PENDING) {
        if (spi_radio_read_response(RADIO_A, &cmd, payload, &len) == 0) {
            /* Modify packet if needed (future: GATT rewrite) */
            spi_radio_send_data(RADIO_B, payload, len);
            LED_RADIO_A_TOGGLE();
        }
    }

    if (spi_radio_get_status(RADIO_B) & RADIO_STATUS_RX_PENDING) {
        if (spi_radio_read_response(RADIO_B, &cmd, payload, &len) == 0) {
            spi_radio_send_data(RADIO_A, payload, len);
            LED_RADIO_B_TOGGLE();
        }
    }
}

/* ============================================================
 * System clock initialization
 * ============================================================ */
static void system_clock_init(void) {
    /* Enable HSE (8 MHz external crystal) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL: HSE/8 * 336 / 4 = 84 MHz, /7 for USB 48 MHz */
    RCC->PLLCFGR = (8 << 0)               /* PLLM = 8 */
                  | (336 << 6)             /* PLLN = 336 */
                  | (0 << 16)              /* PLLP = 2 (encoded as 0) */
                  | (7 << 24)              /* PLLQ = 7 */
                  | RCC_PLLCFGR_PLLSRC_HSE;

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Flash: 2 wait states for 84 MHz */
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_2WS;

    /* Switch to PLL: AHB=/1, APB1=/4, APB2=/2 */
    RCC->CFGR = (0 << 0)      /* SW = PLL */
              | (0 << 4)      /* AHB = /1 */
              | (0b101 << 10) /* APB1 = /4 */
              | (0b100 << 13);/* APB2 = /2 */

    /* Wait for PLL to be system clock */
    while ((RCC->CFGR & (3U << 2)) != (2U << 2));
}

/* ============================================================
 * GPIO initialization
 * ============================================================ */
static void gpio_init(void) {
    /* Enable all GPIO clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;

    /* PA0: Radio A IRQ — input, pull-up */
    GPIOA->MODER &= ~(3U << (PIN_RADIO_A_IRQ * 2));
    GPIOA->PUPDR |= (1U << (PIN_RADIO_A_IRQ * 2));

    /* PA1: Radio B IRQ — input, pull-up */
    GPIOA->MODER &= ~(3U << (PIN_RADIO_B_IRQ * 2));
    GPIOA->PUPDR |= (1U << (PIN_RADIO_B_IRQ * 2));

    /* PA4: Radio A RESET — output, push-pull, fast */
    GPIOA->MODER |= (1U << (PIN_RADIO_A_RST * 2));
    GPIOA->OTYPER &= ~(1U << PIN_RADIO_A_RST);
    GPIOA->OSPEEDR |= (2U << (PIN_RADIO_A_RST * 2));
    RESET_A_DEASSERT();

    /* PA5/PA6/PA7: SPI1 — AF5, fast */
    GPIOA->MODER |= (2U << (5 * 2)) | (2U << (6 * 2)) | (2U << (7 * 2));
    GPIOA->AFRL |= (5U << (5 * 4)) | (5U << (6 * 4)) | (5U << (7 * 4));
    GPIOA->OSPEEDR |= (2U << (5 * 2)) | (2U << (6 * 2)) | (2U << (7 * 2));

    /* PA8: Radio A CS — output, push-pull */
    GPIOA->MODER |= (1U << (PIN_RADIO_A_CS * 2));
    GPIOA->OTYPER &= ~(1U << PIN_RADIO_A_CS);
    GPIOA->OSPEEDR |= (2U << (PIN_RADIO_A_CS * 2));
    CS_A_DEASSERT();

    /* PA9: VBUS detect — input */
    GPIOA->MODER &= ~(3U << (PIN_VBUS_DETECT * 2));

    /* PA11/PA12: USB — AF10 (OTG_FS), very high speed */
    GPIOA->MODER |= (2U << (11 * 2)) | (2U << (12 * 2));
    GPIOA->AFRH |= (10U << ((11 - 8) * 4)) | (10U << ((12 - 8) * 4));
    GPIOA->OSPEEDR |= (3U << (11 * 2)) | (3U << (12 * 2));

    /* PB0: LED Radio A — output */
    GPIOB->MODER |= (1U << (PIN_LED_RADIO_A * 2));

    /* PB1: LED Radio B — output */
    GPIOB->MODER |= (1U << (PIN_LED_RADIO_B * 2));

    /* PB2: Radio B RESET — output, push-pull */
    GPIOB->MODER |= (1U << (PIN_RADIO_B_RST * 2));
    GPIOB->OTYPER &= ~(1U << PIN_RADIO_B_RST);
    GPIOB->OSPEEDR |= (2U << (PIN_RADIO_B_RST * 2));
    RESET_B_DEASSERT();

    /* PB10: SPI2_SCK — AF5 */
    GPIOB->MODER |= (2U << (10 * 2));
    GPIOB->AFRH |= (5U << ((10 - 8) * 4));
    GPIOB->OSPEEDR |= (2U << (10 * 2));

    /* PB11: SPI2_MISO — AF5 */
    GPIOB->MODER |= (2U << (11 * 2));
    GPIOB->AFRH |= (5U << ((11 - 8) * 4));

    /* PB12: Radio B CS — output */
    GPIOB->MODER |= (1U << (PIN_RADIO_B_CS * 2));
    GPIOB->OTYPER &= ~(1U << PIN_RADIO_B_CS);
    GPIOB->OSPEEDR |= (2U << (PIN_RADIO_B_CS * 2));
    CS_B_DEASSERT();

    /* PB15: SPI2_MOSI — AF5 */
    GPIOB->MODER |= (2U << (15 * 2));
    GPIOB->AFRH |= (5U << ((15 - 8) * 4));
    GPIOB->OSPEEDR |= (2U << (15 * 2));

    /* PB5/PB6: I2C1 — AF4, open-drain */
    GPIOB->MODER |= (2U << (5 * 2)) | (2U << (6 * 2));
    GPIOB->AFRL |= (4U << (5 * 4)) | (4U << (6 * 4));
    GPIOB->OTYPER |= (1U << 5) | (1U << 6);
    GPIOB->OSPEEDR |= (1U << (5 * 2)) | (1U << (6 * 2));

    /* PC13: User button — input, pull-up */
    GPIOC->MODER &= ~(3U << (PIN_USER_BUTTON * 2));
    GPIOC->PUPDR |= (1U << (PIN_USER_BUTTON * 2));

    /* PC14: LED USB — output */
    GPIOC->MODER |= (1U << (PIN_LED_USB * 2));

    /* PC15: LED Power — output */
    GPIOC->MODER |= (1U << (PIN_LED_POWER * 2));
}

/* ============================================================
 * EXTI configuration for Radio IRQs
 * ============================================================ */
static void exti_init(void) {
    /* Enable SYSCFG clock */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* Map PA0 to EXTI0 (Radio A IRQ) */
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0x000FU) | 0x0000U; /* PA0 */

    /* Map PA1 to EXTI1 (Radio B IRQ) */
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~0x00F0U) | 0x0000U; /* PA1 */

    /* Enable EXTI lines 0 and 1 */
    EXTI->IMR |= (1U << 0) | (1U << 1);

    /* Falling edge trigger for both */
    EXTI->FTSR |= (1U << 0) | (1U << 1);
    EXTI->RTSR &= ~((1U << 0) | (1U << 1));

    /* Enable NVIC interrupts */
    NVIC->ISER[EXTI0_IRQn >> 5] |= (1U << (EXTI0_IRQn & 0x1F));
    NVIC->ISER[EXTI1_IRQn >> 5] |= (1U << (EXTI1_IRQn & 0x1F));
}

/* ============================================================
 * SPI initialization
 * ============================================================ */
static void spi_init(void) {
    /* Enable SPI clocks */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    /* Configure SPI1 (Radio A): Master, Mode 0, 8-bit, no CRC */
    SPI1->CR1 = SPI_CR1_MSTR            /* Master mode */
              | (SPI_CR1_BR_SHIFT + SPI_CR1_BR_FOR_8MHZ)  /* Baud rate */
              | SPI_CR1_SSM              /* Software NSS */
              | SPI_CR1_SSI;             /* Internal NSS high */
    SPI1->CR2 = 0;
    SPI1->CR1 |= SPI_CR1_SPE;            /* Enable SPI1 */

    /* Configure SPI2 (Radio B): same settings */
    SPI2->CR1 = SPI_CR1_MSTR
              | (SPI_CR1_BR_SHIFT + SPI_CR1_BR_FOR_8MHZ)
              | SPI_CR1_SSM
              | SPI_CR1_SSI;
    SPI2->CR2 = 0;
    SPI2->CR1 |= SPI_CR1_SPE;            /* Enable SPI2 */
}

/* ============================================================
 * Main entry point
 * ============================================================ */
int main(void) {
    /* Initialize clocks and peripherals */
    system_clock_init();
    gpio_init();
    exti_init();
    spi_init();

    /* Initialize SysTick for 1 ms interval at 84 MHz */
    SYSTICK->RVR = 84000U - 1;   /* 84 MHz / 84000 = 1 kHz */
    SYSTICK->CVR = 0;
    SYSTICK->CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;

    /* Initialize USB CDC */
    usb_cdc_init();
    usb_cdc_set_rx_callback(handle_host_command);

    /* Initialize SPI radios */
    spi_radio_init(RADIO_A);
    spi_radio_init(RADIO_B);

    /* Default configuration */
    config_a.channel = 37;       /* BLE advertising channel 37 */
    config_a.tx_power = 4;        /* +4 dBm */
    config_a.adv_interval = 160;  /* 100 ms */
    config_a.scan_window = 16;    /* 10 ms */
    config_a.scan_interval = 16;
    config_a.phy = 1;             /* 1 Mbps */

    config_b.channel = 38;       /* BLE advertising channel 38 */
    config_b.tx_power = 4;
    config_b.adv_interval = 160;
    config_b.scan_window = 16;
    config_b.scan_interval = 16;
    config_b.phy = 1;

    spi_radio_configure(RADIO_A, &config_a);
    spi_radio_configure(RADIO_B, &config_b);

    /* Turn on power LED */
    LED_POWER_ON();

    /* Main event loop */
    while (1) {
        /* Process Radio A if data pending */
        if (spi_radio_get_status(RADIO_A) & RADIO_STATUS_RX_PENDING) {
            process_radio_a();
        }

        /* Process Radio B if data pending */
        if (spi_radio_get_status(RADIO_B) & RADIO_STATUS_RX_PENDING) {
            process_radio_b();
        }

        /* MITM relay mode */
        if (current_mode == MODE_MITM) {
            mitm_relay();
        }

        /* Button press: cycle through modes */
        static uint32_t last_press = 0;
        if (BUTTON_PRESSED() && (systic_ms - last_press > 300)) {
            last_press = systic_ms;
            current_mode = (device_mode_t)((current_mode + 1) % 7);
            update_leds();

            /* Acknowledge mode change to host */
            if (usb_cdc_is_configured()) {
                uint8_t resp[2] = {0xB0, (uint8_t)current_mode};
                usb_cdc_send(resp, 2);
            }
        }
    }

    return 0; /* Never reached */
}

/* ============================================================
 * Interrupt handlers
 * ============================================================ */

void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1U << 0)) {
        EXTI->PR = (1U << 0);
        spi_radio_irq_handler(RADIO_A);
    }
}

void EXTI1_IRQHandler(void) {
    if (EXTI->PR & (1U << 1)) {
        EXTI->PR = (1U << 1);
        spi_radio_irq_handler(RADIO_B);
    }
}

void OTG_FS_IRQHandler(void) {
    usb_cdc_isr();
}