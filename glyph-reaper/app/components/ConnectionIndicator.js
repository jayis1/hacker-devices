/**
 * ConnectionIndicator.js
 *
 * Status bar component showing BLE/USB connection state,
 * device name, battery level, and scan button.
 *
 * Author: jayis1
 * @license MIT
 */

import React from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
} from 'react-native';

const ConnectionIndicator = ({
  bleConnected,
  usbConnected,
  deviceName,
  batteryMv,
  isScanning,
  onScanPress,
}) => {
  const batteryPercent = batteryMv > 0 
    ? Math.min(100, Math.round((batteryMv / 3200) * 100))
    : 0;

  const batteryColor = batteryMv < 2400 ? '#FF4757' :
                        batteryMv < 2700 ? '#FFD700' : '#00D4AA';

  return (
    <View style={styles.container}>
      {/* Connection status */}
      <View style={styles.statusRow}>
        <View style={styles.statusGroup}>
          <View style={[
            styles.statusDot,
            { backgroundColor: bleConnected ? '#00D4AA' : '#5C6877' }
          ]} />
          <Text style={styles.statusText}>BLE</Text>
        </View>
        <View style={styles.statusGroup}>
          <View style={[
            styles.statusDot,
            { backgroundColor: usbConnected ? '#00D4AA' : '#5C6877' }
          ]} />
          <Text style={styles.statusText}>USB</Text>
        </View>
        {deviceName ? (
          <Text style={styles.deviceName} numberOfLines={1}>
            {deviceName}
          </Text>
        ) : (
          <Text style={styles.noDevice}>No device</Text>
        )}
      </View>

      {/* Battery and scan */}
      <View style={styles.rightRow}>
        {batteryMv > 0 && (
          <View style={styles.batteryContainer}>
            <Text style={[styles.batteryText, { color: batteryColor }]}>
              {batteryPercent}%
            </Text>
          </View>
        )}
        <TouchableOpacity
          style={styles.scanBtn}
          onPress={onScanPress}
          disabled={isScanning}
        >
          <Text style={styles.scanBtnText}>
            {isScanning ? '...' : 'Scan'}
          </Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    backgroundColor: '#131720',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1E242C',
  },
  statusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
  },
  statusGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    marginRight: 12,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 4,
  },
  statusText: {
    color: '#5C6877',
    fontSize: 11,
    fontWeight: '600',
  },
  deviceName: {
    color: '#E6EDF3',
    fontSize: 13,
    fontWeight: '600',
    flex: 1,
  },
  noDevice: {
    color: '#5C6877',
    fontSize: 13,
  },
  rightRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  batteryContainer: {
    marginRight: 12,
  },
  batteryText: {
    fontSize: 13,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  scanBtn: {
    paddingHorizontal: 16,
    paddingVertical: 6,
    backgroundColor: '#00D4AA20',
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#00D4AA',
  },
  scanBtnText: {
    color: '#00D4AA',
    fontSize: 12,
    fontWeight: '700',
  },
});

export default ConnectionIndicator;