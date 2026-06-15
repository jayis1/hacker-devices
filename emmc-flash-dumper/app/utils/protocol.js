/**
 * protocol.js — Binary Wire Protocol for eMMC Flash Dumper
 *
 * Implements the complete USB communication protocol between the
 * React Native companion app and the eMMC Flash Dumper firmware.
 * Handles frame construction, CRC-16/XMODEM validation, command
 * sequencing, data streaming with CRC-32, and automatic retry.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

import { Buffer } from 'buffer';
import CRC32 from 'crc-32';

/*===========================================================================
 * Protocol Constants
 *===========================================================================*/

const SYNC_COMMAND = 0xFD4D4644;  /* "FDMD" — Flash Dumper Magic */
const SYNC_DATA    = 0xFD444154;  /* "FDAT" — Flash Data */
const SYNC_BYTES_CMD = Buffer.from([0xFD, 0x4D, 0x46, 0x44]);
const SYNC_BYTES_DATA = Buffer.from([0xFD, 0x44, 0x41, 0x54]);

const FRAME_HEADER_SIZE = 12;  /* SYNC(4) + CMD(2) + LEN(4) + SEQ(2) */
const FRAME_CRC_SIZE    = 2;   /* CRC-16/XMODEM */
const FRAME_MIN_SIZE    = FRAME_HEADER_SIZE + FRAME_CRC_SIZE;

const DATA_FRAME_HEADER_SIZE = 8;   /* SYNC(4) + SEQ(4) */
const DATA_FRAME_CRC_SIZE    = 4;   /* CRC-32/MPEG2 */
const DATA_FRAME_MAX_PAYLOAD = 65536;
const DATA_FRAME_MIN_SIZE    = DATA_FRAME_HEADER_SIZE + DATA_FRAME_CRC_SIZE;

const USB_BULK_TRANSFER_SIZE = 512;
const USB_BULK_TIMEOUT_MS    = 5000;

/*===========================================================================
 * Command IDs
 *===========================================================================*/

export const CMD = {
  GET_INFO:           0x0001,
  INFO_RESP:          0x0002,
  EMMC_DETECT:        0x0010,
  EMMC_INFO:          0x0011,
  EMMC_ACQUIRE:       0x0012,
  EMMC_DATA:          0x0013,
  EMMC_PROGRESS:      0x0014,
  EMMC_COMPLETE:      0x0015,
  EMMC_ABORT:         0x0016,
  NAND_DETECT:        0x0020,
  NAND_INFO:          0x0021,
  NAND_ACQUIRE:       0x0022,
  NAND_DATA:          0x0023,
  NAND_PROGRESS:      0x0024,
  NAND_COMPLETE:      0x0025,
  SPINOR_DETECT:      0x0030,
  SPINOR_INFO:        0x0031,
  SPINOR_ACQUIRE:     0x0032,
  SPINOR_DATA:        0x0033,
  SPINOR_COMPLETE:    0x0034,
  SDCARD_STATUS:      0x0040,
  SDCARD_START:       0x0041,
  SDCARD_PROGRESS:    0x0042,
  HASH_START:         0x0050,
  HASH_RESULT:        0x0051,
  SELFTEST:           0x0060,
  SELFTEST_RESULT:    0x0061,
  FPGA_LOAD:          0x0070,
  FPGA_STATUS:        0x0071,
  ERROR:              0x00FF,
  ACK:                0x0100,
  NACK:               0x0101,
};

/*===========================================================================
 * Error Codes
 *===========================================================================*/

export const ERR = {
  NONE:               0x0000,
  SDRAM_INIT:         0x1001,
  SDRAM_TEST:         0x1002,
  FPGA_LOAD:          0x2001,
  FPGA_ID:            0x2002,
  FPGA_TIMEOUT:       0x2003,
  EMMC_NO_CARD:       0x3001,
  EMMC_INIT_FAIL:     0x3002,
  EMMC_HS400_FAIL:    0x3003,
  EMMC_CRC:           0x3004,
  EMMC_TIMEOUT:       0x3005,
  NAND_NO_CHIP:       0x4001,
  NAND_ONFI_FAIL:     0x4002,
  NAND_TIMING:        0x4003,
  NAND_READ_FAIL:     0x4004,
  SPINOR_NO_CHIP:     0x5001,
  SPINOR_SFDP_FAIL:   0x5002,
  SPINOR_READ_FAIL:   0x5003,
  USB_ENUM_FAIL:      0x6001,
  USB_DISCONNECT:     0x6002,
  USB_FIFO_OVERFLOW:  0x6003,
  SDCARD_NO_CARD:     0x7001,
  SDCARD_FULL:        0x7002,
  SDCARD_WRITE_FAIL:  0x7003,
  HASH_FAIL:          0x8001,
  POWER_LOW_BATTERY:  0x9001,
  POWER_RAIL_FAIL:    0x9002,
  CMD_INVALID:        0xA001,
  CMD_PARAM:          0xA002,
  CMD_SEQUENCE:       0xA003,
};

export const ERROR_STRINGS = {
  [ERR.NONE]:              'No error',
  [ERR.SDRAM_INIT]:        'SDRAM initialization failed',
  [ERR.SDRAM_TEST]:        'SDRAM memory test failed',
  [ERR.FPGA_LOAD]:         'FPGA bitstream load failed',
  [ERR.FPGA_ID]:           'FPGA ID verification failed',
  [ERR.FPGA_TIMEOUT]:      'FPGA operation timed out',
  [ERR.EMMC_NO_CARD]:      'No eMMC device detected',
  [ERR.EMMC_INIT_FAIL]:    'eMMC initialization failed',
  [ERR.EMMC_HS400_FAIL]:   'eMMC HS400 mode switch failed',
  [ERR.EMMC_CRC]:          'eMMC CRC error',
  [ERR.EMMC_TIMEOUT]:      'eMMC operation timed out',
  [ERR.NAND_NO_CHIP]:      'No NAND flash detected',
  [ERR.NAND_ONFI_FAIL]:    'ONFI parameter page invalid',
  [ERR.NAND_TIMING]:       'NAND timing configuration failed',
  [ERR.NAND_READ_FAIL]:    'NAND page read failed',
  [ERR.SPINOR_NO_CHIP]:    'No SPI NOR flash detected',
  [ERR.SPINOR_SFDP_FAIL]:  'SFDP table parse failed',
  [ERR.SPINOR_READ_FAIL]:  'SPI NOR read failed',
  [ERR.USB_ENUM_FAIL]:     'USB enumeration failed',
  [ERR.USB_DISCONNECT]:    'USB disconnected',
  [ERR.USB_FIFO_OVERFLOW]: 'USB FIFO overflow',
  [ERR.SDCARD_NO_CARD]:    'No microSD card inserted',
  [ERR.SDCARD_FULL]:       'microSD card full',
  [ERR.SDCARD_WRITE_FAIL]: 'microSD write failed',
  [ERR.HASH_FAIL]:         'SHA-256 hash computation failed',
  [ERR.POWER_LOW_BATTERY]: 'Battery low',
  [ERR.POWER_RAIL_FAIL]:   'Power rail failure',
  [ERR.CMD_INVALID]:       'Invalid command',
  [ERR.CMD_PARAM]:         'Invalid command parameter',
  [ERR.CMD_SEQUENCE]:      'Command sequence error',
};

/*===========================================================================
 * CRC-16/XMODEM Implementation
 *===========================================================================*/

function crc16Xmodem(data, offset = 0, length = data.length) {
  let crc = 0;
  for (let i = offset; i < offset + length; i++) {
    crc ^= (data[i] << 8);
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
  }
  return crc;
}

/*===========================================================================
 * Frame Builder
 *===========================================================================*/

/**
 * Build a command frame.
 *
 * Frame format:
 *   SYNC (4 bytes): 0xFD 0x4D 0x46 0x44
 *   CMD  (2 bytes): Command ID (little-endian)
 *   LEN  (4 bytes): Payload length (little-endian)
 *   SEQ  (2 bytes): Sequence number (little-endian)
 *   PAYLOAD (LEN bytes): Variable-length data
 *   CRC16 (2 bytes): CRC-16/XMODEM over CMD+LEN+SEQ+PAYLOAD
 *
 * Returns a Buffer containing the complete frame.
 */
export function buildCommandFrame(cmdId, payload = null, seq = 0) {
  const payloadLen = payload ? payload.length : 0;
  const totalLen = FRAME_HEADER_SIZE + payloadLen + FRAME_CRC_SIZE;
  const frame = Buffer.alloc(totalLen);

  /* SYNC */
  frame.writeUInt32LE(SYNC_COMMAND, 0);

  /* CMD */
  frame.writeUInt16LE(cmdId, 4);

  /* LEN */
  frame.writeUInt32LE(payloadLen, 6);

  /* SEQ */
  frame.writeUInt16LE(seq, 10);

  /* PAYLOAD */
  if (payload && payloadLen > 0) {
    payload.copy(frame, FRAME_HEADER_SIZE);
  }

  /* CRC16 over CMD+LEN+SEQ+PAYLOAD */
  const crc = crc16Xmodem(frame, 4, 8 + payloadLen);
  frame.writeUInt16LE(crc, FRAME_HEADER_SIZE + payloadLen);

  return frame;
}

/**
 * Build a data stream frame for high-throughput transfer.
 *
 * Frame format:
 *   SYNC (4 bytes): 0xFD 0x44 0x41 0x54
 *   SEQ  (4 bytes): Block sequence number (little-endian)
 *   DATA (N bytes): Raw flash data (up to 65536 bytes)
 *   CRC32 (4 bytes): CRC-32/MPEG2 over SEQ+DATA
 *
 * Returns a Buffer containing the complete frame.
 */
export function buildDataFrame(seq, data) {
  const totalLen = DATA_FRAME_HEADER_SIZE + data.length + DATA_FRAME_CRC_SIZE;
  const frame = Buffer.alloc(totalLen);

  /* SYNC */
  frame.writeUInt32LE(SYNC_DATA, 0);

  /* SEQ */
  frame.writeUInt32LE(seq, 4);

  /* DATA */
  data.copy(frame, DATA_FRAME_HEADER_SIZE);

  /* CRC32 over SEQ+DATA */
  const crcInput = Buffer.alloc(4 + data.length);
  crcInput.writeUInt32LE(seq, 0);
  data.copy(crcInput, 4);
  const crc = CRC32.buf(crcInput) >>> 0;  /* Unsigned 32-bit */
  frame.writeUInt32LE(crc, DATA_FRAME_HEADER_SIZE + data.length);

  return frame;
}

/*===========================================================================
 * Frame Parser
 *===========================================================================*/

/**
 * Parse a command frame from raw bytes.
 *
 * Returns an object:
 *   { valid: boolean, cmd: number, length: number, seq: number,
 *     payload: Buffer, crc: number, error: string|null }
 */
export function parseCommandFrame(data) {
  if (data.length < FRAME_MIN_SIZE) {
    return { valid: false, error: 'Frame too short' };
  }

  /* Check SYNC */
  const sync = data.readUInt32LE(0);
  if (sync !== SYNC_COMMAND) {
    return { valid: false, error: `Invalid sync: 0x${sync.toString(16)}` };
  }

  /* Extract fields */
  const cmd = data.readUInt16LE(4);
  const len = data.readUInt32LE(6);
  const seq = data.readUInt16LE(10);

  if (data.length < FRAME_HEADER_SIZE + len + FRAME_CRC_SIZE) {
    return { valid: false, error: 'Frame truncated' };
  }

  /* Extract payload */
  const payload = len > 0
    ? Buffer.from(data.slice(FRAME_HEADER_SIZE, FRAME_HEADER_SIZE + len))
    : null;

  /* Extract CRC */
  const receivedCrc = data.readUInt16LE(FRAME_HEADER_SIZE + len);

  /* Verify CRC */
  const computedCrc = crc16Xmodem(data, 4, 8 + len);
  if (receivedCrc !== computedCrc) {
    return {
      valid: false,
      cmd,
      seq,
      error: `CRC mismatch: received 0x${receivedCrc.toString(16)}, computed 0x${computedCrc.toString(16)}`,
    };
  }

  return {
    valid: true,
    cmd,
    length: len,
    seq,
    payload,
    crc: receivedCrc,
    error: null,
  };
}

/**
 * Parse a data stream frame from raw bytes.
 *
 * Returns an object:
 *   { valid: boolean, seq: number, data: Buffer, crc: number, error: string|null }
 */
export function parseDataFrame(data) {
  if (data.length < DATA_FRAME_MIN_SIZE) {
    return { valid: false, error: 'Data frame too short' };
  }

  /* Check SYNC */
  const sync = data.readUInt32LE(0);
  if (sync !== SYNC_DATA) {
    return { valid: false, error: `Invalid data sync: 0x${sync.toString(16)}` };
  }

  /* Extract fields */
  const seq = data.readUInt32LE(4);
  const dataLen = data.length - DATA_FRAME_HEADER_SIZE - DATA_FRAME_CRC_SIZE;
  const payload = Buffer.from(data.slice(DATA_FRAME_HEADER_SIZE, DATA_FRAME_HEADER_SIZE + dataLen));
  const receivedCrc = data.readUInt32LE(DATA_FRAME_HEADER_SIZE + dataLen);

  /* Verify CRC32 */
  const crcInput = Buffer.alloc(4 + dataLen);
  crcInput.writeUInt32LE(seq, 0);
  payload.copy(crcInput, 4);
  const computedCrc = CRC32.buf(crcInput) >>> 0;

  if (receivedCrc !== computedCrc) {
    return {
      valid: false,
      seq,
      error: `CRC32 mismatch: received 0x${receivedCrc.toString(16)}, computed 0x${computedCrc.toString(16)}`,
    };
  }

  return {
    valid: true,
    seq,
    data: payload,
    crc: receivedCrc,
    error: null,
  };
}

/*===========================================================================
 * FlashDumperProtocol Class
 *===========================================================================*/

export class FlashDumperProtocol extends EventEmitter {
  constructor() {
    super();
    this._device = null;
    this._seq = 0;
    this._pendingCommands = new Map();
    this._discoveryTimer = null;
    this._connected = false;
    this._commandQueue = [];
    this._processingQueue = false;
    this._dataStreamActive = false;
    this._dataStreamSeq = 0;
    this._dataStreamBuffer = Buffer.alloc(0);
    this._dataStreamTotalBlocks = 0;
    this._dataStreamBlocksReceived = 0;
    this._retryCount = 0;
    this._maxRetries = 3;
  }

  /*=======================================================================
   * Device Discovery
   *=======================================================================*/

  startDiscovery() {
    this._discoveryTimer = setInterval(() => {
      this._scanForDevice();
    }, 2000);
    this._scanForDevice();
  }

  stopDiscovery() {
    if (this._discoveryTimer) {
      clearInterval(this._discoveryTimer);
      this._discoveryTimer = null;
    }
  }

  async _scanForDevice() {
    if (this._connected) return;

    try {
      const devices = await USB.getDevices();
      for (const device of devices) {
        if (device.vendorId === 0x4E4F && device.productId === 0x4644) {
          await this._connectDevice(device);
          return;
        }
      }
    } catch (e) {
      /* No USB permission or no devices */
    }
  }

  async _connectDevice(device) {
    try {
      await device.open();
      await device.selectConfiguration(1);
      await device.claimInterface(0);

      this._device = device;
      this._connected = true;

      /* Get device info */
      const info = await this.sendCommand(CMD.GET_INFO);
      if (info && info.valid) {
        const payload = info.payload;
        const deviceInfo = {
          hwRev: payload[0],
          fwMajor: payload[1],
          fwMinor: payload[2],
          fwPatch: payload[3],
          batteryMv: payload.readUInt16LE(4),
          sdCardPresent: payload[6] === 1,
          usbConnected: payload[7] === 1,
        };
        this.emit('attached', deviceInfo);
        this.emit('battery', deviceInfo.batteryMv);
      }

      /* Start polling for battery updates */
      this._startBatteryPolling();
    } catch (e) {
      console.error('Device connection failed:', e);
    }
  }

  _startBatteryPolling() {
    this._batteryTimer = setInterval(async () => {
      if (!this._connected) return;
      try {
        const info = await this.sendCommand(CMD.GET_INFO);
        if (info && info.valid) {
          const mv = info.payload.readUInt16LE(4);
          this.emit('battery', mv);
        }
      } catch (e) {
        /* Ignore polling errors */
      }
    }, 5000);
  }

  /*=======================================================================
   * Command Sending
   *=======================================================================*/

  async sendCommand(cmdId, payload = null, timeoutMs = 5000) {
    if (!this._connected) {
      return { valid: false, error: 'Device not connected' };
    }

    const seq = this._seq++;
    const frame = buildCommandFrame(cmdId, payload, seq);

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this._pendingCommands.delete(seq);
        reject(new Error(`Command 0x${cmdId.toString(16)} timed out`));
      }, timeoutMs);

      this._pendingCommands.set(seq, { resolve, reject, timeout });

      this._sendRawFrame(frame).catch((e) => {
        clearTimeout(timeout);
        this._pendingCommands.delete(seq);
        reject(e);
      });
    });
  }

  async _sendRawFrame(frame) {
    if (!this._device) throw new Error('No device');

    /* Split frame into bulk transfer chunks */
    for (let offset = 0; offset < frame.length; offset += USB_BULK_TRANSFER_SIZE) {
      const chunk = frame.slice(offset, offset + USB_BULK_TRANSFER_SIZE);
      await this._device.transferOut(0x01, chunk);  /* EP1 OUT */
    }
  }

  /*=======================================================================
   * Response Handling
   *=======================================================================*/

  async _readResponse() {
    if (!this._device) return null;

    try {
      const result = await this._device.transferIn(0x81, USB_BULK_TRANSFER_SIZE);
      if (result.status === 'ok' && result.data) {
        return Buffer.from(result.data.buffer);
      }
    } catch (e) {
      /* No data available */
    }
    return null;
  }

  async _processResponses() {
    while (this._connected) {
      const data = await this._readResponse();
      if (!data) {
        await new Promise(r => setTimeout(r, 10));
        continue;
      }

      /* Check if this is a data stream frame */
      const sync = data.readUInt32LE(0);
      if (sync === SYNC_DATA) {
        this._handleDataFrame(data);
        continue;
      }

      /* Parse as command frame */
      const parsed = parseCommandFrame(data);
      if (!parsed.valid) {
        console.warn('Invalid response frame:', parsed.error);
        continue;
      }

      /* Dispatch to pending command */
      const pending = this._pendingCommands.get(parsed.seq);
      if (pending) {
        clearTimeout(pending.timeout);
        this._pendingCommands.delete(parsed.seq);
        pending.resolve(parsed);
      }

      /* Handle unsolicited messages */
      switch (parsed.cmd) {
        case CMD.EMMC_PROGRESS:
          this.emit('emmcProgress', {
            blocksRead: parsed.payload.readUInt32LE(0),
            totalBlocks: parsed.payload.readUInt32LE(4),
          });
          break;
        case CMD.NAND_PROGRESS:
          this.emit('nandProgress', {
            pagesRead: parsed.payload.readUInt32LE(0),
            totalPages: parsed.payload.readUInt32LE(4),
          });
          break;
        case CMD.ERROR:
          this.emit('deviceError', {
            code: parsed.payload.readUInt16LE(0),
            message: ERROR_STRINGS[parsed.payload.readUInt16LE(0)] || 'Unknown error',
          });
          break;
      }
    }
  }

  /*=======================================================================
   * Data Stream Handling
   *=======================================================================*/

  _handleDataFrame(data) {
    const parsed = parseDataFrame(data);
    if (!parsed.valid) {
      console.warn('Invalid data frame:', parsed.error);
      /* Request retransmission */
      this._requestRetransmit(parsed.seq);
      return;
    }

    /* Check sequence */
    if (parsed.seq !== this._dataStreamSeq) {
      console.warn(`Data seq mismatch: expected ${this._dataStreamSeq}, got ${parsed.seq}`);
      if (parsed.seq < this._dataStreamSeq) {
        return;  /* Duplicate, ignore */
      }
      /* Missed frames — request retransmission */
      this._requestRetransmit(this._dataStreamSeq);
      return;
    }

    /* Append data */
    this._dataStreamBuffer = Buffer.concat([this._dataStreamBuffer, parsed.data]);
    this._dataStreamSeq++;
    this._dataStreamBlocksReceived++;

    /* Emit progress */
    this.emit('dataProgress', {
      blocksReceived: this._dataStreamBlocksReceived,
      totalBlocks: this._dataStreamTotalBlocks,
      bytesReceived: this._dataStreamBuffer.length,
    });
  }

  async _requestRetransmit(seq) {
    if (this._retryCount >= this._maxRetries) {
      this.emit('dataError', 'Max retries exceeded');
      this._dataStreamActive = false;
      return;
    }
    this._retryCount++;
    /* Send NACK with the expected sequence number */
    const payload = Buffer.alloc(4);
    payload.writeUInt32LE(seq, 0);
    await this.sendCommand(CMD.NACK, payload);
  }

  /*=======================================================================
   * High-Level API
   *=======================================================================*/

  /**
   * Get device information.
   */
  async getDeviceInfo() {
    const resp = await this.sendCommand(CMD.GET_INFO);
    if (!resp.valid) throw new Error(resp.error);
    return {
      hwRev: resp.payload[0],
      fwVersion: `${resp.payload[1]}.${resp.payload[2]}.${resp.payload[3]}`,
      batteryMv: resp.payload.readUInt16LE(4),
      sdCardPresent: resp.payload[6] === 1,
      usbConnected: resp.payload[7] === 1,
    };
  }

  /**
   * Detect eMMC device and return its info.
   */
  async detectEmmc() {
    const resp = await this.sendCommand(CMD.EMMC_DETECT);
    if (!resp.valid) throw new Error(resp.error);

    /* Wait for EMMC_INFO response */
    const infoResp = await this.sendCommand(CMD.EMMC_INFO);
    if (!infoResp.valid) throw new Error(infoResp.error);

    return {
      capacityBlocks: infoResp.payload.readUInt32LE(0),
      capacityBytes: infoResp.payload.readUInt32LE(4),
      hs400Supported: infoResp.payload[8] === 1,
      boot0Size: infoResp.payload.readUInt32LE(9),
      boot1Size: infoResp.payload.readUInt32LE(13),
      rpmbSize: infoResp.payload.readUInt32LE(17),
      lifeEstA: infoResp.payload[21],
      lifeEstB: infoResp.payload[22],
      preEol: infoResp.payload[23],
    };
  }

  /**
   * Start eMMC acquisition.
   * Returns a data stream that can be iterated.
   */
  async startEmmcAcquire(startBlock, totalBlocks, onProgress) {
    const payload = Buffer.alloc(8);
    payload.writeUInt32LE(startBlock, 0);
    payload.writeUInt32LE(totalBlocks, 4);

    const resp = await this.sendCommand(CMD.EMMC_ACQUIRE, payload);
    if (!resp.valid) throw new Error(resp.error);

    /* Set up data stream */
    this._dataStreamActive = true;
    this._dataStreamSeq = 0;
    this._dataStreamBuffer = Buffer.alloc(0);
    this._dataStreamTotalBlocks = totalBlocks;
    this._dataStreamBlocksReceived = 0;
    this._retryCount = 0;

    /* Listen for progress */
    if (onProgress) {
      this.on('dataProgress', onProgress);
    }

    /* Wait for completion */
    return new Promise((resolve, reject) => {
      const onComplete = (resp) => {
        this.removeListener('dataError', onError);
        if (onProgress) this.removeListener('dataProgress', onProgress);
        resolve({
          data: this._dataStreamBuffer,
          sha256: resp.payload.slice(0, 32),
          totalBytes: this._dataStreamBuffer.length,
        });
      };

      const onError = (msg) => {
        this.removeListener('emmcComplete', onComplete);
        if (onProgress) this.removeListener('dataProgress', onProgress);
        reject(new Error(msg));
      };

      this.once('emmcComplete', onComplete);
      this.once('dataError', onError);
    });
  }

  /**
   * Abort eMMC acquisition.
   */
  async abortEmmcAcquire() {
    const resp = await this.sendCommand(CMD.EMMC_ABORT);
    this._dataStreamActive = false;
    return resp.valid;
  }

  /**
   * Detect NAND flash and return ONFI info.
   */
  async detectNand() {
    const resp = await this.sendCommand(CMD.NAND_DETECT);
    if (!resp.valid) throw new Error(resp.error);

    const infoResp = await this.sendCommand(CMD.NAND_INFO);
    if (!infoResp.valid) throw new Error(infoResp.error);

    return {
      manufacturerId: infoResp.payload[0],
      deviceId: infoResp.payload[1],
      pageSize: infoResp.payload.readUInt32LE(2),
      oobSize: infoResp.payload.readUInt16LE(6),
      pagesPerBlock: infoResp.payload.readUInt32LE(8),
      blocksPerLun: infoResp.payload.readUInt32LE(12),
      totalSizeBytes: infoResp.payload.readUInt32LE(16),
      onfiCompliant: infoResp.payload[20] === 1,
    };
  }

  /**
   * Start NAND acquisition.
   */
  async startNandAcquire(onProgress) {
    const resp = await this.sendCommand(CMD.NAND_ACQUIRE);
    if (!resp.valid) throw new Error(resp.error);

    this._dataStreamActive = true;
    this._dataStreamSeq = 0;
    this._dataStreamBuffer = Buffer.alloc(0);
    this._retryCount = 0;

    if (onProgress) {
      this.on('dataProgress', onProgress);
    }

    return new Promise((resolve, reject) => {
      const onComplete = (resp) => {
        this.removeListener('dataError', onError);
        if (onProgress) this.removeListener('dataProgress', onProgress);
        resolve({
          data: this._dataStreamBuffer,
          sha256: resp.payload.slice(0, 32),
          totalBytes: this._dataStreamBuffer.length,
        });
      };
      const onError = (msg) => {
        this.removeListener('nandComplete', onComplete);
        if (onProgress) this.removeListener('dataProgress', onProgress);
        reject(new Error(msg));
      };
      this.once('nandComplete', onComplete);
      this.once('dataError', onError);
    });
  }

  /**
   * Detect SPI NOR flash.
   */
  async detectSpiNor() {
    const resp = await this.sendCommand(CMD.SPINOR_DETECT);
    if (!resp.valid) throw new Error(resp.error);

    const infoResp = await this.sendCommand(CMD.SPINOR_INFO);
    if (!infoResp.valid) throw new Error(infoResp.error);

    return {
      manufacturerId: infoResp.payload[0],
      memoryType: infoResp.payload[1],
      capacity: infoResp.payload[2],
      sizeBytes: 1 << infoResp.payload[2],
      quadSpiSupported: infoResp.payload[3] === 1,
    };
  }

  /**
   * Start SHA-256 hash computation over acquired data.
   */
  async startHash() {
    const resp = await this.sendCommand(CMD.HASH_START);
    if (!resp.valid) throw new Error(resp.error);

    const hashResp = await this.sendCommand(CMD.HASH_RESULT);
    if (!hashResp.valid) throw new Error(hashResp.error);

    return {
      digest: hashResp.payload.slice(0, 32),
      digestHex: hashResp.payload.slice(0, 32).toString('hex'),
    };
  }

  /**
   * Run device self-test.
   */
  async runSelfTest() {
    const resp = await this.sendCommand(CMD.SELFTEST);
    if (!resp.valid) throw new Error(resp.error);

    const resultResp = await this.sendCommand(CMD.SELFTEST_RESULT);
    if (!resultResp.valid) throw new Error(resultResp.error);

    return {
      errors: resultResp.payload.readUInt16LE(0),
      sdramOk: resultResp.payload[2] === 1,
      fpgaOk: resultResp.payload[3] === 1,
      oledOk: resultResp.payload[4] === 1,
      sdCardOk: resultResp.payload[5] === 1,
      batteryOk: resultResp.payload[6] === 1,
    };
  }

  /**
   * Disconnect from device.
   */
  async disconnect() {
    this._connected = false;
    this._dataStreamActive = false;
    if (this._batteryTimer) clearInterval(this._batteryTimer);
    if (this._device) {
      try {
        await this._device.close();
      } catch (e) {
        /* Ignore */
      }
      this._device = null;
    }
    this.emit('detached');
  }
}

/*===========================================================================
 * EventEmitter Polyfill (minimal)
 *===========================================================================*/

class EventEmitter {
  constructor() {
    this._listeners = {};
  }

  on(event, fn) {
    if (!this._listeners[event]) this._listeners[event] = [];
    this._listeners[event].push(fn);
    return this;
  }

  once(event, fn) {
    const wrapper = (...args) => {
      this.removeListener(event, wrapper);
      fn(...args);
    };
    this.on(event, wrapper);
    return this;
  }

  removeListener(event, fn) {
    if (!this._listeners[event]) return this;
    this._listeners[event] = this._listeners[event].filter(f => f !== fn);
    return this;
  }

  removeAllListeners(event) {
    if (event) {
      delete this._listeners[event];
    } else {
      this._listeners = {};
    }
    return this;
  }

  emit(event, ...args) {
    if (!this._listeners[event]) return false;
    this._listeners[event].forEach(fn => fn(...args));
    return true;
  }
}

/*===========================================================================
 * USB Mock (for environments without react-native-usb)
 *===========================================================================*/

const USB = {
  getDevices: async () => [],
};
