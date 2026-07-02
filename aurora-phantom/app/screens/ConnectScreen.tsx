// screens/ConnectScreen.tsx — BLE scan & pair
// Author: jayis1   License: GPL-2.0
import React, { useState } from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet } from 'react-native';
import { ble } from '../src/ble';
import { useStore } from '../src/store';

export default function ConnectScreen() {
  const [devices, setDevices] = useState<any[]>([]);
  const [scanning, setScanning] = useState(false);
  const { connected, setConnected, batteryPct, sdFreeKB, mode } = useStore();

  const onScan = async () => {
    setScanning(true);
    const found = await ble.scan(5000);
    setDevices(found);
    setScanning(false);
  };

  const onConnect = async (id: string) => {
    await ble.connect(id);
    setConnected(true, id);
    useStore.getState().pushLog('BLE', 'connected to ' + id);
  };

  const onDisconnect = async () => {
    await ble.disconnect();
    setConnected(false);
  };

  return (
    <View style={s.container}>
      <Text style={s.title}>Aurora-Phantom</Text>
      <Text style={s.sub}>author: jayis1 · v1.0.0</Text>

      <View style={s.statusRow}>
        <Text style={s.label}>State:</Text>
        <Text style={s.value}>{connected ? 'CONNECTED' : 'disconnected'}</Text>
      </View>
      <View style={s.statusRow}>
        <Text style={s.label}>Mode:</Text>
        <Text style={s.value}>{mode}</Text>
      </View>
      <View style={s.statusRow}>
        <Text style={s.label}>Battery:</Text>
        <Text style={s.value}>{batteryPct}%</Text>
      </View>
      <View style={s.statusRow}>
        <Text style={s.label}>SD free:</Text>
        <Text style={s.value}>{sdFreeKB} KB</Text>
      </View>

      {connected ? (
        <TouchableOpacity style={s.btnDanger} onPress={onDisconnect}>
          <Text style={s.btnText}>Disconnect</Text>
        </TouchableOpacity>
      ) : (
        <TouchableOpacity style={s.btn} onPress={onScan} disabled={scanning}>
          <Text style={s.btnText}>{scanning ? 'Scanning…' : 'Scan for devices'}</Text>
        </TouchableOpacity>
      )}

      <FlatList
        data={devices}
        keyExtractor={(item) => item.id}
        renderItem={({ item }) => (
          <TouchableOpacity style={s.deviceRow} onPress={() => onConnect(item.id)}>
            <Text style={s.deviceName}>{item.name ?? 'aurora-phantom'}</Text>
            <Text style={s.deviceId}>{item.id}</Text>
          </TouchableOpacity>
        )}
        ListEmptyComponent={<Text style={s.empty}>No devices found.</Text>}
      />
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#7fd4ff', fontSize: 24, fontWeight: 'bold', marginTop: 24 },
  sub: { color: '#555', fontSize: 12, marginBottom: 24 },
  statusRow: { flexDirection: 'row', marginBottom: 8 },
  label: { color: '#888', width: 100, fontSize: 14 },
  value: { color: '#ddd', fontSize: 14 },
  btn: { backgroundColor: '#1a3a5c', padding: 14, borderRadius: 8, marginVertical: 12, alignItems: 'center' },
  btnDanger: { backgroundColor: '#5c1a1a', padding: 14, borderRadius: 8, marginVertical: 12, alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
  deviceRow: { padding: 12, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  deviceName: { color: '#7fd4ff', fontSize: 15 },
  deviceId: { color: '#555', fontSize: 12 },
  empty: { color: '#555', textAlign: 'center', marginTop: 20 },
});