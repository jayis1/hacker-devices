// src/store.ts — Zustand app store
// Author: jayis1   License: GPL-2.0

import { create } from 'zustand';

export interface MissionParams {
  loFreqHz: number;
  intWindowUs: number;
  framePeriodUs: number;
  durationS: number;
  wavelengthNm: number;
  biasTrim: number;
  tdcRes: number;
  exfilPolicy: number;
  streamEvents: boolean;
  name: string;
}

export interface FramePreview {
  seq: number;
  width: number;
  height: number;
  mags: Uint8Array;       // 8-bit magnitude per pixel
}

export interface LogEntry {
  ts: number;
  tag: string;
  msg: string;
}

interface AuroraState {
  connected: boolean;
  deviceId: string | null;
  batteryPct: number;
  sdFreeKB: number;
  mode: string;
  mission: MissionParams;
  frames: FramePreview[];
  spectrum: number[];        // last FFT magnitudes
  log: LogEntry[];
  setConnected: (v: boolean, id?: string) => void;
  setDeviceStatus: (b: number, sd: number, m: string) => void;
  setMission: (p: Partial<MissionParams>) => void;
  pushFrame: (f: FramePreview) => void;
  setSpectrum: (s: number[]) => void;
  pushLog: (tag: string, msg: string) => void;
  clearFrames: () => void;
}

const defaultMission: MissionParams = {
  loFreqHz: 0,
  intWindowUs: 50000,
  framePeriodUs: 100000,
  durationS: 120,
  wavelengthNm: 550,
  biasTrim: 2048,
  tdcRes: 1,
  exfilPolicy: 0,
  streamEvents: true,
  name: 'mission-1',
};

export const useStore = create<AuroraState>((set) => ({
  connected: false,
  deviceId: null,
  batteryPct: 0,
  sdFreeKB: 0,
  mode: 'standby',
  mission: defaultMission,
  frames: [],
  spectrum: [],
  log: [],
  setConnected: (v, id) => set((s) => ({
    connected: v, deviceId: id ?? s.deviceId,
  })),
  setDeviceStatus: (b, sd, m) => set({ batteryPct: b, sdFreeKB: sd, mode: m }),
  setMission: (p) => set((s) => ({ mission: { ...s.mission, ...p } })),
  pushFrame: (f) => set((s) => ({ frames: [...s.frames.slice(-99), f] })),
  setSpectrum: (sp) => set({ spectrum: sp }),
  pushLog: (tag, msg) => set((s) => ({
    log: [...s.log.slice(-199), { ts: Date.now(), tag, msg }],
  })),
  clearFrames: () => set({ frames: [] }),
}));