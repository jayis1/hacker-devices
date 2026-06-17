/**
 * CaptureScreen.js — Frame capture controls and interval recording
 * Author: jayis1
 */

import React, {useState} from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Switch,
  Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const CaptureScreen = ({status, protocol}) => {
  const [intervalMode, setIntervalMode] = useState(false);
  const [intervalMs, setIntervalMs] = useState(1000);
  const [capturing, setCapturing] = useState(false);
  const [motionDetect, setMotionDetect] = useState(false);

  const handleSingleCapture = () => {
    if (!protocol) return;
    setCapturing(true);
    protocol.captureFrame(result => {
      setCapturing(false);
      if (result?.status === 'ok') {
        Alert.alert('Capture Complete', `Frame #${result.frame_id} captured`);
      } else {
        Alert.alert('Capture Failed', result?.msg || 'Unknown error');
      }
    });
  };

  const handleIntervalToggle = value => {
    setIntervalMode(value);
    if (!value) {
      protocol?.sendCommand('mode', {mode: 'passthrough'});
    } else {
      protocol?.startCovert({
        interval_ms: intervalMs,
        trigger: motionDetect ? 'motion' : 'interval',
      });
    }
  };

  const frameCount = status?.frame_count ?? 0;
  const sdFree = status?.sd_free ?? 0;

  return (
    <View style={styles.container}>
      {/* Single Capture */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Single Capture</Text>
        <Text style={styles.cardDesc}>
          Capture the current display frame to SD card
        </Text>
        <TouchableOpacity
          style={[styles.captureButton, capturing && styles.captureButtonDisabled]}
          onPress={handleSingleCapture}
          disabled={capturing}>
          <Icon
            name={capturing ? 'loading' : 'camera'}
            size={32}
            color="#FFF"
          />
          <Text style={styles.captureText}>
            {capturing ? 'CAPTURING...' : 'CAPTURE FRAME'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Capture Info */}
      <View style={styles.infoRow}>
        <View style={styles.infoCard}>
          <Icon name="counter" size={20} color="#AB47BC" />
          <Text style={styles.infoLabel}>Frames</Text>
          <Text style={styles.infoValue}>{frameCount}</Text>
        </View>
        <View style={styles.infoCard}>
          <Icon name="harddisk" size={20} color="#26C6DA" />
          <Text style={styles.infoLabel}>Free Space</Text>
          <Text style={styles.infoValue}>
            {sdFree ? `${(sdFree / (1024 * 1024)).toFixed(0)} MB` : 'N/A'}
          </Text>
        </View>
        <View style={styles.infoCard}>
          <Icon name="camera-burst" size={20} color="#FFA726" />
          <Text style={styles.infoLabel}>Resolution</Text>
          <Text style={styles.infoValue}>
            {status?.resolution || '--x--'}
          </Text>
        </View>
      </View>

      {/* Interval Recording */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Interval Recording</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Enable Interval</Text>
          <Switch
            value={intervalMode}
            onValueChange={handleIntervalToggle}
            trackColor={{false: '#555', true: '#E53935'}}
            thumbColor={intervalMode ? '#FFF' : '#CCC'}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Motion Detection</Text>
          <Switch
            value={motionDetect}
            onValueChange={setMotionDetect}
            trackColor={{false: '#555', true: '#E53935'}}
            thumbColor={motionDetect ? '#FFF' : '#CCC'}
          />
        </View>
        <Text style={styles.intervalHint}>
          {intervalMode
            ? `Capturing every ${intervalMs / 1000}s`
            : 'Toggle to enable automatic capture'}
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
    marginBottom: 4,
  },
  cardDesc: {
    color: '#B0B0B0',
    fontSize: 13,
    marginBottom: 12,
  },
  captureButton: {
    backgroundColor: '#E53935',
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 16,
    borderRadius: 10,
  },
  captureButtonDisabled: {
    backgroundColor: '#666',
  },
  captureText: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: '800',
    marginLeft: 10,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 12,
  },
  infoCard: {
    backgroundColor: '#1E1E2E',
    borderRadius: 10,
    padding: 12,
    flex: 1,
    alignItems: 'center',
    marginHorizontal: 3,
  },
  infoLabel: {
    color: '#B0B0B0',
    fontSize: 11,
    marginTop: 4,
  },
  infoValue: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '700',
    marginTop: 2,
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  switchLabel: {
    color: '#FFF',
    fontSize: 15,
  },
  intervalHint: {
    color: '#78909C',
    fontSize: 12,
    marginTop: 8,
    textAlign: 'center',
  },
});

export default CaptureScreen;
