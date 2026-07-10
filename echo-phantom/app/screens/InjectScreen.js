/**
 * InjectScreen.js — Audio Injection Control
 *
 * Author: jayis1
 * License: MIT
 *
 * Upload, manage, and inject audio clips into the I²S/TDM bus.
 * Supports replace, mix, and overlay injection modes.
 */

import React, { useState, useContext, useEffect } from 'react';
import { View, Text, StyleSheet, Alert, FlatList, TouchableOpacity } from 'react-native';
import { Card, Title, Paragraph, Button, RadioButton, Slider, ActivityIndicator } from 'react-native-paper';
import RNFS from 'react-native-fs';
import { BLEContext } from '../components/BLEManager';

const INJECT_REPLACE = 0;
const INJECT_MIX = 1;
const INJECT_OVERLAY = 2;

const MODE_NAMES = {
  0: 'Replace (full injection)',
  1: 'Mix (overlay with mic)',
  2: 'Overlay (both audible)',
};

export default function InjectScreen() {
  const ble = useContext(BLEContext);
  const [injecting, setInjecting] = useState(false);
  const [mode, setMode] = useState(INJECT_REPLACE);
  const [gain, setGain] = useState(100);
  const [selectedClip, setSelectedClip] = useState(0);
  const [clips, setClips] = useState([]);
  const [uploading, setUploading] = useState(false);
  const [uploadProgress, setUploadProgress] = useState(0);

  const loadClips = () => {
    // In a real implementation, we'd query the device for stored clips
    // For now, we show a placeholder list
    setClips([
      { id: 0, name: 'Clip 0', size: 0, duration: '--' },
      { id: 1, name: 'Clip 1', size: 0, duration: '--' },
      { id: 2, name: 'Clip 2', size: 0, duration: '--' },
    ]);
  };

  useEffect(() => {
    loadClips();
  }, []);

  const handleUploadClip = async (clipId) => {
    Alert.alert(
      'Upload Audio Clip',
      'Select source for the inject clip:',
      [
        { text: 'Record New', onPress: () => recordAndUpload(clipId) },
        { text: 'Select WAV File', onPress: () => selectAndUpload(clipId) },
        { text: 'Generate TTS', onPress: () => generateTTSAndUpload(clipId) },
        { text: 'Cancel', style: 'cancel' },
      ]
    );
  };

  const recordAndUpload = async (clipId) => {
    // In a real implementation, this would record audio using the phone's mic
    // and upload it as PCM data to the device
    Alert.alert('Recording', 'Audio recording would start here (not implemented in this demo).');
  };

  const selectAndUpload = async (clipId) => {
    try {
      // In a real implementation, use a file picker to select a WAV file
      // For now, we just simulate the upload
      setUploading(true);
      setUploadProgress(0);

      // Simulate uploading a 4-second 48kHz 16-bit mono clip
      const clipSize = 48000 * 2 * 4; // 384000 bytes
      const chunkSize = 200;
      const numChunks = Math.ceil(clipSize / chunkSize);

      // Create a dummy WAV-like payload
      for (let i = 0; i < numChunks; i++) {
        const chunk = new Array(Math.min(chunkSize, chunkSize)).fill(0);
        await ble.uploadClip(clipId, chunk, clipSize);
        setUploadProgress((i + 1) / numChunks);
      }

      setUploading(false);
      Alert.alert('Success', `Clip ${clipId} uploaded successfully (${clipSize} bytes).`);
    } catch (err) {
      setUploading(false);
      Alert.alert('Error', `Upload failed: ${err.message}`);
    }
  };

  const generateTTSAndUpload = async (clipId) => {
    // In a real implementation, use the phone's TTS engine to generate audio
    Alert.alert('TTS', 'Text-to-speech generation would happen here.');
  };

  const handleStartInjection = async () => {
    const result = await ble.startInjection(selectedClip, mode, gain);
    if (result) {
      setInjecting(true);
    }
  };

  const handleStopInjection = async () => {
    const result = await ble.stopInjection();
    if (result) {
      setInjecting(false);
    }
  };

  const handleEraseClips = () => {
    Alert.alert(
      'Erase All Clips',
      'This will permanently erase all inject clips from the device. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Erase', onPress: () => ble.eraseClips(), style: 'destructive' },
      ]
    );
  };

  const renderClipItem = ({ item }) => (
    <TouchableOpacity
      onPress={() => setSelectedClip(item.id)}
      style={[
        styles.clipItem,
        selectedClip === item.id && styles.clipItemSelected,
      ]}
    >
      <View style={styles.clipInfo}>
        <Text style={styles.clipName}>Clip {item.id}</Text>
        <Text style={styles.clipSize}>{item.duration}</Text>
      </View>
      <Button
        mode="text"
        onPress={() => handleUploadClip(item.id)}
        color="#e91e63"
        compact
      >
        Upload
      </Button>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Audio Injection</Title>
          <Paragraph style={styles.description}>
            Replace the microphone's audio with attacker-chosen content.
            The target device's ASR/wake-word engine will process this
            as genuine microphone input — completely silent, below the OS.
          </Paragraph>
        </Card.Content>
      </Card>

      {/* Clip selection */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Inject Clips</Title>
          <FlatList
            data={clips}
            renderItem={renderClipItem}
            keyExtractor={item => item.id.toString()}
            style={styles.clipList}
          />
          <Button
            mode="outlined"
            onPress={handleEraseClips}
            style={styles.eraseButton}
            color="#f44336"
            compact
          >
            Erase All Clips
          </Button>
        </Card.Content>
      </Card>

      {/* Upload progress */}
      {uploading && (
        <Card style={styles.card}>
          <Card.Content>
            <View style={styles.uploadRow}>
              <ActivityIndicator animating={true} color="#e91e63" />
              <Text style={styles.uploadText}>Uploading clip... {(uploadProgress * 100).toFixed(0)}%</Text>
            </View>
            <ProgressBar
              progress={uploadProgress}
              color="#e91e63"
              style={styles.progressBar}
            />
          </Card.Content>
        </Card>
      )}

      {/* Injection mode */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Injection Mode</Title>
          <RadioButton.Group
            onValueChange={value => setMode(parseInt(value))}
            value={mode.toString()}
          >
            <View style={styles.radioRow}>
              <RadioButton value={INJECT_REPLACE.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{MODE_NAMES[INJECT_REPLACE]}</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={INJECT_MIX.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{MODE_NAMES[INJECT_MIX]}</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={INJECT_OVERLAY.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{MODE_NAMES[INJECT_OVERLAY]}</Text>
            </View>
          </RadioButton.Group>
        </Card.Content>
      </Card>

      {/* Gain control */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Inject Gain: {gain}%</Title>
          <Slider
            value={gain}
            onValueChange={setGain}
            minimumValue={0}
            maximumValue={200}
            step={1}
            color="#e91e63"
            style={styles.slider}
          />
          <Text style={styles.gainHint}>
            100% = original volume. Values {'>'}100% amplify the injected audio.
          </Text>
        </Card.Content>
      </Card>

      {/* Injection controls */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Controls</Title>
          <View style={styles.buttonRow}>
            <Button
              mode="contained"
              onPress={handleStartInjection}
              disabled={injecting || !ble.connected}
              style={[styles.actionButton, { backgroundColor: '#2196f3' }]}
              icon="play-arrow"
            >
              Start Injection
            </Button>
            <Button
              mode="contained"
              onPress={handleStopInjection}
              disabled={!injecting || !ble.connected}
              style={[styles.actionButton, { backgroundColor: '#f44336' }]}
              icon="stop"
            >
              Stop Injection
            </Button>
          </View>
          {injecting && (
            <View style={styles.activeIndicator}>
              <View style={styles.injectDot} />
              <Text style={styles.injectText}>● INJECTING — Clip {selectedClip}</Text>
            </View>
          )}
        </Card.Content>
      </Card>

      <Card style={styles.warningCard}>
        <Card.Content>
          <Paragraph style={styles.warningText}>
            ⚠ Injecting voice commands into devices you do not own or do not
            have authorization to test may violate computer fraud and abuse
            laws (18 U.S.C. § 1030) and wiretap statutes.
          </Paragraph>
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0f0f1e' },
  card: { marginBottom: 12, backgroundColor: '#1a1a2e' },
  title: { color: '#e0e0e0', fontSize: 20 },
  sectionTitle: { color: '#e0e0e0', fontSize: 16 },
  description: { color: '#888', fontSize: 13, marginTop: 8 },
  clipList: { maxHeight: 200, marginTop: 8 },
  clipItem: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    padding: 12,
    marginVertical: 4,
    backgroundColor: '#2a2a3e',
    borderRadius: 8,
    borderWidth: 2,
    borderColor: 'transparent',
  },
  clipItemSelected: { borderColor: '#e91e63' },
  clipInfo: { flex: 1 },
  clipName: { color: '#e0e0e0', fontSize: 14, fontWeight: 'bold' },
  clipSize: { color: '#888', fontSize: 12 },
  eraseButton: { marginTop: 8, borderColor: '#f44336' },
  uploadRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  uploadText: { color: '#e0e0e0', marginLeft: 12, fontSize: 14 },
  progressBar: { marginTop: 4, height: 6 },
  radioRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  radioLabel: { color: '#e0e0e0', fontSize: 14 },
  slider: { marginTop: 8 },
  gainHint: { color: '#888', fontSize: 12, marginTop: 4 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 12 },
  actionButton: { flex: 1, marginHorizontal: 4 },
  activeIndicator: { flexDirection: 'row', alignItems: 'center', justifyContent: 'center', marginTop: 12 },
  injectDot: { width: 8, height: 8, borderRadius: 4, backgroundColor: '#2196f3', marginRight: 8 },
  injectText: { color: '#2196f3', fontWeight: 'bold', fontSize: 14 },
  warningCard: { marginTop: 8, backgroundColor: '#2a1a1a' },
  warningText: { color: '#ff9800', fontSize: 12 },
});