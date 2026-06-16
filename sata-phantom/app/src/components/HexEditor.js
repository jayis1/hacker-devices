/**
 * HexEditor.js — Hex Data Editor Component
 * Author: jayis1
 *
 * A hex data input component for editing raw binary frame data.
 * Displays hex bytes in a structured layout with ASCII preview.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, StyleSheet, ScrollView } from 'react-native';

const BYTES_PER_ROW = 16;

const HexEditor = ({ value, onChange }) => {
  const cleanHex = value.replace(/\s+/g, '').toUpperCase();
  const bytes = [];
  for (let i = 0; i < cleanHex.length; i += 2) {
    if (i + 1 < cleanHex.length) {
      bytes.push(cleanHex.substring(i, i + 2));
    }
  }

  const handleTextChange = (text) => {
    // Only allow hex characters and spaces
    const filtered = text.replace(/[^0-9a-fA-F\s]/g, '');
    onChange(filtered);
  };

  const getAsciiPreview = (startIdx) => {
    let ascii = '';
    for (let i = startIdx; i < startIdx + BYTES_PER_ROW && i < bytes.length; i++) {
      const code = parseInt(bytes[i], 16);
      ascii += (code >= 32 && code <= 126) ? String.fromCharCode(code) : '.';
    }
    return ascii;
  };

  const rows = [];
  for (let i = 0; i < bytes.length; i += BYTES_PER_ROW) {
    const rowBytes = bytes.slice(i, i + BYTES_PER_ROW);
    const hexStr = rowBytes.map((b, j) => {
      const spacer = j === 8 ? '  ' : ' ';
      return spacer + b;
    }).join('');
    rows.push({
      offset: i,
      hex: hexStr,
      ascii: getAsciiPreview(i),
    });
  }

  return (
    <View style={styles.container}>
      <TextInput
        style={styles.input}
        value={value}
        onChangeText={handleTextChange}
        multiline
        placeholder="Enter hex data (e.g., 27 00 00 00 ...)"
        placeholderTextColor="#444"
        autoCapitalize="characters"
        spellCheck={false}
      />
      {bytes.length > 0 && (
        <ScrollView style={styles.preview} horizontal>
          <View>
            <View style={styles.headerRow}>
              <Text style={styles.offsetHeader}>OFFSET</Text>
              <Text style={styles.hexHeader}>00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F</Text>
              <Text style={styles.asciiHeader}>ASCII</Text>
            </View>
            {rows.map((row, ri) => (
              <View key={ri} style={styles.dataRow}>
                <Text style={styles.offsetText}>{row.offset.toString(16).padStart(8, '0')}</Text>
                <Text style={styles.hexText}>{row.hex}</Text>
                <Text style={styles.asciiText}>{row.ascii}</Text>
              </View>
            ))}
          </View>
        </ScrollView>
      )}
      <View style={styles.stats}>
        <Text style={styles.statsText}>
          {bytes.length} bytes | FIS type: 0x{bytes.length > 0 ? bytes[0] : '??'}
          {bytes.length >= 20 ? ' | Valid FIS header' : ' | Too short'}
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { marginVertical: 8 },
  input: {
    backgroundColor: '#0a0a2a', borderRadius: 8, padding: 12,
    color: '#00ff88', fontSize: 14, fontFamily: 'monospace',
    borderWidth: 1, borderColor: '#2a2a5a', minHeight: 80,
    textAlignVertical: 'top',
  },
  preview: { maxHeight: 300, marginTop: 8 },
  headerRow: { flexDirection: 'row', marginBottom: 4 },
  offsetHeader: { color: '#555', fontSize: 10, width: 70, fontFamily: 'monospace' },
  hexHeader: { color: '#555', fontSize: 10, width: 310, fontFamily: 'monospace' },
  asciiHeader: { color: '#555', fontSize: 10, width: 80, fontFamily: 'monospace', marginLeft: 8 },
  dataRow: { flexDirection: 'row', marginBottom: 2 },
  offsetText: { color: '#888', fontSize: 11, width: 70, fontFamily: 'monospace' },
  hexText: { color: '#0f0', fontSize: 11, fontFamily: 'monospace', width: 310 },
  asciiText: { color: '#aaa', fontSize: 11, fontFamily: 'monospace', width: 80, marginLeft: 8 },
  stats: { marginTop: 4 },
  statsText: { color: '#555', fontSize: 11, fontFamily: 'monospace' },
});

export default HexEditor;
