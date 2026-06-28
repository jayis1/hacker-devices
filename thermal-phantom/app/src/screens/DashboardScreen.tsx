/**
 * DashboardScreen.tsx - Main control dashboard
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, ScrollView, StyleSheet, Alert } from 'react-native';
import { Button, Card, Title, Paragraph, Divider, TextInput, IconButton } from 'react-native-paper';
import TemperatureGauge from '../components/TemperatureGauge';
import StatusBadge from '../components/StatusBadge';

interface DashboardScreenProps {
  phantom: any;
}

export default function DashboardScreen({ phantom }: DashboardScreenProps) {
  const [targetTemp, setTargetTemp] = useState('85.0');
  const [autoConnect, setAutoConnect] = useState(false);
  const { state } = phantom;

  useEffect(() => {
    if (autoConnect && !state.isConnected) {
      phantom.connect();
    }
  }, [autoConnect]);

  const handleSetTemp = () => {
    const temp = parseFloat(targetTemp);
    if (isNaN(temp) || temp < -15 || temp > 120) {
      Alert.alert('Invalid Temperature', 'Range: -15°C to 120°C');
      return;
    }
    phantom.setTargetTemp(temp);
  };

  const handleStop = () => {
    Alert.alert(
      'Emergency Stop',
      'Stop all outputs immediately?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'STOP', style: 'destructive', onPress: () => phantom.stop() },
      ]
    );
  };

  const getSafetyStatus = () => {
    if (state.safetyStatus === 'OK') return 'ok';
    if (state.safetyStatus === 'BATTERY LOW') return 'warning';
    return 'error';
  };

  const getModeStatus = () => {
    const modes: Record<string, 'ok' | 'warning' | 'error' | 'neutral'> = {
      idle: 'neutral',
      heat: 'warning',
      cool: 'ok',
      laser: 'error',
      profile: 'warning',
    };
    return modes[state.mode] || 'neutral';
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection Card */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Device Connection</Title>
          <View style={styles.row}>
            <StatusBadge
              label="Connection"
              value={state.isConnected ? 'Connected' : 'Disconnected'}
              status={state.isConnected ? 'ok' : 'error'}
            />
            <StatusBadge
              label="Safety"
              value={state.safetyStatus}
              status={getSafetyStatus()}
            />
          </View>
          <View style={styles.buttonRow}>
            {!state.isConnected ? (
              <Button
                mode="contained"
                onPress={() => phantom.connect()}
                style={styles.button}
              >
                Connect
              </Button>
            ) : (
              <Button
                mode="outlined"
                onPress={() => phantom.disconnect()}
                style={styles.button}
              >
                Disconnect
              </Button>
            )}
            <Button
              mode="contained"
              onPress={handleStop}
              style={[styles.button, styles.stopButton]}
              color="#F44336"
            >
              E-STOP
            </Button>
          </View>
        </Card.Content>
      </Card>

      {/* Temperature Card */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Temperature Control</Title>
          <View style={styles.gaugeRow}>
            <TemperatureGauge
              label="Target Surface"
              value={state.currentTemp}
              target={state.targetTemp}
            />
            <TemperatureGauge
              label="Target Set"
              value={state.targetTemp}
              min={-15}
              max={120}
            />
          </View>
          <Divider style={styles.divider} />
          <View style={styles.inputRow}>
            <TextInput
              label="Target Temp (°C)"
              value={targetTemp}
              onChangeText={setTargetTemp}
              keyboardType="numeric"
              style={styles.input}
              mode="outlined"
            />
            <IconButton
              icon="arrow-right"
              onPress={handleSetTemp}
              style={styles.sendButton}
            />
          </View>
          <View style={styles.modeButtons}>
            <Button
              mode={state.mode === 'heat' ? 'contained' : 'outlined'}
              onPress={() => phantom.setMode('heat')}
              style={styles.modeButton}
            >
              Heat
            </Button>
            <Button
              mode={state.mode === 'cool' ? 'contained' : 'outlined'}
              onPress={() => phantom.setMode('cool')}
              style={styles.modeButton}
            >
              Cool
            </Button>
            <Button
              mode={state.mode === 'idle' ? 'contained' : 'outlined'}
              onPress={() => phantom.setMode('idle')}
              style={styles.modeButton}
            >
              Idle
            </Button>
          </View>
        </Card.Content>
      </Card>

      {/* System Status Card */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>System Status</Title>
          <View style={styles.statusGrid}>
            <StatusBadge
              label="Mode"
              value={state.mode.toUpperCase()}
              status={getModeStatus()}
            />
            <StatusBadge
              label="TEC Duty"
              value={`${state.tecDuty.toFixed(0)}%`}
            />
            <StatusBadge
              label="Battery"
              value={`${state.batteryPct}%`}
              status={state.batteryPct < 20 ? 'warning' : 'ok'}
            />
            <StatusBadge
              label="Trigger"
              value={state.triggerArmed ? 'ARMED' : 'SAFE'}
              status={state.triggerArmed ? 'warning' : 'ok'}
            />
            <StatusBadge
              label="Laser"
              value={state.laserArmed ? 'ARMED' : 'SAFE'}
              status={state.laserArmed ? 'error' : 'ok'}
            />
            <StatusBadge
              label="Profile"
              value={state.profileRunning ? `Step ${state.profileStep}` : 'Idle'}
              status={state.profileRunning ? 'warning' : 'neutral'}
            />
          </View>
        </Card.Content>
      </Card>

      {/* Quick Actions Card */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Quick Actions</Title>
          <Paragraph style={styles.disclaimer}>
            ⚠️ Authorized security research only. Thermal fault injection
            can permanently damage target hardware.
          </Paragraph>
          <View style={styles.actionRow}>
            <Button mode="outlined" onPress={() => phantom.setTargetTemp(-5)}>
              Cool -5°C
            </Button>
            <Button mode="outlined" onPress={() => phantom.setTargetTemp(85)}>
              Heat 85°C
            </Button>
            <Button mode="outlined" onPress={() => phantom.setTargetTemp(110)}>
              Heat 110°C
            </Button>
          </View>
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
  row: {
    flexDirection: 'row',
    marginBottom: 8,
  },
  buttonRow: {
    flexDirection: 'row',
    marginTop: 8,
  },
  button: {
    flex: 1,
    marginHorizontal: 4,
  },
  stopButton: {
    backgroundColor: '#F44336',
  },
  gaugeRow: {
    flexDirection: 'row',
    justifyContent: 'center',
    flexWrap: 'wrap',
  },
  divider: {
    marginVertical: 12,
  },
  inputRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  input: {
    flex: 1,
    backgroundColor: '#0f172a',
  },
  sendButton: {
    marginLeft: 8,
  },
  modeButtons: {
    flexDirection: 'row',
    marginTop: 12,
    justifyContent: 'space-around',
  },
  modeButton: {
    flex: 1,
    marginHorizontal: 4,
  },
  statusGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  actionRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginTop: 8,
  },
  disclaimer: {
    color: '#FF9800',
    fontSize: 12,
    marginTop: 8,
    fontStyle: 'italic',
  },
});

// Author: jayis1