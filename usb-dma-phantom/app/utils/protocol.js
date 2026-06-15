/**
 * protocol.js — BLE C2 protocol for USB DMA Phantom
 * Handles encrypted communication between companion app and device
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 *
 * Protocol:
 *   - BLE Service UUID: 6e400001-b5a3-f393-e0a9-e50e24dcca9e (Nordic UART)
 *   - RX Characteristic: 6e400002-b5a3-f393-e0a9-e50e24dcca9e (write)
 *   - TX Characteristic: 6e400003-b5a3-f393-e0a9-e50e24dcca9e (notify)
 *   - Encryption: AES-256-CTR with session key derived from ECDH
 *   - Packet format: [MAGIC(1) | CMD(1) | SEQ(1) | FLAGS(1) | ADDR(8) | LEN(4) | DATA(...)]
 *   - Magic: 0xD4
 */

import React, { createContext, useState, useContext, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import AES from 'aes-js';

// BLE Service and Characteristic UUIDs (Nordic UART Service)
const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const RX_CHAR_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';  // Write
const TX_CHAR_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';  // Notify

// DMA command codes
const DMA_CMD_READ    = 0x01;
const DMA_CMD_WRITE   = 0x02;
const DMA_CMD_SCAN    = 0x03;
const DMA_CMD_INJECT  = 0x04;
const DMA_CMD_LIST    = 0x10;
const DMA_CMD_UPLOAD  = 0x11;
const DMA_CMD_DELETE  = 0x12;
const DMA_CMD_SET_VID = 0x20;
const DMA_CMD_RESET   = 0xFF;

// DMA magic byte
const DMA_MAGIC = 0xD4;

// Response codes
const DMA_RESP_OK  = 0xA0;
const DMA_RESP_ERR = 0xA1;

// Packet flags
const FLAG_ASYNC    = 0x01;
const FLAG_ENCRYPTED = 0x02;

// Maximum BLE MTU payload
const BLE_MTU = 244;  // 247 - 3 header bytes

class DmaProtocol {
  constructor() {
    this.bleManager = new BleManager();
    this.device = null;
    this.rxChar = null;
    this.txChar = null;
    this.sequence = 0;
    this.encryptionKey = null;  // AES-256 key (32 bytes)
    this.encryptionIv = null;    // AES-CTR IV (16 bytes)
    this.rxBuffer = new Uint8Array(0);
    this.pendingRequests = new Map();
    this.notificationCallback = null;
  }

  /**
   * Scan for and connect to USB DMA Phantom device
   */
  async connect() {
    try {
      // Scan for devices advertising our service
      const device = await new Promise((resolve, reject) => {
        const subscription = this.bleManager.scanForDevices(
          [SERVICE_UUID],
          null,
          (error, scannedDevice) => {
            if (error) {
              subscription.remove();
              reject(error);
              return;
            }
            if (scannedDevice.name?.startsWith('DMA-Phantom')) {
              subscription.remove();
              this.bleManager.stopDeviceScan();
              resolve(scannedDevice);
            }
          }
        );

        // Timeout after 30 seconds
        setTimeout(() => {
          this.bleManager.stopDeviceScan();
          subscription.remove();
          reject(new Error('Scan timeout: device not found'));
        }, 30000);
      });

      // Connect to device
      this.device = await device.connect();
      await this.device.discoverAllServicesAndCharacteristics();

      // Get characteristics
      const services = await this.device.services();
      const uartService = services.find(s => s.uuid === SERVICE_UUID);

      if (!uartService) {
        throw new Error('UART service not found');
      }

      const characteristics = await uartService.characteristics();
      this.rxChar = characteristics.find(c => c.uuid === RX_CHAR_UUID);
      this.txChar = characteristics.find(c => c.uuid === TX_CHAR_UUID);

      if (!this.rxChar || !this.txChar) {
        throw new Error('UART characteristics not found');
      }

      // Set up notification listener
      this.txChar.monitor((error, characteristic) => {
        if (error) {
          console.error('BLE notification error:', error);
          return;
        }
        if (characteristic?.value) {
          const data = Buffer.from(characteristic.value, 'base64');
          this._handleIncomingData(data);
        }
      });

      // Perform key exchange
      await this._keyExchange();

      return this.device;
    } catch (error) {
      this.device = null;
      throw error;
    }
  }

  /**
   * Disconnect from device
   */
  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.rxChar = null;
      this.txChar = null;
    }
  }

  /**
   * Perform ECDH key exchange for session encryption
   */
  async _keyExchange() {
    // Generate ephemeral key pair (simplified — production uses actual ECDH)
    const ephemeralKey = new Uint8Array(32);
    for (let i = 0; i < 32; i++) {
      ephemeralKey[i] = Math.floor(Math.random() * 256);
    }

    // Send our public key to device
    const keyExchangePacket = this._buildPacket(0x00, 0, 0, ephemeralKey);
    await this._sendPacket(keyExchangePacket);

    // Wait for device's public key response
    const response = await this._waitForResponse(0x00, 5000);

    // Derive shared secret (simplified — production uses ECDH)
    this.encryptionKey = new Uint8Array(32);
    for (let i = 0; i < 32; i++) {
      this.encryptionKey[i] = ephemeralKey[i] ^ (response.data?.[i] || 0);
    }

    // Initialize IV (all zeros for first session)
    this.encryptionIv = new Uint8Array(16);
  }

  /**
   * Send DMA command
   */
  async sendDmaCommand({ cmd, host_addr, length, data }) {
    const seq = this.sequence++ & 0xFF;
    const flags = this.encryptionKey ? FLAG_ENCRYPTED : 0;

    const addrBuffer = Buffer.alloc(8);
    addrBuffer.writeBigUInt64LE(BigInt(host_addr), 0);

    const lenBuffer = Buffer.alloc(4);
    lenBuffer.writeUInt32LE(length, 0);

    let payload = Buffer.concat([
      addrBuffer,
      lenBuffer,
    ]);

    if (data) {
      if (Buffer.isBuffer(data)) {
        payload = Buffer.concat([payload, data]);
      } else if (typeof data === 'object') {
        payload = Buffer.concat([payload, Buffer.from(JSON.stringify(data))]);
      }
    }

    const packet = this._buildPacket(cmd, seq, flags, payload);
    await this._sendPacket(packet);

    // Wait for response (with timeout)
    return this._waitForResponse(cmd, 10000);
  }

  /**
   * Build a protocol packet
   */
  _buildPacket(cmd, seq, flags, data) {
    const header = Buffer.from([DMA_MAGIC, cmd, seq, flags]);
    if (data && Buffer.isBuffer(data)) {
      return Buffer.concat([header, data]);
    }
    return header;
  }

  /**
   * Send packet over BLE
   */
  async _sendPacket(packet) {
    if (!this.rxChar) {
      throw new Error('Not connected');
    }

    // Encrypt if key is available
    if (this.encryptionKey && (packet[3] & FLAG_ENCRYPTED)) {
      packet = this._encrypt(packet);
    }

    // Chunk to MTU size and send
    const chunks = this._chunk(packet, BLE_MTU);
    for (const chunk of chunks) {
      await this.rxChar.writeWithResponse(
        chunk.toString('base64')
      );
    }
  }

  /**
   * Handle incoming BLE data
   */
  _handleIncomingData(data) {
    // Append to receive buffer
    const newBuf = new Uint8Array(this.rxBuffer.length + data.length);
    newBuf.set(this.rxBuffer);
    newBuf.set(data, this.rxBuffer.length);
    this.rxBuffer = newBuf;

    // Check for complete packet
    if (this.rxBuffer.length >= 4) {
      const magic = this.rxBuffer[0];
      if (magic === DMA_MAGIC || magic === DMA_RESP_OK || magic === DMA_RESP_ERR) {
        // Extract packet
        const cmd = this.rxBuffer[1];
        const seq = this.rxBuffer[2];
        const flags = this.rxBuffer[3];
        const dataLen = this.rxBuffer.length - 4;
        const data = this.rxBuffer.slice(4);

        // Decrypt if needed
        let payload = data;
        if (flags & FLAG_ENCRYPTED && this.encryptionKey) {
          payload = this._decrypt(Buffer.from(payload));
        }

        // Resolve pending request
        const key = `${cmd}-${seq}`;
        if (this.pendingRequests.has(key)) {
          const { resolve } = this.pendingRequests.get(key);
          this.pendingRequests.delete(key);
          resolve({
            success: magic === DMA_RESP_OK,
            cmd,
            seq,
            data: payload,
            message: magic === DMA_RESP_OK ? 'OK' : 'Error',
          });
        }

        // Reset buffer
        this.rxBuffer = new Uint8Array(0);
      }
    }
  }

  /**
   * Wait for response to a command
   */
  _waitForResponse(cmd, timeout = 10000) {
    return new Promise((resolve, reject) => {
      const key = `${cmd}-${this.sequence & 0xFF}`;
      this.pendingRequests.set(key, { resolve, reject });

      setTimeout(() => {
        if (this.pendingRequests.has(key)) {
          this.pendingRequests.delete(key);
          reject(new Error('Response timeout'));
        }
      }, timeout);
    });
  }

  /**
   * AES-256-CTR encryption
   */
  _encrypt(data) {
    if (!this.encryptionKey) return data;
    const aesCtr = new AES.ModeOfOperation.ctr(
      this.encryptionKey,
      new AES.Counter(this.encryptionIv)
    );
    return Buffer.from(aesCtr.encrypt(data));
  }

  /**
   * AES-256-CTR decryption
   */
  _decrypt(data) {
    if (!this.encryptionKey) return data;
    const aesCtr = new AES.ModeOfOperation.ctr(
      this.encryptionKey,
      new AES.Counter(this.encryptionIv)
    );
    return Buffer.from(aesCtr.decrypt(data));
  }

  /**
   * Chunk a buffer into pieces of maxLen
   */
  _chunk(buf, maxLen) {
    const chunks = [];
    for (let i = 0; i < buf.length; i += maxLen) {
      chunks.push(buf.slice(i, Math.min(i + maxLen, buf.length)));
    }
    return chunks;
  }
}

// React Context for BLE state management
const BleContext = createContext(null);

export { BleContext };

export function BleManagerProvider({ children }) {
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const [dmaStatus, setDmaStatus] = useState({ active: false, mode: 'STEALTH' });
  const [deviceInfo, setDeviceInfo] = useState({ mac: null });
  const protocolRef = useRef(new DmaProtocol());

  const connect = useCallback(async () => {
    setConnectionStatus('connecting');
    try {
      const device = await protocolRef.current.connect();
      setConnectionStatus('connected');
      setDeviceInfo({ mac: device.id });
    } catch (err) {
      setConnectionStatus('error');
      console.error('Connection failed:', err);
    }
  }, []);

  const disconnect = useCallback(async () => {
    await protocolRef.current.disconnect();
    setConnectionStatus('disconnected');
    setDmaStatus({ active: false, mode: 'STEALTH' });
  }, []);

  const sendDmaCommand = useCallback(async (params) => {
    try {
      const result = await protocolRef.current.sendDmaCommand(params);
      if (params.cmd === DMA_CMD_READ || params.cmd === DMA_CMD_SCAN) {
        setDmaStatus(prev => ({ ...prev, active: true }));
      }
      return result;
    } catch (err) {
      console.error('DMA command failed:', err);
      throw err;
    }
  }, []);

  const value = {
    device: protocolRef.current.device,
    connectionStatus,
    dmaStatus,
    deviceInfo,
    connect,
    disconnect,
    sendDmaCommand,
  };

  return (
    <BleContext.Provider value={value}>
      {children}
    </BleContext.Provider>
  );
}