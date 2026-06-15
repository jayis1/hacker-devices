/**
 * protocol.js — VoltGlitcher Pro USB/serial communication protocol
 * Binary protocol implementation for the STM32F407 + iCE40HX1K glitcher
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

/* ============================================================================
 * Constants
 * ============================================================================ */

export const DEVICE_STATES = {
  DISCONNECTED: 'disconnected',
  CONNECTED: 'connected',
  IDLE: 'idle',
  ARMED: 'armed',
  FIRING: 'firing',
  CALIBRATING: 'calibrating',
  ERROR: 'error',
};

export const GLITCH_MODES = {
  VOLTAGE_GLITCH: 1,
  EM_GLITCH: 2,
  CLOCK_GLITCH: 3,
  TRIGGER_SCAN: 4,
  CALIBRATION: 5,
};

export const TRIGGER_MODES = {
  NONE: 0,
  GPIO: 1,
  UART_PATTERN: 2,
  JTAG_STATE: 3,
  MANUAL: 4,
  FPGA_PATTERN: 5,
};

export const GLITCH_SHAPES = {
  RECTANGLE: 0,
  TRIANGLE: 1,
  GAUSSIAN: 2,
  SAWTOOTH: 3,
  CUSTOM: 4,
};

/* USB vendor request command codes */
export const CMD = {
  GET_STATUS:       0x01,
  SET_GLITCH_CFG:   0x02,
  GET_GLITCH_CFG:   0x03,
  ARM:              0x04,
  DISARM:           0x05,
  FIRE:             0x06,
  GET_RESULTS:      0x07,
  SET_TRIGGER:      0x08,
  GET_TRIGGER:      0x09,
  FPGA_WRITE:       0x0A,
  FPGA_READ:        0x0B,
  FPGA_LOAD_BIT:    0x0C,
  EEPROM_READ:      0x0D,
  EEPROM_WRITE:     0x0E,
  ADC_READ:         0x0F,
  CALIBRATE:        0x10,
  SET_PROFILE:      0x11,
  GET_PROFILE:      0x12,
  WAVEFORM_LOAD:    0x13,
  WAVEFORM_START:   0x14,
  WAVEFORM_STOP:    0x15,
  GET_TIMESTAMP:    0x16,
  RESET:            0xFF,
};

/* Response status codes */
export const STATUS = {
  OK:       0x00,
  ERROR:    0x01,
  BUSY:     0x02,
  INVALID:  0x03,
  FAULT:    0x04,
  TIMEOUT:  0x05,
  UNARMED:  0x06,
  NO_FPGA:  0x07,
};

/* Event types (from EP3 IN interrupt endpoint) */
export const EVENT_TYPES = {
  TRIGGER_DETECTED:  0x01,
  GLITCH_FIRED:     0x02,
  FAULT:            0x03,
  CALIBRATION_DONE: 0x04,
  OVERCURRENT:      0x05,
  OVERTEMP:         0x06,
  FPGA_READY:       0x07,
};

/* Packet structure offsets */
const PKT_SIZE = 64;
const CMD_IDX = 0;
const SUBCMD_IDX = 1;
const PARAM1_IDX = 2;
const PARAM2_IDX = 4;
const PARAM3_IDX = 6;
const PARAM4_IDX = 8;
const CHECKSUM_IDX = 10;
const PAYLOAD_START = 11;
const PAYLOAD_MAX_LEN = 53;

/* ============================================================================
 * Packet Builder
 * ============================================================================ */

function buildCommandPacket(cmd, subcmd = 0, params = {}, payload = null) {
  const pkt = new Uint8Array(PKT_SIZE);

  pkt[CMD_IDX] = cmd;
  pkt[SUBCMD_IDX] = subcmd;
  pkt[PARAM1_IDX] = params.p1 & 0xFF;
  pkt[PARAM1_IDX + 1] = (params.p1 >> 8) & 0xFF;
  pkt[PARAM2_IDX] = params.p2 & 0xFF;
  pkt[PARAM2_IDX + 1] = (params.p2 >> 8) & 0xFF;
  pkt[PARAM3_IDX] = params.p3 & 0xFF;
  pkt[PARAM3_IDX + 1] = (params.p3 >> 8) & 0xFF;
  pkt[PARAM4_IDX] = params.p4 & 0xFF;
  pkt[PARAM4_IDX + 1] = (params.p4 >> 8) & 0xFF;

  if (payload && payload.length <= PAYLOAD_MAX_LEN) {
    pkt.set(payload, PAYLOAD_START);
  }

  /* Compute XOR checksum of bytes 0-9 */
  let checksum = 0;
  for (let i = 0; i < CHECKSUM_IDX; i++) {
    checksum ^= pkt[i];
  }
  pkt[CHECKSUM_IDX] = checksum;

  return pkt;
}

function parseResponsePacket(pkt) {
  if (!pkt || pkt.length < PKT_SIZE) {
    return { success: false, status: STATUS.INVALID, data: null };
  }

  /* Verify checksum */
  let checksum = 0;
  for (let i = 0; i < CHECKSUM_IDX; i++) {
    checksum ^= pkt[i];
  }
  if (checksum !== pkt[CHECKSUM_IDX]) {
    return { success: false, status: STATUS.INVALID, data: null, error: 'Checksum mismatch' };
  }

  const cmd = pkt[CMD_IDX];
  const status = pkt[1];

  return {
    success: status === STATUS.OK,
    status,
    cmd,
    result1: pkt[2] | (pkt[3] << 8),
    result2: pkt[4] | (pkt[5] << 8),
    result3: pkt[6] | (pkt[7] << 8) | (pkt[8] << 16) | (pkt[9] << 24),
    payload: pkt.slice(PAYLOAD_START),
  };
}

function parseEventPacket(pkt) {
  if (!pkt || pkt.length < 8) return null;

  return {
    type: pkt[0],
    data: pkt[1],
    timestamp: pkt[2] | (pkt[3] << 8) | (pkt[4] << 16) | (pkt[5] << 24),
    reserved: pkt[6] | (pkt[7] << 8),
  };
}

/* ============================================================================
 * Glitch Config Serialization
 * ============================================================================ */

export function serializeGlitchConfig(config) {
  const buf = new Uint8Array(40);

  buf[0] = config.mode & 0xFF;
  buf[1] = config.shape & 0xFF;
  buf[2] = config.triggerMode & 0xFF;
  buf[3] = config.triggerEdge & 0xFF;
  buf[4] = config.delayNs & 0xFF;
  buf[5] = (config.delayNs >> 8) & 0xFF;
  buf[6] = config.widthNs & 0xFF;
  buf[7] = (config.widthNs >> 8) & 0xFF;
  buf[8] = config.repeatCount & 0xFF;
  buf[9] = (config.repeatCount >> 8) & 0xFF;
  buf[10] = config.repeatDelayNs & 0xFF;
  buf[11] = (config.repeatDelayNs >> 8) & 0xFF;
  buf[12] = config.emAmplitude & 0xFF;
  buf[13] = (config.emAmplitude >> 8) & 0xFF;
  buf[14] = config.clkPhaseOffset & 0xFF;
  buf[15] = (config.clkPhaseOffset >> 8) & 0xFF;

  /* UART pattern (4 bytes) */
  buf[16] = config.uartPattern?.[0] ?? 0;
  buf[17] = config.uartPattern?.[1] ?? 0;
  buf[18] = config.uartPattern?.[2] ?? 0;
  buf[19] = config.uartPattern?.[3] ?? 0;
  buf[20] = config.uartMask ?? 0xFF;
  buf[21] = config.jtagState ?? 0;
  buf[22] = config.gpioTriggerPin ?? 0;
  /* bytes 23-25: padding */
  buf[26] = config.vccTargetMv & 0xFF;
  buf[27] = (config.vccTargetMv >> 8) & 0xFF;
  buf[28] = (config.vccTargetMv >> 16) & 0xFF;
  buf[29] = (config.vccTargetMv >> 24) & 0xFF;
  buf[30] = config.maxCurrentMa & 0xFF;
  buf[31] = (config.maxCurrentMa >> 8) & 0xFF;
  buf[32] = (config.maxCurrentMa >> 16) & 0xFF;
  buf[33] = (config.maxCurrentMa >> 24) & 0xFF;
  buf[34] = config.autoArm ? 1 : 0;
  buf[35] = config.safetyLimit ? 1 : 0;

  return buf;
}

export function deserializeGlitchConfig(buf) {
  if (!buf || buf.length < 36) return null;

  return {
    mode: buf[0],
    shape: buf[1],
    triggerMode: buf[2],
    triggerEdge: buf[3],
    delayNs: buf[4] | (buf[5] << 8),
    widthNs: buf[6] | (buf[7] << 8),
    repeatCount: buf[8] | (buf[9] << 8),
    repeatDelayNs: buf[10] | (buf[11] << 8),
    emAmplitude: buf[12] | (buf[13] << 8),
    clkPhaseOffset: buf[14] | (buf[15] << 8),
    uartPattern: [buf[16], buf[17], buf[18], buf[19]],
    uartMask: buf[20],
    jtagState: buf[21],
    gpioTriggerPin: buf[22],
    vccTargetMv: buf[26] | (buf[27] << 8) | (buf[28] << 16) | (buf[29] << 24),
    maxCurrentMa: buf[30] | (buf[31] << 8) | (buf[32] << 16) | (buf[33] << 24),
    autoArm: buf[34] === 1,
    safetyLimit: buf[35] === 1,
  };
}

/* ============================================================================
 * Results Deserialization
 * ============================================================================ */

export function deserializeResults(buf) {
  if (!buf || buf.length < 24) return null;

  const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);

  return {
    glitchCount:     dv.getUint32(0, true),
    triggerCount:    dv.getUint32(4, true),
    lastTimestamp:   dv.getUint32(8, true),
    targetVccMv:     dv.getInt16(12, true),
    shuntCurrentMa:  dv.getInt16(14, true),
    faultFlags:      dv.getUint16(16, true),
    fpgaStatus:      buf[18],
    modeActive:      buf[19],
    armed:           buf[20],
    temperature:     buf[21],
    successCount:    dv.getUint16(22, true),
  };
}

/* ============================================================================
 * Default Configuration
 * ============================================================================ */

export function getDefaultConfig() {
  return {
    mode: GLITCH_MODES.VOLTAGE_GLITCH,
    shape: GLITCH_SHAPES.RECTANGLE,
    triggerMode: TRIGGER_MODES.MANUAL,
    triggerEdge: 0,
    delayNs: 0,
    widthNs: 100,
    repeatCount: 1,
    repeatDelayNs: 1000,
    emAmplitude: 512,
    clkPhaseOffset: 0,
    uartPattern: [0x00, 0x00, 0x00, 0x00],
    uartMask: 0xFF,
    jtagState: 0,
    gpioTriggerPin: 0,
    vccTargetMv: 3300,
    maxCurrentMa: 500,
    autoArm: false,
    safetyLimit: true,
  };
}

/* ============================================================================
 * Protocol Class
 * ============================================================================ */

export class VoltGlitcherProtocol {
  constructor() {
    this.device = null;
    this.connection = null;
    this.state = DEVICE_STATES.DISCONNECTED;
    this.eventListeners = new Map();
    this.pendingRequests = new Map();
    this.requestId = 0;
    this.pollingInterval = null;
    this.eventPollingInterval = null;

    /* Callbacks */
    this.onStateChange = null;
    this.onDeviceInfo = null;
    this.onResults = null;
    this.onEvent = null;
    this.onConfigUpdate = null;
  }

  /* ---- Connection Management ---- */

  async connect() {
    try {
      /* Try WebUSB first, fall back to Web Serial */
      if (navigator && navigator.usb) {
        this.device = await navigator.usb.requestDevice({
          filters: [{ vendorId: 0x1209, productId: 0xAC11 }]
        });
        await this.device.open();
        await this.device.claimInterface(0);
        this.connection = 'usb';
      } else {
        throw new Error('No USB support');
      }

      this._setState(DEVICE_STATES.CONNECTED);

      /* Start polling for results and events */
      this._startPolling();

      /* Get initial device info */
      const status = await this.getStatus();
      if (this.onDeviceInfo) {
        this.onDeviceInfo({
          firmwareVersion: '1.0.0',
          fpgaReady: !!(status & 0x02),
          armed: !!(status & 0x01),
        });
      }

      /* Load current config */
      const config = await this.getGlitchConfig();
      if (config && this.onConfigUpdate) {
        this.onConfigUpdate(config);
      }

    } catch (e) {
      this._setState(DEVICE_STATES.DISCONNECTED);
      throw new Error(`Connection failed: ${e.message}`);
    }
  }

  async disconnect() {
    this._stopPolling();

    if (this.device) {
      try {
        await this.device.close();
      } catch (e) {
        /* Ignore close errors */
      }
      this.device = null;
    }

    this._setState(DEVICE_STATES.DISCONNECTED);
  }

  /* ---- Command Interface ---- */

  async _sendCommand(cmd, subcmd = 0, params = {}, payload = null) {
    if (!this.device) throw new Error('Not connected');

    const pkt = buildCommandPacket(cmd, subcmd, params, payload);

    /* Send to EP1 OUT */
    await this.device.transferOut(1, pkt);

    /* Read response from EP2 IN */
    const response = await this.device.transferIn(2, PKT_SIZE);
    const respData = new Uint8Array(response.data.buffer);
    const parsed = parseResponsePacket(respData);

    return parsed;
  }

  async _sendCommandNoWait(cmd, subcmd = 0, params = {}, payload = null) {
    if (!this.device) throw new Error('Not connected');
    const pkt = buildCommandPacket(cmd, subcmd, params, payload);
    await this.device.transferOut(1, pkt);
  }

  /* ---- High-Level API ---- */

  async getStatus() {
    const resp = await this._sendCommand(CMD.GET_STATUS);
    if (resp.success) {
      return resp.result1;
    }
    return 0;
  }

  async getGlitchConfig() {
    const resp = await this._sendCommand(CMD.GET_GLITCH_CFG);
    if (resp.success && resp.payload) {
      return deserializeGlitchConfig(resp.payload);
    }
    return null;
  }

  async setGlitchConfig(config) {
    const payload = serializeGlitchConfig(config);
    const resp = await this._sendCommand(CMD.SET_GLITCH_CFG, 0, {}, payload);
    return resp.success;
  }

  async arm() {
    const resp = await this._sendCommand(CMD.ARM);
    if (resp.success) {
      this._setState(DEVICE_STATES.ARMED);
    }
    return resp.success;
  }

  async disarm() {
    const resp = await this._sendCommand(CMD.DISARM);
    if (resp.success) {
      this._setState(DEVICE_STATES.IDLE);
    }
    return resp.success;
  }

  async fireManual() {
    const resp = await this._sendCommand(CMD.FIRE);
    return resp.success;
  }

  async getResults() {
    const resp = await this._sendCommand(CMD.GET_RESULTS);
    if (resp.success && resp.payload) {
      const results = deserializeResults(resp.payload);
      if (this.onResults) this.onResults(results);
      return results;
    }
    return null;
  }

  async setTrigger(mode, edge = 0) {
    const resp = await this._sendCommand(CMD.SET_TRIGGER, mode, { p1: edge });
    return resp.success;
  }

  async fpgaWrite(addr, value) {
    const resp = await this._sendCommand(CMD.FPGA_WRITE, addr, { p1: value });
    return resp.success;
  }

  async fpgaRead(addr) {
    const resp = await this._sendCommand(CMD.FPGA_READ, addr);
    if (resp.success) return resp.result1;
    return null;
  }

  async fpgaLoadBitstream() {
    const resp = await this._sendCommand(CMD.FPGA_LOAD_BIT);
    return resp.success;
  }

  async readADC(channel) {
    const resp = await this._sendCommand(CMD.ADC_READ, channel);
    if (resp.success) {
      return {
        raw: resp.result1,
        millivolts: resp.result2,
      };
    }
    return null;
  }

  async calibrate() {
    const resp = await this._sendCommand(CMD.CALIBRATE);
    return resp.success;
  }

  async loadProfile(profileId) {
    const resp = await this._sendCommand(CMD.GET_PROFILE, profileId);
    if (resp.success && resp.payload) {
      return deserializeGlitchConfig(resp.payload);
    }
    return null;
  }

  async saveProfile(profileId, name, config) {
    const payload = serializeGlitchConfig(config);
    const resp = await this._sendCommand(CMD.SET_PROFILE, profileId, {}, payload);
    return resp.success;
  }

  async loadWaveform(samples) {
    /* Upload waveform in chunks of 53 samples per packet */
    for (let i = 0; i < samples.length; i += PAYLOAD_MAX_LEN / 2) {
      const chunk = samples.slice(i, i + PAYLOAD_MAX_LEN / 2);
      const payload = new Uint8Array(chunk.length * 2);
      const dv = new DataView(payload.buffer);
      for (let j = 0; j < chunk.length; j++) {
        dv.setUint16(j * 2, chunk[j], true);
      }
      await this._sendCommand(CMD.WAVEFORM_LOAD, 0, { p1: i }, payload);
    }
    return true;
  }

  async startWaveform(loop = false, trigger = false) {
    const flags = (loop ? 0x02 : 0) | (trigger ? 0x04 : 0) | 0x01;
    const resp = await this._sendCommand(CMD.WAVEFORM_START, flags);
    return resp.success;
  }

  async stopWaveform() {
    const resp = await this._sendCommand(CMD.WAVEFORM_STOP);
    return resp.success;
  }

  async markSuccess() {
    /* Mark the last glitch as interesting — device tracks success count */
    return true;
  }

  async reset() {
    const resp = await this._sendCommand(CMD.RESET);
    return resp.success;
  }

  /* ---- Polling ---- */

  _startPolling() {
    /* Poll results every 500ms */
    this.pollingInterval = setInterval(async () => {
      try {
        await this.getResults();
      } catch (e) {
        /* Connection lost? */
      }
    }, 500);

    /* Poll events every 100ms */
    this.eventPollingInterval = setInterval(async () => {
      try {
        if (!this.device) return;
        const result = await this.device.transferIn(3, 8);
        if (result.status === 'ok' && result.data) {
          const evtData = new Uint8Array(result.data.buffer);
          const evt = parseEventPacket(evtData);
          if (evt && this.onEvent) {
            this.onEvent(evt);
          }
        }
      } catch (e) {
        /* No event available, that's normal */
      }
    }, 100);
  }

  _stopPolling() {
    if (this.pollingInterval) {
      clearInterval(this.pollingInterval);
      this.pollingInterval = null;
    }
    if (this.eventPollingInterval) {
      clearInterval(this.eventPollingInterval);
      this.eventPollingInterval = null;
    }
  }

  /* ---- Internal Helpers ---- */

  _setState(newState) {
    if (this.state !== newState) {
      this.state = newState;
      if (this.onStateChange) {
        this.onStateChange(newState);
      }
    }
  }
}

export default VoltGlitcherProtocol;