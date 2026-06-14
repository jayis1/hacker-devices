/**
 * InjectScreen.js — CAN frame injection tool
 * Craft and send individual CAN frames or replay captured sequences
 */

import React, { useState } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Switch,
  Alert,
} from 'react-native';
import { BLE_PROTOCOL } from '../utils/protocol';

const PRESET_FRAMES = [
  { name: 'OBD-II PID Request', id: '7DF', dlc: 8, data: '02 01 00 00 00 00 00 00', idType: 0 },
  { name: 'OBD-II PID Response (RPM)', id: '7E8', dlc: 8, data: '04 41 0C 1A 98 00 00 00', idType: 0 },
  { name: 'ISO-TP First Frame', id: '7E0', dlc: 8, data: '10 0A 49 04 01 2C 30 00', idType: 0 },
  { name: 'UDL Diagnostic Session', id: '7E0', dlc: 8, data: '02 10 03 00 00 00 00 00', idType: 0 },
  { name: 'Door Unlock (test)', id: '3B1', dlc: 8, data: '05 30 01 00 00 00 00 00', idType: 0 },
];

export default function InjectScreen() {
  const [canId, setCanId] = useState('7DF');
  const [idType, setIdType] = useState(0); // 0=std, 1=ext
  const [dlc, setDlc] = useState('8');
  const [data, setData] = useState('02 01 00 00 00 00 00 00');
  const [rtr, setRtr] = useState(false);
  const [channel, setChannel] = useState(1);
  const [repeatCount, setRepeatCount] = useState('1');
  const [repeatDelay, setRepeatDelay] = useState('100');

  const sendFrame = async () => {
    try {
      const id = parseInt(canId, 16);
      if (isNaN(id)) { Alert.alert('Error', 'Invalid CAN ID'); return; }

      const dlcVal = parseInt(dlc);
      if (dlcVal < 1 || dlcVal > 8) { Alert.alert('Error', 'DLC must be 1-8'); return; }

      const dataBytes = data.trim().split(/\s+/).map(b => parseInt(b, 16));
      if (dataBytes.some(isNaN)) { Alert.alert('Error', 'Invalid data bytes'); return; }

      // Pad or truncate to 8 bytes
      while (dataBytes.length < 8) dataBytes.push(0);
      const dataTruncated = dataBytes.slice(0, 8);

      // Build command packet
      const cmd = new Uint8Array(13);
      cmd[0] = id & 0xFF;
      cmd[1] = (id >> 8) & 0xFF;
      cmd[2] = (id >> 16) & 0xFF;
      cmd[3] = (id >> 24) & 0xFF;
      cmd[4] = ((idType & 1) << 7) | ((rtr ? 1 : 0) << 6) | (channel & 1) << 2 | (dlcVal & 0x0F);
      cmd[5] = (channel & 1);
      for (let i = 0; i < 8; i++) cmd[6 + i] = dataTruncated[i] & 0xFF;

      const count = parseInt(repeatCount) || 1;
      const delay = parseInt(repeatDelay) || 100;

      for (let i = 0; i < count; i++) {
        await BLE_PROTOCOL.sendCommand(0x03, cmd);
        if (count > 1 && i < count - 1) {
          await new Promise(resolve => setTimeout(resolve, delay));
        }
      }

      Alert.alert('Sent', `${count} frame(s) injected on CH${channel + 1}`);
    } catch (e) {
      Alert.alert('Error', `Injection failed: ${e.message}`);
    }
  };

  const loadPreset = (preset) => {
    setCanId(preset.id);
    setIdType(preset.idType);
    setDlc(String(preset.dlc));
    setData(preset.data);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>CAN Frame Injection</Text>

      {/* CAN ID */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>CAN ID (hex):</Text>
        <TextInput
          style={styles.input}
          value={canId}
          onChangeText={setCanId}
          placeholder="e.g. 7DF"
          placeholderTextColor="#666"
          autoCapitalize="characters"
        />
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Extended (29-bit)</Text>
          <Switch value={idType === 1} onValueChange={v => setIdType(v ? 1 : 0)} />
        </View>
      </View>

      {/* DLC */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>DLC:</Text>
        <TextInput
          style={[styles.input, { width: 60 }]}
          value={dlc}
          onChangeText={setDlc}
          keyboardType="numeric"
        />
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>RTR</Text>
          <Switch value={rtr} onValueChange={setRtr} />
        </View>
      </View>

      {/* Data */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>Data (hex):</Text>
      </View>
      <TextInput
        style={[styles.input, styles.dataInput]}
        value={data}
        onChangeText={setData}
        placeholder="02 01 00 00 00 00 00 00"
        placeholderTextColor="#666"
        autoCapitalize="characters"
      />

      {/* Channel */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>Channel:</Text>
        <TouchableOpacity
          style={[styles.chButton, channel === 0 ? styles.chActive : null]}
          onPress={() => setChannel(0)}
        >
          <Text style={styles.chText}>CH1</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.chButton, channel === 1 ? styles.chActive : null]}
          onPress={() => setChannel(1)}
        >
          <Text style={styles.chText}>CH2</Text>
        </TouchableOpacity>
      </View>

      {/* Repeat */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>Repeat:</Text>
        <TextInput
          style={[styles.input, { width: 60 }]}
          value={repeatCount}
          onChangeText={setRepeatCount}
          keyboardType="numeric"
        />
        <Text style={styles.label}>Delay (ms):</Text>
        <TextInput
          style={[styles.input, { width: 80 }]}
          value={repeatDelay}
          onChangeText={setRepeatDelay}
          keyboardType="numeric"
        />
      </View>

      {/* Send Button */}
      <TouchableOpacity style={styles.sendButton} onPress={sendFrame}>
        <Text style={styles.sendButtonText}>⚡ INJECT FRAME</Text>
      </TouchableOpacity>

      {/* Presets */}
      <Text style={styles.subtitle}>Preset Frames</Text>
      {PRESET_FRAMES.map((preset, idx) => (
        <TouchableOpacity key={idx} style={styles.presetButton} onPress={() => loadPreset(preset)}>
          <Text style={styles.presetName}>{preset.name}</Text>
          <Text style={styles.presetDetail}>
            {preset.id} [{preset.dlc}] {preset.data}
          </Text>
        </TouchableOpacity>
      ))}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0D1117', padding: 12 },
  title: { color: '#E6EDF3', fontSize: 18, fontWeight: 'bold', marginBottom: 16 },
  subtitle: { color: '#8B949E', fontSize: 14, fontWeight: 'bold', marginTop: 16, marginBottom: 8 },
  fieldRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  label: { color: '#C9D1D9', fontSize: 13, marginRight: 8 },
  input: {
    backgroundColor: '#161B22',
    color: '#E6EDF3',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    fontSize: 13,
    borderWidth: 1,
    borderColor: '#30363D',
    flex: 1,
  },
  dataInput: { fontFamily: 'Courier', marginBottom: 12 },
  switchRow: { flexDirection: 'row', alignItems: 'center', marginLeft: 12 },
  switchLabel: { color: '#8B949E', fontSize: 12, marginRight: 4 },
  chButton: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    backgroundColor: '#21262D',
    borderRadius: 6,
    marginHorizontal: 4,
  },
  chActive: { backgroundColor: '#1F6FEB' },
  chText: { color: '#C9D1D9', fontSize: 13 },
  sendButton: {
    backgroundColor: '#DA3633',
    borderRadius: 8,
    paddingVertical: 14,
    alignItems: 'center',
    marginTop: 16,
    marginBottom: 8,
  },
  sendButtonText: { color: '#FFFFFF', fontSize: 16, fontWeight: 'bold' },
  presetButton: {
    backgroundColor: '#161B22',
    borderRadius: 6,
    padding: 10,
    marginBottom: 6,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  presetName: { color: '#79C0FF', fontSize: 13, fontWeight: 'bold' },
  presetDetail: { color: '#8B949E', fontSize: 11, fontFamily: 'Courier', marginTop: 2 },
});