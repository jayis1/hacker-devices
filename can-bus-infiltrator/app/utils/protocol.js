/**
 * protocol.js — Binary protocol for CAN Bus Infiltrator
 * Handles BLE SPI communication and CAN frame encoding/decoding
 */

// ========== BLE Connection Management ==========
const BLE_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const BLE_CHAR_TX_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const BLE_CHAR_RX_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

// ========== Command Opcodes ==========
export const OPCODES = {
  SNIFF_START:    0x01,
  SNIFF_STOP:     0x02,
  INJECT:         0x03,
  FUZZ_START:     0x04,
  FUZZ_STOP:      0x05,
  SET_FILTER:     0x06,
  SET_BAUD:       0x07,
  SD_RECORD:      0x08,
  SD_RECORD_STOP: 0x09,
  SD_REPLAY:      0x0A,
  SD_REPLAY_STOP: 0x0B,
  SD_LIST:        0x0C,
  DEVICE_INFO:    0x0D,
  DFU_ENTER:      0x0E,
  LED_SET:        0x0F,
};

// ========== Response Opcodes ==========
export const RESPONSES = {
  CAN_FRAME:     0x80,
  CAN_ERROR:    0x81,
  FUZZ_STATUS:  0x82,
  FILE_LIST:    0x83,
  DEVICE_INFO:  0x84,
  NACK:         0xFF,
};

// ========== CAN Frame Parser ==========
export function parseCanFrame(data) {
  if (!data || data.length < 13) return null;

  const id = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  const flags = data[4];
  const idType = (flags >> 7) & 1;   // Bit 7: IDE
  const rtr = (flags >> 6) & 1;      // Bit 6: RTR
  const channel = (flags >> 2) & 1;   // Bit 2: Channel
  const dlc = flags & 0x0F;          // Bits 3-0: DLC

  const canData = [];
  for (let i = 0; i < 8; i++) {
    canData.push(data[5 + i] || 0);
  }

  // Timestamp comes after data (if present)
  const timestamp = data.length >= 17
    ? data[13] | (data[14] << 8) | (data[15] << 16) | (data[16] << 24)
    : Date.now();

  return {
    id,
    idType,
    rtr,
    channel,
    dlc,
    data: canData,
    timestamp,
  };
}

// ========== CAN Frame Encoder ==========
export function encodeCanFrame(frame) {
  const data = new Uint8Array(13);

  data[0] = frame.id & 0xFF;
  data[1] = (frame.id >> 8) & 0xFF;
  data[2] = (frame.id >> 16) & 0xFF;
  data[3] = (frame.id >> 24) & 0xFF;
  data[4] = ((frame.idType & 1) << 7) |
            ((frame.rtr ? 1 : 0) << 6) |
            ((frame.channel & 1) << 2) |
            (frame.dlc & 0x0F);

  for (let i = 0; i < 8; i++) {
    data[5 + i] = (frame.data && frame.data[i]) || 0;
  }

  return data;
}

// ========== Packet Builder ==========
export function buildPacket(opcode, data) {
  const payload = data || new Uint8Array(0);
  const packet = new Uint8Array(5 + payload.length + 1);

  packet[0] = 0xAA;               // Sync byte
  packet[1] = opcode;             // Opcode
  packet[2] = sequenceCounter++;   // Sequence
  packet[3] = (payload.length >> 8) & 0xFF;  // Length high
  packet[4] = payload.length & 0xFF;          // Length low

  for (let i = 0; i < payload.length; i++) {
    packet[5 + i] = payload[i];
  }

  // Checksum: XOR of all bytes except sync
  let checksum = 0;
  for (let i = 1; i < packet.length - 1; i++) {
    checksum ^= packet[i];
  }
  packet[packet.length - 1] = checksum;

  return packet;
}

let sequenceCounter = 0;

// ========== BLE Protocol Handler ==========
class BLEProtocol {
  constructor() {
    this.connected = false;
    this.device = null;
    this.characteristic = null;
    this.rxCharacteristic = null;
    this.frameCallback = null;
    this.errorCallback = null;
    this.fuzzCallback = null;
  }

  async connect() {
    // This is a simplified BLE connection handler.
    // In production, use react-native-ble-manager or @react-native-community/ble-plx
    try {
      // Placeholder: actual BLE connection would use:
      // BleManager.scan([BLE_SERVICE_UUID], 5, true)
      // BleManager.connect(peripheral.id)
      // BleManager.retrieveServices(peripheral.id, [BLE_SERVICE_UUID])
      // BleManager.startNotification(peripheral.id, BLE_SERVICE_UUID, BLE_CHAR_RX_UUID)
      this.connected = true;
      return true;
    } catch (e) {
      console.error('BLE connect failed:', e);
      this.connected = false;
      return false;
    }
  }

  async disconnect() {
    try {
      // BleManager.disconnect(this.device.id)
      this.connected = false;
      this.device = null;
    } catch (e) {
      console.error('BLE disconnect failed:', e);
    }
  }

  async sendCommand(opcode, data) {
    if (!this.connected) {
      console.warn('Not connected');
      return null;
    }

    const packet = buildPacket(opcode, data);

    // In production, write to BLE characteristic:
    // await BleManager.write(this.device.id, BLE_SERVICE_UUID, BLE_CHAR_TX_UUID, packet)

    console.log(`[BLE TX] Opcode: 0x${opcode.toString(16)}, Len: ${data ? data.length : 0}`);
    return null;
  }

  async sendCommandWithResponse(opcode, data) {
    await this.sendCommand(opcode, data);

    // In production, wait for response notification
    // This would use a Promise that resolves when the BLE notification arrives
    return new Uint8Array([0x84, 0x01, 0x00, 0x02, 0x00, 0x00]);
  }

  onDataReceived(data) {
    // Parse incoming BLE data
    if (data[0] !== 0xAA) return;

    const opcode = data[1];
    // const seq = data[2];
    const length = (data[3] << 8) | data[4];
    const payload = data.slice(5, 5 + length);

    switch (opcode) {
      case RESPONSES.CAN_FRAME:
        if (this.frameCallback) {
          const frame = parseCanFrame(payload);
          this.frameCallback(frame);
        }
        break;

      case RESPONSES.CAN_ERROR:
        if (this.errorCallback) {
          this.errorCallback({
            channel: payload[0],
            errorCode: payload[1],
            tec: payload[2],
            rec: payload[3],
          });
        }
        break;

      case RESPONSES.FUZZ_STATUS:
        if (this.fuzzCallback) {
          this.fuzzCallback({
            channel: payload[0],
            count: (payload[1]) | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24),
          });
        }
        break;

      default:
        console.log(`[BLE RX] Unknown opcode: 0x${opcode.toString(16)}`);
    }
  }

  onFrame(callback) {
    this.frameCallback = callback;
  }

  onError(callback) {
    this.errorCallback = callback;
  }

  onFuzzProgress(callback) {
    this.fuzzCallback = callback;
  }
}

export const BLE_PROTOCOL = new BLEProtocol();