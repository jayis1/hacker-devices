/**
 * Tactile-Phantom — Companion App
 * src/store.ts — App state management (Zustand)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import { create } from 'zustand';
import { TouchEvent, BusStatus, InjectCommand } from './ble';

interface LayoutKey {
  x: number;
  y: number;
  width: number;
  height: number;
  label: string;
}

interface TactilePhantomState {
  // Connection
  connected: boolean;
  deviceName: string | null;

  // Bus status
  status: BusStatus | null;

  // Events
  events: TouchEvent[];
  maxEvents: number;

  // Layout
  layoutKeys: LayoutKey[];
  screenResolution: { width: number; height: number };

  // Injection
  injectArmed: boolean;

  // Actions
  setConnected: (connected: boolean, name?: string) => void;
  setStatus: (status: BusStatus) => void;
  addEvent: (event: TouchEvent) => void;
  clearEvents: () => void;
  setLayout: (keys: LayoutKey[]) => void;
  setScreenResolution: (w: number, h: number) => void;
  setInjectArmed: (armed: boolean) => void;
}

export const useStore = create<TactilePhantomState>((set, get) => ({
  connected: false,
  deviceName: null,
  status: null,
  events: [],
  maxEvents: 10000,
  layoutKeys: [],
  screenResolution: { width: 1080, height: 2400 },
  injectArmed: false,

  setConnected: (connected, name) =>
    set({ connected, deviceName: name || null }),

  setStatus: (status) => set({ status }),

  addEvent: (event) => {
    const { events, maxEvents } = get();
    const newEvents = [...events, event];
    if (newEvents.length > maxEvents) {
      newEvents.shift();  // remove oldest
    }
    set({ events: newEvents });
  },

  clearEvents: () => set({ events: [] }),

  setLayout: (keys) => set({ layoutKeys: keys }),

  setScreenResolution: (w, h) =>
    set({ screenResolution: { width: w, height: h } }),

  setInjectArmed: (armed) => set({ injectArmed: armed }),
}));

// Injection command type constants (mirror firmware)
export const INJ_TYPE = {
  TAP: 0,
  SWIPE: 1,
  LONG_PRESS: 2,
  MULTI: 3,
  PATTERN: 4,
  TYPE: 5,
  RAW_REG: 6,
};

// Event type constants (mirror firmware)
export const EV_TYPE = {
  NONE: 0,
  TOUCH: 1,
  RELEASE: 2,
  GESTURE: 3,
  CONFIG: 4,
  FW_UPDATE: 5,
  BUS_EVENT: 6,
  INJECT: 7,
};

export const EV_TYPE_NAMES: Record<number, string> = {
  0: 'NONE',
  1: 'TOUCH',
  2: 'RELEASE',
  3: 'GESTURE',
  4: 'CONFIG',
  5: 'FW_UPDATE',
  6: 'BUS_EVENT',
  7: 'INJECT',
};

export const VENDOR_NAMES: Record<number, string> = {
  0: 'unknown',
  1: 'Goodix',
  2: 'FocalTech',
  3: 'Synaptics',
  4: 'Ilitek',
  5: 'Cypress',
  6: 'Atmel',
  7: 'Novatek',
};