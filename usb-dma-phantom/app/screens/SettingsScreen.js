/**
 * SettingsScreen — Device configuration and preferences
 * USB DMA Phantom Companion App
 */

import React, { useContext, useState } from 'react';
import { View, StyleSheet, ScrollView, Alert } from 'react-native';
import { Text, Card, List, Switch, TextInput, Button, Divider } from 'react-native-paper';
import { BleContext } from '../utils/protocol';

export default function SettingsScreen({ navigation }) {
  const { connectionStatus, sendDmaCommand, deviceInfo } = useContext(BleContext);

  const [autoConnect, setAutoConnect] = useState(true);
  const [encryptedC2, setEncryptedC2] = useState(true);
  const [stealthMode, setStealthMode] = useState(true);
  const [firmwareVersion, setFirmwareVersion] = useState('1.0.0');
  const [blePower, setBlePower] = useState('0 dBm');
  const [dmaTimeout, setDmaTimeout] = useState('30');
  const [scanDepth, setScanDepth] = useState('4');

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Settings */}
      <Card style={styles.card}>
        <Card.Title title="Connection" />
        <Card.Content>
          <List.Item
            title="Auto-Connect"
            description="Automatically connect to known devices"
            left={(props) => <List.Icon {...props} icon="bluetooth-connect" />}
            right={() => (
              <Switch value={autoConnect} onValueChange={setAutoConnect} />
            )}
          />
          <List.Item
            title="Encrypted C2 Link"
            description="AES-256-CTR encrypted BLE channel"
            left={(props) => <List.Icon {...props} icon="lock" />}
            right={() => (
              <Switch value={encryptedC2} onValueChange={setEncryptedC2} />
            )}
          />
          <List.Item
            title="BLE TX Power"
            description={`Current: ${blePower}`}
            left={(props) => <List.Icon {...props} icon="antenna" />}
          />
        </Card.Content>
      </Card>

      {/* Stealth Settings */}
      <Card style={styles.card}>
        <Card.Title title="Stealth Configuration" />
        <Card.Content>
          <List.Item
            title="Stealth Mode"
            description="Emulate legitimate PCIe device"
            left={(props) => <List.Icon {...props} icon="incognito" />}
            right={() => (
              <Switch value={stealthMode} onValueChange={setStealthMode} />
            )}
          />
          <TextInput
            label="DMA Timeout (seconds)"
            value={dmaTimeout}
            onChangeText={setDmaTimeout}
            mode="outlined"
            style={styles.input}
            dense
            keyboardType="numeric"
          />
          <TextInput
            label="Memory Scan Depth (GB)"
            value={scanDepth}
            onChangeText={setScanDepth}
            mode="outlined"
            style={styles.input}
            dense
            keyboardType="numeric"
          />
        </Card.Content>
      </Card>

      {/* Device Info */}
      <Card style={styles.card}>
        <Card.Title title="Device Information" />
        <Card.Content>
          <List.Item
            title="Firmware Version"
            description={firmwareVersion}
            left={(props) => <List.Icon {...props} icon="chip" />}
          />
          <List.Item
            title="Connection Status"
            description={connectionStatus}
            left={(props) => <List.Icon {...props} icon="link-variant" />}
          />
          <List.Item
            title="BLE MAC"
            description={deviceInfo?.mac || '—'}
            left={(props) => <List.Icon {...props} icon="access-point" />}
          />
          <List.Item
            title="MCU"
            description="STM32F423CHU6 (Cortex-M4F @ 120 MHz)"
            left={(props) => <List.Icon {...props} icon="memory" />}
          />
          <List.Item
            title="PCIe Bridge"
            description="TI XIO2001 (Gen2 x1, DMA capable)"
            left={(props) => <List.Icon {...props} icon="bridge" />}
          />
          <Divider style={styles.divider} />
          <Button
            mode="outlined"
            onPress={() => {
              sendDmaCommand({ cmd: 0xFF, host_addr: 0, length: 0 });
            }}
            icon="restart"
            style={styles.resetButton}
            textColor="#ff3366"
          >
            Factory Reset
          </Button>
        </Card.Content>
      </Card>

      {/* Firmware Update */}
      <Card style={styles.card}>
        <Card.Title title="Firmware Update" />
        <Card.Content>
          <Text style={styles.infoText}>
            Current: v{firmwareVersion}
          </Text>
          <Text style={styles.infoText}>
            Update via USB DFU or OTA BLE
          </Text>
          <Button
            mode="contained"
            onPress={() => Alert.alert('Firmware Update', 'Connect device via USB and use dfu-util to update firmware.')}
            icon="update"
            style={styles.updateButton}
          >
            Check for Updates
          </Button>
        </Card.Content>
      </Card>

      {/* Legal Notice */}
      <Card style={[styles.card, styles.warningCard]}>
        <Card.Content>
          <Text style={styles.warningText}>
            ⚠️ This tool is for AUTHORIZED security testing only. Using this
            device against systems you do not own or have explicit written
            permission to test is illegal. The user assumes all legal
            responsibility for their actions.
          </Text>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  card: {
    backgroundColor: '#1a1a2e',
    marginBottom: 16,
    borderRadius: 12,
  },
  warningCard: {
    backgroundColor: '#2a1a1a',
    borderColor: '#ff3366',
    borderWidth: 1,
  },
  warningText: {
    color: '#ff8866',
    fontSize: 12,
    textAlign: 'center',
  },
  input: {
    backgroundColor: '#16213e',
    marginBottom: 8,
  },
  infoText: {
    color: '#888',
    fontSize: 12,
    marginBottom: 4,
  },
  divider: {
    marginVertical: 8,
    backgroundColor: '#2a2a4a',
  },
  resetButton: {
    marginTop: 8,
    borderColor: '#ff3366',
  },
  updateButton: {
    marginTop: 8,
    backgroundColor: '#00ff88',
    borderRadius: 8,
  },
});