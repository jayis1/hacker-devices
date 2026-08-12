/**
 * protocol.js
 *
 * Binary protocol handler for GLYPH-REAPER companion app.
 * Manages BLE/USB connection, command sending, and event dispatching
 * for frame data, OCR text, and credential alerts.
 *
 * Author: jayis1
 * @license MIT
 */

import { EventEmitter } from 'events';

/* Protocol constants */
const SYNC1 = 0xAA;
const SYNC2 = 0x55;
const HEADER_SIZE = 5;

/* Command opcodes */
const CMD_START_CAPTURE = 0x01;
const CMD_STOP_CAPTURE = 0x02;
const CMD_SET_PROTOCOL = 0x03;
const CMD_SET_RESOLUTION = 0x04;
const CMD_SET_COLOR_DEPTH = 0x05;
const CMD_SET_CAPTURE_FPS = 0x06;
const CMD_SET_TRIGGER_MODE = 0x07;
const CMD_GET_STATUS = 0x08;
const CMD_GET_FRAME = 0x09;
const CMD_SET_OCR_MODE = 0x0A;
const CMD_SET_ENCRYPTION_KEY = 0x0E;
const CMD_GET_DEVICE_INFO = 0x12;
const CMD_FACTORY_RESET = 0x13;
const CMD_SET_POWER_MODE = 0x10;
const CMD_ERASE_HISTORY = 0x11;
const CMD_FIRMWARE_UPDATE = 0x0D;

/* Message opcodes */
const MSG_FRAME_DATA = 0x81;
const MSG_OCR_TEXT = 0x82;
const MSG_CREDENTIAL_ALERT = 0x83;
const MSG_STATUS_UPDATE = 0x84;
const MSG_ERROR = 0x85;
const MSG_DEVICE_INFO = 0x87;

/* CRC8 table (polynomial 0x07) */
const crc8Table = [
  0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
  0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
  0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
  0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
  0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
  0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
  0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
  0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
];

function calculateCRC8(data) {
  let crc = 0;
  for (let i = 0; i < data.length; i++) {
    crc = crc8Table[crc ^ data[i]];
  }
  return crc;
}

/**
 * ProtocolHandler class
 * Manages communication with the GLYPH-REAPER hardware device.
 */
class ProtocolHandler extends EventEmitter {
  constructor() {
    super();
    this.bleConnected = false;
    this.usbConnected = false;
    this.deviceName = '';
    this.deviceId = '';
    this.rxBuffer = [];
    this.parseState = 'WAIT_SYNC1';
    this.pendingCmd = 0;
    this.pendingLen = 0;
    this.pendingParams = [];
    this.paramIdx = 0;
  }

  /**
   * Auto-connect to any available GLYPH-REAPER device
   */
  async autoConnect() {
    /* Try BLE first, then USB */
    try {
      await this.startBLEScan();
    } catch (e) {
      /* BLE scan will find devices via callback */
    }
  }

  /**
   * Start BLE scanning for GLYPH-REAPER devices
   */
  async startBLEScan() {
    /* In production, this uses react-native-ble-manager:
     * BleManager.scan([GLYPH_SERVICE_UUID], 5, true)
     * and listens for 'BleManagerDiscoverPeripheral' events
     */
    this.emit('scanningStarted');
  }

  /**
   * Connect to a BLE device by ID
   */
  async connectBLE(deviceId) {
    /* BleManager.connect(deviceId) */
    this.deviceId = deviceId;
    this.bleConnected = true;
    this.deviceName = 'GLYPH-REAPER';
    this.emit('connectionChange', {
      bleConnected: true,
      usbConnected: this.usbConnected,
      deviceName: this.deviceName,
      deviceId: this.deviceId,
    });
  }

  /**
   * Connect via USB CDC
   */
  async connectUSB() {
    this.usbConnected = true;
    this.emit('connectionChange', {
      bleConnected: this.bleConnected,
      usbConnected: true,
      deviceName: 'GLYPH-REAPER',
      deviceId: 'usb',
    });
  }

  /**
   * Disconnect from device
   */
  disconnect() {
    this.bleConnected = false;
    this.usbConnected = false;
    this.emit('connectionChange', {
      bleConnected: false,
      usbConnected: false,
      deviceName: '',
      deviceId: '',
    });
  }

  /**
   * Attempt to reconnect
   */
  reconnect() {
    if (this.deviceId) {
      this.connectBLE(this.deviceId);
    } else {
      this.startBLEScan();
    }
  }

  /**
   * Send a command to the device
   */
  sendCommand(command, params = []) {
    const packet = this.buildPacket(command, params);
    this.sendRaw(packet);
  }

  /**
   * Build a protocol packet
   */
  buildPacket(command, params = []) {
    const len = params.length;
    const packet = new Uint8Array(HEADER_SIZE + len + 1);

    packet[0] = SYNC1;
    packet[1] = SYNC2;
    packet[2] = command;
    packet[3] = (len >> 8) & 0xFF;
    packet[4] = len & 0xFF;

    for (let i = 0; i < len; i++) {
      packet[HEADER_SIZE + i] = params[i];
    }

    /* Calculate CRC8 over command + length + params */
    const crcData = new Uint8Array(3 + len);
    crcData[0] = command;
    crcData[1] = (len >> 8) & 0xFF;
    crcData[2] = len & 0xFF;
    for (let i = 0; i < len; i++) {
      crcData[3 + i] = params[i];
    }
    packet[HEADER_SIZE + len] = calculateCRC8(crcData);

    return packet;
  }

  /**
   * Send raw bytes to the device (BLE or USB)
   */
  sendRaw(data) {
    if (this.bleConnected) {
      /* BleManager.write(deviceId, serviceUUID, charUUID, data) */
    } else if (this.usbConnected) {
      /* SerialPort.write(data) */
    }
  }

  /**
   * Process incoming data from the device
   */
  receiveData(data) {
    for (let i = 0; i < data.length; i++) {
      this.parseByte(data[i]);
    }
  }

  /**
   * Parse a single byte from the device
   */
  parseByte(byte) {
    switch (this.parseState) {
    case 'WAIT_SYNC1':
      if (byte === SYNC1) {
        this.parseState = 'WAIT_SYNC2';
      }
      break;

    case 'WAIT_SYNC2':
      if (byte === SYNC2) {
        this.parseState = 'WAIT_CMD';
      } else {
        this.parseState = 'WAIT_SYNC1';
      }
      break;

    case 'WAIT_CMD':
      this.pendingCmd = byte;
      this.parseState = 'WAIT_LEN_HI';
      break;

    case 'WAIT_LEN_HI':
      this.pendingLen = byte << 8;
      this.parseState = 'WAIT_LEN_LO';
      break;

    case 'WAIT_LEN_LO':
      this.pendingLen |= byte;
      if (this.pendingLen === 0) {
        this.parseState = 'WAIT_CRC';
      } else {
        this.pendingParams = new Array(this.pendingLen);
        this.paramIdx = 0;
        this.parseState = 'WAIT_PARAMS';
      }
      break;

    case 'WAIT_PARAMS':
      this.pendingParams[this.paramIdx++] = byte;
      if (this.paramIdx >= this.pendingLen) {
        this.parseState = 'WAIT_CRC';
      }
      break;

    case 'WAIT_CRC':
      this.processMessage(this.pendingCmd, this.pendingParams);
      this.parseState = 'WAIT_SYNC1';
      break;

    default:
      this.parseState = 'WAIT_SYNC1';
      break;
    }
  }

  /**
   * Process a complete message from the device
   */
  processMessage(cmd, params) {
    switch (cmd) {
    case MSG_FRAME_DATA:
      this.handleFrameData(params);
      break;

    case MSG_OCR_TEXT:
      this.handleOcrText(params);
      break;

    case MSG_CREDENTIAL_ALERT:
      this.handleCredentialAlert(params);
      break;

    case MSG_STATUS_UPDATE:
      this.handleStatusUpdate(params);
      break;

    case MSG_ERROR:
      this.handleError(params);
      break;

    case MSG_DEVICE_INFO:
      this.handleDeviceInfo(params);
      break;

    default:
      break;
    }
  }

  handleFrameData(params) {
    const frameIndex = (params[0] << 24) | (params[1] << 16) | 
                       (params[2] << 8) | params[3];
    const width = (params[4] << 8) | params[5];
    const height = (params[6] << 8) | params[7];
    const colorDepth = params[8];
    const compressionType = params[9];
    const compressedSize = (params[10] << 24) | (params[11] << 16) |
                           (params[12] << 8) | params[13];
    const flags = params[14];

    this.emit('frameReceived', {
      frameIndex,
      width,
      height,
      colorDepth,
      compressionType,
      compressedSize,
      flags,
      pixelData: null, /* Would be assembled from subsequent data chunks */
    });
  }

  handleOcrText(params) {
    const frameIndex = (params[0] << 24) | (params[1] << 16) |
                       (params[2] << 8) | params[3];
    const length = (params[4] << 8) | params[5];
    let text = '';
    for (let i = 6; i < 6 + length && i < params.length; i++) {
      text += String.fromCharCode(params[i]);
    }

    this.emit('ocrText', { frameIndex, text });
  }

  handleCredentialAlert(params) {
    const frameIndex = (params[0] << 24) | (params[1] << 16) |
                       (params[2] << 8) | params[3];
    const length = (params[4] << 8) | params[5];

    /* Parse alert data (simplified) */
    const patternType = params[6] || 0;
    const x = (params[7] << 8) | params[8] || 0;
    const y = (params[9] << 8) | params[10] || 0;
    const confidence = params[11] || 0;

    let text = '';
    for (let i = 12; i < 12 + length && i < params.length; i++) {
      text += String.fromCharCode(params[i]);
    }

    this.emit('credentialAlert', {
      frameIndex,
      patternType,
      x, y,
      confidence,
      text,
    });
  }

  handleStatusUpdate(params) {
    const sysState = params[0];
    const batMv = (params[1] << 8) | params[2];
    const temp = params[3];

    const stateNames = ['INIT', 'IDLE', 'CAPTURING', 'PROCESSING', 
                        'EXFILTRATING', 'SLEEP', 'ERROR', 'FW_UPDATE'];

    this.emit('statusUpdate', {
      systemState: stateNames[sysState] || 'UNKNOWN',
      batteryMv: batMv,
      temperature: temp,
      uptime: 0, /* Would be parsed from params */
    });
  }

  handleError(params) {
    const errorCode = params[0];
    this.emit('error', {
      code: errorCode,
      message: `Device error 0x${errorCode.toString(16)}`,
    });
  }

  handleDeviceInfo(params) {
    const deviceId = params.slice(0, 8).map(b => 
      b.toString(16).padStart(2, '0')
    ).join('');

    this.deviceId = deviceId;
    this.emit('deviceInfo', { deviceId });
  }
}

export default ProtocolHandler;