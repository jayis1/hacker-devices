/**
 * MemoryViewScreen.tsx — Target Memory Explorer
 * Author: jayis1
 * License: MIT
 *
 * Interactive hex viewer for reading and writing target memory.
 * Supports arbitrary address access, byte/word/dword sizes,
 * and hexdump export for flash extraction analysis.
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  TextInput,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Alert,
  Clipboard,
  Share,
} from 'react-native';
import forgeProbeService from '../services/ForgeProbeService';

const MemoryViewScreen: React.FC = () => {
  const [address, setAddress] = useState('08000000');
  const [length, setLength] = useState('64');
  const [data, setData] = useState<number[]>([]);
  const [loading, setLoading] = useState(false);
  const [accessSize, setAccessSize] = useState<'8' | '16' | '32'>('32');
  const [writeValue, setWriteValue] = useState('');

  const readMemory = useCallback(async () => {
    const addr = parseInt(address, 16);
    const len = parseInt(length, 10);

    if (isNaN(addr) || isNaN(len) || len > 4096) {
      Alert.alert('Invalid Input', 'Address must be hex, length max 4096');
      return;
    }

    setLoading(true);
    try {
      const raw = await forgeProbeService.readMemory(addr, len);
      const bytes: number[] = [];
      for (let i = 0; i < raw.length; i++) {
        bytes.push(raw[i]);
      }
      setData(bytes);
    } catch (error) {
      Alert.alert('Read Failed', 'Could not read memory at this address');
    } finally {
      setLoading(false);
    }
  }, [address, length]);

  const writeMemory = useCallback(async () => {
    const addr = parseInt(address, 16);
    const val = parseInt(writeValue, 16);

    if (isNaN(addr) || isNaN(val)) {
      Alert.alert('Invalid', 'Address or value invalid');
      return;
    }

    Alert.alert(
      'Confirm Write',
      `Write 0x${val.toString(16).toUpperCase()} to 0x${addr.toString(16)}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Write',
          style: 'destructive',
          onPress: async () => {
            try {
              const payload = new Uint8Array(8);
              payload.set([
                (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
                (addr >> 8) & 0xFF, addr & 0xFF,
                (val >> 24) & 0xFF, (val >> 16) & 0xFF,
                (val >> 8) & 0xFF, val & 0xFF,
              ]);
              // Use raw command for write
              await forgeProbeService.sendCommand(0x03 as any, payload);
              Alert.alert('Written', `Value 0x${val.toString(16)} written`);
              await readMemory();
            } catch {
              Alert.alert('Error', 'Write failed');
            }
          },
        },
      ],
    );
  }, [address, writeValue, readMemory]);

  const exportHexdump = useCallback(async () => {
    const lines: string[] = [];
    const addrBase = parseInt(address, 16);
    for (let i = 0; i < data.length; i += 16) {
      const offset = addrBase + i;
      const hexBytes = data.slice(i, i + 16)
        .map((b) => b.toString(16).padStart(2, '0'))
        .join(' ');
      const ascii = data.slice(i, i + 16)
        .map((b) => (b >= 32 && b <= 126 ? String.fromCharCode(b) : '.'))
        .join('');
      lines.push(`${offset.toString(16).padStart(8, '0')}  ${hexBytes.padEnd(47)}  ${ascii}`);
    }

    const dump = lines.join('\n');
    await Share.share({ message: dump, title: 'Memory Dump' });
  }, [data, address]);

  const renderHexView = () => {
    if (data.length === 0) return null;

    const addrBase = parseInt(address, 16);
    const rows: JSX.Element[] = [];

    for (let i = 0; i < data.length; i += 16) {
      const rowAddr = addrBase + i;
      const hexBytes = data.slice(i, i + 16)
        .map((b) => b.toString(16).padStart(2, '0'))
        .join(' ');
      const ascii = data.slice(i, i + 16)
        .map((b) => (b >= 32 && b <= 126 ? String.fromCharCode(b) : '.'))
        .join('');

      rows.push(
        <View key={i} style={styles.hexRow}>
          <Text style={styles.hexAddr}>
            {rowAddr.toString(16).padStart(8, '0')}
          </Text>
          <Text style={styles.hexData}>{hexBytes}</Text>
          <Text style={styles.hexAscii}>{ascii}</Text>
        </View>,
      );
    }

    return rows;
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Memory Explorer</Text>

      {/* Address and Length Input */}
      <View style={styles.inputRow}>
        <View style={styles.inputGroup}>
          <Text style={styles.inputLabel}>Address (hex)</Text>
          <TextInput
            style={styles.input}
            value={address}
            onChangeText={setAddress}
            placeholder="08000000"
            placeholderTextColor="#555"
            autoCapitalize="none"
          />
        </View>
        <View style={styles.inputGroup}>
          <Text style={styles.inputLabel}>Length (bytes)</Text>
          <TextInput
            style={styles.input}
            value={length}
            onChangeText={setLength}
            placeholder="64"
            placeholderTextColor="#555"
            keyboardType="numeric"
          />
        </View>
      </View>

      {/* Access Size Selector */}
      <View style={styles.sizeRow}>
        {(['8', '16', '32'] as const).map((size) => (
          <TouchableOpacity
            key={size}
            style={[
              styles.sizeBtn,
              accessSize === size && styles.sizeBtnActive,
            ]}
            onPress={() => setAccessSize(size)}
          >
            <Text
              style={[
                styles.sizeBtnText,
                accessSize === size && styles.sizeBtnTextActive,
              ]}
            >
              {size}-bit
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.actionBtn, styles.readBtn]}
          onPress={readMemory}
          disabled={loading}
        >
          <Text style={styles.actionBtnText}>
            {loading ? 'Reading...' : 'Read'}
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.actionBtn, styles.exportBtn]}
          onPress={exportHexdump}
          disabled={data.length === 0}
        >
          <Text style={styles.actionBtnText}>Export</Text>
        </TouchableOpacity>
      </View>

      {/* Write Section */}
      <View style={styles.writeSection}>
        <Text style={styles.sectionTitle}>Write Value</Text>
        <View style={styles.writeRow}>
          <TextInput
            style={[styles.input, { flex: 1, marginRight: 8 }]}
            value={writeValue}
            onChangeText={setWriteValue}
            placeholder="Value (hex)"
            placeholderTextColor="#555"
            autoCapitalize="none"
          />
          <TouchableOpacity style={styles.writeBtn} onPress={writeMemory}>
            <Text style={styles.writeBtnText}>Write</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Hexdump Display */}
      <View style={styles.hexContainer}>
        {data.length > 0 ? (
          <View>
            <View style={styles.hexHeader}>
              <Text style={styles.hexHeaderAddr}>Address</Text>
              <Text style={styles.hexHeaderData}>00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F</Text>
              <Text style={styles.hexHeaderAscii}>ASCII</Text>
            </View>
            {renderHexView()}
          </View>
        ) : (
          <Text style={styles.emptyText}>
            {loading ? 'Reading...' : 'Read a memory region to view hexdump'}
          </Text>
        )}
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#121212', padding: 16 },
  title: { fontSize: 22, fontWeight: '700', color: '#E0E0E0', marginBottom: 16 },
  inputRow: { flexDirection: 'row', marginBottom: 12 },
  inputGroup: { flex: 1, marginRight: 8 },
  inputLabel: { fontSize: 12, color: '#888', marginBottom: 4 },
  input: {
    backgroundColor: '#1E1E1E',
    color: '#00E676',
    fontFamily: 'monospace',
    fontSize: 14,
    padding: 10,
    borderRadius: 8,
    borderColor: '#333',
    borderWidth: 1,
  },
  sizeRow: { flexDirection: 'row', marginBottom: 16, gap: 8 },
  sizeBtn: {
    flex: 1,
    padding: 10,
    borderRadius: 8,
    backgroundColor: '#2C2C2C',
    alignItems: 'center',
    borderColor: '#444',
    borderWidth: 1,
  },
  sizeBtnActive: { backgroundColor: '#2196F3', borderColor: '#2196F3' },
  sizeBtnText: { color: '#888', fontSize: 13, fontWeight: '600' },
  sizeBtnTextActive: { color: '#FFF' },
  actionRow: { flexDirection: 'row', marginBottom: 16, gap: 8 },
  actionBtn: { flex: 1, padding: 14, borderRadius: 8, alignItems: 'center' },
  readBtn: { backgroundColor: '#4CAF50' },
  exportBtn: { backgroundColor: '#FF9800' },
  actionBtnText: { color: '#FFF', fontSize: 15, fontWeight: '700' },
  writeSection: { marginBottom: 16 },
  sectionTitle: { fontSize: 14, color: '#AAA', marginBottom: 8, fontWeight: '600' },
  writeRow: { flexDirection: 'row', alignItems: 'center' },
  writeBtn: {
    backgroundColor: '#F44336',
    padding: 10,
    borderRadius: 8,
    paddingHorizontal: 20,
  },
  writeBtnText: { color: '#FFF', fontWeight: '700' },
  hexContainer: {
    backgroundColor: '#1A1A1A',
    borderRadius: 8,
    padding: 12,
    marginBottom: 32,
  },
  hexHeader: { flexDirection: 'row', marginBottom: 8, paddingBottom: 8, borderBottomWidth: 1, borderBottomColor: '#333' },
  hexHeaderAddr: { width: 72, fontSize: 10, color: '#888', fontFamily: 'monospace' },
  hexHeaderData: { flex: 1, fontSize: 10, color: '#888', fontFamily: 'monospace' },
  hexHeaderAscii: { width: 80, fontSize: 10, color: '#888', fontFamily: 'monospace' },
  hexRow: { flexDirection: 'row', marginVertical: 1 },
  hexAddr: { width: 72, fontSize: 11, color: '#888', fontFamily: 'monospace' },
  hexData: { flex: 1, fontSize: 11, color: '#00E676', fontFamily: 'monospace', letterSpacing: 0.5 },
  hexAscii: { width: 80, fontSize: 11, color: '#AAA', fontFamily: 'monospace' },
  emptyText: { color: '#555', textAlign: 'center', padding: 40, fontSize: 13 },
});

export default MemoryViewScreen;