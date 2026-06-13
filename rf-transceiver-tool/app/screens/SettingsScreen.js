/**
 * SettingsScreen.js — Device configuration screen
 *
 * Allows the user to configure CC1101 frequency, modulation, data rate,
 * TX power, and nRF24L01+ channel, data rate, address, and TX power.
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  Alert,
  Picker,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import {
  CMD,
  SUBGHZ_FREQS,
  NRF24_CHANNELS,
  CC1101_POWER,
  CC1101_DATARATE,
  MODULATION,
  NRF24_DATARATE,
  NRF24_POWER,
  buildPacket,
  encodeFrequency,
} from '../utils/protocol';

export default function SettingsScreen() {
  const navigation = useNavigation();

  // CC1101 Sub-GHz settings
  const [subghzFreq, setSubghzFreq] = useState('868000000');
  const [subghzModulation, setSubghzModulation] = useState('GFSK');
  const [subghzDatarate, setSubghzDatarate] = useState('1.2 kbps');
  const [subghzPower, setSubghzPower] = useState('0 dBm');
  const [subghzSyncWord, setSubghzSyncWord] = useState('D391');
  const [subghzChannel, setSubghzChannel] = useState('0');

  // nRF24L01+ 2.4 GHz settings
  const [nrf24Channel, setNrf24Channel] = useState('76');
  const [nrf24Datarate, setNrf24Datarate] = useState('2 Mbps');
  const [nrf24Power, setNrf24Power] = useState('0 dBm');
  const [nrf24AddrP0, setNrf24AddrP0] = useState('E7E7E7E7E7');
  const [nrf24AddrP1, setNrf24AddrP1] = useState('C2C2C2C2C2');
  const [nrf24PayloadWidth, setNrf24PayloadWidth] = useState('32');

  // Helper to send commands
  const sendCommand = useCallback((cmdId, payload = []) => {
    // This would use the serial connection from DeviceScreen context
    // For now, we construct the packet and log it
    const packet = buildPacket(cmdId, payload);
    console.log('Sending packet:', Array.from(packet).map(b => b.toString(16).padStart(2, '0')).join(' '));
    // In production: serialRef.current.write(Array.from(packet));
  }, []);

  const applySubGhzSettings = useCallback(() => {
    const freq = parseInt(subghzFreq, 10);
    if (isNaN(freq) || freq < 300000000 || freq > 928000000) {
      Alert.alert('Invalid Frequency', 'Sub-GHz frequency must be 300-928 MHz');
      return;
    }

    const freqBytes = encodeFrequency(freq);
    sendCommand(CMD.CC1101_SET_FREQ, freqBytes);

    const modValue = MODULATION[subghzModulation] || 0x10;
    sendCommand(CMD.CC1101_SET_MOD, [modValue]);

    const powerValue = CC1101_POWER[subghzPower] || 0x0C;
    sendCommand(CMD.CC1101_SET_POWER, [powerValue]);

    const syncInt = parseInt(subghzSyncWord, 16);
    if (!isNaN(syncInt)) {
      sendCommand(CMD.CC1101_SET_SYNC, [
        (syncInt >> 8) & 0xFF,
        syncInt & 0xFF,
      ]);
    }

    const ch = parseInt(subghzChannel, 10);
    if (!isNaN(ch)) {
      // Channel is encoded as part of config command
      sendCommand(CMD.CC1101_CONFIG, [ch & 0xFF]);
    }

    Alert.alert('Settings Applied', 'Sub-GHz configuration sent to device');
  }, [subghzFreq, subghzModulation, subghzPower, subghzSyncWord, subghzChannel, sendCommand]);

  const applyNrf24Settings = useCallback(() => {
    const ch = parseInt(nrf24Channel, 10);
    if (isNaN(ch) || ch < 0 || ch > 127) {
      Alert.alert('Invalid Channel', 'nRF24 channel must be 0-127');
      return;
    }

    sendCommand(CMD.NRF24_SET_CHANNEL, [ch]);

    const drValue = NRF24_DATARATE[nrf24Datarate];
    sendCommand(CMD.NRF24_SET_DATARATE, [drValue]);

    const pwValue = NRF24_POWER[nrf24Power];
    sendCommand(CMD.NRF24_SET_POWER, [pwValue]);

    // Address bytes (LSB first, hex string to byte array)
    const addrBytes = nrf24AddrP0.match(/.{2}/g)?.map(hex => parseInt(hex, 16)) || [];
    if (addrBytes.length >= 3 && addrBytes.length <= 5) {
      sendCommand(CMD.NRF24_SET_ADDR, [0x00, addrBytes.length, ...addrBytes]);
    }

    const pw = parseInt(nrf24PayloadWidth, 10);
    if (!isNaN(pw) && pw >= 1 && pw <= 32) {
      sendCommand(CMD.NRF24_SET_PIPE, [0x00, 0x01, pw & 0xFF]);
    }

    Alert.alert('Settings Applied', '2.4 GHz configuration sent to device');
  }, [nrf24Channel, nrf24Datarate, nrf24Power, nrf24AddrP0, nrf24PayloadWidth, sendCommand]);

  return (
    <ScrollView style={styles.container}>
      {/* CC1101 Sub-GHz Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CC1101 Sub-GHz Configuration</Text>

        <Text style={styles.label}>Frequency (Hz)</Text>
        <TextInput
          style={styles.input}
          value={subghzFreq}
          onChangeText={setSubghzFreq}
          keyboardType="numeric"
          placeholder="868000000"
        />
        <View style={styles.presetRow}>
          {Object.keys(SUBGHZ_FREQS).map((name) => (
            <TouchableOpacity
              key={name}
              style={styles.presetButton}
              onPress={() => setSubghzFreq(String(SUBGHZ_FREQS[name]))}
            >
              <Text style={styles.presetText}>{name}</Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.label}>Modulation</Text>
        <View style={styles.pickerContainer}>
          <Picker
            selectedValue={subghzModulation}
            onValueChange={setSubghzModulation}
            style={styles.picker}
            dropdownIconColor="#00ff88"
          >
            {Object.keys(MODULATION).map((name) => (
              <Picker.Item key={name} label={name} value={name} color="#e0e0e0" />
            ))}
          </Picker>
        </View>

        <Text style={styles.label}>Data Rate</Text>
        <View style={styles.pickerContainer}>
          <Picker
            selectedValue={subghzDatarate}
            onValueChange={setSubghzDatarate}
            style={styles.picker}
            dropdownIconColor="#00ff88"
          >
            {Object.keys(CC1101_DATARATE).map((name) => (
              <Picker.Item key={name} label={name} value={name} color="#e0e0e0" />
            ))}
          </Picker>
        </View>

        <Text style={styles.label}>TX Power</Text>
        <View style={styles.pickerContainer}>
          <Picker
            selectedValue={subghzPower}
            onValueChange={setSubghzPower}
            style={styles.picker}
            dropdownIconColor="#00ff88"
          >
            {Object.keys(CC1101_POWER).map((name) => (
              <Picker.Item key={name} label={name} value={name} color="#e0e0e0" />
            ))}
          </Picker>
        </View>

        <Text style={styles.label}>Sync Word (hex)</Text>
        <TextInput
          style={styles.input}
          value={subghzSyncWord}
          onChangeText={setSubghzSyncWord}
          placeholder="D391"
          autoCapitalize="characters"
        />

        <Text style={styles.label}>Channel Number</Text>
        <TextInput
          style={styles.input}
          value={subghzChannel}
          onChangeText={setSubghzChannel}
          keyboardType="numeric"
          placeholder="0"
        />

        <TouchableOpacity style={styles.applyButton} onPress={applySubGhzSettings}>
          <Text style={styles.applyButtonText}>Apply Sub-GHz Settings</Text>
        </TouchableOpacity>
      </View>

      {/* nRF24L01+ 2.4 GHz Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>nRF24L01+ 2.4 GHz Configuration</Text>

        <Text style={styles.label}>Channel (0-127)</Text>
        <TextInput
          style={styles.input}
          value={nrf24Channel}
          onChangeText={setNrf24Channel}
          keyboardType="numeric"
          placeholder="76"
        />
        <View style={styles.presetRow}>
          {Object.keys(NRF24_CHANNELS).map((name) => (
            <TouchableOpacity
              key={name}
              style={styles.presetButton}
              onPress={() => setNrf24Channel(String(NRF24_CHANNELS[name]))}
            >
              <Text style={styles.presetText}>{name}</Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.label}>Data Rate</Text>
        <View style={styles.pickerContainer}>
          <Picker
            selectedValue={nrf24Datarate}
            onValueChange={setNrf24Datarate}
            style={styles.picker}
            dropdownIconColor="#ff6b6b"
          >
            {Object.keys(NRF24_DATARATE).map((name) => (
              <Picker.Item key={name} label={name} value={name} color="#e0e0e0" />
            ))}
          </Picker>
        </View>

        <Text style={styles.label}>TX Power</Text>
        <View style={styles.pickerContainer}>
          <Picker
            selectedValue={nrf24Power}
            onValueChange={setNrf24Power}
            style={styles.picker}
            dropdownIconColor="#ff6b6b"
          >
            {Object.keys(NRF24_POWER).map((name) => (
              <Picker.Item key={name} label={name} value={name} color="#e0e0e0" />
            ))}
          </Picker>
        </View>

        <Text style={styles.label}>RX Address (Pipe 0, hex, 5 bytes)</Text>
        <TextInput
          style={styles.input}
          value={nrf24AddrP0}
          onChangeText={setNrf24AddrP0}
          placeholder="E7E7E7E7E7"
          autoCapitalize="characters"
        />

        <Text style={styles.label}>Payload Width (1-32)</Text>
        <TextInput
          style={styles.input}
          value={nrf24PayloadWidth}
          onChangeText={setNrf24PayloadWidth}
          keyboardType="numeric"
          placeholder="32"
        />

        <TouchableOpacity style={[styles.applyButton, styles.applyNrf24]} onPress={applyNrf24Settings}>
          <Text style={styles.applyButtonText}>Apply 2.4 GHz Settings</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
    padding: 8,
  },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 16,
    marginBottom: 12,
  },
  sectionTitle: {
    color: '#00ff88',
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  label: {
    color: '#e0e0e0',
    fontSize: 14,
    marginTop: 8,
    marginBottom: 4,
  },
  input: {
    backgroundColor: '#0f3460',
    borderRadius: 6,
    padding: 10,
    color: '#ffffff',
    fontSize: 14,
    marginBottom: 4,
  },
  presetRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginBottom: 8,
  },
  presetButton: {
    backgroundColor: '#0f3460',
    borderRadius: 4,
    paddingHorizontal: 8,
    paddingVertical: 4,
    marginRight: 4,
    marginBottom: 4,
  },
  presetText: {
    color: '#4ecdc4',
    fontSize: 11,
  },
  pickerContainer: {
    backgroundColor: '#0f3460',
    borderRadius: 6,
    marginBottom: 4,
  },
  picker: {
    color: '#ffffff',
    height: 40,
  },
  applyButton: {
    backgroundColor: '#00ff88',
    borderRadius: 8,
    paddingVertical: 12,
    alignItems: 'center',
    marginTop: 12,
  },
  applyNrf24: {
    backgroundColor: '#ff6b6b',
  },
  applyButtonText: {
    color: '#1a1a2e',
    fontWeight: 'bold',
    fontSize: 16,
  },
});