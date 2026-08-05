/**
 * ConnectScreen.js — Device connection screen
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: MIT
 *
 * Shows USB and BLE connection options, device scanning,
 * and connection status. Displays firmware version after connecting.
 */

import React, { useState, useContext, useEffect } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ActivityIndicator,
  ScrollView, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { DeviceContext } from '../utils/deviceContext';
import { CMD_PING, CMD_GET_VERSION, RESP_VERSION } from '../utils/protocol';

export default function ConnectScreen() {
  const { connected, connectDevice, disconnectDevice, sendCommand } = useContext(DeviceContext);
  const [scanning, setScanning] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [usbDevices, setUsbDevices] = useState([]);
  const [bleDevices, setBleDevices] = useState([]);
  const [version, setVersion] = useState(null);

  // Scan for USB devices (Android)
  const scanUsb = async () => {
    setScanning(true);
    try {
      const UsbSerial = require('react-native-usb-serial').default;
      const devices = await UsbSerial.getDeviceList();
      setUsbDevices(devices || []);
    } catch (err) {
      Alert.alert('USB Scan Failed', err.message);
    }
    setScanning(false);
  };

  // Scan for BLE devices
  const scanBle = async () => {
    setScanning(true);
    try {
      const { BleManager } = require('react-native-ble-plx');
      const bleManager = new BleManager();
      const subscription = bleManager.onStateChange((state) => {
        if (state === 'PoweredOn') {
          bleManager.startDeviceScan(null, null, (error, device) => {
            if (error) return;
            if (device.name && device.name.includes('PlasmaReaper')) {
              setBleDevices(prev => {
                if (prev.find(d => d.id === device.id)) return prev;
                return [...prev, device];
              });
            }
          });
          subscription.remove();
        }
      }, true);
      // Stop scan after 10 seconds
      setTimeout(() => {
        bleManager.stopDeviceScan();
        setScanning(false);
      }, 10000);
    } catch (err) {
      Alert.alert('BLE Scan Failed', err.message);
      setScanning(false);
    }
  };

  // Connect to a device
  const handleConnect = async (type, deviceId) => {
    setConnecting(true);
    const success = await connectDevice(type, { deviceId });
    setConnecting(false);

    if (success) {
      // Get firmware version
      const result = await sendCommand(CMD_GET_VERSION);
      if (result && result.type === RESP_VERSION) {
        setVersion(`v${result.data.major}.${result.data.minor}.${result.data.patch}`);
      }
      // Ping
      await sendCommand(CMD_PING);
    } else {
      Alert.alert('Connection Failed', 'Could not connect to device.');
    }
  };

  const handleDisconnect = async () => {
    await disconnectDevice();
    setVersion(null);
  };

  if (connected) {
    return (
      <ScrollView style={styles.container}>
        <View style={styles.connectedCard}>
          <Icon name="check-circle" size={64} color="#4CAF50" />
          <Text style={styles.connectedTitle}>Connected</Text>
          {version && <Text style={styles.versionText}>Firmware: {version}</Text>}
          <Text style={styles.authorText}>PlasmaReaper by jayis1</Text>
          <TouchableOpacity style={styles.disconnectBtn} onPress={handleDisconnect}>
            <Text style={styles.disconnectBtnText}>Disconnect</Text>
          </TouchableOpacity>
        </View>
      </ScrollView>
    );
  }

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>PlasmaReaper</Text>
        <Text style={styles.subtitle}>Multi-Vector Fault Injection Toolkit</Text>
        <Text style={styles.author}>by jayis1</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>USB Connection (Android)</Text>
        <Text style={styles.sectionDesc}>Connect via USB-C cable. Requires OTG support.</Text>
        <TouchableOpacity style={styles.scanBtn} onPress={scanUsb} disabled={scanning}>
          {scanning ? <ActivityIndicator color="#fff" /> : <Text style={styles.btnText}>Scan USB</Text>}
        </TouchableOpacity>
        {usbDevices.map((dev, i) => (
          <TouchableOpacity
            key={i}
            style={styles.deviceItem}
            onPress={() => handleConnect('usb', dev.deviceId)}
            disabled={connecting}
          >
            <Icon name="usb" size={24} color="#333" />
            <View style={styles.deviceInfo}>
              <Text style={styles.deviceName}>{dev.deviceName || 'USB Device'}</Text>
              <Text style={styles.deviceId}>VID:{dev.vendorId} PID:{dev.productId}</Text>
            </View>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>BLE Connection (iOS/Android)</Text>
        <Text style={styles.sectionDesc}>Connect via Bluetooth Low Energy.</Text>
        <TouchableOpacity style={styles.scanBtn} onPress={scanBle} disabled={scanning}>
          {scanning ? <ActivityIndicator color="#fff" /> : <Text style={styles.btnText}>Scan BLE</Text>}
        </TouchableOpacity>
        {bleDevices.map((dev, i) => (
          <TouchableOpacity
            key={i}
            style={styles.deviceItem}
            onPress={() => handleConnect('ble', dev.id)}
            disabled={connecting}
          >
            <Icon name="bluetooth" size={24} color="#333" />
            <View style={styles.deviceInfo}>
              <Text style={styles.deviceName}>{dev.name}</Text>
              <Text style={styles.deviceId}>{dev.id}</Text>
            </View>
          </TouchableOpacity>
        ))}
      </View>

      {connecting && (
        <View style={styles.overlay}>
          <ActivityIndicator size="large" color="#e91e63" />
          <Text style={styles.connectingText}>Connecting...</Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  header: { padding: 20, alignItems: 'center', backgroundColor: '#1a1a2e' },
  title: { fontSize: 28, fontWeight: 'bold', color: '#e91e63' },
  subtitle: { fontSize: 14, color: '#aaa', marginTop: 4 },
  author: { fontSize: 12, color: '#666', marginTop: 2 },
  section: { padding: 16, backgroundColor: '#fff', margin: 8, borderRadius: 8 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 4 },
  sectionDesc: { fontSize: 12, color: '#666', marginBottom: 12 },
  scanBtn: { backgroundColor: '#e91e63', padding: 12, borderRadius: 6, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: 'bold' },
  deviceItem: { flexDirection: 'row', alignItems: 'center', padding: 12, borderBottomWidth: 1, borderBottomColor: '#eee' },
  deviceInfo: { marginLeft: 12, flex: 1 },
  deviceName: { fontSize: 16, fontWeight: '500' },
  deviceId: { fontSize: 12, color: '#999' },
  connectedCard: { padding: 40, alignItems: 'center', backgroundColor: '#fff', margin: 16, borderRadius: 12 },
  connectedTitle: { fontSize: 24, fontWeight: 'bold', marginTop: 12, color: '#4CAF50' },
  versionText: { fontSize: 16, color: '#666', marginTop: 8 },
  authorText: { fontSize: 14, color: '#999', marginTop: 4 },
  disconnectBtn: { marginTop: 20, paddingHorizontal: 24, paddingVertical: 12, backgroundColor: '#f44336', borderRadius: 6 },
  disconnectBtnText: { color: '#fff', fontWeight: 'bold' },
  overlay: { position: 'absolute', top: 0, left: 0, right: 0, bottom: 0, backgroundColor: 'rgba(0,0,0,0.5)', justifyContent: 'center', alignItems: 'center' },
  connectingText: { color: '#fff', marginTop: 12 },
});