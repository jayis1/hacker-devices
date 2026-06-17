/**
 * DashboardScreen.js — FiberPhantom Dashboard
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Displays live device status: connection, battery, deployment mode,
 * optical link rate, capture statistics, and recent alerts.
 */

import React from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, RefreshControl } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import StatusIndicator from '../components/StatusIndicator';
import { formatBytes, formatUptime } from '../utils/protocol';

export default function DashboardScreen({ ble, connected, scanning, status, alerts, onScan }) {
  const [refreshing, setRefreshing] = React.useState(false);

  const onRefresh = async () => {
    setRefreshing(true);
    if (connected) {
      await ble.getStatus();
      await ble.getStats();
    }
    setRefreshing(false);
  };

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} tintColor="#00ff88" />
      }
    >
      {/* Connection Status */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="connection" size={24} color="#00ff88" />
          <Text style={styles.cardTitle}>Connection</Text>
        </View>
        <StatusIndicator
          label="BLE Status"
          value={connected ? 'Connected' : (scanning ? 'Scanning...' : 'Disconnected')}
          status={connected ? 'good' : (scanning ? 'pending' : 'error')}
        />
        <StatusIndicator
          label="FPGA"
          value={status.fpgaReady ? 'Ready' : 'Not Configured'}
          status={status.fpgaReady ? 'good' : 'error'}
        />
        {!connected && (
          <TouchableOpacity style={styles.button} onPress={onScan}>
            <Text style={styles.buttonText}>
              {scanning ? 'Scanning...' : 'Scan for Device'}
            </Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Deployment Mode */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="screwdriver-top" size={24} color="#58a6ff" />
          <Text style={styles.cardTitle}>Deployment</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Mode</Text>
          <Text style={styles.value}>{status.mode}</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>State</Text>
          <Text style={[styles.value, { color: getStateColor(status.state) }]}>
            {status.state}
          </Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Link Rate</Text>
          <Text style={[styles.value, { color: status.linkRate !== 'Down' ? '#00ff88' : '#f85149' }]}>
            {status.linkRate}
          </Text>
        </View>
      </View>

      {/* Power */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="battery" size={24} color="#3fb950" />
          <Text style={styles.cardTitle}>Power</Text>
        </View>
        <View style={styles.batteryContainer}>
          <View style={styles.batteryBar}>
            <View
              style={[
                styles.batteryFill,
                {
                  width: `${status.batteryPct}%`,
                  backgroundColor: status.batteryPct > 20 ? '#3fb950' : '#f85149',
                },
              ]}
            />
          </View>
          <Text style={styles.batteryText}>
            {status.batteryPct}%{status.charging ? ' ⚡' : ''}
          </Text>
        </View>
      </View>

      {/* Storage */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="sd" size={24} color="#d29922" />
          <Text style={styles.cardTitle}>Storage</Text>
        </View>
        <StatusIndicator
          label="SD Card"
          value={status.sdInserted ? 'Inserted' : 'Not Present'}
          status={status.sdInserted ? 'good' : 'error'}
        />
        <View style={styles.row}>
          <Text style={styles.label}>Free Space</Text>
          <Text style={styles.value}>{formatBytes(status.sdFreeMB * 1024 * 1024)}</Text>
        </View>
      </View>

      {/* Capture Statistics */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="chart-bar" size={24} color="#a371f7" />
          <Text style={styles.cardTitle}>Capture Statistics</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Total Frames</Text>
          <Text style={styles.value}>{status.stats.totalFrames.toLocaleString()}</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Dropped Frames</Text>
          <Text style={[styles.value, { color: status.stats.droppedFrames > 0 ? '#f85149' : '#e6edf3' }]}>
            {status.stats.droppedFrames.toLocaleString()}
          </Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>MITM Modified</Text>
          <Text style={styles.value}>{status.stats.mitmModified.toLocaleString()}</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>MITM Dropped</Text>
          <Text style={styles.value}>{status.stats.mitmDropped.toLocaleString()}</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>CRC Errors</Text>
          <Text style={[styles.value, { color: status.stats.crcErrors > 0 ? '#d29922' : '#e6edf3' }]}>
            {status.stats.crcErrors.toLocaleString()}
          </Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Uptime</Text>
          <Text style={styles.value}>{formatUptime(status.stats.uptimeSec)}</Text>
        </View>
      </View>

      {/* Recent Alerts */}
      {alerts.length > 0 && (
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Icon name="alert-circle" size={24} color="#f85149" />
            <Text style={styles.cardTitle}>Recent Alerts</Text>
          </View>
          {alerts.slice(-5).map((alert, i) => (
            <View key={alert.id} style={styles.alertRow}>
              <Icon
                name="alert"
                size={16}
                color={alert.severity >= 2 ? '#f85149' : '#d29922'}
              />
              <Text style={styles.alertText}>{alert.title}</Text>
            </View>
          ))}
        </View>
      )}
    </ScrollView>
  );
}

function getStateColor(state) {
  switch (state) {
    case 'Capturing': return '#3fb950';
    case 'MITM Active': return '#f85149';
    case 'Idle': return '#8b949e';
    case 'SD Full': return '#d29922';
    case 'Low Battery': return '#f85149';
    case 'Error': return '#f85149';
    default: return '#e6edf3';
  }
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1117',
    padding: 12,
  },
  card: {
    backgroundColor: '#161b22',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#30363d',
  },
  cardHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 12,
    gap: 8,
  },
  cardTitle: {
    color: '#e6edf3',
    fontSize: 16,
    fontWeight: 'bold',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  label: {
    color: '#8b949e',
    fontSize: 14,
  },
  value: {
    color: '#e6edf3',
    fontSize: 14,
    fontWeight: '500',
  },
  batteryContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 8,
  },
  batteryBar: {
    flex: 1,
    height: 20,
    backgroundColor: '#21262d',
    borderRadius: 10,
    marginRight: 12,
    borderWidth: 1,
    borderColor: '#30363d',
    overflow: 'hidden',
  },
  batteryFill: {
    height: '100%',
    borderRadius: 10,
  },
  batteryText: {
    color: '#e6edf3',
    fontSize: 14,
    fontWeight: 'bold',
    minWidth: 60,
  },
  button: {
    backgroundColor: '#238636',
    borderRadius: 8,
    padding: 12,
    marginTop: 12,
    alignItems: 'center',
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  alertRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 6,
    gap: 8,
  },
  alertText: {
    color: '#e6edf3',
    fontSize: 13,
  },
});