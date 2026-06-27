/**
 * Tactile-Phantom — Companion App
 * screens/StorageScreen.tsx — SD card log browser and export
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, StyleSheet, Alert, Share,
} from 'react-native';
import { useStore } from '../src/store';
import { EV_TYPE_NAMES } from '../src/store';

interface LogFile {
  name: string;
  size: number;
  events: number;
  date: string;
}

export default function StorageScreen() {
  const events = useStore((s) => s.events);
  const [files, setFiles] = useState<LogFile[]>([]);
  const [selectedFile, setSelectedFile] = useState<string | null>(null);

  // Simulated file list (in production, read from device via BLE)
  useEffect(() => {
    // Generate simulated file list
    const now = new Date();
    const simulated: LogFile[] = [];
    for (let i = 0; i < 5; i++) {
      const date = new Date(now.getTime() - i * 600000);
      simulated.push({
        name: `capture_${date.getTime()}.bin`,
        size: 45000 + Math.random() * 100000,
        events: Math.floor(200 + Math.random() * 500),
        date: date.toLocaleString(),
      });
    }
    setFiles(simulated);
  }, []);

  const exportEvents = async (format: 'json' | 'csv') => {
    if (events.length === 0) {
      Alert.alert('No Events', 'No events to export');
      return;
    }

    let content = '';
    if (format === 'json') {
      content = JSON.stringify(events, null, 2);
    } else {
      // CSV format
      content = 'index,type,vendor,finger_count,x,y,pressure,timestamp\n';
      events.forEach((ev, i) => {
        if (ev.fingers.length > 0) {
          ev.fingers.forEach((f) => {
            content += `${i},${EV_TYPE_NAMES[ev.type] || ev.type},${ev.vendor},${ev.fingerCount},${f.x},${f.y},${f.pressure},${ev.timestamp}\n`;
          });
        } else {
          content += `${i},${EV_TYPE_NAMES[ev.type] || ev.type},${ev.vendor},0,0,0,0,${ev.timestamp}\n`;
        }
      });
    }

    try {
      await Share.share({
        message: content,
        title: `Tactile-Phantom events (${format.toUpperCase()})`,
      });
    } catch (error) {
      Alert.alert('Export Failed', String(error));
    }
  };

  const downloadFile = async (file: LogFile) => {
    Alert.alert(
      'Download',
      `Download ${file.name} (${(file.size / 1024).toFixed(1)} KB, ${file.events} events)?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Download via BLE',
          onPress: () => Alert.alert('Downloading', `Started BLE download of ${file.name}`),
        },
        {
          text: 'Download via WiFi',
          onPress: () => Alert.alert('Downloading', `Started WiFi download of ${file.name}`),
        },
      ]
    );
  };

  const renderItem = ({ item }: { item: LogFile }) => (
    <TouchableOpacity
      style={[styles.fileItem, selectedFile === item.name && styles.fileItemSelected]}
      onPress={() => setSelectedFile(item.name)}
      onLongPress={() => downloadFile(item)}
    >
      <View style={styles.fileInfo}>
        <Text style={styles.fileName}>{item.name}</Text>
        <Text style={styles.fileDate}>{item.date}</Text>
      </View>
      <View style={styles.fileStats}>
        <Text style={styles.fileSize}>{(item.size / 1024).toFixed(1)} KB</Text>
        <Text style={styles.fileEvents}>{item.events} events</Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Storage</Text>

      <View style={styles.statsBar}>
        <Text style={styles.statsText}>
          In-memory: {events.length} events
        </Text>
        <Text style={styles.statsText}>
          Files: {files.length}
        </Text>
      </View>

      <View style={styles.exportSection}>
        <Text style={styles.exportTitle}>Export Current Events</Text>
        <View style={styles.exportRow}>
          <TouchableOpacity style={styles.exportBtn} onPress={() => exportEvents('json')}>
            <Text style={styles.exportBtnText}>Export JSON</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.exportBtn} onPress={() => exportEvents('csv')}>
            <Text style={styles.exportBtnText}>Export CSV</Text>
          </TouchableOpacity>
        </View>
      </View>

      <Text style={styles.fileListTitle}>SD Card Files (tap to select, long-press to download)</Text>
      <FlatList
        data={files}
        keyExtractor={(item) => item.name}
        renderItem={renderItem}
        style={styles.fileList}
        ListEmptyComponent={
          <Text style={styles.emptyText}>No files on SD card</Text>
        }
      />

      <Text style={styles.footer}>
        Author: jayis1 · GPL-2.0 · Authorized use only
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 15 },
  title: { color: '#e0e0e0', fontSize: 22, fontWeight: 'bold', textAlign: 'center', marginBottom: 10 },
  statsBar: { flexDirection: 'row', justifyContent: 'space-around', backgroundColor: '#16213e', borderRadius: 8, padding: 10, marginBottom: 15, borderWidth: 1, borderColor: '#0f3460' },
  statsText: { color: '#aaa', fontSize: 12 },
  exportSection: { backgroundColor: '#16213e', borderRadius: 10, padding: 12, marginBottom: 15, borderWidth: 1, borderColor: '#0f3460' },
  exportTitle: { color: '#00ff88', fontSize: 13, fontWeight: 'bold', marginBottom: 8 },
  exportRow: { flexDirection: 'row', gap: 8 },
  exportBtn: { flex: 1, backgroundColor: '#0a2a1a', borderRadius: 8, padding: 10, alignItems: 'center', borderWidth: 1, borderColor: '#00ff88' },
  exportBtnText: { color: '#00ff88', fontSize: 13, fontWeight: 'bold' },
  fileListTitle: { color: '#888', fontSize: 13, marginBottom: 8 },
  fileList: { flex: 1 },
  fileItem: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', backgroundColor: '#16213e', borderRadius: 8, padding: 12, marginBottom: 6, borderWidth: 1, borderColor: '#0f3460' },
  fileItemSelected: { borderColor: '#00ff88', backgroundColor: '#0a2a1a' },
  fileInfo: { flex: 1 },
  fileName: { color: '#e0e0e0', fontSize: 13, fontWeight: 'bold' },
  fileDate: { color: '#666', fontSize: 10, marginTop: 2 },
  fileStats: { alignItems: 'flex-end' },
  fileSize: { color: '#00ff88', fontSize: 12 },
  fileEvents: { color: '#888', fontSize: 10 },
  emptyText: { color: '#555', textAlign: 'center', marginTop: 20 },
  footer: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 10 },
});