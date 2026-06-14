/**
 * NFC Relay Phantom — 125 kHz RFID Screen
 * EM4100 / HID Prox read and T5577 clone
 */

import React, { useState, useCallback } from 'react';
import { View, ScrollView, StyleSheet } from 'react-native';
import { Text, Card, Button, TextInput, RadioButton, Divider } from 'react-native-paper';
import { bleManager } from '../App';

export default function RFID125Screen() {
  const [reading, setReading] = useState(false);
  const [writing, setWriting] = useState(false);
  const [protocol, setProtocol] = useState('EM4100');
  const [lastRead, setLastRead] = useState(null);
  const [cloneUID, setCloneUID] = useState('');
  const [log, setLog] = useState([]);

  const addLog = (msg) => {
    setLog(prev => [msg, ...prev.slice(0, 99)]);
  };

  const startRead = useCallback(async () => {
    setReading(true);
    addLog(`[${protocol}] Starting read...`);
    await bleManager.sendCommand('RFID125', 'READ_START', { protocol });
  }, [protocol]);

  const stopRead = useCallback(async () => {
    setReading(false);
    addLog('[RFID125] Read stopped');
    await bleManager.sendCommand('RFID125', 'READ_STOP', {});
  }, []);

  const writeT5577 = useCallback(async () => {
    setWriting(true);
    addLog(`[T5577] Writing UID: ${cloneUID}`);
    await bleManager.sendCommand('RFID125', 'WRITE_T5577', {
      uid: cloneUID,
      protocol,
    });
    setWriting(false);
    addLog('[T5577] Write complete');
  }, [cloneUID, protocol]);

  // Subscribe to RFID data
  React.useEffect(() => {
    const unsub = bleManager.onRFID125Data((data) => {
      setLastRead(data);
      addLog(`[${data.protocol}] ID: ${data.uid_hex} | FC: ${data.facility_code} | CN: ${data.card_number}`);
    });
    return unsub;
  }, []);

  return (
    <ScrollView style={styles.container}>
      {/* Protocol Selector */}
      <Card style={styles.card}>
        <Card.Title title="125 kHz RFID" subtitle="EM4100 / HID Prox / T5577" />
        <Card.Content>
          <RadioButton.Group value={protocol} onValueChange={setProtocol}>
            <View style={styles.radioRow}>
              <RadioButton value="EM4100" />
              <Text>EM4100</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value="HID_PROX" />
              <Text>HID Prox II</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value="AWID" />
              <Text>AWID</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value="IO_PROX" />
              <Text>ioProx</Text>
            </View>
          </RadioButton.Group>
        </Card.Content>
      </Card>

      {/* Read */}
      <Card style={styles.card}>
        <Card.Title title="Read Tag" />
        <Card.Content>
          <Button
            mode="contained"
            onPress={reading ? stopRead : startRead}
            color={reading ? '#F44336' : '#6200EE'}
          >
            {reading ? 'Stop Reading' : 'Start Reading'}
          </Button>
          {reading && (
            <Text style={styles.scanning}>📡 Scanning... Present a 125 kHz tag</Text>
          )}
        </Card.Content>
      </Card>

      {/* Last Read Result */}
      {lastRead && (
        <Card style={styles.card}>
          <Card.Title title="Last Tag Read" />
          <Card.Content>
            <Text style={styles.resultLabel}>Protocol:</Text>
            <Text style={styles.resultValue}>{lastRead.protocol}</Text>
            <Divider style={styles.divider} />
            <Text style={styles.resultLabel}>Hex ID:</Text>
            <Text style={styles.uidHex}>{lastRead.uid_hex}</Text>
            <Divider style={styles.divider} />
            <Text style={styles.resultLabel}>Facility Code:</Text>
            <Text style={styles.resultValue}>{lastRead.facility_code}</Text>
            <Divider style={styles.divider} />
            <Text style={styles.resultLabel}>Card Number:</Text>
            <Text style={styles.resultValue}>{lastRead.card_number}</Text>
          </Card.Content>
        </Card>
      )}

      {/* Clone to T5577 */}
      <Card style={styles.card}>
        <Card.Title title="Clone to T5577" subtitle="Write UID to blank T5577 tag" />
        <Card.Content>
          <TextInput
            label="UID to write (hex)"
            value={cloneUID}
            onChangeText={setCloneUID}
            mode="outlined"
            style={styles.input}
            placeholder="e.g., 01020304"
            autoCapitalize="characters"
          />
          {lastRead && (
            <Button
              mode="text"
              onPress={() => setCloneUID(lastRead.uid_hex.replace(/:/g, ''))}
              style={styles.fillButton}
            >
              Fill from last read
            </Button>
          )}
          <Button
            mode="contained"
            onPress={writeT5577}
            disabled={!cloneUID || writing}
            color="#4CAF50"
            style={styles.button}
          >
            {writing ? 'Writing...' : 'Write to T5577'}
          </Button>
          <Text style={styles.warning}>
            ⚠️ Place blank T5577 tag near antenna before writing
          </Text>
        </Card.Content>
      </Card>

      {/* Activity Log */}
      <Card style={styles.card}>
        <Card.Title title="Activity Log" />
        <Card.Content>
          {log.slice(0, 10).map((entry, i) => (
            <Text key={i} style={styles.logEntry}>{entry}</Text>
          ))}
          {log.length === 0 && <Text style={styles.emptyLog}>No activity yet</Text>}
        </Card.Content>
      </Card>
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
  radioRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  scanning: {
    color: '#4CAF50',
    textAlign: 'center',
    marginTop: 8,
    fontSize: 14,
  },
  resultLabel: {
    fontSize: 12,
    color: '#999',
  },
  resultValue: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
  },
  uidHex: {
    fontFamily: 'monospace',
    fontSize: 20,
    color: '#6200EE',
    letterSpacing: 1,
  },
  divider: {
    marginVertical: 4,
  },
  input: {
    marginVertical: 8,
  },
  fillButton: {
    marginBottom: 8,
  },
  button: {
    marginTop: 4,
  },
  warning: {
    color: '#E65100',
    fontSize: 12,
    textAlign: 'center',
    marginTop: 8,
  },
  logEntry: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#666',
    paddingVertical: 1,
  },
  emptyLog: {
    color: '#999',
    textAlign: 'center',
  },
});