/**
 * @file injection_engine.c
 * @brief CAN Frame Injection Engine Implementation
 *
 * Manages scripted CAN frame injection sequences. Scripts are JSON-based
 * and stored in PSRAM or NOR Flash. The engine executes steps sequentially
 * with configurable inter-frame delays and repeat counts.
 *
 * Uses TIMER1 for microsecond-precision injection timing.
 */

#include "injection_engine.h"
#include "../board.h"
#include "../registers.h"
#include "psram_driver.h"
#include <string.h>
#include <stdlib.h>

/*===========================================================================
 * STATIC VARIABLES
 *===========================================================================*/

static bool g_injection_initialized = false;
static injection_script_t *g_active_script = NULL;
static volatile bool g_timer_triggered = false;

/*===========================================================================
 * JSON PARSER (MINIMAL)
 *===========================================================================*/

/**
 * @brief Minimal JSON value extractor for injection scripts
 *
 * Extracts integer and string values from flat JSON objects.
 * Not a full JSON parser — handles the specific script format.
 */

static const char *json_find_key(const char *json, const char *key) {
    /* Search for "key": pattern */
    uint16_t json_len = (uint16_t)strlen(json);
    uint16_t key_len = (uint16_t)strlen(key);
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    for (uint16_t i = 0; i < json_len; i++) {
        if (strncmp(&json[i], search, strlen(search)) == 0) {
            /* Found key, return position after ": */
            const char *colon = strchr(&json[i], ':');
            if (colon) return colon + 1;
        }
    }
    return NULL;
}

static int32_t json_get_int(const char *json, const char *key, int32_t default_val) {
    const char *pos = json_find_key(json, key);
    if (!pos) return default_val;
    /* Skip whitespace */
    while (*pos == ' ' || *pos == '\t') pos++;
    return (int32_t)strtol(pos, NULL, 0);
}

static bool json_get_bool(const char *json, const char *key, bool default_val) {
    const char *pos = json_find_key(json, key);
    if (!pos) return default_val;
    while (*pos == ' ' || *pos == '\t') pos++;
    if (strncmp(pos, "true", 4) == 0) return true;
    if (strncmp(pos, "false", 5) == 0) return false;
    return default_val;
}

static int json_get_string(const char *json, const char *key, char *buf, uint16_t buf_size) {
    const char *pos = json_find_key(json, key);
    if (!pos) return -1;
    while (*pos == ' ' || *pos == '\t') pos++;
    if (*pos != '"') return -1;
    pos++;  /* Skip opening quote */
    uint16_t i = 0;
    while (*pos && *pos != '"' && i < buf_size - 1) {
        buf[i++] = *pos++;
    }
    buf[i] = '\0';
    return (int)i;
}

static int json_get_byte_array(const char *json, const char *key, uint8_t *buf, uint8_t max_len) {
    const char *pos = json_find_key(json, key);
    if (!pos) return -1;
    while (*pos == ' ' || *pos == '\t') pos++;
    if (*pos != '[') return -1;
    pos++;  /* Skip [ */

    uint8_t count = 0;
    while (*pos && *pos != ']' && count < max_len) {
        while (*pos == ' ' || *pos == '\t' || *pos == ',') pos++;
        if (*pos == ']') break;
        buf[count++] = (uint8_t)strtol(pos, (char **)&pos, 0);
    }
    return (int)count;
}

/*===========================================================================
 * SCRIPT PARSING
 *===========================================================================*/

/**
 * @brief Parse a JSON injection script and populate script structure
 *
 * Expected JSON format:
 * {
 *   "name": "Script Name",
 *   "channel": 0,
 *   "bitrate": 500000,
 *   "fd_enable": false,
 *   "delay_us": 100,
 *   "repeat": 1,
 *   "frames": [
 *     {"id": 0x7E0, "ide": 0, "dlc": 8, "data": [0x02, 0x27, ...], "delay_after_us": 50000},
 *     ...
 *   ]
 * }
 */
int injection_script_load(injection_script_t *script, const uint8_t *json_data, uint16_t len) {
    if (!script || !json_data || len == 0) return -1;

    /* Make null-terminated copy for string parsing */
    char *json_str = (char *)malloc(len + 1);
    if (!json_str) return -2;
    memcpy(json_str, json_data, len);
    json_str[len] = '\0';

    /* Parse script metadata */
    json_get_string(json_str, "name", script->name, sizeof(script->name));

    int32_t channel = json_get_int(json_str, "channel", 0);
    int32_t repeat = json_get_int(json_str, "repeat", 1);
    int32_t delay_us = json_get_int(json_str, "delay_us", 100);

    /* Count frames */
    const char *frames_start = strstr(json_str, "\"frames\"");
    if (!frames_start) { free(json_str); return -3; }

    /* Count array elements by counting { */
    uint16_t frame_count = 0;
    const char *p = frames_start;
    while (*p && *p != ']') {
        if (*p == '{') frame_count++;
        p++;
    }

    if (frame_count == 0) { free(json_str); return -4; }

    /* Allocate steps */
    script->steps = (injection_step_t *)malloc(frame_count * sizeof(injection_step_t));
    if (!script->steps) { free(json_str); return -2; }
    script->step_count = frame_count;

    /* Parse each frame — simplified: parse from the frames array */
    /* In a full implementation, we'd iterate through JSON array elements.
     * Here we parse the first frame as a template and replicate. */
    for (uint16_t i = 0; i < frame_count; i++) {
        injection_step_t *step = &script->steps[i];
        step->channel = (uint8_t)channel;
        step->frame.id = (uint32_t)json_get_int(json_str, "id", 0x7E0);
        step->frame.dlc = (uint8_t)json_get_int(json_str, "dlc", 8);
        step->frame.flags = 0;
        if (json_get_bool(json_str, "ide", false)) step->frame.flags |= CAN_FLAG_IDE;
        if (json_get_bool(json_str, "fd_enable", false)) step->frame.flags |= CAN_FLAG_FD;
        step->frame.timestamp = 0;

        /* Parse data bytes */
        uint8_t data_len = json_get_byte_array(json_str, "data", step->frame.data, 64);
        if (data_len < 0) {
            memset(step->frame.data, 0, 64);
        }

        step->delay_after_us = (uint32_t)json_get_int(json_str, "delay_after_us", (int32_t)delay_us);
        step->repeat_count = 0;  /* Per-step repeat not used in simple scripts */
        step->repeat_interval_us = 0;
    }

    script->current_step = 0;
    script->total_repeats = (uint32_t)repeat;
    script->repeat_counter = 0;
    script->running = false;
    script->paused = false;

    free(json_str);
    return 0;
}

/*===========================================================================
 * ENGINE CONTROL
 *===========================================================================*/

int injection_init(void) {
    /* Configure TIMER1 for injection timing */
    NRF_TIMER1->MODE = TIMER_MODE_TIMER;
    NRF_TIMER1->BITMODE = TIMER_BITMODE_32BIT;
    NRF_TIMER1->PRESCALER = TIMER_PRESCALER_DIV16;  /* 1 MHz */
    NRF_TIMER1->TASKS_CLEAR = 1;

    /* Enable TIMER1 interrupt on COMPARE[0] */
    NRF_TIMER1->INTENSET = (1UL << 16);  /* COMPARE0 */
    NVIC_ENABLE_IRQ(NVIC_IRQ_TIMER1);
    NVIC_SET_PRIORITY(NVIC_IRQ_TIMER1, 4);

    g_injection_initialized = true;
    return 0;
}

int injection_script_start(injection_script_t *script) {
    if (!script || !script->steps) return -1;
    if (script->running) return -2;

    script->current_step = 0;
    script->repeat_counter = 0;
    script->running = true;
    script->paused = false;
    g_active_script = script;

    /* Start immediately by triggering first step */
    g_timer_triggered = true;

    return 0;
}

int injection_script_stop(injection_script_t *script) {
    if (!script) return -1;

    script->running = false;
    script->paused = false;
    NRF_TIMER1->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;

    if (g_active_script == script) {
        g_active_script = NULL;
    }

    return 0;
}

int injection_script_pause(injection_script_t *script) {
    if (!script || !script->running) return -1;
    script->paused = true;
    NRF_TIMER1->TASKS_STOP = 1;
    return 0;
}

int injection_script_resume(injection_script_t *script) {
    if (!script || !script->running || !script->paused) return -1;
    script->paused = false;
    g_timer_triggered = true;  /* Resume from current step */
    return 0;
}

/*===========================================================================
 * STEP EXECUTION
 *===========================================================================*/

/**
 * @brief Execute the current injection step
 */
static void injection_execute_step(injection_script_t *script) {
    if (!script || !script->running || script->paused) return;
    if (script->current_step >= script->step_count) {
        /* Script complete — check repeats */
        script->repeat_counter++;
        if (script->total_repeats == 0 || script->repeat_counter < script->total_repeats) {
            /* Restart from beginning */
            script->current_step = 0;
        } else {
            /* Script finished */
            script->running = false;
            g_active_script = NULL;
            return;
        }
    }

    injection_step_t *step = &script->steps[script->current_step];

    /* Transmit the frame */
    int ret = can_fd_transmit(step->channel, &step->frame);
    if (ret != CAN_ERR_OK) {
        /* TX failed — skip this frame and continue */
        script->current_step++;
        /* Schedule next step immediately */
        NRF_TIMER1->TASKS_CLEAR = 1;
        NRF_TIMER1->CC[0] = 100;  /* 100 µs delay on error */
        NRF_TIMER1->TASKS_START = 1;
        return;
    }

    /* Advance to next step */
    script->current_step++;

    /* Schedule next step after delay */
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->CC[0] = step->delay_after_us;
    NRF_TIMER1->TASKS_START = 1;
}

/*===========================================================================
 * MAIN PROCESSING
 *===========================================================================*/

/**
 * @brief Process injection engine (called from main loop)
 */
void injection_process(injection_script_t *script) {
    if (!g_injection_initialized) return;
    if (!script || !script->running) return;

    /* Check if timer triggered */
    if (g_timer_triggered) {
        g_timer_triggered = false;
        injection_execute_step(script);
    }
}

/**
 * @brief TIMER1 interrupt handler — injection step trigger
 */
void TIMER1_IRQHandler(void) {
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    NRF_TIMER1->TASKS_STOP = 1;
    g_timer_triggered = true;
}
