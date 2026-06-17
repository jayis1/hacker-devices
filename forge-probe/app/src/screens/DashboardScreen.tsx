/**
 * DashboardScreen.tsx — Main Forge-Probe Dashboard
 * Author: jayis1
 * License: MIT
 *
 * Primary screen showing device connection status, active targets,
 * protocol information, and quick-action buttons for common operations.
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Switch,
  RefreshControl,
  ActivityIndicator,
  Alert,
} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import forgeProbeService, {
  TargetDescriptor,
  DebugProtocol,
  ForgeProbeCommand,
} from '../services/ForgeProbeService';

/* ─── Types ────────────────────────────────────────────────────────────────── */

interface DashboardProps {
  navigation: any;
}

interface ConnectionState {
  isConnected: boolean;
  connectionType: 'usb' | 'ble' | null;
  target?: TargetDescriptor;
  lastScan?: string;
  targetPresent: boolean;
  targetVoltage: number;
  targetCurrent: number;
}

/* ─── Component ────────────────────────────────────────────────────────────── */

const DashboardScreen: React.FC<DashboardProps> = ({ navigation }) => {
  const [connection, setConnection] = useState<ConnectionState>({
    isConnected: false,
    connectionType: null,
    targetPresent: false,
    targetVoltage: 0,
    targetCurrent: 0,
  });
  const [isScanning, setIsScanning] = useState(false);
  const [autoScan, setAutoScan] = useState(true);
  const [refreshing, setRefreshing] = useState(false);

  /* ── Connect to Forge-Probe ────────────────────────────────────────────── */

  const connect = useCallback(async () => {
    try {
      // Try USB first, fall back to BLE
      const usbOk = await forgeProbeService.connectViaUSB();
      if (usbOk) {
        setConnection((prev) => ({
          ...prev,
          isConnected: true,
          connectionType: 'usb',
        }));
        // Initiate target scan automatically
        await performScan();
        return;
      }

      const bleOk = await forgeProbeService.connectViaBLE();
      if (bleOk) {
        setConnection((prev) => ({
          ...prev,
          isConnected: true,
          connectionType: 'ble',
        }));
        await performScan();
        return;
      }

      Alert.alert('Connection Failed', 'No Forge-Probe device found via USB or BLE');
    } catch (error) {
      console.error('Connection error:', error);
      Alert.alert('Error', 'Failed to connect to Forge-Probe');
    }
  }, []);

  /* ── Scan for Targets ────────────────────────────────────────────────────── */

  const performScan = useCallback(async () => {
    if (!connection.isConnected) return;

    setIsScanning(true);
    try {
      const target = await forgeProbeService.scan();
      setConnection((prev) => ({
        ...prev,
        target,
        targetPresent: target.idcode !== 0 && target.idcode !== 0xFFFFFFFF,
        lastScan: new Date().toISOString(),
        targetVoltage: 3300, // Placeholder — read from device
      }));
    } catch (error) {
      console.error('Scan error:', error);
    } finally {
      setIsScanning(false);
    }
  }, [connection.isConnected]);

  /* ── Disconnect ──────────────────────────────────────────────────────────── */

  const disconnect = useCallback(() => {
    forgeProbeService.disconnect();
    setConnection({
      isConnected: false,
      connectionType: null,
      targetPresent: false,
      targetVoltage: 0,
      targetCurrent: 0,
    });
  }, []);

  /* ── Refresh Handler ─────────────────────────────────────────────────────── */

  const onRefresh = useCallback(async () => {
    setRefreshing(true);
    await performScan();
    setRefreshing(false);
  }, [performScan]);

  /* ── Auto-scan timer ─────────────────────────────────────────────────────── */

  useEffect(() => {
    if (!autoScan || !connection.isConnected) return;

    const interval = setInterval(performScan, 5000);
    return () => clearInterval(interval);
  }, [autoScan, connection.isConnected, performScan]);

  /* ── Protocol label helper ──────────────────────────────────────────────── */

  const protocolLabel = (p?: DebugProtocol): string => {
    switch (p) {
      case DebugProtocol.JTAG: return 'JTAG';
      case DebugProtocol.SWD: return 'SWD';
      case DebugProtocol.CJTAG: return 'cJTAG';
      case DebugProtocol.SWJ: return 'SWJ (Auto)';
      default: return 'Unknown';
    }
  };

  /* ── Render ──────────────────────────────────────────────────────────────── */

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} />
      }
    >
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>Forge-Probe</Text>
        <Text style={styles.subtitle}>Silicon Backdoor Debug Probe</Text>
        <Text style={styles.author}>by jayis1</Text>
      </View>

      {/* Connection Card */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Text style={styles.cardTitle}>Connection</Text>
          <View
            style={[
              styles.statusDot,
              { backgroundColor: connection.isConnected ? '#4CAF50' : '#F44336' },
            ]}
          />
        </View>

        {connection.isConnected ? (
          <View>
            <Text style={styles.infoRow}>
              Type: {connection.connectionType === 'usb' ? 'USB HS' : 'BLE'}
            </Text>
            <Text style={styles.infoRow}>
              Target: {connection.targetPresent ? 'PRESENT' : 'NOT FOUND'}
            </Text>
            {connection.target && (
              <>
                <Text style={styles.infoRow}>
                  Protocol: {protocolLabel(connection.target.protocol)}
                </Text>
                <Text style={styles.infoRow}>
                  IDCODE: 0x{connection.target.idcode.toString(16).padStart(8, '0')}
                </Text>
                <Text style={styles.infoRow}>
                  Target: {connection.target.description}
                </Text>
                <Text style={styles.infoRow}>
                  IR Length: {connection.target.irLength} bits
                </Text>
                <Text style={styles.infoRow}>
                  TAPs: {connection.target.tapCount}
                </Text>
                {connection.target.flashLocked && (
                  <View style={styles.warningBadge}>
                    <Text style={styles.warningText}>
                      ⚠ Flash Locked (RDP Level {connection.target.debugLevel})
                    </Text>
                  </View>
                )}
              </>
            )}
            <View style={styles.targetVoltage}>
              <Text style={styles.voltageText}>
                {connection.targetVoltage} mV
              </Text>
              <Text style={styles.voltageLabel}>Target VREF</Text>
            </View>

            <TouchableOpacity style={styles.disconnectBtn} onPress={disconnect}>
              <Text style={styles.disconnectBtnText}>Disconnect</Text>
            </TouchableOpacity>
          </View>
        ) : (
          <View>
            <Text style={styles.infoRow}>Not connected</Text>
            <TouchableOpacity style={styles.connectBtn} onPress={connect}>
              <Text style={styles.connectBtnText}>Connect to Forge-Probe</Text>
            </TouchableOpacity>
          </View>
        )}
      </View>

      {/* Scan Controls */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Target Scan</Text>

        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Auto-scan</Text>
          <Switch
            value={autoScan}
            onValueChange={setAutoScan}
            trackColor={{ false: '#555', true: '#2196F3' }}
          />
        </View>

        <TouchableOpacity
          style={[styles.scanBtn, isScanning && styles.scanBtnDisabled]}
          onPress={performScan}
          disabled={isScanning || !connection.isConnected}
        >
          {isScanning ? (
            <ActivityIndicator color="#fff" size="small" />
          ) : (
            <Text style={styles.scanBtnText}>Scan Now</Text>
          )}
        </TouchableOpacity>
      </View>

      {/* Quick Actions */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Quick Actions</Text>

        <View style={styles.actionRow}>
          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={() => navigation.navigate('MemoryView')}
            disabled={!connection.targetPresent}
          >
            <Text style={styles.actionIcon}>📖</Text>
            <Text style={styles.actionLabel}>Memory</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={() => navigation.navigate('FlashDump')}
            disabled={!connection.targetPresent}
          >
            <Text style={styles.actionIcon}>💾</Text>
            <Text style={styles.actionLabel}>Flash Dump</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={() => navigation.navigate('RegisterView')}
            disabled={!connection.targetPresent}
          >
            <Text style={styles.actionIcon}>🔢</Text>
            <Text style={styles.actionLabel}>Registers</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={() => navigation.navigate('BoundaryScan')}
            disabled={!connection.targetPresent}
          >
            <Text style={styles.actionIcon}>🔍</Text>
            <Text style={styles.actionLabel}>BScan</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={() => navigation.navigate('Settings')}
          >
            <Text style={styles.actionIcon}>⚙</Text>
            <Text style={styles.actionLabel}>Settings</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={async () => {
              await forgeProbeService.haltTarget();
              Alert.alert('Target', 'Halted');
            }}
            disabled={!connection.targetPresent}
          >
            <Text style={styles.actionIcon}>⏹</Text>
            <Text style={styles.actionLabel}>Halt</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={async () => {
              await forgeProbeService.resumeTarget();
              Alert.alert('Target', 'Resumed');
            }}
            disabled={!connection.targetPresent}
          >
            <Text style={styles.actionIcon}>▶</Text>
            <Text style={styles.actionLabel}>Resume</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.actionBtn,
              !connection.targetPresent && styles.actionBtnDisabled,
            ]}
            onPress={async () => {
              await forgeProbeService.resetTarget();
              Alert.alert('Target', 'Reset signal sent');
            }}
            disabled={!connection.targetPresent}
          >
            <Text style={styles.actionIcon}>🔄</Text>
            <Text style={styles.actionLabel}>Reset</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Last Scan Info */}
      {connection.lastScan && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Device Info</Text>
          <Text style={styles.infoRow}>
            Last scan: {new Date(connection.lastScan).toLocaleTimeString()}
          </Text>
          <Text style={styles.infoRow}>
            Manufacturer ID: 0x{connection.target?.manufacturerId.toString(16)}
          </Text>
          <Text style={styles.infoRow}>
            Boundary scan chain: {connection.target?.boundaryChainLen ?? 0} cells
          </Text>
        </View>
      )}

      {/* Legal Disclaimer */}
      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠ For authorized security research and penetration testing only.
          Unauthorized access to computer systems is illegal. Always obtain
          proper written authorization before testing any system.
        </Text>
      </View>
    </ScrollView>
  );
};

/* ─── Styles ────────────────────────────────────────────────────────────────── */

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#121212' },
  header: { padding: 24, alignItems: 'center' },
  title: { fontSize: 28, fontWeight: '800', color: '#00BCD4', letterSpacing: 1 },
  subtitle: { fontSize: 14, color: '#888', marginTop: 4 },
  author: { fontSize: 12, color: '#555', marginTop: 4 },

  card: {
    backgroundColor: '#1E1E1E',
    marginHorizontal: 16,
    marginVertical: 8,
    borderRadius: 12,
    padding: 16,
    borderColor: '#333',
    borderWidth: 1,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  cardTitle: { fontSize: 16, fontWeight: '700', color: '#E0E0E0', marginBottom: 12 },
  statusDot: { width: 12, height: 12, borderRadius: 6 },

  infoRow: { fontSize: 14, color: '#B0B0B0', marginVertical: 3, fontFamily: 'monospace' },

  connectBtn: {
    backgroundColor: '#2196F3',
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 12,
  },
  connectBtnText: { color: '#FFF', fontSize: 16, fontWeight: '600' },

  disconnectBtn: {
    backgroundColor: '#F44336',
    padding: 10,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 12,
  },
  disconnectBtnText: { color: '#FFF', fontSize: 14, fontWeight: '600' },

  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  switchLabel: { color: '#B0B0B0', fontSize: 14 },

  scanBtn: {
    backgroundColor: '#4CAF50',
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  scanBtnDisabled: { backgroundColor: '#333' },
  scanBtnText: { color: '#FFF', fontSize: 16, fontWeight: '600' },

  actionRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  actionBtn: {
    backgroundColor: '#2C2C2C',
    width: '23%',
    aspectRatio: 1,
    borderRadius: 10,
    alignItems: 'center',
    justifyContent: 'center',
    marginVertical: 4,
    borderColor: '#444',
    borderWidth: 1,
  },
  actionBtnDisabled: { opacity: 0.4 },
  actionIcon: { fontSize: 24 },
  actionLabel: { fontSize: 10, color: '#AAA', marginTop: 4 },

  targetVoltage: {
    backgroundColor: '#1B2A1B',
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    marginTop: 12,
  },
  voltageText: { fontSize: 22, fontWeight: '700', color: '#4CAF50', fontFamily: 'monospace' },
  voltageLabel: { fontSize: 11, color: '#888', marginTop: 2 },

  warningBadge: {
    backgroundColor: '#3E2723',
    borderRadius: 6,
    padding: 8,
    marginTop: 8,
  },
  warningText: { color: '#FF9800', fontSize: 12, fontWeight: '600' },

  disclaimer: { padding: 16, marginHorizontal: 16, marginVertical: 16 },
  disclaimerText: { fontSize: 10, color: '#555', textAlign: 'center', lineHeight: 14 },
});

export default DashboardScreen;