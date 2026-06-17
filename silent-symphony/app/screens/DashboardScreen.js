/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Dashboard Screen — Device status, connection, battery, mode
 *
 * Shows current device state and allows BLE connection management.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  ActivityIndicator,
  Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import {
  buildGetStatus,
  buildSetMode,
  buildSetPower,
  buildSetGain,
  parseStatus,
  MODE_NAMES,
  TX_POWER_NAMES,
  RX_GAIN_NAMES,
  MODE,
} from '../utils/protocol';
import StatusBar from '../components/StatusBar';

const SERVICE_UUID = '00001810-0000-1000-8000-00805f9b34fb';
const CHARACTERISTIC_UUID = '00002a35-0000-1000-8000-00805f9b34fb';

/**
 * Dashboard screen — main device overview and connection management.
 */
export default function DashboardScreen({
  bleManager,
  connectedDevice,
  setConnectedDevice,
  status,
  setStatus,
  onDisconnect,
}) {
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [connecting, setConnecting] = useState(false);
  const [statusPolling, setStatusPolling] = useState(null);
  const [expandedMode, setExpandedMode] = useState(false);

  // Start scanning for SILENT-SYMPHONY devices
  const startScan = useCallback(() => {
    setScanning(true);
    setDevices([]);

    bleManager.startDeviceScan(
      null,
      { allowDuplicates: false },
      (error, scannedDevice) => {
        if (error) {
          console.warn('BLE Scan error:', error);
          setScanning(false);
          return;
        }

        if (
          scannedDevice &&
          scannedDevice.name &&
          scannedDevice.name.includes('SILENT-SYMPHONY')
        ) {
          setDevices((prev) => {
            if (!prev.find((d) => d.id === scannedDevice.id)) {
              return [...prev, scannedDevice];
            }
            return prev;
          });
        }
      },
    );

    // Stop scan after 10 seconds
    setTimeout(() => {
      bleManager.stopDeviceScan();
      setScanning(false);
    }, 10000);
  }, [bleManager]);

  // Connect to a device
  const connect = useCallback(
    async (device) => {
      setConnecting(true);
      try {
        const connected = await device.connect();
        await connected.discoverAllServicesAndCharacteristics();
        setConnectedDevice(connected);

        // Start polling status every 2 seconds
        const interval = setInterval(async () => {
          try {
            const data = await connected.writeCharacteristicWithResponseForService(
              SERVICE_UUID,
              CHARACTERISTIC_UUID,
              Buffer.from(buildGetStatus()).toString('base64'),
            );
            // Read response via notification
          } catch (e) {
            // Device may be disconnected
          }
        }, 2000);
        setStatusPolling(interval);

        // Subscribe to notifications
        connected.monitorCharacteristicForService(
          SERVICE_UUID,
          CHARACTERISTIC_UUID,
          (error, char) => {
            if (error) return;
            if (char && char.value) {
              const bytes = Buffer.from(char.value, 'base64');
              if (bytes.length >= 5 && bytes[0] === 0xAA) {
                // Parse packet
                // For status response, parse payload
                if (bytes[3] === 0x02) {
                  // STATUS_RESP
                  const payload = bytes.slice(5, -2);
                  const parsed = parseStatus(payload);
                  setStatus(parsed);
                }
              }
            }
          },
        );
      } catch (error) {
        Alert.alert('Connection failed', error.message);
      }
      setConnecting(false);
    },
    [bleManager, setConnectedDevice, setStatus, setStatusPolling],
  );

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (statusPolling) clearInterval(statusPolling);
      if (connectedDevice) {
        connectedDevice.cancelConnection().catch(() => {});
      }
    };
  }, [statusPolling, connectedDevice]);

  // Battery color
  const batteryColor =
    status && status.batteryPct < 15
      ? '#ff4444'
      : status && status.batteryPct < 40
      ? '#ffaa00'
      : '#00c853';

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Section */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Connection</Text>

        {!connectedDevice ? (
          <>
            <TouchableOpacity
              style={styles.button}
              onPress={startScan}
              disabled={scanning}
            >
              <Icon name="bluetooth-searching" size={20} color="#fff" />
              <Text style={styles.buttonText}>
                {scanning ? 'Scanning...' : 'Scan for Devices'}
              </Text>
            </TouchableOpacity>

            {scanning && <ActivityIndicator size="small" color="#00d4ff" />}

            {devices.length === 0 && scanning && (
              <Text style={styles.hint}>
                Searching for SILENT-SYMPHONY devices...
              </Text>
            )}

            {devices.map((device) => (
              <TouchableOpacity
                key={device.id}
                style={styles.deviceItem}
                onPress={() => connect(device)}
                disabled={connecting}
              >
                <Icon name="bluetooth-connected" size={18} color="#00d4ff" />
                <View style={styles.deviceInfo}>
                  <Text style={styles.deviceName}>{device.name}</Text>
                  <Text style={styles.deviceId}>{device.id}</Text>
                </View>
                {connecting && (
                  <ActivityIndicator size="small" color="#00d4ff" />
                )}
              </TouchableOpacity>
            ))}
          </>
        ) : (
          <View>
            <StatusBar status={status} />

            <TouchableOpacity style={styles.disconnectBtn} onPress={onDisconnect}>
              <Icon name="bluetooth-disabled" size={18} color="#ff4444" />
              <Text style={styles.disconnectText}>Disconnect</Text>
            </TouchableOpacity>
          </View>
        )}
      </View>

      {/* Status Section (only when connected) */}
      {connectedDevice && status && (
        <>
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Device Status</Text>
            <View style={styles.statusGrid}>
              <View style={styles.statusItem}>
                <Icon name="battery-full" size={24} color={batteryColor} />
                <Text style={styles.statusValue}>{status.batteryPct}%</Text>
                <Text style={styles.statusLabel}>Battery</Text>
              </View>
              <View style={styles.statusItem}>
                <Icon name="signal-cellular-alt" size={24} color="#00d4ff" />
                <Text style={styles.statusValue}>{status.signalQuality}%</Text>
                <Text style={styles.statusLabel}>Signal</Text>
              </View>
              <View style={styles.statusItem}>
                <Icon name="storage" size={24} color="#ffaa00" />
                <Text style={styles.statusValue}>
                  {(status.storageUsed / 1048576).toFixed(1)} MB
                </Text>
                <Text style={styles.statusLabel}>Storage</Text>
              </View>
              <View style={styles.statusItem}>
                <Icon name="timer" size={24} color="#7c4dff" />
                <Text style={styles.statusValue}>
                  {Math.floor(status.uptimeS / 60)}m
                </Text>
                <Text style={styles.statusLabel}>Uptime</Text>
              </View>
            </View>
          </View>

          {/* Current Mode & Configuration */}
          <View style={styles.card}>
            <TouchableOpacity
              style={styles.expandHeader}
              onPress={() => setExpandedMode(!expandedMode)}
            >
              <Text style={styles.cardTitle}>
                Mode: {MODE_NAMES[status.mode] || 'Unknown'}
              </Text>
              <Icon
                name={expandedMode ? 'expand-less' : 'expand-more'}
                size={24}
                color="#e0e0e0"
              />
            </TouchableOpacity>

            {expandedMode && (
              <View>
                <Text style={styles.configRow}>
                  Tx Power: {TX_POWER_NAMES[status.txPower] || 'N/A'}
                </Text>
                <Text style={styles.configRow}>
                  Rx Gain: {RX_GAIN_NAMES[status.rxGain] || 'N/A'}
                </Text>
                <Text style={styles.configRow}>
                  Firmware: v{status.fwVersion}
                </Text>
              </View>
            )}
          </View>

          {/* Quick Actions */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Quick Actions</Text>
            <View style={styles.actionRow}>
              <TouchableOpacity
                style={styles.actionBtn}
                onPress={() => {
                  /* Send SET_MODE = RX_FSK */
                  if (connectedDevice) {
                    connectedDevice
                      .writeCharacteristicWithResponseForService(
                        SERVICE_UUID,
                        CHARACTERISTIC_UUID,
                        Buffer.from(buildSetMode(MODE.RX_FSK)).toString(
                          'base64',
                        ),
                      )
                      .catch((e) => console.warn(e));
                  }
                }}
              >
                <Icon name="mic" size={20} color="#fff" />
                <Text style={styles.actionText}>Start Rx</Text>
              </TouchableOpacity>

              <TouchableOpacity
                style={styles.actionBtn}
                onPress={() => {
                  if (connectedDevice) {
                    connectedDevice
                      .writeCharacteristicWithResponseForService(
                        SERVICE_UUID,
                        CHARACTERISTIC_UUID,
                        Buffer.from(buildSetMode(MODE.IDLE)).toString('base64'),
                      )
                      .catch((e) => console.warn(e));
                  }
                }}
              >
                <Icon name="stop" size={20} color="#fff" />
                <Text style={styles.actionText}>Stop</Text>
              </TouchableOpacity>

              <TouchableOpacity
                style={[styles.actionBtn, { backgroundColor: '#333' }]}
                onPress={() => {
                  if (connectedDevice) {
                    connectedDevice
                      .writeCharacteristicWithResponseForService(
                        SERVICE_UUID,
                        CHARACTERISTIC_UUID,
                        Buffer.from(buildGetStatus()).toString('base64'),
                      )
                      .catch((e) => console.warn(e));
                  }
                }}
              >
                <Icon name="refresh" size={20} color="#fff" />
                <Text style={styles.actionText}>Refresh</Text>
              </TouchableOpacity>
            </View>
          </View>
        </>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#16213e',
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
    marginBottom: 12,
  },
  button: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#0d7377',
    borderRadius: 8,
    padding: 12,
    gap: 8,
  },
  buttonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  hint: {
    color: '#6c757d',
    textAlign: 'center',
    marginTop: 12,
    fontSize: 12,
  },
  deviceItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 12,
    marginTop: 8,
    gap: 10,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    color: '#e0e0e0',
    fontSize: 14,
    fontWeight: '600',
  },
  deviceId: {
    color: '#6c757d',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  disconnectBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#2a1a1a',
    borderRadius: 8,
    padding: 10,
    marginTop: 12,
    gap: 8,
    borderWidth: 1,
    borderColor: '#441111',
  },
  disconnectText: {
    color: '#ff4444',
    fontSize: 14,
    fontWeight: '600',
  },
  statusGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-around',
  },
  statusItem: {
    alignItems: 'center',
    padding: 8,
    minWidth: 70,
  },
  statusValue: {
    color: '#e0e0e0',
    fontSize: 16,
    fontWeight: 'bold',
    marginTop: 4,
  },
  statusLabel: {
    color: '#6c757d',
    fontSize: 11,
    marginTop: 2,
  },
  expandHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  configRow: {
    color: '#b0b0b0',
    fontSize: 13,
    paddingVertical: 4,
    borderTopWidth: 1,
    borderTopColor: '#16213e',
  },
  actionRow: {
    flexDirection: 'row',
    gap: 8,
  },
  actionBtn: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#0d7377',
    borderRadius: 8,
    padding: 10,
    gap: 6,
  },
  actionText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: '600',
  },
});