/*
 * main.c — Main firmware for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * MagLance: Targeted Magnetic Field Injection Platform
 *
 * This is the main entry point and control loop for the MagLance device.
 * It initializes all subsystems, manages the user interface (OLED display,
 * buttons, rotary encoder), and dispatches commands from USB/BLE.
 *
 * Build: make
 * Flash: make flash (requires OpenOCD + ST-Link)
 *
 * Target: STM32G474VET6 (Cortex-M4F, 170 MHz, 512 KB flash, 128 KB SRAM)
 */

#include "board.h"
#include "registers.h"
#include "coil_driver.h"
#include "magnetometer.h"
#include "profile_manager.h"
#include "usb_iface.h"
#include <string.h>

/* ---- Global system status ---- */
system_status_t g_status;

/* ---- SysTick (1 ms tick) ---- */
static volatile uint32_t s_tick_ms = 0;
static volatile uint32_t s_safety_check_counter = 0;
static volatile uint32_t s_display_update_counter = 0;
static volatile uint32_t s_mag_update_counter = 0;
static volatile uint32_t s_pi_update_counter = 0;

/* ---- SysTick interrupt handler ---- */
void SysTick_Handler(void)
{
    s_tick_ms++;

    /* Safety check every 1 ms */
    s_safety_check_counter++;

    /* Display update every 100 ms */
    s_display_update_counter++;

    /* Magnetometer update every 10 ms (100 Hz) */
    s_mag_update_counter++;

    /* PI controller update every 2 ms (500 Hz) */
    s_pi_update_counter++;

    /* Sweep state machine tick every 1 ms */
    coil_driver_sweep_tick();
}

static void systick_init(void)
{
    /* Configure SysTick for 1 ms interrupt at 170 MHz */
    SysTick->LOAD = (BOARD_HCLK_HZ / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT |
                    SysTick_CTRL_CLKSOURCE;
}

static uint32_t get_tick_ms(void)
{
    return s_tick_ms;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = s_tick_ms;
    while ((s_tick_ms - start) < ms) { }
}

/* ---- Clock configuration ---- */

static void clock_init(void)
{
    /*
     * Configure PLL for 170 MHz system clock from 16 MHz HSE:
     *   PLLM = 4 → 4 MHz VCO input
     *   PLLN = 85 → 340 MHz VCO output
     *   PLLP = 2 → 170 MHz SYSCLK
     *   PLLQ = 8 → 42.5 MHz USB (not used here)
     *   PLLR = 2 → 170 MHz
     */

    /* Enable HSE (external 16 MHz crystal) */
    SET_BIT(RCC->CR, (1U << 16));  /* HSEON */
    while (!(READ_REG(RCC->CR) & (1U << 17))) { }  /* Wait for HSERDY */

    /* Configure flash latency for 170 MHz (5 wait states) */
    REG32(0x40022000 + 0x00) = 0x05;  /* FLASH_ACR: LATENCY = 5 */

    /* Configure PLLCFGR */
    uint32_t pllcfgr = 0;
    pllcfgr |= (1U << 0);        /* PLLSRC = HSE */
    pllcfgr |= (4U << 4);        /* PLLM = 4 */
    pllcfgr |= (85U << 8);       /* PLLN = 85 */
    pllcfgr |= (0U << 25);       /* PLLP = 2 (00) */
    pllcfgr |= (8U << 20);       /* PLLQ = 8 */
    pllcfgr |= (2U << 28);       /* PLLR = 2 */
    RCC->PLLCFGR = pllcfgr;

    /* Enable PLL */
    SET_BIT(RCC->CR, (1U << 24));  /* PLLON */
    while (!(READ_REG(RCC->CR) & (1U << 25))) { }  /* Wait for PLLRDY */

    /* Switch system clock to PLL */
    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~(0x03U << 0);  /* Clear SW bits */
    cfgr |= (0x03U << 0);   /* SW = PLL */
    RCC->CFGR = cfgr;
    while ((READ_REG(RCC->CFGR) & (0x03U << 2)) != (0x03U << 2)) { }

    /* Enable all GPIO clocks */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIODEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOGEN);
}

/* ---- GPIO helper ---- */

static void gpio_config_input_pu(GPIO_TypeDef *port, uint8_t pin)
{
    uint32_t moder = port->MODER;
    moder &= ~(0x03U << (pin * 2));
    port->MODER = moder;  /* Input mode */

    uint32_t pupdr = port->PUPDR;
    pupdr &= ~(0x03U << (pin * 2));
    pupdr |= (GPIO_PUPD_PU << (pin * 2));  /* Pull-up */
    port->PUPDR = pupdr;
}

static uint8_t gpio_read_pin(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR >> pin) & 1U;
}

/* ---- Rotary Encoder (via TIM3) ---- */

static volatile int16_t s_encoder_count = 0;

static void encoder_init(void)
{
    /* Enable TIM3 clock */
    SET_BIT(RCC->APB1ENR1, RCC_APB1ENR1_TIM3EN);

    /* Configure PC6 and PC7 as AF2 (TIM3 CH1/CH2) */
    for (int pin = 6; pin <= 7; pin++) {
        uint32_t moder = ENC_A_PORT->MODER;
        moder &= ~(0x03U << (pin * 2));
        moder |= (GPIO_MODE_AF << (pin * 2));
        ENC_A_PORT->MODER = moder;

        uint32_t afrl = ENC_A_PORT->AFRL;
        afrl &= ~(0x0FU << (pin * 4));
        afrl |= (0x02U << (pin * 4));  /* AF2 = TIM3 */
        ENC_A_PORT->AFRL = afrl;
    }

    /* Configure TIM3 in encoder mode 3 (both edges, both channels) */
    TIM3->ARR = 0xFFFF;  /* Maximum count */
    TIM3->CCMR1 = TIM_CCMR1_CC1S_INPUT_TI2 | TIM_CCMR1_CC2S_INPUT_TI1;
    TIM3->CCER = 0;  /* No capture, just count */
    TIM3->SMCR = TIM_SMCR_SMS_ENCODER3;  /* Encoder mode 3 */
    TIM3->CR1 = TIM_SMCR_CEN;  /* Enable counter */
}

static int16_t encoder_get_delta(void)
{
    static uint16_t s_last_count = 0;
    uint16_t current = (uint16_t)TIM3->CNT;
    int16_t delta = (int16_t)(current - s_last_count);
    s_last_count = current;
    return delta;
}

/* ---- OLED Display (SSD1306 via I2C1) ---- */

static void oled_send_command(uint8_t cmd)
{
    /* I2C write: addr=0x3C, data=[0x00, cmd] */
    uint8_t buf[2] = {0x00, cmd};
    /* Use profile_manager's I2C (already initialized) */
    /* For simplicity, we send directly */
    /* TODO: implement I2C write for OLED */
    (void)buf;
}

static void oled_send_data(const uint8_t *data, uint16_t len)
{
    /* I2C write: addr=0x3C, data=[0x40, data...] */
    (void)data;
    (void)len;
}

static void oled_init(void)
{
    /* SSD1306 initialization sequence */
    oled_send_command(0xAE);  /* Display off */
    oled_send_command(0x20);  /* Set memory addressing mode */
    oled_send_command(0x10);  /* Page addressing mode */
    oled_send_command(0xB0);  /* Set page start address */
    oled_send_command(0xC8);  /* COM scan direction (remapped) */
    oled_send_command(0x40);  /* Set display start line = 0 */
    oled_send_command(0x81);  /* Set contrast */
    oled_send_command(0xFF);  /* Maximum contrast */
    oled_send_command(0xA1);  /* Segment remap (col 127 = SEG0) */
    oled_send_command(0xA6);  /* Normal display (not inverted) */
    oled_send_command(0xA8);  /* Set multiplex ratio */
    oled_send_command(0x3F);  /* 1/64 duty */
    oled_send_command(0xD3);  /* Set display offset */
    oled_send_command(0x00);  /* No offset */
    oled_send_command(0xD5);  /* Set display clock divide */
    oled_send_command(0x80);  /* Default */
    oled_send_command(0xD9);  /* Set pre-charge period */
    oled_send_command(0x22);  /* Default */
    oled_send_command(0xDA);  /* Set COM pins */
    oled_send_command(0x12);  /* Alternative COM pin config */
    oled_send_command(0xDB);  /* Set VCOMH deselect level */
    oled_send_command(0x20);  /* 0.77 × VCC */
    oled_send_command(0x8D);  /* Charge pump setting */
    oled_send_command(0x14);  /* Enable charge pump */
    oled_send_command(0xAF);  /* Display on */
}

static void oled_clear(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_send_command(0xB0 + page);  /* Set page address */
        oled_send_command(0x00);          /* Lower column address */
        oled_send_command(0x10);          /* Upper column address */
        uint8_t empty[128] = {0};
        oled_send_data(empty, 128);
    }
}

/* Simple 5×8 font for OLED display */
static const uint8_t s_font5x8[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    /* ... abbreviated font table ... */
    {0x7C,0x12,0x11,0x12,0x7C}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x3E,0x41,0x41,0x51,0x32}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x1E,0x21,0x21,0x21,0x7E}, /* Q */
    {0x7F,0x09,0x09,0x09,0x76}, /* R */
    {0x26,0x49,0x49,0x49,0x32}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
};

static uint8_t char_to_font_idx(char c)
{
    if (c >= 'A' && c <= 'Z') return 2 + (c - 'A');
    if (c >= '0' && c <= '9') return 2 + 26 + (c - '0');
    if (c == ' ') return 0;
    if (c == '!') return 1;
    return 0;  /* Default to space */
}

static void oled_draw_char(uint8_t page, uint8_t col, char c)
{
    uint8_t idx = char_to_font_idx(c);
    oled_send_command(0xB0 + page);
    oled_send_command(col & 0x0F);
    oled_send_command(0x10 | (col >> 4));
    oled_send_data((uint8_t *)s_font5x8[idx], 5);
    oled_send_data((uint8_t *)"\x00", 1);  /* Space between chars */
}

static void oled_draw_string(uint8_t page, uint8_t col, const char *str)
{
    while (*str && col < 122) {
        oled_draw_char(page, col, *str);
        col += 6;
        str++;
    }
}

/* ---- Display update ---- */

static const char *mode_to_string(operating_mode_t mode)
{
    switch (mode) {
    case MODE_IDLE:      return "IDLE";
    case MODE_PULSE:     return "PULSE";
    case MODE_DC:        return "DC";
    case MODE_SWEEP:     return "SWEEP";
    case MODE_SENSE:     return "SENSE";
    case MODE_PROFILE:   return "PROFILE";
    case MODE_CALIBRATE: return "CAL";
    case MODE_ERROR:      return "ERROR";
    default:             return "???";
    }
}

static void update_display(void)
{
    oled_clear();

    /* Line 0: Mode and status */
    char buf[22];
    buf[0] = 'M'; buf[1] = ':';
    int i = 2;
    const char *mode_str = mode_to_string(g_status.mode);
    while (*mode_str && i < 21) buf[i++] = *mode_str++;
    buf[i] = '\0';
    oled_draw_string(0, 0, buf);

    /* Line 1: Current and field */
    /* Format: "I:XXXXmA F:XXXuT" */
    buf[0] = 'I'; buf[1] = ':';
    /* Simple integer to string for current */
    uint32_t cur = g_status.current_ma;
    i = 2;
    if (cur == 0) { buf[i++] = '0'; }
    else {
        char tmp[10];
        int ti = 0;
        while (cur > 0 && ti < 9) { tmp[ti++] = '0' + (cur % 10); cur /= 10; }
        while (ti > 0 && i < 21) buf[i++] = tmp[--ti];
    }
    buf[i++] = 'm'; buf[i++] = 'A';
    buf[i] = '\0';
    oled_draw_string(1, 0, buf);

    /* Line 2: Battery and temperature */
    buf[0] = 'V'; buf[1] = ':';
    uint32_t vbat = g_status.vbat_mv;
    i = 2;
    if (vbat == 0) { buf[i++] = '0'; }
    else {
        char tmp[10];
        int ti = 0;
        while (vbat > 0 && ti < 9) { tmp[ti++] = '0' + (vbat % 10); vbat /= 10; }
        while (ti > 0 && i < 21) buf[i++] = tmp[--ti];
    }
    buf[i++] = 'm'; buf[i++] = 'V';
    buf[i] = '\0';
    oled_draw_string(2, 0, buf);

    /* Line 3: Profile and tip */
    buf[0] = 'P'; buf[1] = ':';
    buf[2] = '0' + (g_status.active_profile / 10);
    buf[3] = '0' + (g_status.active_profile % 10);
    buf[4] = ' '; buf[5] = 'T'; buf[6] = ':';
    buf[7] = '0' + (g_status.tip_id / 10);
    buf[8] = '0' + (g_status.tip_id % 10);
    buf[9] = '\0';
    oled_draw_string(3, 0, buf);

    /* Line 4-7: Field readings (3 sensors) */
    for (int s = 0; s < 3; s++) {
        buf[0] = 'S'; buf[1] = '0' + s; buf[2] = ':';
        buf[3] = 'X'; buf[4] = '0' + (g_status.field_x[s] / 1000);
        buf[5] = 'Y'; buf[6] = '0' + (g_status.field_y[s] / 1000);
        buf[7] = 'Z'; buf[8] = '0' + (g_status.field_z[s] / 1000);
        buf[9] = '\0';
        oled_draw_string(4 + s, 0, buf);
    }
}

/* ---- Safety monitoring ---- */

static void safety_check(void)
{
    static uint32_t s_last_check = 0;
    if ((s_tick_ms - s_last_check) < 10) return;  /* Every 10 ms */
    s_last_check = s_tick_ms;

    /* Check battery voltage (simulated — would read from ADC2) */
    /* TODO: implement ADC2 read for battery voltage */
    /* For now, use nominal */
    g_status.vbat_mv = BAT_NOMINAL_MV;

    if (g_status.vbat_mv < BAT_CRIT_MV) {
        g_status.error_code = ERR_BAT_CRIT;
        coil_driver_emergency_stop();
        g_status.mode = MODE_ERROR;
        return;
    }
    if (g_status.vbat_mv < BAT_LOW_MV) {
        g_status.error_code = ERR_BAT_LOW;
    }

    /* Check temperatures (simulated — would read from ADS1115) */
    /* TODO: implement ADS1115 read for NTC thermistors */
    g_status.temp_bridge_c = 25;  /* Ambient default */
    g_status.temp_coil_c = 25;

    if (g_status.temp_bridge_c > COIL_MAX_TEMP_C) {
        g_status.error_code = ERR_OVERTEMP_BRIDGE;
        coil_driver_emergency_stop();
        g_status.mode = MODE_ERROR;
        return;
    }
    if (g_status.temp_coil_c > COIL_MAX_TEMP_C) {
        g_status.error_code = ERR_OVERTEMP_COIL;
        coil_driver_emergency_stop();
        g_status.mode = MODE_ERROR;
        return;
    }

    /* Check coil current */
    uint32_t current = coil_driver_get_current_ma();
    if (current > COIL_MAX_CURRENT_MA + 5000) {  /* 5 A overlimit */
        g_status.error_code = ERR_OVERCURRENT;
        coil_driver_emergency_stop();
        g_status.mode = MODE_ERROR;
        return;
    }

    /* Check dead-man's switch during active output */
    bridge_state_t state = coil_driver_get_state();
    if (state == BRIDGE_FORWARD || state == BRIDGE_REVERSE) {
        if (!gpio_read_pin(BTN_FIRE_PORT, BTN_FIRE_PIN)) {
            /* Fire button released — stop output */
            coil_driver_dc_stop();
            coil_driver_sweep_stop();
        }
    }
}

/* ---- Button handling ---- */

static uint8_t btn_debounce(GPIO_TypeDef *port, uint8_t pin)
{
    /* Simple debounce: read 3 times with small delay */
    uint8_t v1 = gpio_read_pin(port, pin);
    for (volatile int d = 0; d < 1000; d++) { }
    uint8_t v2 = gpio_read_pin(port, pin);
    for (volatile int d = 0; d < 1000; d++) { }
    uint8_t v3 = gpio_read_pin(port, pin);
    return (v1 == v2 && v2 == v3) ? v1 : 1;  /* Default to not pressed (high) */
}

static void handle_buttons(void)
{
    static uint32_t s_last_btn_check = 0;
    if ((s_tick_ms - s_last_btn_check) < 50) return;  /* Every 50 ms */
    s_last_btn_check = s_tick_ms;

    /* MODE button: cycle through modes */
    static uint8_t s_mode_btn_last = 1;
    uint8_t mode_btn = btn_debounce(BTN_MODE_PORT, BTN_MODE_PIN);
    if (s_mode_btn_last == 1 && mode_btn == 0) {
        /* Button pressed (falling edge) */
        g_status.mode = (operating_mode_t)((g_status.mode + 1) % MODE_COUNT);
        if (g_status.mode == MODE_ERROR) {
            g_status.mode = MODE_IDLE;  /* Skip ERROR state */
        }
    }
    s_mode_btn_last = mode_btn;

    /* SELECT button: fire pulse / confirm */
    static uint8_t s_sel_btn_last = 1;
    uint8_t sel_btn = btn_debounce(BTN_SELECT_PORT, BTN_SELECT_PIN);
    if (s_sel_btn_last == 1 && sel_btn == 0) {
        /* Button pressed */
        switch (g_status.mode) {
        case MODE_PULSE:
            /* Fire single pulse with current settings */
            if (coil_driver_arm() == 0) {
                coil_driver_pulse(g_status.pulse_width_ns,
                                  g_status.target_current_ma,
                                  g_status.polarity);
                coil_driver_disarm();
            }
            break;
        case MODE_DC:
            /* Toggle DC mode */
            if (coil_driver_get_state() == BRIDGE_IDLE) {
                if (coil_driver_arm() == 0) {
                    coil_driver_dc_start(g_status.target_current_ma,
                                         g_status.polarity);
                }
            } else {
                coil_driver_dc_stop();
                coil_driver_disarm();
            }
            break;
        case MODE_PROFILE:
            /* Load and execute profile */
            {
                profile_t p;
                if (profile_manager_load(g_status.active_profile, &p) == 0) {
                    g_status.pulse_width_ns = p.pulse_width_ns;
                    g_status.target_current_ma = p.current_ma;
                    g_status.polarity = (polarity_t)p.polarity;
                    g_status.mode = (operating_mode_t)p.mode;
                }
            }
            break;
        default:
            break;
        }
    }
    s_sel_btn_last = sel_btn;
}

/* ---- Rotary encoder handling ---- */

static void handle_encoder(void)
{
    int16_t delta = encoder_get_delta();
    if (delta == 0) return;

    /* Encoder adjusts the active parameter based on mode */
    switch (g_status.mode) {
    case MODE_PULSE:
        /* Adjust pulse width */
        if (delta > 0) {
            g_status.pulse_width_ns += 1000;  /* +1 µs per click */
        } else {
            if (g_status.pulse_width_ns > 1000) {
                g_status.pulse_width_ns -= 1000;
            }
        }
        break;
    case MODE_DC:
        /* Adjust target current */
        if (delta > 0) {
            g_status.target_current_ma += 100;  /* +100 mA per click */
            if (g_status.target_current_ma > COIL_MAX_DC_CURRENT_MA) {
                g_status.target_current_ma = COIL_MAX_DC_CURRENT_MA;
            }
        } else {
            if (g_status.target_current_ma >= 100) {
                g_status.target_current_ma -= 100;
            }
        }
        break;
    case MODE_PROFILE:
        /* Cycle profile slot */
        if (delta > 0) {
            g_status.active_profile = (g_status.active_profile + 1) %
                                      EEPROM_NUM_PROFILES;
        } else {
            g_status.active_profile = (g_status.active_profile +
                                       EEPROM_NUM_PROFILES - 1) %
                                      EEPROM_NUM_PROFILES;
        }
        break;
    default:
        break;
    }
}

/* ---- Magnetometer update ---- */

static void update_magnetometers(void)
{
    mag_array_t reading;
    if (magnetometer_read_all(&reading) == 0) {
        /* Update global status (done inside magnetometer_read_all) */
    }
}

/* ---- 1-Wire coil tip identification ---- */

static uint8_t read_tip_id(void)
{
    /*
     * Read coil tip ID from DS2431 1-Wire EEPROM.
     * The DS2431 has a 256-byte EEPROM. We store the tip ID
     * and calibration data in the first few bytes.
     *
     * 1-Wire protocol:
     * 1. Reset pulse (pull low for 480 µs, release, wait 480 µs)
     * 2. ROM command (0x33 = Read ROM)
     * 3. Read 8 bytes (ROM ID + CRC)
     * 4. Memory command (0xF0 = Read Memory)
     * 5. Read tip ID from address 0x0000
     *
     * This is a simplified implementation.
     */
    /* TODO: implement full 1-Wire protocol */
    /* For now, return a default tip ID */
    return 1;  /* Standard tip */
}

/* ---- Default profile ---- */

static void init_default_profile(void)
{
    g_status.mode = MODE_IDLE;
    g_status.polarity = POLARITY_NORTH;
    g_status.pulse_width_ns = 100000;  /* 100 µs default */
    g_status.pulse_count = 1;
    g_status.pulse_fired = 0;
    g_status.delay_us = 1000;  /* 1 ms */
    g_status.target_current_ma = 5000;  /* 5 A default */
    g_status.active_profile = 0;
    g_status.error_code = ERR_NONE;
}

/* ---- Main function ---- */

int main(void)
{
    /* Disable interrupts during init */
    __asm volatile ("cpsid i" : : : "memory");

    /* Initialize clock to 170 MHz */
    clock_init();

    /* Initialize SysTick for 1 ms tick */
    systick_init();

    /* Initialize global status */
    memset(&g_status, 0, sizeof(g_status));
    init_default_profile();

    /* Initialize GPIO for buttons (input with pull-up) */
    gpio_config_input_pu(BTN_FIRE_PORT, BTN_FIRE_PIN);
    gpio_config_input_pu(BTN_MODE_PORT, BTN_MODE_PIN);
    gpio_config_input_pu(BTN_SELECT_PORT, BTN_SELECT_PIN);

    /* Initialize rotary encoder */
    encoder_init();

    /* Initialize OLED display */
    oled_init();
    oled_clear();
    oled_draw_string(0, 0, "MagLance v1.0");
    oled_draw_string(1, 0, "by jayis1");
    oled_draw_string(2, 0, "Initializing...");

    /* Initialize coil driver */
    coil_driver_init();

    /* Initialize magnetometers */
    if (magnetometer_init() != 0) {
        oled_draw_string(3, 0, "MAG ERROR!");
        g_status.error_code = ERR_INVALID_PARAM;
    } else {
        oled_draw_string(3, 0, "MAG OK");
    }

    /* Initialize profile manager (EEPROM) */
    if (profile_manager_init() != 0) {
        oled_draw_string(4, 0, "EEPROM ERR");
    } else {
        oled_draw_string(4, 0, "EEPROM OK");
    }

    /* Initialize USB interface */
    usb_iface_init();

    /* Read coil tip ID */
    g_status.tip_id = read_tip_id();

    /* Enable interrupts */
    __asm volatile ("cpsie i" : : : "memory");

    /* Startup display */
    oled_clear();
    oled_draw_string(0, 0, "MagLance Ready");
    delay_ms(1000);
    oled_clear();

    /* ---- Main loop ---- */
    while (1) {
        /* Process USB/BLE commands */
        usb_iface_poll();

        /* Handle user input */
        handle_buttons();
        handle_encoder();

        /* Safety monitoring (every 10 ms via counter) */
        if (s_safety_check_counter >= 10) {
            s_safety_check_counter = 0;
            safety_check();
        }

        /* Update magnetometers (every 10 ms = 100 Hz) */
        if (s_mag_update_counter >= 10) {
            s_mag_update_counter = 0;
            update_magnetometers();
        }

        /* Update PI controller (every 2 ms = 500 Hz) */
        if (s_pi_update_counter >= 2) {
            s_pi_update_counter = 0;
            coil_driver_pi_update();
        }

        /* Update display (every 100 ms = 10 Hz) */
        if (s_display_update_counter >= 100) {
            s_display_update_counter = 0;
            update_display();
        }

        /* Low-power wait for next interrupt */
        __asm volatile ("wfi" : : : "memory");
    }

    return 0;
}

/* ---- Interrupt handlers ---- */

/* HRTIM fault interrupt handler */
void HRTIM1_FLT_IRQHandler(void)
{
    /* Hardware fault detected — immediate shutdown */
    coil_driver_emergency_stop();
    g_status.error_code = ERR_HRTIM_FAULT;
    g_status.mode = MODE_ERROR;

    /* Clear fault flags */
    HRTIM1->ISR = 0xFFFF;
}

/* Hard fault handler */
void HardFault_Handler(void)
{
    while (1) {
        /* Lockup — flash error on display if possible */
    }
}

/* ---- Vector table (simplified) ---- */
/*
 * The actual vector table would be defined in a startup .s file.
 * Key entries:
 *   [0]  Initial stack pointer
 *   [1]  Reset_Handler → main()
 *   [15] SysTick_Handler
 *   [112] HRTIM1_Master_IRQHandler
 *   [118] HRTIM1_FLT_IRQHandler
 *   [29]  TIM3_IRQHandler (for encoder, if needed)
 */

/* ---- End of main.c ---- */