/**
 * DashboardScreen.js — live P2/P3 bar graph, ratio, classification, battery
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React from 'react';
import { View, Text, StyleSheet, ProgressBarAndroid } from 'react-native';
import { MODES, CLASSIFY } from '../utils/protocol';

const MODE_NAMES = {
  [MODES.IDLE]: 'IDLE',
  [MODES.QUIET_RX]: 'QUIET RX',
  [MODES.SWEEP_CW]: 'CW SWEEP',
  [MODES.SWEEP_PULSED]: 'PULSED',
  [MODES.CALIBRATE]: 'CALIBRATE',
  [MODES.FAULT]: 'FAULT',
};

const VERDICT_COLORS = {
  [CLASSIFY.NONE]: '#666',
  [CLASSIFY.SEMI]: '#ff3333',
  [CLASSIFY.METAL]: '#888',
  [CLASSIFY.AMBIGUOUS]: '#ffaa00',
};

const VERDICT_LABELS = {
  [CLASSIFY.NONE]: '—',
  [CLASSIFY.SEMI]: 'SEMICONDUCTOR',
  [CLASSIFY.METAL]: 'DISSIMILAR METAL',
  [CLASSIFY.AMBIGUOUS]: 'AMBIGUOUS',
};

function dbfsToPct(db) {
  // Map -120..0 dBFS to 0..1
  const pct = (db + 120) / 120;
  return Math.max(0, Math.min(1, pct));
}

export default function DashboardScreen({ route, ble, status }) {
  const s = status || { mode: 0, p2_dbfs: -120, p3_dbfs: -120, ratio_db: 0,
                         classify: 0, tx_power_dbm: 0, batt_pct: 0, hit_count: 0 };

  return (
    <View style={styles.container}>
      <View style={styles.headerRow}>
        <Text style={styles.modeLabel}>{MODE_NAMES[s.mode] || 'UNKNOWN'}</Text>
        <Text style={styles.battLabel}>🔋 {s.batt_pct}%</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>2f₀ Channel (4.8 GHz)</Text>
        <ProgressBarAndroid
          styleAttr="Horizontal"
          color="#00ff88"
          progress={dbfsToPct(s.p2_dbfs)}
        />
        <Text style={styles.dbLabel}>{s.p2_dbfs} dBFS</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>3f₀ Channel (7.2 GHz)</Text>
        <ProgressBarAndroid
          styleAttr="Horizontal"
          color="#0088ff"
          progress={dbfsToPct(s.p3_dbfs)}
        />
        <Text style={styles.dbLabel}>{s.p3_dbfs} dBFS</Text>
      </View>

      <View style={styles.ratioRow}>
        <Text style={styles.ratioLabel}>Ratio 2f/3f:</Text>
        <Text style={styles.ratioValue}>{s.ratio_db > 0 ? '+' : ''}{s.ratio_db} dB</Text>
      </View>

      <View style={[styles.verdictBox, { borderColor: VERDICT_COLORS[s.classify] }]}>
        <Text style={[styles.verdictText, { color: VERDICT_COLORS[s.classify] }]}>
          {VERDICT_LABELS[s.classify]}
        </Text>
      </View>

      <View style={styles.footerRow}>
        <Text style={styles.footerText}>TX: {s.tx_power_dbm} dBm</Text>
        <Text style={styles.footerText}>Hits: {s.hit_count}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
    padding: 16,
  },
  headerRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 12,
  },
  modeLabel: {
    color: '#00ff88',
    fontSize: 18,
    fontWeight: 'bold',
  },
  battLabel: {
    color: '#888',
    fontSize: 14,
  },
  section: {
    marginBottom: 20,
  },
  sectionTitle: {
    color: '#aaa',
    fontSize: 12,
    marginBottom: 4,
  },
  dbLabel: {
    color: '#ccc',
    fontSize: 16,
    textAlign: 'right',
    marginTop: 2,
  },
  ratioRow: {
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 16,
  },
  ratioLabel: {
    color: '#888',
    fontSize: 16,
    marginRight: 8,
  },
  ratioValue: {
    color: '#fff',
    fontSize: 24,
    fontWeight: 'bold',
  },
  verdictBox: {
    borderWidth: 2,
    borderRadius: 8,
    padding: 16,
    alignItems: 'center',
    marginBottom: 16,
  },
  verdictText: {
    fontSize: 20,
    fontWeight: 'bold',
  },
  footerRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  footerText: {
    color: '#666',
    fontSize: 14,
  },
});