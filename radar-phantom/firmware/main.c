/*
 * main.c — RadarPhantom firmware main loop & command dispatcher
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Boot, bring up all drivers, run the super-loop:
 *   1. poll joystick + BLE + USB for operator commands
 *   2. tick the scenario VM at 1 kHz
 *   3. update OLED status
 *   4. sample battery, watchdog kick
 *
 * External declarations for driver functions are kept minimal; the
 * drivers are compiled in the same translation unit set via the Makefile.
 */
#include "board.h"
#include "registers.h"

/* ---- Forward declarations from drivers ---------------------------- */
extern void board_init(void);
extern void board_delay_ms(uint32_t ms);
extern uint32_t board_millis(void);
extern void board_watchdog_kick(void);
extern void board_power_enable_rf(void);
extern void board_power_disable_rf(void);
extern void board_led_set(uint32_t port, uint8_t pin, uint8_t on);
extern void spi_init(uint32_t base, uint32_t baud_div, uint8_t cpol, uint8_t cpha);
extern uint8_t spi_xfer(uint32_t base, uint8_t data);

extern void oled_init(void);
extern void oled_clear(void);
extern void oled_flush(void);
extern void oled_render_status(uint8_t bat_pct, const char *mode,
        uint32_t range_cm, int32_t vel_mmps, int16_t rcs_qdb,
        uint8_t taps, uint8_t armed);
extern void oled_render_menu(const void *m);

extern void drfm_reset(void);
extern uint8_t drfm_is_ready(void);
extern uint8_t drfm_selftest(void);
extern void drfm_arm(uint8_t armed);
extern void drfm_clear(void);
extern void drfm_set_range_cm(uint32_t range_cm);
extern void drfm_set_velocity_mmps(int32_t vel_mmps);
extern void drfm_set_rcs_qdb(int16_t rcs_qdb);
extern uint8_t drfm_status(void);
extern void drfm_write_u8(uint8_t addr, uint8_t val);

extern void lo_pll_init(void);
extern void lo_pll_set_frequency(uint64_t f_lo_hz);
extern void lo_pll_enable(uint8_t on);

extern uint8_t radar_sniff(uint8_t timeout_s);
extern uint8_t radar_band_valid(void);
extern uint64_t radar_band_start_hz(void);
extern uint64_t radar_band_stop_hz(void);

extern void scenario_load_builtin(uint8_t idx);
extern void scenario_stop(void);
extern void scenario_tick(void);
extern uint8_t scenario_is_running(void);
extern uint8_t scenario_is_armed(void);
extern void scenario_override_range(uint32_t cm);
extern void scenario_override_vel(int32_t mmps);
extern void scenario_override_rcs(int16_t qdb);
extern uint32_t scenario_get_range_cm(void);
extern int32_t  scenario_get_vel_mmps(void);
extern int16_t  scenario_get_rcs_qdb(void);
extern uint8_t  scenario_get_taps(void);

extern void ble_init(void);
extern void ble_poll_rx(void);
extern void ble_send_frame(uint8_t op, const uint8_t *payload, uint8_t plen);

extern void usb_init(void);
extern void usb_poll_rx(void);

extern uint8_t sd_init(void);
extern uint8_t sd_present(void);

extern void power_init(void);
extern void power_task(void);
extern uint8_t power_battery_percent(void);
extern uint16_t power_battery_mv(void);
extern uint8_t power_low_battery(void);
extern uint8_t power_rf_ok(void);

extern void joystick_task(void);
extern int joystick_get_event(void);
extern const char *joystick_event_name(int e);

/* ---- Two-stage arm interlock ------------------------------------- */
static uint32_t arm_request_ms;
static uint8_t  arm_stage;          /* 0=disarmed, 1=arm requested, 2=armed */

void rp_arm_request(void)
{
    arm_request_ms = board_millis();
    arm_stage = 1;
}

static void arm_interlock_task(void)
{
    if (arm_stage == 1) {
        if (board_millis() - arm_request_ms > ARM_TIMEOUT_MS) {
            arm_stage = 0;            /* timed out, cancel */
            board_led_set(LED_ARM_PORT, LED_ARM_PIN, 0);
        }
    }
}

/* ---- Operator state ---------------------------------------------- */
typedef enum {
    UI_STATUS,
    UI_MENU,
    UI_EDIT_RANGE,
    UI_EDIT_VEL,
    UI_EDIT_RCS,
    UI_EDIT_TAPS,
    UI_SCENARIO_LIST,
    UI_SNIFF,
    UI_LOG,
    UI_POWER_OFF_CONFIRM
} ui_state_t;

static ui_state_t ui_state;
static uint8_t menu_idx;
static uint8_t edit_dirty;

static const char *menu_items[] = {
    "STATIC VEHICLE",
    "CLOSING SWEEP",
    "RGPO WALK-OFF",
    "PEDESTRIAN MD",
    "CLUSTER MULTI",
    "BAND SNIFF",
    "LOG DUMP",
    "POWER OFF"
};
#define MENU_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

/* ---- Edit helpers ------------------------------------------------- */
static const char *mode_name(uint8_t idx)
{
    if (idx < MENU_COUNT - 3) return menu_items[idx];
    return "MANUAL";
}

static void joystick_handle(int e)
{
    if (e == 0) return;   /* JOY_NONE */
    switch (ui_state) {
    case UI_STATUS:
        if (e == 1) ui_state = UI_MENU;   /* UP -> menu */
        break;
    case UI_MENU:
        if (e == 1) {               /* UP = prev */
            if (menu_idx > 0) menu_idx--;
        } else if (e == 2) {        /* DOWN = next */
            if (menu_idx < MENU_COUNT - 1) menu_idx++;
        } else if (e == 3) {        /* LEFT = cancel */
            ui_state = UI_STATUS;
        } else if (e == 4) {        /* RIGHT = enter */
            if (menu_idx < 5) {
                scenario_load_builtin(menu_idx);
                ui_state = UI_STATUS;
            } else if (menu_idx == 5) {
                ui_state = UI_SNIFF;
            } else if (menu_idx == 7) {
                ui_state = UI_POWER_OFF_CONFIRM;
            }
        }
        break;
    case UI_EDIT_RANGE:
        if (e == 3)      scenario_override_range(scenario_get_range_cm() < RP_MAX_RANGE_CM ? scenario_get_range_cm() + 50 : RP_MAX_RANGE_CM);
        else if (e == 4) scenario_override_range(scenario_get_range_cm() > 50 ? scenario_get_range_cm() - 50 : 0);
        else if (e == 5) ui_state = UI_STATUS;
        break;
    case UI_EDIT_VEL:
        if (e == 3)      scenario_override_vel(scenario_get_vel_mmps() + 1000);
        else if (e == 4) scenario_override_vel(scenario_get_vel_mmps() - 1000);
        else if (e == 5) ui_state = UI_STATUS;
        break;
    case UI_EDIT_RCS:
        if (e == 3)      scenario_override_rcs(scenario_get_rcs_qdb() + 8);
        else if (e == 4) scenario_override_rcs(scenario_get_rcs_qdb() - 8);
        else if (e == 5) ui_state = UI_STATUS;
        break;
    case UI_SNIFF:
        if (e == 5) {
            radar_sniff(10);
            ui_state = UI_STATUS;
        }
        break;
    case UI_POWER_OFF_CONFIRM:
        if (e == 5) {
            scenario_stop();
            board_power_disable_rf();
            while (1) { board_watchdog_kick(); board_delay_ms(100); }
        } else if (e == 3 || e == 4) {
            ui_state = UI_STATUS;
        }
        break;
    default:
        ui_state = UI_STATUS;
        break;
    }
}

/* ---- UI render ---------------------------------------------------- */
static void ui_render(void)
{
    static uint32_t last_render;
    uint32_t now = board_millis();
    if (now - last_render < 200) return;
    last_render = now;

    if (ui_state == UI_STATUS) {
        oled_render_status(
            power_battery_percent(),
            mode_name(menu_idx),
            scenario_get_range_cm(),
            scenario_get_vel_mmps(),
            scenario_get_rcs_qdb(),
            scenario_get_taps(),
            scenario_is_armed()
        );
    }
}

/* ---- Scenario 1 kHz tick ----------------------------------------- */
static uint32_t last_tick_ms;
static void scenario_tick_task(void)
{
    uint32_t now = board_millis();
    while (now != last_tick_ms) {
        last_tick_ms++;
        scenario_tick();
    }
}

/* ---- main --------------------------------------------------------- */
int main(void)
{
    board_init();

    /* boot banner on OLED */
    oled_init();
    oled_clear();
    /* draw "RADARPHANTOM" via menu status renderer using a hack: */
    extern void oled_draw_string(uint8_t x, uint8_t page, const char *s);
    oled_draw_string(0, 0, "RADARPHANTOM");
    oled_draw_string(0, 2, "BOOT");
    extern void oled_flush(void);
    oled_flush();

    /* bring up RF control plane (no emission yet) */
    lo_pll_init();
    drfm_reset();
    if (!drfm_is_ready()) {
        board_led_set(LED_FAULT_PORT, LED_FAULT_PIN, 1);
    }
    if (!drfm_selftest()) {
        board_led_set(LED_FAULT_PORT, LED_FAULT_PIN, 1);
    }

    ble_init();
    usb_init();
    power_init();
    if (sd_present()) sd_init();

    board_led_set(LED_STATUS_PORT, LED_STATUS_PIN, 1);

    ui_state = UI_STATUS;
    menu_idx = 0;
    last_tick_ms = board_millis();

    /* main super-loop */
    while (1) {
        board_watchdog_kick();

        /* 1. poll operator inputs */
        joystick_task();
        int e;
        while ((e = joystick_get_event()) != 0) {
            joystick_handle(e);
        }
        ble_poll_rx();
        usb_poll_rx();

        /* 2. arm interlock */
        arm_interlock_task();

        /* 3. scenario VM tick */
        scenario_tick_task();

        /* 4. reflect arm state on LED */
        board_led_set(LED_ARM_PORT, LED_ARM_PIN,
                      scenario_is_armed() ? 1 : 0);

        /* 5. power / battery */
        power_task();
        if (power_low_battery()) {
            board_led_set(LED_FAULT_PORT, LED_FAULT_PIN, 1);
            if (scenario_is_armed()) {
                scenario_stop();
            }
        } else {
            board_led_set(LED_FAULT_PORT, LED_FAULT_PIN, 0);
        }

        /* 6. render UI (throttled) */
        ui_render();
    }
    return 0;
}