/**
 * DeviceContext.js — React Context for Device State Management
 *
 * Provides device connection state, protocol handler, device info,
 * battery monitoring, and connection lifecycle management to all
 * screens in the companion app. Acts as the central state hub for
 * all USB communication with the eMMC Flash Dumper hardware.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

import React, { createContext, useState, useEffect, useRef, useCallback } from 'react';
import { AppState, Alert } from 'react-native';
import { FlashDumperProtocol } from '../utils/protocol';

/*===========================================================================
 * Default Context Value
 *===========================================================================*/

const defaultContext = {
  connected: false,
  deviceInfo: null,
  batteryMv: 0,
  protocol: null,
  isScanning: false,
  lastError: null,
  connect: () => {},
  disconnect: () => {},
  refreshInfo: () => {},
};

export const DeviceContext = createContext(defaultContext);

/*===========================================================================
 * DeviceProvider Component
 *
 * Wraps the app and manages all device state. Handles USB device
 * discovery, connection lifecycle, battery polling, and error
 * propagation.
 *===========================================================================*/

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [batteryMv, setBatteryMv] = useState(0);
  const [isScanning, setIsScanning] = useState(false);
  const [lastError, setLastError] = useState(null);
  const protocolRef = useRef(null);
  const batteryTimerRef = useRef(null);
  const discoveryTimerRef = useRef(null);

  /*-----------------------------------------------------------------------
   * Initialize protocol handler on mount
   *-----------------------------------------------------------------------*/
  useEffect(() => {
    protocolRef.current = new FlashDumperProtocol();

    /* Device attached handler */
    const onAttached = async (info) => {
      setConnected(true);
      setDeviceInfo(info);
      setBatteryMv(info.batteryMv || 0);
      setLastError(null);
      setIsScanning(false);
    };

    /* Device detached handler */
    const onDetached = () => {
      setConnected(false);
      setDeviceInfo(null);
      setBatteryMv(0);
      setIsScanning(false);
      stopBatteryPolling();
    };

    /* Battery update handler */
    const onBattery = (mv) => {
      setBatteryMv(mv);
    };

    /* Device error handler */
    const onDeviceError = (err) => {
      setLastError(err);
      console.warn('Device error:', err.code, err.message);
    };

    protocolRef.current.on('attached', onAttached);
    protocolRef.current.on('detached', onDetached);
    protocolRef.current.on('battery', onBattery);
    protocolRef.current.on('deviceError', onDeviceError);

    return () => {
      protocolRef.current?.removeAllListeners();
      protocolRef.current?.disconnect();
      stopBatteryPolling();
      stopDiscovery();
    };
  }, []);

  /*-----------------------------------------------------------------------
   * App state handling (pause discovery when backgrounded)
   *-----------------------------------------------------------------------*/
  useEffect(() => {
    const handleAppState = (nextState) => {
      if (nextState === 'active' && !connected) {
        startDiscovery();
      } else if (nextState === 'background') {
        stopDiscovery();
      }
    };
    const subscription = AppState.addEventListener('change', handleAppState);
    return () => subscription.remove();
  }, [connected]);

  /*-----------------------------------------------------------------------
   * Battery polling (every 5 seconds when connected)
   *-----------------------------------------------------------------------*/
  const startBatteryPolling = useCallback(() => {
    stopBatteryPolling();
    batteryTimerRef.current = setInterval(async () => {
      if (!protocolRef.current || !connected) return;
      try {
        const info = await protocolRef.current.getDeviceInfo();
        if (info) {
          setBatteryMv(info.batteryMv);
          setDeviceInfo(prev => ({ ...prev, ...info }));
        }
      } catch (e) {
        /* Silent — battery polling is best-effort */
      }
    }, 5000);
  }, [connected]);

  const stopBatteryPolling = useCallback(() => {
    if (batteryTimerRef.current) {
      clearInterval(batteryTimerRef.current);
      batteryTimerRef.current = null;
    }
  }, []);

  /*-----------------------------------------------------------------------
   * Device discovery
   *-----------------------------------------------------------------------*/
  const startDiscovery = useCallback(() => {
    if (connected) return;
    setIsScanning(true);
    protocolRef.current?.startDiscovery();
  }, [connected]);

  const stopDiscovery = useCallback(() => {
    setIsScanning(false);
    protocolRef.current?.stopDiscovery();
  }, []);

  /* Start discovery on mount if not connected */
  useEffect(() => {
    if (!connected) {
      startDiscovery();
    }
    return () => stopDiscovery();
  }, [connected, startDiscovery, stopDiscovery]);

  /*-----------------------------------------------------------------------
   * Public API: connect, disconnect, refreshInfo
   *-----------------------------------------------------------------------*/
  const connect = useCallback(async () => {
    if (connected) return;
    setIsScanning(true);
    protocolRef.current?.startDiscovery();
    /* Discovery is async — connection handled by 'attached' event */
  }, [connected]);

  const disconnect = useCallback(async () => {
    if (!connected) return;
    try {
      await protocolRef.current?.disconnect();
    } catch (e) {
      console.warn('Disconnect error:', e);
    }
    setConnected(false);
    setDeviceInfo(null);
    setBatteryMv(0);
    stopBatteryPolling();
  }, [connected, stopBatteryPolling]);

  const refreshInfo = useCallback(async () => {
    if (!connected || !protocolRef.current) return;
    try {
      const info = await protocolRef.current.getDeviceInfo();
      if (info) {
        setDeviceInfo(prev => ({ ...prev, ...info }));
        setBatteryMv(info.batteryMv);
      }
    } catch (e) {
      setLastError({ code: 0, message: e.message });
    }
  }, [connected]);

  /*-----------------------------------------------------------------------
   * Start battery polling when connection established
   *-----------------------------------------------------------------------*/
  useEffect(() => {
    if (connected) {
      startBatteryPolling();
    }
    return () => stopBatteryPolling();
  }, [connected, startBatteryPolling, stopBatteryPolling]);

  /*-----------------------------------------------------------------------
   * Context value
   *-----------------------------------------------------------------------*/
  const contextValue = {
    connected,
    deviceInfo,
    batteryMv,
    protocol: protocolRef.current,
    isScanning,
    lastError,
    connect,
    disconnect,
    refreshInfo,
  };

  return (
    <DeviceContext.Provider value={contextValue}>
      {children}
    </DeviceContext.Provider>
  );
}

export default DeviceContext;
