/**
 * TRACE-REAPER — Live trace + correlation screen
 *
 * Shows the live power trace as a sparkline and the per-byte best-guess key
 * with its correlation coefficient, updating as the device pushes
 * notifications (~5 Hz).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useContext, useEffect, useState } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { ConnectionContext, hex, rhoColor } from '../utils/protocol';
import TraceChart from '../components/TraceChart';

export default function LiveTraceScreen() {
  const conn = useContext(ConnectionContext);
  const [snap, setSnap] = useState(conn ? conn._snapshot() : null);

  useEffect(() => {
    if (!conn) return;
    return conn.subscribe(() => setSnap(conn._snapshot()));
  }, [conn]);

  const s = snap || {};
  const samples = s.liveTrace ? Array.from(s.liveTrace) : [];
  const corr = s.corrBest || { best: [], rho: [], nbytes: 0 };
  const nbytes = corr.nbytes || 16;

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Live Acquisition</Text>

      <Text style={styles.label}>Power trace (live)</Text>
      <TraceChart samples={samples} color="#00e5ff" height={90} />

      <Text style={styles.label}>Per-byte best hypothesis (CPA)</Text>
      <ScrollView style={styles.list}>
        {Array.from({ length: nbytes }).map((_, i) => {
          const k = corr.best ? corr.best[i] : 0;
          const r = corr.rho ? corr.rho[i] : 0;
          const color = rhoColor(r);
          return (
            <View key={i} style={styles.row}>
              <Text style={styles.byteIdx}>byte {i.toString(16).padStart(2,'0')}</Text>
              <Text style={styles.byteVal}>{k !== undefined ? k.toString(16).padStart(2,'0') : '--'}</Text>
              <View style={styles.barWrap}>
                <View style={[styles.bar, { width: `${Math.min(100, Math.abs(r) * 200)}%`, backgroundColor: color }]} />
              </View>
              <Text style={[styles.rho, { color }]}>{(r || 0).toFixed(3)}</Text>
            </View>
          );
        })}
      </ScrollView>

      <Text style={styles.hint}>
        ρ &gt; 0.5 = green (recovered). ρ &gt; 0.25 = amber. Below = red.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1622', padding: 16 },
  title: { color: '#00e5ff', fontSize: 20, fontWeight: '700', marginTop: 16, marginBottom: 8 },
  label: { color: '#9fb1c7', fontSize: 12, marginTop: 12, marginBottom: 4 },
  list: { marginTop: 8, flex: 1 },
  row: { flexDirection: 'row', alignItems: 'center', paddingVertical: 4 },
  byteIdx: { color: '#9fb1c7', width: 64, fontSize: 12 },
  byteVal: { color: '#e6edf3', width: 40, fontFamily: 'monospace', fontSize: 13, fontWeight: '700' },
  barWrap: { flex: 1, height: 10, backgroundColor: '#13243a', borderRadius: 5, marginHorizontal: 8, overflow: 'hidden' },
  bar: { height: '100%', borderRadius: 5 },
  rho: { width: 60, textAlign: 'right', fontFamily: 'monospace', fontSize: 12 },
  hint: { color: '#5d7088', fontSize: 10, marginTop: 10 },
});