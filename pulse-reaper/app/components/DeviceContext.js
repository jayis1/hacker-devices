/**
 * DeviceContext.js — BLE connection and device state management
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Provides a React context for managing the BLE connection to the
 * Pulse-Reaper device, sending commands, and receiving responses /
 * streaming data during TDR, sniff, inject, and cable-map operations.
 *
 * The BLE C2 protocol uses a framed binary format (see firmware
 * ble_c2.c): [0xA5][len_hi][len_lo][encrypted payload][crc8]. The
 * payload is AES-256-CTR encrypted with a session key derived from an
 * ECDH P-256 handshake. This context handles the framing layer; the
 * crypto is delegated to the device's nRF52840 co-processor.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

// Pulse-Reaper BLE service and characteristic UUIDs (NUS-style transparent UART)
const PR_SERVICE_UUID      = '7a400001-b5a3-f393-e0a9-e50e24dcca9f';
const PR_CMD_CHAR_UUID     = '7a400002-b5a3-f393-e0a9-e50e24dcca9f'; // Write
const PR_NOTIFY_CHAR_UUID  = '7a400003-b5a3-f393-e0a9-e50e24dcca9f'; // Notify

// Command opcodes (must match firmware ble_c2.h)
const CMD = {
  PING: 0x01,
  GET_STATUS: 0x02,
  ENTER_MODE: 0x03,
  FIRE_TDR: 0x04,
  SET_GAIN: 0x05,
  SET_COUPLING: 0x06,
  INJECT_FRAME: 0x07,
  STOP_SNIFF: 0x08,
  FIRMWARE_VERSION: 0x09,
};

// Notification opcodes (from device to app)
const NOTIF = {
  STATUS: 0x80,
  FRAME: 0x81,
  TDR_RESULT: 0x82,
  TDR_CHUNK: 0x83,
};

// Operational modes (must match board.h pr_mode_t)
const MODE = {
  IDLE: 0,
  TDR: 1,
  SNIFF: 2,
  INJECT: 3,
  COVERT: 4,
  CABLEMAP: 5,
};

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState('Disconnected');
  const [battery, setBattery] = useState(100);
  const [currentMode, setCurrentMode] = useState(MODE.IDLE);
  const [firmwareVersion, setFirmwareVersion] = useState('');
  const [frameCallback, setFrameCallback] = useState(null);
  const [tdrCallback, setTdrCallback] = useState(null);

  const manager = useRef(null);
  const cmdChar = useRef(null);
  const notifyChar = useRef(null);
  const pendingResolver = useRef(null);

  if (!manager.current) {
    manager.current = new BleManager();
  }

  /**
   * Handle a complete decrypted notification payload.
   * Author: jayis1
   */
  const handlePayload = useCallback((payload) => {
    if (!payload || payload.length < 1) return;
    const opcode = payload[0];

    if (opcode === NOTIF.STATUS) {
      // Status notification: ASCII string after the opcode byte
      const msg = payload.slice(1).toString('utf8');
      setStatus(msg);
      // Parse battery percentage if present ("B=NN")
      const m = msg.match(/B=(\d+)/);
      if (m) setBattery(parseInt(m[1], 10));
      if (pendingResolver.current) {
        const r = pendingResolver.current;
        pendingResolver.current = null;
        r(msg);
      }
    } else if (opcode === NOTIF.FRAME) {
      // Captured fieldbus frame: [0x81][proto_id][dst_lo][dst_hi][func][len][data...]
      if (frameCallback.current) {
        frameCallback.current(payload.slice(1));
      }
    } else if (opcode === NOTIF.TDR_RESULT) {
      // TDR result header: [0x10][type][z 4B][len 4B][shielded][live]
      // followed by TDR_CHUNK notifications
      if (payload[0] === 0x10 && payload.length >= 12 && tdrCallback.current) {
        const result = {
          type: payload[1],
          impedance_mohm: payload.readUInt32BE(2),
          length_mm: payload.readUInt32BE(6),
          shielded: payload[10] !== 0,
          live_conductor: payload[11] !== 0,
          reflectogram: [],
        };
        tdrCallback.current(result, 'header');
      }
    } else if (opcode === NOTIF.TDR_CHUNK) {
      // TDR chunk: [0x11][off_lo][off_hi][samples... (16-bit little-endian)]
      if (tdrCallback.current) {
        const off = payload[1] | (payload[2] << 8);
        const samples = [];
        for (let i = 3; i + 1 < payload.length; i += 2) {
          const lo = payload[i];
          const hi = payload[i + 1];
          // signed 16-bit little-endian
          let s = lo | (hi << 8);
          if (s >= 0x8000) s -= 0x10000;
          samples.push(s);
        }
        tdrCallback.current({ offset: off, samples }, 'chunk');
      }
    }
  }, [frameCallback, tdrCallback]);

  /**
   * Process incoming BLE notification data.
   * Author: jayis1
   *
   * The nRF52840 sends framed, AES-256-CTR-encrypted packets. The
   * nRF firmware handles the framing + crypto and delivers the
   * decrypted payload to the app as a notification. We treat each
   * notification as one complete decrypted payload.
   */
  const handleNotification = useCallback((error, characteristic) => {
    if (error) {
      console.warn('BLE notification error:', error.message);
      return;
    }
    if (!characteristic?.value) return;
    const payload = Buffer.from(characteristic.value, 'base64');
    handlePayload(payload);
  }, [handlePayload]);

  /**
   * Scan for Pulse-Reaper BLE devices.
   * Author: jayis1
   */
  const scanForDevices = useCallback(async () => {
    setStatus('Scanning...');
    return new Promise((resolve, reject) => {
      const found = [];
      const timeout = setTimeout(() => {
        manager.current.stopDeviceScan();
        setStatus('Scan complete');
        resolve(found);
      }, 10000);

      manager.current.startDeviceScan([PR_SERVICE_UUID], null, (error, dev) => {
        if (error) {
          clearTimeout(timeout);
          reject(error);
          return;
        }
        if (dev && !found.find((d) => d.id === dev.id)) {
          found.push(dev);
        }
      });
    });
  }, []);

  /**
   * Connect to a Pulse-Reaper device by ID.
   * Author: jayis1
   */
  const connectToDevice = useCallback(async (deviceId) => {
    setStatus('Connecting...');
    try {
      const dev = await manager.current.connectToDevice(deviceId);
      await dev.discoverAllServicesAndCharacteristics();
      const chars = await dev.characteristicsForService(PR_SERVICE_UUID);

      cmdChar.current = chars.find((c) => c.uuid === PR_CMD_CHAR_UUID);
      notifyChar.current = chars.find((c) => c.uuid === PR_NOTIFY_CHAR_UUID);

      await notifyChar.current.monitor(handleNotification);

      setDevice(dev);
      setConnected(true);
      setStatus('Connected');

      // Fetch firmware version
      const ver = await sendCommand(Buffer.from([CMD.FIRMWARE_VERSION]));
      setFirmwareVersion(ver || '');

      return true;
    } catch (e) {
      setStatus('Connection failed: ' + e.message);
      return false;
    }
  }, [handleNotification]);

  /**
   * Disconnect from the device.
   * Author: jayis1
   */
  const disconnect = useCallback(async () => {
    if (device) {
      await device.cancelConnection();
    }
    setDevice(null);
    setConnected(false);
    setStatus('Disconnected');
    setCurrentMode(MODE.IDLE);
  }, [device]);

  /**
   * Send a binary command to the device and wait for the status response.
   * Author: jayis1
   *
   * The cmd Buffer is sent as a single BLE write. The nRF52840 handles
   * framing + encryption before forwarding to the MCU.
   */
  const sendCommand = useCallback(async (cmdBuffer, timeoutMs = 3000) => {
    if (!cmdChar.current) {
      return 'ERR not connected';
    }
    return new Promise(async (resolve) => {
      const timer = setTimeout(() => {
        if (pendingResolver.current) {
          pendingResolver.current = null;
          resolve('ERR timeout');
        }
      }, timeoutMs);

      pendingResolver.current = (response) => {
        clearTimeout(timer);
        resolve(response);
      };

      try {
        await cmdChar.current.writeWithResponse(cmdBuffer.toString('base64'));
      } catch (e) {
        clearTimeout(timer);
        if (pendingResolver.current) {
          pendingResolver.current = null;
        }
        resolve('ERR write failed: ' + e.message);
      }
    });
  }, []);

  /**
   * Enter a specific operational mode.
   * Author: jayis1
   */
  const enterMode = useCallback(async (mode) => {
    const buf = Buffer.from([CMD.ENTER_MODE, mode]);
    const resp = await sendCommand(buf);
    if (resp && !resp.startsWith('ERR')) {
      setCurrentMode(mode);
    }
    return resp;
  }, [sendCommand]);

  /**
   * Fire the TDR engine (acquire + classify + stream reflectogram).
   * Author: jayis1
   */
  const fireTDR = useCallback(async () => {
    const buf = Buffer.from([CMD.FIRE_TDR]);
    return sendCommand(buf, 5000);
  }, [sendCommand]);

  /**
   * Set the clamp front-end gain.
   * Author: jayis1
   */
  const setGain = useCallback(async (gain) => {
    const buf = Buffer.from([CMD.SET_GAIN, gain]);
    return sendCommand(buf);
  }, [sendCommand]);

  /**
   * Set the clamp coupling mode.
   * Author: jayis1
   */
  const setCoupling = useCallback(async (coupling) => {
    const buf = Buffer.from([CMD.SET_COUPLING, coupling]);
    return sendCommand(buf);
  }, [sendCommand]);

  /**
   * Inject a frame on an unpowered bus.
   * Author: jayis1
   */
  const injectFrame = useCallback(async (frameData) => {
    const buf = Buffer.concat([Buffer.from([CMD.INJECT_FRAME]), frameData]);
    return sendCommand(buf, 2000);
  }, [sendCommand]);

  /**
   * Stop sniffing and close the capture file.
   * Author: jayis1
   */
  const stopSniff = useCallback(async () => {
    const buf = Buffer.from([CMD.STOP_SNIFF]);
    return sendCommand(buf);
  }, [sendCommand]);

  /**
   * Register a callback for received frames during sniff mode.
   * Author: jayis1
   */
  const setFrameCallback = useCallback((cb) => {
    setFrameCallback({ current: cb });
  }, []);

  /**
   * Register a callback for TDR reflectogram data.
   * Author: jayis1
   */
  const setTdrCallback = useCallback((cb) => {
    setTdrCallback({ current: cb });
  }, []);

  const value = {
    device,
    connected,
    status,
    battery,
    currentMode,
    firmwareVersion,
    MODE,
    CMD,
    scanForDevices,
    connectToDevice,
    disconnect,
    sendCommand,
    enterMode,
    fireTDR,
    setGain,
    setCoupling,
    injectFrame,
    stopSniff,
    setFrameCallback,
    setTdrCallback,
  };

  return <DeviceContext.Provider value={value}>{children}</DeviceContext.Provider>;
}

/**
 * Hook to access the device context.
 * Author: jayis1
 */
export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) {
    throw new Error('useDevice must be used within a DeviceProvider');
  }
  return ctx;
}