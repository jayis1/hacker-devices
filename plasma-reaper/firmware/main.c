/*
 * main.c — Main firmware for PlasmaReaper
 * Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This is the main control loop for the PlasmaReaper fault injection
 * tool. It initializes all hardware, loads the FPGA bitstream, and
 * runs the super-loop that processes host commands, manages sweeps,
 * and coordinates glitch firing.
 */

#include "board.h"
#include "registers.h"

/* Driver headers */
#include "drivers/hrtim_glitch.h"
#include "drivers/fpga_if.h"
#include "drivers/power_glitch.h"
#include "drivers/clock_glitch.h"
#include "drivers/em_pulse.h"
#include "drivers/trigger.h"
#include "drivers/uart_monitor.h"
#include "drivers/adc_capture.h"
#include "drivers/usb_cdc.h"
#include "drivers/ble_if.h"
#include "drivers/sdcard.h"
#include "drivers/oled.h"
#include "drivers/protocol.h"

/* ---- Global state -------------------------------------------------- */

static glitch_params_t  g_current_params;
static trigger_config_t g_trigger_config;
static pattern_config_t g_patterns;
static glitch_result_t  g_last_result;
static sweep_config_t   g_sweep_config;
static sweep_status_t   g_sweep_status;

/* Statistics counters */
static uint32_t g_total_shots;
static uint32_t g_success_shots;
static uint32_t g_failure_shots;
static uint32_t g_no_response_shots;
static uint32_t g_invalid_shots;

/* Boot timestamp (SysTick ticks, 1 ms each) */
static volatile uint32_t g_systick_ms;

/* ---- SysTick handler (1 ms) ---------------------------------------- */

void SysTick_Handler(void)
{
    g_systick_ms++;
}

uint32_t board_millis(void)
{
    return g_systick_ms;
}

void board_delay_ms(uint32_t ms)
{
    uint32_t start = g_systick_ms;
    while ((g_systick_ms - start) < ms) {
        /* wait */
    }
}

/* ---- Board initialization ------------------------------------------ */

void board_init(void)
{
    /*
     * Basic clock setup: configure PLL for 550 MHz system clock.
     * In a real build this would use the full STM32 HAL SystemClock_Config;
     * here we outline the key steps.
     *
     * HSE = 8 MHz (external crystal on the PlasmaReaper board)
     * PLL: M=1, N=137, P=2 → VCO=1100 MHz, SYSCLK=550 MHz
     */
    /* Enable HSE */
    RCC_CR |= BIT(16);             /* HSEON */
    while (!(RCC_CR & BIT(17)))    /* wait HSERDY */
        ;

    /* Configure PLL */
    RCC_PLLCFGR = (1U << 0)       /* M = 1 */
                | (137U << 8)     /* N = 137 */
                | (0U << 16)      /* P = 2 (00 → /2) */
                | BIT(24);        /* PLLSRC = HSE */

    RCC_CR |= BIT(24);             /* PLLON */
    while (!(RCC_CR & BIT(25)))    /* wait PLLRDY */
        ;

    /* Set flash latency for 550 MHz (5 wait states) */
    /* FLASH_ACR = 5; — omitted for brevity, see FLASH_BASE+0x00 */

    /* Switch SYSCLK to PLL */
    RCC_CFGR = (3U << 0);          /* SW = PLL */
    while (((RCC_CFGR >> 2) & 3) != 3)  /* wait SWS = PLL */
        ;

    /* Enable peripheral clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOA_EN | RCC_AHB4ENR_GPIOB_EN
                 | RCC_AHB4ENR_GPIOC_EN | RCC_AHB4ENR_GPIOD_EN
                 | RCC_AHB4ENR_GPIOE_EN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2 | RCC_APB1ENR_USART3 | RCC_APB1ENR_I2C1;
    RCC_APB2ENR |= RCC_APB2ENR_USART1 | RCC_APB2ENR_SPI1
                 | RCC_APB2ENR_SPI2 | RCC_APB2ENR_ADC1 | RCC_APB2ENR_HRTIM1;
    RCC_AHB2ENR |= RCC_AHB2ENR_USB;
    RCC_AHB1ENR |= BIT(21);        /* DMA1 clock */

    /* Configure SysTick for 1 ms @ 550 MHz */
    SYSTICK_RVR = 550000 - 1;
    SYSTICK_CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSRC;

    /* Enable global interrupts */
    /* __enable_irq(); — in bare-metal, interrupts are on by default after reset
     * but we make sure NVIC is configured per-driver. */
}

/* ---- GPIO helper --------------------------------------------------- */

static void gpio_config(gpio_regs_t *port, uint8_t pin, uint8_t mode,
                         uint8_t speed, uint8_t pupd, uint8_t af)
{
    port->MODER &= ~(3U << (pin * 2));
    port->MODER |= (mode << (pin * 2));

    port->OSPEEDR &= ~(3U << (pin * 2));
    port->OSPEEDR |= (speed << (pin * 2));

    port->PUPDR &= ~(3U << (pin * 2));
    port->PUPDR |= (pupd << (pin * 2));

    if (af != 0) {
        if (pin < 8) {
            port->AFRL &= ~(0xFU << (pin * 4));
            port->AFRL |= (af << (pin * 4));
        } else {
            port->AFRH &= ~(0xFU << ((pin - 8) * 4));
            port->AFRH |= (af << ((pin - 8) * 4));
        }
    }
}

/* ---- Status LED ---------------------------------------------------- */

void board_led_set(bool on)
{
    if (on)
        GPIOE->BSRR = BIT(1);    /* set PE1 */
    else
        GPIOE->BRR = BIT(1);     /* reset PE1 */
}

void board_led_toggle(void)
{
    GPIOE->ODR ^= BIT(1);
}

/* ---- Series resistance table --------------------------------------- */

const float series_resistance_table[SERIES_R_TABLE_SIZE] = {
    0.5f,   /* index 0 — minimum resistance, maximum glitch depth */
    1.0f,
    2.0f,
    5.0f,
    10.0f,
    20.0f,
    33.0f,
    50.0f,  /* index 7 — maximum resistance, minimum glitch depth */
};

/* ---- Default configuration ----------------------------------------- */

static void init_default_params(void)
{
    /* Zero the structure */
    for (uint8_t *p = (uint8_t *)&g_current_params;
         p < (uint8_t *)&g_current_params + sizeof(g_current_params); p++)
        *p = 0;

    g_current_params.vector_mask = GLITCH_VECTOR_POWER;
    g_current_params.trigger_offset_ns = 1000;
    g_current_params.glitch_width_ns = 200;
    g_current_params.power_depth_mv = 900;
    g_current_params.power_series_r = 3;
    g_current_params.repeat_count = 1;
    g_current_params.repeat_delay_ns = 100000;

    g_trigger_config.source = TRIG_SOURCE_GPIO;
    g_trigger_config.trigger_edge = 0; /* rising */

    /* Default success pattern: a common shell prompt */
    g_patterns.success_pattern_count = 1;
    my_strncpy(g_patterns.success_patterns[0], "# ", MAX_PATTERN_LEN);
    g_patterns.failure_pattern_count = 1;
    my_strncpy(g_patterns.failure_patterns[0], "Access denied",
               MAX_PATTERN_LEN);
}

/* ---- String helper (no libc) --------------------------------------- */

void my_strncpy(char *dst, const char *src, uint32_t maxlen)
{
    uint32_t i = 0;
    while (i < maxlen - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

int my_strstr(const char *haystack, uint32_t haystack_len,
              const char *needle)
{
    uint32_t i, j;
    if (!needle[0])
        return 1; /* empty pattern always matches */
    for (i = 0; i < haystack_len; i++) {
        for (j = 0; needle[j] && (i + j) < haystack_len; j++) {
            if (haystack[i + j] != needle[j])
                break;
        }
        if (!needle[j])
            return 1; /* match found */
    }
    return 0; /* no match */
}

/* ---- Single glitch shot -------------------------------------------- */

static void execute_single_shot(const glitch_params_t *params,
                                glitch_result_t *result)
{
    uint32_t start_ms = board_millis();

    /* Clear result */
    for (uint8_t *p = (uint8_t *)result;
         p < (uint8_t *)result + sizeof(glitch_result_t); p++)
        *p = 0;
    result->outcome = RESULT_PENDING;

    /* Configure the glitch output stages based on vector_mask */
    if (params->vector_mask & GLITCH_VECTOR_POWER) {
        power_glitch_configure(params->power_depth_mv,
                               params->power_series_r,
                               params->glitch_width_ns);
    }
    if (params->vector_mask & GLITCH_VECTOR_CLOCK) {
        clock_glitch_configure(params->clock_shape,
                               params->clock_cycle_offset,
                               params->glitch_width_ns);
    }
    if (params->vector_mask & GLITCH_VECTOR_EM) {
        em_pulse_configure(params->em_pulse_mv,
                           params->em_pulse_width_ns);
    }

    /* Program the HRTIM for the trigger offset and inter-vector delay */
    hrtim_glitch_program(params->trigger_offset_ns,
                         params->glitch_width_ns,
                         params->inter_vector_ns,
                         params->vector_mask);

    /* Arm the ADC to capture the VCC waveform during the glitch */
    adc_capture_arm(result->waveform, &result->waveform_len, 256);

    /* Clear the UART monitor buffer */
    uart_monitor_reset();

    /* Arm the trigger through the FPGA */
    fpga_arm_trigger(g_trigger_config.source, params->trigger_offset_ns);

    /* Wait for the glitch to fire (or timeout) */
    uint32_t timeout_ms = 500;
    uint32_t wait_start = board_millis();
    while (!fpga_glitch_fired() && !fpga_trigger_timed_out()) {
        if ((board_millis() - wait_start) > timeout_ms) {
            result->outcome = RESULT_TIMEOUT;
            result->elapsed_us = (board_millis() - start_ms) * 1000;
            return;
        }
    }

    if (fpga_trigger_timed_out()) {
        result->outcome = RESULT_TIMEOUT;
        result->elapsed_us = (board_millis() - start_ms) * 1000;
        return;
    }

    /* Small delay for the target to respond */
    board_delay_ms(50);

    /* Read the ADC waveform */
    adc_capture_read(result->waveform, &result->waveform_len);

    /* Classify the glitch quality from the waveform */
    if (adc_capture_classify(result->waveform, result->waveform_len,
                             params->power_depth_mv) == GLITCH_QUALITY_INVALID) {
        result->outcome = RESULT_GLITCH_INVALID;
        g_invalid_shots++;
        result->elapsed_us = (board_millis() - start_ms) * 1000;
        return;
    }

    /* Read the target UART response */
    uart_monitor_get_response(result->target_response,
                              sizeof(result->target_response));

    /* Classify the response against patterns */
    if (g_patterns.success_pattern_count > 0) {
        for (uint8_t i = 0; i < g_patterns.success_pattern_count; i++) {
            if (my_strstr(result->target_response,
                          sizeof(result->target_response),
                          g_patterns.success_patterns[i])) {
                result->outcome = RESULT_SUCCESS;
                g_success_shots++;
                result->elapsed_us = (board_millis() - start_ms) * 1000;
                g_total_shots++;
                return;
            }
        }
    }

    if (g_patterns.failure_pattern_count > 0) {
        for (uint8_t i = 0; i < g_patterns.failure_pattern_count; i++) {
            if (my_strstr(result->target_response,
                          sizeof(result->target_response),
                          g_patterns.failure_patterns[i])) {
                result->outcome = RESULT_FAILURE;
                g_failure_shots++;
                result->elapsed_us = (board_millis() - start_ms) * 1000;
                g_total_shots++;
                return;
            }
        }
    }

    /* No pattern matched — check if there was any response at all */
    if (result->target_response[0] != 0) {
        /* Garbage/unexpected output — classify as failure for safety */
        result->outcome = RESULT_FAILURE;
        g_failure_shots++;
    } else {
        result->outcome = RESULT_NO_RESPONSE;
        g_no_response_shots++;
    }

    result->elapsed_us = (board_millis() - start_ms) * 1000;
    g_total_shots++;
}

/* ---- Repeat shot (multi-shot per cell) ----------------------------- */

static void execute_repeated(const glitch_params_t *params,
                             glitch_result_t *result)
{
    glitch_result_t shot_result;
    uint16_t shot;

    for (shot = 0; shot < params->repeat_count; shot++) {
        execute_single_shot(params, &shot_result);

        if (shot_result.outcome == RESULT_SUCCESS) {
            /* Copy the successful result and return */
            *result = shot_result;
            result->shots_fired = shot + 1;
            return;
        }

        if (shot + 1 < params->repeat_count) {
            /* Inter-shot delay */
            uint32_t delay_ms = params->repeat_delay_ns / 1000000;
            if (delay_ms < 1)
                delay_ms = 1;
            board_delay_ms(delay_ms);
        }
    }

    /* No success — return the last result */
    *result = shot_result;
    result->shots_fired = params->repeat_count;
}

/* ---- Sweep controller ---------------------------------------------- */

static void apply_sweep_param(glitch_params_t *params, sweep_param_t param,
                               uint32_t value)
{
    switch (param) {
    case SWEEP_PARAM_OFFSET:
        params->trigger_offset_ns = value;
        break;
    case SWEEP_PARAM_WIDTH:
        params->glitch_width_ns = value;
        break;
    case SWEEP_PARAM_DEPTH:
        params->power_depth_mv = (uint16_t)value;
        break;
    case SWEEP_PARAM_EM_VOLTAGE:
        params->em_pulse_mv = (uint16_t)value;
        break;
    case SWEEP_PARAM_EM_WIDTH:
        params->em_pulse_width_ns = (uint16_t)value;
        break;
    case SWEEP_PARAM_CLOCK_CYCLE:
        params->clock_cycle_offset = value;
        break;
    case SWEEP_PARAM_SERIES_R:
        params->power_series_r = (uint8_t)value;
        break;
    default:
        break;
    }
}

static uint32_t count_sweep_steps(uint32_t start, uint32_t end, uint32_t step)
{
    if (step == 0 || end <= start)
        return 1;
    return ((end - start) / step) + 1;
}

static void sweep_init_status(void)
{
    uint32_t x_steps = count_sweep_steps(g_sweep_config.x_start,
                                         g_sweep_config.x_end,
                                         g_sweep_config.x_step);
    uint32_t y_steps = count_sweep_steps(g_sweep_config.y_start,
                                         g_sweep_config.y_end,
                                         g_sweep_config.y_step);
    uint32_t z_steps = 1;
    if (g_sweep_config.z_param != SWEEP_PARAM_COUNT && g_sweep_config.z_step > 0)
        z_steps = count_sweep_steps(g_sweep_config.z_start,
                                    g_sweep_config.z_end,
                                    g_sweep_config.z_step);

    g_sweep_status.total_cells = x_steps * y_steps * z_steps;
    g_sweep_status.completed_cells = 0;
    g_sweep_status.success_count = 0;
    g_sweep_status.failure_count = 0;
    g_sweep_status.no_response_count = 0;
    g_sweep_status.invalid_count = 0;
    g_sweep_status.running = true;
}

static void sweep_run(void)
{
    glitch_params_t params;
    glitch_result_t result;

    sweep_init_status();

    uint32_t z_start = g_sweep_config.z_start;
    uint32_t z_end = (g_sweep_config.z_param != SWEEP_PARAM_COUNT)
                     ? g_sweep_config.z_end : z_start;
    uint32_t z_step = (g_sweep_config.z_step > 0)
                      ? g_sweep_config.z_step : 1;

    for (uint32_t z = z_start; z <= z_end; z += z_step) {
        for (uint32_t y = g_sweep_config.y_start;
             y <= g_sweep_config.y_end;
             y += g_sweep_config.y_step) {
            for (uint32_t x = g_sweep_config.x_start;
                 x <= g_sweep_config.x_end;
                 x += g_sweep_config.x_step) {

                if (!g_sweep_status.running)
                    return; /* sweep stopped by host */

                params = g_sweep_config.base_params;
                if (g_sweep_config.z_param != SWEEP_PARAM_COUNT)
                    apply_sweep_param(&params, g_sweep_config.z_param, z);
                apply_sweep_param(&params, g_sweep_config.y_param, y);
                apply_sweep_param(&params, g_sweep_config.x_param, x);

                params.repeat_count = g_sweep_config.shots_per_cell;
                if (params.repeat_count == 0)
                    params.repeat_count = 1;

                execute_repeated(&params, &result);

                switch (result.outcome) {
                case RESULT_SUCCESS:
                    g_sweep_status.success_count++;
                    break;
                case RESULT_FAILURE:
                    g_sweep_status.failure_count++;
                    break;
                case RESULT_NO_RESPONSE:
                    g_sweep_status.no_response_count++;
                    break;
                case RESULT_GLITCH_INVALID:
                    g_sweep_status.invalid_count++;
                    break;
                default:
                    break;
                }

                g_sweep_status.completed_cells++;
                g_last_result = result;

                /* Log to SD card */
                sdcard_log_shot(&params, &result);

                /* Report progress to host */
                protocol_send_sweep_progress(&g_sweep_status, &result);

                /* Adaptive mode: skip ahead if region is dead */
                if (g_sweep_config.adaptive &&
                    g_sweep_status.completed_cells > 4 &&
                    g_sweep_status.success_count == 0) {
                    /* Simple heuristic: if no successes after 4 cells
                     * in a row on this y-row, skip to next y */
                    /* (Simplified — real implementation would track
                     * per-row success counts) */
                }

                /* Inter-cell delay for target recovery */
                board_delay_ms(20);
            }
        }
    }

    g_sweep_status.running = false;
    protocol_send_sweep_complete(&g_sweep_status);
}

/* ---- OLED status update -------------------------------------------- */

static void oled_update_status(void)
{
    oled_clear();
    oled_set_cursor(0, 0);
    oled_print("PlasmaReaper v1.0");
    oled_set_cursor(1, 0);
    oled_print("Shots: ");
    oled_print_u32(g_total_shots);
    oled_set_cursor(2, 0);
    oled_print("OK: ");
    oled_print_u32(g_success_shots);
    oled_print(" Fail: ");
    oled_print_u32(g_failure_shots);
    oled_set_cursor(3, 0);
    if (g_sweep_status.running) {
        oled_print("SWEEP ");
        oled_print_u32(g_sweep_status.completed_cells);
        oled_print("/");
        oled_print_u32(g_sweep_status.total_cells);
    } else {
        oled_print("IDLE");
    }
    oled_refresh();
}

/* ---- Main ---------------------------------------------------------- */

int main(void)
{
    /* Initialize board (clocks, GPIO, peripherals) */
    board_init();

    /* Configure status LED pin PE1 as output */
    gpio_config(GPIOE, 1, GPIO_MODE_OUTPUT, GPIO_OSPEED_LOW,
                GPIO_PUPD_NONE, 0);
    board_led_set(true);

    /* Initialize OLED display first so we can show boot status */
    oled_init();
    oled_clear();
    oled_set_cursor(0, 0);
    oled_print("PlasmaReaper boot");
    oled_refresh();

    /* Initialize the FPGA interface and load bitstream */
    oled_set_cursor(1, 0);
    oled_print("Loading FPGA...");
    oled_refresh();
    fpga_init();
    if (!fpga_load_bitstream()) {
        oled_set_cursor(2, 0);
        oled_print("FPGA FAIL!");
        oled_refresh();
        /* Continue anyway — some functionality works without FPGA */
    } else {
        oled_set_cursor(2, 0);
        oled_print("FPGA OK");
        oled_refresh();
    }

    /* Initialize glitch output stages */
    power_glitch_init();
    clock_glitch_init();
    em_pulse_init();

    /* Initialize HRTIM glitch timer */
    hrtim_glitch_init();

    /* Initialize trigger subsystem */
    trigger_init();

    /* Initialize UART monitor (target response capture) */
    uart_monitor_init(115200);

    /* Initialize ADC for VCC waveform capture */
    adc_capture_init();

    /* Initialize USB CDC */
    usb_cdc_init();

    /* Initialize BLE interface */
    ble_if_init();

    /* Initialize SD card logging */
    sdcard_init();

    /* Load default parameters */
    init_default_params();

    /* Boot complete */
    board_led_set(false);
    oled_clear();
    oled_set_cursor(0, 0);
    oled_print("PlasmaReaper v1.0");
    oled_set_cursor(1, 0);
    oled_print("Ready.");
    oled_refresh();

    uint32_t last_oled_update = 0;

    /* ---- Super-loop ---- */
    while (1) {
        /* Process host commands (USB or BLE) */
        protocol_poll();

        /* If a sweep is running, let it proceed */
        if (g_sweep_status.running) {
            /* Sweep runs synchronously; it calls protocol_send_* to
             * report progress. The host can stop the sweep by sending
             * CMD_STOP_SWEEP, which sets g_sweep_status.running = false. */
            sweep_run();
        }

        /* Update OLED display at 5 Hz */
        if ((board_millis() - last_oled_update) > 200) {
            oled_update_status();
            last_oled_update = board_millis();
            board_led_toggle();
        }
    }

    return 0; /* never reached */
}

/* ---- Protocol command handlers (called from protocol.c) ------------ */

void protocol_handle_fire(glitch_params_t *params, glitch_result_t *result)
{
    g_current_params = *params;
    execute_repeated(&g_current_params, result);
    g_last_result = *result;
}

void protocol_handle_start_sweep(sweep_config_t *config)
{
    g_sweep_config = *config;
    g_sweep_status.running = true;
    /* The super-loop will call sweep_run() on the next iteration */
}

void protocol_handle_stop_sweep(void)
{
    g_sweep_status.running = false;
}

void protocol_handle_set_trigger(trigger_config_t *config)
{
    g_trigger_config = *config;
    trigger_configure(&g_trigger_config);
}

void protocol_handle_set_patterns(pattern_config_t *patterns)
{
    g_patterns = *patterns;
}

void protocol_handle_get_status(sweep_status_t *status,
                                glitch_result_t *last_result,
                                uint32_t *total_shots,
                                uint32_t *success_shots)
{
    *status = g_sweep_status;
    *last_result = g_last_result;
    *total_shots = g_total_shots;
    *success_shots = g_success_shots;
}

void protocol_handle_get_waveform(uint16_t *waveform, uint16_t *len)
{
    *len = g_last_result.waveform_len;
    for (uint16_t i = 0; i < *len; i++)
        waveform[i] = g_last_result.waveform[i];
}

/* ---- NVIC interrupt handlers --------------------------------------- */

/* These are thin wrappers that dispatch to the driver-specific ISRs.
 * In a real build with the STM32 HAL, these would be named exactly
 * as the startup file expects. */

void USART3_IRQHandler(void)
{
    uart_monitor_isr();
}

void DMA1_Stream0_IRQHandler(void)
{
    adc_capture_dma_isr();
}

void DMA1_Stream1_IRQHandler(void)
{
    uart_monitor_dma_isr();
}

/* ---- Required by protocol.c / other drivers ------------------------ */

void my_strncpy(char *dst, const char *src, uint32_t maxlen);
int  my_strstr(const char *haystack, uint32_t haystack_len, const char *needle);