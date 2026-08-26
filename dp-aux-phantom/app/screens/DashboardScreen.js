// DP AUX Phantom dashboard screen
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import StatCard from '../components/StatCard';

export default function DashboardScreen({ status }) {
  return (
    <View style={styles.wrapper}>
      <View style={styles.hero}>
        <Text style={styles.title}>Inline Display Trust Telemetry</Text>
        <Text style={styles.subtitle}>
          Observe AUX transactions, HPD behavior, DPCD negotiation, and EDID identity changes from a single field console.
        </Text>
      </View>

      <View style={styles.row}>
        <StatCard label="Active Profile" value={status.profile} hint="Policy enforced in FPGA/MCU control plane" />
        <StatCard label="Sink Identity" value={status.sinkName} hint={`${status.vendor} · ${status.serial}`} />
      </View>

      <View style={styles.row}>
        <StatCard label="Link" value={`${status.linkRate} x${status.lanes}`} hint={`HPD ${status.hpd}`} />
        <StatCard label="Wireless" value={status.radio} hint="BLE + Wi-Fi operator backhaul" />
      </View>

      <View style={styles.row}>
        <StatCard label="AUX Reads" value={String(status.auxReads)} hint={`I2C-over-AUX reads ${status.i2cReads}`} />
        <StatCard label="AUX Writes" value={String(status.auxWrites)} hint={`Rule hits ${status.ruleHits} · Alerts ${status.alerts}`} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    gap: 16,
  },
  hero: {
    backgroundColor: '#0f172a',
    borderRadius: 18,
    padding: 18,
    borderWidth: 1,
    borderColor: '#1e293b',
    gap: 10,
  },
  title: {
    color: '#f8fafc',
    fontSize: 24,
    fontWeight: '900',
  },
  subtitle: {
    color: '#94a3b8',
    lineHeight: 20,
  },
  row: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 14,
  },
});
