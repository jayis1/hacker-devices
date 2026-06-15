/**
 * AnalysisScreen.js — Statistical analysis and visualization for glitch results
 * Histograms, success rate tracking, parameter sweep visualization
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext, useCallback, useEffect, useMemo, useRef } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Dimensions, Alert, Share,
} from 'react-native';
import { DeviceContext } from '../App';
import { EVENT_TYPES, GLITCH_MODES } from '../utils/protocol';

const SCREEN_WIDTH = Dimensions.get('window').width;

/* ============================================================================
 * Mini Bar Chart (pure RN, no dependencies)
 * ============================================================================ */

function BarChart({ data, width = SCREEN_WIDTH - 64, height = 150, color = '#6c5ce7' }) {
  if (!data || data.length === 0) {
    return <Text style={styles.emptyChart}>No data yet</Text>;
  }

  const maxVal = Math.max(...data.map((d) => d.value), 1);
  const barWidth = Math.max(Math.floor(width / data.length) - 2, 4);

  return (
    <View style={[styles.chartContainer, { width, height }]}>
      <View style={styles.chartBars}>
        {data.map((d, i) => {
          const barH = Math.max((d.value / maxVal) * (height - 30), 2);
          return (
            <View key={i} style={styles.barWrap}>
              <View style={[styles.bar, { height: barH, width: barWidth, backgroundColor: d.color || color }]} />
              {data.length <= 20 && (
                <Text style={styles.barLabel} numberOfLines={1}>{d.label}</Text>
              )}
            </View>
          );
        })}
      </View>
    </View>
  );
}

/* ============================================================================
 * Parameter Distribution Analysis
 * ============================================================================ */

function DelayDistribution({ events }) {
  const glitchEvents = events.filter((e) => e.type === EVENT_TYPES.GLITCH_FIRED);
  const bins = useMemo(() => {
    if (glitchEvents.length === 0) return [];

    /* Bin by timestamp intervals (100ms buckets) */
    const bucketSize = 100;
    const buckets = {};
    for (const evt of glitchEvents) {
      const bucket = Math.floor(evt.timestamp / bucketSize);
      buckets[bucket] = (buckets[bucket] || 0) + 1;
    }

    const sorted = Object.entries(buckets)
      .sort(([a], [b]) => Number(a) - Number(b))
      .slice(-30);

    return sorted.map(([k, v]) => ({
      label: `${Number(k) * bucketSize}ms`,
      value: v,
    }));
  }, [glitchEvents.length]);

  return (
    <View style={styles.analysisCard}>
      <Text style={styles.cardTitle}>Glitch Timeline</Text>
      <Text style={styles.cardSubtitle}>Glitches per 100ms bucket</Text>
      <BarChart data={bins} height={120} color="#6c5ce7" />
      <Text style={styles.statText}>Total: {glitchEvents.length} glitches</Text>
    </View>
  );
}

/* ============================================================================
 * Success Rate Tracker
 * ============================================================================ */

function SuccessRateTracker({ results }) {
  const total = results?.glitchCount ?? 0;
  const successes = results?.successCount ?? 0;
  const rate = total > 0 ? (successes / total * 100).toFixed(2) : '0.00';

  return (
    <View style={styles.analysisCard}>
      <Text style={styles.cardTitle}>Success Rate</Text>
      <View style={styles.rateDisplay}>
        <Text style={styles.rateValue}>{rate}%</Text>
        <Text style={styles.rateDetail}>{successes} / {total} glitches</Text>
      </View>
      {/* Visual bar */}
      <View style={styles.rateBar}>
        <View style={[styles.rateBarFill, { width: `${parseFloat(rate)}%` }]} />
      </View>
    </View>
  );
}

/* ============================================================================
 * Fault Analysis
 * ============================================================================ */

function FaultAnalysis({ results }) {
  const faultFlags = results?.faultFlags ?? 0;
  const faults = [];

  if (faultFlags & 0x01) faults.push({ label: 'ADC Overcurrent', severity: 'critical' });
  if (faultFlags & 0x02) faults.push({ label: 'FPGA Fault', severity: 'warning' });
  if (faultFlags & 0x04) faults.push({ label: 'Overtemperature', severity: 'critical' });
  if (faultFlags & 0x08) faults.push({ label: 'Power Rail Fault', severity: 'critical' });
  if (faultFlags & 0x10) faults.push({ label: 'Glitch Timeout', severity: 'warning' });

  return (
    <View style={styles.analysisCard}>
      <Text style={styles.cardTitle}>Fault Status</Text>
      {faults.length === 0 ? (
        <Text style={styles.noFaults}>✅ No active faults</Text>
      ) : (
        faults.map((f, i) => (
          <View key={i} style={[styles.faultRow, f.severity === 'critical' && styles.faultCritical]}>
            <Text style={styles.faultIcon}>{f.severity === 'critical' ? '🔴' : '🟡'}</Text>
            <Text style={styles.faultLabel}>{f.label}</Text>
          </View>
        ))
      )}
    </View>
  );
}

/* ============================================================================
 * Voltage Analysis
 * ============================================================================ */

function VoltageAnalysis({ results }) {
  const vcc = results?.targetVccMv ?? 0;
  const current = results?.shuntCurrentMa ?? 0;

  /* Categorize VCC level */
  let vccStatus = 'normal';
  let vccColor = '#55efc4';
  if (vcc < 500) { vccStatus = 'very low'; vccColor = '#ff6b6b'; }
  else if (vcc < 2000) { vccStatus = 'low'; vccColor = '#ffeaa7'; }
  else if (vcc > 4000) { vccStatus = 'high'; vccColor = '#ffeaa7'; }

  /* Categorize current */
  let currStatus = 'normal';
  let currColor = '#55efc4';
  if (Math.abs(current) > 400) { currStatus = 'high'; currColor = '#ff6b6b'; }
  else if (Math.abs(current) > 200) { currStatus = 'elevated'; currColor = '#ffeaa7'; }

  return (
    <View style={styles.analysisCard}>
      <Text style={styles.cardTitle}>Voltage / Current at Last Glitch</Text>
      <View style={styles.voltRow}>
        <View style={styles.voltItem}>
          <Text style={styles.voltLabel}>VCC</Text>
          <Text style={[styles.voltValue, { color: vccColor }]}>{vcc} mV</Text>
          <Text style={[styles.voltStatus, { color: vccColor }]}>{vccStatus}</Text>
        </View>
        <View style={styles.voltDivider} />
        <View style={styles.voltItem}>
          <Text style={styles.voltLabel}>Current</Text>
          <Text style={[styles.voltValue, { color: currColor }]}>{current} mA</Text>
          <Text style={[styles.voltStatus, { color: currColor }]}>{currStatus}</Text>
        </View>
      </View>
    </View>
  );
}

/* ============================================================================
 * Mode Summary
 * ============================================================================ */

function ModeSummary({ results }) {
  const modeNames = {
    [GLITCH_MODES.VOLTAGE_GLITCH]: '⚡ Voltage Glitch',
    [GLITCH_MODES.EM_GLITCH]: '🧲 EM Glitch',
    [GLITCH_MODES.CLOCK_GLITCH]: '⏱ Clock Glitch',
    [GLITCH_MODES.TRIGGER_SCAN]: '👁 Trigger Scan',
    [GLITCH_MODES.CALIBRATION]: '🔧 Calibration',
  };

  const mode = results?.modeActive ?? 0;

  return (
    <View style={styles.analysisCard}>
      <Text style={styles.cardTitle}>Active Mode</Text>
      <Text style={styles.modeText}>{modeNames[mode] || `Mode ${mode}`}</Text>
      <Text style={styles.modeArmed}>
        {results?.armed ? '🟡 ARMED' : '⚪ Disarmed'}
      </Text>
    </View>
  );
}

/* ============================================================================
 * Data Export
 * ============================================================================ */

function DataExport({ results, events }) {
  const handleExport = useCallback(async () => {
    const data = {
      exported: new Date().toISOString(),
      results: results,
      eventCount: events.length,
      lastEvents: events.slice(-50).map((e) => ({
        type: e.type,
        data: e.data,
        timestamp: e.timestamp,
      })),
    };

    try {
      await Share.share({
        message: JSON.stringify(data, null, 2),
        title: 'VoltGlitcher Export',
      });
    } catch (e) {
      Alert.alert('Export failed', e.message);
    }
  }, [results, events]);

  return (
    <TouchableOpacity style={styles.exportBtn} onPress={handleExport}>
      <Text style={styles.exportBtnText}>📤 Export Data</Text>
    </TouchableOpacity>
  );
}

/* ============================================================================
 * Main AnalysisScreen Component
 * ============================================================================ */

function AnalysisScreen() {
  const { deviceState, results, events, glitchConfig } = useContext(DeviceContext);

  const isConnected = deviceState !== 'disconnected';

  if (!isConnected) {
    return (
      <View style={styles.container}>
        <Text style={styles.disconnected}>Connect a device to view analysis</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
      {/* Success Rate */}
      <SuccessRateTracker results={results} />

      {/* Glitch Timeline */}
      <DelayDistribution events={events} />

      {/* Voltage/Current Analysis */}
      <VoltageAnalysis results={results} />

      {/* Fault Analysis */}
      <FaultAnalysis results={results} />

      {/* Mode Summary */}
      <ModeSummary results={results} />

      {/* Quick Stats */}
      <View style={styles.analysisCard}>
        <Text style={styles.cardTitle}>Quick Stats</Text>
        <View style={styles.statsGrid}>
          <Text style={styles.statRow}>
            Last trigger timestamp: {results?.lastTimestamp ?? 'N/A'}
          </Text>
          <Text style={styles.statRow}>
            FPGA status: 0x{(results?.fpgaStatus ?? 0).toString(16).padStart(2, '0')}
          </Text>
          <Text style={styles.statRow}>
            Board temperature: {results?.temperature ?? 'N/A'}°C
          </Text>
          <Text style={styles.statRow}>
            Events buffered: {events.length}
          </Text>
        </View>
      </View>

      {/* Export */}
      <DataExport results={results} events={events} />
    </ScrollView>
  );
}

/* ============================================================================
 * Styles
 * ============================================================================ */

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0f' },
  scrollContent: { padding: 16, paddingBottom: 40 },
  disconnected: { color: '#555', fontSize: 16, textAlign: 'center', marginTop: 60 },
  analysisCard: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16, marginBottom: 12,
  },
  cardTitle: { color: '#6c5ce7', fontSize: 16, fontWeight: '700', marginBottom: 8 },
  cardSubtitle: { color: '#555', fontSize: 12, marginBottom: 8 },
  emptyChart: { color: '#444', fontSize: 13, textAlign: 'center', padding: 30 },
  chartContainer: { marginVertical: 4 },
  chartBars: { flexDirection: 'row', alignItems: 'flex-end', justifyContent: 'center', flex: 1 },
  barWrap: { alignItems: 'center', marginHorizontal: 1 },
  bar: { borderRadius: 2 },
  barLabel: { color: '#555', fontSize: 8, marginTop: 4, maxWidth: 30, textAlign: 'center' },
  statText: { color: '#888', fontSize: 12, marginTop: 8, textAlign: 'center' },
  rateDisplay: { alignItems: 'center', marginVertical: 8 },
  rateValue: { color: '#55efc4', fontSize: 36, fontWeight: '900' },
  rateDetail: { color: '#666', fontSize: 14, marginTop: 4 },
  rateBar: {
    height: 8, backgroundColor: '#252540', borderRadius: 4,
    marginTop: 8, overflow: 'hidden',
  },
  rateBarFill: { height: '100%', backgroundColor: '#55efc4', borderRadius: 4 },
  noFaults: { color: '#55efc4', fontSize: 14, textAlign: 'center', padding: 8 },
  faultRow: {
    flexDirection: 'row', alignItems: 'center',
    paddingVertical: 6, borderBottomWidth: 1, borderBottomColor: '#252540',
  },
  faultCritical: { backgroundColor: 'rgba(255,107,107,0.1)' },
  faultIcon: { marginRight: 8, fontSize: 14 },
  faultLabel: { color: '#ccc', fontSize: 14 },
  voltRow: { flexDirection: 'row', justifyContent: 'space-around', alignItems: 'center' },
  voltItem: { alignItems: 'center', flex: 1 },
  voltLabel: { color: '#666', fontSize: 12, marginBottom: 4 },
  voltValue: { fontSize: 20, fontWeight: '700' },
  voltStatus: { fontSize: 11, marginTop: 2 },
  voltDivider: { width: 1, height: 50, backgroundColor: '#252540' },
  modeText: { color: '#fff', fontSize: 18, fontWeight: '600', textAlign: 'center' },
  modeArmed: { color: '#888', fontSize: 13, textAlign: 'center', marginTop: 4 },
  statsGrid: { gap: 4 },
  statRow: { color: '#888', fontSize: 13, lineHeight: 22 },
  exportBtn: {
    backgroundColor: '#252540', borderRadius: 10, padding: 16,
    alignItems: 'center', marginBottom: 12,
  },
  exportBtnText: { color: '#6c5ce7', fontSize: 16, fontWeight: '600' },
});

export default AnalysisScreen;