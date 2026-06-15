/**
 * DashboardScreen — Main BLE Phantom status and control screen
 *
 * Shows device connection status, current mode, radio status,
 * and provides quick actions for common tasks.
 */

import React, { useState, useEffect, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Switch,
  Alert,
} from 'react-native';
import { ConnectionContext } from '../components/ConnectionContext';

const MODES = [
  { id: 0, name: 'Idle', color: '#666666' },
  { id: 1, name: 'Sniff', color: '#00AAFF' },
  { id: 2, name: 'Scan', color: '#FFAA00' },
  { id: 3, name: 'Advertise', color: '#FF00FF' },
  { id: 4, name: 'Connect', color: '#00FF88' },
  { id: 5, name: 'MITM', color: '#FF3333' },
  { id: 6, name: 'Replay', color: '#FFFF00' },
];

export default function DashboardScreen({ navigation }) {
  const { connected, deviceStatus, sendCommand } = useContext(ConnectionContext);
  const [currentMode, setCurrentMode] = useState(0);
  const [radioAStatus, setRadioAStatus] = useState(0);
  const [radioBStatus, setRadioBStatus] = useState(0);
  const [autoConnect, setAutoConnect] = useState(true);

  useEffect(() => {
    if (connected) {
      const interval = setInterval(() => {
        sendCommand([0xA5]) // CMD_HOST_STATUS
          .then(response => {
            if (response && response.length >= 4) {
              setCurrentMode(response[1]);
              setRadioAStatus(response[2]);
              setRadioBStatus(response[3]);
            }
          })
          .catch(() => {});
      }, 1000);
      return () => clearInterval(interval);
    }
  }, [connected]);

  const handleModeChange = (modeId) => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to BLE Phantom first');
      return;
    }
    sendCommand([0xA0, modeId]) // CMD_HOST_SET_MODE
      .then(response => {
        if (response && response[1] === 0x00) {
          setCurrentMode(modeId);
        }
      })
      .catch(err => Alert.alert('Error', err.message));
  };

  const getModeName = () => MODES[currentMode]?.name || 'Unknown';
  const getModeColor = () => MODES[currentMode]?.color || '#666666';

  const getStatusFlags = (flags) => {
    const names = [];
    if (flags & 0x01) names.push('Init');
    if (flags & 0x02) names.push('Scanning');
    if (flags & 0x04) names.push('Adv');
    if (flags & 0x08) names.push('Conn');
    if (flags & 0x10) names.push('RX');
    if (flags & 0x20) names.push('TX Done');
    if (flags & 0x80) names.push('Error');
    return names.length > 0 ? names.join(', ') : 'Idle';
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection Status Banner */}
      <View style={[styles.statusBanner, { backgroundColor: connected ? '#003322' : '#331111' }]}>
        <Text style={styles.statusText}>
          {connected ? '● CONNECTED' : '○ DISCONNECTED'}
        </Text>
      </View>

      {/* Current Mode Display */}
      <View style={styles.modeCard}>
        <Text style={styles.cardTitle}>Current Mode</Text>
        <Text style={[styles.modeText, { color: getModeColor() }]}>
          {getModeName()}
        </Text>
      </View>

      {/* Mode Selection Grid */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Select Mode</Text>
        <View style={styles.modeGrid}>
          {MODES.map(mode => (
            <TouchableOpacity
              key={mode.id}
              style={[
                styles.modeButton,
                currentMode === mode.id && styles.modeButtonActive,
                { borderColor: mode.color },
              ]}
              onPress={() => handleModeChange(mode.id)}
            >
              <Text style={[styles.modeButtonText, { color: mode.color }]}>
                {mode.name}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Radio Status Cards */}
      <View style={styles.radioRow}>
        <View style={[styles.radioCard, styles.radioACard]}>
          <Text style={styles.radioTitle}>Radio A</Text>
          <Text style={styles.radioStatus}>
            {getStatusFlags(radioAStatus)}
          </Text>
          <View style={[styles.radioIndicator, { backgroundColor: radioAStatus ? '#00FF88' : '#333333' }]} />
        </View>
        <View style={[styles.radioCard, styles.radioBCard]}>
          <Text style={styles.radioTitle}>Radio B</Text>
          <Text style={styles.radioStatus}>
            {getStatusFlags(radioBStatus)}
          </Text>
          <View style={[styles.radioIndicator, { backgroundColor: radioBStatus ? '#00FF88' : '#333333' }]} />
        </View>
      </View>

      {/* Quick Actions */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Quick Actions</Text>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('Sniffer')}
        >
          <Text style={styles.actionButtonText}>▶ Start Sniffing</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('MITM')}
        >
          <Text style={styles.actionButtonText}>⚡ Start MITM</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => {
            if (connected) sendCommand([0xFF]); // CMD_RESET
          }}
        >
          <Text style={[styles.actionButtonText, styles.dangerText]}>↻ Reset Device</Text>
        </TouchableOpacity>
      </View>

      {/* Auto-connect toggle */}
      <View style={styles.card}>
        <View style={styles.toggleRow}>
          <Text style={styles.toggleLabel}>Auto-connect on USB</Text>
          <Switch
            value={autoConnect}
            onValueChange={setAutoConnect}
            trackColor={{ false: '#333333', true: '#006644' }}
            thumbColor={autoConnect ? '#00FF88' : '#666666'}
          />
        </View>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D0D1A',
  },
  statusBanner: {
    padding: 12,
    alignItems: 'center',
  },
  statusText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  modeCard: {
    backgroundColor: '#1A1A2E',
    margin: 8,
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
  },
  cardTitle: {
    color: '#AAAAAA',
    fontSize: 12,
    fontFamily: 'monospace',
    marginBottom: 8,
  },
  modeText: {
    fontSize: 28,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  card: {
    backgroundColor: '#1A1A2E',
    margin: 8,
    padding: 16,
    borderRadius: 8,
  },
  modeGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  modeButton: {
    width: '30%',
    padding: 10,
    marginVertical: 4,
    borderWidth: 1,
    borderRadius: 6,
    alignItems: 'center',
    backgroundColor: '#111122',
  },
  modeButtonActive: {
    backgroundColor: '#222244',
    borderWidth: 2,
  },
  modeButtonText: {
    fontSize: 12,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  radioRow: {
    flexDirection: 'row',
    margin: 8,
  },
  radioCard: {
    flex: 1,
    backgroundColor: '#1A1A2E',
    padding: 12,
    borderRadius: 8,
    marginHorizontal: 4,
    alignItems: 'center',
  },
  radioACard: {
    borderLeftWidth: 3,
    borderLeftColor: '#00AAFF',
  },
  radioBCard: {
    borderLeftWidth: 3,
    borderLeftColor: '#FFAA00',
  },
  radioTitle: {
    color: '#FFFFFF',
    fontSize: 14,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  radioStatus: {
    color: '#AAAAAA',
    fontSize: 10,
    fontFamily: 'monospace',
    marginVertical: 4,
  },
  radioIndicator: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginVertical: 4,
  },
  actionButton: {
    backgroundColor: '#222244',
    padding: 12,
    borderRadius: 6,
    marginVertical: 4,
    alignItems: 'center',
  },
  actionButtonText: {
    color: '#00FF88',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  dangerText: {
    color: '#FF4444',
  },
  toggleRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  toggleLabel: {
    color: '#CCCCCC',
    fontSize: 14,
  },
});