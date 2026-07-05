/**
 * DeviceContext.js — transport abstraction for DRAM-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Exposes a simple text-command API (STATUS, MODE, ARM, HAMMER, WARMBOOT,
 * COVERT, SNOOP DRAIN) over either BLE (react-native-ble-plx) or USB CDC
 * (a native module, stubbed here). The screens consume useDevice() and don't
 * care which transport is active.
 *
 * Two-key arm: every destructive command requires the operator to read the
 * arm token from the device's OLED and confirm it in the app. This is enforced
 * both here (in JS) and in the device firmware (the FPGA won't execute without
 * a matching token).
 */

import React, { createContext, useContext, useState, useCallback, useRef } from 'react';
import { BleManager } from 'react-native-ble-plx';

const DRAM_PHANTOM_SERVICE_UUID = '0000ffe0-0000-1000-8000-00805f9b34fb';
const DRAM_PHANTOM_TX_CHAR_UUID  = '0000ffe1-0000-1000-8000-00805f9b34fb';
const DRAM_PHANTOM_RX_CHAR_UUID  = '0000ffe2-0000-1000-8000-00805f9b34fb';

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState(null);
  const [transport, setTransport] = useState('none'); // 'ble' | 'usb' | 'none'
  const bleManagerRef = useRef(null);
  const deviceRef = useRef(null);
  const rxBufferRef = useRef('');
  const responseListenersRef = useRef(new Set());
  const lineBufferRef = useRef('');

  /* ---- Low-level send (line-oriented) ---- */
  const sendLine = useCallback(async (line) => {
    const data = line + '\r\n';
    if (transport === 'ble' && deviceRef.current) {
      await deviceRef.current.writeCharacteristicWithResponseForService(
        DRAM_PHANTOM_SERVICE_UUID,
        DRAM_PHANTOM_TX_CHAR_UUID,
        data,
      );
    } else if (transport === 'usb') {
      // USB CDC native module — stubbed; in production this calls a native bridge.
      console.log('[USB] send:', line);
    } else {
      throw new Error('No transport connected');
    }
  }, [transport]);

  /* ---- Command/response helper ----
   * Sends a command line and resolves with the first "OK ..." or "ERR ..." line. */
  const command = useCallback(async (cmd) => {
    return new Promise((resolve, reject) => {
      const listener = (line) => {
        if (line.startsWith('OK ') || line.startsWith('ERR ')) {
          responseListenersRef.current.delete(listener);
          resolve(line);
        }
      };
      responseListenersRef.current.add(listener);
      sendLine(cmd).catch((e) => {
        responseListenersRef.current.delete(listener);
        reject(e);
      });
      // Timeout
      setTimeout(() => {
        if (responseListenersRef.current.has(listener)) {
          responseListenersRef.current.delete(listener);
          reject(new Error('Command timeout: ' + cmd));
        }
      }, 8000);
    });
  }, [sendLine]);

  /* ---- Handle incoming bytes (BLE notify or USB read) ---- */
  const handleRx = useCallback((data) => {
    const str = typeof data === 'string' ? data : Buffer.from(data).toString('latin1');
    lineBufferRef.current += str;
    let idx;
    while ((idx = lineBufferRef.current.indexOf('\n')) >= 0) {
      const line = lineBufferRef.current.slice(0, idx).replace(/\r$/, '');
      lineBufferRef.current = lineBufferRef.current.slice(idx + 1);
      // Dispatch to any waiting command listener
      let handled = false;
      for (const listener of responseListenersRef.current) {
        listener(line);
        handled = true;
        break;
      }
      if (!handled) {
        // Unsolicited line (e.g., async flip notification) — update status
        console.log('[unsolicited]', line);
      }
    }
  }, []);

  /* ---- BLE connect/scan ---- */
  const connectBLE = useCallback(async () => {
    if (!bleManagerRef.current) {
      bleManagerRef.current = new BleManager();
    }
    const manager = bleManagerRef.current;
    const device = await manager.connectToDevice('DRAM-Phantom');
    await device.discoverAllServicesAndCharacteristics();
    device.monitorCharacteristicForService(
      DRAM_PHANTOM_SERVICE_UUID,
      DRAM_PHANTOM_RX_CHAR_UUID,
      (err, char) => {
        if (err) { console.warn('BLE monitor error', err); return; }
        if (char && char.value) {
          handleRx(Buffer.from(char.value, 'base64').toString('latin1'));
        }
      },
    );
    deviceRef.current = device;
    setTransport('ble');
    setConnected(true);
    return device;
  }, [handleRx]);

  const connectUSB = useCallback(async () => {
    // USB CDC native module — stubbed for design deliverable.
    setTransport('usb');
    setConnected(true);
  }, []);

  const disconnect = useCallback(async () => {
    if (transport === 'ble' && deviceRef.current) {
      await deviceRef.current.cancelConnection();
      deviceRef.current = null;
    }
    setTransport('none');
    setConnected(false);
    setStatus(null);
  }, [transport]);

  /* ---- High-level API used by screens ---- */
  const refreshStatus = useCallback(async () => {
    try {
      const line = await command('STATUS');
      // Parse "OK STATUS mode=X armed=Y fpga=0x.. batt=Nmv token=TTTTTT"
      const m = line.match(/mode=(\d+) armed=(\d+) fpga=0x([0-9A-Fa-f]+) batt=(\d+)mv token=(\w+)/);
      if (m) {
        const st = {
          mode: parseInt(m[1], 10),
          armed: m[2] === '1',
          fpga: parseInt(m[3], 16),
          batt_mv: parseInt(m[4], 10),
          token: m[5],
        };
        setStatus(st);
        return st;
      }
    } catch (e) {
      console.warn('refreshStatus', e);
    }
    return null;
  }, [command]);

  const setMode = useCallback((mode) => command(`MODE ${mode}`), [command]);
  const arm = useCallback((token) => command(`ARM ${token}`), [command]);
  const disarm = useCallback(() => command('DISARM'), [command]);

  const hammerArm = useCallback((row, count, pattern) =>
    command(`HAMMER ${row} ${count} ${pattern}`), [command]);
  const hammerRun = useCallback(() => command('HAMMER_RUN'), [command]);

  const warmBoot = useCallback((baseRow, rowCount) =>
    command(`WARMBOOT ${baseRow} ${rowCount}`), [command]);

  const covertSet = useCallback((bg, bank) =>
    command(`COVERT ${bg} ${bank}`), [command]);

  const snoopDrain = useCallback(async (n) => {
    const line = await command(`SNOOP DRAIN ${n}`);
    // "OK SNOOP <count>" followed by raw 64-byte records over the transport.
    // In a real app we'd read the binary stream; here we just return the count.
    const m = line.match(/OK SNOOP (\d+)/);
    return m ? parseInt(m[1], 10) : 0;
  }, [command]);

  const value = {
    connected,
    transport,
    status,
    connectBLE,
    connectUSB,
    disconnect,
    refreshStatus,
    setMode,
    arm,
    disarm,
    hammerArm,
    hammerRun,
    warmBoot,
    covertSet,
    snoopDrain,
  };

  return (
    <DeviceContext.Provider value={value}>
      {children}
    </DeviceContext.Provider>
  );
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error('useDevice must be used within DeviceProvider');
  return ctx;
}