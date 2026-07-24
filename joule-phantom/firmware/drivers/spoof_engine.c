/*
 * spoof_engine.c — Telemetry spoofing profiles for Joule-Phantom.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Each profile installs one or more MITM rules that rewrite the
 * battery's SBS telemetry as seen by the host controller.  Profiles
 * are composable and can be mixed with operator-defined rules.
 */

#include "spoof_engine.h"
#include "smbus_port.h"
#include <string.h>

/* Cached identity for clone profile */
static uint8_t  cache_mfr[32];
static uint8_t  cache_mfr_len;
static uint8_t  cache_dev[32];
static uint8_t  cache_dev_len;
static uint16_t cache_serial;
static uint16_t cache_date;
static uint16_t cache_design_cap;
static uint8_t  cache_valid = 0;

/* Helper: install a spoof-word rule */
static void spoof_word(uint8_t cmd, uint16_t val)
{
    mitm_rule_t r;
    memset(&r, 0, sizeof(r));
    r.cmd       = cmd;
    r.mask      = 0xFF;
    r.action    = MITM_ACT_SPOOF;
    r.spoof[0]  = (uint8_t)(val & 0xFF);
    r.spoof[1]  = (uint8_t)(val >> 8);
    r.spoof_len = 2;
    r.enabled   = 1;
    mitm_rule_add(&r);
}

void spoof_profile_full_charge(void)
{
    /* RelativeStateOfCharge = 100 % */
    spoof_word(SBS_REL_STATE_OF_CHARGE, 100);
    /* AbsoluteStateOfCharge = 100 % */
    spoof_word(SBS_ABS_STATE_OF_CHARGE, 100);
    /* RemainingCapacity = FullChargeCapacity (spoof both to 5000 mAh) */
    spoof_word(SBS_REM_CAPACITY, 5000);
    spoof_word(SBS_FULL_CHARGE_CAPACITY, 5000);
    /* BatteryStatus = FULLY_CHARGED | INITIALIZED */
    spoof_word(SBS_BATTERY_STATUS, BSTAT_FULLY_CHARGED | BSTAT_INITIALIZED);
    /* TimeToEmpty = 65535 (not emptying) */
    spoof_word(SBS_RUN_TIME_TO_EMPTY, 0xFFFF);
    spoof_word(SBS_AVG_TIME_TO_EMPTY, 0xFFFF);
    /* Voltage = 12.6 V (3S LiPo full) */
    spoof_word(SBS_VOLTAGE, 12600);
}

void spoof_profile_empty(void)
{
    spoof_word(SBS_REL_STATE_OF_CHARGE, 1);
    spoof_word(SBS_ABS_STATE_OF_CHARGE, 1);
    spoof_word(SBS_REM_CAPACITY, 10);
    spoof_word(SBS_BATTERY_STATUS,
               BSTAT_FULLY_DISCHARGED | BSTAT_INITIALIZED |
               BSTAT_REMAINING_CAP_ALARM | BSTAT_REMAINING_TIME_ALARM);
    spoof_word(SBS_RUN_TIME_TO_EMPTY, 1);
    spoof_word(SBS_AVG_TIME_TO_EMPTY, 1);
    spoof_word(SBS_VOLTAGE, 3000);
    spoof_word(SBS_AVG_CURRENT, 0);
}

void spoof_profile_overtemp(void)
{
    /* Temperature = 333.0 K = 60 C (in 0.1 K units) */
    spoof_word(SBS_TEMPERATURE, 3330);
    spoof_word(SBS_BATTERY_STATUS,
               BSTAT_OVER_TEMP_ALARM | BSTAT_INITIALIZED);
    /* Tell the charger to stop */
    spoof_word(SBS_CHARGING_CURRENT, 0);
    spoof_word(SBS_CHARGING_VOLTAGE, 0);
}

void spoof_profile_overcurrent(void)
{
    spoof_word(SBS_CURRENT, 8000);      /* 8 A discharge */
    spoof_word(SBS_AVG_CURRENT, 8000);
    spoof_word(SBS_BATTERY_STATUS,
               BSTAT_OVER_CURRENT_ALARM | BSTAT_OVERLOAD_ALARM |
               BSTAT_INITIALIZED);
    spoof_word(SBS_RUN_TIME_TO_EMPTY, 2);
}

void spoof_capture_identity(uint8_t *mfr_name, uint8_t *dev_name,
                            uint16_t *serial, uint16_t *date,
                            uint16_t *design_cap)
{
    if (mfr_name) {
        cache_mfr_len = mfr_name[0];   /* first byte = SBS block length */
        if (cache_mfr_len > sizeof(cache_mfr)) cache_mfr_len = sizeof(cache_mfr);
        memcpy(cache_mfr, &mfr_name[1], cache_mfr_len);
    }
    if (dev_name) {
        cache_dev_len = dev_name[0];
        if (cache_dev_len > sizeof(cache_dev)) cache_dev_len = sizeof(cache_dev);
        memcpy(cache_dev, &dev_name[1], cache_dev_len);
    }
    if (serial)     cache_serial    = *serial;
    if (date)       cache_date      = *date;
    if (design_cap) cache_design_cap = *design_cap;
    cache_valid = 1;
}

void spoof_profile_clone(void)
{
    if (!cache_valid) return;
    /* Replay serial, date, design capacity */
    spoof_word(SBS_SERIAL_NUMBER,     cache_serial);
    spoof_word(SBS_MANUFACTURE_DATE,  cache_date);
    spoof_word(SBS_DESIGN_CAPACITY,   cache_design_cap);
    /* Note: string (block) spoofing requires block-rule support; the
     * MITM rule engine supports word spoofing for numeric telemetry.
     * Block spoofing is handled in the bridge path by checking the
     * cache when ManufacturerName / DeviceName is requested. */
}

void spoof_profile_no_charge(void)
{
    spoof_word(SBS_CHARGING_CURRENT, 0);
    spoof_word(SBS_CHARGING_VOLTAGE, 0);
    spoof_word(SBS_BATTERY_STATUS,
               BSTAT_TERMINATE_CHARGE | BSTAT_INITIALIZED);
}

void spoof_clear(void)
{
    mitm_rules_clear();
    cache_valid = 0;
}