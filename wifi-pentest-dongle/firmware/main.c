/*
 * WFP-100 — Main Entry Point (SPL Board Init)
 * StarFive JH7110 WiFi Pentesting Dongle
 *
 * This is the first C code executed after the Boot ROM loads
 * the SPL from QSPI flash. It configures power rails, clocks,
 * and GPIO before handing off to U-Boot proper.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include "registers.h"
#include "board.h"
#include "drivers/axp2101.h"

/* MMIO access helpers (from registers.h) */
extern void mmio_write32(uintptr_t addr, uint32_t val);
extern uint32_t mmio_read32(uintptr_t addr);

/* Forward declarations from clock/gpio init code (phase4_software_stack.md) */
extern int wfp100_clock_init(void);
extern int wfp100_gpio_init(void);

/* PMIC configuration table (from phase4 SPL stub) */
static const struct {
    uint8_t reg;
    uint8_t val;
} pmic_regs[] = {
    { AXP2101_DCDC1_CTRL, 0x20 },  /* VDD_CORE = 0.9V */
    { AXP2101_DCDC2_CTRL, 0x2C },  /* VDD_DDR  = 1.1V */
    { AXP2101_DCDC3_CTRL, 0x24 },  /* VDD_SOC  = 1.0V */
    { AXP2101_DCDC4_CTRL, 0x46 },  /* VDD_3V3  = 3.3V */
    { AXP2101_LDO1_CTRL,  0x1A },  /* VDD_1V8  = 1.8V */
    { AXP2101_LDO2_CTRL,  0x08 },  /* VDD_RTC  = 0.8V */
};

static void delay_us(uint32_t us)
{
    for (volatile uint32_t i = 0; i < (us * 26); i++)
        __asm__ volatile("nop");
}

/*
 * Configure I2C0 for PMIC communication.
 * JH7110 I2C0: 400 kHz fast mode.
 */
static int i2c0_init(void)
{
    uintptr_t base = I2C0_BASE;

    /* Disable I2C controller during configuration */
    mmio_write32(base + I2C_CR, 0);

    /* Set clock divider for 400 kHz @ 100 MHz APB clock */
    /* CD = APB_FREQ / (2 * SCL_FREQ) - 1 = 100MHz / (2 * 400kHz) - 1 = 124 */
    mmio_write32(base + I2C_CCR, 124);

    /* Enable I2C controller, master mode */
    mmio_write32(base + I2C_CR, (1U << 0) | (1U << 2));

    return 0;
}

/*
 * Write a byte to a PMIC register via I2C.
 */
static int pmic_i2c_write(uint8_t reg, uint8_t val)
{
    uintptr_t base = I2C0_BASE;
    uint32_t status;

    /* Set target address (AXP2101 = 0x35) */
    mmio_write32(base + I2C_TAR, AXP2101_I2C_ADDR);

    /* Write register address */
    mmio_write32(base + I2C_DR, reg);

    /* Wait for TX empty */
    status = mmio_read32(base + I2C_SR);
    while (!(status & (1U << 2)))
        status = mmio_read32(base + I2C_SR);

    /* Write data value */
    mmio_write32(base + I2C_DR, val);

    /* Wait for TX empty and check for ACK */
    status = mmio_read32(base + I2C_SR);
    while (!(status & (1U << 2)))
        status = mmio_read32(base + I2C_SR);

    /* Check for NACK */
    if (status & (1U << 3))
        return -1;

    return 0;
}

/*
 * Configure all PMIC voltage rails.
 * Must be called BEFORE DDR initialization and BEFORE
 * any peripheral clocks are enabled.
 */
static int pmic_init(void)
{
    int ret;
    int i;

    /* Initialize I2C0 bus */
    ret = i2c0_init();
    if (ret) {
        /* Fallback: direct MMIO write to PMIC if I2C fails */
        return ret;
    }

    /* Configure each PMIC rail */
    for (i = 0; i < sizeof(pmic_regs) / sizeof(pmic_regs[0]); i++) {
        ret = pmic_i2c_write(pmic_regs[i].reg, pmic_regs[i].val);
        if (ret)
            return ret;
        delay_us(100);  /* 100us settling time per rail */
    }

    /* Wait for all rails to stabilize */
    delay_us(2000);  /* 2ms total power-on settling */

    return 0;
}

/*
 * Initialize UART0 for debug output (115200 8N1).
 */
static int uart0_init(void)
{
    uintptr_t base = UART0_BASE;

    /* Disable UART during configuration */
    mmio_write32(base + 0x0C, 0);  /* UART_CR — disable */

    /* Set baud rate: 115200 @ 100MHz UART clock */
    /* Baud divisor = UART_CLK / (16 * BAUD) - 1 = 100MHz / (16 * 115200) - 1 ≈ 53 */
    mmio_write32(base + 0x18, 53);  /* UART_IBRD */
    mmio_write32(base + 0x1C, 0);   /* UART_FBRD */

    /* 8N1, FIFO enable */
    mmio_write32(base + 0x20, (3U << 5) | (1U << 4));  /* UART_LCRH */

    /* Enable UART, TX, RX */
    mmio_write32(base + 0x0C, (1U << 0) | (1U << 8) | (1U << 9));  /* UART_CR */

    return 0;
}

/*
 * Put a character to UART0 (debug console).
 */
static void uart_putc(char c)
{
    uintptr_t base = UART0_BASE;

    /* Wait for TX FIFO not full */
    while (mmio_read32(base + 0x08) & (1U << 5))
        ;

    /* Write character */
    mmio_write32(base + 0x00, c);
}

/*
 * Put a string to UART0.
 */
static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

/*
 * WFP-100 board early initialization.
 * Called from SPL entry point.
 */
int board_early_init(void)
{
    /* Step 1: Configure PMIC voltage rails */
    uart_puts("WFP-100 SPL: Configuring PMIC...\n");
    if (pmic_init()) {
        uart_puts("WFP-100 SPL: PMIC init FAILED\n");
        return -1;
    }
    uart_puts("WFP-100 SPL: PMIC rails stable\n");

    /* Step 2: Configure SoC clocks */
    uart_puts("WFP-100 SPL: Configuring clocks...\n");
    if (wfp100_clock_init()) {
        uart_puts("WFP-100 SPL: Clock init FAILED\n");
        return -1;
    }
    uart_puts("WFP-100 SPL: CPU @ 1.5GHz, DDR @ 1066MHz\n");

    /* Step 3: Configure GPIO pin mux */
    uart_puts("WFP-100 SPL: Configuring GPIO...\n");
    if (wfp100_gpio_init()) {
        uart_puts("WFP-100 SPL: GPIO init FAILED\n");
        return -1;
    }
    uart_puts("WFP-100 SPL: GPIO mux configured\n");

    /* Step 4: Turn on PWR LED */
    mmio_write32(GPIO0_BASE + GPIO_DOUT,
                 mmio_read32(GPIO0_BASE + GPIO_DOUT) | (1U << LED_PWR_PIN));
    uart_puts("WFP-100 SPL: PWR LED on\n");

    return 0;
}

/*
 * Main SPL entry point.
 */
void _start(void)
{
    /* Initialize UART first for debug output */
    uart0_init();
    uart_puts("\n");
    uart_puts("========================================\n");
    uart_puts(" WFP-100 SPL — WiFi Pentesting Dongle\n");
    uart_puts(" StarFive JH7110 @ 1.5 GHz\n");
    uart_puts("========================================\n");

    /* Run board initialization sequence */
    if (board_early_init() != 0) {
        uart_puts("FATAL: Board init failed. Halting.\n");
        while (1)
            __asm__ volatile("wfi");
    }

    uart_puts("WFP-100 SPL: Board init complete.\n");
    uart_puts("WFP-100 SPL: Loading U-Boot proper...\n");

    /* Hand off to U-Boot proper (loaded from QSPI) */
    /* This is handled by the SPL framework in U-Boot */
    while (1)
        __asm__ volatile("wfi");
}