/**
 * SettingsScreen — Device configuration and firmware update
 *
 * Allows configuring radio parameters, viewing device info,
 * and performing firmware updates.
 */

import React, { useState, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  ScrollView,
  Switch,
  Alert,
} from 'react-native';
import { ConnectionContext } from '../components/ConnectionContext';

export default function SettingsScreen() {
  const { connected, sendCommand, deviceInfo } = useContext(ConnectionContext);
  const [radioATxPower, setRadioATxPower] = useState('4');
  const [radioBTxPower, setRadioBTxPower] = useState('4');
  const [radioAPhy, setRadioAPhy] = useState('1');
  const [radioBPhy, setRadioBPhy] = useState('1');
  const [scanInterval, setScanInterval] = useState('16');
  const [scanWindow, setScanWindow] = useState('16');
  const [advInterval, setAdvInterval] = useState('160');

  const applyRadioConfig = (radio) => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to BLE Phantom first');
      return;
    }
    const txPwr = parseInt(radio === 0 ? radioATxPower : radioBTxPower);
    const phy = parseInt(radio === 0 ? radioAPhy : radioBPhy);
    const channel = radio === 0 ? 37 : 38;

    sendCommand([
      0xA1, radio, channel,
      txPwr & 0xFF,
      (parseInt(advInterval) >> 8) & 0xFF, parseInt(advInterval) & 0xFF,
      (parseInt(scanWindow) >> 8) & 0xFF, parseInt(scanWindow) & 0xFF,
      (parseInt(scanInterval) >> 8) & 0xFF, parseInt(scanInterval) & 0xFF,
      phy,
    ])
    .then(() => Alert.alert('Success', `Radio ${radio === 0 ? 'A' : 'B'} configured`))
    .catch(err => Alert.alert('Error', err.message));
  };

  const factoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will reset the device to default settings. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Reset', style: 'destructive', onPress: () => {
          sendCommand([0xFF]); // CMD_RESET
        }},
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* Device Info */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device Information</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Connection</Text>
          <Text style={[styles.infoValue, { color: connected ? '#00FF88' : '#FF4444' }]}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Device</Text>
          <Text style={styles.infoValue}>BLE Phantom v1.0</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Serial</Text>
          <Text style={styles.infoValue}>{deviceInfo?.serial || 'N/A'}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Firmware</Text>
          <Text style={styles.infoValue}>{deviceInfo?.firmware || 'N/A'}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Radio A</Text>
          <Text style={styles.infoValue}>{deviceInfo?.radioA || 'nRF52832'}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Radio B</Text>
          <Text style={styles.infoValue}>{deviceInfo?.radioB || 'nRF52832'}</Text>
        </View>
      </View>

      {/* Radio A Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Radio A Configuration</Text>
        <Text style={styles.label}>TX Power (dBm): -40 to +4</Text>
        <TextInput
          style={styles.input}
          placeholder="4"
          placeholderTextColor="#555555"
          value={radioATxPower}
          onChangeText={setRadioATxPower}
          keyboardType="number-pad"
        />
        <Text style={styles.label}>PHY: 1=1M, 2=2M, 3=Coded</Text>
        <TextInput
          style={styles.input}
          placeholder="1"
          placeholderTextColor="#555555"
          value={radioAPhy}
          onChangeText={setRadioAPhy}
          keyboardType="number-pad"
        />
        <TouchableOpacity style={styles.applyButton} onPress={() => applyRadioConfig(0)}>
          <Text style={styles.applyButtonText}>Apply to Radio A</Text>
        </TouchableOpacity>
      </View>

      {/* Radio B Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Radio B Configuration</Text>
        <Text style={styles.label}>TX Power (dBm): -40 to +4</Text>
        <TextInput
          style={styles.input}
          placeholder="4"
          placeholderTextColor="#555555"
          value={radioBTxPower}
          onChangeText={setRadioBTxPower}
          keyboardType="number-pad"
        />
        <Text style={styles.label}>PHY: 1=1M, 2=2M, 3=Coded</Text>
        <TextInput
          style={styles.input}
          placeholder="1"
          placeholderTextColor="#555555"
          value={radioBPhy}
          onChangeText={setRadioBPhy}
          keyboardType="number-pad"
        />
        <TouchableOpacity style={styles.applyButton} onPress={() => applyRadioConfig(1)}>
          <Text style={styles.applyButtonText}>Apply to Radio B</Text>
        </TouchableOpacity>
      </View>

      {/* Scan/Advertising Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Scan & Advertising</Text>
        <Text style={styles.label}>Scan Interval (×0.625 ms)</Text>
        <TextInput
          style={styles.input}
          placeholder="16"
          placeholderTextColor="#555555"
          value={scanInterval}
          onChangeText={setScanInterval}
          keyboardType="number-pad"
        />
        <Text style={styles.label}>Scan Window (×0.625 ms)</Text>
        <TextInput
          style={styles.input}
          placeholder="16"
          placeholderTextColor="#555555"
          value={scanWindow}
          onChangeText={setScanWindow}
          keyboardType="number-pad"
        />
        <Text style={styles.label}>Advertising Interval (×0.625 ms)</Text>
        <TextInput
          style={styles.input}
          placeholder="160"
          placeholderTextColor="#555555"
          value={advInterval}
          onChangeText={setAdvInterval}
          keyboardType="number-pad"
        />
      </View>

      {/* Dangerous Actions */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Danger Zone</Text>
        <TouchableOpacity style={styles.dangerButton} onPress={factoryReset}>
          <Text style={styles.dangerButtonText}>Factory Reset</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D0D1A',
  },
  card: {
    backgroundColor: '#1A1A2E',
    margin: 8,
    padding: 16,
    borderRadius: 8,
  },
  cardTitle: {
    color: '#AAAAAA',
    fontSize: 12,
    fontFamily: 'monospace',
    marginBottom: 10,
    textTransform: 'uppercase',
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 3,
  },
  infoLabel: {
    color: '#888888',
    fontSize: 13,
    fontFamily: 'monospace',
  },
  infoValue: {
    color: '#CCCCCC',
    fontSize: 13,
    fontFamily: 'monospace',
  },
  label: {
    color: '#CCCCCC',
    fontSize: 12,
    fontFamily: 'monospace',
    marginTop: 6,
    marginBottom: 2,
  },
  input: {
    backgroundColor: '#222244',
    color: '#00FF88',
    padding: 10,
    borderRadius: 6,
    fontFamily: 'monospace',
    fontSize: 14,
    marginBottom: 4,
  },
  applyButton: {
    backgroundColor: '#004422',
    padding: 10,
    borderRadius: 6,
    marginTop: 8,
    alignItems: 'center',
  },
  applyButtonText: {
    color: '#00FF88',
    fontFamily: 'monospace',
    fontSize: 13,
    fontWeight: 'bold',
  },
  dangerButton: {
    backgroundColor: '#440000',
    padding: 10,
    borderRadius: 6,
    marginTop: 4,
    alignItems: 'center',
  },
  dangerButtonText: {
    color: '#FF4444',
    fontFamily: 'monospace',
    fontSize: 13,
    fontWeight: 'bold',
  },
});