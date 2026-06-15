/**
 * PayloadManagerScreen — Manage stored DMA payloads
 * USB DMA Phantom Companion App
 */

import React, { useContext, useState, useEffect } from 'react';
import { View, StyleSheet, ScrollView, Alert } from 'react-native';
import { Text, Card, Button, List, Dialog, Portal, TextInput, IconButton, FAB } from 'react-native-paper';
import { BleContext } from '../utils/protocol';

const PRESET_PAYLOADS = [
  { name: 'Credential Dump', description: 'Extract Windows credentials from SAM', size: 2048, vid: '10EC', pid: '8168' },
  { name: 'BitLocker Key', description: 'Extract BitLocker volume key from memory', size: 4096, vid: '10EC', pid: '8168' },
  { name: 'Kernel Shellcode', description: 'Execute arbitrary shellcode in kernel mode', size: 1024, vid: '8086', pid: '1563' },
  { name: 'Process List', description: 'Enumerate running processes via EPROCESS scan', size: 512, vid: '10EC', pid: '8168' },
  { name: 'IOMMU Bypass', description: 'Bypass IOMMU protections on unpatched hosts', size: 3072, vid: '8086', pid: '1563' },
];

export default function PayloadManagerScreen({ navigation }) {
  const { sendDmaCommand, connectionStatus } = useContext(BleContext);
  const [payloads, setPayloads] = useState([]);
  const [selectedPayload, setSelectedPayload] = useState(null);
  const [dialogVisible, setDialogVisible] = useState(false);
  const [vidInput, setVidInput] = useState('10EC');
  const [pidInput, setPidInput] = useState('8168');
  const [deviceNameInput, setDeviceNameInput] = useState('DMA Phantom');
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    // Load stored payloads from device
    if (connectionStatus === 'connected') {
      loadPayloads();
    }
  }, [connectionStatus]);

  const loadPayloads = async () => {
    try {
      const response = await sendDmaCommand({
        cmd: 0x10,  // CMD_LIST_PAYLOADS
        host_addr: 0,
        length: 0,
      });
      if (response?.payloads) {
        setPayloads(response.payloads);
      }
    } catch (err) {
      // Use presets for demo
      setPayloads(PRESET_PAYLOADS);
    }
  };

  const uploadPayload = async (payload) => {
    setLoading(true);
    try {
      await sendDmaCommand({
        cmd: 0x11,  // CMD_UPLOAD_PAYLOAD
        host_addr: 0,
        length: payload.size,
        data: {
          name: payload.name,
          vid: parseInt(vidInput, 16),
          pid: parseInt(pidInput, 16),
        },
      });
      Alert.alert('Success', `Payload "${payload.name}" uploaded to device`);
      loadPayloads();
    } catch (err) {
      Alert.alert('Upload Error', err.message);
    } finally {
      setLoading(false);
    }
  };

  const deletePayload = async (index) => {
    try {
      await sendDmaCommand({
        cmd: 0x12,  // CMD_DELETE_PAYLOAD
        host_addr: 0,
        length: 0,
        data: { index },
      });
      Alert.alert('Deleted', 'Payload removed from device');
      loadPayloads();
    } catch (err) {
      Alert.alert('Delete Error', err.message);
    }
  };

  const executePayload = async (index) => {
    Alert.alert(
      'Execute Payload',
      'This will execute the selected DMA payload on the connected host. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Execute',
          style: 'destructive',
          onPress: async () => {
            try {
              await sendDmaCommand({
                cmd: 0x04,  // DMA_CMD_INJECT
                host_addr: 0,
                length: 0,
                data: { payload_index: index },
              });
            } catch (err) {
              Alert.alert('Execution Error', err.message);
            }
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Stored Payloads */}
      <Card style={styles.card}>
        <Card.Title
          title="Stored Payloads"
          subtitle={`${payloads.length} payloads on device`}
        />
        <Card.Content>
          {payloads.map((payload, index) => (
            <List.Item
              key={index}
              title={payload.name}
              description={`${payload.description}\nSize: ${payload.size} bytes · VID:${payload.vid} PID:${payload.pid}`}
              left={(props) => <List.Icon {...props} icon="file-code" color="#00ff88" />}
              right={(props) => (
                <View style={styles.payloadActions}>
                  <IconButton
                    icon="play"
                    size={20}
                    onPress={() => executePayload(index)}
                    iconColor="#ff3366"
                  />
                  <IconButton
                    icon="delete"
                    size={20}
                    onPress={() => deletePayload(index)}
                    iconColor="#888"
                  />
                </View>
              )}
              style={styles.payloadItem}
            />
          ))}
        </Card.Content>
      </Card>

      {/* Preset Library */}
      <Card style={styles.card}>
        <Card.Title title="Preset Library" subtitle="Upload preset payloads to device" />
        <Card.Content>
          {PRESET_PAYLOADS.map((preset, index) => (
            <List.Item
              key={index}
              title={preset.name}
              description={preset.description}
              left={(props) => <List.Icon {...props} icon="download" color="#00ff88" />}
              right={(props) => (
                <Button
                  mode="text"
                  onPress={() => uploadPayload(preset)}
                  icon="upload"
                  labelStyle={{ fontSize: 11 }}
                  textColor="#00ff88"
                >
                  Upload
                </Button>
              )}
            />
          ))}
        </Card.Content>
      </Card>

      {/* Device Configuration */}
      <Card style={styles.card}>
        <Card.Title title="Device Identity" subtitle="Configure VID/PID for social engineering" />
        <Card.Content>
          <TextInput
            label="Vendor ID (hex)"
            value={vidInput}
            onChangeText={setVidInput}
            mode="outlined"
            style={styles.input}
            dense
            autoCapitalize="none"
          />
          <TextInput
            label="Product ID (hex)"
            value={pidInput}
            onChangeText={setPidInput}
            mode="outlined"
            style={styles.input}
            dense
            autoCapitalize="none"
          />
          <TextInput
            label="Device Name"
            value={deviceNameInput}
            onChangeText={setDeviceNameInput}
            mode="outlined"
            style={styles.input}
            dense
          />
          <Button
            mode="contained"
            onPress={() => {
              sendDmaCommand({
                cmd: 0x20,  // CMD_SET_VIDPID
                host_addr: 0,
                length: 0,
                data: {
                  vid: parseInt(vidInput, 16),
                  pid: parseInt(pidInput, 16),
                  name: deviceNameInput,
                },
              });
            }}
            style={styles.configButton}
            icon="content-save"
          >
            Save Configuration
          </Button>
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
    paddingBottom: 80,
  },
  card: {
    backgroundColor: '#1a1a2e',
    marginBottom: 16,
    borderRadius: 12,
  },
  payloadItem: {
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a4a',
  },
  payloadActions: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  input: {
    backgroundColor: '#16213e',
    marginBottom: 8,
  },
  configButton: {
    backgroundColor: '#00ff88',
    marginTop: 8,
    borderRadius: 8,
  },
});