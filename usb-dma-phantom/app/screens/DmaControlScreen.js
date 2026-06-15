/**
 * DmaControlScreen — Real-time DMA read/write/scan interface
 * USB DMA Phantom Companion App
 */

import React, { useContext, useState, useCallback } from 'react';
import { View, StyleSheet, ScrollView, Alert, Clipboard } from 'react-native';
import { Text, Card, Button, TextInput, SegmentedButtons, List, HelperText } from 'react-native-paper';
import { BleContext } from '../utils/protocol';

export default function DmaControlScreen({ navigation }) {
  const { sendDmaCommand, dmaStatus } = useContext(BleContext);

  const [mode, setMode] = useState('read');
  const [address, setAddress] = useState('0x1000');
  const [length, setLength] = useState('256');
  const [pattern, setPattern] = useState('');
  const [scanStart, setScanStart] = useState('0x0');
  const [scanEnd, setScanEnd] = useState('0xFFFFFFFF');
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [history, setHistory] = useState([]);

  const executeDma = useCallback(async () => {
    const addr = parseInt(address, 16);
    const len = parseInt(length, 10);

    if (isNaN(addr) || isNaN(len) || len <= 0 || len > 4096) {
      Alert.alert('Invalid Input', 'Check address and length values');
      return;
    }

    setLoading(true);
    try {
      let response;
      switch (mode) {
        case 'read':
          response = await sendDmaCommand({
            cmd: 0x01,  // DMA_CMD_READ
            host_addr: addr,
            length: len,
          });
          break;
        case 'write':
          response = await sendDmaCommand({
            cmd: 0x02,  // DMA_CMD_WRITE
            host_addr: addr,
            length: len,
            data: pattern ? Buffer.from(pattern, 'hex') : null,
          });
          break;
        case 'scan':
          response = await sendDmaCommand({
            cmd: 0x03,  // DMA_CMD_SCAN
            host_addr: parseInt(scanStart, 16),
            length: parseInt(scanEnd, 16) - parseInt(scanStart, 16),
            data: Buffer.from(pattern, 'hex'),
          });
          break;
        case 'inject':
          response = await sendDmaCommand({
            cmd: 0x04,  // DMA_CMD_INJECT
            host_addr: addr,
            length: len,
            data: Buffer.from(pattern, 'hex'),
          });
          break;
      }

      setResult(response);
      setHistory(prev => [{
        id: Date.now(),
        mode,
        address: address,
        length: length,
        timestamp: new Date().toLocaleTimeString(),
        success: response?.success ?? false,
      }, ...prev].slice(0, 50));

    } catch (err) {
      Alert.alert('DMA Error', err.message);
    } finally {
      setLoading(false);
    }
  }, [mode, address, length, pattern, scanStart, scanEnd]);

  const copyResult = () => {
    if (result?.data) {
      Clipboard.setString(result.data.toString('hex'));
      Alert.alert('Copied', 'Hex data copied to clipboard');
    }
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Mode Selection */}
      <Card style={styles.card}>
        <Card.Title title="DMA Operation" />
        <Card.Content>
          <SegmentedButtons
            value={mode}
            onValueChange={setMode}
            buttons={[
              { value: 'read', label: 'Read' },
              { value: 'write', label: 'Write' },
              { value: 'scan', label: 'Scan' },
              { value: 'inject', label: 'Inject' },
            ]}
            style={styles.segmented}
          />
        </Card.Content>
      </Card>

      {/* Address Input */}
      <Card style={styles.card}>
        <Card.Title title="Parameters" />
        <Card.Content>
          <TextInput
            label="Physical Address (hex)"
            value={address}
            onChangeText={setAddress}
            mode="outlined"
            style={styles.input}
            dense
            keyboardType="default"
            autoCapitalize="none"
          />
          <HelperText type="info">Target host physical memory address</HelperText>

          <TextInput
            label="Length (bytes)"
            value={length}
            onChangeText={setLength}
            mode="outlined"
            style={styles.input}
            dense
            keyboardType="numeric"
          />
          <HelperText type="info">1–4096 bytes per operation</HelperText>

          {(mode === 'write' || mode === 'inject' || mode === 'scan') && (
            <TextInput
              label="Data Pattern (hex)"
              value={pattern}
              onChangeText={setPattern}
              mode="outlined"
              style={styles.input}
              dense
              multiline={mode === 'inject'}
              numberOfLines={mode === 'inject' ? 4 : 1}
              autoCapitalize="none"
            />
          )}

          {mode === 'scan' && (
            <>
              <TextInput
                label="Scan Start Address (hex)"
                value={scanStart}
                onChangeText={setScanStart}
                mode="outlined"
                style={styles.input}
                dense
              />
              <TextInput
                label="Scan End Address (hex)"
                value={scanEnd}
                onChangeText={setScanEnd}
                mode="outlined"
                style={styles.input}
                dense
              />
            </>
          )}
        </Card.Content>
      </Card>

      {/* Execute Button */}
      <Button
        mode="contained"
        onPress={executeDma}
        loading={loading}
        disabled={loading}
        style={styles.executeButton}
        labelStyle={styles.executeLabel}
        icon="play"
      >
        {mode === 'read' ? 'READ MEMORY' :
         mode === 'write' ? 'WRITE MEMORY' :
         mode === 'scan' ? 'SCAN MEMORY' : 'INJECT SHELLCODE'}
      </Button>

      {/* Results */}
      {result && (
        <Card style={styles.card}>
          <Card.Title
            title="Result"
            right={(props) => (
              <IconButton {...props} icon="content-copy" onPress={copyResult} />
            )}
          />
          <Card.Content>
            <Text style={styles.resultText}>
              {result.data
                ? `Data (${result.data.length} bytes):\n${result.data.toString('hex').match(/.{1,32}/g)?.join('\n')}`
                : result.message || 'Operation completed'}
            </Text>
          </Card.Content>
        </Card>
      )}

      {/* History */}
      {history.length > 0 && (
        <Card style={styles.card}>
          <Card.Title title="History" />
          <Card.Content>
            {history.slice(0, 10).map((item) => (
              <List.Item
                key={item.id}
                title={`${item.mode.toUpperCase()} @ ${item.address}`}
                description={`${item.length} bytes · ${item.timestamp}`}
                left={(props) => (
                  <List.Icon {...props}
                    icon={item.success ? 'check-circle' : 'alert-circle'}
                    color={item.success ? '#00ff88' : '#ff3366'}
                  />
                )}
              />
            ))}
          </Card.Content>
        </Card>
      )}
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
  },
  card: {
    backgroundColor: '#1a1a2e',
    marginBottom: 16,
    borderRadius: 12,
  },
  segmented: {
    marginBottom: 8,
  },
  input: {
    backgroundColor: '#16213e',
    marginBottom: 4,
  },
  executeButton: {
    backgroundColor: '#ff3366',
    marginBottom: 16,
    borderRadius: 8,
    padding: 4,
  },
  executeLabel: {
    fontSize: 14,
    fontWeight: 'bold',
  },
  resultText: {
    color: '#00ff88',
    fontFamily: 'monospace',
    fontSize: 11,
  },
});