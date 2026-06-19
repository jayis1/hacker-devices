// utils/protocol.js — CBOR-style wire protocol between app and Powerline-Reaper
// Author: jayis1
// License: MIT
//
// The device speaks a length-prefixed binary protocol over BLE (covert mode)
// and over the USB CDC-ACM console (inline mode). Frames are:
//   [TYPE:1][LEN:2][PAYLOAD:LEN][CRC16:2]
// Records above that are ChaCha20-Poly1305 encrypted (tunnel mode).
// This module defines the message types and a React context that exposes
// a connection object + device state to all screens.

import React, { createContext, useContext, useState, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';

// ---- Service / characteristic UUIDs (Powerline-Reaper GATT) ----
export const REAPER_SERVICE_UUID      = '0000a155-0000-1000-8000-00805f9b34fb';
export const REAPER_CMD_CHAR_UUID     = '0000a156-0000-1000-8000-00805f9b34fb';
export const REAPER_NOTIFY_CHAR_UUID   = '0000a157-0000-1000-8000-00805f9b34fb';

// ---- Message types (operator -> device) ----
export const MSG = {
  PING:               0x01,
  SET_MODE:           0x02,
  START_SNIFF:        0x03,
  STOP_SNIFF:         0x04,
  SET_FILTER:         0x05,
  INJECT_BEACON:      0x06,
  INJECT_DEAUTH:      0x07,
  ROGUE_CCO:          0x08,
  NMK_CRACK_OFFLINE:  0x09,
  NMK_CRACK_ONLINE:   0x0A,
  NMK_ABORT:          0x0B,
  SET_NMK:            0x0C,
  TUNNEL_SET_KEY:     0x0D,
  TUNNEL_CONNECT:     0x0E,
  ZEROIZE:             0x0F,
  OTA_BEGIN:          0x10,
  OTA_CHUNK:          0x11,
  OTA_END:            0x12,
  SET_REGION:         0x13,
};

// ---- Event types (device -> operator) ----
export const EVT = {
  LINK_STATUS:   0x81,
  STATION_LIST:   0x82,
  FRAME:          0x83,
  INJECT_ACK:    0x84,
  NMK_PROGRESS:  0x85,
  NMK_RECOVERED: 0x86,
  TUNNEL_STATUS: 0x87,
  FAULT:          0x88,
  BATTERY:       0x89,
  SD_STATUS:     0x8A,
  FIRMWARE_INFO: 0x8B,
};

// ---- CRC16 (matches firmware ble_uart.c) ----
export function crc16(data) {
  let c = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    c ^= data[i];
    for (let j = 0; j < 8; j++) {
      c = (c & 1) ? (c >> 1) ^ 0xA001 : (c >> 1);
    }
  }
  return c & 0xFFFF;
}

// ---- Frame encode/decode ----
export function encodeFrame(type, payload) {
  const len = payload ? payload.length : 0;
  const buf = new Uint8Array(1 + 2 + len + 2);
  buf[0] = type;
  buf[1] = len & 0xFF;
  buf[2] = (len >> 8) & 0xFF;
  if (payload) buf.set(payload, 3);
  const c = crc16(buf.slice(0, 3 + len));
  buf[3 + len] = c & 0xFF;
  buf[4 + len] = (c >> 8) & 0xFF;
  return buf;
}

export function decodeFrame(buf) {
  if (buf.length < 5) return null;
  const type = buf[0];
  const len = buf[1] | (buf[2] << 8);
  if (buf.length < 3 + len + 2) return null;
  const payload = buf.slice(3, 3 + len);
  const crc = buf[3 + len] | (buf[4 + len] << 8);
  const calc = crc16(buf.slice(0, 3 + len));
  if (crc !== calc) return null;
  return { type, payload };
}

// ---- React context ----
const ReaperCtx = createContext(null);

export function ReaperProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [device, setDevice] = useState(null);
  const [status, setStatus] = useState({
    plcLink: false,
    networkId: '',
    ccoMac: '',
    stationCount: 0,
    batteryPct: 0,
    batteryMv: 0,
    sdFree: 0,
    captureRate: 0,
    mode: 'INIT',
    caps: 0,
  });
  const [stations, setStations] = useState([]);
  const [frames, setFrames] = useState([]);
  const [nmkProgress, setNmkProgress] = useState({ running: false, tried: 0, rate: 0, recovered: false, nmk: null });
  const [tunnel, setTunnel] = useState({ up: false, peer: '', rxBytes: 0, txBytes: 0 });
  const manager = React.useRef(null);

  const connect = useCallback(async () => {
    if (!manager.current) manager.current = new BleManager();
    const m = manager.current;
    m.startDeviceScan([REAPER_SERVICE_UUID], null, (err, dev) => {
      if (err) { console.warn('scan err', err); return; }
      if (dev.name && dev.name.includes('Powerline-Reaper')) {
        m.stopDeviceScan();
        dev.connect().then((d) => {
          setDevice(d);
          setConnected(true);
          return d.discoverAllServicesAndCharacteristics();
        }).then((d) => {
          d.setupNotifications(REAPER_NOTIFY_CHAR_UUID, (data) => {
            const f = decodeFrame(new Uint8Array(data));
            if (f) handleEvent(f);
          });
        }).catch((e) => console.warn('connect err', e));
      }
    });
  }, []);

  const send = useCallback(async (type, payload) => {
    if (!device) return;
    const f = encodeFrame(type, payload || new Uint8Array(0));
    await device.writeCharacteristicWithoutResponse(
      REAPER_SERVICE_UUID, REAPER_CMD_CHAR_UUID, f.buffer);
  }, [device]);

  const handleEvent = useCallback((f) => {
    const p = f.payload;
    switch (f.type) {
    case EVT.LINK_STATUS:
      setStatus((s) => ({
        ...s,
        plcLink: p[0] ? true : false,
        mode: ['INIT','LINK','SNIFF','BRIDGE','INJECT','NMK','TUNNEL','COVERT','FAULT'][p[1]] || 'INIT',
        caps: p[2] | (p[3] << 8),
      }));
      break;
    case EVT.STATION_LIST: {
      // each station: 6 mac + 1 tei + 1 snr + 1 role + 1 nmk_match = 10 bytes
      const list = [];
      for (let i = 0; i + 10 <= p.length; i += 10) {
        list.push({
          mac: Array.from(p.slice(i, i + 6)).map((b) => b.toString(16).padStart(2,'0')).join(':'),
          tei: p[i + 6],
          snr: p[i + 7],
          role: ['NONE','CCO','PROXY','LEAF'][p[i + 8]] || 'NONE',
          nmkMatch: p[i + 9] ? true : false,
        });
      }
      setStations(list);
      setStatus((s) => ({ ...s, stationCount: list.length }));
      break;
    }
    case EVT.FRAME: {
      const ts = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
      const tei = p[4], snr = p[5], opcode = p[6];
      const src = Array.from(p.slice(7, 13)).map((b)=>b.toString(16).padStart(2,'0')).join(':');
      const dst = Array.from(p.slice(13, 19)).map((b)=>b.toString(16).padStart(2,'0')).join(':');
      setFrames((arr) => {
        const next = [{ ts, tei, snr, opcode, src, dst }, ...arr].slice(0, 200);
        return next;
      });
      break;
    }
    case EVT.NMK_PROGRESS:
      setNmkProgress({
        running: p[0] ? true : false,
        tried: p[1] | (p[2] << 8) | (p[3] << 16) | (p[4] << 24),
        rate: p[5] | (p[6] << 8),
        recovered: p[7] ? true : false,
        nmk: p[7] ? Array.from(p.slice(8, 24)) : null,
      });
      break;
    case EVT.BATTERY:
      setStatus((s) => ({
        ...s,
        batteryMv: p[0] | (p[1] << 8),
        batteryPct: p[2],
      }));
      break;
    case EVT.TUNNEL_STATUS:
      setTunnel({
        up: p[0] ? true : false,
        peer: Array.from(p.slice(1, 7)).map((b)=>b.toString(16).padStart(2,'0')).join(':'),
        rxBytes: p[7] | (p[8] << 8) | (p[9] << 16) | (p[10] << 24),
        txBytes: p[11] | (p[12] << 8) | (p[13] << 16) | (p[14] << 24),
      });
      break;
    case EVT.FAULT:
      setStatus((s) => ({ ...s, mode: 'FAULT' }));
      break;
    default:
      break;
    }
  }, []);

  const value = {
    connected, device, status, stations, frames, nmkProgress, tunnel,
    connect, send,
  };

  return <ReaperCtx.Provider value={value}>{children}</ReaperCtx.Provider>;
}

export function useReaper() {
  const ctx = useContext(ReaperCtx);
  if (!ctx) throw new Error('useReaper must be used within ReaperProvider');
  return ctx;
}