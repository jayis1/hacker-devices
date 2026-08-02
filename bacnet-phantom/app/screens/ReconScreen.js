/**
 * ReconScreen.js — live BACnet device / object map.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * Sends a Who-Is on the connected Phantom, then renders the stream of
 * I-Am events captured by the firmware as a flat list. Long-press a device
 * to issue a ReadProperty against its device object's object-name.
 */
import React, { useState } from 'react';
import { View, Text, Button, FlatList, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { usePhantom } from '../src/PhantomContext';

export default function ReconScreen() {
  const { sendCommand, events, connected } = usePhantom();
  const [scanning, setScanning] = useState(false);

  const iamEvents = events
    .filter((e) => typeof e.msg === 'object' && e.msg.type === 'iam')
    .map((e) => e.msg);

  const startScan = async () => {
    if (!connected) { Alert.alert('Not connected'); return; }
    setScanning(true);
    await sendCommand({ op: 'whois', low: 0, high: 4194303 });
    setTimeout(() => setScanning(false), 3000);
  };

  const readName = (instance) =>
    sendCommand({ op: 'readprop', a1: 8, a2: instance, a3: 77 }); /* prop 77 = object-name */

  return (
    <View style={s.wrap}>
      <Text style={s.h2}>Recon</Text>
      <Text style={s.sub}>
        {scanning ? 'scanning…' : `${iamEvents.length} devices seen`}
      </Text>
      <Button title="Who-Is (0..4194303)" onPress={startScan} disabled={!connected || scanning} />

      <FlatList
        data={iamEvents}
        keyExtractor={(e, i) => `${e.instance}-${i}`}
        renderItem={({ item }) => (
          <TouchableOpacity
            style={s.row}
            onLongPress={() => readName(item.instance)}
          >
            <Text style={s.inst}>dev {item.instance}</Text>
            <Text style={s.meta}>vendor {item.vendor} · max-apdu {item.max_apdu}</Text>
          </TouchableOpacity>
        )}
        ListEmptyComponent={<Text style={s.empty}>No devices yet. Run Who-Is.</Text>}
      />
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, padding: 20, backgroundColor: '#0f1115' },
  h2: { fontSize: 20, fontWeight: 'bold', color: '#06d6a0', marginTop: 8 },
  sub: { color: '#888', marginBottom: 8 },
  row: { padding: 14, borderBottomWidth: 1, borderColor: '#222' },
  inst: { color: '#fff', fontSize: 15 },
  meta: { color: '#666', fontSize: 11 },
  empty: { color: '#666', padding: 16 },
});