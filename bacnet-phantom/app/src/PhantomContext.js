/**
 * PhantomContext.js — shared BLE connection and command channel.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * Centralises the BLE peripheral scan / connect / pair lifecycle and exposes
 * a single sendCommand() to the four screens. The wire protocol is one
 * newline-terminated JSON object per command:
 *
 *   {"op":"whois","low":0,"high":4194303}\n
 *   {"op":"writeprop","a1":2,"a2":1,"a3":85,"f":22.5}\n
 *
 * Replies are newline-terminated JSON objects written by the firmware to the
 * Nordic-UART-Service TX characteristic and parsed here into an event queue.
 */
import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import base64 from 'base64-js';

const NUS_SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX_CHAR  = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';  /* write */
const NUS_TX_CHAR  = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';  /* notify */

const PhantomContext = createContext(null);
export const usePhantom = () => useContext(PhantomContext);

export function PhantomProvider({ children }) {
  const manager = useRef(new BleManager()).current;
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState('idle');
  const [devices, setDevices] = useState([]);
  const [events, setEvents] = useState([]);
  const txChar = useRef(null);
  const lineBuf = useRef('');

  const log = useCallback((msg) => {
    setEvents((prev) => [{ t: Date.now(), msg }, ...prev].slice(0, 200));
  }, []);

  const scan = useCallback(async () => {
    setStatus('scanning');
    setDevices([]);
    manager.startDeviceScan(null, { allowDuplicates: false }, (err, dev) => {
      if (err) { setStatus(`scan err: ${err.message}`); return; }
      if (dev.name && dev.name.startsWith('BACNET-PHANTOM-')) {
        setDevices((prev) =>
          prev.find((d) => d.id === dev.id) ? prev : [...prev, dev]);
      }
    });
    setTimeout(() => { manager.stopDeviceScan(); setStatus('scan done'); }, 5000);
  }, [manager]);

  const connect = useCallback(async (dev) => {
    try {
      setStatus(`connecting ${dev.name}`);
      const d = await dev.connect({ autoConnect: false, requestMTU: 247 });
      await d.discoverAllServicesAndCharacteristics();
      const chars = await d.characteristicsForService(NUS_SERVICE);
      txChar.current = chars.find((c) => c.uuid === NUS_TX_CHAR);
      await txChar.current.monitor((err, char) => {
        if (err) { log(`monitor err: ${err.message}`); return; }
        if (char.value) {
          lineBuf.current += base64.toString(char.value);
          let nl;
          while ((nl = lineBuf.current.indexOf('\n')) >= 0) {
            const line = lineBuf.current.slice(0, nl);
            lineBuf.current = lineBuf.current.slice(nl + 1);
            try { log(JSON.parse(line)); } catch { log(line); }
          }
        }
      });
      setDevice(d);
      setConnected(true);
      setStatus(`connected ${d.name}`);
      log(`paired ${d.name} — author=jayis1`);
    } catch (e) {
      setStatus(`connect err: ${e.message}`);
    }
  }, [log]);

  const disconnect = useCallback(async () => {
    if (device) { try { await device.cancelConnection(); } catch {} }
    setDevice(null); setConnected(false); setStatus('disconnected');
  }, [device]);

  /* JSON-over-BLE command. Returns true on successful write. */
  const sendCommand = useCallback(async (obj) => {
    if (!device || !connected) { log('not connected'); return false; }
    const line = JSON.stringify(obj) + '\n';
    try {
      await device.writeCharacteristicWithResponseForService(
        NUS_SERVICE, NUS_RX_CHAR, base64.fromUint8Array(
          new Uint8Array(line.split('').map((c) => c.charCodeAt(0)))));
      return true;
    } catch (e) {
      log(`write err: ${e.message}`);
      return false;
    }
  }, [device, connected, log]);

  return (
    <PhantomContext.Provider value={{
      manager, device, connected, status, devices, events,
      scan, connect, disconnect, sendCommand, log,
    }}>
      {children}
    </PhantomContext.Provider>
  );
}