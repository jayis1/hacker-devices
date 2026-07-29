/*
 * main.c — NVMe-Phantom firmware entry point and command dispatcher
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Responsibilities:
 *   1. Boot orchestration: clocks, GPIO, PEX8606 switch, ECP5 FPGA bitstream
 *      load from W25Q128, BLE module reset, USB init, OLED init.
 *   2. Cooperative scheduler: poll BLE, poll USB, poll FPGA decoded-cmd FIFO,
 *      update OLED, update link state, sample battery ADC.
 *   3. Command dispatcher: decode BLE/USB commands and route to the
 *      appropriate driver (tlp_engine, pcie_switch, opal_probe, pcap, etc.).
 *   4. Safety: refuse to arm DMA / modify modes without two-stage confirmation;
 *      default to MODE_SAFE on boot; provide a hardware "panic" button that
 *      instantly disables all injection.
 *
 * Target: STM32H563ZIT6, bare-metal (no RTOS, no HAL).
 */

#include "board.h"
#include "registers.h"
#include "drivers/pcie_switch.h"
#include "drivers/tlp_engine.h"
#include "drivers/fpga_spi.h"
#include "drivers/ble_c2.h"
#include "drivers/sd_pcap.h"
#include "drivers/usb_cdc.h"
#include "drivers/opal_probe.h"
#include "drivers/storage.h"
#include <string.h>

/* ---- Global state ------------------------------------------------------ */

board_state_t g_state = {
    .mode = MODE_SAFE,
    .link = LINK_DOWN,
    .link_width = 0,
    .capture_count = 0,
    .inject_count = 0,
    .sd_free_kb = 0,
    .battery_mv = 0,
    .charging = 0,
    .fpga_ready = 0,
    .switch_ready = 0,
    .uptime_s = 0,
};

/* ---- SysTick (1 ms tick) ----------------------------------------------- */

static volatile uint32_t s_ticks = 0;

void systick_init(void)
{
    SYST_RVR = BOARD_TICKS_PER_MS - 1;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

/* SysTick interrupt handler */
void SysTick_Handler(void)
{
    s_ticks++;
    if ((s_ticks % 1000) == 0) g_state.uptime_s++;
}

static uint32_t ticks(void) { return s_ticks; }

/* ---- Clock & GPIO init ------------------------------------------------- */

void board_clock_init(void)
{
    /* Enable HSE and wait for ready */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY)) { }
    /* Configure PLL1: input = HSE 16 MHz, VCO = 16 * 125 = 2000 MHz,
     * SYSCLK = VCO / 8 = 250 MHz. */
    RCC_PLL1CFGR = (16U << 0) | (125U << 8) | (8U << 20) | (1U << 24);
    RCC_PLL1DIVR = 0;
    RCC_CR |= RCC_CR_PLL1ON;
    while (!(RCC_CR & RCC_CR_PLL1RDY)) { }
    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (3U << 0);                       /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 0x7) != 3) { }
    /* Enable peripheral clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB |
                   RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD;
}

void board_gpio_init(void)
{
    /* Status / error LEDs on PA0/PA1 (output) */
    volatile uint32_t *gpioa_moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpioa_odr   = (volatile uint32_t *)(GPIOA_BASE + GPIO_ODR_OFF);
    *gpioa_moder |=  (1U << (0*2)) | (1U << (1*2));   /* output mode */
    *gpioa_odr   |=  (1U << 0);                        /* status LED on at boot */
    *gpioa_odr   &= ~(1U << 1);                        /* error LED off */
    /* User button PC13 (input, pull-up) */
    volatile uint32_t *gpioc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpioc_pupdr = (volatile uint32_t *)(GPIOC_BASE + GPIO_PUPDR_OFF);
    *gpioc_moder &= ~(3U << (13*2));
    *gpioc_pupdr |=  (1U << (13*2));                   /* pull-up */
    /* PEX LINKOK PA8 (input) */
    *gpioa_moder &= ~(3U << (8*2));
}

void board_state_init(void)
{
    g_state.mode = MODE_SAFE;
    g_state.link = LINK_DOWN;
    g_state.fpga_ready = 0;
    g_state.switch_ready = 0;
}

/* ---- OLED (SSD1306) — minimal I²C display ------------------------------ */

static void oled_i2c_init(void)
{
    RCC_APB1LENR |= RCC_APB1LENR_I2C2;
    volatile uint32_t *gpiob_moder = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpiob_afrl  = (volatile uint32_t *)(GPIOB_BASE + GPIO_AFRL_OFF);
    *gpiob_moder &= ~((3U << (10*2)) | (3U << (11*2)));
    *gpiob_moder |=  ((2U << (10*2)) | (2U << (11*2)));
    *gpiob_afrl  &= ~((0xFU << (10*4)) | (0xFU << (11*4)));
    *gpiob_afrl  |=  ((4U  << (10*4)) | (4U  << (11*4)));
    volatile uint32_t *i2c_cr1 = (volatile uint32_t *)(I2C2_BASE + I2C_CR1);
    volatile uint32_t *i2c_timingr = (volatile uint32_t *)(I2C2_BASE + 0x10);
    *i2c_cr1 = 0;
    *i2c_timingr = 0x10B17DB5U;
    *i2c_cr1 = I2C_CR1_PE;
}

static void oled_cmd(uint8_t cmd)
{
    volatile uint32_t *cr2  = (volatile uint32_t *)(I2C2_BASE + I2C_CR2);
    volatile uint32_t *isr  = (volatile uint32_t *)(I2C2_BASE + I2C_ISR);
    volatile uint32_t *txdr = (volatile uint32_t *)(I2C2_BASE + I2C_TXDR);
    volatile uint32_t *icr  = (volatile uint32_t *)(I2C2_BASE + I2C_ICR);
    *cr2 = ((uint32_t)SSD1306_I2C_ADDR << 1) | (2U << 16) | I2C_CR2_START;
    while (!(*isr & I2C_ISR_TXE)) { }
    *txdr = 0x00;                               /* Co=0, D/C#=0 (command) */
    while (!(*isr & I2C_ISR_TXE)) { }
    *txdr = cmd;
    while (!(*isr & I2C_ISR_TC)) { }
    *cr2 |= I2C_CR2_STOP;
    *icr = 0x3FFF;
}

static void oled_init(void)
{
    oled_i2c_init();
    board_delay_ms(50);
    oled_cmd(0xAE);                             /* display off */
    oled_cmd(0xD5); oled_cmd(0x80);             /* clock divide */
    oled_cmd(0xA8); oled_cmd(0x3F);             /* multiplex 1/64 */
    oled_cmd(0xD3); oled_cmd(0x00);             /* display offset */
    oled_cmd(0x40);                             /* start line 0 */
    oled_cmd(0x8D); oled_cmd(0x14);             /* charge pump on */
    oled_cmd(0x20); oled_cmd(0x00);             /* horizontal addressing */
    oled_cmd(0xA1);                             /* segment remap */
    oled_cmd(0xC8);                             /* COM scan remap */
    oled_cmd(0xDA); oled_cmd(0x12);             /* COM pins */
    oled_cmd(0x81); oled_cmd(0xCF);             /* contrast */
    oled_cmd(0xD9); oled_cmd(0xF1);             /* pre-charge */
    oled_cmd(0xDB); oled_cmd(0x40);             /* VCOMH deselect */
    oled_cmd(0xA4); oled_cmd(0xA6);             /* normal display */
    oled_cmd(0xAF);                             /* display on */
}

static void oled_clear(void)
{
    oled_cmd(0x21); oled_cmd(0);   oled_cmd(127); /* col addr */
    oled_cmd(0x22); oled_cmd(0);   oled_cmd(7);   /* page addr */
    volatile uint32_t *cr2  = (volatile uint32_t *)(I2C2_BASE + I2C_CR2);
    volatile uint32_t *isr  = (volatile uint32_t *)(I2C2_BASE + I2C_ISR);
    volatile uint32_t *txdr = (volatile uint32_t *)(I2C2_BASE + I2C_TXDR);
    volatile uint32_t *icr  = (volatile uint32_t *)(I2C2_BASE + I2C_ICR);
    for (int page = 0; page < 8; page++) {
        oled_cmd(0xB0 + page);
        oled_cmd(0x00); oled_cmd(0x10);
        *cr2 = ((uint32_t)SSD1306_I2C_ADDR << 1) | (129U << 16) | I2C_CR2_START;
        while (!(*isr & I2C_ISR_TXE)) { }
        *txdr = 0x40;                           /* Co=0, D/C#=1 (data) */
        for (int col = 0; col < 128; col++) {
            while (!(*isr & I2C_ISR_TXE)) { }
            *txdr = 0x00;
        }
        while (!(*isr & I2C_ISR_TC)) { }
        *cr2 |= I2C_CR2_STOP;
        *icr = 0x3FFF;
    }
}

/* Minimal 5x8 font for OLED status display (digits + uppercase A-Z + space) */
static const uint8_t s_font5x8[][5] = {
    ['0']={0x3E,0x51,0x49,0x45,0x3E}, ['1']={0x00,0x42,0x7F,0x40,0x00},
    ['2']={0x42,0x61,0x51,0x49,0x46}, ['3']={0x21,0x41,0x45,0x4B,0x31},
    ['4']={0x18,0x14,0x12,0x7F,0x10}, ['5']={0x27,0x45,0x45,0x45,0x39},
    ['6']={0x3C,0x4A,0x49,0x49,0x30}, ['7']={0x01,0x71,0x09,0x05,0x03},
    ['8']={0x36,0x49,0x49,0x49,0x36}, ['9']={0x06,0x49,0x49,0x29,0x1E},
    ['A']={0x7E,0x11,0x11,0x11,0x7E}, ['B']={0x7F,0x49,0x49,0x49,0x36},
    ['C']={0x3E,0x41,0x41,0x41,0x22}, ['D']={0x7F,0x41,0x41,0x22,0x1C},
    ['E']={0x7F,0x49,0x49,0x49,0x41}, ['F']={0x7F,0x09,0x09,0x09,0x01},
    ['G']={0x3E,0x41,0x49,0x49,0x7A}, ['H']={0x7F,0x08,0x08,0x08,0x7F},
    ['I']={0x00,0x41,0x7F,0x41,0x00}, ['J']={0x20,0x40,0x41,0x3F,0x01},
    ['K']={0x7F,0x08,0x14,0x22,0x41}, ['L']={0x7F,0x40,0x40,0x40,0x40},
    ['M']={0x7F,0x02,0x0C,0x02,0x7F}, ['N']={0x7F,0x04,0x08,0x10,0x7F},
    ['O']={0x3E,0x41,0x41,0x41,0x3E}, ['P']={0x7F,0x09,0x09,0x09,0x06},
    ['R']={0x7F,0x09,0x19,0x29,0x46}, ['S']={0x46,0x49,0x49,0x49,0x31},
    ['T']={0x01,0x01,0x7F,0x01,0x01}, ['U']={0x3F,0x40,0x40,0x40,0x3F},
    ['V']={0x1F,0x20,0x40,0x20,0x1F}, ['W']={0x3F,0x40,0x38,0x40,0x3F},
    ['X']={0x63,0x14,0x08,0x14,0x63}, ['Y']={0x07,0x08,0x70,0x08,0x07},
    ['Z']={0x61,0x51,0x49,0x45,0x43}, [' ']={0,0,0,0,0},
    [':']={0,0x36,0x36,0,0}, ['-']={0x08,0x08,0x08,0x08,0x08},
    ['/']={0x20,0x10,0x08,0x04,0x02}, ['=']={0x14,0x14,0x14,0x14,0x14},
    ['<']={0x08,0x14,0x22,0x41,0x00}, ['>']={0x00,0x41,0x22,0x14,0x08},
};

static void oled_draw_str(int page, int col, const char *s)
{
    volatile uint32_t *cr2  = (volatile uint32_t *)(I2C2_BASE + I2C_CR2);
    volatile uint32_t *isr  = (volatile uint32_t *)(I2C2_BASE + I2C_ISR);
    volatile uint32_t *txdr = (volatile uint32_t *)(I2C2_BASE + I2C_TXDR);
    volatile uint32_t *icr  = (volatile uint32_t *)(I2C2_BASE + I2C_ICR);
    oled_cmd(0xB0 + page);
    oled_cmd(0x00 + (col & 0x0F));
    oled_cmd(0x10 + ((col >> 4) & 0x0F));
    *cr2 = ((uint32_t)SSD1306_I2C_ADDR << 1) | ((1U + (uint32_t)strlen(s) * 5) << 16) | I2C_CR2_START;
    while (!(*isr & I2C_ISR_TXE)) { }
    *txdr = 0x40;
    for (const char *p = s; *p; p++) {
        uint8_t ch = (uint8_t)*p;
        if (ch >= sizeof(s_font5x8)/sizeof(s_font5x8[0]) || !s_font5x8[ch][0]) ch = ' ';
        for (int b = 0; b < 5; b++) {
            while (!(*isr & I2C_ISR_TXE)) { }
            *txdr = s_font5x8[ch][b];
        }
        while (!(*isr & I2C_ISR_TXE)) { }
        *txdr = 0x00;                           /* 1px spacing */
    }
    while (!(*isr & I2C_ISR_TC)) { }
    *cr2 |= I2C_CR2_STOP;
    *icr = 0x3FFF;
}

static void oled_status_update(void)
{
    char line[22];
    oled_clear();
    /* Line 0: device name */
    oled_draw_str(0, 0, "NVME PHANTOM");
    /* Line 1: mode */
    static const char *mode_names[] = {
        "TAP", "MITM", "OPAL", "SPOOF", "DMA", "FWEXT", "HPLUG", "EXFIL", "SAFE"
    };
    snprintf(line, sizeof(line), "MODE:%s", mode_names[g_state.mode]);
    oled_draw_str(1, 0, line);
    /* Line 2: link */
    snprintf(line, sizeof(line), "LINK:GEN%d X%u",
             (g_state.link == LINK_DOWN) ? 0 : g_state.link, g_state.link_width);
    oled_draw_str(2, 0, line);
    /* Line 3: capture / inject counters */
    snprintf(line, sizeof(line), "CAP:%lu INJ:%lu",
             (unsigned long)g_state.capture_count, (unsigned long)g_state.inject_count);
    oled_draw_str(3, 0, line);
    /* Line 4: SD free */
    snprintf(line, sizeof(line), "SD:%luKB", (unsigned long)g_state.sd_free_kb);
    oled_draw_str(4, 0, line);
    /* Line 5: battery */
    snprintf(line, sizeof(line), "BAT:%dmV %s",
             g_state.battery_mv, g_state.charging ? "CHG" : "   ");
    oled_draw_str(5, 0, line);
    /* Line 6: FPGA / switch ready */
    oled_draw_str(6, 0, g_state.fpga_ready ? "FPGA:OK " : "FPGA:ERR");
    oled_draw_str(6, 64, g_state.switch_ready ? "SW:OK" : "SW:ERR");
    /* Line 7: uptime */
    snprintf(line, sizeof(line), "UP:%lus", (unsigned long)g_state.uptime_s);
    oled_draw_str(7, 0, line);
}

/* ---- ADC (battery monitor) --------------------------------------------- */

static uint16_t adc_read_vbat(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_ADC1;
    volatile uint32_t *adc_cr  = (volatile uint32_t *)(ADC1_BASE + ADC_CR);
    volatile uint32_t *adc_isr = (volatile uint32_t *)(ADC1_BASE + ADC_ISR);
    volatile uint32_t *adc_cfgr= (volatile uint32_t *)(ADC1_BASE + ADC_CFGR);
    volatile uint32_t *adc_sqr1= (volatile uint32_t *)(ADC1_BASE + ADC_SQR1);
    volatile uint32_t *adc_dr  = (volatile uint32_t *)(ADC1_BASE + ADC_DR);
    *adc_cr = 0;
    board_delay_ms(1);
    *adc_cr |= ADC_CR_ADVREGEN;                 /* enable regulator */
    board_delay_ms(1);
    *adc_cr |= ADC_CR_ADEN;
    while (!(*adc_isr & ADC_ISR_ADRDY)) { }
    *adc_cfgr = 0;                              /* single conversion, 12-bit */
    *adc_sqr1 = (VBAT_ADC_CHAN << 6) | (1U << 0); /* 1 conversion, channel = VBAT */
    *adc_cr |= ADC_CR_ADSTART;
    while (!(*adc_isr & ADC_ISR_EOC)) { }
    uint16_t raw = (uint16_t)(*adc_dr & 0xFFF);
    /* Convert: Vref = 3.3 V, divider = 1/3, so Vbat = raw/4095 * 3.3 * 3 */
    uint32_t mv = ((uint32_t)raw * 3300UL * VBAT_DIVIDER) / 4095UL;
    return (uint16_t)mv;
}

/* ---- DMA two-stage arm (safety interlock) ------------------------------ */

static struct {
    uint64_t host_phys;
    uint32_t len;
    uint8_t  dir;
    uint8_t  armed;
    uint32_t arm_token;
} s_dma;

static uint32_t s_dma_next_token = 0x5A5A0001;

/* ---- Command dispatcher ------------------------------------------------ */

static void dispatch_ble_cmd(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    uint8_t resp[248];
    uint16_t rlen = 0;
    switch (cmd) {
    case BLE_CMD_PING:
        resp[0] = 0x01; rlen = 1;                 /* version byte */
        break;
    case BLE_CMD_GET_STATUS: {
        resp[0] = (uint8_t)g_state.mode;
        resp[1] = (uint8_t)g_state.link;
        resp[2] = g_state.link_width;
        resp[3] = (uint8_t)(g_state.capture_count & 0xFF);
        resp[4] = (uint8_t)(g_state.capture_count >> 8);
        resp[5] = (uint8_t)(g_state.sd_free_kb & 0xFF);
        resp[6] = (uint8_t)(g_state.sd_free_kb >> 8);
        resp[7] = (uint8_t)(g_state.battery_mv & 0xFF);
        resp[8] = (uint8_t)(g_state.battery_mv >> 8);
        resp[9] = g_state.charging;
        rlen = 10;
        break;
    }
    case BLE_CMD_SET_MODE: {
        uint8_t mode = payload[0];
        if (mode >= MODE_COUNT) { resp[0]=0xFF; rlen=1; break; }
        g_state.mode = (board_mode_t)mode;
        tlp_engine_set_mode(g_state.mode);
        /* Arm modify override only in modes that need it */
        if (mode == MODE_NVME_MITM || mode == MODE_CTRL_SPOOF ||
            mode == MODE_DMA_BRIDGE || mode == MODE_COVERT_EXFIL) {
            tlp_engine_arm_modify();
        } else {
            tlp_engine_disarm_modify();
        }
        resp[0] = 0; rlen = 1;
        break;
    }
    case BLE_CMD_START_CAP: {
        uint32_t sid = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8);
        int rc = pcap_open(sid);
        resp[0] = (uint8_t)(rc == 0 ? sid : 0xFF); rlen = 1;
        break;
    }
    case BLE_CMD_STOP_CAP: {
        uint32_t recs, dur;
        pcap_close(&recs, &dur);
        resp[0] = (uint8_t)(recs & 0xFF);
        resp[1] = (uint8_t)(recs >> 8);
        resp[2] = (uint8_t)(dur & 0xFF);
        resp[3] = (uint8_t)(dur >> 8);
        rlen = 4;
        break;
    }
    case BLE_CMD_LOAD_RULES: {
        if (len == 32) {
            tlp_engine_load_rule(payload);
            resp[0] = 0;
        } else { resp[0] = 0xFF; }
        rlen = 1;
        break;
    }
    case BLE_CMD_CLEAR_RULES:
        tlp_engine_clear_rules();
        resp[0] = 0; rlen = 1;
        break;
    case BLE_CMD_INJECT_CMD: {
        if (len == 64) {
            tlp_engine_inject_sq(payload);
            resp[0] = 0;
        } else { resp[0] = 0xFF; }
        rlen = 1;
        break;
    }
    case BLE_CMD_SPOOF_IDENT: {
        if (len == 4096) {
            tlp_engine_spoof_ident(payload);
            resp[0] = 0;
        } else { resp[0] = 0xFF; }
        rlen = 1;
        break;
    }
    case BLE_CMD_DMA_ARM: {
        /* Two-stage: ARM records the request, returns a token.  GO uses it. */
        s_dma.host_phys = 0;
        for (int i = 0; i < 8; i++)
            s_dma.host_phys |= ((uint64_t)payload[i]) << (i*8);
        s_dma.len = (uint32_t)payload[8] | ((uint32_t)payload[9] << 8) |
                    ((uint32_t)payload[10] << 16) | ((uint32_t)payload[11] << 24);
        s_dma.dir = payload[12];
        s_dma.armed = 1;
        s_dma.arm_token = s_dma_next_token++;
        resp[0] = (uint8_t)(s_dma.arm_token & 0xFF);
        resp[1] = (uint8_t)(s_dma.arm_token >> 8);
        resp[2] = (uint8_t)(s_dma.arm_token >> 16);
        resp[3] = (uint8_t)(s_dma.arm_token >> 24);
        rlen = 4;
        break;
    }
    case BLE_CMD_DMA_GO: {
        uint32_t token = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8) |
                         ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
        if (!s_dma.armed || token != s_dma.arm_token) {
            resp[0] = 0xEE; rlen = 1;             /* not armed / bad token */
            break;
        }
        uint16_t cid;
        int rc = tlp_engine_dma_build(s_dma.host_phys, s_dma.len, s_dma.dir,
                                      0x100, &cid);
        resp[0] = (uint8_t)(rc == 0 ? 0 : 0xFF);
        resp[1] = (uint8_t)(cid & 0xFF);
        resp[2] = (uint8_t)(cid >> 8);
        rlen = 3;
        s_dma.armed = 0;                          /* one-shot */
        break;
    }
    case BLE_CMD_HOTPLUG: {
        pcie_switch_inject_hotplug(payload[0]);
        resp[0] = 0; rlen = 1;
        break;
    }
    case BLE_CMD_OPAL_SEND: {
        /* Build an Opal UNLOCK method call and inject as Security Send */
        uint8_t opal_payload[256];
        int plen = opal_build_unlock(opal_payload, sizeof(opal_payload),
                                     (const char *)payload);
        uint8_t sq[64];
        opal_build_security_send(sq, 1, opal_payload, (uint16_t)plen);
        tlp_engine_inject_sq(sq);
        resp[0] = 0; rlen = 1;
        break;
    }
    case BLE_CMD_FW_DOWNLOAD: {
        /* Chunked firmware download to W25Q128 (offset 0x000000) */
        static uint32_t fw_offset = 0;
        if (len >= 4 && payload[0] == 0xFF && payload[1] == 0xFF &&
            payload[2] == 0xFF && payload[3] == 0xFF) {
            fw_offset = 0;                        /* reset signal */
            resp[0] = 0; rlen = 1;
        } else {
            storage_write(NOR_OFFSET_BITSTREAM + fw_offset, payload, len);
            fw_offset += len;
            resp[0] = 0; rlen = 1;
        }
        break;
    }
    case BLE_CMD_FW_COMMIT: {
        /* Reboot the FPGA with the new bitstream */
        uint32_t off, blen;
        storage_load_bitstream_offset(&off, &blen);
        fpga_load_from_nor(off);
        resp[0] = g_state.fpga_ready ? 0 : 0xFF; rlen = 1;
        break;
    }
    default:
        resp[0] = 0xFE; rlen = 1;                 /* unknown command */
    }
    ble_send(cmd, resp, rlen);
}

/* ---- Panic button (PC13) ----------------------------------------------- */

static uint8_t s_btn_debounce = 0;
static void check_panic_button(void)
{
    volatile uint32_t *gpioc_idr = (volatile uint32_t *)(GPIOC_BASE + GPIO_IDR_OFF);
    if (!(*gpioc_idr & (1U << BTN_USER_PIN))) {
        if (s_btn_debounce < 10) s_btn_debounce++;
        if (s_btn_debounce == 10) {
            /* PANIC: disable all injection, go to SAFE mode */
            tlp_engine_disarm_modify();
            tlp_engine_clear_rules();
            pcie_switch_safe_mode();
            g_state.mode = MODE_SAFE;
        }
    } else {
        s_btn_debounce = 0;
    }
}

/* ---- Main loop --------------------------------------------------------- */

int main(void)
{
    board_clock_init();
    board_gpio_init();
    board_state_init();
    systick_init();
    oled_init();
    oled_clear();
    oled_draw_str(0, 0, "NVME PHANTOM");
    oled_draw_str(1, 0, "BOOTING...");

    /* 1. Init W25Q128 NOR and load ECP5 bitstream */
    if (storage_init() != 0) {
        oled_draw_str(2, 0, "NOR:FAIL");
        /* continue without FPGA — passive-only */
    } else {
        uint32_t bs_off, bs_len;
        if (storage_load_bitstream_offset(&bs_off, &bs_len) == 0) {
            fpga_load_from_nor(bs_off);
        }
    }

    /* 2. Init PEX8606 PCIe switch */
    if (pcie_switch_init() != 0) {
        oled_draw_str(2, 0, "PEX:FAIL");
    }

    /* 3. Init FPGA TLP engine */
    if (g_state.fpga_ready) {
        tlp_engine_init();
        tlp_engine_set_mode(MODE_SAFE);
    }

    /* 4. Init BLE + USB */
    ble_init();
    usb_cdc_init();

    /* 5. Enter main loop */
    uint32_t last_oled = 0;
    uint32_t last_adc  = 0;
    oled_draw_str(1, 0, "READY     ");

    while (1) {
        /* Poll BLE */
        uint8_t cmd, payload[244];
        uint16_t plen = sizeof(payload);
        if (ble_poll(&cmd, &plen, payload, plen) == 0) {
            dispatch_ble_cmd(cmd, payload, plen);
        }

        /* Poll USB */
        usb_cdc_poll();

        /* Poll FPGA decoded-cmd FIFO (in tap modes) */
        if (g_state.fpga_ready &&
            (g_state.mode == MODE_PASSIVE_TAP || g_state.mode == MODE_FW_EXTRACT ||
             g_state.mode == MODE_OPAL_INTERROG)) {
            nvme_cmd_t cmd_dec;
            if (tlp_engine_read_decoded(&cmd_dec) == 0) {
                char line[24];
                nvme_format_cmd(&cmd_dec, line, sizeof(line));
                /* Log to SD if capture is open */
                nvme_cpl_t cpl_dummy = {0};
                pcap_write_cmd(&cmd_dec, &cpl_dummy,
                               (uint64_t)g_state.uptime_s * 1000000000ULL,
                               NULL, 0);
            }
        }

        /* Update link state (every 500 ms) */
        if ((ticks() - last_adc) > 500) {
            last_adc = ticks();
            g_state.link = pcie_switch_link_state();
            g_state.link_width = pcie_switch_link_width();
            g_state.battery_mv = adc_read_vbat();
            g_state.sd_free_kb = pcap_sd_free_kb();
        }

        /* Update OLED (every 250 ms) */
        if ((ticks() - last_oled) > 250) {
            last_oled = ticks();
            oled_status_update();
        }

        /* Panic button */
        check_panic_button();

        /* Error LED if something is wrong */
        volatile uint32_t *gpioa_odr = (volatile uint32_t *)(GPIOA_BASE + GPIO_ODR_OFF);
        if (!g_state.switch_ready || !g_state.fpga_ready) {
            *gpioa_odr |=  (1U << LED_ERROR_PIN);   /* error LED on */
        } else {
            *gpioa_odr &= ~(1U << LED_ERROR_PIN);
        }
        /* Blink status LED every 500 ms */
        if ((ticks() % 1000) < 500) {
            *gpioa_odr |=  (1U << LED_STATUS_PIN);
        } else {
            *gpioa_odr &= ~(1U << LED_STATUS_PIN);
        }
    }
    return 0;
}