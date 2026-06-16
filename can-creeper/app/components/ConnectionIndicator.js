/**
 * ConnectionIndicator.js
 *
 * Status bar component showing BLE/USB connection state,
 * battery level, and device name. Provides quick scan button
 * for discovering CAN Creeper devices.
 */

import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';

const ConnectionIndicator = ({
  bleConnected,
  usbConnected,
  deviceName,
  batteryMv,
  isScanning,
  onScanPress,
}) => {
  const batteryPercent = batteryMv
    ? Math.min(100, Math.max(0, ((batteryMv - 3100) / (4200 - 3100)) * 100))
    : 0;

  const batteryColor = batteryPercent > 20
    ? '#238636'
    : batteryPercent > 10
      ? '#FF6B00'
      : '#DA3633';

  const isConnected = bleConnected || usbConnected;

  return (
    <View style={styles.container}>
      <View style={styles.leftSection}>
        {/* Connection dots */}
        <View style={styles.connectionRow}>
          <View style={[styles.dot, {
            backgroundColor: bleConnected ? '#238636' : '#30363D',
          }]} />
          <View style={[styles.dot, {
            backgroundColor: usbConnected ? '#238636' : '#30363D',
            marginLeft: 4,
          }]} />
          <Text style={styles.connectionText}>
            {isConnected
              ? (deviceName || 'CAN Creeper')
              : 'Disconnected'}
          </Text>
        </View>

        {/* Battery indicator */}
        {isConnected && batteryMv > 0 && (
          <View style={styles.batteryRow}>
            <View style={styles.batteryIcon}>
              <View style={[styles.batteryFill, {
                width: `${batteryPercent}%`,
                backgroundColor: batteryColor,
              }]} />
            </View>
            <Text style={[styles.batteryText, { color: batteryColor }]}>
              {(batteryMv / 1000).toFixed(1)}V ({Math.round(batteryPercent)}%)
            </Text>
          </View>
        )}
      </View>

      <View style={styles.rightSection}>
        {/* Scan button */}
        <TouchableOpacity
          style={[styles.scanBtn, isScanning && styles.scanBtnActive]}
          onPress={onScanPress}
          disabled={isScanning}
          activeOpacity={0.7}
        >
          <Text style={styles.scanBtnText}>
            {isScanning ? '⏳' : '🔍'}
          </Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#161B22',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#30363D',
  },
  leftSection: {
    flex: 1,
  },
  connectionRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  dot: {
    width: 6,
    height: 6,
    borderRadius: 3,
  },
  connectionText: {
    color: '#E6EDF3',
    fontSize: 13,
    fontWeight: '600',
    marginLeft: 8,
  },
  batteryRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 4,
  },
  batteryIcon: {
    width: 24,
    height: 12,
    borderWidth: 1,
    borderColor: '#30363D',
    borderRadius: 2,
    padding: 1,
    marginRight: 6,
  },
  batteryFill: {
    height: '100%',
    borderRadius: 1,
  },
  batteryText: {
    fontSize: 10,
    fontFamily: 'monospace',
  },
  rightSection: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  scanBtn: {
    width: 32,
    height: 32,
    borderRadius: 16,
    backgroundColor: '#21262D',
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 1,
    borderColor: '#30363D',
  },
  scanBtnActive: {
    borderColor: '#FF6B00',
  },
  scanBtnText: {
    fontSize: 14,
  },
});

export default ConnectionIndicator;
