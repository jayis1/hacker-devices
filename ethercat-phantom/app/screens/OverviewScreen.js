// Overview screen for EtherCAT Phantom
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import MetricCard from '../components/MetricCard';

export default function OverviewScreen({ device }) {
  return (
    <View>
      <Text style={styles.sectionTitle}>Inline OT Research Dashboard</Text>
      <View style={styles.metricRow}>
        <MetricCard label="Link State" value={device.status} accent="#2ac769" />
        <MetricCard label="Cycle Budget" value={`${device.cycleBudgetNs} ns`} accent="#7e8cff" />
      </View>
      <View style={styles.metricRow}>
        <MetricCard label="Timing Margin" value={`${device.timingMarginNs} ns`} accent="#ffb347" />
        <MetricCard label="Battery" value={`${device.batteryMv} mV`} accent="#43d6ff" />
      </View>
      <Text style={styles.copy}>
        EtherCAT Phantom lets authorized operators monitor live fieldbus timing and confirm whether manipulation
        rules remain inside a deterministic safety envelope.
      </Text>
      <Text style={styles.author}>Author: jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  sectionTitle: {
    color: '#eaf3fe',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  metricRow: {
    flexDirection: 'row',
    gap: 12,
    flexWrap: 'wrap',
  },
  copy: {
    color: '#94a9c3',
    marginTop: 14,
    lineHeight: 21,
  },
  author: {
    color: '#445267',
    marginTop: 8,
    fontSize: 11,
  },
});
