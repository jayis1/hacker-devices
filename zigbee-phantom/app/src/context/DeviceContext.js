// src/context/DeviceContext.js — BLE connection & device state management
// Author: jayis1
// License: MIT
//
// Manages the BLE connection to ZIGBEE-PHANTOM, parses incoming frame/event
// notifications, and exposes device state + command methods to all screens.

import React, { createContext, useContext, useState, useEffect, useRef } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

// Nordic UART Service UUIDs (reused for ZIGBEE-PHANTOM custom GATT profile)
const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const TX_CHAR_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write (app → device)
const RX_CHAR_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify (device → app)

// Command opcodes (must match esp_backhaul.h)
const CMD = {
  SET_MODE: 0x10,
  SET_CHANNEL: 0x11,
  SET_PAN_FILTER: 0x12,
  SET_EUI_FILTER: 0x13,
  INJECT_ZCL: 0x20,
  INJECT_BEACON: 0x21,
  INJECT_REJOIN: 0x22,
  BRUTE_KEY: 0x30,
  ENERGY_SCAN: 0x40,
  GET_STATUS: 0x50,
};

// Event IDs
const EVT = {
  KEY_CAPTURED: 0x01,
  MODE_CHANGE: 0x02,
  CHANNEL_CHANGE: 0x03,
  JAM_TRIGGER: 0x04,
  BATT_LOW: 0x05,
  ERROR: 0xFF,
};

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [device, setDevice] = useState(null);
  const [status, setStatus] = useState({
    mode: 0, channel: 15, rssi: -128, battPct: 100,
    frameCount: 0, keyCount: 0, injectCount: 0,
  });
  const [frames, setFrames] = useState([]);       // recent captured frames
  const [keyFrames, setKeyFrames] = useState([]); // Transport-Key captures
  const [topology, setTopology] = useState({});   // EUI64 → device info
  const [energyScan, setEnergyScan] = useState(new Array(16).fill(-100));
  const bleManager = useRef(null);
  const rxBuffer = useRef([]);

  useEffect(() => {
    bleManager.current = new BleManager();
    return () => {
      bleManager.current?.destroy();
    };
  }, []);

  // Scan for and connect to ZIGBEE-PHANTOM
  async function connect() {
    if (!bleManager.current) return;
    bleManager.current.startDeviceScan([SERVICE_UUID], null, (error, dev) => {
      if (error) { console.warn('Scan error:', error); return; }
      if (dev?.name?.includes('ZB-PHANTOM')) {
        bleManager.current.stopDeviceScan();
        dev.connect()
          .then((d) => d.discoverAllServicesAndCharacteristics())
          .then((d) => {
            setDevice(d);
            setConnected(true);
            // Subscribe to RX notifications
            d.setupNotification(RX_CHAR_UUID)
              .then(() => d.monitor(RX_CHAR_UUID, onNotify));
          })
          .catch((e) => console.warn('Connect error:', e));
      }
    });
  }

  // Parse incoming BLE notifications (frames + events)
  function onNotify(error, characteristic) {
    if (error || !characteristic?.value) return;
    const buf = Buffer.from(characteristic.value, 'base64');
    const marker = buf[0];
    if (marker === 0xA1) {
      // Frame capture
      const ch = buf[1];
      const rssi = buf.readInt8(2);
      const lqi = buf[3];
      const ts = buf.readUInt32LE(4);
      const len = buf[8];
      const frame = buf.slice(9, 9 + len);
      setFrames((prev) => [
        ...prev.slice(-199),
        { channel: ch, rssi, lqi, ts, frame: frame.toString('hex'), time: Date.now() },
      ]);
      // Update topology from frame addressing
      updateTopology(frame, ch, rssi);
    } else if (marker === 0xB1) {
      // Event
      const evtId = buf[1];
      const plen = buf[2];
      const payload = buf.slice(3, 3 + plen);
      handleEvent(evtId, payload);
    }
  }

  function handleEvent(evtId, payload) {
    switch (evtId) {
      case EVT.KEY_CAPTURED:
        setKeyFrames((prev) => [...prev, {
          time: Date.now(),
          keyBlob: payload.toString('hex'),
          keyType: payload.length > 16 ? 'brute-result' : 'transport-key',
        }]);
        break;
      case EVT.MODE_CHANGE:
        setStatus((s) => ({ ...s, mode: payload[0] }));
        break;
      case EVT.CHANNEL_CHANGE:
        setStatus((s) => ({ ...s, channel: payload[0] }));
        break;
      case EVT.BATT_LOW:
        setStatus((s) => ({ ...s, battPct: payload[0] }));
        break;
      case EVT.ERROR:
        console.warn('Device error event');
        break;
      default:
        // Energy scan response (16 RSSI bytes)
        if (evtId === 0x00 && payload.length === 16) {
          const scan = [];
          for (let i = 0; i < 16; i++) scan.push(payload.readInt8(i));
          setEnergyScan(scan);
        }
    }
  }

  // Extract EUI64 and PAN from raw 802.15.4 frame for topology
  function updateTopology(frame, channel, rssi) {
    if (frame.length < 3) return;
    const fc = frame.readUInt16LE(0);
    const srcMode = (fc >> 14) & 0x03;
    let idx = 2 + 1; // FC + seq
    const dstMode = (fc >> 10) & 0x03;
    if (dstMode !== 0) idx += 2;       // dst PAN
    if (dstMode === 2) idx += 2;      // short dst
    else if (dstMode === 3) idx += 8; // long dst
    if (srcMode !== 0) {
      if (!(fc & 0x40)) idx += 2;     // src PAN (if not compressed)
      if (srcMode === 2) idx += 2;
      else if (srcMode === 3) {
        const eui = frame.slice(idx, idx + 8).toString('hex');
        setTopology((prev) => ({
          ...prev,
          [eui]: {
            eui, channel, rssi, lastSeen: Date.now(),
            pan: frame.slice(3, 5).toString('hex'),
          },
        }));
      }
    }
  }

  // Send a command to the device
  async function sendCommand(cmdId, payload) {
    if (!device) return;
    const buf = Buffer.alloc(2 + (payload?.length || 0));
    buf[0] = cmdId;
    buf[1] = payload?.length || 0;
    if (payload) Buffer.from(payload).copy(buf, 2);
    await device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, TX_CHAR_UUID, buf.toString('base64'));
  }

  // Public command helpers
  const commands = {
    setMode: (mode) => sendCommand(CMD.SET_MODE, Buffer.from([mode])),
    setChannel: (ch) => sendCommand(CMD.SET_CHANNEL, Buffer.from([ch])),
    setPanFilter: (pan) => sendCommand(CMD.SET_PAN_FILTER, Buffer.from([pan & 0xFF, (pan >> 8) & 0xFF])),
    setEuiFilter: (euiHex) => sendCommand(CMD.SET_EUI_FILTER, Buffer.from(euiHex.match(/.{2}/g).map(h => parseInt(h, 16)))),
    injectZcl: (dstShort, zclCmd) => sendCommand(CMD.INJECT_ZCL, Buffer.from([dstShort & 0xFF, (dstShort >> 8) & 0xFF, zclCmd])),
    injectBeacon: () => sendCommand(CMD.INJECT_BEACON, Buffer.alloc(0)),
    bruteKey: (encKeyHex, icLen) => {
      const key = Buffer.from(encKeyHex.match(/.{2}/g).map(h => parseInt(h, 16)));
      return sendCommand(CMD.BRUTE_KEY, Buffer.concat([key, Buffer.from([icLen])]));
    },
    energyScan: () => sendCommand(CMD.ENERGY_SCAN, Buffer.alloc(0)),
    getStatus: () => sendCommand(CMD.GET_STATUS, Buffer.alloc(0)),
  };

  return (
    <DeviceContext.Provider value={{
      connected, connect, device,
      status, frames, keyFrames, topology, energyScan,
      commands,
    }}>
      {children}
    </DeviceContext.Provider>
  );
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error('useDevice must be used within DeviceProvider');
  return ctx;
}