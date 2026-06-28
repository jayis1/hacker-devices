/**
 * LaserControlScreen.tsx - Laser pulse control with safety
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, ScrollView, StyleSheet, Alert } from 'react-native';
import { Button, Card, Title, Text, TextInput, Slider, Switch, Divider, ProgressBar } from 'react-native-paper';

interface LaserControlScreenProps {
  phantom: any;
}

export default function LaserControlScreen({ phantom }: LaserControlScreenProps) {
  const [power, setPower] = useState(50);
  const [duration, setDuration] = useState('50');
  const [shutterOpen, setShutterOpen] = useState(false);
  const [acknowledged, setAcknowledged] = useState(false);
  const { state } = phantom;

  const handleShutterToggle = (open: boolean) => {
    if (open && !state.interlockOk) {
      Alert.alert(
        'Interlock Not Ready',
        'Laser interlock is not engaged. Check safety circuit.',
        [{ text: 'OK' }]
      );
      return;
    }
    if (open) {
      Alert.alert(
        'Open Shutter',
        'Opening the laser shutter. Ensure safety goggles are worn and target area is clear.',
        [
          { text: 'Cancel', style: 'cancel' },
          {
            text: 'Open',
            style: 'destructive',
            onPress: () => {
              setShutterOpen(true);
              phantom.addLog('warn', 'Laser shutter opened');
            },
          },
        ]
      );
    } else {
      setShutterOpen(false);
      phantom.addLog('info', 'Laser shutter closed');
    }
  };

  const handleFire = () => {
    if (!shutterOpen) {
      Alert.alert('Shutter Closed', 'Open the shutter first', [{ text: 'OK' }]);
      return;
    }
    if (!acknowledged) {
      Alert.alert('Acknowledge Required', 'Please acknowledge safety warning', [{ text: 'OK' }]);
      return;
    }

    const dur = parseInt(duration) || 0;
    if (dur <= 0 || dur > 500) {
      Alert.alert('Invalid Duration', 'Range: 1-500ms', [{ text: 'OK' }]);
      return;
    }

    Alert.alert(
      '⚠️ FIRE LASER',
      `Power: ${power}%\nDuration: ${dur}ms\n\nThis will fire a Class 4 laser pulse. Ensure area is clear and safety measures are in place.`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'FIRE',
          style: 'destructive',
          onPress: () => {
            phantom.fireLaser(power, dur);
            setAcknowledged(false);
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* Safety Warning Card */}
      <Card style={[styles.card, styles.warningCard]}>
        <Card.Content>
          <Title style={styles.warningTitle}>⚠️ CLASS 4 LASER WARNING</Title>
          <Text style={styles.warningText}>
            This device contains a Class 4 laser (450nm, 1W).{'\n'}
            {'\n'}• Always wear 450nm safety goggles{'\n'}
            • Never look directly at the beam{'\n'}
            • Ensure target area is clear{'\n'}
            • Use interlocked enclosure{'\n'}
            • Have fire extinguisher nearby
          </Text>
          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>I acknowledge safety procedures</Text>
            <Switch
              value={acknowledged}
              onValueChange={setAcknowledged}
              color="#FF5722"
            />
          </View>
        </Card.Content>
      </Card>

      {/* Interlock Status */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Safety Interlock</Title>
          <View style={styles.statusRow}>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>Interlock</Text>
              <Text style={[styles.statusValue, { color: state.interlockOk ? '#4CAF50' : '#F44336' }]}>
                {state.interlockOk ? 'OK' : 'NOT ENGAGED'}
              </Text>
            </View>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>Shutter</Text>
              <Text style={[styles.statusValue, { color: shutterOpen ? '#FF9800' : '#4CAF50' }]}>
                {shutterOpen ? 'OPEN' : 'CLOSED'}
              </Text>
            </View>
          </View>
          <Button
            mode={shutterOpen ? 'outlined' : 'contained'}
            onPress={() => handleShutterToggle(!shutterOpen)}
            style={styles.button}
          >
            {shutterOpen ? 'Close Shutter' : 'Open Shutter'}
          </Button>
        </Card.Content>
      </Card>

      {/* Power Control */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Laser Power</Title>
          <View style={styles.powerDisplay}>
            <Text style={styles.powerValue}>{power}%</Text>
            <Text style={styles.powerLabel}>{Math.round(power * 10)}mW</Text>
          </View>
          <Slider
            value={power}
            onValueChange={setPower}
            minimumValue={0}
            maximumValue={100}
            step={1}
            color="#FF5722"
          />
          <View style={styles.powerPresets}>
            <Button mode="outlined" onPress={() => setPower(25)} style={styles.presetBtn}>25%</Button>
            <Button mode="outlined" onPress={() => setPower(50)} style={styles.presetBtn}>50%</Button>
            <Button mode="outlined" onPress={() => setPower(75)} style={styles.presetBtn}>75%</Button>
            <Button mode="outlined" onPress={() => setPower(100)} style={styles.presetBtn}>100%</Button>
          </View>
        </Card.Content>
      </Card>

      {/* Duration Control */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Pulse Duration</Title>
          <TextInput
            label="Duration (ms)"
            value={duration}
            onChangeText={setDuration}
            keyboardType="numeric"
            style={styles.input}
            mode="outlined"
          />
          <Text style={styles.rangeText}>Range: 1-500ms (hardware limited)</Text>
          <View style={styles.durationPresets}>
            <Button mode="outlined" onPress={() => setDuration('10')} style={styles.presetBtn}>10ms</Button>
            <Button mode="outlined" onPress={() => setDuration('50')} style={styles.presetBtn}>50ms</Button>
            <Button mode="outlined" onPress={() => setDuration('100')} style={styles.presetBtn}>100ms</Button>
            <Button mode="outlined" onPress={() => setDuration('500')} style={styles.presetBtn}>500ms</Button>
          </View>
        </Card.Content>
      </Card>

      {/* Fire Button */}
      <Card style={styles.card}>
        <Card.Content>
          <Button
            mode="contained"
            onPress={handleFire}
            style={[styles.fireButton, (!acknowledged || !shutterOpen) && styles.disabledButton]}
            disabled={!acknowledged || !shutterOpen}
            icon="flash"
          >
            FIRE LASER
          </Button>
          {!acknowledged && (
            <Text style={styles.helperText}>Acknowledge safety warning first</Text>
          )}
          {acknowledged && !shutterOpen && (
            <Text style={styles.helperText}>Open shutter first</Text>
          )}
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
  warningCard: {
    borderColor: '#F44336',
    borderWidth: 2,
  },
  warningTitle: {
    color: '#F44336',
    fontSize: 16,
  },
  warningText: {
    color: '#FFCCBC',
    fontSize: 13,
    lineHeight: 20,
    marginTop: 8,
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginTop: 12,
  },
  switchLabel: {
    color: '#e0e0e0',
    fontSize: 13,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginVertical: 12,
  },
  statusItem: {
    alignItems: 'center',
  },
  statusLabel: {
    color: '#888',
    fontSize: 12,
    textTransform: 'uppercase',
  },
  statusValue: {
    fontSize: 18,
    fontWeight: 'bold',
    marginTop: 4,
  },
  button: {
    marginVertical: 4,
  },
  powerDisplay: {
    alignItems: 'center',
    marginVertical: 16,
  },
  powerValue: {
    fontSize: 48,
    fontWeight: 'bold',
    color: '#FF5722',
  },
  powerLabel: {
    fontSize: 16,
    color: '#888',
  },
  powerPresets: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 8,
  },
  presetBtn: {
    flex: 1,
    marginHorizontal: 2,
  },
  input: {
    backgroundColor: '#0f172a',
    marginTop: 8,
  },
  rangeText: {
    color: '#666',
    fontSize: 12,
    marginTop: 4,
  },
  durationPresets: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 8,
  },
  fireButton: {
    backgroundColor: '#F44336',
    padding: 8,
  },
  disabledButton: {
    opacity: 0.5,
  },
  helperText: {
    color: '#888',
    fontSize: 12,
    textAlign: 'center',
    marginTop: 8,
  },
});

// Author: jayis1