/**
 * TRACE-REAPER — Results screen
 *
 * Shows the final recovered key, per-byte confidence, recovered byte count,
 * and provides export + raw-trace dump request buttons.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useContext, useEffect, useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert, Share } from 'react-native';
import { ConnectionContext, hex, rhoColor } from '../utils/protocol';

export default function ResultsScreen() {
  const conn = useContext(ConnectionContext);
  const [snap, setSnap] = useState(conn ? conn._snapshot() : null);

  useEffect(() => {
    if (!conn) return;
    return conn.subscribe(() => setSnap(conn._snapshot()));
  }, [conn]);

  const s = snap || {};
  const r = s.result;
  const nbytes = r ? r.nbytes : 0;

  const requestResult = async () => {
    try { await conn.getResult(); }
    catch (e) { Alert.alert('Failed', String(e)); }
  };

  const dumpTrace = async () => {
    const idx = (s.status && s.status.traces) ? s.status.traces - 1 : 0;
    try { await conn.dumpTrace(idx); }
    catch (e) { Alert.alert('Dump failed', String(e)); }
  };

  const exportKey = async () => {
    if (!r) return;
    const keyHex = r.bestKey.map(b => b.toString(16).padStart(2, '0')).join(' ');
    try {
      await Share.share({ message: `TRACE-REAPER recovered key (${r.nbytes} bytes):\n${keyHex}\nrecovered bytes: ${r.recoveredBytes}/${r.nbytes}\nconfidence ok: ${r.confidenceOk ? 'yes' : 'no'}\n\nby jayis1 — GPL-2.0` });
    } catch (e) { /* ignore */ }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Result</Text>
      {!r && (
        <View>
          <Text style={styles.empty}>No result yet. Run a session first.</Text>
          <TouchableOpacity style={styles.btn} onPress={requestResult}>
            <Text style={styles.btnText}>Request latest result</Text>
          </TouchableOpacity>
        </View>
      )}
      {r && (
        <View>
          <Text style={styles.label}>Recovered key ({r.nbytes} bytes)</Text>
          <Text style={styles.keyHex}>{hex(r.bestKey)}</Text>

          <Text style={styles.label}>Per-byte confidence</Text>
          {Array.from({ length: nbytes }).map((_, i) => {
            const rho = r.rho[i] || 0;
            const color = rhoColor(rho);
            return (
              <View key={i} style={styles.row}>
                <Text style={styles.byteIdx}>byte {i.toString(16).padStart(2,'0')}</Text>
                <View style={styles.barWrap}>
                  <View style={[styles.bar, { width: `${Math.min(100, Math.abs(rho) * 200)}%`, backgroundColor: color }]} />
                </View>
                <Text style={[styles.rho, { color }]}>{rho.toFixed(3)}</Text>
              </View>
            );
          })}

          <Text style={styles.summary}>
            Recovered: {r.recoveredBytes}/{r.nbytes} bytes ·
            {' '}Confidence: {r.confidenceOk ? 'OK' : 'incomplete'}
          </Text>

          <View style={styles.buttons}>
            <TouchableOpacity style={styles.btn} onPress={dumpTrace}>
              <Text style={styles.btnText}>Dump last trace</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.btn} onPress={exportKey}>
              <Text style={styles.btnText}>Export key</Text>
            </TouchableOpacity>
          </View>
        </View>
      )}
      <Text style={styles.disclaimer}>
        Authorized security research only. Handle recovered secret material
        per your authorization scope and retention policy. — jayis1
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1622', padding: 16 },
  title: { color: '#00e5ff', fontSize: 20, fontWeight: '700', marginTop: 16, marginBottom: 8 },
  empty: { color: '#9fb1c7', fontSize: 13, marginBottom: 12 },
  label: { color: '#9fb1c7', fontSize: 12, marginTop: 14, marginBottom: 4 },
  keyHex: { color: '#2ecc71', fontFamily: 'monospace', fontSize: 14, backgroundColor: '#13243a', padding: 10, borderRadius: 6 },
  row: { flexDirection: 'row', alignItems: 'center', paddingVertical: 3 },
  byteIdx: { color: '#9fb1c7', width: 64, fontSize: 12 },
  barWrap: { flex: 1, height: 10, backgroundColor: '#13243a', borderRadius: 5, marginHorizontal: 8, overflow: 'hidden' },
  bar: { height: '100%', borderRadius: 5 },
  rho: { width: 60, textAlign: 'right', fontFamily: 'monospace', fontSize: 12 },
  summary: { color: '#e6edf3', fontSize: 13, marginTop: 14, fontWeight: '600' },
  buttons: { flexDirection: 'row', marginTop: 18, gap: 8 },
  btn: { backgroundColor: '#1f6feb', padding: 12, borderRadius: 8, marginRight: 8 },
  btnText: { color: '#fff', fontWeight: '700', fontSize: 13 },
  disclaimer: { color: '#5d7088', fontSize: 10, marginTop: 24, lineHeight: 14 },
});