/**
 * AperturePhantom — DeviceContext (BLE connection + state store)
 *
 * Holds the active BLE connection, the binary protocol parser, the live
 * device status, and exposes actions used by every screen. All screens
 * consume this via the useDevice() hook.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { Platform, PermissionsAndroid } from 'react-native';
import { BleManager, State as BleState } from 'react-native-ble-plx';
import { FrameParser, CMD, MODE, encodeFrame, fromBase64, toBase64,
  buildSetMode, buildArm, buildSensorRead, buildSensorWrite,
  buildInjectLoad, buildFuzzStart, buildScriptLoad } from '../utils/protocol';

const APERTURE_SERVICE_UUID = '0000a001-0000-1000-8000-00805f9b34fb';
const APERTURE_TX_CHAR_UUID = '0000a002-0000-1000-8000-00805f9b34fb'; /* notify */
const APERTURE_RX_CHAR_UUID = '0000a003-0000-1000-8000-00805f9b34fb'; /* write */

const DeviceCtx = createContext(null);

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState(null);
  const [log, setLog] = useState([]);
  const [liveFrame, setLiveFrame] = useState(null);
  const [linkInfo, setLinkInfo] = useState(null);
  const [scanResults, setScanResults] = useState([]);
  const bleRef = useRef(null);
  const parserRef = useRef(new FrameParser());
  const deviceRef = useRef(null);

  const addLog = useCallback((line) => {
    setLog((l) => [...l.slice(-200), `[${new Date().toLocaleTimeString()}] ${line}`]);
  }, []);

  /* ---- BLE setup ---- */
  const ensureManager = useCallback(() => {
    if (!bleRef.current) bleRef.current = new BleManager();
    return bleRef.current;
  }, []);

  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every((v) => v === 'granted');
    }
    return true;
  };

  const startScan = useCallback(async () => {
    const mgr = ensureManager();
    const ok = await requestPermissions();
    if (!ok) { addLog('permissions denied'); return; }
    setScanResults([]);
    mgr.startDeviceScan(null, { allowDuplicates: false }, (err, dev) => {
      if (err) { addLog('scan err: ' + err.message); return; }
      if (dev && dev.name && dev.name.startsWith('APERTURE')) {
        setScanResults((s) => {
          if (s.find((d) => d.id === dev.id)) return s;
          return [...s, dev];
        });
      }
    });
  }, [ensureManager, addLog]);

  const stopScan = useCallback(() => {
    bleRef.current?.stopDeviceScan();
  }, []);

  /* ---- Connect + authenticate ---- */
  const connect = useCallback(async (deviceId) => {
    const mgr = ensureManager();
    addLog('connecting ' + deviceId);
    const dev = await mgr.connectToDevice(deviceId);
    await dev.discoverAllServicesAndCharacteristics();
    deviceRef.current = dev;

    /* Subscribe to TX notifications (device → app). */
    await dev.monitorCharacteristicForService(
      APERTURE_SERVICE_UUID, APERTURE_TX_CHAR_UUID,
      (err, char) => {
        if (err) { addLog('notify err: ' + err.message); return; }
        if (char && char.value) {
          const bytes = fromBase64(char.value);
          parserRef.current.push(bytes);
          let f;
          while ((f = parserRef.current.next())) {
            handleFrame(f.op, f.payload);
          }
        }
      }
    );
    setConnected(true);
    addLog('connected; sending PING');
    send(CMD.PING, null);
    send(CMD.GET_STATUS, null);
    send(CMD.GET_LINK_INFO, null);
  }, [ensureManager, addLog]);

  const disconnect = useCallback(async () => {
    if (deviceRef.current) {
      await deviceRef.current.cancelConnection();
      deviceRef.current = null;
    }
    setConnected(false);
    setStatus(null);
    setLinkInfo(null);
    addLog('disconnected');
  }, [addLog]);

  /* ---- Send a command ---- */
  const send = useCallback(async (op, payload) => {
    if (!deviceRef.current) return;
    const frame = encodeFrame(op, payload);
    const b64 = toBase64(frame);
    try {
      await deviceRef.current.writeCharacteristicWithoutResponseForService(
        APERTURE_SERVICE_UUID, APERTURE_RX_CHAR_UUID, b64);
    } catch (e) {
      addLog('write err: ' + e.message);
    }
  }, [addLog]);

  /* ---- Frame dispatch ---- */
  const handleFrame = useCallback((op, payload) => {
    switch (op) {
    case CMD.PING:
      addLog('PING ok'); break;
    case CMD.GET_STATUS:
      if (payload && payload.length >= 14) {
        const status = (payload[0] << 24) | (payload[1] << 16) |
                       (payload[2] << 8) | payload[3];
        const fc = (payload[4] << 24) | (payload[5] << 16) |
                   (payload[6] << 8) | payload[7];
        const crc = (payload[8] << 24) | (payload[9] << 16) |
                    (payload[10] << 8) | payload[11];
        const mode = payload[12];
        const armed = payload[13];
        setStatus({ status, frameCount: fc, crcErrors: crc, mode, armed });
      }
      break;
    case CMD.GET_LINK_INFO:
      if (payload && payload.length >= 8) {
        const lanes = payload[0];
        const rate = (payload[1] << 8) | payload[2];
        setLinkInfo({ lanes, rateMbps: rate / 10, dataType: payload[3],
                      sensorAddr: payload[4], battery: payload[5],
                      sdFreeKB: (payload[6] << 8) | payload[7] });
      }
      break;
    case CMD.LOG:
      if (payload) {
        const s = String.fromCharCode(...payload);
        addLog(s);
      }
      break;
    case CMD.FRAME:
      if (payload && payload.length > 5) {
        /* [dt][seq16][len16][data] — we stash the raw data for the view. */
        setLiveFrame({ dt: payload[0], seq: (payload[1] << 8) | payload[2],
                       len: (payload[3] << 8) | payload[4],
                       data: payload.slice(5) });
      }
      break;
    case CMD.ERROR:
      addLog('device ERROR'); break;
    default:
      /* other responses (REPLAY_LIST, SENSOR_READ, etc.) are handled
       * by screen-specific callbacks; we expose send() for them. */
      break;
    }
  }, [addLog]);

  /* ---- Convenience actions ---- */
  const setMode = useCallback((m) => send(CMD.SET_MODE, [m]), [send]);
  const arm = useCallback((on) => send(CMD.ARM, [on ? 1 : 0]), [send]);
  const startCapture = useCallback(() => send(CMD.CAP_START, null), [send]);
  const stopCapture  = useCallback(() => send(CMD.CAP_STOP, null), [send]);
  const snapshot = useCallback(() => send(CMD.CAP_SNAPSHOT, null), [send]);
  const streamFrames = useCallback((on) => send(CMD.CAP_STREAM, [on?1:0]), [send]);
  const injectLoad = useCallback((slot, bytes) =>
    send(CMD.INJECT_LOAD, [slot, ...Array.from(bytes)]), [send]);
  const injectNow = useCallback((slot) => send(CMD.INJECT_NOW, [slot]), [send]);
  const injectStop = useCallback(() => send(CMD.INJECT_STOP, null), [send]);
  const replayList = useCallback(() => send(CMD.REPLAY_LIST, null), [send]);
  const replayStart = useCallback((idx) => send(CMD.REPLAY_START, [idx]), [send]);
  const replayStop = useCallback(() => send(CMD.REPLAY_STOP, null), [send]);
  const scriptLoad = useCallback((bc) => send(CMD.SCRIPT_LOAD, bc), [send]);
  const scriptRun = useCallback(() => send(CMD.SCRIPT_RUN, null), [send]);
  const scriptStop = useCallback(() => send(CMD.SCRIPT_STOP, null), [send]);
  const sensorScan = useCallback(() => send(CMD.SENSOR_SCAN, null), [send]);
  const sensorRead = useCallback((a, r, l) =>
    send(CMD.SENSOR_READ, [a, (r >> 8) & 0xFF, r & 0xFF, l]), [send]);
  const sensorWrite = useCallback((a, r, v) =>
    send(CMD.SENSOR_WRITE, [a, (r >> 8) & 0xFF, r & 0xFF,
                            (v >> 8) & 0xFF, v & 0xFF]), [send]);
  const sensorAutoconfig = useCallback(() =>
    send(CMD.SENSOR_AUTOCONFIG, null), [send]);
  const fuzzStart = useCallback((strat, count, delay) =>
    send(CMD.FUZZ_START, [strat, (count >> 8) & 0xFF, count & 0xFF,
                          (delay >> 8) & 0xFF, delay & 0xFF]), [send]);
  const fuzzStop = useCallback(() => send(CMD.FUZZ_STOP, null), [send]);
  const triggerSet = useCallback((thumb) =>
    send(CMD.TRIGGER_SET, Array.from(thumb)), [send]);
  const triggerEnable = useCallback(() => send(CMD.TRIGGER_ENABLE, null), [send]);
  const triggerDisable = useCallback(() => send(CMD.TRIGGER_DISABLE, null), [send]);

  const value = {
    connected, status, linkInfo, log, liveFrame, scanResults,
    startScan, stopScan, connect, disconnect,
    setMode, arm, startCapture, stopCapture, snapshot, streamFrames,
    injectLoad, injectNow, injectStop,
    replayList, replayStart, replayStop,
    scriptLoad, scriptRun, scriptStop,
    sensorScan, sensorRead, sensorWrite, sensorAutoconfig,
    fuzzStart, fuzzStop, triggerSet, triggerEnable, triggerDisable,
    send,
  };

  return <DeviceCtx.Provider value={value}>{children}</DeviceCtx.Provider>;
}

export function useDevice() {
  const ctx = useContext(DeviceCtx);
  if (!ctx) throw new Error('useDevice must be used within DeviceProvider');
  return ctx;
}