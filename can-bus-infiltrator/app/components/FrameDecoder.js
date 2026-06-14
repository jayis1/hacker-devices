/**
 * FrameDecoder.js — CAN frame protocol decoder component
 * Decodes OBD-II, UDS, and SAE J1939 protocols
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export function decodeObd2(id, data, idType) {
  // OBD-II request: 0x7DF (standard), 0x7E0-0x7E7 (specific ECU)
  // OBD-II response: 0x7E8-0x7EF
  if (idType === 0 && id >= 0x7E0 && id <= 0x7EF) {
    const isRequest = id === 0x7DF;
    const ecuId = id - 0x7E0;
    const mode = data[0];
    const pid = data[1];

    const modeNames = {
      0x01: 'Show current data',
      0x02: 'Show freeze frame data',
      0x03: 'Show stored DTCs',
      0x04: 'Clear DTCs',
      0x05: 'Test results',
      0x06: 'On-board monitoring',
      0x07: 'Show pending DTCs',
      0x08: 'Control operation',
      0x09: 'Request vehicle info',
      0x0A: 'Permanent DTCs',
    };

    const pidNames = {
      0x00: 'PIDs supported [01-20]',
      0x01: 'Monitor status',
      0x04: 'Engine load',
      0x05: 'Coolant temp',
      0x0C: 'Engine RPM',
      0x0D: 'Vehicle speed',
      0x2F: 'Fuel level',
      0x46: 'Ambient temp',
    };

    return {
      protocol: 'OBD-II',
      direction: isRequest ? 'Request' : 'Response',
      ecu: `ECU ${ecuId}`,
      mode: `Mode 0x${mode.toString(16).toUpperCase()} - ${modeNames[mode] || 'Unknown'}`,
      pid: `PID 0x${pid.toString(16).toUpperCase()} - ${pidNames[pid] || 'Unknown'}`,
    };
  }
  return null;
}

export function decodeJ1939(id, data) {
  if (id > 0x7FF) {
    const pgn = (id >> 8) & 0xFFFF;
    const priority = (id >> 26) & 0x07;
    const sourceAddr = id & 0xFF;

    const pgnNames = {
      0xF004: 'Time/Date',
      0xFEF1: 'Cruise Control',
      0xFEF5: 'Direction Selector',
      0xFECA: 'DM1 Active DTCs',
      0xFEE0: 'Vehicle Distance',
      0xFEE1: 'Vehicle Speed',
      0xFEE2: 'Engine Speed',
      0xFEE3: 'Engine Coolant Temp',
      0xFEE4: 'Engine Oil Temp',
      0xFEE5: 'Engine Oil Pressure',
      0xFEE6: 'Fuel Rate',
      0xFEE7: 'Engine Coolant Pressure',
    };

    return {
      protocol: 'J1939',
      pgn: `PGN ${pgn.toString(16).toUpperCase()} - ${pgnNames[pgn] || 'Unknown'}`,
      priority: `Priority ${priority}`,
      source: `SA ${sourceAddr.toString(16).toUpperCase()}`,
    };
  }
  return null;
}

export default function FrameDecoder({ frame }) {
  const obd2 = decodeObd2(frame.id, frame.data, frame.idType);
  const j1939 = decodeJ1939(frame.id, frame.data);

  if (!obd2 && !j1939) return null;

  const decoded = obd2 || j1939;

  return (
    <View style={styles.decoder}>
      <Text style={styles.protocolTag}>{decoded.protocol}</Text>
      {Object.entries(decoded).map(([key, value]) => {
        if (key === 'protocol') return null;
        return (
          <Text key={key} style={styles.decoderLine}>
            <Text style={styles.decoderKey}>{key}: </Text>
            <Text style={styles.decoderValue}>{value}</Text>
          </Text>
        );
      })}
    </View>
  );
}

const styles = StyleSheet.create({
  decoder: {
    backgroundColor: '#1A1A2E',
    borderRadius: 4,
    padding: 6,
    marginTop: 2,
    borderLeftWidth: 3,
    borderLeftColor: '#8957E5',
  },
  protocolTag: {
    color: '#8957E5',
    fontSize: 10,
    fontWeight: 'bold',
    marginBottom: 2,
  },
  decoderLine: { color: '#C9D1D9', fontSize: 10 },
  decoderKey: { color: '#8B949E' },
  decoderValue: { color: '#E6EDF3' },
});