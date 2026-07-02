// screens/StorageScreen.tsx — SD file browser + download over BLE
// Author: jayis1   License: GPL-2.0
import React, { useState } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet } from 'react-native';
import { useStore } from '../src/store';
import { ble } from '../src/ble';

interface FileEntry { name: string; size: number; }

export default function StorageScreen() {
  const [files, setFiles] = useState<FileEntry[]>([]);
  const [downloading, setDownloading] = useState<string | null>(null);
  const { connected, sdFreeKB } = useStore();

  const refresh = async () => {
    if (connected) {
      await ble.sendCommand('{"cmd":"list","dir":"/aurora/frames"}');
      useStore.getState().pushLog('SD', 'listed /aurora/frames');
    }
    // Demo entries
    setFiles([
      { name: '/aurora/frames/100023.dat', size: 512 * 256 },
      { name: '/aurora/frames/100523.dat', size: 512 * 128 },
      { name: '/aurora/events/100023.evt', size: 1024 * 1024 * 8 },
      { name: '/aurora/log.txt', size: 4096 },
    ]);
  };

  const download = async (name: string) => {
    setDownloading(name);
    if (connected) {
      await ble.sendCommand(JSON.stringify({ cmd: 'download', path: name }));
      useStore.getState().pushLog('SD', 'download ' + name);
    }
    setTimeout(() => setDownloading(null), 2000);
  };

  return (
    <View style={s.container}>
      <Text style={s.title}>Storage</Text>
      <Text style={s.hint}>SD free: {sdFreeKB} KB</Text>
      <TouchableOpacity style={s.btn} onPress={refresh}>
        <Text style={s.btnText}>Refresh list</Text>
      </TouchableOpacity>
      <FlatList
        data={files}
        keyExtractor={(item) => item.name}
        renderItem={({ item }) => (
          <View style={s.row}>
            <View style={{ flex: 1 }}>
              <Text style={s.fname}>{item.name}</Text>
              <Text style={s.fsize}>{(item.size / 1024).toFixed(1)} KB</Text>
            </View>
            <TouchableOpacity style={s.dlBtn} onPress={() => download(item.name)}>
              <Text style={s.dlText}>
                {downloading === item.name ? '…' : 'Get'}
              </Text>
            </TouchableOpacity>
          </View>
        )}
        ListEmptyComponent={<Text style={s.empty}>No files. Press Refresh.</Text>}
      />
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#7fd4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 4 },
  hint: { color: '#888', fontSize: 12, marginBottom: 12 },
  btn: { backgroundColor: '#1a3a5c', padding: 12, borderRadius: 8, marginBottom: 12, alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 15, fontWeight: '600' },
  row: { flexDirection: 'row', alignItems: 'center', paddingVertical: 10, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  fname: { color: '#ddd', fontSize: 13 },
  fsize: { color: '#555', fontSize: 11 },
  dlBtn: { backgroundColor: '#2a2a4e', paddingHorizontal: 16, paddingVertical: 8, borderRadius: 6 },
  dlText: { color: '#7fd4ff', fontSize: 13, fontWeight: '600' },
  empty: { color: '#555', textAlign: 'center', marginTop: 20 },
});