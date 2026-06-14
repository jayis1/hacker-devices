/**
 * NFC Relay Phantom — NFC Screen
 * 13.56 MHz NFC operations: Reader, Emulator, Sniffer modes
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, ScrollView, StyleSheet, FlatList } from 'react-native';
import { Text, Card, Button, SegmentedButtons, List, Chip, Dialog, Portal } from 'react-native-paper';
import { bleManager } from '../App';

const NFC_PROTOCOLS = [
  { label: 'NFC-A (106k)', value: 'A_106' },
  { label: 'NFC-A (212k)', value: 'A_212' },
  { label: 'NFC-A (424k)', value: 'A_424' },
  { label: 'NFC-B (106k)', value: 'B_106' },
  { label: 'NFC-F (212k)', value: 'F_212' },
  { label: 'NFC-F (424k)', value: 'F_424' },
  { label: 'NFC-V (15693)', value: 'V' },
];

export default function NFCScreen({ route }) {
  const [mode, setMode] = useState(route?.params?.mode || 'reader');
  const [protocol, setProtocol] = useState('A_106');
  const [scanning, setScanning] = useState(false);
  const [emulating, setEmulating] = useState(false);
  const [sniffing, setSniffing] = useState(false);
  const [tags, setTags] = useState([]);
  const [frames, setFrames] = useState([]);
  const [emulateUID, setEmulateUID] = useState('01020304');

  useEffect(() => {
    const unsub = bleManager.onNFCData((data) => {
      if (data.type === 'tag') {
        setTags(prev => [data, ...prev.slice(0, 49)]);
      } else if (data.type === 'frame') {
        setFrames(prev => [data, ...prev.slice(0, 199)]);
      }
    });
    return unsub;
  }, []);

  const startReader = useCallback(async () => {
    setScanning(true);
    setTags([]);
    await bleManager.sendCommand('NFC', 'READER_START', { protocol });
  }, [protocol]);

  const stopReader = useCallback(async () => {
    setScanning(false);
    await bleManager.sendCommand('NFC', 'READER_STOP', {});
  }, []);

  const startEmulator = useCallback(async () => {
    setEmulating(true);
    await bleManager.sendCommand('NFC', 'EMULATE_START', {
      uid: emulateUID,
      protocol,
    });
  }, [emulateUID, protocol]);

  const stopEmulator = useCallback(async () => {
    setEmulating(false);
    await bleManager.sendCommand('NFC', 'EMULATE_STOP', {});
  }, []);

  const startSniffer = useCallback(async () => {
    setSniffing(true);
    setFrames([]);
    await bleManager.sendCommand('NFC', 'SNIFFER_START', { protocol });
  }, [protocol]);

  const stopSniffer = useCallback(async () => {
    setSniffing(false);
    await bleManager.sendCommand('NFC', 'SNIFFER_STOP', {});
  }, []);

  const renderTag = useCallback(({ item }) => (
    <List.Item
      title={`UID: ${item.uid}`}
      description={`${item.protocol} | ATQA: ${item.atqa || 'N/A'} | SAK: ${item.sak || 'N/A'}`}
      left={(props) => <List.Icon {...props} icon="nfc-variant" />}
      right={(props) => (
        <View style={styles.tagActions}>
          <Button
            compact
            mode="text"
            onPress={() => {
              setEmulateUID(item.uid.replace(/:/g, ''));
              setMode('emulate');
            }}
          >
            Emulate
          </Button>
          <Button compact mode="text" onPress={() => { /* Save to history */ }}>
            Save
          </Button>
        </View>
      )}
    />
  ), []);

  return (
    <ScrollView style={styles.container}>
      {/* Mode Selector */}
      <Card style={styles.card}>
        <Card.Title title="NFC 13.56 MHz" subtitle="ISO 14443 / FeliCa / ISO 15693" />
        <Card.Content>
          <SegmentedButtons
            value={mode}
            onValueChange={setMode}
            buttons={[
              { value: 'reader', label: 'Reader' },
              { value: 'emulate', label: 'Emulate' },
              { value: 'sniffer', label: 'Sniffer' },
            ]}
          />
        </Card.Content>
      </Card>

      {/* Protocol Selector */}
      <Card style={styles.card}>
        <Card.Title title="Protocol" />
        <Card.Content>
          <View style={styles.chipRow}>
            {NFC_PROTOCOLS.map((p) => (
              <Chip
                key={p.value}
                selected={protocol === p.value}
                onPress={() => setProtocol(p.value)}
                style={styles.chip}
                mode="outlined"
              >
                {p.label}
              </Chip>
            ))}
          </View>
        </Card.Content>
      </Card>

      {/* Reader Mode */}
      {mode === 'reader' && (
        <Card style={styles.card}>
          <Card.Title title="Tag Reader" />
          <Card.Content>
            <Button
              mode="contained"
              onPress={scanning ? stopReader : startReader}
              color={scanning ? '#F44336' : '#6200EE'}
            >
              {scanning ? 'Stop Scanning' : 'Start Scanning'}
            </Button>
          </Card.Content>
        </Card>
      )}

      {/* Emulator Mode */}
      {mode === 'emulate' && (
        <Card style={styles.card}>
          <Card.Title title="Card Emulator" />
          <Card.Content>
            <Text style={styles.label}>Emulated UID (hex):</Text>
            <Text style={styles.uidDisplay}>{emulateUID.match(/.{2}/g)?.join(':') || emulateUID}</Text>
            <Button
              mode="contained"
              onPress={emulating ? stopEmulator : startEmulator}
              color={emulating ? '#F44336' : '#4CAF50'}
              style={styles.button}
            >
              {emulating ? 'Stop Emulation' : 'Start Emulation'}
            </Button>
            {emulating && (
              <Text style={styles.status}>🟢 Emulating — Present device to reader</Text>
            )}
          </Card.Content>
        </Card>
      )}

      {/* Sniffer Mode */}
      {mode === 'sniffer' && (
        <Card style={styles.card}>
          <Card.Title title="Frame Sniffer" />
          <Card.Content>
            <Button
              mode="contained"
              onPress={sniffing ? stopSniffer : startSniffer}
              color={sniffing ? '#F44336' : '#FF9800'}
            >
              {sniffing ? 'Stop Sniffing' : 'Start Sniffing'}
            </Button>
            {frames.length > 0 && (
              <Text style={styles.frameCount}>{frames.length} frames captured</Text>
            )}
          </Card.Content>
        </Card>
      )}

      {/* Tag List (Reader mode) */}
      {mode === 'reader' && tags.length > 0 && (
        <Card style={styles.card}>
          <Card.Title title={`Tags Found (${tags.length})`} />
          <FlatList
            data={tags}
            renderItem={renderTag}
            keyExtractor={(item, index) => `${item.uid}-${index}`}
            scrollEnabled={false}
          />
        </Card>
      )}

      {/* Frame Log (Sniffer mode) */}
      {mode === 'sniffer' && frames.length > 0 && (
        <Card style={styles.card}>
          <Card.Title title={`Captured Frames (${frames.length})`} />
          <Card.Content>
            {frames.slice(0, 20).map((frame, i) => (
              <View key={i} style={styles.frameItem}>
                <Text style={styles.frameDir}>{frame.direction === 'PCD' ? '→' : '←'}</Text>
                <Text style={styles.frameData} numberOfLines={1}>{frame.hex}</Text>
              </View>
            ))}
            {frames.length > 20 && (
              <Text style={styles.moreFrames}>... and {frames.length - 20} more</Text>
            )}
          </Card.Content>
        </Card>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  card: {
    margin: 8,
  },
  chipRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 4,
  },
  chip: {
    margin: 2,
  },
  label: {
    fontSize: 14,
    color: '#666',
    marginTop: 8,
  },
  uidDisplay: {
    fontFamily: 'monospace',
    fontSize: 20,
    color: '#6200EE',
    marginVertical: 8,
    letterSpacing: 1,
  },
  button: {
    marginTop: 8,
  },
  status: {
    fontSize: 14,
    color: '#4CAF50',
    marginTop: 8,
    textAlign: 'center',
  },
  frameCount: {
    fontSize: 14,
    color: '#666',
    marginTop: 8,
    textAlign: 'center',
  },
  tagActions: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  frameItem: {
    flexDirection: 'row',
    paddingVertical: 2,
  },
  frameDir: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#6200EE',
    width: 20,
  },
  frameData: {
    fontFamily: 'monospace',
    fontSize: 10,
    flex: 1,
  },
  moreFrames: {
    fontSize: 12,
    color: '#999',
    textAlign: 'center',
    marginTop: 4,
  },
});