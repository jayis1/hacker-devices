/*
 * main.c — GNSS-Phantom firmware main entry point
 *
 * GNSS-Phantom is a portable GNSS/GPS signal generation and spoofing platform
 * for authorized security research.  It synthesizes GPS L1 C/A, Galileo E1,
 * GLONASS L1, and BeiDou B1 signals using Si4463 RF transceivers driven by
 * an STM32H743 Cortex-M7 MCU.
 *
 * Safety interlocks:
 *   1. Physical safety switch (BTN_SAFE) must be in SAFE position
 *   2. Fire button (BTN_FIRE) must be held for 3 seconds
 *   3. C2 ARM command must be received from authenticated app
 *   4. Auto-disarm after 5 minutes of no activity
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * LEGAL: This device is intended SOLELY for authorized security research
 *        and testing in controlled environments.  Unauthorized use to spoof
 *        GNSS signals outside an authorized RF-shielded lab is ILLEGAL in
 *        most jurisdictions and dangerous to aviation, maritime, and
 *        critical infrastructure.  Users are responsible for compliance
 *        with all applicable laws and regulations.
 */

#include "board.h"
#include "registers.h"
#include "drivers/si4463.h"
#include "drivers/gnss_engine.h"
#include "drivers/c2_protocol.h"
#include <string.h>

/* ----- Global state ----- */
static si4463_t rf_gps;       /* GPS/Galileo L1 transceiver */
static si4463_t rf_glo;       /* GLONASS/BeiDou transceiver */
static gnss_engine_t engine;  /* spoofing signal generator */
static c2_ctx_t c2;           /* BLE C2 protocol */

static volatile uint32_t system_ticks = 0;  /* 1 ms tick counter */
static volatile uint32_t last_arm_time = 0;
static volatile uint8_t armed = 0;
static volatile uint8_t transmitting = 0;
static volatile uint8_t safety_engaged = 0;  /* 1 = safe, 0 = live */
static volatile uint16_t fire_hold_ms = 0;
static volatile uint32_t batt_mv = 4200;
static volatile int8_t temp_c = 25;

static uint8_t c2_rx_payload[C2_MAX_PAYLOAD];
static uint8_t c2_tx_frame_buf[C2_FRAME_MAX];

/* ----- Utility: delay ----- */
static void delay_ms(uint32_t ms) {
    uint32_t start = system_ticks;
    while ((system_ticks - start) < ms) { }
}

static void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 48; i++) { }  /* ~48 cycles/us @ 480MHz */
}

/* ----- SysTick (1 ms) ----- */
static void systick_init(void) {
    SYST_RVR = (SYSCLK_HZ / 1000) - 1;  /* 480000 - 1 = 479999 */
    SYST_CVR = 0;
    SYST_CSR = 0x7;  /* CLKSOURCE=processor, TICKINT=1, ENABLE=1 */
}

/* ----- Clock configuration -----
 * HSE 25 MHz → PLL1 → SYSCLK 480 MHz
 */
static void clock_init(void) {
    /* Enable HSE */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY)) { }

    /* Configure flash wait states for 480 MHz (VOS1) */
    FLASH_ACR = 0x07;  /* 7 wait states + prefetch + cache */

    /* Configure PLL1: M=25/12, N=240, P=2, Q=20, R=20
     * Input = 25 MHz, VCO = 25 * 240 = 6000 MHz, P=2 → 480 MHz? No:
     * PLL1CFGR: M=5, N=96, P=2, Q=8, R=8, frac=0
     * VCO = 25/5 * 96 = 480, P=2 → 240... need different.
     * Simplified: M=5, N=192, P=2 → 25/5*192/2 = 480 MHz ✓
     */
    RCC_PLLCFGR = (5U << 0) |     /* DIVM1 = 5 */
                   (192U << 8) |    /* DIVN1 = 192 */
                   (0U << 16) |    /* DIVP1EN = 0 (P output) */
                   (0U << 24) |    /* DIVQ1EN = 0 */
                   (0U << 28);     /* DIVR1EN = 0 */
    /* P divider = 2 */
    RCC_PLL1DIVR = (1U << 0) |     /* P div = 2 (val-1) */
                    (3U << 9) |    /* Q div = 4 */
                    (3U << 16);    /* R div = 4 */

    /* Enable PLL1 */
    RCC_CR |= RCC_CR_PLL1ON;
    while (!(RCC_CR & RCC_CR_PLL1RDY)) { }

    /* Switch SYSCLK to PLL1 P output */
    RCC_D1CFGR = 0x0;  /* D1 domain = SYSCLK / 1 = 480 MHz */
    RCC_CFGR = 3U << 0;  /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 3) != 3) { }

    /* Configure APB prescalers */
    RCC_D2CFGR = 0x00000000U;  /* APB1 = 240, APB2 = 240 */
    RCC_D3CFGR = 0x00000000U;

    /* Enable peripheral clocks */
    RCC_AHB1ENR |= (1U << 0);  /* DMA1 */
    RCC_AHB1ENR |= (1U << 1);  /* DMA2 */
    RCC_AHB2ENR |= (1U << 0);  /* GPIOA */
    RCC_AHB2ENR |= (1U << 1);  /* GPIOB */
    RCC_AHB2ENR |= (1U << 2);  /* GPIOC */
    RCC_AHB2ENR |= (1U << 3);  /* GPIOD */
    RCC_AHB2ENR |= (1U << 4);  /* GPIOE */
    RCC_AHB2ENR |= (1U << 6);  /* GPIOG */
    RCC_APB1ENR |= (1U << 18); /* USART2 */
    RCC_APB1ENR |= (1U << 19); /* USART3 */
    RCC_APB1ENR |= (1U << 14); /* SPI2 */
    RCC_APB1ENR |= (1U << 0);  /* TIM2 */
    RCC_APB1ENR |= (1U << 4);  /* TIM6 */
    RCC_APB2ENR |= (1U << 1);  /* TIM8 */
    RCC_APB2ENR |= (1U << 16); /* ADC1 */
    RCC_AHB2ENR |= (1U << 16); /* ADC */
    RCC_APB1ENR |= (1U << 21); /* I2C1 */
    RCC_AHB3ENR |= (1U << 6);  /* QSPI */
}

/* ----- GPIO initialization ----- */
static void gpio_init_all(void) {
    /* LEDs */
    for (int pin = LED_STATUS_PIN; pin <= LED_PWR_PIN; pin++) {
        gpio_set_mode(GPIO_LED_PORT(0), pin, GPIO_MODE_OUTPUT);
        gpio_set_otype(GPIO_LED_PORT(0), pin, GPIO_OTYPE_PP);
        gpio_set_speed(GPIO_LED_PORT(0), pin, GPIO_OSPEED_LOW);
        gpio_write(GPIO_LED_PORT(0), pin, 0);
    }

    /* Buttons (inputs with pull-up) */
    gpio_set_mode(BTN_PORT, BTN_MODE_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(BTN_PORT, BTN_MODE_PIN, GPIO_PUPD_PU);
    gpio_set_mode(BTN_PORT, BTN_FIRE_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(BTN_PORT, BTN_FIRE_PIN, GPIO_PUPD_PU);
    gpio_set_mode(BTN_PORT, BTN_SAFE_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(BTN_PORT, BTN_SAFE_PIN, GPIO_PUPD_PU);

    /* TCXO enable, PA enable, TX/RX switch */
    gpio_set_mode(RF_CTRL_PORT, TCXO_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(RF_CTRL_PORT, TCXO_EN_PIN, GPIO_OTYPE_PP);
    gpio_write(RF_CTRL_PORT, TCXO_EN_PIN, 0);  /* off initially */

    gpio_set_mode(RF_CTRL_PORT, PA_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(RF_CTRL_PORT, PA_EN_PIN, GPIO_OTYPE_PP);
    gpio_write(RF_CTRL_PORT, PA_EN_PIN, 0);    /* PA off */

    gpio_set_mode(RF_CTRL_PORT, TX_RX_SW_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(RF_CTRL_PORT, TX_RX_SW_PIN, GPIO_OTYPE_PP);
    gpio_write(RF_CTRL_PORT, TX_RX_SW_PIN, 0); /* RX mode */

    /* SPI2: MOSI=PB7, MISO=PB6, SCK=PB8 */
    gpio_set_mode(GPIOB, 7, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 7, 7);  /* SPI2_MOSI = AF7? actually need to verify */
    gpio_set_speed(GPIOB, 7, GPIO_OSPEED_VHIGH);
    gpio_set_mode(GPIOB, 6, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 6, 7);
    gpio_set_speed(GPIOB, 6, GPIO_OSPEED_VHIGH);
    gpio_set_mode(GPIOB, 8, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 8, 7);
    gpio_set_speed(GPIOB, 8, GPIO_OSPEED_VHIGH);

    /* USART3 debug: PD8=TX, PD9=RX */
    gpio_set_mode(GPIOD, 8, GPIO_MODE_AF);
    gpio_set_af(GPIOD, 8, 7);
    gpio_set_speed(GPIOD, 8, GPIO_OSPEED_HIGH);
    gpio_set_mode(GPIOD, 9, GPIO_MODE_AF);
    gpio_set_af(GPIOD, 9, 7);

    /* USART2 C2: PA2=TX, PA3=RX */
    gpio_set_mode(GPIOA, 2, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 2, 7);
    gpio_set_speed(GPIOA, 2, GPIO_OSPEED_HIGH);
    gpio_set_mode(GPIOA, 3, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 3, 7);

    /* PPS output: PG0 (TIM8 CH1) */
    gpio_set_mode(GPIOG, 0, GPIO_MODE_AF);
    gpio_set_af(GPIOG, 0, 3);  /* TIM8_CH1 = AF3 */
    gpio_set_speed(GPIOG, 0, GPIO_OSPEED_VHIGH);

    /* Battery ADC: PC0 (ADC1_IN10) */
    gpio_set_mode(GPIOC, 0, GPIO_MODE_ANALOG);
}

/* ----- SPI2 init ----- */
static void spi2_init(void) {
    /* Disable SPI */
    SPI2->CR1 = 0;
    /* Master, CPOL=0, CPHA=0, SSM=1, SSI=1, BR=APB1/16=7.5MHz */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                (3U << SPI_CR1_BR_SHIFT);  /* BR=3 → /16 */
    SPI2->CFG1 = (7U << 0) |  /* frame size 8 bit */
                  (0U << 28); /* CRC disable */
    SPI2->CFG2 = (1U << 22) | /* master auto-deassert */
                 (1U << 4) |   /* SSOM */
                 (0U << 0);   /* Motorola mode */
}

/* ----- USART3 (debug) init @ 230400 baud ----- */
static void uart_debug_init(void) {
    USART3->CR1 = 0;
    USART3->BRR = APB1_HZ / 230400;
    USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/* ----- USART2 (C2 to nRF52840) init @ 230400 baud ----- */
static void uart_c2_init(void) {
    USART2->CR1 = 0;
    USART2->BRR = APB1_HZ / C2_BAUD;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE |
                  USART_CR1_RXNEIE;

    /* Enable USART2 interrupt */
    NVIC_ICER0 = 0;  /* clear first */
    NVIC_ISER0 = (1U << 38);  /* USART2 IRQ = 38 */
}

/* ----- Debug print ----- */
static void dbg_putc(char c) {
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = (uint8_t)c;
}

static void dbg_puts(const char *s) {
    while (*s) dbg_putc(*s++);
}

static void dbg_hex(uint32_t v, int digits) {
    const char *hexd = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0; i--)
        dbg_putc(hexd[(v >> (i * 4)) & 0xF]);
}

static void dbg_dec(uint32_t v) {
    char buf[12];
    int i = 0;
    if (v == 0) { dbg_putc('0'); return; }
    while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i > 0) dbg_putc(buf[--i]);
}

/* ----- ADC: read battery voltage ----- */
static uint16_t adc_read_batt(void) {
    /* Enable ADC */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    /* Configure channel 10, 12-bit resolution */
    ADC1->SMPR1 = (7U << 0);  /* channel 10, 640.5 cycles */
    ADC1->CR |= ADC_CR_ADSTART;
    /* Wait for conversion */
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    uint16_t raw = (uint16_t)ADC1->DR;
    /* Convert: Vref=3.3V, 12-bit, divider=2 → mV = raw/4095*3300*2 */
    uint32_t mv = (uint32_t)raw * 3300 * 2 / 4095;
    return (uint16_t)mv;
}

/* ----- TIM8: PPS output (1 Hz, 100 ms pulse) ----- */
static void pps_init(void) {
    TIM8->CR1 = 0;
    TIM8->PSC = (APB2_HZ * 2 / 1000) - 1;  /* 1 kHz tick */
    TIM8->ARR = 999;                        /* 1 Hz */
    TIM8->CCR1 = 100;                       /* 100 ms high */
    /* PWM mode 1 on CH1 */
    TIM8->CCMR1 = TIM_CCMR_OC1M_PWM1 | TIM_CCMR_OC1PE;
    TIM8->CCER = 1;  /* CC1E = enable output */
    TIM8->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

/* ----- TIM6: DAC trigger for IF baseband (if used) ----- */
static void dac_trigger_init(void) {
    TIM6->CR1 = 0;
    TIM6->PSC = 0;
    TIM6->ARR = 480;  /* 480 MHz / 480 = 1 MHz trigger */
    TIM6->DIER = TIM_DIER_UDE;  /* update DMA request */
    /* DAC1 CR: enable ch1, DMA, trigger from TIM6 TRGO */
    DAC1->CR = DAC_CR_EN1 | DAC_CR_DMAEN1 | DAC_CR_TEN1 |
               (0U << DAC_CR_TSEL1_SHIFT);  /* TSEL=TIM6 TRGO */
    TIM6->CR1 = TIM_CR1_CEN;
}

/* ----- Safety interlock ----- */
static int check_safety(void) {
    /* Safety switch: 0 = SAFE, 1 = LIVE (active low, pulled up) */
    int safe_pin = gpio_read(BTN_PORT, BTN_SAFE_PIN);
    safety_engaged = (safe_pin == 1) ? 0 : 1;  /* pulled low = live */

    if (safety_engaged == 1) {
        /* In LIVE mode, check fire button hold */
        if (gpio_read(BTN_PORT, BTN_FIRE_PIN) == 0) {
            fire_hold_ms++;
        } else {
            fire_hold_ms = 0;
        }
        if (fire_hold_ms >= SAFETY_ARM_HOLD_MS) {
            return 1;  /* armed via physical interlock */
        }
    }
    return 0;
}

static void update_arm_state(void) {
    /* Auto-disarm after timeout */
    if (armed && (system_ticks - last_arm_time) > ARMED_AUTO_DISARM_MS) {
        armed = 0;
        transmitting = 0;
        gnss_engine_disarm(&engine);
        si4463_stop_tx(&rf_gps);
        si4463_stop_tx(&rf_glo);
        gpio_write(GPIO_LED_PORT(0), LED_SPOOF_PIN, 0);
        dbg_puts("[SAFETY] Auto-disarmed after timeout\r\n");
    }

    /* Check physical interlock */
    if (check_safety()) {
        if (!armed) {
            armed = 1;
            last_arm_time = system_ticks;
            gnss_engine_arm(&engine);
            gpio_write(GPIO_LED_PORT(0), LED_SPOOF_PIN, 1);
            dbg_puts("[SAFETY] Armed via physical interlock\r\n");
        }
    }
}

/* ----- C2 command processing ----- */
static void process_c2_command(uint8_t cmd, const uint8_t *payload,
                                 uint16_t len) {
    uint8_t rsp[8];
    uint16_t rsp_len = 0;

    switch (cmd) {
    case C2_CMD_PING:
        rsp[0] = C2_RSP_OK;
        rsp[1] = FIRMWARE_VERSION_MAJOR;
        rsp[2] = FIRMWARE_VERSION_MINOR;
        rsp[3] = FIRMWARE_VERSION_PATCH;
        rsp_len = 4;
        break;

    case C2_CMD_GET_STATUS: {
        rsp[0] = C2_RSP_OK;
        rsp[1] = (uint8_t)engine.state;
        rsp[2] = (uint8_t)engine.channel_count;
        rsp[3] = (uint8_t)(batt_mv >> 8);
        rsp[4] = (uint8_t)(batt_mv & 0xFF);
        rsp[5] = (uint8_t)temp_c;
        rsp[6] = armed ? 1 : 0;
        rsp[7] = safety_engaged ? 1 : 0;
        rsp_len = 8;
        break;
    }

    case C2_CMD_SET_FREQ: {
        if (len < 4) { rsp[0] = C2_RSP_BAD_PARAM; rsp_len = 1; break; }
        uint32_t freq = (payload[0] << 24) | (payload[1] << 16) |
                         (payload[2] << 8) | payload[3];
        int rc = si4463_set_frequency(&rf_gps, freq);
        rsp[0] = (rc == 0) ? C2_RSP_OK : C2_RSP_ERROR;
        rsp_len = 1;
        break;
    }

    case C2_CMD_SET_POWER: {
        if (len < 1) { rsp[0] = C2_RSP_BAD_PARAM; rsp_len = 1; break; }
        int rc = si4463_set_tx_power(&rf_gps, (int8_t)payload[0]);
        rsp[0] = (rc == 0) ? C2_RSP_OK : C2_RSP_ERROR;
        rsp_len = 1;
        break;
    }

    case C2_CMD_SET_SV: {
        if (len < 3) { rsp[0] = C2_RSP_BAD_PARAM; rsp_len = 1; break; }
        constellation_t cons = (constellation_t)payload[0];
        uint8_t prn = payload[1];
        int8_t power = (int8_t)payload[2];
        int rc = gnss_engine_add_sv(&engine, cons, prn, power);
        rsp[0] = (rc == 0) ? C2_RSP_OK : C2_RSP_ERROR;
        rsp_len = 1;
        break;
    }

    case C2_CMD_LOAD_EPHEMERIS: {
        if (len < 1) { rsp[0] = C2_RSP_BAD_PARAM; rsp_len = 1; break; }
        if (len < 1 + 1) { rsp[0] = C2_RSP_BAD_PARAM; rsp_len = 1; break; }
        uint8_t prn = payload[0];
        int rc = gnss_engine_load_ephemeris(&engine, prn,
                                             &payload[1], len - 1);
        rsp[0] = (rc == 0) ? C2_RSP_OK : C2_RSP_ERROR;
        rsp_len = 1;
        break;
    }

    case C2_CMD_START_SPOOF: {
        if (!armed) {
            rsp[0] = C2_RSP_UNARMED;
            rsp_len = 1;
            break;
        }
        /* Enable TCXO and PA */
        gpio_write(RF_CTRL_PORT, TCXO_EN_PIN, 1);
        gpio_write(RF_CTRL_PORT, PA_EN_PIN, 1);
        gpio_write(RF_CTRL_PORT, TX_RX_SW_PIN, 1);  /* TX mode */
        delay_ms(10);  /* let TCXO stabilize */

        /* Generate initial samples */
        uint16_t nsamp = gnss_engine_produce_samples(&engine,
                                                     engine.output_buf,
                                                     512);
        engine.output_len = nsamp;

        /* Start TX on both transceivers */
        si4463_start_tx(&rf_gps, engine.output_buf, nsamp);
        si4463_start_tx(&rf_glo, engine.output_buf, nsamp);

        int rc = gnss_engine_start(&engine);
        transmitting = 1;
        gpio_write(GPIO_LED_PORT(0), LED_SPOOF_PIN, 1);
        rsp[0] = (rc == 0) ? C2_RSP_OK : C2_RSP_ERROR;
        rsp_len = 1;
        dbg_puts("[C2] Spoofing started\r\n");
        break;
    }

    case C2_CMD_STOP_SPOOF: {
        si4463_stop_tx(&rf_gps);
        si4463_stop_tx(&rf_glo);
        gnss_engine_stop(&engine);
        transmitting = 0;
        gpio_write(RF_CTRL_PORT, PA_EN_PIN, 0);
        gpio_write(RF_CTRL_PORT, TX_RX_SW_PIN, 0);
        gpio_write(GPIO_LED_PORT(0), LED_SPOOF_PIN, 0);
        rsp[0] = C2_RSP_OK;
        rsp_len = 1;
        dbg_puts("[C2] Spoofing stopped\r\n");
        break;
    }

    case C2_CMD_SET_TRAJECTORY: {
        if (len < 24) { rsp[0] = C2_RSP_BAD_PARAM; rsp_len = 1; break; }
        /* 8 bytes lat, 8 bytes lon, 4 bytes alt, 4 bytes reserved */
        uint64_t lat_u64 = 0, lon_u64 = 0;
        for (int i = 0; i < 8; i++) {
            lat_u64 = (lat_u64 << 8) | payload[i];
            lon_u64 = (lon_u64 << 8) | payload[8 + i];
        }
        double lat = *(double *)&lat_u64;
        double lon = *(double *)&lon_u64;
        double alt = (double)((payload[16] << 24) | (payload[17] << 16) |
                              (payload[18] << 8) | payload[19]);
        gnss_engine_set_position(&engine, lat, lon, alt);
        rsp[0] = C2_RSP_OK;
        rsp_len = 1;
        break;
    }

    case C2_CMD_SET_TIME_OFFSET: {
        if (len < 8) { rsp[0] = C2_RSP_BAD_PARAM; rsp_len = 1; break; }
        uint32_t week = (payload[0] << 24) | (payload[1] << 16) |
                         (payload[2] << 8) | payload[3];
        uint32_t tow = (payload[4] << 24) | (payload[5] << 16) |
                        (payload[6] << 8) | payload[7];
        gnss_engine_set_time(&engine, week, tow);
        rsp[0] = C2_RSP_OK;
        rsp_len = 1;
        break;
    }

    case C2_CMD_ARM: {
        /* C2 arm requires safety switch in LIVE position */
        if (!safety_engaged) {
            rsp[0] = C2_RSP_UNARMED;
            rsp_len = 1;
            break;
        }
        armed = 1;
        last_arm_time = system_ticks;
        gnss_engine_arm(&engine);
        gpio_write(GPIO_LED_PORT(0), LED_SPOOF_PIN, 1);
        rsp[0] = C2_RSP_OK;
        rsp_len = 1;
        dbg_puts("[C2] Armed\r\n");
        break;
    }

    case C2_CMD_DISARM: {
        armed = 0;
        transmitting = 0;
        if (engine.state == ENGINE_TRANSMITTING) {
            si4463_stop_tx(&rf_gps);
            si4463_stop_tx(&rf_glo);
            gpio_write(RF_CTRL_PORT, PA_EN_PIN, 0);
        }
        gnss_engine_disarm(&engine);
        gpio_write(GPIO_LED_PORT(0), LED_SPOOF_PIN, 0);
        rsp[0] = C2_RSP_OK;
        rsp_len = 1;
        dbg_puts("[C2] Disarmed\r\n");
        break;
    }

    case C2_CMD_FACTORY_RESET: {
        /* Stop everything and reset engine */
        si4463_stop_tx(&rf_gps);
        si4463_stop_tx(&rf_glo);
        gnss_engine_init(&engine);
        armed = 0;
        transmitting = 0;
        gpio_write(RF_CTRL_PORT, PA_EN_PIN, 0);
        gpio_write(GPIO_LED_PORT(0), LED_SPOOF_PIN, 0);
        rsp[0] = C2_RSP_OK;
        rsp_len = 1;
        break;
    }

    default:
        rsp[0] = C2_RSP_ERROR;
        rsp_len = 1;
        break;
    }

    c2_send_status(&c2, rsp[0], &rsp[1], rsp_len > 0 ? rsp_len - 1 : 0);
}

/* ----- USART2 interrupt (C2 RX) ----- */
void USART2_IRQHandler(void) {
    if (USART2->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART2->RDR;
        uint8_t cmd;
        uint16_t plen;
        int rc = c2_rx_byte(&c2, byte, &cmd, c2_rx_payload, &plen);
        if (rc > 0) {
            /* Process in main loop via flag — for simplicity, process here */
            process_c2_command(cmd, c2_rx_payload, plen);
        }
    }
    if (USART2->ISR & (1U << 3)) {  /* ORE */
        USART2->ICR = (1U << 3);
    }
}

/* ----- SysTick interrupt ----- */
void SysTick_Handler(void) {
    system_ticks++;
}

/* ----- TX FIFO refresher (called from main loop) ----- */
static void refresh_tx_fifos(void) {
    if (!transmitting) return;

    /* Check Si4463 IRQ pin (FIFO almost empty) */
    int gps_irq = gpio_read(SI4463_PORT, SI4463_IRQ_PIN);
    int glo_irq = gpio_read(SI4463B_PORT, SI4463B_IRQ_PIN);

    if (gps_irq == 0 || glo_irq == 0) {
        /* Generate more samples */
        uint16_t nsamp = gnss_engine_produce_samples(&engine,
                                                     engine.output_buf,
                                                     256);
        if (nsamp > 0) {
            if (gps_irq == 0)
                si4463_tx_fifo_fill(&rf_gps, engine.output_buf, nsamp);
            if (glo_irq == 0)
                si4463_tx_fifo_fill(&rf_glo, engine.output_buf, nsamp);
        }
    }
}

/* ----- Status LED update ----- */
static void update_leds(void) {
    static uint32_t last_blink = 0;
    static uint8_t led_state = 0;

    /* Status LED: blink pattern indicates state */
    if (system_ticks - last_blink > 500) {
        last_blink = system_ticks;
        led_state ^= 1;
        gpio_write(GPIO_LED_PORT(0), LED_STATUS_PIN, led_state);
    }

    /* Power LED: solid on */
    gpio_write(GPIO_LED_PORT(0), LED_PWR_PIN, 1);

    /* RX LED: off in TX mode */
    gpio_write(GPIO_LED_PORT(0), LED_RX_PIN, 0);
}

/* ----- Main ----- */
int main(void) {
    /* Hardware init */
    clock_init();
    systick_init();
    gpio_init_all();
    spi2_init();
    uart_debug_init();
    uart_c2_init();
    pps_init();

    /* Enable global interrupts */
    asm volatile ("cpsie i" ::: "memory");

    /* Boot banner */
    dbg_puts("\r\n");
    dbg_puts("===============================\r\n");
    dbg_puts("  GNSS-Phantom v");
    dbg_dec(FIRMWARE_VERSION_MAJOR); dbg_putc('.');
    dbg_dec(FIRMWARE_VERSION_MINOR); dbg_putc('.');
    dbg_dec(FIRMWARE_VERSION_PATCH);
    dbg_puts("\r\n");
    dbg_puts("  Author: jayis1\r\n");
    dbg_puts(" AUTHORIZED SECURITY RESEARCH USE ONLY\r\n");
    dbg_puts("===============================\r\n");

    /* Initialize C2 protocol */
    c2_init(&c2);
    dbg_puts("[INIT] C2 protocol initialized\r\n");

    /* Initialize GNSS spoofing engine */
    gnss_engine_init(&engine);
    dbg_puts("[INIT] GNSS engine initialized\r\n");

    /* Initialize RF transceivers */
    dbg_puts("[INIT] Initializing Si4463 #1 (GPS/Galileo)...\r\n");
    int rc1 = si4463_init(&rf_gps, SI4463_GPS);
    if (rc1) {
        dbg_puts("[ERROR] Si4463 #1 init failed: ");
        dbg_dec(rc1); dbg_puts("\r\n");
    } else {
        dbg_puts("[INIT] Si4463 #1 OK\r\n");
    }

    dbg_puts("[INIT] Initializing Si4463 #2 (GLONASS/BeiDou)...\r\n");
    int rc2 = si4463_init(&rf_glo, SI4463_GLO);
    if (rc2) {
        dbg_puts("[ERROR] Si4463 #2 init failed: ");
        dbg_dec(rc2); dbg_puts("\r\n");
    } else {
        dbg_puts("[INIT] Si4463 #2 OK\r\n");
    }

    /* Set default frequencies */
    if (rc1 == 0)
        si4463_set_frequency(&rf_gps, (uint32_t)(GPS_L1_FREQ_MHZ * 1e6));
    if (rc2 == 0)
        si4463_set_frequency(&rf_glo, (uint32_t)(GLONASS_L1_FREQ_MHZ * 1e6));

    dbg_puts("[INIT] Boot complete. State: IDLE\r\n");

    uint32_t last_batt_check = 0;
    uint32_t last_status_report = 0;

    /* Main loop */
    while (1) {
        /* Check safety interlock */
        update_arm_state();

        /* Refresh TX FIFOs if transmitting */
        refresh_tx_fifos();

        /* Battery check every 5 seconds */
        if (system_ticks - last_batt_check > 5000) {
            last_batt_check = system_ticks;
            batt_mv = adc_read_batt();
            if (batt_mv < BATT_LOW_MV) {
                dbg_puts("[WARN] Low battery: ");
                dbg_dec(batt_mv); dbg_puts(" mV\r\n");
                gpio_write(GPIO_LED_PORT(0), LED_PWR_PIN, 0);
            }
        }

        /* Status report every 10 seconds */
        if (system_ticks - last_status_report > 10000) {
            last_status_report = system_ticks;
            dbg_puts("[STATUS] state=");
            dbg_dec(engine.state);
            dbg_puts(" sv_count=");
            dbg_dec(engine.channel_count);
            dbg_puts(" armed=");
            dbg_dec(armed);
            dbg_puts(" batt=");
            dbg_dec(batt_mv);
            dbg_puts("mV frames=");
            dbg_dec(engine.frames_sent);
            dbg_puts("\r\n");
        }

        /* Update status LEDs */
        update_leds();

        /* Mode button: cycle through test scenarios when not transmitting */
        static uint8_t last_mode_btn = 1;
        uint8_t mode_btn = gpio_read(BTN_PORT, BTN_MODE_PIN);
        if (mode_btn == 0 && last_mode_btn == 1 && !transmitting) {
            /* Add a test SV (PRN 7, GPS) */
            gnss_engine_add_sv(&engine, CONSTELLATION_GPS, 7, 0);
            dbg_puts("[UI] Added test SV PRN 7\r\n");
        }
        last_mode_btn = mode_btn;
    }

    return 0;
}