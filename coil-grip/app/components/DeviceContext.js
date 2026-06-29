/*
 * DeviceContext.js — React context for the BLE connection to CoilGrip
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Central state for the BLE link: scanning, pairing, AES-256-CTR session
 * key derivation, command framing, and telemetry dispatch. All screens
 * consume this context via useDevice().
 */

import React, { createContext, useContext, useState, useCallback, useRef } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

// CoilGrip Nordic UART Service UUIDs
const NUS_SERVICE_UUID  = '6e400001-b5a3-fd39-21e4-6fa150e2a5a3';
const NUS_TX_CHAR_UUID  = '6e400002-b5a3-fd39-21e4-6fa150e2a5a3'; // write to device
const NUS_RX_CHAR_UUID  = '6e400003-b5a3-fd39-21e4-6fa150e2a5a3'; // notify from device

// Frame markers (mirror firmware ble_uart.c)
const SOF = 0xAA, EOF = 0x55;
const FRAME_MAX = 128;

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [device, setDevice] = useState(null);
  const [telemetry, setTelemetry] = useState({
    mode: 'IDLE',
    coilTempC: 25,
    currentA: 0,
    powerW: 0,
    fodUnlocked: false,
    glitchArmed: false,
    glitchFires: 0,
  });
  const [log, setLog] = useState([]);
  const managerRef = useRef(null);
  const sessionKeyRef = useRef(null);   // Uint8Array(32)
  const seqRef = useRef(0);

  const addLog = useCallback((entry) => {
    setLog((prev) => [...prev.slice(-199), { t: Date.now(), ...entry }]);
  }, []);

  // ---- AES-CTR helpers (software, matches firmware) --------------------
  // We use a tiny pure-JS AES-256 block encrypt for the C2 keystream.
  // In production you'd use expo-crypto's AES, but this keeps the app
  // self-contained for the reference build.
  const aesSbox = [
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
  ];
  const xtime = (x) => ((x << 1) ^ ((x & 0x80) ? 0x1b : 0)) & 0xff;

  const aesEncryptBlock = (key, input) => {
    const state = Array.from(input);
    const rk = expandKey(key);
    for (let i = 0; i < 16; i++) state[i] ^= rk[i];
    for (let round = 1; round < 14; round++) {
      for (let i = 0; i < 16; i++) state[i] = aesSbox[state[i]];
      // ShiftRows
      let t = state[1]; state[1]=state[5]; state[5]=state[9]; state[9]=state[13]; state[13]=t;
      t=state[2]; state[2]=state[10]; state[10]=t; t=state[6]; state[6]=state[14]; state[14]=t;
      t=state[15]; state[15]=state[11]; state[11]=state[7]; state[7]=state[3]; state[3]=t;
      // MixColumns
      for (let c=0;c<4;c++){
        const a0=state[c*4],a1=state[c*4+1],a2=state[c*4+2],a3=state[c*4+3];
        const tt=a0^a1^a2^a3;
        state[c*4]^=tt^xtime(a0^a1);
        state[c*4+1]^=tt^xtime(a1^a2);
        state[c*4+2]^=tt^xtime(a2^a3);
        state[c*4+3]^=tt^xtime(a3^a0);
      }
      for (let i=0;i<16;i++) state[i]^=rk[round*16+i];
    }
    for (let i=0;i<16;i++) state[i]=aesSbox[state[i]];
    let t=state[1]; state[1]=state[5]; state[5]=state[9]; state[9]=state[13]; state[13]=t;
    t=state[2]; state[2]=state[10]; state[10]=t; t=state[6]; state[6]=state[14]; state[14]=t;
    t=state[15]; state[15]=state[11]; state[11]=state[7]; state[7]=state[3]; state[3]=t;
    for (let i=0;i<16;i++) state[i]^=rk[14*16+i];
    return state;
  };

  const expandKey = (key) => {
    const rk = Array.from(key);
    let rcon = 1;
    for (let i = 32; i < 240; i += 4) {
      const t = [rk[i-4], rk[i-3], rk[i-2], rk[i-1]];
      if (i % 32 === 0) {
        const tmp = t[0];
        t[0] = aesSbox[t[1]] ^ rcon;
        t[1] = aesSbox[t[2]];
        t[2] = aesSbox[t[3]];
        t[3] = aesSbox[tmp];
        rcon = ((rcon << 1) ^ ((rcon & 0x80) ? 0x1b : 0)) & 0xff;
      }
      for (let j = 0; j < 4; j++) rk[i+j] = rk[i-32+j] ^ t[j];
    }
    return rk;
  };

  // ---- Frame helpers ----------------------------------------------------
  const crc16 = (data) => {
    let crc = 0xffff;
    for (const b of data) {
      crc ^= (b << 8);
      for (let i = 0; i < 8; i++)
        crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
    }
    return crc;
  };

  const encryptPayload = (data, key, ctr) => {
    const out = new Uint8Array(data.length);
    let ks = new Uint8Array(16);
    let kidx = 16;
    let c = ctr;
    for (let i = 0; i < data.length; i++) {
      if (kidx >= 16) {
        const blk = new Uint8Array(16);
        for (let j = 0; j < 16; j++) blk[j] = key[j] ^ key[(j+16) & 31];
        blk[15] ^= (c & 0xff); blk[14] ^= ((c>>8)&0xff);
        ks = aesEncryptBlock(key, blk);
        kidx = 0; c++;
      }
      out[i] = data[i] ^ ks[kidx++];
    }
    return out;
  };

  // ---- Connection -------------------------------------------------------
  const connect = useCallback(async () => {
    setConnecting(true);
    try {
      if (!managerRef.current) managerRef.current = new BleManager();
      const mgr = managerRef.current;
      const dev = await mgr.connectToDevice(NUS_SERVICE_UUID);
      await dev.discoverAllServicesAndCharacteristics();
      const txChar = await dev.readCharacteristicForService(NUS_SERVICE_UUID, NUS_TX_CHAR_UUID);
      const rxChar = await dev.readCharacteristicForService(NUS_SERVICE_UUID, NUS_RX_CHAR_UUID);

      // Derive a session key (in production: ECDH P-256 with device public key)
      const key = new Uint8Array(32);
      for (let i = 0; i < 32; i++) key[i] = (Math.random()*256)&0xff;
      sessionKeyRef.current = key;
      seqRef.current = 0;

      // Subscribe to notifications
      dev.monitorCharacteristicForService(NUS_SERVICE_UUID, NUS_RX_CHAR_UUID, (err, char) => {
        if (err || !char) return;
        const buf = Buffer.from(char.value, 'base64');
        handleFrame(buf);
      });

      setDevice(dev);
      setConnected(true);
      addLog({ type: 'connect', msg: 'CoilGrip connected' });
    } catch (e) {
      addLog({ type: 'error', msg: 'connect failed: ' + e.message });
    } finally {
      setConnecting(false);
    }
  }, []);

  const disconnect = useCallback(async () => {
    if (device) { try { await device.cancelConnection(); } catch(e){} }
    setDevice(null);
    setConnected(false);
    sessionKeyRef.current = null;
    addLog({ type: 'disconnect', msg: 'disconnected' });
  }, [device]);

  const handleFrame = (buf) => {
    // Minimal parse: [SOF][seq(2)][len(1)][payload][crc(2)][EOF]
    if (buf[0] !== SOF || buf[buf.length-1] !== EOF) return;
    const len = buf[3];
    let payload = buf.slice(4, 4+len);
    if (sessionKeyRef.current) {
      payload = Buffer.from(encryptPayload(payload, sessionKeyRef.current, seqRef.current));
    }
    // Heuristic: first payload byte often a status tag from firmware
    if (payload.length >= 8 && payload[0] === 0x01) {
      // telemetry frame
      const mode = payload[1];
      const modes = ['IDLE','SNIFFER','MITM','PROFILE','GLITCH','EXFIL','FOD'];
      setTelemetry((t) => ({
        ...t,
        mode: modes[mode] || '?',
        coilTempC: (payload[2] << 8) | payload[3],
        currentA: ((payload[4]<<8)|payload[5]) / 1000,
        powerW: ((payload[6]<<8)|payload[7]) / 1000,
      }));
    } else {
      addLog({ type: 'rx', msg: Array.from(payload).map(b=>b.toString(16).padStart(2,'0')).join(' ') });
    }
    seqRef.current++;
  };

  // ---- Command sending --------------------------------------------------
  const sendCommand = useCallback(async (text) => {
    if (!device) return;
    const data = Buffer.from(text, 'utf8');
    let payload = data;
    if (sessionKeyRef.current) {
      payload = Buffer.from(encryptPayload(new Uint8Array(data), sessionKeyRef.current, seqRef.current));
    }
    const frame = Buffer.alloc(payload.length + 7);
    frame[0] = SOF;
    frame.writeUInt16BE(seqRef.current, 1);
    frame[3] = payload.length & 0xff;
    payload.copy(frame, 4);
    const crc = crc16(frame.slice(1, 4 + payload.length));
    frame.writeUInt16BE(crc, 4 + payload.length);
    frame[6 + payload.length] = EOF;
    try {
      await device.writeCharacteristicWithResponseForService(
        NUS_SERVICE_UUID, NUS_TX_CHAR_UUID, frame.toString('base64'));
      addLog({ type: 'tx', msg: text });
    } catch (e) {
      addLog({ type: 'error', msg: 'send failed: ' + e.message });
    }
    seqRef.current++;
  }, [device]);

  const value = {
    connected, connecting, telemetry, log,
    connect, disconnect, sendCommand, addLog,
  };
  return <DeviceContext.Provider value={value}>{children}</DeviceContext.Provider>;
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error('useDevice must be inside DeviceProvider');
  return ctx;
}