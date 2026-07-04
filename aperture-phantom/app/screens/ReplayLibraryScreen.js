/**
 * AperturePhantom — ReplayLibraryScreen
 *
 * Lists .APF replay files stored on the device's SD card, lets the operator
 * pick one to replay (freeze-frame attack), and stop replay.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useEffect, useState } from 'react';
import { View, Text, Button, FlatList, StyleSheet, TouchableOpacity } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ReplayLibraryScreen() {
  const { replayList, replayStart, replayStop, send } = useDevice();
  const [files, setFiles] = useState([]);
  const [replaying, setReplaying] = useState(false);

  /* Request the file list on mount. We rely on a future GET response to
   * populate the list; in a fuller build we'd register a one-shot handler. */
  useEffect(() => { refresh(); }, []);

  const refresh = () => {
    replayList();
    /* For the design reference we also seed a placeholder list to show UI. */
    setFiles([
      { name: 'CAP00000.APF', size: 614400 },
      { name: 'CAP00001.APF', size: 614400 },
      { name: 'FACE_AUTH.APF', size: 307200 },
      { name: 'EMPTY_ROAD.APF', size: 921600 },
    ]);
  };

  const onReplay = (idx) => {
    replayStart(idx);
    setReplaying(true);
  };

  const onStop = () => {
    replayStop();
    setReplaying(false);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>Replay Library (SD card)</Text>
      <View style={styles.row}>
        <Button title="Refresh" onPress={refresh} />
        {replaying && <Button title="Stop replay" color="#a04040" onPress={onStop} />}
      </View>

      <FlatList
        data={files}
        keyExtractor={(f, i) => f.name + i}
        renderItem={({ item, index }) => (
          <TouchableOpacity style={styles.row2} onPress={() => onReplay(index)}>
            <Text style={styles.name}>{item.name}</Text>
            <Text style={styles.size}>{(item.size / 1024).toFixed(0)} KB</Text>
          </TouchableOpacity>
        )}
        ListEmptyComponent={<Text style={styles.empty}>No replay files.</Text>}
      />

      <Text style={styles.warn}>
        ⚠ Replay holds the chosen frame indefinitely; the host CV pipeline will
        process the recorded scene instead of reality. Authorized testing only.
      </Text>
      <Text style={styles.author}>author: jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#101418' },
  h1: { color: '#d0e0f0', fontSize: 16, fontWeight: '600', marginBottom: 8 },
  row: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  row2: { flexDirection: 'row', justifyContent: 'space-between',
          paddingVertical: 12, borderBottomColor: '#202830',
          borderBottomWidth: 1 },
  name: { color: '#d0e0f0', fontSize: 13, fontFamily: 'monospace' },
  size: { color: '#8aa0b8', fontSize: 11 },
  empty: { color: '#607080', fontSize: 12, paddingVertical: 8 },
  warn: { color: '#e0a040', fontSize: 11, marginTop: 16, fontStyle: 'italic' },
  author: { color: '#5a7088', fontSize: 10, marginTop: 12 },
});