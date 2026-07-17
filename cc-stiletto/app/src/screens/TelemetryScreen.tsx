// src/screens/TelemetryScreen.tsx — VBUS voltage/current + thermal dashboard.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React, { useEffect } from 'react';
import { View, Text, Pressable, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../state/DeviceContext';
import { ConnectionBar } from '../components/ConnectionBar';

function Gauge({ label, value, unit, warn }: { label: string; value: string; unit: string; warn?: boolean }) {
  return (
    <View style={[styles.gauge, warn ? { borderColor: '#d62828' } : null]}>
      <Text style={styles.gaugeLabel}>{label}</Text>
      <Text style={[styles.gaugeValue, warn ? { color: '#d62828' } : null]}>{value}</Text>
      <Text style={styles.gaugeUnit}>{unit}</Text>
    </View>
  );
}

export default function TelemetryScreen() {
  const { telemetry, send, lastLog } = useDevice();

  useEffect(() => {
    const id = setInterval(() => send({ cmd: 'telemetry' }), 1000);
    return () => clearInterval(id);
  }, [send]);

  const t = telemetry;
  const tempWarn = !!t && t.temp_c > 70;
  const tempCrit = !!t && t.temp_c > 85;

  return (
    <ScrollView style={styles.container}>
      <ConnectionBar />
      <Text style={styles.h1}>Power Path Telemetry</Text>

      <View style={styles.grid}>
        <Gauge label="VBUS source" value={t ? String(t.src_mv) : '—'} unit="mV" warn={!!t && t.src_mv > 20000} />
        <Gauge label="VBUS sink"   value={t ? String(t.snk_mv) : '—'} unit="mV" warn={!!t && t.snk_mv > 20000} />
        <Gauge label="I source"    value={t ? String(t.src_ma) : '—'} unit="mA" />
        <Gauge label="I sink"      value={t ? String(t.snk_ma) : '—'} unit="mA" />
        <Gauge label="MOSFET temp" value={t ? String(t.temp_c) : '—'} unit="°C" warn={tempWarn} />
        <Gauge label="eFuse"       value={t ? (t.efuse_fault ? 'FAULT' : 'OK') : '—'} unit="" warn={!!t && t.efuse_fault} />
      </View>

      <Pressable style={styles.btn} onPress={() => send({ cmd: 'disarm' })}>
        <Text style={styles.btnText}>Emergency Disarm</Text>
      </Pressable>

      <Text style={styles.h2}>Last device log</Text>
      <Text style={styles.log}>{lastLog ?? '(no output yet)'}</Text>

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d0d' },
  h1:        { color: '#eee', fontSize: 16, fontWeight: 'bold', padding: 12 },
  h2:        { color: '#bbb', fontSize: 13, fontWeight: 'bold', padding: 12, marginTop: 8 },
  grid:      { flexDirection: 'row', flexWrap: 'wrap', paddingHorizontal: 8 },
  gauge:     { width: '46%', margin: 8, backgroundColor: '#1a1a1a', borderRadius: 8, padding: 14, borderWidth: 1, borderColor: '#333', alignItems: 'center' },
  gaugeLabel:{ color: '#999', fontSize: 11, marginBottom: 6 },
  gaugeValue:{ color: '#eee', fontSize: 22, fontWeight: 'bold' },
  gaugeUnit:{ color: '#888', fontSize: 12, marginTop: 4 },
  btn:       { backgroundColor: '#d62828', margin: 16, padding: 14, borderRadius: 8, alignItems: 'center' },
  btnText:   { color: 'white', fontWeight: 'bold' },
  log:       { color: '#6c8', fontFamily: 'monospace', fontSize: 11, paddingHorizontal: 12, paddingBottom: 20 },
});