/**
 * deviceContext.js — React context for device connection state
 * Author: jayis1
 * License: MIT
 */

import { createContext } from 'react';

export const DeviceContext = createContext({
  device: null,
  connected: false,
  status: {
    totalShots: 0,
    successShots: 0,
    failureShots: 0,
    sweepRunning: false,
  },
  connectDevice: async () => false,
  disconnectDevice: async () => {},
  sendCommand: async () => null,
});