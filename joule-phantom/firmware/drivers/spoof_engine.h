/*
 * spoof_engine.h — Smart-battery telemetry spoofing & fault-injection
 *                  policy engine for Joule-Phantom.
 *
 * Author : jayis1
 * License: GPL-2.0
 */

#ifndef JOULE_PHANTOM_SPOOF_ENGINE_H
#define JOULE_PHANTOM_SPOOF_ENGINE_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pre-built spoof profiles that install a set of MITM rules at once. */

/* Make the host believe the battery is at 100 % SoC regardless of truth */
void spoof_profile_full_charge(void);

/* Make the host believe the battery is critically low (1 % SoC, 3.0V) */
void spoof_profile_empty(void);

/* Inject persistent over-temperature alarm (60 C) */
void spoof_profile_overtemp(void);

/* Inject over-current alarm to force host to throttle / shut down */
void spoof_profile_overcurrent(void);

/* Clone: copy a real battery's ManufacturerName/DeviceName/Serial into
 * local cache so the clone profile can replay them later.            */
void spoof_capture_identity(uint8_t *mfr_name, uint8_t *dev_name,
                            uint16_t *serial, uint16_t *date,
                            uint16_t *design_cap);

/* Replay previously-captured identity onto a different battery pack */
void spoof_profile_clone(void);

/* Disable charging by spoofing ChargingCurrent = 0 and
 * ChargingVoltage = 0 to the host.                                    */
void spoof_profile_no_charge(void);

/* Clear all spoof rules */
void spoof_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* JOULE_PHANTOM_SPOOF_ENGINE_H */