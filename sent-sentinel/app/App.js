/* SENT Sentinel React Native companion
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React, { useMemo, useState } from 'react';
import {
  SafeAreaView, ScrollView, StyleSheet, Text, TouchableOpacity, View,
} from 'react-native';

const COLORS = { bg: '#071017', panel: '#10222c', ink: '#e5f7ff', cyan: '#58d6ff', amber: '#ffbf69', red: '#ff6b6b', muted: '#8aa6b3' };
const seedEvents = [
  { id: 1, channel: 0, score: 76, kind: 'analog mismatch', value: 2035, analog: 4180, time: '00:14.202' },
  { id: 2, channel: 2, score: 52, kind: 'timing shift', value: 882, analog: 1080, time: '00:14.894' },
  { id: 3, channel: 0, score: 47, kind: 'counter discontinuity', value: 2041, analog: 2493, time: '00:15.206' },
];

function Metric({ label, value, tone = COLORS.cyan }) {
  return <View style={styles.metric}><Text style={styles.metricLabel}>{label}</Text><Text style={[styles.metricValue, { color: tone }]}>{value}</Text></View>;
}

function StatusScreen() {
  return <ScrollView contentContainerStyle={styles.content}>
    <Text style={styles.title}>Passive capture</Text>
    <Text style={styles.subtitle}>USB connected · baseline frozen · raw nibble storage off</Text>
    <View style={styles.grid}>
      <Metric label="Valid frames" value="184,220" />
      <Metric label="Anomalies" value="3" tone={COLORS.amber} />
      <Metric label="CRC rejects" value="17" tone={COLORS.red} />
      <Metric label="Capture loss" value="0" />
    </View>
    {[0, 1, 2, 3].map(channel => <View key={channel} style={styles.card}>
      <View style={styles.row}><Text style={styles.cardTitle}>SENT channel {channel}</Text><Text style={styles.good}>LOCKED</Text></View>
      <Text style={styles.body}>Tick {(3.000 + channel * 0.002).toFixed(3)} µs · 1.000 ms period</Text>
      <Text style={styles.body}>Fast value {1020 + channel * 341} · analog {1240 + channel * 410} mV</Text>
    </View>)}
  </ScrollView>;
}

function EventsScreen() {
  const [minimum, setMinimum] = useState(50);
  const visible = useMemo(() => seedEvents.filter(event => event.score >= minimum), [minimum]);
  return <ScrollView contentContainerStyle={styles.content}>
    <Text style={styles.title}>Evidence timeline</Text>
    <View style={styles.row}><Text style={styles.subtitle}>Minimum score: {minimum}</Text>
      <TouchableOpacity style={styles.smallButton} onPress={() => setMinimum(minimum === 50 ? 0 : 50)}><Text style={styles.buttonText}>Toggle filter</Text></TouchableOpacity>
    </View>
    {visible.map(event => <View key={event.id} style={styles.card}>
      <View style={styles.row}><Text style={styles.cardTitle}>{event.kind}</Text><Text style={styles.alert}>{event.score}/100</Text></View>
      <Text style={styles.body}>CH{event.channel} · {event.time} · value {event.value} · analog {event.analog} mV</Text>
      <Text style={styles.note}>Signed summary only; no raw sensor payload retained.</Text>
    </View>)}
  </ScrollView>;
}

function BaselineScreen() {
  const [staged, setStaged] = useState(null);
  return <ScrollView contentContainerStyle={styles.content}>
    <Text style={styles.title}>Baseline control</Text>
    <Text style={styles.subtitle}>Changes are staged here and require the physical button on SENT Sentinel.</Text>
    <View style={styles.card}><Text style={styles.cardTitle}>Current policy</Text>
      <Text style={styles.body}>16+ observations per channel · 3.5% timing tolerance</Text>
      <Text style={styles.body}>120 mV analog-correlation tolerance · threshold 45</Text>
      <Text style={styles.body}>Learned channels: 0, 1, 2, 3</Text></View>
    {['Start a new learning window', 'Freeze current baseline', 'Export signed summary'].map(action =>
      <TouchableOpacity key={action} style={styles.action} onPress={() => setStaged(action)}><Text style={styles.buttonText}>{action}</Text></TouchableOpacity>)}
    <Text style={styles.note}>{staged ? `Staged: ${staged}. Confirm locally on device.` : 'No change staged.'}</Text>
  </ScrollView>;
}

export default function App() {
  const [screen, setScreen] = useState('Status');
  const screens = { Status: <StatusScreen />, Events: <EventsScreen />, Baseline: <BaselineScreen /> };
  return <SafeAreaView style={styles.root}><View style={styles.header}><Text style={styles.brand}>SENT SENTINEL</Text><Text style={styles.author}>by jayis1</Text></View>
    {screens[screen]}
    <View style={styles.tabs}>{Object.keys(screens).map(name => <TouchableOpacity key={name} style={[styles.tab, screen === name && styles.activeTab]} onPress={() => setScreen(name)}><Text style={styles.buttonText}>{name}</Text></TouchableOpacity>)}</View>
  </SafeAreaView>;
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: COLORS.bg }, header: { padding: 18, borderBottomWidth: 1, borderColor: '#21404e' }, brand: { color: COLORS.ink, fontSize: 22, fontWeight: '800', letterSpacing: 2 }, author: { color: COLORS.muted, marginTop: 3 }, content: { padding: 18, paddingBottom: 90 }, title: { color: COLORS.ink, fontSize: 25, fontWeight: '700' }, subtitle: { color: COLORS.muted, marginTop: 5, marginBottom: 16, lineHeight: 20 }, grid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' }, metric: { width: '48%', backgroundColor: COLORS.panel, padding: 14, marginBottom: 10, borderRadius: 8 }, metricLabel: { color: COLORS.muted }, metricValue: { fontSize: 24, fontWeight: '700', marginTop: 5 }, card: { backgroundColor: COLORS.panel, padding: 15, borderRadius: 8, marginTop: 10 }, cardTitle: { color: COLORS.ink, fontWeight: '700', fontSize: 16 }, body: { color: COLORS.ink, marginTop: 7 }, note: { color: COLORS.muted, marginTop: 12, lineHeight: 19 }, row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' }, good: { color: COLORS.cyan, fontWeight: '700' }, alert: { color: COLORS.amber, fontWeight: '800' }, smallButton: { backgroundColor: '#1e5368', padding: 9, borderRadius: 7 }, action: { backgroundColor: '#1e5368', padding: 15, borderRadius: 8, marginTop: 10 }, buttonText: { color: COLORS.ink, fontWeight: '700' }, tabs: { position: 'absolute', bottom: 0, left: 0, right: 0, flexDirection: 'row', backgroundColor: '#0b1921', borderTopWidth: 1, borderColor: '#21404e' }, tab: { flex: 1, padding: 17, alignItems: 'center' }, activeTab: { borderTopWidth: 3, borderColor: COLORS.cyan },
});
