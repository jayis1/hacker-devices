/**
 * VBUSGauge.js — Animated VBUS voltage/current gauge widget
 *
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function VBUSGauge({ mv, ma }) {
  const volts = (mv / 1000).toFixed(2);
  const amps = (ma / 1000).toFixed(2);
  const watts = ((mv * ma) / 1000000).toFixed(2);

  /* Voltage bar: 0–20V mapped to 0–100% */
  const vPct = Math.min((mv / 20000) * 100, 100);
  /* Current bar: 0–5A mapped to 0–100% */
  const aPct = Math.min((ma / 5000) * 100, 100);

  /* Color based on voltage level */
  const vColor = mv > 15000 ? '#FF4444' : mv > 9000 ? '#FFAA00' : '#4CAF50';

  return (
    <View style={styles.container}>
      <View style={styles.gaugeRow}>
        <Text style={styles.gaugeLabel}>VBUS</Text>
        <Text style={[styles.gaugeValue, { color: vColor }]}>{volts} V</Text>
      </View>
      <View style={styles.barContainer}>
        <View style={[styles.bar, { width: `${vPct}%`, backgroundColor: vColor }]} />
      </View>

      <View style={styles.gaugeRow}>
        <Text style={styles.gaugeLabel}>Current</Text>
        <Text style={styles.gaugeValue}>{amps} A</Text>
      </View>
      <View style={styles.barContainer}>
        <View style={[styles.bar, { width: `${aPct}%`, backgroundColor: '#2196F3' }]} />
      </View>

      <View style={styles.powerRow}>
        <Text style={styles.powerLabel}>Power:</Text>
        <Text style={styles.powerValue}>{watts} W</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#16213e', borderRadius: 10, padding: 16,
    marginBottom: 16, borderWidth: 1, borderColor: '#333',
  },
  gaugeRow: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'baseline',
    marginBottom: 4,
  },
  gaugeLabel: { color: '#888', fontSize: 14 },
  gaugeValue: { color: '#fff', fontSize: 24, fontWeight: 'bold' },
  barContainer: {
    height: 8, backgroundColor: '#0a0a14', borderRadius: 4,
    marginBottom: 12, overflow: 'hidden',
  },
  bar: { height: '100%', borderRadius: 4 },
  powerRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    borderTopWidth: 1, borderTopColor: '#333', paddingTop: 8,
  },
  powerLabel: { color: '#888', fontSize: 14 },
  powerValue: { color: '#FF6600', fontSize: 18, fontWeight: 'bold' },
});

/* end of file — author: jayis1 */