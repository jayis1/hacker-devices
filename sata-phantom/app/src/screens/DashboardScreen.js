/**
 * DashboardScreen.js — Real-Time SATA Traffic Dashboard
 * Author: jayis1
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import { LineChart } from 'react-native-chart-kit';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import DeviceStatus from '../components/DeviceStatus';
import TrafficChart from '../components/TrafficChart';
import { getStatus, getTrafficStats } from '../services/api';

const DashboardScreen = ({ route }) => {
  const [status, setStatus] = useState({
    mode: 0,
    linkUp: false,
    speed: 0,
    battery: 3700,
    framesRead: 0,
    framesWrite: 0,
    rulesActive: 0,
    framesCaptured: 0,
    bytesExfiltrated: 0,
    uptime: 0,
  });
  const [trafficData, setTrafficData] = useState({ labels: [], reads: [], writes: [] });
  const [refreshing, setRefreshing] = useState(false);

  const fetchData = useCallback(async () => {
    try {
      const s = await getStatus();
      setStatus(s);
      const t = await getTrafficStats();
      setTrafficData(prev => {
        const labels = [...(prev.labels || []), t.timestamp].slice(-20);
        const reads = [...(prev.reads || []), t.readsPerSec].slice(-20);
        const writes = [...(prev.writes || []), t.writesPerSec].slice(-20);
        return { labels, reads, writes };
      });
    } catch (e) {
      console.error('Dashboard fetch error:', e);
    }
  }, []);

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 2000);
    return () => clearInterval(interval);
  }, [fetchData]);

  const modeColors = ['#4a4', '#48f', '#f44', '#fa4', '#888', '#f0f'];
  const modeNames = ['Transparent', 'Monitor', 'Active', 'Exfil', 'Sleep', 'USB Config'];

  return (
    <View style={styles.container}>
      <DeviceStatus status={status} />

      <View style={styles.statsGrid}>
        <View style={[styles.statCard, { borderLeftColor: '#48f' }]}>
          <Icon name="file-download" size={20} color="#48f" />
          <Text style={styles.statValue}>{status.framesRead.toLocaleString()}</Text>
          <Text style={styles.statLabel}>Read Frames</Text>
        </View>
        <View style={[styles.statCard, { borderLeftColor: '#f44' }]}>
          <Icon name="file-upload" size={20} color="#f44" />
          <Text style={styles.statValue}>{status.framesWrite.toLocaleString()}</Text>
          <Text style={styles.statLabel}>Write Frames</Text>
        </View>
      </View>

      <View style={styles.statsGrid}>
        <View style={[styles.statCard, { borderLeftColor: '#fa4' }]}>
          <Icon name="target" size={20} color="#fa4" />
          <Text style={styles.statValue}>{status.framesCaptured}</Text>
          <Text style={styles.statLabel}>Captured</Text>
        </View>
        <View style={[styles.statCard, { borderLeftColor: '#f0f' }]}>
          <Icon name="upload-network" size={20} color="#f0f" />
          <Text style={styles.statValue}>{status.bytesExfiltrated}</Text>
          <Text style={styles.statLabel}>Bytes Exfil'd</Text>
        </View>
      </View>

      <View style={styles.chartContainer}>
        <Text style={styles.chartTitle}>I/O Throughput (frames/sec)</Text>
        <TrafficChart data={trafficData} />
      </View>

      <View style={styles.infoRow}>
        <Text style={styles.infoLabel}>Uptime:</Text>
        <Text style={styles.infoValue}>{status.uptime}s</Text>
        <Text style={styles.infoLabel}>Speed:</Text>
        <Text style={styles.infoValue}>
          {status.speed === 2 ? '6.0 Gbps' : status.speed === 1 ? '3.0 Gbps' : '1.5 Gbps'}
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a', padding: 12 },
  statsGrid: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  statCard: {
    flex: 1,
    backgroundColor: '#12122a',
    borderRadius: 10,
    padding: 12,
    borderLeftWidth: 3,
  },
  statValue: { color: '#fff', fontSize: 22, fontWeight: 'bold', marginTop: 4 },
  statLabel: { color: '#888', fontSize: 11, marginTop: 2 },
  chartContainer: {
    backgroundColor: '#12122a',
    borderRadius: 10,
    padding: 12,
    marginBottom: 8,
  },
  chartTitle: { color: '#aaa', fontSize: 13, marginBottom: 8 },
  infoRow: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#12122a', borderRadius: 8, padding: 10,
  },
  infoLabel: { color: '#888', fontSize: 12, marginRight: 4 },
  infoValue: { color: '#e0e0e0', fontSize: 12, marginRight: 16 },
});

export default DashboardScreen;
