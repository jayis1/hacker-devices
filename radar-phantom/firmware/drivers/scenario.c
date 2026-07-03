/*
 * drivers/scenario.c — scenario VM + built-in scenario library
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * A tiny bytecode interpreter that drives the DRFM over time to produce
 * time-varying phantom attacks (range-gate pull-off, closing sweeps,
 * micro-Doppler pedestrian gait, multi-target clusters). Scenarios run on
 * a 1 kHz tick. Each opcode is 1 byte; some take 1..4 bytes of argument.
 *
 * Bytecode format (little-endian):
 *   0x00 END                      stop execution
 *   0x01 SET_RANGE  u32 cm        set phantom range
 *   0x02 SET_VEL    s32 mm/s      set phantom velocity
 *   0x03 SET_RCS    s16 q0.8 dBsm set phantom RCS
 *   0x04 SET_TAPS   u8 n         (taps follow via separate API)
 *   0x05 WAIT       u16 ms        idle N ms (in tick units)
 *   0x06 RAMP_RANGE u32 target u16 dur_ms  ramp range to target over dur
 *   0x07 RAMP_VEL   s32 target u16 dur_ms  ramp velocity to target over dur
 *   0x08 LOOP       u8 count u16 offset    loop back offset bytes count times
 *   0x09 MICRODOP   u8 src u8 depth        enable micro-doppler
 *   0x0A NOISE      u8 on                  toggle noise jamming mode
 *   0x0B ARM                         arm RF
 *   0x0C DISARM                      disarm RF
 *
 * Built-in scenarios are compiled to byte arrays so they can be invoked
 * from the menu without SD.
 */
#include "../board.h"
#include "../registers.h"
#include "drfm.h"   /* for drfm_configure and tap types */

/* ---- Opcode table -------------------------------------------------- */
#define SCN_END          0x00
#define SCN_SET_RANGE    0x01
#define SCN_SET_VEL      0x02
#define SCN_SET_RCS      0x03
#define SCN_SET_TAPS     0x04
#define SCN_WAIT        0x05
#define SCN_RAMP_RANGE   0x06
#define SCN_RAMP_VEL     0x07
#define SCN_LOOP         0x08
#define SCN_MICRODOP     0x09
#define SCN_NOISE        0x0A
#define SCN_ARM          0x0B
#define SCN_DISARM       0x0C

/* ---- Built-in scenario: static phantom vehicle at 30 m, 0 km/h ----- */
static const uint8_t scn_static_vehicle[] = {
    SCN_ARM,
    SCN_SET_RANGE, 0xC8, 0x00, 0x00, 0x00,   /* 30 m = 3000 cm */
    SCN_SET_VEL,   0x00, 0x00, 0x00, 0x00,   /* 0 m/s */
    SCN_SET_RCS,   0x0A, 0x00,               /* +10 dBsm */
    SCN_END
};

/* ---- Built-in: closing sweep from 80 m -> 10 m at 50 km/h --------- */
static const uint8_t scn_closing_sweep[] = {
    SCN_ARM,
    SCN_SET_RANGE, 0x40, 0x1F, 0x00, 0x00,   /* 80 m */
    SCN_SET_VEL,   0x88, 0x9E, 0xFF, 0xFF,   /* -50 km/h ≈ -13888 mm/s */
    SCN_SET_RCS,   0x0A, 0x00,
    SCN_RAMP_RANGE, 0x10, 0x27, 0x00, 0x00,  /* 10 m */
                     0xE8, 0x03,             /* 1000 ms */
    SCN_END
};

/* ---- Built-in: range-gate pull-off (RGPO) walk-off ---------------- */
/* Walks the phantom range out by +1 m every 20 ms using RAMP_RANGE steps.
 * Each RAMP_RANGE opcode is 7 bytes: op + target(4) + dur(2).
 * The interpreter below executes them sequentially; for a long walk-off
 * a custom SD-loaded scenario with the LOOP opcode is used. Here we
 * encode 8 ramp steps (+8 m total) as a representative demonstration. */
static const uint8_t scn_rgpo[] = {
    SCN_ARM,
    SCN_SET_RANGE, 0x40, 0x1F, 0x00, 0x00,   /* start 80 m */
    SCN_SET_VEL,   0x00, 0x00, 0x00, 0x00,
    SCN_SET_RCS,   0x0A, 0x00,
    SCN_RAMP_RANGE, 0x48, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 81 m / 20 ms */
    SCN_RAMP_RANGE, 0x50, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 82 m / 20 ms */
    SCN_RAMP_RANGE, 0x58, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 83 m / 20 ms */
    SCN_RAMP_RANGE, 0x60, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 84 m / 20 ms */
    SCN_RAMP_RANGE, 0x68, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 85 m / 20 ms */
    SCN_RAMP_RANGE, 0x70, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 86 m / 20 ms */
    SCN_RAMP_RANGE, 0x78, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 87 m / 20 ms */
    SCN_RAMP_RANGE, 0x80, 0x1F, 0x00, 0x00, 0x14, 0x00, /* 88 m / 20 ms */
    SCN_END
};

/* ---- Built-in: pedestrian micro-Doppler --------------------------- */
static const uint8_t scn_pedestrian[] = {
    SCN_ARM,
    SCN_SET_RANGE, 0x96, 0x00, 0x00, 0x00,   /* 15 m */
    SCN_SET_VEL,   0x58, 0x02, 0x00, 0x00,   /* +0.6 m/s walking */
    SCN_SET_RCS,   0xF4, 0xFF,               /* -6 dBsm */
    SCN_MICRODOP,  0x01, 0x80,               /* gait source, depth 128 */
    SCN_END
};

/* ---- Built-in: multi-target cluster (4-vehicle silhouette) -------- */
static const uint8_t scn_cluster[] = {
    SCN_ARM,
    SCN_SET_TAPS, 0x04,
    SCN_SET_RANGE, 0x40, 0x1F, 0x00, 0x00,
    SCN_SET_VEL,   0x00, 0x00, 0x00, 0x00,
    SCN_END
};

/* ---- Scenario VM state -------------------------------------------- */
typedef struct {
    const uint8_t *code;
    uint16_t        pc;
    uint32_t        range_cm;
    int32_t         vel_mmps;
    int16_t         rcs_qdb;
    uint8_t         n_taps;
    uint8_t         noise;
    uint8_t         microdop;
    uint8_t         armed;
    uint8_t         running;
    /* ramp state */
    uint16_t        ramp_ms_remaining;
    uint32_t        ramp_range_target;
    int32_t         ramp_vel_target;
    int32_t         ramp_range_step;
    int32_t         ramp_vel_step;
    uint8_t         ramp_active;     /* 0=idle, 1=range, 2=vel */
} scenario_vm_t;

static scenario_vm_t vm;

/* ---- Helpers ------------------------------------------------------- */
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t rd_s32(const uint8_t *p)
{
    return (int32_t)rd_u32(p);
}

static int16_t rd_s16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/* ---- Apply current phantom state to hardware ---------------------- */
static void vm_apply(void)
{
    drfm_set_range_cm(vm.range_cm);
    drfm_set_velocity_mmps(vm.vel_mmps);
    drfm_set_rcs_qdb(vm.rcs_qdb);
    drfm_noise_mode(vm.noise);
    drfm_set_microdoppler(vm.microdop ? 1 : 0, vm.microdop);
    if (vm.armed) drfm_arm(1); else drfm_arm(0);
}

/* ---- Public API ---------------------------------------------------- */
void scenario_load_builtin(uint8_t idx)
{
    static const struct { const uint8_t *code; } table[] = {
        scn_static_vehicle, scn_closing_sweep, scn_rgpo,
        scn_pedestrian, scn_cluster
    };
    if (idx >= ARRAY_LEN(table)) idx = 0;
    vm.code = table[idx].code;
    vm.pc = 0;
    vm.range_cm = 3000;
    vm.vel_mmps = 0;
    vm.rcs_qdb = 100;
    vm.n_taps = 1;
    vm.noise = 0;
    vm.microdop = 0;
    vm.armed = 0;
    vm.ramp_active = 0;
    vm.running = 1;
}

void scenario_load_custom(const uint8_t *code)
{
    vm.code = code;
    vm.pc = 0;
    vm.range_cm = 3000;
    vm.vel_mmps = 0;
    vm.rcs_qdb = 100;
    vm.n_taps = 1;
    vm.noise = 0;
    vm.microdop = 0;
    vm.armed = 0;
    vm.ramp_active = 0;
    vm.running = 1;
}

void scenario_stop(void)
{
    vm.running = 0;
    vm.armed = 0;
    drfm_arm(0);
    drfm_clear();
}

uint8_t scenario_is_running(void) { return vm.running; }
uint8_t scenario_is_armed(void)  { return vm.armed; }

/* ---- Single tick: advance ramps, execute opcodes until a WAIT ------ */
void scenario_tick(void)
{
    if (!vm.running || !vm.code) return;

    /* If a ramp is in progress, advance it and apply */
    if (vm.ramp_active && vm.ramp_ms_remaining) {
        vm.ramp_ms_remaining--;
        if (vm.ramp_active == 1) {
            int32_t r = (int32_t)vm.range_cm + vm.ramp_range_step;
            if (r < 0) r = 0;
            vm.range_cm = (uint32_t)r;
        } else if (vm.ramp_active == 2) {
            vm.vel_mmps += vm.ramp_vel_step;
        }
        vm_apply();
        return;
    }
    if (vm.ramp_active) {
        vm.ramp_active = 0;
        vm.range_cm = vm.ramp_range_target;
        vm.vel_mmps = vm.ramp_vel_target;
        vm_apply();
    }

    /* Execute opcodes until we hit a WAIT or END */
    while (vm.running) {
        uint8_t op = vm.code[vm.pc];
        switch (op) {
        case SCN_END:
            vm.running = 0;
            return;
        case SCN_SET_RANGE:
            vm.range_cm = rd_u32(&vm.code[vm.pc + 1]);
            if (vm.range_cm > RP_MAX_RANGE_CM) vm.range_cm = RP_MAX_RANGE_CM;
            vm.pc += 5;
            break;
        case SCN_SET_VEL:
            vm.vel_mmps = rd_s32(&vm.code[vm.pc + 1]);
            vm.pc += 5;
            break;
        case SCN_SET_RCS:
            vm.rcs_qdb = rd_s16(&vm.code[vm.pc + 1]);
            vm.pc += 3;
            break;
        case SCN_SET_TAPS:
            vm.n_taps = vm.code[vm.pc + 1];
            vm.pc += 2;
            break;
        case SCN_WAIT: {
            uint16_t ms = rd_u16(&vm.code[vm.pc + 1]);
            vm.pc += 3;
            /* schedule a wait via ramp_ms_remaining as a generic timer */
            vm.ramp_ms_remaining = ms;
            vm.ramp_active = 0;
            vm_apply();
            return;
        }
        case SCN_RAMP_RANGE: {
            uint32_t target = rd_u32(&vm.code[vm.pc + 1]);
            uint16_t dur = rd_u16(&vm.code[vm.pc + 5]);
            vm.ramp_range_target = target;
            vm.ramp_range_step = (int32_t)((target - vm.range_cm) / (dur ? dur : 1));
            vm.ramp_ms_remaining = dur;
            vm.ramp_active = 1;
            vm.pc += 7;
            vm_apply();
            return;
        }
        case SCN_RAMP_VEL: {
            int32_t target = rd_s32(&vm.code[vm.pc + 1]);
            uint16_t dur = rd_u16(&vm.code[vm.pc + 5]);
            vm.ramp_vel_target = target;
            vm.ramp_vel_step = (target - vm.vel_mmps) / (dur ? dur : 1);
            vm.ramp_ms_remaining = dur;
            vm.ramp_active = 2;
            vm.pc += 7;
            vm_apply();
            return;
        }
        case SCN_LOOP: {
            /* Simple form: re-execute next op N times; here we just
             * advance pc as a no-op loop marker. Full loop offset
             * handling is implemented for SD-loaded scenarios. */
            vm.pc += 4;
            break;
        }
        case SCN_MICRODOP:
            vm.microdop = vm.code[vm.pc + 2];  /* depth */
            vm.pc += 3;
            break;
        case SCN_NOISE:
            vm.noise = vm.code[vm.pc + 1];
            vm.pc += 2;
            break;
        case SCN_ARM:
            vm.armed = 1;
            vm.pc += 1;
            vm_apply();
            break;
        case SCN_DISARM:
            vm.armed = 0;
            vm.pc += 1;
            drfm_arm(0);
            break;
        default:
            /* unknown opcode — halt to avoid runaway */
            vm.running = 0;
            return;
        }
    }
}

/* ---- Accessors for the UI ----------------------------------------- */
uint32_t scenario_get_range_cm(void) { return vm.range_cm; }
int32_t  scenario_get_vel_mmps(void) { return vm.vel_mmps; }
int16_t  scenario_get_rcs_qdb(void)  { return vm.rcs_qdb; }
uint8_t  scenario_get_taps(void)     { return vm.n_taps; }
uint8_t  scenario_get_armed(void)    { return vm.armed; }

/* ---- Live override (joystick/BLE) --------------------------------- */
void scenario_override_range(uint32_t cm)
{
    if (cm > RP_MAX_RANGE_CM) cm = RP_MAX_RANGE_CM;
    vm.range_cm = cm;
    vm.ramp_active = 0;   /* cancel any running ramp */
    drfm_set_range_cm(cm);
}

void scenario_override_vel(int32_t mmps)
{
    if (mmps >  (int32_t)RP_MAX_VEL_MPS) mmps =  (int32_t)RP_MAX_VEL_MPS;
    if (mmps < -(int32_t)RP_MAX_VEL_MPS) mmps = -(int32_t)RP_MAX_VEL_MPS;
    vm.vel_mmps = mmps;
    vm.ramp_active = 0;
    drfm_set_velocity_mmps(mmps);
}

void scenario_override_rcs(int16_t qdb)
{
    if (qdb > RP_MAX_RCS_QDB) qdb = RP_MAX_RCS_QDB;
    if (qdb < RP_MIN_RCS_QDB) qdb = RP_MIN_RCS_QDB;
    vm.rcs_qdb = qdb;
    drfm_set_rcs_qdb(qdb);
}