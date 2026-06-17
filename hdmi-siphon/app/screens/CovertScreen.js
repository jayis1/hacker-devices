/**
 * CovertScreen.js — Covert mode configuration
 * Author: jayis1
 */

import React, {useState} from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Switch,
  TextInput,
  Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const CovertScreen = ({protocol}) => {
  const [armed, setArmed] = useState(false);
  const [intervalMs, setIntervalMs] = useState('1000');
  const [maxFrames, setMaxFrames] = useState('100');
  const [useMotion, setUseMotion] = useState(true);
  const [ledDisable, setLedDisable] = useState(true);

  const handleArm = () => {
    if (!protocol) return;

    if (!armed) {
      // Arm device
      protocol.startCovert(
        {
          interval_ms: parseInt(intervalMs) || 1000,
          max_frames: parseInt(maxFrames) || 100,
          trigger: useMotion ? 'motion' : 'interval',
        },
        result => {
          if (result?.status === 'ok') {
            setArmed(true);
            Alert.alert(
              'Covert Mode ARMED',
              'Device is recording in stealth mode. All LEDs disabled.',
            );
          } else {
            Alert.alert('Error', 'Failed to arm covert mode');
          }
        },
      );

      if (ledDisable) {
        protocol.sendCommand('config', {stealth: true});
      }
    } else {
      // Disarm
      protocol.stopCovert(result => {
        setArmed(false);
        protocol.sendCommand('config', {stealth: false});
        Alert.alert('Covert Mode DISARMED', 'Capture stopped.');
      });
    }
  };

  return (
    <View style={styles.container}>
      {/* Warning Banner */}
      <View style={styles.warningBanner}>
        <Icon name="alert" size={24} color="#FF9800" />
        <Text style={styles.warningText}>
          LEGAL USE ONLY — Requires explicit authorization
        </Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Covert Recording</Text>
        <Text style={styles.cardDesc}>
          Stealth frame capture with no visible indicators
        </Text>

        {/* Arming Button */}
        <TouchableOpacity
          style={[styles.armButton, armed ? styles.disarmButton : styles.armButtonColor]}
          onPress={handleArm}>
          <Icon
            name={armed ? 'shield-off' : 'shield-lock'}
            size={32}
            color="#FFF"
          />
          <Text style={styles.armText}>
            {armed ? 'DISARM' : 'ARM DEVICE'}
          </Text>
        </TouchableOpacity>

        <Text style={styles.statusText}>
          Status: {armed ? 'RECORDING' : 'STANDBY'}
        </Text>

        {/* Configuration */}
        <View style={styles.configSection}>
          <Text style={styles.configTitle}>Configuration</Text>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Capture Interval (ms)</Text>
            <TextInput
              style={styles.configInput}
              value={intervalMs}
              onChangeText={setIntervalMs}
              keyboardType="numeric"
              editable={!armed}
            />
          </View>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Max Frames</Text>
            <TextInput
              style={styles.configInput}
              value={maxFrames}
              onChangeText={setMaxFrames}
              keyboardType="numeric"
              editable={!armed}
            />
          </View>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Motion Detection</Text>
            <Switch
              value={useMotion}
              onValueChange={setUseMotion}
              disabled={armed}
              trackColor={{false: '#555', true: '#E53935'}}
              thumbColor={useMotion ? '#FFF' : '#CCC'}
            />
          </View>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Disable LEDs</Text>
            <Switch
              value={ledDisable}
              onValueChange={setLedDisable}
              disabled={armed}
              trackColor={{false: '#555', true: '#E53935'}}
              thumbColor={ledDisable ? '#FFF' : '#CCC'}
            />
          </View>
        </View>
      </View>

      <View style={styles.infoCard}>
        <Icon name="information" size={20} color="#42A5F5" />
        <Text style={styles.infoText}>
          In covert mode, all status LEDs are disabled and the device
          operates silently. Captured frames are stored on the SD card
          for later retrieval.
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#121212',
    padding: 12,
  },
  warningBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3E2723',
    padding: 10,
    borderRadius: 8,
    marginBottom: 12,
  },
  warningText: {
    color: '#FF9800',
    fontSize: 12,
    fontWeight: '600',
    marginLeft: 8,
    flex: 1,
  },
  card: {
    backgroundColor: '#1E1E2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: '700',
  },
  cardDesc: {
    color: '#B0B0B0',
    fontSize: 13,
    marginBottom: 16,
  },
  armButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 20,
    borderRadius: 12,
    borderWidth: 2,
    borderColor: '#FFF',
  },
  armButtonColor: {
    backgroundColor: '#E53935',
  },
  disarmButton: {
    backgroundColor: '#546E7A',
  },
  armText: {
    color: '#FFF',
    fontSize: 22,
    fontWeight: '800',
    letterSpacing: 2,
    marginLeft: 12,
  },
  statusText: {
    color: '#FFF',
    fontSize: 16,
    textAlign: 'center',
    marginTop: 12,
    fontWeight: '600',
  },
  configSection: {
    marginTop: 20,
    borderTopWidth: 1,
    borderTopColor: '#333',
    paddingTop: 12,
  },
  configTitle: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '600',
    marginBottom: 12,
  },
  configRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  configLabel: {
    color: '#B0B0B0',
    fontSize: 14,
    flex: 1,
  },
  configInput: {
    backgroundColor: '#2A2A3E',
    color: '#FFF',
    borderRadius: 6,
    padding: 8,
    width: 80,
    textAlign: 'center',
    fontSize: 14,
  },
  infoCard: {
    flexDirection: 'row',
    backgroundColor: '#1B2A3A',
    padding: 12,
    borderRadius: 8,
    alignItems: 'flex-start',
  },
  infoText: {
    color: '#B0B0B0',
    fontSize: 12,
    marginLeft: 8,
    flex: 1,
    lineHeight: 18,
  },
});

export default CovertScreen;
