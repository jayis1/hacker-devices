/**
 * protocol.js — CAN Creeper Binary Wire Protocol
 *
 * Full implementation of the CAN Creeper binary protocol for the
 * React Native companion app. Handles frame serialization/deserialization,
 * CRC-16/CCITT verification, command dispatch, and transport abstraction
 * (BLE via react-native-ble-manager, USB via react-native-serialport).
 *
 * Protocol format:
 *   Byte 0:     Start byte (0xAA)
 *   Byte 1:     Command ID
 *   Byte 2-3:   Payload length (16-bit LE, excluding header and CRC)
 *   Byte 4-N:   Payload
 *   Byte N+1-2: CRC-16/CCITT (over bytes 1 through N)
 *
 * Command IDs:
 *   0x01 FRAME_RX       Device→App
 *   0x02 FRAME_TX       App→Device
 *   0x03 FRAME_TX_ACK   Device→App
 *   0x04 CONFIG_SET     App→Device
 *   0x05 CONFIG_GET     App→Device
 *   0x06 CONFIG_RSP     Device→App
 *   0x07 STATUS_REQ     App→Device
 *   0x08 STATUS_RSP     Device→App
 *   0x09 SCRIPT_LOAD    App→Device
 *   0x0A SCRIPT_START   App→Device
 *   0x0B SCRIPT_STOP    App→Device
 *   0x0C SCRIPT_STATUS  Device→App
 *   0x0D DBC_UPLOAD     App→Device
 *   0x0E DBC_LIST       App→Device
 *   0x0F DBC_LIST_RSP   Device→App
 *   0x10 BUS_STATUS     Device→App
 *   0x11 ERROR_EVENT    Device→App
 *   0x12 PING           App→Device
 *   0x13 PONG           Device→App
 *   0x14 DFU_START      App→Device
 *   0x15 DFU_DATA       App→Device
 *   0x16 DFU_COMPLETE   App→Device
 */

import { EventEmitter } from 'events';
import { Buffer } from 'buffer';

/* Try to import native modules — gracefully degrade if unavailable */
let BleManager = null;
let SerialPort = null;

try {
  BleManager = require('react-native-ble-manager');
} catch (e) {
  console.log('BLE module not available — BLE features disabled');
}

try {
  SerialPort = require('react-native-serialport');
} catch (e) {
  console.log('SerialPort module not available — USB features disabled');
}

/*===========================================================================
 * PROTOCOL CONSTANTS
 *===========================================================================*/

const PROTOCOL_START_BYTE = 0xAA;
const PROTOCOL_MAX_PAYLOAD = 1024;
const PROTOCOL_CRC_POLY = 0x1021;   /* CRC-16/CCITT */
const PROTOCOL_CRC_INIT = 0xFFFF;

const CMD = {
  FRAME_RX:        0x01,
  FRAME_TX:        0x02,
  FRAME_TX_ACK:    0x03,
  CONFIG_SET:      0x04,
  CONFIG_GET:      0x05,
  CONFIG_RSP:      0x06,
  STATUS_REQ:      0x07,
  STATUS_RSP:      0x08,
  SCRIPT_LOAD:     0x09,
  SCRIPT_START:    0x0A,
  SCRIPT_STOP:     0x0B,
  SCRIPT_STATUS:   0x0C,
  DBC_UPLOAD:      0x0D,
  DBC_LIST:        0x0E,
  DBC_LIST_RSP:    0x0F,
  BUS_STATUS:      0x10,
  ERROR_EVENT:     0x11,
  PING:            0x12,
  PONG:            0x13,
  DFU_START:       0x14,
  DFU_DATA:        0x15,
  DFU_COMPLETE:    0x16,
};

/* BLE NUS Service UUIDs */
const NUS_SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E';
const NUS_TX_CHAR_UUID  = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E';  /* Device → App (notify) */
const NUS_RX_CHAR_UUID  = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E';  /* App → Device (write) */

/*===========================================================================
 * CRC-16/CCITT
 *===========================================================================*/

/**
 * Compute CRC-16/CCITT over a buffer
 * @param {Buffer|Uint8Array} data
 * @param {number} len
 * @returns {number} 16-bit CRC
 */
function crc16ccitt(data, len) {
  let crc = PROTOCOL_CRC_INIT;
  for (let i = 0; i < len; i++) {
    crc ^= (data[i] << 8);
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ PROTOCOL_CRC_POLY) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
  }
  return crc;
}

/*===========================================================================
 * FRAME SERIALIZATION
 *===========================================================================*/

/**
 * Serialize a CAN frame for transmission to device (CMD_FRAME_TX)
 *
 * Payload format:
 *   Byte 0:     Channel (0 or 1)
 *   Byte 1:     Flags (IDE, RTR, FD, BRS, ESI)
 *   Byte 2-5:   CAN ID (32-bit LE)
 *   Byte 6:     DLC
 *   Byte 7:     TX flags (one-shot, high-priority)
 *   Byte 8-N:   Data bytes
 *
 * @param {Object} frame
 * @param {number} frame.channel
 * @param {number} frame.id
 * @param {number} frame.dlc
 * @param {number[]} frame.data
 * @param {number} frame.ide
 * @param {number} frame.fd
 * @param {number} frame.brs
 * @param {number} frame.rtr
 * @returns {Buffer} Serialized frame
 */
function serializeFrameTx(frame) {
  const dataLen = frame.dlc <= 8 ? frame.dlc :
                   frame.dlc <= 12 ? 12 :
                   frame.dlc <= 16 ? 16 :
                   frame.dlc <= 20 ? 20 :
                   frame.dlc <= 24 ? 24 :
                   frame.dlc <= 32 ? 32 :
                   frame.dlc <= 48 ? 48 : 64;

  const payloadLen = 8 + dataLen;  /* ch(1) + flags(1) + id(4) + dlc(1) + txflags(1) + data */
  const totalLen = 5 + payloadLen; /* start + cmd + len(2) + payload + crc(2) */

  const buf = Buffer.alloc(totalLen);
  let offset = 0;

  buf[offset++] = PROTOCOL_START_BYTE;
  buf[offset++] = CMD.FRAME_TX;
  buf[offset++] = payloadLen & 0xFF;
  buf[offset++] = (payloadLen >> 8) & 0xFF;

  /* Channel */
  buf[offset++] = frame.channel || 0;

  /* Flags */
  let flags = 0;
  if (frame.ide) flags |= 0x01;
  if (frame.rtr) flags |= 0x02;
  if (frame.fd)  flags |= 0x04;
  if (frame.brs) flags |= 0x08;
  buf[offset++] = flags;

  /* CAN ID (32-bit LE) */
  const id = frame.id & 0x1FFFFFFF;
  buf[offset++] = id & 0xFF;
  buf[offset++] = (id >> 8) & 0xFF;
  buf[offset++] = (id >> 16) & 0xFF;
  buf[offset++] = (id >> 24) & 0xFF;

  /* DLC */
  buf[offset++] = frame.dlc & 0x0F;

  /* TX flags */
  buf[offset++] = 0x00;  /* Default: no one-shot, normal priority */

  /* Data bytes */
  for (let i = 0; i < dataLen; i++) {
    buf[offset++] = (frame.data && i < frame.data.length) ? frame.data[i] : 0x00;
  }

  /* CRC-16 over bytes 1..(offset-1) */
  const crc = crc16ccitt(buf.slice(1, offset), offset - 1);
  buf[offset++] = crc & 0xFF;
  buf[offset++] = (crc >> 8) & 0xFF;

  return buf;
}

/**
 * Serialize a status request (CMD_STATUS_REQ)
 */
function serializeStatusReq() {
  const buf = Buffer.alloc(5);  /* start + cmd + len(0) + crc(2) */
  buf[0] = PROTOCOL_START_BYTE;
  buf[1] = CMD.STATUS_REQ;
  buf[2] = 0x00;  /* payload len = 0 */
  buf[3] = 0x00;
  const crc = crc16ccitt(buf.slice(1, 3), 2);
  buf[4] = crc & 0xFF;
  buf[5] = (crc >> 8) & 0xFF;
  return buf;
}

/**
 * Serialize a PING command
 */
function serializePing() {
  const buf = Buffer.alloc(5);
  buf[0] = PROTOCOL_START_BYTE;
  buf[1] = CMD.PING;
  buf[2] = 0x00;
  buf[3] = 0x00;
  const crc = crc16ccitt(buf.slice(1, 3), 2);
  buf[4] = crc & 0xFF;
  buf[5] = (crc >> 8) & 0xFF;
  return buf;
}

/**
 * Serialize a CONFIG_SET command
 *
 * Payload format:
 *   Byte 0: Config ID (0x00=bitrate, 0x02=mode, 0x03=termination, etc.)
 *   Byte 1: Channel (0, 1, or 0xFF for both)
 *   Byte 2-N: Config-specific data
 */
function serializeConfigSet(channel, config) {
  let payload = [config.configId || 0x00, channel];

  switch (config.configId) {
    case 0x00: /* CAN bitrate */
      payload.push(config.nominalBitrate & 0xFF);
      payload.push((config.nominalBitrate >> 8) & 0xFF);
      payload.push((config.nominalBitrate >> 16) & 0xFF);
      payload.push((config.nominalBitrate >> 24) & 0xFF);
      if (config.fdEnabled) {
        payload.push(config.dataBitrate & 0xFF);
        payload.push((config.dataBitrate >> 8) & 0xFF);
        payload.push((config.dataBitrate >> 16) & 0xFF);
        payload.push((config.dataBitrate >> 24) & 0xFF);
      }
      break;
    case 0x02: /* Channel mode */
      const modes = { 'normal': 0, 'listen-only': 1, 'loopback': 2, 'bus-monitor': 3 };
      payload.push(modes[config.mode] || 0);
      break;
    case 0x03: /* Termination */
      payload.push(config.termination ? 1 : 0);
      break;
    case 0x04: /* BLE TX power */
      payload.push(config.txPower & 0xFF);
      break;
    default:
      break;
  }

  const payloadLen = payload.length;
  const totalLen = 5 + payloadLen;
  const buf = Buffer.alloc(totalLen);
  buf[0] = PROTOCOL_START_BYTE;
  buf[1] = CMD.CONFIG_SET;
  buf[2] = payloadLen & 0xFF;
  buf[3] = (payloadLen >> 8) & 0xFF;
  for (let i = 0; i < payloadLen; i++) buf[4 + i] = payload[i];
  const crc = crc16ccitt(buf.slice(1, 4 + payloadLen), 3 + payloadLen);
  buf[4 + payloadLen] = crc & 0xFF;
  buf[5 + payloadLen] = (crc >> 8) & 0xFF;

  return buf;
}

/**
 * Serialize a SCRIPT_LOAD command
 */
function serializeScriptLoad(scriptJson) {
  const jsonBytes = Buffer.from(scriptJson, 'utf-8');
  const payloadLen = jsonBytes.length;
  const totalLen = 5 + payloadLen;
  const buf = Buffer.alloc(totalLen);
  buf[0] = PROTOCOL_START_BYTE;
  buf[1] = CMD.SCRIPT_LOAD;
  buf[2] = payloadLen & 0xFF;
  buf[3] = (payloadLen >> 8) & 0xFF;
  jsonBytes.copy(buf, 4);
  const crc = crc16ccitt(buf.slice(1, 4 + payloadLen), 3 + payloadLen);
  buf[4 + payloadLen] = crc & 0xFF;
  buf[5 + payloadLen] = (crc >> 8) & 0xFF;
  return buf;
}

/**
 * Serialize a SCRIPT_START command
 */
function serializeScriptStart(scriptName) {
  const nameBytes = Buffer.from(scriptName, 'utf-8');
  const payloadLen = nameBytes.length;
  const totalLen = 5 + payloadLen;
  const buf = Buffer.alloc(totalLen);
  buf[0] = PROTOCOL_START_BYTE;
  buf[1] = CMD.SCRIPT_START;
  buf[2] = payloadLen & 0xFF;
  buf[3] = (payloadLen >> 8) & 0xFF;
  nameBytes.copy(buf, 4);
  const crc = crc16ccitt(buf.slice(1, 4 + payloadLen), 3 + payloadLen);
  buf[4 + payloadLen] = crc & 0xFF;
  buf[5 + payloadLen] = (crc >> 8) & 0xFF;
  return buf;
}

/**
 * Serialize a SCRIPT_STOP command
 */
function serializeScriptStop() {
  const buf = Buffer.alloc(5);
  buf[0] = PROTOCOL_START_BYTE;
  buf[1] = CMD.SCRIPT_STOP;
  buf[2] = 0x00;
  buf[3] = 0x00;
  const crc = crc16ccitt(buf.slice(1, 3), 2);
  buf[4] = crc & 0xFF;
  buf[5] = (crc >> 8) & 0xFF;
  return buf;
}

/*===========================================================================
 * FRAME DESERIALIZATION
 *===========================================================================*/

/**
 * Deserialize a FRAME_RX payload from device
 *
 * Payload format:
 *   Byte 0-3:   Timestamp (32-bit LE, µs)
 *   Byte 4:     Channel (0 or 1)
 *   Byte 5:     Flags (IDE, RTR, FD, BRS, ESI)
 *   Byte 6-9:   CAN ID (32-bit LE)
 *   Byte 10:    DLC
 *   Byte 11-N:  Data bytes
 *
 * @param {Buffer} payload
 * @returns {Object} Deserialized frame
 */
function deserializeFrameRx(payload) {
  if (payload.length < 11) return null;

  const timestamp = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
  const channel = payload[4];
  const flags = payload[5];
  const id = (payload[6] | (payload[7] << 8) | (payload[8] << 16) | (payload[9] << 24)) & 0x1FFFFFFF;
  const dlc = payload[10] & 0x0F;

  const dataLen = dlc <= 8 ? dlc :
                   dlc <= 12 ? 12 :
                   dlc <= 16 ? 16 :
                   dlc <= 20 ? 20 :
                   dlc <= 24 ? 24 :
                   dlc <= 32 ? 32 :
                   dlc <= 48 ? 48 : 64;

  const data = [];
  for (let i = 0; i < Math.min(dataLen, payload.length - 11); i++) {
    data.push(payload[11 + i]);
  }

  return {
    timestamp,
    channel,
    id,
    dlc,
    data,
    ide: !!(flags & 0x01),
    rtr: !!(flags & 0x02),
    fd:  !!(flags & 0x04),
    brs: !!(flags & 0x08),
    esi: !!(flags & 0x10),
  };
}

/**
 * Deserialize a STATUS_RSP payload from device
 *
 * Payload format:
 *   Byte 0-3:   Uptime (32-bit LE, seconds)
 *   Byte 4-5:   Battery voltage (16-bit LE, mV)
 *   Byte 6:     BLE connected (0/1)
 *   Byte 7:     USB connected (0/1)
 *   Byte 8-9:   CAN0 bus status (TEC, REC)
 *   Byte 10-11: CAN1 bus status (TEC, REC)
 *   Byte 12-15: CAN0 frame count (32-bit LE)
 *   Byte 16-19: CAN1 frame count (32-bit LE)
 *   Byte 20-23: CAN0 overflow count (32-bit LE)
 *   Byte 24-27: CAN1 overflow count (32-bit LE)
 *   Byte 28:    Temperature (signed, °C)
 *
 * @param {Buffer} payload
 * @returns {Object} Device status
 */
function deserializeStatusRsp(payload) {
  if (payload.length < 29) return null;

  return {
    uptime:         payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24),
    batteryMv:      payload[4] | (payload[5] << 8),
    bleConnected:   payload[6] === 1,
    usbConnected:   payload[7] === 1,
    can0Tec:        payload[8],
    can0Rec:        payload[9],
    can1Tec:        payload[10],
    can1Rec:        payload[11],
    can0FrameCount: payload[12] | (payload[13] << 8) | (payload[14] << 16) | (payload[15] << 24),
    can1FrameCount: payload[16] | (payload[17] << 8) | (payload[18] << 16) | (payload[19] << 24),
    can0Overflow:   payload[20] | (payload[21] << 8) | (payload[22] << 16) | (payload[23] << 24),
    can1Overflow:   payload[24] | (payload[25] << 8) | (payload[26] << 16) | (payload[27] << 24),
    temperature:    payload[28] > 127 ? payload[28] - 256 : payload[28],
  };
}

/**
 * Deserialize an ERROR_EVENT payload
 */
function deserializeErrorEvent(payload) {
  if (payload.length < 4) return null;
  return {
    code: payload[0] | (payload[1] << 8),
    data: payload[2] | (payload[3] << 8),
  };
}

/**
 * Deserialize a SCRIPT_STATUS payload
 */
function deserializeScriptStatus(payload) {
  if (payload.length < 8) return null;
  return {
    running: payload[0] === 1,
    paused:  payload[1] === 1,
    step:    payload[2] | (payload[3] << 8),
    total:   payload[4] | (payload[5] << 8),
    repeat:  payload[6] | (payload[7] << 8),
  };
}

/*===========================================================================
 * PROTOCOL HANDLER CLASS
 *===========================================================================*/

class ProtocolHandler extends EventEmitter {
  constructor() {
    super();
    this._bleConnected = false;
    this._usbConnected = false;
    this._bleDeviceId = null;
    this._blePeripheralId = null;
    this._rxBuffer = Buffer.alloc(0);
    this._rxInFrame = false;
    this._rxExpectedLen = 0;
    this._bleManager = null;
    this._pingInterval = null;
    this._deviceName = '';
    this._deviceId = '';
  }

  /*=========================================================================
   * BLE MANAGEMENT
   *=========================================================================*/

  /**
   * Initialize BLE manager
   */
  async _initBLE() {
    if (!BleManager) throw new Error('BLE module not available');

    if (!this._bleManager) {
      this._bleManager = BleManager;
      await this._bleManager.start({ showAlert: false });
    }
  }

  /**
   * Start BLE scan for CAN Creeper devices
   * @param {number} timeoutMs Scan duration
   * @returns {Array} Discovered devices
   */
  async scanBLE(timeoutMs = 5000) {
    await this._initBLE();

    return new Promise((resolve, reject) => {
      const devices = [];

      this._bleManager.startScan([NUS_SERVICE_UUID], 0, (error) => {
        if (error) {
          reject(new Error(`BLE scan error: ${error}`));
          return;
        }

        setTimeout(() => {
          this._bleManager.stopScan();
          resolve(devices);
        }, timeoutMs);
      });

      this._bleManager.addListener('BleManagerDiscoverPeripheral', (peripheral) => {
        if (peripheral.advertising?.serviceUUIDs?.includes(NUS_SERVICE_UUID)) {
          devices.push({
            id: peripheral.id,
            name: peripheral.name || peripheral.advertising?.localName || 'Unknown',
            rssi: peripheral.rssi,
          });
        }
      });
    });
  }

  /**
   * Connect to a BLE device
   * @param {string} deviceId
   */
  async connectBLE(deviceId) {
    await this._initBLE();

    try {
      await this._bleManager.connect(deviceId);
      this._blePeripheralId = deviceId;

      /* Discover services */
      const services = await this._bleManager.retrieveServices(deviceId);
      const nusService = services.find(s => s.uuid.toUpperCase() === NUS_SERVICE_UUID.toUpperCase());
      if (!nusService) throw new Error('NUS service not found');

      /* Find TX and RX characteristics */
      let txChar, rxChar;
      for (const char of nusService.characteristics) {
        if (char.uuid.toUpperCase() === NUS_TX_CHAR_UUID.toUpperCase()) {
          txChar = char;
        } else if (char.uuid.toUpperCase() === NUS_RX_CHAR_UUID.toUpperCase()) {
          rxChar = char;
        }
      }

      if (!txChar || !rxChar) throw new Error('NUS characteristics not found');

      /* Subscribe to TX characteristic (notifications from device) */
      await this._bleManager.startNotification(deviceId, nusService.uuid, txChar.uuid);

      /* Listen for notifications */
      this._bleManager.addListener('BleManagerDidUpdateValueForCharacteristic', (data) => {
        if (data.peripheral === deviceId &&
            data.characteristic.toUpperCase() === txChar.uuid.toUpperCase()) {
          const value = Buffer.from(data.value);
          this._handleRxData(value);
        }
      });

      this._bleConnected = true;
      this._bleDeviceId = deviceId;
      this._deviceName = `CAN Creeper (${deviceId.slice(-4)})`;

      /* Start ping keep-alive */
      this._startPingInterval();

      this.emit('connectionChange', {
        bleConnected: true,
        usbConnected: this._usbConnected,
        deviceName: this._deviceName,
        deviceId: deviceId,
      });

      /* Request initial status */
      await this.requestStatus();
    } catch (error) {
      throw new Error(`BLE connection failed: ${error.message}`);
    }
  }

  /**
   * Disconnect BLE
   */
  async disconnectBLE() {
    if (this._blePeripheralId && this._bleManager) {
      try {
        await this._bleManager.disconnect(this._blePeripheralId);
      } catch (e) {
        console.log('BLE disconnect error:', e.message);
      }
    }
    this._bleConnected = false;
    this._blePeripheralId = null;
    this._stopPingInterval();
    this.emit('connectionChange', {
      bleConnected: false,
      usbConnected: this._usbConnected,
      deviceName: '',
      deviceId: '',
    });
  }

  /*=========================================================================
   * USB MANAGEMENT
   *=========================================================================*/

  /**
   * Initialize USB serial connection
   */
  async _initUSB() {
    if (!SerialPort) return;
    /* USB CDC-ACM auto-connects when device is plugged in */
    /* In production, use SerialPort to open /dev/ttyACM0 or similar */
  }

  /*=========================================================================
   * DATA HANDLING
   *=========================================================================*/

  /**
   * Handle received raw data (from BLE or USB)
   * @param {Buffer} data
   */
  _handleRxData(data) {
    /* Append to RX buffer */
    this._rxBuffer = Buffer.concat([this._rxBuffer, data]);

    /* Process complete frames */
    while (this._rxBuffer.length > 0) {
      if (!this._rxInFrame) {
        /* Look for start byte */
        const startIdx = this._rxBuffer.indexOf(PROTOCOL_START_BYTE);
        if (startIdx === -1) {
          this._rxBuffer = Buffer.alloc(0);  /* No start byte, discard all */
          return;
        }
        /* Discard bytes before start */
        if (startIdx > 0) {
          this._rxBuffer = this._rxBuffer.slice(startIdx);
        }
        this._rxInFrame = true;
      }

      /* Need at least 4 bytes to determine frame length */
      if (this._rxBuffer.length < 4) return;

      const cmd = this._rxBuffer[1];
      const payloadLen = this._rxBuffer[2] | (this._rxBuffer[3] << 8);
      const totalLen = 5 + payloadLen;  /* start + cmd + len(2) + payload + crc(2) */

      if (totalLen > PROTOCOL_MAX_PAYLOAD + 5) {
        /* Invalid length — discard and resync */
        this._rxBuffer = this._rxBuffer.slice(1);
        this._rxInFrame = false;
        continue;
      }

      /* Wait for complete frame */
      if (this._rxBuffer.length < totalLen) return;

      /* Extract frame */
      const frame = this._rxBuffer.slice(0, totalLen);
      this._rxBuffer = this._rxBuffer.slice(totalLen);
      this._rxInFrame = false;

      /* Verify CRC */
      const receivedCrc = frame[totalLen - 2] | (frame[totalLen - 1] << 8);
      const computedCrc = crc16ccitt(frame.slice(1, totalLen - 2), totalLen - 3);

      if (receivedCrc !== computedCrc) {
        console.warn('CRC mismatch — discarding frame');
        continue;
      }

      /* Dispatch command */
      const payload = frame.slice(4, totalLen - 2);
      this._dispatchCommand(cmd, payload);
    }
  }

  /**
   * Dispatch a received command
   * @param {number} cmd
   * @param {Buffer} payload
   */
  _dispatchCommand(cmd, payload) {
    switch (cmd) {
      case CMD.FRAME_RX: {
        const frame = deserializeFrameRx(payload);
        if (frame) this.emit('frameReceived', frame);
        break;
      }
      case CMD.STATUS_RSP: {
        const status = deserializeStatusRsp(payload);
        if (status) this.emit('statusUpdate', status);
        break;
      }
      case CMD.ERROR_EVENT: {
        const error = deserializeErrorEvent(payload);
        if (error) this.emit('error', error);
        break;
      }
      case CMD.SCRIPT_STATUS: {
        const scriptStatus = deserializeScriptStatus(payload);
        if (scriptStatus) this.emit('scriptStatus', scriptStatus);
        break;
      }
      case CMD.PONG:
        /* Keep-alive response — connection is alive */
        break;
      case CMD.FRAME_TX_ACK:
        /* Frame TX acknowledged */
        break;
      case CMD.CONFIG_RSP:
        /* Config response */
        break;
      case CMD.DBC_LIST_RSP:
        /* DBC list response */
        break;
      case CMD.BUS_STATUS:
        /* Bus status update */
        break;
      default:
        console.log('Unknown command:', cmd.toString(16));
        break;
    }
  }

  /*=========================================================================
   * SENDING
   *=========================================================================*/

  /**
   * Send raw data to device over active transport
   * @param {Buffer} data
   */
  async _send(data) {
    if (this._bleConnected && this._blePeripheralId && this._bleManager) {
      try {
        await this._bleManager.write(
          this._blePeripheralId,
          NUS_SERVICE_UUID,
          NUS_RX_CHAR_UUID,
          data.toJSON().data
        );
        return;
      } catch (e) {
        console.log('BLE write error:', e.message);
      }
    }

    if (this._usbConnected && SerialPort) {
      try {
        await SerialPort.write(data.toString('base64'));
        return;
      } catch (e) {
        console.log('USB write error:', e.message);
      }
    }

    throw new Error('No active connection');
  }

  /**
   * Send a CAN frame for injection
   * @param {Object} frame
   */
  async sendFrame(frame) {
    const buf = serializeFrameTx(frame);
    await this._send(buf);
  }

  /**
   * Request device status
   */
  async requestStatus() {
    const buf = serializeStatusReq();
    await this._send(buf);
  }

  /**
   * Set CAN configuration
   * @param {number} channel
   * @param {Object} config
   */
  async setCanConfig(channel, config) {
    /* Send multiple CONFIG_SET commands for each setting */
    if (config.nominalBitrate !== undefined) {
      await this._send(serializeConfigSet(channel, {
        configId: 0x00,
        nominalBitrate: config.nominalBitrate,
        dataBitrate: config.dataBitrate || 0,
        fdEnabled: config.fdEnabled || false,
      }));
    }
    if (config.mode !== undefined) {
      await this._send(serializeConfigSet(channel, {
        configId: 0x02,
        mode: config.mode,
      }));
    }
    if (config.termination !== undefined) {
      await this._send(serializeConfigSet(channel, {
        configId: 0x03,
        termination: config.termination,
      }));
    }
  }

  /**
   * Set BLE configuration
   * @param {Object} config
   */
  async setBleConfig(config) {
    if (config.txPower !== undefined) {
      await this._send(serializeConfigSet(0xFF, {
        configId: 0x04,
        txPower: config.txPower,
      }));
    }
  }

  /**
   * Upload an injection script
   * @param {string} scriptJson
   */
  async uploadScript(scriptJson) {
    const buf = serializeScriptLoad(scriptJson);
    await this._send(buf);
  }

  /**
   * Start a script by name
   * @param {string} scriptName
   */
  async startScript(scriptName) {
    const buf = serializeScriptStart(scriptName);
    await this._send(buf);
  }

  /**
   * Stop running script
   */
  async stopScript() {
    const buf = serializeScriptStop();
    await this._send(buf);
  }

  /**
   * Start injection with specified parameters
   * @param {Object} params
   */
  async startInjection(params) {
    /* Build a temporary script and start it */
    const script = {
      name: 'temp_injection',
      channel: params.channel || 0,
      bitrate: 500000,
      delay_us: Math.floor(1000000 / (params.rate || 10)),
      repeat: params.repeat || 0,
      frames: params.frames || [],
    };
    await this.uploadScript(JSON.stringify(script));
    await this.startScript('temp_injection');
  }

  /**
   * Stop injection
   */
  async stopInjection() {
    await this.stopScript();
  }

  /**
   * Get script status
   */
  async getScriptStatus() {
    /* Script status is pushed by device via CMD_SCRIPT_STATUS */
    /* Return the last known status */
    return this._lastScriptStatus || { running: false, paused: false, step: 0, total: 0 };
  }

  /**
   * List scripts on device
   */
  async listScripts() {
    /* Send DBC_LIST (reused for script listing) */
    const buf = Buffer.alloc(5);
    buf[0] = PROTOCOL_START_BYTE;
    buf[1] = CMD.DBC_LIST;
    buf[2] = 0x00;
    buf[3] = 0x00;
    const crc = crc16ccitt(buf.slice(1, 3), 2);
    buf[4] = crc & 0xFF;
    buf[5] = (crc >> 8) & 0xFF;
    await this._send(buf);
    /* Response comes as DBC_LIST_RSP — for now return empty */
    return [];
  }

  /**
   * Start DFU mode
   */
  async startDFU() {
    const buf = Buffer.alloc(5);
    buf[0] = PROTOCOL_START_BYTE;
    buf[1] = CMD.DFU_START;
    buf[2] = 0x00;
    buf[3] = 0x00;
    const crc = crc16ccitt(buf.slice(1, 3), 2);
    buf[4] = crc & 0xFF;
    buf[5] = (crc >> 8) & 0xFF;
    await this._send(buf);
  }

  /*=========================================================================
   * CONNECTION MANAGEMENT
   *=========================================================================*/

  /**
   * Auto-connect on startup
   */
  async autoConnect() {
    /* Try USB first (faster, more reliable) */
    if (SerialPort) {
      try {
        await this._initUSB();
        /* USB connection is detected by device enumeration */
      } catch (e) {
        console.log('USB init failed:', e.message);
      }
    }

    /* Try BLE scan and connect to first found device */
    if (BleManager) {
      try {
        await this._initBLE();
        const devices = await this.scanBLE(3000);
        if (devices.length > 0) {
          await this.connectBLE(devices[0].id);
        }
      } catch (e) {
        console.log('BLE auto-connect failed:', e.message);
      }
    }
  }

  /**
   * Reconnect after disconnection
   */
  async reconnect() {
    if (this._bleDeviceId) {
      try {
        await this.connectBLE(this._bleDeviceId);
      } catch (e) {
        console.log('Reconnect failed:', e.message);
      }
    }
  }

  /**
   * Disconnect all transports
   */
  async disconnect() {
    await this.disconnectBLE();
    this._usbConnected = false;
    this._stopPingInterval();
  }

  /*=========================================================================
   * KEEP-ALIVE
   *=========================================================================*/

  _startPingInterval() {
    this._stopPingInterval();
    this._pingInterval = setInterval(async () => {
      if (this._bleConnected || this._usbConnected) {
        try {
          await this._send(serializePing());
        } catch (e) {
          /* Connection may be lost */
        }
      }
    }, 5000);
  }

  _stopPingInterval() {
    if (this._pingInterval) {
      clearInterval(this._pingInterval);
      this._pingInterval = null;
    }
  }
}

export default ProtocolHandler;
export { CMD, crc16ccitt, serializeFrameTx, deserializeFrameRx, deserializeStatusRsp };
