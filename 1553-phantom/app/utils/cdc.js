/**
 * utils/cdc.js — USB CDC bridge to the 1553-Phantom device
 *
 * Author: jayis1
 * License: MIT
 *
 * Wraps react-native-usb-serialport behind a React context so every screen
 * can send CLI commands and subscribe to a line-buffered response stream.
 * On unsupported platforms (no USB-OTG) the bridge falls back to a
 * mock that echoes a canned capture, so the UI is testable in the
 * Expo Go playground.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { Platform } from 'react-native';

const CdcContext = createContext(null);

/* ---- Mock state (used when no USB-OTG available) ---- */
const MOCK_LINES = [
  '1553-Phantom v1.0 (c) jayis1',
  'role=BM armed=0 inj=0 cap=0 chA_err=0 chB_err=0',
  '1,0,CMD,1C07,0000',
  '1,0,STA,4007,0001',
  '2,0,CMD,1C2F,0000',
  '2,0,STA,402F,0001',
  '1,0,DAT,0123,0000',
  '1,0,DAT,4567,0000',
];

export function CdcProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [mockMode, setMockMode] = useState(false);
  const portRef = useRef(null);
  const listeners = useRef(new Set());
  const rxBuffer = useRef('');

  const emitLine = useCallback((line) => {
    listeners.current.forEach((cb) => cb(line));
  }, []);

  const onLine = useCallback((cb) => {
    listeners.current.add(cb);
    return () => listeners.current.delete(cb);
  }, []);

  /* ---- Connect (tries real USB first, falls back to mock) ---- */
  const connect = useCallback(async () => {
    if (Platform.OS === 'web') {
      setMockMode(true);
      setConnected(true);
      return;
    }
    try {
      const UsbSerial = require('react-native-usb-serialport').default;
      const devices = await UsbSerial.list();
      if (devices.length === 0) {
        setMockMode(true);
        setConnected(true);
        return;
      }
      const port = await UsbSerial.open(devices[0].deviceId, 115200);
      portRef.current = port;
      port.onReceived((data) => {
        rxBuffer.current += data;
        let nl;
        while ((nl = rxBuffer.current.indexOf('\n')) >= 0) {
          const line = rxBuffer.current.slice(0, nl).replace(/\r$/, '');
          rxBuffer.current = rxBuffer.current.slice(nl + 1);
          emitLine(line);
        }
      });
      setMockMode(false);
      setConnected(true);
    } catch (e) {
      console.warn('CDC connect failed, using mock:', e);
      setMockMode(true);
      setConnected(true);
    }
  }, [emitLine]);

  /* ---- Send a CLI command ---- */
  const send = useCallback(async (cmd) => {
    if (!connected) return;
    if (mockMode) {
      emitLine(`> ${cmd}`);
      // emit a plausible response
      if (cmd.startsWith('status')) {
        emitLine(MOCK_LINES[1]);
      } else if (cmd === 'help') {
        emitLine('Commands: role arm disarm status rt bc mitm bm ...');
      } else if (cmd.startsWith('bm flush')) {
        MOCK_LINES.slice(2).forEach(emitLine);
      } else {
        emitLine('OK');
      }
      return;
    }
    if (portRef.current) {
      await portRef.current.write(cmd + '\r\n');
    }
  }, [connected, mockMode, emitLine]);

  /* ---- Disconnect ---- */
  const disconnect = useCallback(async () => {
    if (portRef.current) {
      await portRef.current.close();
      portRef.current = null;
    }
    setConnected(false);
  }, []);

  const value = { connected, mockMode, connect, disconnect, send, onLine };
  return <CdcContext.Provider value={value}>{children}</CdcContext.Provider>;
}

export function useCdc() {
  return useContext(CdcContext);
}