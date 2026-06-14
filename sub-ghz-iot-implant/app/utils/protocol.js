/**
 * protocol.js — Protocol decoding utilities for Substation
 * Parses captured packets and identifies protocols
 */

// Protocol name display mapping
export const protocolNames = {
  zigbee: 'Zigbee',
  zwave: 'Z-Wave',
  subghz: 'Sub-GHz',
  ble: 'BLE',
  unknown: 'Unknown',
};

// Protocol badge colors
export const protocolColors = {
  zigbee: '#4CAF50',
  zwave: '#2196F3',
  subghz: '#FF9800',
  ble: '#9C27B0',
  unknown: '#757575',
};

/**
 * Decode a raw packet buffer into a structured object
 * @param {Uint8Array} data - Raw packet bytes from BLE
 * @returns {Object} Decoded packet object
 */
export function decodePacket(data) {
  if (!data || data.length < 8) {
    return { protocol: 'unknown', error: 'Packet too short' };
  }

  const timestamp = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
  const rssi = data[4] > 127 ? data[4] - 256 : data[4];
  const channel = data[5];
  const length = data[6];
  const payload = data.slice(7, 7 + length);
  const lqi = data.length > 7 + length ? data[7 + length] : 0;
  const crcValid = data.length > 8 + length ? data[8 + length] === 1 : false;

  // Detect protocol from payload
  const protocol = detectProtocol(payload, channel);

  return {
    timestamp,
    rssi,
    channel,
    length,
    payload,
    lqi,
    crcValid,
    protocol,
    decoded: decodeProtocolData(protocol, payload),
  };
}

/**
 * Detect protocol from packet payload
 */
function detectProtocol(payload, channel) {
  if (payload.length < 2) return 'unknown';

  // IEEE 802.15.4 (Zigbee) frame detection
  // Frame control field: first 2 bytes
  const fcf = (payload[1] << 8) | payload[0];
  const frameType = fcf & 0x07;

  if (frameType <= 0x03 && channel >= 11 && channel <= 26) {
    // Valid 802.15.4 frame type on a valid Zigbee channel
    return 'zigbee';
  }

  // Z-Wave detection: single-byte frame header
  // Z-Wave frames start with home ID (4 bytes) + node ID (1 byte)
  if (payload.length >= 5 && channel >= 0 && channel <= 3) {
    // Z-Wave channels 0-3 map to specific frequencies
    if (payload[0] >= 0x01 && payload[0] <= 0x09) {
      return 'zwave';
    }
  }

  // BLE detection (not expected on Sub-GHz, but included)
  if (payload.length >= 2 && (payload[0] & 0xF0) === 0x00) {
    // BLE advertising PDU type
    if (payload.length >= 6) return 'ble';
  }

  // Default: generic Sub-GHz
  return 'subghz';
}

/**
 * Decode protocol-specific data
 */
function decodeProtocolData(protocol, payload) {
  switch (protocol) {
    case 'zigbee':
      return decodeZigbee(payload);
    case 'zwave':
      return decodeZwave(payload);
    case 'subghz':
      return decodeSubGhz(payload);
    default:
      return { raw: formatHex(payload) };
  }
}

/**
 * Decode IEEE 802.15.4 / Zigbee frame
 */
function decodeZigbee(payload) {
  if (payload.length < 2) return { error: 'Frame too short' };

  const fcf = (payload[1] << 8) | payload[0];
  const frameType = fcf & 0x07;
  const security = (fcf >> 3) & 0x01;
  const framePending = (fcf >> 4) & 0x01;
  const ackRequest = (fcf >> 5) & 0x01;
  const panIdCompression = (fcf >> 6) & 0x01;
  const dstAddrMode = (fcf >> 10) & 0x03;
  const frameVersion = (fcf >> 12) & 0x03;
  const srcAddrMode = (fcf >> 14) & 0x03;

  const frameTypes = ['Beacon', 'Data', 'ACK', 'MAC Command', 'Reserved', 'Reserved', 'Reserved', 'Reserved'];

  let offset = 2;
  const seqNumber = payload.length > offset ? payload[offset] : 0;
  offset++;

  // Destination PAN ID
  let dstPanId = null;
  let dstAddr = null;
  if (dstAddrMode >= 2 && payload.length > offset + 3) {
    dstPanId = (payload[offset + 1] << 8) | payload[offset];
    offset += 2;
    if (dstAddrMode === 2) {
      dstAddr = (payload[offset + 1] << 8) | payload[offset];
      offset += 2;
    } else if (dstAddrMode === 3) {
      dstAddr = payload.slice(offset, offset + 8)
        .reduce((acc, b, i) => acc | (b << (i * 8)), 0);
      offset += 8;
    }
  }

  // Source PAN ID and address
  let srcPanId = null;
  let srcAddr = null;
  if (srcAddrMode >= 2 && payload.length > offset + 1) {
    if (!panIdCompression) {
      srcPanId = (payload[offset + 1] << 8) | payload[offset];
      offset += 2;
    } else {
      srcPanId = dstPanId;
    }
    if (srcAddrMode === 2) {
      srcAddr = (payload[offset + 1] << 8) | payload[offset];
      offset += 2;
    } else if (srcAddrMode === 3) {
      srcAddr = payload.slice(offset, offset + 8)
        .reduce((acc, b, i) => acc | (b << (i * 8)), 0);
      offset += 8;
    }
  }

  return {
    frameType: frameTypes[frameType] || `Unknown (${frameType})`,
    security: security === 1,
    framePending: framePending === 1,
    ackRequest: ackRequest === 1,
    frameVersion,
    seqNumber,
    dstPanId: dstPanId !== null ? `0x${dstPanId.toString(16).padStart(4, '0')}` : null,
    dstAddr: dstAddr !== null ? `0x${dstAddr.toString(16)}` : null,
    srcPanId: srcPanId !== null ? `0x${srcPanId.toString(16).padStart(4, '0')}` : null,
    srcAddr: srcAddr !== null ? `0x${srcAddr.toString(16)}` : null,
    payloadLength: payload.length - offset,
    payloadHex: formatHex(payload.slice(offset)),
  };
}

/**
 * Decode Z-Wave frame
 */
function decodeZwave(payload) {
  if (payload.length < 5) return { error: 'Frame too short' };

  const homeId = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
  const sourceNodeId = payload[4];
  let frameControl = 0;
  let length = 0;
  let destNodeId = 0;

  if (payload.length > 5) {
    frameControl = payload[5];
    length = payload.length > 6 ? payload[6] : 0;
  }
  if (payload.length > 7) {
    destNodeId = payload[7];
  }

  const frameTypes = { 0x01: 'Single', 0x02: 'Multi', 0x03: 'Ack', 0x04: 'Routed' };
  const type = frameTypes[frameControl] || `Unknown (0x${frameControl.toString(16)})`;

  return {
    homeId: `0x${homeId.toString(16).padStart(8, '0')}`,
    sourceNodeId,
    frameControl: `0x${frameControl.toString(16).padStart(2, '0')}`,
    frameType: type,
    length,
    destNodeId,
    payloadHex: formatHex(payload.slice(8)),
  };
}

/**
 * Decode generic Sub-GHz frame
 */
function decodeSubGhz(payload) {
  return {
    rawHex: formatHex(payload),
    byteCount: payload.length,
    firstByte: `0x${payload[0].toString(16).padStart(2, '0')}`,
  };
}

/**
 * Format byte array as hex string
 */
function formatHex(bytes) {
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, '0'))
    .join(' ');
}

/**
 * Parse hex string to byte array
 */
export function parseHex(hexString) {
  return hexString
    .trim()
    .split(/\s+/)
    .map((h) => parseInt(h, 16));
}

/**
 * Calculate RSSI quality percentage
 */
export function rssiToQuality(rssi) {
  // Map RSSI (-100 to -20 dBm) to 0-100%
  const clamped = Math.max(-100, Math.min(-20, rssi));
  return Math.round(((clamped + 100) / 80) * 100);
}

/**
 * Get signal strength bar count (0-4)
 */
export function rssiToBars(rssi) {
  if (rssi >= -50) return 4;
  if (rssi >= -65) return 3;
  if (rssi >= -80) return 2;
  if (rssi >= -95) return 1;
  return 0;
}