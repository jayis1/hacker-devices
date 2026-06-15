/**
 * ConnectionContext — React context for USB serial communication
 *
 * Provides connection state, command sending, and packet stream
 * to all screens in the app. Uses react-native-usb-serial for
 * communication with the BLE Phantom hardware.
 */

import React, { createContext, useState, useEffect, useRef } from 'react';
import { NativeEventEmitter, NativeModules } from 'react-native';
import { BehaviorSubject } from 'rxjs';

// Command protocol constants (matching firmware)
const CMD_HOST_SET_MODE      = 0xA0;
const CMD_HOST_SET_CONFIG     = 0xA1;
const CMD_HOST_START          = 0xA2;
const CMD_HOST_STOP           = 0xA3;
const CMD_HOST_REPLAY         = 0xA4;
const CMD_HOST_STATUS         = 0xA5;

export const ConnectionContext = createContext({
  connected: false,
  deviceInfo: null,
  sendCommand: async () => null,
  packetStream: new BehaviorSubject([]),
});

export function ConnectionProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState({
    serial: '00000001',
    firmware: '1.0.0',
    radioA: 'nRF52832',
    radioB: 'nRF52832',
  });
  const packetStreamRef = useRef(new BehaviorSubject([]));
  const usbRef = useRef(null);
  const bufferRef = useRef([]);

  // Initialize USB serial connection
  useEffect(() => {
    const UsbSerial = NativeModules.UsbSerial;
    if (!UsbSerial) return;

    const connectDevice = async () => {
      try {
        const devices = await UsbSerial.listDevices();
        const phantom = devices.find(d =>
          d.vendorId === 0x1783 && d.productId === 0xF040
        );

        if (phantom) {
          await UsbSerial.connect(phantom.deviceId, {
            baudRate: 115200,
            dataBits: 8,
            stopBits: 1,
            parity: 0,
          });
          setConnected(true);
          usbRef.current = UsbSerial;
        }
      } catch (err) {
        console.log('USB connect error:', err);
        setConnected(false);
      }
    };

    connectDevice();

    // Listen for USB attach/detach events
    const emitter = new NativeEventEmitter(UsbSerial);
    const attachSub = emitter.addListener('UsbAttach', () => connectDevice());
    const detachSub = emitter.addListener('UsbDetach', () => {
      setConnected(false);
      usbRef.current = null;
    });

    return () => {
      attachSub.remove();
      detachSub.remove();
      if (usbRef.current) {
        usbRef.current.disconnect().catch(() => {});
      }
    };
  }, []);

  // Read loop for incoming data
  useEffect(() => {
    if (!connected || !usbRef.current) return;

    const readInterval = setInterval(async () => {
      try {
        const data = await usbRef.current.read();
        if (data && data.length > 0) {
          // Process the received data
          processIncomingData(data);
        }
      } catch (err) {
        // Read error — device may have disconnected
        console.log('USB read error:', err);
      }
    }, 50); // Poll every 50ms

    return () => clearInterval(readInterval);
  }, [connected]);

  // Process incoming USB data into packets
  const processIncomingData = (data) => {
    const newBuffer = [...bufferRef.current, ...data];
    bufferRef.current = newBuffer;

    // Try to extract complete packets from buffer
    while (bufferRef.current.length >= 4) {
      const radio = bufferRef.current[0];
      if (radio !== 0x01 && radio !== 0x02) {
        // Invalid marker — skip byte
        bufferRef.current.shift();
        continue;
      }

      const cmd = bufferRef.current[1];
      const len = (bufferRef.current[2] << 8) | bufferRef.current[3];
      const totalLen = 4 + len;

      if (bufferRef.current.length < totalLen) break; // Incomplete packet

      // Extract complete packet
      const packet = bufferRef.current.slice(0, totalLen);
      bufferRef.current = bufferRef.current.slice(totalLen);

      // Emit to stream
      const current = packetStreamRef.current.value;
      packetStreamRef.current.next([...current.slice(-100), packet]);
    }
  };

  // Send command to device
  const sendCommand = async (data) => {
    if (!connected || !usbRef.current) {
      throw new Error('Device not connected');
    }

    try {
      await usbRef.current.write(Uint8Array.from(data));
      return data;
    } catch (err) {
      console.log('USB write error:', err);
      throw err;
    }
  };

  return (
    <ConnectionContext.Provider value={{
      connected,
      deviceInfo,
      sendCommand,
      packetStream: packetStreamRef.current,
    }}>
      {children}
    </ConnectionContext.Provider>
  );
}