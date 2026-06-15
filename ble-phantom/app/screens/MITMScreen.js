/**
 * MITMScreen — BLE Man-in-the-Middle configuration and control
 *
 * Configures and controls the dual-radio MITM relay:
 * - Radio A connects to the target peripheral
 * - Radio B connects to the target central
 * - Packets are relayed between the two, with optional modification
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

export default function MITMScreen() {
  const { connected, sendCommand } = useContext(ConnectionContext);
  const [targetAddr, setTargetAddr] = useState('');
  const [centralAddr, setCentralAddr] = useState('');
  const [relayActive, setRelayActive] = useState(false);
  const [modifyPackets, setModifyPackets] = useState(false);
  const [dropPackets, setDropPackets] = useState(false);
  const [logAllTraffic, setLogAllTraffic] = useState(true);
  const [radioAChannel, setRadioAChannel] = useState('37');
  const [radioBChannel, setRadioBChannel] = useState('38');
  const [relayCount, setRelayCount] = useState(0);
  const [errorCount, setErrorCount] = useState(0);

  const startMITM = () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to BLE Phantom first');
      return;
    }
    if (!targetAddr) {
      Alert.alert('Target Required', 'Enter the target peripheral address');
      return;
    }

    // Set MITM mode
    sendCommand([0xA0, 0x05]) // CMD_HOST_SET_MODE = MITM
      .then(() => {
        // Configure Radio A to connect to peripheral
        const addrBytes = parseAddress(targetAddr);
        if (addrBytes) {
          sendCommand([0xA1, 0x00, parseInt(radioAChannel), 4, 0xA0, 0x00, 0x10, 0x00, 0x10, 0x00, 0x01]);
          sendCommand([0xA4, 0x00, ...addrBytes, 0x00]); // Connect Radio A
        }

        // Configure Radio B to connect to central
        if (centralAddr) {
          const centralBytes = parseAddress(centralAddr);
          if (centralBytes) {
            sendCommand([0xA1, 0x01, parseInt(radioBChannel), 4, 0xA0, 0x00, 0x10, 0x00, 0x10, 0x00, 0x01]);
            sendCommand([0xA4, 0x01, ...centralBytes, 0x01]); // Connect Radio B
          }
        }

        setRelayActive(true);
        Alert.alert('MITM Active', 'Dual-radio relay is now active');
      })
      .catch(err => Alert.alert('Error', err.message));
  };

  const stopMITM = () => {
    sendCommand([0xA0, 0x00]); // Set mode to IDLE
    sendCommand([0xA3, 0x00]); // Stop Radio A
    sendCommand([0xA3, 0x01]); // Stop Radio B
    setRelayActive(false);
    setRelayCount(0);
    setErrorCount(0);
  };

  const parseAddress = (addrStr) => {
    try {
      const bytes = addrStr.replace(/[:-]/g, '').match(/.{2}/g);
      if (bytes && bytes.length === 6) {
        return bytes.map(b => parseInt(b, 16));
      }
    } catch (e) {}
    return null;
  };

  return (
    <ScrollView style={styles.container}>
      {/* Status Banner */}
      <View style={[styles.statusBanner, relayActive ? styles.activeBanner : styles.inactiveBanner]}>
        <Text style={styles.statusText}>
          {relayActive ? '⚡ MITM RELAY ACTIVE' : '○ MITM INACTIVE'}
        </Text>
      </View>

      {/* Target Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Target Configuration</Text>
        <Text style={styles.label}>Peripheral Address (Radio A →)</Text>
        <TextInput
          style={styles.input}
          placeholder="AA:BB:CC:DD:EE:FF"
          placeholderTextColor="#555555"
          value={targetAddr}
          onChangeText={setTargetAddr}
          autoCapitalize="none"
          editable={!relayActive}
        />
        <Text style={styles.label}>Central Address (← Radio B)</Text>
        <TextInput
          style={styles.input}
          placeholder="11:22:33:44:55:66"
          placeholderTextColor="#555555"
          value={centralAddr}
          onChangeText={setCentralAddr}
          autoCapitalize="none"
          editable={!relayActive}
        />
      </View>

      {/* Channel Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Channel Configuration</Text>
        <View style={styles.row}>
          <View style={styles.halfRow}>
            <Text style={styles.label}>Radio A Channel</Text>
            <TextInput
              style={styles.smallInput}
              placeholder="37"
              placeholderTextColor="#555555"
              value={radioAChannel}
              onChangeText={setRadioAChannel}
              keyboardType="number-pad"
              editable={!relayActive}
            />
          </View>
          <View style={styles.halfRow}>
            <Text style={styles.label}>Radio B Channel</Text>
            <TextInput
              style={styles.smallInput}
              placeholder="38"
              placeholderTextColor="#555555"
              value={radioBChannel}
              onChangeText={setRadioBChannel}
              keyboardType="number-pad"
              editable={!relayActive}
            />
          </View>
        </View>
      </View>

      {/* Relay Options */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Relay Options</Text>
        <View style={styles.toggleRow}>
          <Text style={styles.toggleLabel}>Modify Packets</Text>
          <Switch
            value={modifyPackets}
            onValueChange={setModifyPackets}
            trackColor={{ false: '#333333', true: '#664400' }}
            thumbColor={modifyPackets ? '#FFAA00' : '#666666'}
            disabled={relayActive}
          />
        </View>
        <View style={styles.toggleRow}>
          <Text style={styles.toggleLabel}>Drop Selected Packets</Text>
          <Switch
            value={dropPackets}
            onValueChange={setDropPackets}
            trackColor={{ false: '#333333', true: '#664400' }}
            thumbColor={dropPackets ? '#FFAA00' : '#666666'}
            disabled={relayActive}
          />
        </View>
        <View style={styles.toggleRow}>
          <Text style={styles.toggleLabel}>Log All Traffic</Text>
          <Switch
            value={logAllTraffic}
            onValueChange={setLogAllTraffic}
            trackColor={{ false: '#333333', true: '#006644' }}
            thumbColor={logAllTraffic ? '#00FF88' : '#666666'}
          />
        </View>
      </View>

      {/* Control Buttons */}
      <View style={styles.buttonRow}>
        <TouchableOpacity
          style={[styles.startButton, relayActive && styles.disabledButton]}
          onPress={startMITM}
          disabled={relayActive}
        >
          <Text style={styles.startButtonText}>
            {relayActive ? 'RELAYING...' : '▶ START MITM'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.stopButton, !relayActive && styles.disabledButton]}
          onPress={stopMITM}
          disabled={!relayActive}
        >
          <Text style={styles.stopButtonText}>■ STOP</Text>
        </TouchableOpacity>
      </View>

      {/* Statistics */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Statistics</Text>
        <View style={styles.statsRow}>
          <View style={styles.statBox}>
            <Text style={styles.statValue}>{relayCount}</Text>
            <Text style={styles.statLabel}>Relayed</Text>
          </View>
          <View style={styles.statBox}>
            <Text style={[styles.statValue, styles.errorValue]}>{errorCount}</Text>
            <Text style={styles.statLabel}>Errors</Text>
          </View>
        </View>
      </View>

      {/* Warning */}
      <View style={styles.warningCard}>
        <Text style={styles.warningText}>
          ⚠ MITM attacks require establishing parallel connections to both
          the peripheral and central devices. This may be detected by
          security-aware implementations.
        </Text>
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
  activeBanner: {
    backgroundColor: '#332200',
  },
  inactiveBanner: {
    backgroundColor: '#1A1A2E',
  },
  statusText: {
    color: '#FFAA00',
    fontSize: 14,
    fontFamily: 'monospace',
    fontWeight: 'bold',
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
  },
  label: {
    color: '#CCCCCC',
    fontSize: 13,
    fontFamily: 'monospace',
    marginTop: 8,
    marginBottom: 4,
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
  smallInput: {
    backgroundColor: '#222244',
    color: '#00FF88',
    padding: 8,
    borderRadius: 6,
    fontFamily: 'monospace',
    fontSize: 14,
    width: 80,
    textAlign: 'center',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  halfRow: {
    flex: 1,
    marginRight: 8,
  },
  toggleRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 4,
  },
  toggleLabel: {
    color: '#CCCCCC',
    fontSize: 13,
  },
  buttonRow: {
    flexDirection: 'row',
    marginHorizontal: 8,
    marginVertical: 4,
  },
  startButton: {
    flex: 1,
    backgroundColor: '#664400',
    padding: 14,
    borderRadius: 6,
    marginRight: 4,
    alignItems: 'center',
  },
  stopButton: {
    flex: 1,
    backgroundColor: '#662222',
    padding: 14,
    borderRadius: 6,
    marginLeft: 4,
    alignItems: 'center',
  },
  disabledButton: {
    opacity: 0.4,
  },
  startButtonText: {
    color: '#FFAA00',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
  },
  stopButtonText: {
    color: '#FF4444',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
  },
  statsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  statBox: {
    alignItems: 'center',
  },
  statValue: {
    color: '#00FF88',
    fontSize: 24,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  errorValue: {
    color: '#FF4444',
  },
  statLabel: {
    color: '#888888',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  warningCard: {
    backgroundColor: '#332200',
    margin: 8,
    padding: 12,
    borderRadius: 8,
    borderLeftWidth: 3,
    borderLeftColor: '#FFAA00',
  },
  warningText: {
    color: '#FFAA00',
    fontSize: 11,
    fontFamily: 'monospace',
    lineHeight: 16,
  },
});