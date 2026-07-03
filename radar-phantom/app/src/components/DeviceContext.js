/**
 * DeviceContext.js — BLE connection manager & shared device state
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Manages the BLE link to the RadarPhantom device, exposes the binary
 * command protocol to all screens, and tracks live phantom parameters.
 * Uses react-native-ble-plx for the actual radio link.
 */

import React, { createContext, useContext, useState, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import {
  OP, RSP, encodeFrame, FrameDecoder,
  u32, s32, s16, parseU32, parseS32, parseU64,
} from './DeviceProtocol';

const SERVICE_UUID = '0000ff00-0000-1000-8000-00805f9b34fb';
const TX_CHAR_UUID  = '0000ff01-0000-1000-8000-00805f9b34fb';  // write (phone -> device)
const RX_CHAR_UUID  = '0000ff02-0000-1000-8000-00805f9b34fb';  // notify (device -> phone)

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [manager] = useState(() => new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [firmware, setFirmware] = useState(0);
  const [battery, setBattery] = useState(0);
  const [armed, setArmed] = useState(false);
  const [rangeCm, setRangeCm] = useState(3000);
  const [velMmps, setVelMmps] = useState(0);
  const [rcsQdb, setRcsQdb] = useState(100);
  const [taps, setTaps] = useState(1);
  const [bandInfo, setBandInfo] = useState(null);
  const [logEntries, setLogEntries] = useState([]);
  const decoder = new FrameDecoder();

  const handleFrame = useCallback((opcode, payload) => {
    switch (opcode) {
      case RSP.PING:
        setFirmware((payload[0] | (payload[1] << 8)));
        setBattery(payload[2]);
        break;
      case RSP.SET_RANGE:
        break;
      case RSP.SNIFF: {
        const ok = payload[0];
        if (ok) {
          const fStart = parseU64(payload, 1);
          const bw = parseU64(payload, 6);
          setBandInfo({ valid: true, startHz: fStart, bwHz: bw });
        } else {
          setBandInfo({ valid: false });
        }
        break;
      }
      case RSP.ARM:
        setArmed(true);
        break;
      case RSP.DISARM:
        setArmed(false);
        break;
      default:
        break;
    }
  }, []);

  const send = useCallback(async (opcode, payload) => {
    if (!device) return;
    const frame = encodeFrame(opcode, payload);
    const base64 = btoa(String.fromCharCode(...frame));
    await device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, TX_CHAR_UUID, base64);
  }, [device]);

  // Commands
  const commands = {
    ping: async () => { await send(OP.PING, []); },
    setRange: async (cm) => {
      await send(OP.SET_RANGE, u32(Math.round(cm)));
      setRangeCm(cm);
    },
    setVelocity: async (mmps) => {
      await send(OP.SET_VEL, s32(mmps));
      setVelMmps(mmps);
    },
    setRcs: async (qdb) => {
      await send(OP.SET_RCS, s16(qdb));
      setRcsQdb(qdb);
    },
    loadScenario: async (slot) => { await send(OP.LOAD_SCN, [slot]); },
    sniff: async (timeoutS) => { await send(OP.SNIFF, [timeoutS]); },
    arm: async () => { await send(OP.ARM, []); },
    disarm: async () => { await send(OP.DISARM, []); },
    setPower: async (code) => { await send(OP.SET_POWER, [code]); },
    disconnect: async () => {
      if (device) { await device.cancelConnection(); }
      setDevice(null);
      setConnected(false);
      setArmed(false);
    },
  };

  const connect = useCallback(async (deviceId) => {
    const d = await manager.connectToDevice(deviceId);
    await d.discoverAllServicesAndCharacteristics();
    // subscribe to notifications
    await d.monitorCharacteristicForService(
      SERVICE_UUID, RX_CHAR_UUID,
      (err, char) => {
        if (err || !char?.value) return;
        const bytes = Uint8Array.from(
          atob(char.value), c => c.charCodeAt(0));
        decoder.feed(bytes, handleFrame);
      });
    setDevice(d);
    setConnected(true);
    // initial ping
    await send(OP.PING, []);
  }, [manager, decoder, handleFrame, send]);

  const scan = useCallback((onDevice, onError) => {
    manager.startDeviceScan([SERVICE_UUID], null, (err, d) => {
      if (err) { onError && onError(err); return; }
      if (d) onDevice && onDevice(d);
    });
    return () => manager.stopDeviceScan();
  }, [manager]);

  const value = {
    manager, device, connected, firmware, battery, armed,
    rangeCm, velMmps, rcsQdb, taps, bandInfo, logEntries,
    connect, disconnect, scan, commands,
    setRangeCm, setVelMmps, setRcsQdb, setTaps,
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