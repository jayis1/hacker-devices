/*
 * FlexRay Sentinel React Native companion application
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React, { useMemo, useState } from 'react';
import { FlatList, Pressable, SafeAreaView, StyleSheet, Text, TextInput, View } from 'react-native';

const seedEvents = [
  { id: '1', slot: 44, cycle: 12, score: 82, kind: 'Timing drift', detail: 'Arrival +2.7 us outside baseline' },
  { id: '2', slot: 301, cycle: 12, score: 68, kind: 'Length change', detail: 'Expected 16 bytes, observed 24' },
  { id: '3', slot: 77, cycle: 13, score: 31, kind: 'New cycle', detail: 'Slot valid but first observation in cycle 13' },
];

function Pill({ label, active, onPress }) {
  return <Pressable accessibilityRole="button" onPress={onPress} style={[styles.pill, active && styles.pillActive]}><Text style={styles.pillText}>{label}</Text></Pressable>;
}

function StatusScreen() {
  return <View style={styles.panel}>
    <Text style={styles.title}>Live bus health</Text>
    <View style={styles.metricRow}><Metric label="Channel A" value="10.0 Mbit/s" tone="good" /><Metric label="Channel B" value="10.0 Mbit/s" tone="good" /></View>
    <View style={styles.metricRow}><Metric label="Learned slots" value="138" /><Metric label="Anomalies" value="2" tone="warn" /></View>
    <Text style={styles.section}>Cluster state</Text>
    <Text style={styles.body}>Cycle 13 · static slot 312 · sync nodes 4 · capture loss 0</Text>
    <Text style={styles.notice}>Receive-only monitoring is active. No bus transmitter is connected to application controls.</Text>
  </View>;
}

function Metric({ label, value, tone }) {
  return <View style={styles.metric}><Text style={styles.muted}>{label}</Text><Text style={[styles.metricValue, tone === 'good' && styles.good, tone === 'warn' && styles.warn]}>{value}</Text></View>;
}

function EventScreen() {
  const [minimum, setMinimum] = useState('50');
  const filtered = useMemo(() => seedEvents.filter((event) => event.score >= Number(minimum || 0)), [minimum]);
  return <View style={styles.panel}>
    <Text style={styles.title}>Evidence timeline</Text>
    <Text style={styles.label}>Minimum anomaly score</Text>
    <TextInput accessibilityLabel="Minimum anomaly score" keyboardType="numeric" value={minimum} onChangeText={setMinimum} style={styles.input} />
    <FlatList data={filtered} keyExtractor={(item) => item.id} renderItem={({ item }) => <View style={styles.event}>
      <View><Text style={styles.eventTitle}>Slot {item.slot} · cycle {item.cycle}</Text><Text style={styles.muted}>{item.kind} — {item.detail}</Text></View>
      <Text style={[styles.score, item.score >= 70 && styles.danger]}>{item.score}</Text>
    </View>} ListEmptyComponent={<Text style={styles.muted}>No events at this threshold.</Text>} />
  </View>;
}

function BaselineScreen() {
  const [frozen, setFrozen] = useState(true);
  const [privacy, setPrivacy] = useState(true);
  return <View style={styles.panel}>
    <Text style={styles.title}>Baseline policy</Text>
    <Text style={styles.body}>A baseline maps slot identity, cycle membership, channel, length, timing distribution, and a non-reversible payload fingerprint.</Text>
    <Toggle label="Freeze learned schedule" value={frozen} onChange={setFrozen} />
    <Toggle label="Hash payload evidence" value={privacy} onChange={setPrivacy} />
    <Text style={styles.notice}>Changing policy requires a physical confirmation press on the device. Raw payload storage is disabled by default.</Text>
  </View>;
}

function Toggle({ label, value, onChange }) {
  return <Pressable onPress={() => onChange(!value)} style={styles.toggle}><Text style={styles.body}>{label}</Text><Text style={value ? styles.good : styles.muted}>{value ? 'ON' : 'OFF'}</Text></Pressable>;
}

export default function App() {
  const [tab, setTab] = useState('Status');
  return <SafeAreaView style={styles.root}>
    <View style={styles.header}><Text style={styles.brand}>FLEXRAY SENTINEL</Text><Text style={styles.connected}>● USB CONNECTED</Text></View>
    <View style={styles.tabs}>{['Status', 'Events', 'Baseline'].map((name) => <Pill key={name} label={name} active={tab === name} onPress={() => setTab(name)} />)}</View>
    {tab === 'Status' && <StatusScreen />}{tab === 'Events' && <EventScreen />}{tab === 'Baseline' && <BaselineScreen />}
    <Text style={styles.footer}>Designed by jayis1 · authorized laboratory use only</Text>
  </SafeAreaView>;
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#071018', padding: 18 }, header: { marginBottom: 16 }, brand: { color: '#62e6ff', fontWeight: '800', fontSize: 22, letterSpacing: 2 },
  connected: { color: '#68f5a4', marginTop: 5, fontSize: 12 }, tabs: { flexDirection: 'row', gap: 8, marginBottom: 14 }, pill: { backgroundColor: '#172431', paddingVertical: 9, paddingHorizontal: 15, borderRadius: 18 },
  pillActive: { backgroundColor: '#16637a' }, pillText: { color: '#e7f6fa', fontWeight: '700' }, panel: { flex: 1, backgroundColor: '#0e1a24', borderRadius: 14, padding: 16 }, title: { color: '#fff', fontSize: 20, fontWeight: '700', marginBottom: 14 },
  section: { color: '#62e6ff', fontSize: 15, fontWeight: '700', marginTop: 18, marginBottom: 5 }, body: { color: '#d4e4e9', lineHeight: 21 }, metricRow: { flexDirection: 'row', gap: 10, marginBottom: 10 }, metric: { flex: 1, padding: 13, backgroundColor: '#142532', borderRadius: 10 },
  metricValue: { color: '#fff', fontSize: 18, fontWeight: '700', marginTop: 5 }, muted: { color: '#8fa8b3' }, good: { color: '#68f5a4', fontWeight: '700' }, warn: { color: '#ffd166' }, danger: { color: '#ff667c' }, notice: { color: '#bdd4db', backgroundColor: '#112d38', padding: 12, borderRadius: 8, marginTop: 18, lineHeight: 19 },
  label: { color: '#8fa8b3', marginBottom: 6 }, input: { color: '#fff', borderColor: '#2d4b59', borderWidth: 1, borderRadius: 8, padding: 10, marginBottom: 14 }, event: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 12, borderBottomColor: '#233641', borderBottomWidth: 1 }, eventTitle: { color: '#fff', fontWeight: '700', marginBottom: 4 }, score: { color: '#ffd166', fontWeight: '800', fontSize: 18 },
  toggle: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 15, borderBottomColor: '#233641', borderBottomWidth: 1 }, footer: { color: '#66808b', fontSize: 11, textAlign: 'center', paddingTop: 12 },
});
