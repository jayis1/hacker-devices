/**
 * LogScreen.tsx — browse & export scan logs from the SD card
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet, Share } from 'react-native';
import { useBle } from '../services/BleContext';
import RNFS from 'react-native-fs';

export default function LogScreen() {
  const ble = useBle();
  const [logs, setLogs] = useState<string[]>([]);
  const [selected, setSelected] = useState<string | null>(null);
  const [content, setContent] = useState<string>('');

  // In a full app we'd enumerate the SD via a log-list characteristic;
  // here we list the app's local documents directory for any CSVs that
  // were pulled and cached.
  useEffect(() => {
    (async () => {
      const dir = RNFS.DocumentDirectoryPath;
      const files = await RNFS.readDir(dir);
      setLogs(files.filter((f) => f.name.endsWith('.csv')).map((f) => f.name));
    })();
  }, []);

  const openLog = async (name: string) => {
    setSelected(name);
    const path = `${RNFS.DocumentDirectoryPath}/${name}`;
    const txt = await RNFS.readFile(path, 'utf8');
    setContent(txt);
  };

  const shareLog = async () => {
    if (!selected) return;
    const path = `${RNFS.DocumentDirectoryPath}/${selected}`;
    await Share.share({ url: path, title: selected });
  };

  return (
    <View style={styles.container}>
      <Text style={styles.section}>Scan Logs</Text>
      <FlatList
        data={logs}
        keyExtractor={(item) => item}
        renderItem={({ item }) => (
          <TouchableOpacity
            style={[styles.logItem, selected === item && styles.logItemActive]}
            onPress={() => openLog(item)}
          >
            <Text style={styles.logName}>{item}</Text>
          </TouchableOpacity>
        )}
        ListEmptyComponent={<Text style={styles.empty}>No logs cached. Pull from device via the Log characteristic.</Text>}
      />

      {selected && (
        <>
          <Text style={styles.section}>Preview: {selected}</Text>
          <View style={styles.previewBox}>
            <Text style={styles.previewText}>{content.slice(0, 4000)}</Text>
          </View>
          <TouchableOpacity style={styles.button} onPress={shareLog}>
            <Text style={styles.buttonText}>Export / Share</Text>
          </TouchableOpacity>
        </>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16 },
  section: { color: '#e94560', fontSize: 14, fontWeight: '700', marginBottom: 8 },
  logItem: { backgroundColor: '#16213e', padding: 12, borderRadius: 6, marginVertical: 4, borderWidth: 1, borderColor: '#333' },
  logItemActive: { borderColor: '#e94560' },
  logName: { color: '#a0a0c0', fontFamily: 'monospace' },
  empty: { color: '#666', fontSize: 12, padding: 20, textAlign: 'center' },
  previewBox: { backgroundColor: '#16213e', borderRadius: 8, padding: 10, flex: 1, borderWidth: 1, borderColor: '#333' },
  previewText: { color: '#4ecca3', fontFamily: 'monospace', fontSize: 10 },
  button: { backgroundColor: '#16213e', padding: 14, borderRadius: 8, alignItems: 'center', marginVertical: 8, borderWidth: 1, borderColor: '#e94560' },
  buttonText: { color: '#e94560', fontSize: 16, fontWeight: '600' },
});