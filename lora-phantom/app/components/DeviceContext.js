/*
 * lora-phantom / app / components / DeviceContext.js
 * React Context for device connection state and command dispatch.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { createContext, useContext, useState, useCallback, useRef } from 'react';
import { CMD, MODES, REGIONS, encode, decode, parseStatus } from '../utils/protocol';

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [transport, setTransport] = useState(null);  // 'ble' | 'usb' | null
  const [status, setStatus] = useState(null);
  const [frames, setFrames] = useState([]);      // decoded LoRaWAN frames
  const [joinReqs, setJoinReqs] = useState([]);  // captured join-requests
  const [scanResults, setScanResults] = useState([]);
  const [keyResult, setKeyResult] = useState(null);
  const [log, setLog] = useState([]);
  const rxBufRef = useRef(new Uint8Array(2048));
  const rxBufLen = useRef(0);

  const addLog = useCallback((msg) => {
    setLog(prev => [...prev.slice(-200), { ts: Date.now(), msg }]);
  }, []);

  const sendCommand = useCallback(async (cmd, payload) => {
    if (!transport) { addLog('Not connected'); return false; }
    const pkt = encode(cmd, payload || new Uint8Array(0));
    try {
      if (transport === 'ble') {
        // Write to BLE characteristic
        await bleWrite(pkt);
      } else if (transport === 'usb') {
        await usbWrite(pkt);
      }
      return true;
    } catch (e) {
      addLog('Send error: ' + e.message);
      return false;
    }
  }, [transport, addLog]);

  const handlePacket = useCallback((cmd, payload) => {
    switch (cmd) {
      case CMD.PING:
        addLog('Ping reply: ' + hexStr(payload));
        break;
      case CMD.STATUS: {
        const s = parseStatus(payload);
        if (s) setStatus(s);
        break;
      }
      case CMD.FRAME_RECV: {
        if (payload[0] === 0x00) {
          // Join-request
          const jr = parseJoinRequest(payload);
          if (jr) {
            setJoinReqs(prev => [...prev.slice(-100), jr]);
          }
        } else {
          const f = parseFrame(payload);
          if (f) {
            setFrames(prev => [...prev.slice(-200), f]);
          }
        }
        break;
      }
      case CMD.KEYSEARCH_RESULT: {
        const r = parseKeySearchResult(payload);
        setKeyResult(r);
        if (r && r.found) addLog('Key found: ' + r.key);
        else addLog('Key not found (trials: ' + (r?.trials || 0) + ')');
        break;
      }
      case CMD.SCAN_RESULT: {
        const results = parseScanResult(payload);
        setScanResults(results);
        break;
      }
      case CMD.GWEMU_SESSION: {
        if (payload.length >= 20) {
          const devAddr = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
          const devEui = Array.from(payload.slice(4, 12)).map(b => b.toString(16).padStart(2,'0')).join(':');
          addLog(`Rogue session: DevAddr=0x${devAddr.toString(16)} DevEUI=${devEui}`);
        }
        break;
      }
      default:
        addLog(`Cmd 0x${cmd.toString(16)} (${payload.length}B)`);
    }
  }, [addLog]);

  const feedRxData = useCallback((data) => {
    // Accumulate bytes and try to decode complete frames
    const buf = rxBufRef.current;
    const len = rxBufLen.current;
    if (len + data.length > buf.length) {
      rxBufLen.current = 0;
      return;
    }
    buf.set(data, len);
    rxBufLen.current = len + data.length;

    // Try to decode from start
    while (rxBufLen.current >= 7) {
      if (buf[0] !== 0x55 || buf[1] !== 0xAA) {
        // Shift past invalid sync
        buf.copyWithin(0, 1, rxBufLen.current);
        rxBufLen.current--;
        continue;
      }
      const plen = buf[2] | (buf[3] << 8);
      const total = 4 + 1 + plen + 2;
      if (rxBufLen.current < total) break;
      const frame = buf.slice(0, total);
      const decoded = decode(frame);
      if (decoded) {
        handlePacket(decoded.cmd, decoded.payload);
      }
      buf.copyWithin(0, total, rxBufLen.current);
      rxBufLen.current -= total;
    }
  }, [handlePacket]);

  const connect = useCallback(async (transportType) => {
    setTransport(transportType);
    setConnected(true);
    addLog(`Connected via ${transportType}`);
    // Request initial status
    await sendCommand(CMD.PING);
    await sendCommand(CMD.STATUS);
  }, [sendCommand, addLog]);

  const disconnect = useCallback(() => {
    setConnected(false);
    setTransport(null);
    setStatus(null);
    setFrames([]);
    setJoinReqs([]);
    addLog('Disconnected');
  }, [addLog]);

  const clearFrames = useCallback(() => {
    setFrames([]);
    setJoinReqs([]);
  }, []);

  const value = {
    connected, transport, status,
    frames, joinReqs, scanResults, keyResult, log,
    connect, disconnect, sendCommand, feedRxData, clearFrames,
    addLog,
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

/* ---- Transport stubs (platform-specific BLE/USB implementations) ---- */
async function bleWrite(data) {
  // In a real app, this calls react-native-ble-plx characteristic.writeWithoutResponse()
  // For the Expo prototype, we stub this and log.
  // The actual BLE integration requires native module permissions.
  console.log('BLE write:', data.length, 'bytes');
}

async function usbWrite(data) {
  // USB CDC write via WebUSB / native USB serial
  console.log('USB write:', data.length, 'bytes');
}

/* Helpers imported from protocol.js for the log display */
import { hexStr, parseJoinRequest, parseFrame, parseScanResult, parseKeySearchResult } from '../utils/protocol';