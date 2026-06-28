/**
 * TriggerConfigScreen.tsx - Trigger I/O configuration
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, ScrollView, StyleSheet, Alert } from 'react-native';
import { Button, Card, Title, Text, TextInput, SegmentedButtons, Switch, Divider } from 'react-native-paper';

interface TriggerConfigScreenProps {
  phantom: any;
}

export default function TriggerConfigScreen({ phantom }: TriggerConfigScreenProps) {
  const [source, setSource] = useState('external');
  const [edge, setEdge] = useState('rising');
  const [delayUs, setDelayUs] = useState('100');
  const [pulseWidthUs, setPulseWidthUs] = useState('10');
  const [autoRetrigger, setAutoRetrigger] = useState(false);

  const handleArm = () => {
    const delay = parseInt(delayUs) || 0;
    phantom.armTrigger(edge, delay);
    Alert.alert(
      'Trigger Armed',
      `Source: ${source}\nEdge: ${edge}\nDelay: ${delay}µs`,
      [{ text: 'OK' }]
    );
  };

  const handleDisarm = () => {
    phantom.addLog('info', 'Trigger disarmed');
    Alert.alert('Trigger Disarmed', 'All trigger outputs stopped', [{ text: 'OK' }]);
  };

  const handleTestPulse = () => {
    const width = parseInt(pulseWidthUs) || 10;
    phantom.addLog('info', `Test trigger pulse: ${width}µs`);
    Alert.alert('Test Pulse', `Sent ${width}µs pulse to TRIG_OUT`, [{ text: 'OK' }]);
  };

  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Trigger Source</Title>
          <SegmentedButtons
            value={source}
            onValueChange={setSource}
            buttons={[
              { label: 'External', value: 'external' },
              { label: 'Manual', value: 'manual' },
              { label: 'Auto', value: 'auto' },
            ]}
            style={styles.segmented}
          />
          <Text style={styles.description}>
            {source === 'external' && 'Trigger from external signal on SMA TRIG_IN connector'}
            {source === 'manual' && 'Trigger manually from app or front panel button'}
            {source === 'auto' && 'Auto-trigger when target temperature is reached'}
          </Text>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Edge Detection</Title>
          <SegmentedButtons
            value={edge}
            onValueChange={setEdge}
            buttons={[
              { label: 'Rising', value: 'rising' },
              { label: 'Falling', value: 'falling' },
              { label: 'Both', value: 'both' },
            ]}
            style={styles.segmented}
          />
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Timing</Title>
          <TextInput
            label="Delay after trigger (µs)"
            value={delayUs}
            onChangeText={setDelayUs}
            keyboardType="numeric"
            style={styles.input}
            mode="outlined"
          />
          <TextInput
            label="Output pulse width (µs)"
            value={pulseWidthUs}
            onChangeText={setPulseWidthUs}
            keyboardType="numeric"
            style={styles.input}
            mode="outlined"
          />
          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Auto re-trigger</Text>
            <Switch
              value={autoRetrigger}
              onValueChange={setAutoRetrigger}
              color="#FF5722"
            />
          </View>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Actions</Title>
          <Button
            mode="contained"
            onPress={handleArm}
            style={styles.button}
            icon="flash"
          >
            Arm Trigger
          </Button>
          <Button
            mode="outlined"
            onPress={handleDisarm}
            style={styles.button}
            icon="flash-off"
          >
            Disarm Trigger
          </Button>
          <Divider style={styles.divider} />
          <Button
            mode="outlined"
            onPress={handleTestPulse}
            style={styles.button}
            icon="pulse"
          >
            Send Test Pulse
          </Button>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Synchronization Guide</Title>
          <Text style={styles.guideText}>
            For DFA attacks: {'\n'}
            1. Connect TRIG_IN to target's clock or GPIO {'\n'}
            2. Set edge to match target's signal {'\n'}
            3. Set delay to trigger mid-crypto operation {'\n'}
            4. Thermal output activates on trigger {'\n'}
            {'\n'}
            For cold boot: {'\n'}
            1. Connect TRIG_OUT to target's reset {'\n'}
            2. Cool DRAM to target temperature {'\n'}
            3. Trigger resets target while cold {'\n'}
            4. Read memory before cells degrade
          </Text>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    padding: 8,
  },
  card: {
    marginBottom: 12,
    backgroundColor: '#16213e',
  },
  segmented: {
    marginVertical: 8,
  },
  description: {
    color: '#888',
    fontSize: 12,
    marginTop: 4,
  },
  input: {
    backgroundColor: '#0f172a',
    marginVertical: 4,
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginTop: 12,
  },
  switchLabel: {
    color: '#e0e0e0',
  },
  button: {
    marginVertical: 4,
  },
  divider: {
    marginVertical: 8,
  },
  guideText: {
    color: '#aaa',
    fontSize: 13,
    lineHeight: 20,
  },
});

// Author: jayis1