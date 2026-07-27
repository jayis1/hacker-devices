// src/types.ts — Type definitions for Chronos-Phantom app
//
// Author: jayis1
// License: GPL-2.0

export type OpMode = 'inline' | 'rogue_gm' | 'passive' | 'transparent';

export type SkewProfile = 'step' | 'ramp' | 'stealth' | 'jitter' | 'sawtooth';

export interface DeviceStatus {
  connected: boolean;
  mode: OpMode;
  skewActive: boolean;
  skewProfile: SkewProfile;
  currentSkewNs: number;      // int64 as number (may lose precision; use string for full)
  currentSkewNsStr: string;   // string representation for large values
  observedGmPriority1: number;
  observedGmClockClass: number;
  spoofedGmPriority1: number;
  spoofedGmClockClass: number;
  framesCaptured: number;
  framesModified: number;
  ntpRequests: number;
  ntpResponses: number;
  batteryMv: number;
  temperatureC: number;
  gnssLocked: boolean;
  bleRssi: number;
}

export interface PtpNode {
  clockIdentity: string;
  portNumber: number;
  isGrandmaster: boolean;
  clockClass: number;
  clockAccuracy: number;
  priority1: number;
  priority2: number;
  offsetFromMasterNs: number;
  lastSeenMs: number;
}

export interface CapturedFrame {
  timestampNs: number;
  frameLen: number;
  msgType: number;
  srcMac: string;
  dstMac: string;
  correctionField: string;   // hex
  raw: string;               // hex dump (truncated)
}

export interface BmcaConfig {
  priority1: number;
  clockClass: number;
  clockAccuracy: number;
  clockVariance: number;
  priority2: number;
  domainNumber: number;
  clockIdentity: string;
}

export interface SkewConfig {
  profile: SkewProfile;
  offsetNs: number;
  rateNsps: number;
  jitterAmpNs: number;
  sawtoothPeriodMs: number;
  active: boolean;
}

export type SkewPreset =
  | 'kerberos_ext'
  | 'kerberos_replay'
  | 'pmu_slow'
  | 'pmu_sawtooth'
  | 'jitter_100us'
  | 'custom';

export interface CovertMessage {
  timestamp: number;
  data: string;    // hex
  decoded: string; // ASCII
  direction: 'tx' | 'rx';
}

// BLE command opcodes — must match firmware ble_link.h
export const CMD = {
  GET_STATUS:        0x01,
  SET_MODE:           0x02,
  SET_SKEW_PROFILE:   0x03,
  SET_SKEW_PRESET:   0x04,
  BMCA_SPOOF:        0x05,
  BMCA_AUTO_WIN:     0x06,
  COVERT_SEND:       0x07,
  COVERT_RECV_START: 0x08,
  COVERT_RECV_STOP:  0x09,
  CAPTURE_START:     0x0A,
  CAPTURE_STOP:      0x0B,
  GNSS_DISCIPLINE:   0x0C,
  TAMPER_THRESHOLD:  0x0D,
  FIRMWARE_UPDATE:   0x0E,
  SET_SPOOF_GM:      0x0F,
  PING:              0x10,
} as const;

export const EVT = {
  STATUS:           0x81,
  FRAME_CAPTURED:   0x82,
  COVERT_RX:        0x83,
  TAMPER:           0x84,
  GNSS_LOCK:        0x85,
  PING_RESP:        0x90,
  ERROR:            0xFF,
} as const;

// Author: jayis1
// License: GPL-2.0