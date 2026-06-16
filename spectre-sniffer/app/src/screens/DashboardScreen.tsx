//==============================================================================
// DashboardScreen.tsx — Spectre-Sniffer Dashboard
// Author: jayis1
// Description: Main dashboard showing device status, battery, signal strength,
//              and quick controls for the Spectre-Sniffer.
//==============================================================================

import React, { useEffect, useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  RefreshControl,
  ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { SpectreAPI } from '../services/api';
import { DeviceStatus, OperatingMode, OperatingModeNames } from '../types';

//==============================================================================
// Dashboard Screen Component
//==============================================================================
const DashboardScreen: React.FC = () => {
  const [status, setStatus] = useState<DeviceStatus | null>(null);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const fetchStatus = useCallback(async () => {
    try {
      const data = await SpectreAPI.getStatus();
      setStatus(data);
      setError(null);
    } catch (err) {
      setError('Device not connected');
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  }, []);

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 5000);
    return () => clearInterval(interval);
  }, [fetchStatus]);

  const onRefresh = useCallback(() => {
    setRefreshing(true);
    fetchStatus();
  }, [fetchStatus]);

  //============================================================================
  // Battery Indicator
  //============================================================================
  const renderBattery = () => {
    if (!status) return null;
    const pct = status.batteryPercent;
    const color = pct > 50 ? '#00E676' : pct > 20 ? '#FFC107' : '#FF5252';
    const iconName =
      pct > 80
        ? 'battery'
        : pct > 60
        ? 'battery-70'
        : pct > 40
        ? 'battery-50'
        : pct > 20
        ? 'battery-30'
        : 'battery-10';

    return (
      <View style={styles.statusCard}>
        <Icon name={iconName} size={48} color={color} />
        <Text style={[styles.statusValue, { color }]}>{pct}%</Text>
        <Text style={styles.statusLabel}>Battery</Text>
        <Text style={styles.statusSubtext}>
          {status.batteryMillivolts} mV
        </Text>
      </View>
    );
  };

  //============================================================================
  // Mode Indicator
  //============================================================================
  const renderMode = () => {
    if (!status) return null;
    const modeName = OperatingModeNames[status.mode as OperatingMode] || 'Unknown';

    return (
      <View style={styles.statusCard}>
        <Icon
          name={
            status.captureActive ? 'record-rec' : 'radar'
          }
          size={48}
          color={status.captureActive ? '#FF5252' : '#00E676'}
        />
        <Text style={styles.statusValue}>{modeName}</Text>
        <Text style={styles.statusLabel}>Mode</Text>
        {status.captureActive && (
          <Text style={styles.statusSubtext}>
            {status.samplesCaptured.toLocaleString()} samples
          </Text>
        )}
      </View>
    );
  };

  //============================================================================
  // RF Status
  //============================================================================
  const renderRFStatus = () => {
    if (!status) return null;
    const freqMHz = (status.centerFrequencyHz / 1e6).toFixed(1);
    const bwMHz = (status.bandwidthHz / 1e6).toFixed(1);

    return (
      <View style={styles.statusCard}>
        <Icon name="radio-tower" size={48} color="#448AFF" />
        <Text style={styles.statusValue}>{freqMHz} MHz</Text>
        <Text style={styles.statusLabel}>Center Freq</Text>
        <Text style={styles.statusSubtext}>
          BW: {bwMHz} MHz | Gain: {status.lnaGainDb} dB
        </Text>
      </View>
    );
  };

  //============================================================================
  // System Info
  //============================================================================
  const renderSystemInfo = () => {
    if (!status) return null;
    const uptime = formatUptime(status.uptimeSeconds);

    return (
      <View style={styles.infoCard}>
        <Text style={styles.infoTitle}>System Information</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>FPGA Temperature</Text>
          <Text style={styles.infoValue}>{status.fpgaTemperature.toFixed(1)}°C</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Uptime</Text>
          <Text style={styles.infoValue}>{uptime}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>SD Card</Text>
          <Text style={styles.infoValue}>
            {formatBytes(status.sdFreeBytes)} free / {formatBytes(status.sdTotalBytes)}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>WiFi Signal</Text>
          <Text style={styles.infoValue}>{status.wifiSignalDbm} dBm</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Connected Clients</Text>
          <Text style={styles.infoValue}>{status.wifiClients}</Text>
        </View>
      </View>
    );
  };

  //============================================================================
  // Quick Actions
  //============================================================================
  const renderQuickActions = () => {
    return (
      <View style={styles.actionsContainer}>
        <Text style={styles.sectionTitle}>Quick Actions</Text>
        <View style={styles.actionsRow}>
          <TouchableOpacity style={styles.actionButton} onPress={() => {}}>
            <Icon name="record-rec" size={32} color="#FF5252" />
            <Text style={styles.actionLabel}>Start Capture</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionButton} onPress={() => {}}>
            <Icon name="stop" size={32} color="#FF5252" />
            <Text style={styles.actionLabel}>Stop</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionButton} onPress={() => {}}>
            <Icon name="camera" size={32} color="#448AFF" />
            <Text style={styles.actionLabel}>Tempest</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.actionsRow}>
          <TouchableOpacity style={styles.actionButton} onPress={() => {}}>
            <Icon name="lock-outline" size={32} color="#FFC107" />
            <Text style={styles.actionLabel}>Crypto</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionButton} onPress={() => {}}>
            <Icon name="keyboard" size={32} color="#00E676" />
            <Text style={styles.actionLabel}>Keystroke</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionButton} onPress={() => {}}>
            <Icon name="calibrate" size={32} color="#E040FB" />
            <Text style={styles.actionLabel}>Calibrate</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  };

  //============================================================================
  // Render
  //============================================================================
  if (loading) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" color="#00E676" />
        <Text style={styles.loadingText}>Connecting to Spectre-Sniffer...</Text>
      </View>
    );
  }

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl
          refreshing={refreshing}
          onRefresh={onRefresh}
          tintColor="#00E676"
        />
      }
    >
      {/* Connection Status Banner */}
      {error && (
        <View style={styles.errorBanner}>
          <Icon name="alert-circle-outline" size={20} color="#fff" />
          <Text style={styles.errorText}>{error}</Text>
        </View>
      )}

      {/* Device Name */}
      <Text style={styles.deviceName}>Spectre-Sniffer</Text>
      <Text style={styles.deviceSubtitle}>EM Side-Channel Analysis Platform</Text>

      {/* Status Cards */}
      <View style={styles.statusRow}>
        {renderBattery()}
        {renderMode()}
        {renderRFStatus()}
      </View>

      {/* System Info */}
      {renderSystemInfo()}

      {/* Quick Actions */}
      {renderQuickActions()}

      {/* Footer */}
      <View style={styles.footer}>
        <Text style={styles.footerText}>Spectre-Sniffer v1.0</Text>
        <Text style={styles.footerText}>Author: jayis1</Text>
        <Text style={styles.footerWarning}>
          For authorized security research use only
        </Text>
      </View>
    </ScrollView>
  );
};

//==============================================================================
// Utility Functions
//==============================================================================
function formatUptime(seconds: number): string {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;

  if (days > 0) return `${days}d ${hours}h ${mins}m`;
  if (hours > 0) return `${hours}h ${mins}m ${secs}s`;
  if (mins > 0) return `${mins}m ${secs}s`;
  return `${secs}s`;
}

function formatBytes(bytes: number): string {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
}

//==============================================================================
// Styles
//==============================================================================
const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  loadingContainer: {
    flex: 1,
    backgroundColor: '#0f0f23',
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    color: '#aaa',
    fontSize: 16,
    marginTop: 16,
  },
  errorBanner: {
    backgroundColor: '#FF5252',
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    borderRadius: 8,
    marginBottom: 16,
  },
  errorText: {
    color: '#fff',
    fontSize: 14,
    marginLeft: 8,
    fontWeight: '600',
  },
  deviceName: {
    color: '#fff',
    fontSize: 28,
    fontWeight: 'bold',
    textAlign: 'center',
  },
  deviceSubtitle: {
    color: '#888',
    fontSize: 14,
    textAlign: 'center',
    marginBottom: 24,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 16,
  },
  statusCard: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    alignItems: 'center',
    marginHorizontal: 4,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  statusValue: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
    marginTop: 8,
  },
  statusLabel: {
    color: '#888',
    fontSize: 12,
    marginTop: 4,
  },
  statusSubtext: {
    color: '#666',
    fontSize: 11,
    marginTop: 2,
  },
  infoCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  infoTitle: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a4e',
  },
  infoLabel: {
    color: '#888',
    fontSize: 14,
  },
  infoValue: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  sectionTitle: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  actionsContainer: {
    marginBottom: 24,
  },
  actionsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 12,
  },
  actionButton: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    alignItems: 'center',
    width: 100,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  actionLabel: {
    color: '#fff',
    fontSize: 12,
    marginTop: 8,
  },
  footer: {
    alignItems: 'center',
    paddingVertical: 24,
  },
  footerText: {
    color: '#555',
    fontSize: 12,
  },
  footerWarning: {
    color: '#FF5252',
    fontSize: 11,
    marginTop: 8,
    textAlign: 'center',
  },
});

export default DashboardScreen;
