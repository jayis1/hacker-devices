/**
 * ConnectScreen.js — BLE scan / pair / connect.
 *
 * Author: jayis1
 * License: GPLv3
 */
import React, { useEffect } from 'react';
import { View, Text, Button, FlatList, TouchableOpacity, StyleSheet } from 'react-native';
import { usePhantom } from '../src/PhantomContext';

export default function ConnectScreen() {
  const { scan, devices, connect, connected, device, status, disconnect } = usePhantom();

  useEffect(() => { scan(); }, []);

  return (
    <View style={s.wrap}>
      <Text style={s.h1}>BACnet Phantom</Text>
      <Text style={s.author}>companion — jayis1</Text>
      <Text style={s.status}>status: {status}</Text>
      <Text style={s.disclaimer}>
        ⚠ Authorized security research only. BACnet controls HVAC and
        life-safety equipment. Do not use on systems you do not own.
      </Text>

      {!connected ? (
        <>
          <Button title="Rescan" onPress={scan} />
          <FlatList
            data={devices}
            keyExtractor={(d) => d.id}
            renderItem={({ item }) => (
              <TouchableOpacity style={s.row} onPress={() => connect(item)}>
                <Text style={s.name}>{item.name}</Text>
                <Text style={s.id}>{item.id}</Text>
              </TouchableOpacity>
            )}
            ListEmptyComponent={<Text style={s.empty}>No BACNET-PHANTOM-* devices found.</Text>}
          />
        </>
      ) : (
        <>
          <Text style={s.ok}>● paired: {device?.name}</Text>
          <Button title="Disconnect" color="#e63946" onPress={disconnect} />
        </>
      )}
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, padding: 20, backgroundColor: '#0f1115' },
  h1: { fontSize: 24, fontWeight: 'bold', color: '#e63946', marginTop: 8 },
  author: { color: '#888', marginBottom: 8 },
  status: { color: '#ccc', marginBottom: 8 },
  disclaimer: {
    color: '#f4a261', fontSize: 12, marginBottom: 16,
    borderWidth: 1, borderColor: '#f4a261', padding: 8, borderRadius: 4,
  },
  row: { padding: 16, borderBottomWidth: 1, borderColor: '#222' },
  name: { color: '#fff', fontSize: 16 },
  id: { color: '#666', fontSize: 11 },
  empty: { color: '#666', padding: 16 },
  ok: { color: '#06d6a0', fontSize: 16, marginVertical: 16 },
});