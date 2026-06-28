/**
 * ProfileEditorScreen.tsx - Create and edit thermal profiles
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, ScrollView, StyleSheet, FlatList, Alert } from 'react-native';
import { Button, Card, Title, Text, TextInput, List, IconButton, Divider, SegmentedButtons } from 'react-native-paper';

interface ProfileStep {
  id: string;
  type: string;
  targetTemp: number;
  duration: number;
  rampRate: number;
  laserPower: number;
  laserDuration: number;
}

interface ProfileEditorScreenProps {
  phantom: any;
}

const STEP_TYPES = [
  { label: 'Heat', value: 'heat' },
  { label: 'Cool', value: 'cool' },
  { label: 'Hold', value: 'hold' },
  { label: 'Laser', value: 'laser' },
  { label: 'Trigger', value: 'trigger' },
  { label: 'Wait', value: 'wait_trigger' },
  { label: 'Loop', value: 'loop' },
  { label: 'End', value: 'end' },
];

export default function ProfileEditorScreen({ phantom }: ProfileEditorScreenProps) {
  const [profileName, setProfileName] = useState('Cold Boot Attack');
  const [steps, setSteps] = useState<ProfileStep[]>([
    {
      id: '1',
      type: 'cool',
      targetTemp: -5,
      duration: 30000,
      rampRate: 2,
      laserPower: 0,
      laserDuration: 0,
    },
    {
      id: '2',
      type: 'hold',
      targetTemp: -5,
      duration: 10000,
      rampRate: 0,
      laserPower: 0,
      laserDuration: 0,
    },
    {
      id: '3',
      type: 'trigger',
      targetTemp: 0,
      duration: 100,
      rampRate: 0,
      laserPower: 0,
      laserDuration: 0,
    },
    {
      id: '4',
      type: 'heat',
      targetTemp: 85,
      duration: 30000,
      rampRate: 3,
      laserPower: 0,
      laserDuration: 0,
    },
    {
      id: '5',
      type: 'end',
      targetTemp: 0,
      duration: 0,
      rampRate: 0,
      laserPower: 0,
      laserDuration: 0,
    },
  ]);

  const addStep = () => {
    const newStep: ProfileStep = {
      id: Date.now().toString(),
      type: 'hold',
      targetTemp: 25,
      duration: 5000,
      rampRate: 0,
      laserPower: 0,
      laserDuration: 0,
    };
    setSteps([...steps.slice(0, -1), newStep, steps[steps.length - 1]]);
  };

  const removeStep = (id: string) => {
    setSteps(steps.filter(s => s.id !== id));
  };

  const updateStep = (id: string, field: keyof ProfileStep, value: any) => {
    setSteps(steps.map(s => s.id === id ? { ...s, [field]: value } : s));
  };

  const moveStep = (id: string, direction: 'up' | 'down') => {
    const idx = steps.findIndex(s => s.id === id);
    if (idx < 0) return;
    const swapIdx = direction === 'up' ? idx - 1 : idx + 1;
    if (swapIdx < 0 || swapIdx >= steps.length) return;
    const newSteps = [...steps];
    [newSteps[idx], newSteps[swapIdx]] = [newSteps[swapIdx], newSteps[idx]];
    setSteps(newSteps);
  };

  const executeProfile = () => {
    Alert.alert(
      'Execute Profile',
      `Start "${profileName}" with ${steps.length} steps?\n\nThis will control thermal outputs on the target device.`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start',
          style: 'destructive',
          onPress: () => {
            phantom.addLog('info', `Starting profile: ${profileName}`);
            // In production, send profile to device via BLE
          },
        },
      ]
    );
  };

  const renderStep = ({ item, index }: { item: ProfileStep; index: number }) => (
    <Card style={styles.stepCard}>
      <Card.Content>
        <View style={styles.stepHeader}>
          <Text style={styles.stepNumber}>Step {index + 1}</Text>
          <View style={styles.stepActions}>
            <IconButton
              icon="arrow-up"
              size={20}
              onPress={() => moveStep(item.id, 'up')}
              disabled={index === 0}
            />
            <IconButton
              icon="arrow-down"
              size={20}
              onPress={() => moveStep(item.id, 'down')}
              disabled={index === steps.length - 1}
            />
            <IconButton
              icon="delete"
              size={20}
              onPress={() => removeStep(item.id)}
              disabled={item.type === 'end'}
              iconColor="#F44336"
            />
          </View>
        </View>

        <SegmentedButtons
          value={item.type}
          onValueChange={(v) => updateStep(item.id, 'type', v)}
          buttons={STEP_TYPES}
          style={styles.segmented}
        />

        {(item.type === 'heat' || item.type === 'cool') && (
          <View style={styles.inputRow}>
            <TextInput
              label="Target (°C)"
              value={item.targetTemp.toString()}
              onChangeText={(v) => updateStep(item.id, 'targetTemp', parseFloat(v) || 0)}
              keyboardType="numeric"
              style={styles.input}
              mode="outlined"
              dense
            />
            <TextInput
              label="Duration (ms)"
              value={item.duration.toString()}
              onChangeText={(v) => updateStep(item.id, 'duration', parseInt(v) || 0)}
              keyboardType="numeric"
              style={styles.input}
              mode="outlined"
              dense
            />
            <TextInput
              label="Ramp (°C/s)"
              value={item.rampRate.toString()}
              onChangeText={(v) => updateStep(item.id, 'rampRate', parseFloat(v) || 0)}
              keyboardType="numeric"
              style={styles.input}
              mode="outlined"
              dense
            />
          </View>
        )}

        {item.type === 'hold' && (
          <TextInput
            label="Duration (ms)"
            value={item.duration.toString()}
            onChangeText={(v) => updateStep(item.id, 'duration', parseInt(v) || 0)}
            keyboardType="numeric"
            style={styles.fullInput}
            mode="outlined"
            dense
          />
        )}

        {item.type === 'laser' && (
          <View style={styles.inputRow}>
            <TextInput
              label="Power (%)"
              value={item.laserPower.toString()}
              onChangeText={(v) => updateStep(item.id, 'laserPower', parseFloat(v) || 0)}
              keyboardType="numeric"
              style={styles.input}
              mode="outlined"
              dense
            />
            <TextInput
              label="Duration (ms)"
              value={item.laserDuration.toString()}
              onChangeText={(v) => updateStep(item.id, 'laserDuration', parseInt(v) || 0)}
              keyboardType="numeric"
              style={styles.input}
              mode="outlined"
              dense
            />
          </View>
        )}

        {item.type === 'end' && (
          <Text style={styles.endText}>Profile ends here. All outputs will be disabled.</Text>
        )}
      </Card.Content>
    </Card>
  );

  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Thermal Profile</Title>
          <TextInput
            label="Profile Name"
            value={profileName}
            onChangeText={setProfileName}
            style={styles.fullInput}
            mode="outlined"
          />
        </Card.Content>
      </Card>

      <FlatList
        data={steps}
        renderItem={renderStep}
        keyExtractor={item => item.id}
        scrollEnabled={false}
      />

      <View style={styles.actionButtons}>
        <Button
          mode="outlined"
          onPress={addStep}
          icon="plus"
          style={styles.addButton}
        >
          Add Step
        </Button>
        <Button
          mode="contained"
          onPress={executeProfile}
          style={styles.executeButton}
        >
          Execute Profile
        </Button>
      </View>

      <View style={styles.presetSection}>
        <Title style={styles.presetTitle}>Presets</Title>
        <View style={styles.presetRow}>
          <Button
            mode="outlined"
            onPress={() => {
              setProfileName('DFA Attack');
              setSteps([
                { id: '1', type: 'heat', targetTemp: 80, duration: 20000, rampRate: 3, laserPower: 0, laserDuration: 0 },
                { id: '2', type: 'hold', targetTemp: 80, duration: 5000, rampRate: 0, laserPower: 0, laserDuration: 0 },
                { id: '3', type: 'trigger', targetTemp: 0, duration: 100, rampRate: 0, laserPower: 0, laserDuration: 0 },
                { id: '4', type: 'end', targetTemp: 0, duration: 0, rampRate: 0, laserPower: 0, laserDuration: 0 },
              ]);
            }}
          >
            DFA Attack
          </Button>
          <Button
            mode="outlined"
            onPress={() => {
              setProfileName('Cold Boot');
              setSteps([
                { id: '1', type: 'cool', targetTemp: -10, duration: 60000, rampRate: 2, laserPower: 0, laserDuration: 0 },
                { id: '2', type: 'hold', targetTemp: -10, duration: 10000, rampRate: 0, laserPower: 0, laserDuration: 0 },
                { id: '3', type: 'trigger', targetTemp: 0, duration: 100, rampRate: 0, laserPower: 0, laserDuration: 0 },
                { id: '4', type: 'end', targetTemp: 0, duration: 0, rampRate: 0, laserPower: 0, laserDuration: 0 },
              ]);
            }}
          >
            Cold Boot
          </Button>
        </View>
      </View>
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
  stepCard: {
    marginBottom: 8,
    backgroundColor: '#16213e',
  },
  stepHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  stepNumber: {
    color: '#FF5722',
    fontWeight: 'bold',
    fontSize: 16,
  },
  stepActions: {
    flexDirection: 'row',
  },
  segmented: {
    marginBottom: 8,
  },
  inputRow: {
    flexDirection: 'row',
    gap: 8,
  },
  input: {
    flex: 1,
    backgroundColor: '#0f172a',
  },
  fullInput: {
    backgroundColor: '#0f172a',
    marginTop: 8,
  },
  endText: {
    color: '#888',
    fontStyle: 'italic',
    marginTop: 8,
  },
  actionButtons: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginVertical: 12,
  },
  addButton: {
    flex: 1,
    marginRight: 4,
  },
  executeButton: {
    flex: 1,
    marginLeft: 4,
    backgroundColor: '#FF5722',
  },
  presetSection: {
    marginBottom: 24,
  },
  presetTitle: {
    color: '#e0e0e0',
    marginBottom: 8,
  },
  presetRow: {
    flexDirection: 'row',
    gap: 8,
  },
});

// Author: jayis1