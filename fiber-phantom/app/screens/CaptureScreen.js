/**
 * CaptureScreen.js — FiberPhantom Capture Control
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Start/stop packet capture, view live frame counts, download PCAP files.
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert, FlatList
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { C2_MSG_TYPE, formatBytes, formatUptime } from '../utils/protocol';

export default function CaptureScreen({ ble, connected, status }) {
  const [capturing, setCapturing] = useState(false);
  const [files, setFiles] = useState([]);
  const [downloading, setDownloading] = useState(null);

  const isCapturing = status.state === 'Capturing';

  const toggleCapture = async () => {
    if (!connected) return;

    if (!isCapturing) {
      // Check prerequisites
      if (!status.sdInserted) {
        Alert.alert('No SD Card', 'Insert a MicroSD card before starting capture');
        return;
      }
      if (status.linkRate === 'Down') {
        Alert.alert(
          'No Link',
          'No optical link detected. Capture will start but no frames will be captured until link is established.',
          [{ text: 'Start Anyway', onPress: () => startCapture() },
           { text: 'Cancel', style: 'cancel' }]
        );
        return;
      }
      startCapture();
    } else {
      stopCapture();
    }
  };

  const startCapture = async () => {
    try {
      await ble.startCapture();
      setCapturing(true);
    } catch (e) {
      Alert.alert('Error', 'Failed to start capture: ' + e.message);
    }
  };

  const stopCapture = async () => {
    try {
      await ble.stopCapture();
      setCapturing(false);
    } catch (e) {
      Alert.alert('Error', 'Failed to stop capture: ' + e.message);
    }
  };

  const refreshFileList = async () => {
    if (!connected) return;
    try {
      await ble.listFiles();
      // Files will come back via FILE_ENTRY messages
      // For now, populate with mock data structure
      setFiles([
        { name: 'FP_00001.pcap', size: 45 * 1024 * 1024 },
        { name: 'FP_00002.pcap', size: 128 * 1024 * 1024 },
      ]);
    } catch (e) {
      console.error(e);
    }
  };

  const downloadFile = async (file) => {
    if (!connected) return;
    setDownloading(file.name);
    try {
      await ble.downloadPcap(file.name);
      Alert.alert(
        'Download Started',
        `PCAP download for ${file.name} initiated over BLE.\n` +
        `Transfer rate: ~20 KB/s. Estimated time: ${Math.ceil(file.size / 20480 / 60)} min.\n` +
        `For faster transfer, connect via USB-C.`
      );
    } catch (e) {
      Alert.alert('Error', e.message);
    } finally {
      setDownloading(null);
    }
  };

  const deleteFile = (file) => {
    Alert.alert(
      'Delete File',
      `Delete ${file.name} (${formatBytes(file.size)})?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            // Remove from list
            setFiles(prev => prev.filter(f => f.name !== file.name));
            // Send delete command
            if (connected) {
              try {
                const payload = [];
                for (let i = 0; i < file.name.length && i < 32; i++) {
                  payload.push(file.name.charCodeAt(i));
                }
                await ble.sendCommand(C2_MSG_TYPE.DELETE_FILE, payload);
              } catch (e) {
                console.error(e);
              }
            }
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {!connected && (
        <View style={styles.warning}>
          <Icon name="alert-circle" size={20} color="#f85149" />
          <Text style={styles.warningText}>Not connected to device</Text>
        </View>
      )}

      {/* Capture Control */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Packet Capture</Text>

        {/* Big capture button */}
        <TouchableOpacity
          style={[
            styles.captureButton,
            isCapturing ? styles.captureButtonActive : styles.captureButtonIdle,
            !connected && styles.captureButtonDisabled,
          ]}
          onPress={toggleCapture}
          disabled={!connected}
        >
          <Icon
            name={isCapturing ? 'stop' : 'record-rec'}
            size={48}
            color="#fff"
          />
          <Text style={styles.captureButtonText}>
            {isCapturing ? 'STOP CAPTURE' : 'START CAPTURE'}
          </Text>
        </TouchableOpacity>

        {/* Live stats */}
        <View style={styles.statsGrid}>
          <StatBox label="Frames" value={status.stats.totalFrames.toLocaleString()} color="#3fb950" />
          <StatBox label="Dropped" value={status.stats.droppedFrames.toLocaleString()} color="#f85149" />
          <StatBox label="Bytes" value={formatBytes(status.stats.bytesCaptured)} color="#58a6ff" />
          <StatBox label="Uptime" value={formatUptime(status.stats.uptimeSec)} color="#d29922" />
        </View>

        {/* Link info */}
        <View style={styles.linkInfo}>
          <Icon name="fiber-optic" size={20} color={status.linkRate !== 'Down' ? '#00ff88' : '#f85149'} />
          <Text style={styles.linkText}>
            Link: {status.linkRate} {status.linkRate !== 'Down' ? `(${status.linkRate.startsWith('FC') ? 'Fibre Channel' : 'Ethernet'})` : ''}
          </Text>
        </View>
      </View>

      {/* Capture Files */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Text style={styles.cardTitle}>Captured Files</Text>
          <TouchableOpacity onPress={refreshFileList} disabled={!connected}>
            <Icon name="refresh" size={24} color={connected ? '#00ff88' : '#30363d'} />
          </TouchableOpacity>
        </View>

        {!status.sdInserted && (
          <View style={styles.noSd}>
            <Icon name="sd-off" size={32} color="#f85149" />
            <Text style={styles.noSdText}>No SD card inserted</Text>
          </View>
        )}

        {status.sdInserted && files.length === 0 && (
          <View style={styles.emptyState}>
            <Icon name="file-document-multiple" size={40} color="#30363d" />
            <Text style={styles.emptyText}>No capture files yet</Text>
            <Text style={styles.emptySubtext}>Start a capture to create PCAP files</Text>
          </View>
        )}

        {files.map((file, index) => (
          <View key={index} style={styles.fileRow}>
            <Icon name="file-document" size={24} color="#d29922" />
            <View style={styles.fileInfo}>
              <Text style={styles.fileName}>{file.name}</Text>
              <Text style={styles.fileSize}>{formatBytes(file.size)}</Text>
            </View>
            <TouchableOpacity
              onPress={() => downloadFile(file)}
              disabled={!connected || downloading === file.name}
              style={styles.fileAction}
            >
              <Icon
                name={downloading === file.name ? 'progress-download' : 'download'}
                size={20}
                color={connected ? '#58a6ff' : '#30363d'}
              />
            </TouchableOpacity>
            <TouchableOpacity onPress={() => deleteFile(file)} style={styles.fileAction}>
              <Icon name="delete" size={20} color="#f85149" />
            </TouchableOpacity>
          </View>
        ))}
      </View>

      {/* PCAP Info */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>PCAP Format Info</Text>
        <Text style={styles.infoText}>
          Captures are saved in libpcap format (compatible with Wireshark, tcpdump, etc.).
          Ethernet frames use LINKTYPE_ETHERNET (1), Fibre Channel frames use LINKTYPE_FIBRE_CHANNEL (196).
        </Text>
        <Text style={styles.infoText}>
          {'\n'}Transfer options:{'\n'}
          • BLE: ~20 KB/s (slow, for small captures){'\n'}
          • USB-C: ~5 MB/s (recommended for large captures){'\n'}
          • 60 GHz backhaul: ~1.5 Gbps (optional module, real-time streaming)
        </Text>
      </View>
    </ScrollView>
  );
}

// ---- Stat Box Component ----
function StatBox({ label, value, color }) {
  return (
    <View style={styles.statBox}>
      <Text style={[styles.statValue, { color }]}>{value}</Text>
      <Text style={styles.statLabel}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 12 },
  warning: {
    flexDirection: 'row', alignItems: 'center', gap: 8,
    backgroundColor: '#3d1014', borderRadius: 8, padding: 12, marginBottom: 12,
  },
  warningText: { color: '#f85149', fontSize: 14 },
  card: {
    backgroundColor: '#161b22', borderRadius: 12, padding: 16,
    marginBottom: 12, borderWidth: 1, borderColor: '#30363d',
  },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  cardTitle: { color: '#e6edf3', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  captureButton: {
    borderRadius: 16, paddingVertical: 32, alignItems: 'center',
    marginBottom: 16,
  },
  captureButtonIdle: { backgroundColor: '#238636' },
  captureButtonActive: { backgroundColor: '#da3633' },
  captureButtonDisabled: { backgroundColor: '#21262d' },
  captureButtonText: { color: '#fff', fontSize: 18, fontWeight: 'bold', marginTop: 8 },
  statsGrid: {
    flexDirection: 'row', flexWrap: 'wrap', gap: 8, marginBottom: 12,
  },
  statBox: {
    flexBasis: '48%', backgroundColor: '#21262d', borderRadius: 10,
    padding: 12, alignItems: 'center',
  },
  statValue: { fontSize: 18, fontWeight: 'bold' },
  statLabel: { color: '#8b949e', fontSize: 12, marginTop: 4 },
  linkInfo: { flexDirection: 'row', alignItems: 'center', gap: 8, paddingTop: 8, borderTopWidth: 1, borderTopColor: '#30363d' },
  linkText: { color: '#8b949e', fontSize: 13 },
  noSd: { alignItems: 'center', paddingVertical: 24 },
  noSdText: { color: '#f85149', fontSize: 14, marginTop: 8 },
  emptyState: { alignItems: 'center', paddingVertical: 24 },
  emptyText: { color: '#8b949e', fontSize: 14, marginTop: 8 },
  emptySubtext: { color: '#6e7681', fontSize: 12, marginTop: 4 },
  fileRow: {
    flexDirection: 'row', alignItems: 'center', backgroundColor: '#21262d',
    borderRadius: 8, padding: 12, marginBottom: 8,
  },
  fileInfo: { flex: 1, marginLeft: 8 },
  fileName: { color: '#e6edf3', fontSize: 14, fontFamily: 'monospace' },
  fileSize: { color: '#8b949e', fontSize: 12, marginTop: 2 },
  fileAction: { padding: 8 },
  infoText: { color: '#8b949e', fontSize: 12, lineHeight: 18 },
});