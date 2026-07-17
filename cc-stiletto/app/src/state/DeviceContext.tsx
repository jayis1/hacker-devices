// src/state/DeviceContext.tsx — React context that owns the USB-CDC connection.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React, { createContext, useContext, useEffect, useRef, useState, useCallback } from 'react';
import { Platform, NativeModules, NativeEventEmitter } from 'react-native';
import { Cmd, PdMsg, Telemetry, encodeCmd, parseLine } from '../protocol';

// On Android we use expo-usb-serial (native module `UsbSerial`). On other
// platforms the transport is stubbed so the UI is still navigable for layout
// review. The real device communication is over USB-CDC at 1 Mbaud.
const UsbSerial = NativeModules.UsbSerial ?? null;

interface DeviceState {
  connected: boolean;
  connecting: boolean;
  error: string | null;
  messages: PdMsg[];
  telemetry: Telemetry | null;
  activeMode: string;
  connect: () => Promise<void>;
  disconnect: () => void;
  send: (c: Cmd) => void;
  clearMessages: () => void;
  lastLog: string | null;
}

const Ctx = createContext<DeviceState | null>(null);

export function useDevice(): DeviceState {
  const v = useContext(Ctx);
  if (!v) throw new Error('useDevice must be inside <DeviceProvider>');
  return v;
}

const MAX_MESSAGES = 500;

export function DeviceProvider({ children }: { children: React.ReactNode }) {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [messages, setMessages] = useState<PdMsg[]>([]);
  const [telemetry, setTelemetry] = useState<Telemetry | null>(null);
  const [activeMode, setActiveMode] = useState('sniff');
  const [lastLog, setLastLog] = useState<string | null>(null);

  const lineBuf = useRef('');
  const deviceRef = useRef<any>(null);

  const handleLine = useCallback((line: string) => {
    setLastLog(line);
    // Track mode changes the device echoes back
    if (line.includes('evt mode ')) {
      const m = line.match(/evt mode (\S+)/);
      if (m) setActiveMode(m[1]);
      return;
    }
    const parsed = parseLine(line);
    if (!parsed) return;
    if ('timestamp' in parsed) {
      setMessages((prev) => {
        const next = [...prev, parsed as PdMsg];
        return next.length > MAX_MESSAGES ? next.slice(-MAX_MESSAGES) : next;
      });
    } else {
      setTelemetry(parsed as Telemetry);
    }
  }, []);

  const onRawData = useCallback((data: string) => {
    lineBuf.current += data;
    let i;
    while ((i = lineBuf.current.indexOf('\n')) >= 0) {
      const line = lineBuf.current.slice(0, i).trim();
      lineBuf.current = lineBuf.current.slice(i + 1);
      if (line) handleLine(line);
    }
  }, [handleLine]);

  useEffect(() => {
    if (!UsbSerial) return; // nothing to subscribe to on this platform
    const emitter = new NativeEventEmitter(UsbSerial);
    const sub = emitter.addListener('UsbSerialData', (e: { data: string }) => {
      onRawData(e.data);
    });
    return () => sub.remove();
  }, [onRawData]);

  const connect = useCallback(async () => {
    setConnecting(true);
    setError(null);
    try {
      if (!UsbSerial) {
        // Demo / no-hardware mode: pretend we're connected so UI is usable.
        setConnected(true);
        setConnecting(false);
        setActiveMode('sniff');
        return;
      }
      // Vendor/product IDs of the STM32 CDC ACM: 0x0483:0x5710
      const dev = await UsbSerial.open(0x0483, 0x5710, 115200);
      deviceRef.current = dev;
      setConnected(true);
      setActiveMode('sniff');
    } catch (e: any) {
      setError(String(e?.message ?? e));
    } finally {
      setConnecting(false);
    }
  }, []);

  const disconnect = useCallback(() => {
    if (deviceRef.current && UsbSerial) UsbSerial.close(deviceRef.current);
    deviceRef.current = null;
    setConnected(false);
  }, []);

  const send = useCallback((c: Cmd) => {
    const line = encodeCmd(c);
    if (deviceRef.current && UsbSerial) {
      UsbSerial.write(deviceRef.current, line);
    }
    // Echo locally so demo mode shows something
    setLastLog(`>> ${line.trim()}`);
  }, []);

  const clearMessages = useCallback(() => setMessages([]), []);

  const value: DeviceState = {
    connected, connecting, error, messages, telemetry, activeMode,
    connect, disconnect, send, clearMessages, lastLog,
  };

  return <Ctx.Provider value={value}>{children}</Ctx.Provider>;
}