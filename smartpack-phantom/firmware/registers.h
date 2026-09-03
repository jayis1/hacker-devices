/*
 * SmartPack Phantom register abstractions
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef SMARTPACK_REGISTERS_H
#define SMARTPACK_REGISTERS_H

#define SP_REG_STATUS             0x00u
#define SP_REG_CONTROL            0x01u
#define SP_REG_PROFILE_KIND       0x02u
#define SP_REG_SOC_PERCENT        0x03u
#define SP_REG_TEMP_C             0x04u
#define SP_REG_STATUS_WORD        0x05u
#define SP_REG_CHARGE_LIMIT       0x06u
#define SP_REG_AUTH_MODE          0x07u
#define SP_REG_ALERT_CONTROL      0x08u
#define SP_REG_EVENT_COUNT        0x09u
#define SP_REG_EXPORT_CONTROL     0x0Au

#define SP_SBS_TEMPERATURE        0x08u
#define SP_SBS_VOLTAGE            0x09u
#define SP_SBS_CURRENT            0x0Au
#define SP_SBS_AVG_CURRENT        0x0Bu
#define SP_SBS_RELATIVE_SOC       0x0Du
#define SP_SBS_ABSOLUTE_SOC       0x0Eu
#define SP_SBS_REMAINING_CAP      0x0Fu
#define SP_SBS_FULL_CHARGE_CAP    0x10u
#define SP_SBS_RUNTIME_EMPTY      0x11u
#define SP_SBS_AVG_TIME_EMPTY     0x12u
#define SP_SBS_CYCLE_COUNT        0x17u
#define SP_SBS_DESIGN_CAPACITY    0x18u
#define SP_SBS_DESIGN_VOLTAGE     0x19u
#define SP_SBS_MANUFACTURER_DATE  0x1Bu
#define SP_SBS_SERIAL_NUMBER      0x1Cu
#define SP_SBS_MANUFACTURER_NAME  0x20u
#define SP_SBS_DEVICE_NAME        0x21u
#define SP_SBS_DEVICE_CHEMISTRY   0x22u
#define SP_SBS_MANUFACTURER_DATA  0x23u
#define SP_SBS_STATUS             0x16u
#define SP_SBS_MANUFACTURER_ACCESS 0x00u

#define SP_VENDOR_AUTH_CHALLENGE  0x3Cu
#define SP_VENDOR_AUTH_RESPONSE   0x3Du
#define SP_VENDOR_SERVICE_FLAGS   0x3Eu
#define SP_VENDOR_CHARGE_POLICY   0x3Fu

#endif
