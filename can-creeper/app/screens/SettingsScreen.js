/**
 * SettingsScreen.js
 *
 * Device configuration and connection management screen.
 * Handles BLE scanning/connection, USB status, CAN bitrate
 * configuration, termination control, and firmware updates.
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  FlatList,
  Switch,
  TextInput,
  Alert,
  StyleSheet,
  ScrollView,
  ActivityIndicator,
} from 'react-native';

/* Common CAN bitrate presets */
const BITRATE_PRESETS = [
  { label: '125 kbps', nominal: 125000, data: 1000000 },
  { label: '250 kbps', nominal: 250000, data: 2000000 },
  { label: '500 kbps', nominal: 500000, data: 4000000 },
  { label: '1 Mbps', nominal: 1000000, data: 8000000 },
  { label: '2 Mbps FD', nominal: 2000000, data: 8000000 },
  { label: '5 Mbps FD', nominal: 5000000, data: 15000000 },
];

const SettingsScreen = ({ protocol, connectionState, deviceStatus, onConnectDevice }) => {
  const [bleDevices, setBleDevices] = useState([]);
  const [isScanning, setIsScanning] = useState(false);
  const [can0Bitrate, setCan0Bitrate] = useState(500000);
  const [can1Bitrate, setCan1Bitrate] = useState(500000);
  const [can0FdEnabled, setCan0FdEnabled] = useState(true);
  const [can1FdEnabled, setCan1FdEnabled] = useState(true);
  const [can0Termination, setCan0Termination] = useState(false);
  const [can1Termination, setCan1Termination] = useState(false);
  const [can0Mode, setCan0Mode] = useState('normal');
  const [can1Mode, setCan1Mode] = useState('normal');
  const [bleTxPower, setBleTxPower] = useState(0);
  const [deviceName, setDeviceName] = useState('CAN Creeper');
  const [isUpdating, setIsUpdating] = useState(false);

  const isConnected = connectionState.bleConnected || connectionState.usbConnected;

  /* Start BLE scan */
  const startScan = useCallback(async () => {
    setIsScanning(true);
    setBleDevices([]);
    try {
      const devices = await protocol.scanBLE(5000);
      setBleDevices(devices.filter(d => d.name?.includes('CAN Creeper') || d.name?.includes('can-creeper')));
    } catch (error) {
      Alert.alert('Scan Error', error.message);
    }
    setIsScanning(false);
  }, []);

  /* Connect to BLE device */
  const connectDevice = useCallback(async (device) => {
    try {
      await onConnectDevice(device.id);
    } catch (error) {
      Alert.alert('Connection Failed', error.message);
    }
  }, [onConnectDevice]);

  /* Apply CAN configuration */
  const applyCanConfig = useCallback(async (channel) => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Connect to device first.');
      return;
    }

    try {
      const bitrate = channel === 0 ? can0Bitrate : can1Bitrate;
      const fdEnabled = channel === 0 ? can0FdEnabled : can1FdEnabled;
      const termination = channel === 0 ? can0Termination : can1Termination;
      const mode = channel === 0 ? can0Mode : can1Mode;

      await protocol.setCanConfig(channel, {
        nominalBitrate: bitrate,
        dataBitrate: fdEnabled ? bitrate * 8 : 0,
        fdEnabled,
        brsEnabled: fdEnabled,
        termination,
        mode,
      });

      Alert.alert('Config Applied', `CAN${channel} configuration updated.`);
    } catch (error) {
      Alert.alert('Config Error', error.message);
    }
  }, [isConnected, can0Bitrate, can1Bitrate, can0FdEnabled, can1FdEnabled,
      can0Termination, can1Termination, can0Mode, can1Mode]);

  /* Apply BLE settings */
  const applyBleSettings = useCallback(async () => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Connect to device first.');
      return;
    }

    try {
      await protocol.setBleConfig({
        txPower: bleTxPower,
        deviceName,
      });
      Alert.alert('BLE Updated', 'BLE settings applied.');
    } catch (error) {
      Alert.alert('BLE Error', error.message);
    }
  }, [isConnected, bleTxPower, deviceName]);

  /* Start firmware update */
  const startFirmwareUpdate = useCallback(async () => {
    Alert.alert(
      'Firmware Update',
      'This will update the CAN Creeper firmware. The device will reboot after update. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Update',
          style: 'destructive',
          onPress: async () => {
            setIsUpdating(true);
            try {
              /* In production, user would select a firmware file */
              await protocol.startDFU();
              Alert.alert('DFU Mode', 'Device is in DFU mode. Send firmware image to complete update.');
            } catch (error) {
              Alert.alert('DFU Error', error.message);
            }
            setIsUpdating(false);
          },
        },
      ]
    );
  }, []);

  /* Request device status */
  const requestStatus = useCallback(async () => {
    if (!isConnected) return;
    try {
      await protocol.requestStatus();
    } catch (error) {
      console.log('Status request failed:', error.message);
    }
  }, [isConnected]);

  /* Poll status periodically */
  useEffect(() => {
    if (!isConnected) return;
    const interval = setInterval(requestStatus, 2000);
    return () => clearInterval(interval);
  }, [isConnected, requestStatus]);

  /* Render BLE device item */
  const renderBleDevice = useCallback(({ item }) => (
    <TouchableOpacity style={styles.deviceItem} onPress={() => connectDevice(item)}>
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}>{item.name || 'Unknown Device'}</Text>
        <Text style={styles.deviceId}>{item.id}</Text>
        <Text style={styles.deviceRssi}>RSSI: {item.rssi} dBm</Text>
      </View>
      <Text style={styles.connectIcon}>→</Text>
    </TouchableOpacity>
  ), [connectDevice]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
      {/* Connection status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.statusRow}>
          <View style={[styles.statusDot, {
            backgroundColor: connectionState.bleConnected ? '#238636' : '#30363D',
          }]} />
          <Text style={styles.statusLabel}>
            BLE: {connectionState.bleConnected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <View style={[styles.statusDot, {
            backgroundColor: connectionState.usbConnected ? '#238636' : '#30363D',
          }]} />
          <Text style={styles.statusLabel}>
            USB: {connectionState.usbConnected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {connectionState.deviceName && (
          <Text style={styles.deviceInfoText}>
            Device: {connectionState.deviceName} ({connectionState.deviceId})
          </Text>
        )}

        {/* BLE scan */}
        <TouchableOpacity
          style={[styles.scanBtn, isScanning && styles.scanBtnActive]}
          onPress={startScan}
          disabled={isScanning}
        >
          {isScanning ? (
            <View style={styles.scanningRow}>
              <ActivityIndicator color="#FF6B00" size="small" />
              <Text style={styles.scanBtnText}> Scanning...</Text>
            </View>
          ) : (
            <Text style={styles.scanBtnText}>🔍 Scan for Devices</Text>
          )}
        </TouchableOpacity>

        {/* BLE device list */}
        {bleDevices.length > 0 && (
          <View style={styles.deviceList}>
            <Text style={styles.deviceListTitle}>Found Devices:</Text>
            {bleDevices.map((device, idx) => (
              <TouchableOpacity
                key={idx}
                style={styles.deviceItem}
                onPress={() => connectDevice(device)}
              >
                <View style={styles.deviceInfo}>
                  <Text style={styles.deviceName}>{device.name || 'Unknown'}</Text>
                  <Text style={styles.deviceId}>{device.id}</Text>
                </View>
                <Text style={styles.deviceRssi}>{device.rssi} dBm</Text>
              </TouchableOpacity>
            ))}
          </View>
        )}
      </View>

      {/* Device status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Status</Text>
        <View style={styles.statusGrid}>
          <View style={styles.statusItem}>
            <Text style={styles.statusItemLabel}>Uptime</Text>
            <Text style={styles.statusItemValue}>
              {Math.floor(deviceStatus.uptime / 3600)}h {Math.floor((deviceStatus.uptime % 3600) / 60)}m
            </Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusItemLabel}>Battery</Text>
            <Text style={[
              styles.statusItemValue,
              deviceStatus.batteryMv < 3400 && styles.statusWarning,
              deviceStatus.batteryMv < 3100 && styles.statusCritical,
            ]}>
              {(deviceStatus.batteryMv / 1000).toFixed(2)}V
            </Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusItemLabel}>CAN0 Frames</Text>
            <Text style={styles.statusItemValue}>{deviceStatus.can0FrameCount.toLocaleString()}</Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusItemLabel}>CAN1 Frames</Text>
            <Text style={styles.statusItemValue}>{deviceStatus.can1FrameCount.toLocaleString()}</Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusItemLabel}>CAN0 Errors</Text>
            <Text style={[
              styles.statusItemValue,
              deviceStatus.can0Tec >= 96 && styles.statusWarning,
            ]}>
              TEC:{deviceStatus.can0Tec} REC:{deviceStatus.can0Rec}
            </Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusItemLabel}>CAN1 Errors</Text>
            <Text style={[
              styles.statusItemValue,
              deviceStatus.can1Tec >= 96 && styles.statusWarning,
            ]}>
              TEC:{deviceStatus.can1Tec} REC:{deviceStatus.can1Rec}
            </Text>
          </View>
        </View>
      </View>

      {/* CAN0 Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CAN Channel 0</Text>

        <View style={styles.bitrateRow}>
          <Text style={styles.configLabel}>Bitrate:</Text>
          <ScrollView horizontal showsHorizontalScrollIndicator={false}>
            {BITRATE_PRESETS.map(preset => (
              <TouchableOpacity
                key={preset.label}
                style={[styles.bitrateBtn, can0Bitrate === preset.nominal && styles.bitrateBtnActive]}
                onPress={() => setCan0Bitrate(preset.nominal)}
              >
                <Text style={[styles.bitrateBtnText, can0Bitrate === preset.nominal && styles.bitrateBtnTextActive]}>
                  {preset.label}
                </Text>
              </TouchableOpacity>
            ))}
          </ScrollView>
        </View>

        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>CAN FD Mode</Text>
          <Switch
            value={can0FdEnabled}
            onValueChange={setCan0FdEnabled}
            trackColor={{ false: '#30363D', true: '#FF6B0050' }}
            thumbColor={can0FdEnabled ? '#FF6B00' : '#8B949E'}
          />
        </View>

        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>120Ω Termination</Text>
          <Switch
            value={can0Termination}
            onValueChange={setCan0Termination}
            trackColor={{ false: '#30363D', true: '#FF6B0050' }}
            thumbColor={can0Termination ? '#FF6B00' : '#8B949E'}
          />
        </View>

        <View style={styles.modeRow}>
          <Text style={styles.configLabel}>Mode:</Text>
          <View style={styles.modeSelector}>
            {['normal', 'listen-only', 'bus-monitor'].map(mode => (
              <TouchableOpacity
                key={mode}
                style={[styles.modeBtn, can0Mode === mode && styles.modeBtnActive]}
                onPress={() => setCan0Mode(mode)}
              >
                <Text style={[styles.modeBtnText, can0Mode === mode && styles.modeBtnTextActive]}>
                  {mode}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        <TouchableOpacity
          style={[styles.applyBtn, !isConnected && styles.btnDisabled]}
          onPress={() => applyCanConfig(0)}
          disabled={!isConnected}
        >
          <Text style={styles.applyBtnText}>Apply CAN0 Config</Text>
        </TouchableOpacity>
      </View>

      {/* CAN1 Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CAN Channel 1</Text>

        <View style={styles.bitrateRow}>
          <Text style={styles.configLabel}>Bitrate:</Text>
          <ScrollView horizontal showsHorizontalScrollIndicator={false}>
            {BITRATE_PRESETS.map(preset => (
              <TouchableOpacity
                key={preset.label}
                style={[styles.bitrateBtn, can1Bitrate === preset.nominal && styles.bitrateBtnActive]}
                onPress={() => setCan1Bitrate(preset.nominal)}
              >
                <Text style={[styles.bitrateBtnText, can1Bitrate === preset.nominal && styles.bitrateBtnTextActive]}>
                  {preset.label}
                </Text>
              </TouchableOpacity>
            ))}
          </ScrollView>
        </View>

        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>CAN FD Mode</Text>
          <Switch
            value={can1FdEnabled}
            onValueChange={setCan1FdEnabled}
            trackColor={{ false: '#30363D', true: '#FF6B0050' }}
            thumbColor={can1FdEnabled ? '#FF6B00' : '#8B949E'}
          />
        </View>

        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>120Ω Termination</Text>
          <Switch
            value={can1Termination}
            onValueChange={setCan1Termination}
            trackColor={{ false: '#30363D', true: '#FF6B0050' }}
            thumbColor={can1Termination ? '#FF6B00' : '#8B949E'}
          />
        </View>

        <View style={styles.modeRow}>
          <Text style={styles.configLabel}>Mode:</Text>
          <View style={styles.modeSelector}>
            {['normal', 'listen-only', 'bus-monitor'].map(mode => (
              <TouchableOpacity
                key={mode}
                style={[styles.modeBtn, can1Mode === mode && styles.modeBtnActive]}
                onPress={() => setCan1Mode(mode)}
              >
                <Text style={[styles.modeBtnText, can1Mode === mode && styles.modeBtnTextActive]}>
                  {mode}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        <TouchableOpacity
          style={[styles.applyBtn, !isConnected && styles.btnDisabled]}
          onPress={() => applyCanConfig(1)}
          disabled={!isConnected}
        >
          <Text style={styles.applyBtnText}>Apply CAN1 Config</Text>
        </TouchableOpacity>
      </View>

      {/* BLE Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>BLE Settings</Text>

        <View style={styles.inputRow}>
          <Text style={styles.configLabel}>Device Name:</Text>
          <TextInput
            style={styles.textInput}
            value={deviceName}
            onChangeText={setDeviceName}
            placeholder="CAN Creeper"
            placeholderTextColor="#484F58"
          />
        </View>

        <View style={styles.inputRow}>
          <Text style={styles.configLabel}>TX Power (dBm):</Text>
          <View style={styles.powerSelector}>
            {[-40, -20, -16, -12, -8, -4, 0, 3, 4, 8].map(power => (
              <TouchableOpacity
                key={power}
                style={[styles.powerBtn, bleTxPower === power && styles.powerBtnActive]}
                onPress={() => setBleTxPower(power)}
              >
                <Text style={[styles.powerBtnText, bleTxPower === power && styles.powerBtnTextActive]}>
                  {power}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        <TouchableOpacity
          style={[styles.applyBtn, !isConnected && styles.btnDisabled]}
          onPress={applyBleSettings}
          disabled={!isConnected}
        >
          <Text style={styles.applyBtnText}>Apply BLE Settings</Text>
        </TouchableOpacity>
      </View>

      {/* Firmware Update */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <Text style={styles.firmwareInfo}>
          CAN Creeper v1.0.0{'\n'}
          nRF52840 + S140 SoftDevice{'\n'}
          Build: June 2026
        </Text>
        <TouchableOpacity
          style={[styles.dfuBtn, (isUpdating || !isConnected) && styles.btnDisabled]}
          onPress={startFirmwareUpdate}
          disabled={isUpdating || !isConnected}
        >
          {isUpdating ? (
            <ActivityIndicator color="#FFFFFF" size="small" />
          ) : (
            <Text style={styles.dfuBtnText}>Update Firmware (DFU)</Text>
          )}
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>
          CAN Creeper{'\n'}
          Dual CAN FD Sniffer & Injector{'\n'}
          Designed by Nous Research{'\n'}
          Open Source Hardware{'\n\n'}
          Target BOM: &lt;$75 USD{'\n'}
          PCB: 65mm × 35mm, 4-layer{'\n'}
          MCU: nRF52840 Cortex-M4F @ 64 MHz{'\n'}
          CAN: 2× MCP2518FD + TJA1445{'\n'}
          Memory: 8MB PSRAM + 16MB Flash{'\n'}
          Connectivity: BLE 5.2 + USB CDC-ACM
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D1117',
  },
  scrollContent: {
    paddingBottom: 40,
  },
  section: {
    backgroundColor: '#161B22',
    marginHorizontal: 12,
    marginTop: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#30363D',
    padding: 12,
  },
  sectionTitle: {
    color: '#E6EDF3',
    fontSize: 15,
    fontWeight: '700',
    marginBottom: 10,
  },
  statusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 6,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  statusLabel: {
    color: '#E6EDF3',
    fontSize: 13,
  },
  deviceInfoText: {
    color: '#8B949E',
    fontSize: 12,
    fontFamily: 'monospace',
    marginTop: 4,
  },
  scanBtn: {
    marginTop: 10,
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#21262D',
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  scanBtnActive: {
    borderColor: '#FF6B00',
  },
  scanningRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  scanBtnText: {
    color: '#E6EDF3',
    fontSize: 14,
    fontWeight: '600',
  },
  deviceList: {
    marginTop: 8,
  },
  deviceListTitle: {
    color: '#8B949E',
    fontSize: 12,
    fontWeight: '600',
    marginBottom: 6,
  },
  deviceItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#0D1117',
    borderRadius: 6,
    padding: 10,
    marginBottom: 4,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    color: '#E6EDF3',
    fontSize: 13,
    fontWeight: '600',
  },
  deviceId: {
    color: '#484F58',
    fontSize: 10,
    fontFamily: 'monospace',
    marginTop: 2,
  },
  deviceRssi: {
    color: '#8B949E',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  connectIcon: {
    color: '#FF6B00',
    fontSize: 18,
    marginLeft: 8,
  },
  statusGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  statusItem: {
    width: '50%',
    paddingVertical: 6,
    paddingRight: 8,
  },
  statusItemLabel: {
    color: '#484F58',
    fontSize: 10,
    fontWeight: '600',
    textTransform: 'uppercase',
  },
  statusItemValue: {
    color: '#E6EDF3',
    fontSize: 14,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  statusWarning: {
    color: '#FF6B00',
  },
  statusCritical: {
    color: '#DA3633',
  },
  bitrateRow: {
    marginBottom: 10,
  },
  configLabel: {
    color: '#8B949E',
    fontSize: 12,
    fontWeight: '600',
    marginBottom: 4,
  },
  bitrateBtn: {
    paddingHorizontal: 10,
    paddingVertical: 6,
    backgroundColor: '#21262D',
    marginRight: 4,
    borderRadius: 4,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  bitrateBtnActive: {
    backgroundColor: '#FF6B0020',
    borderColor: '#FF6B00',
  },
  bitrateBtnText: {
    color: '#8B949E',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  bitrateBtnTextActive: {
    color: '#FF6B00',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  switchLabel: {
    color: '#E6EDF3',
    fontSize: 13,
  },
  modeRow: {
    marginBottom: 10,
  },
  modeSelector: {
    flexDirection: 'row',
  },
  modeBtn: {
    flex: 1,
    paddingVertical: 6,
    alignItems: 'center',
    backgroundColor: '#21262D',
    marginHorizontal: 2,
    borderRadius: 4,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  modeBtnActive: {
    backgroundColor: '#FF6B0020',
    borderColor: '#FF6B00',
  },
  modeBtnText: {
    color: '#8B949E',
    fontSize: 11,
  },
  modeBtnTextActive: {
    color: '#FF6B00',
  },
  inputRow: {
    marginBottom: 10,
  },
  textInput: {
    backgroundColor: '#0D1117',
    borderWidth: 1,
    borderColor: '#30363D',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 8,
    color: '#E6EDF3',
    fontSize: 14,
  },
  powerSelector: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  powerBtn: {
    paddingHorizontal: 8,
    paddingVertical: 4,
    backgroundColor: '#21262D',
    marginRight: 4,
    marginBottom: 4,
    borderRadius: 4,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  powerBtnActive: {
    backgroundColor: '#FF6B0020',
    borderColor: '#FF6B00',
  },
  powerBtnText: {
    color: '#8B949E',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  powerBtnTextActive: {
    color: '#FF6B00',
  },
  applyBtn: {
    marginTop: 8,
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#FF6B00',
    borderRadius: 6,
  },
  applyBtnText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700',
  },
  btnDisabled: {
    opacity: 0.4,
  },
  firmwareInfo: {
    color: '#8B949E',
    fontSize: 12,
    fontFamily: 'monospace',
    lineHeight: 18,
    marginBottom: 10,
  },
  dfuBtn: {
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#1F6FEB',
    borderRadius: 6,
  },
  dfuBtnText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700',
  },
  aboutText: {
    color: '#8B949E',
    fontSize: 12,
    lineHeight: 18,
  },
});

export default SettingsScreen;
