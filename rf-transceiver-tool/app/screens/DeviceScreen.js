/**
 * DeviceScreen.js — Main device interaction screen
 *
 * Shows connection status, current mode, RSSI level, and packet log.
 * Provides quick-access buttons for common operations.
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  TouchableOpacity,
  FlatList,
  Alert,
  Vibration,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import StatusCard from '../components/StatusCard';
import { CMD, MODE, MODE_NAMES, buildPacket, parsePacket, formatFrequency } from '../utils/protocol';

// USB Serial connection (react-native-usb-serial)
import { UsbSerial } from 'react-native-usb-serial';

export default function DeviceScreen() {
  const navigation = useNavigation();
  const [connected, setConnected] = useState(false);
  const [currentMode, setCurrentMode] = useState(MODE.IDLE);
  const [rssi, setRssi] = useState(-100);
  const [packets, setPackets] = useState([]);
  const [scanning, setScanning] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const serialRef = useRef(null);
  const packetLogRef = useRef([]);

  // Configuration state
  const [subghzFreq, setSubghzFreq] = useState(868000000);
  const [nrf24Channel, setNrf24Channel] = useState(76);

  useEffect(() => {
    // Attempt to connect on mount
    connectDevice();
    return () => {
      disconnectDevice();
    };
  }, []);

  const connectDevice = useCallback(async () => {
    try {
      const devices = await UsbSerial.listDevices();
      if (devices.length === 0) {
        Alert.alert('No Device', 'No USB serial device found. Connect the RF Transceiver Tool via USB-C.');
        return;
      }

      const device = devices[0];
      const serial = new UsbSerial();
      await serial.connect(device.deviceId, {
        baudRate: 115200,
        dataBits: 8,
        stopBits: 1,
        parity: 'none',
      });

      serial.onRead((data) => {
        handleIncomingData(data);
      });

      serialRef.current = serial;
      setConnected(true);

      // Request device status
      sendCommand(CMD.GET_STATUS);
      sendCommand(CMD.GET_VERSION);

    } catch (error) {
      Alert.alert('Connection Error', `Failed to connect: ${error.message}`);
      setConnected(false);
    }
  }, []);

  const disconnectDevice = useCallback(async () => {
    if (serialRef.current) {
      try {
        await serialRef.current.disconnect();
      } catch (e) {
        // Ignore disconnect errors
      }
      serialRef.current = null;
    }
    setConnected(false);
    setCurrentMode(MODE.IDLE);
  }, []);

  const sendCommand = useCallback((cmdId, payload = []) => {
    if (!serialRef.current || !connected) return;
    const packet = buildPacket(cmdId, payload);
    serialRef.current.write(Array.from(packet));
  }, [connected]);

  const handleIncomingData = useCallback((rawData) => {
    const parsed = parsePacket(new Uint8Array(rawData));
    if (!parsed || !parsed.valid) return;

    switch (parsed.cmdId) {
      case CMD.CC1101_PACKET: {
        // Sub-GHz packet received
        const rssiRaw = parsed.payload[0];
        const lqi = parsed.payload[1];
        const length = parsed.payload[2];
        const data = parsed.payload.slice(3, 3 + length);
        const decodedRssi = parsed.payload[0] >= 128
          ? ((parsed.payload[0] - 256) / 2) - 74
          : (parsed.payload[0] / 2) - 74;
        setRssi(Math.round(decodedRssi));

        const packetInfo = {
          id: Date.now(),
          type: 'subghz',
          rssi: decodedRssi,
          lqi: lqi,
          length: length,
          data: data,
          timestamp: new Date().toLocaleTimeString(),
        };
        packetLogRef.current = [packetInfo, ...packetLogRef.current].slice(0, 100);
        setPackets(packetLogRef.current);
        break;
      }

      case CMD.NRF24_PACKET: {
        // 2.4 GHz packet received
        const pipe = parsed.payload[0];
        const length = parsed.payload[1];
        const data = parsed.payload.slice(2, 2 + length);

        const packetInfo = {
          id: Date.now(),
          type: 'nrf24',
          pipe: pipe,
          length: length,
          data: data,
          timestamp: new Date().toLocaleTimeString(),
        };
        packetLogRef.current = [packetInfo, ...packetLogRef.current].slice(0, 100);
        setPackets(packetLogRef.current);
        break;
      }

      case CMD.GET_STATUS: {
        const mode = parsed.payload[0];
        const rssiVal = parsed.payload[1];
        setCurrentMode(mode);
        const decodedRssi = rssiVal >= 128
          ? ((rssiVal - 256) / 2) - 74
          : (rssiVal / 2) - 74;
        setRssi(Math.round(decodedRssi));
        break;
      }

      case CMD.ACK: {
        // Command acknowledged
        break;
      }

      case CMD.ERROR: {
        const errorCode = parsed.payload[0];
        Alert.alert('Device Error', `Error code: 0x${errorCode.toString(16).toUpperCase()}`);
        break;
      }
    }
  }, []);

  const startSubGhzRx = useCallback(() => {
    sendCommand(CMD.CC1101_INIT);
    const freqBytes = [
      subghzFreq & 0xFF,
      (subghzFreq >> 8) & 0xFF,
      (subghzFreq >> 16) & 0xFF,
    ];
    sendCommand(CMD.CC1101_SET_FREQ, freqBytes);
    sendCommand(CMD.CC1101_RX_START);
    setCurrentMode(MODE.SUBGHZ_RX);
    Vibration.vibrate(50);
  }, [sendCommand, subghzFreq]);

  const startSubGhzScan = useCallback(() => {
    sendCommand(CMD.CC1101_INIT);
    sendCommand(CMD.CC1101_SCAN_START);
    setCurrentMode(MODE.SUBGHZ_SCAN);
    setScanning(true);
    Vibration.vibrate(50);
  }, [sendCommand]);

  const stopAll = useCallback(() => {
    sendCommand(CMD.CC1101_RX_STOP);
    sendCommand(CMD.CC1101_SCAN_STOP);
    sendCommand(CMD.NRF24_RX_STOP);
    sendCommand(CMD.NRF24_SCAN_STOP);
    sendCommand(CMD.SET_MODE, [MODE.IDLE]);
    setCurrentMode(MODE.IDLE);
    setScanning(false);
  }, [sendCommand]);

  const startNrf24Rx = useCallback(() => {
    sendCommand(CMD.NRF24_INIT);
    sendCommand(CMD.NRF24_SET_CHANNEL, [nrf24Channel]);
    sendCommand(CMD.NRF24_RX_START);
    setCurrentMode(MODE.NRF24_RX);
    Vibration.vibrate(50);
  }, [sendCommand, nrf24Channel]);

  const startNrf24Scan = useCallback(() => {
    sendCommand(CMD.NRF24_INIT);
    sendCommand(CMD.NRF24_SCAN_START);
    setCurrentMode(MODE.NRF24_SCAN);
    setScanning(true);
    Vibration.vibrate(50);
  }, [sendCommand]);

  const transmitSubGhz = useCallback(() => {
    const testData = [0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04];
    sendCommand(CMD.CC1101_TX, [testData.length, ...testData]);
  }, [sendCommand]);

  const transmitNrf24 = useCallback(() => {
    const testData = [0xCA, 0xFE, 0xBA, 0xBE];
    sendCommand(CMD.NRF24_TX, [testData.length, ...testData]);
  }, [sendCommand]);

  const clearPackets = useCallback(() => {
    packetLogRef.current = [];
    setPackets([]);
  }, []);

  const renderPacket = useCallback(({ item }) => (
    <View style={styles.packetItem}>
      <View style={styles.packetHeader}>
        <Text style={[
          styles.packetType,
          item.type === 'subghz' ? styles.subghzLabel : styles.nrf24Label,
        ]}>
          {item.type === 'subghz' ? 'Sub-GHz' : '2.4 GHz'}
        </Text>
        <Text style={styles.packetTime}>{item.timestamp}</Text>
      </View>
      <View style={styles.packetDetails}>
        {item.rssi !== undefined && (
          <Text style={styles.packetInfo}>RSSI: {Math.round(item.rssi)} dBm</Text>
        )}
        {item.lqi !== undefined && (
          <Text style={styles.packetInfo}>LQI: {item.lqi}</Text>
        )}
        {item.pipe !== undefined && (
          <Text style={styles.packetInfo}>Pipe: {item.pipe}</Text>
        )}
        <Text style={styles.packetInfo}>Len: {item.length}</Text>
      </View>
      <Text style={styles.packetData}>
        {item.data.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ')}
      </Text>
    </View>
  ), []);

  return (
    <View style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />

      {/* Status Cards */}
      <View style={styles.statusRow}>
        <StatusCard
          title="Connection"
          value={connected ? 'Connected' : 'Disconnected'}
          color={connected ? '#00ff88' : '#ff4444'}
          icon="link"
        />
        <StatusCard
          title="Mode"
          value={MODE_NAMES[currentMode] || 'Unknown'}
          color="#4ecdc4"
          icon="radio"
        />
        <StatusCard
          title="RSSI"
          value={`${rssi} dBm`}
          color={rssi > -50 ? '#00ff88' : rssi > -80 ? '#ffaa00' : '#ff4444'}
          icon="signal"
        />
      </View>

      {/* Sub-GHz Controls */}
      <View style={styles.sectionContainer}>
        <Text style={styles.sectionTitle}>Sub-GHz (CC1101)</Text>
        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.button, styles.rxButton]}
            onPress={startSubGhzRx}
            disabled={!connected || currentMode !== MODE.IDLE}
          >
            <Text style={styles.buttonText}>RX</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.scanButton]}
            onPress={startSubGhzScan}
            disabled={!connected || currentMode !== MODE.IDLE}
          >
            <Text style={styles.buttonText}>Scan</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.txButton]}
            onPress={transmitSubGhz}
            disabled={!connected || currentMode !== MODE.IDLE}
          >
            <Text style={styles.buttonText}>TX</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* 2.4 GHz Controls */}
      <View style={styles.sectionContainer}>
        <Text style={styles.sectionTitle}>2.4 GHz (nRF24L01+)</Text>
        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.button, styles.rxButton]}
            onPress={startNrf24Rx}
            disabled={!connected || currentMode !== MODE.IDLE}
          >
            <Text style={styles.buttonText}>RX</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.scanButton]}
            onPress={startNrf24Scan}
            disabled={!connected || currentMode !== MODE.IDLE}
          >
            <Text style={styles.buttonText}>Scan</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.txButton]}
            onPress={transmitNrf24}
            disabled={!connected || currentMode !== MODE.IDLE}
          >
            <Text style={styles.buttonText}>TX</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Stop & Settings */}
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.button, styles.stopButton]}
          onPress={stopAll}
          disabled={currentMode === MODE.IDLE}
        >
          <Text style={styles.buttonText}>⏹ Stop</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.button, styles.settingsButton]}
          onPress={() => navigation.navigate('Settings')}
        >
          <Text style={styles.buttonText}>⚙ Settings</Text>
        </TouchableOpacity>
      </View>

      {/* Packet Log */}
      <View style={styles.logContainer}>
        <View style={styles.logHeader}>
          <Text style={styles.logTitle}>Packet Log</Text>
          <TouchableOpacity onPress={clearPackets}>
            <Text style={styles.clearButton}>Clear</Text>
          </TouchableOpacity>
        </View>
        <FlatList
          data={packets}
          renderItem={renderPacket}
          keyExtractor={(item) => item.id.toString()}
          style={styles.packetList}
          inverted={false}
        />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
    padding: 8,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 12,
  },
  sectionContainer: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  sectionTitle: {
    color: '#e0e0e0',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  button: {
    borderRadius: 8,
    paddingVertical: 10,
    paddingHorizontal: 16,
    alignItems: 'center',
    justifyContent: 'center',
    minWidth: 80,
  },
  rxButton: {
    backgroundColor: '#2196F3',
  },
  scanButton: {
    backgroundColor: '#FF9800',
  },
  txButton: {
    backgroundColor: '#4CAF50',
  },
  stopButton: {
    backgroundColor: '#f44336',
    flex: 1,
    marginRight: 8,
  },
  settingsButton: {
    backgroundColor: '#607D8B',
    flex: 1,
    marginLeft: 8,
  },
  buttonText: {
    color: '#ffffff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  actionRow: {
    flexDirection: 'row',
    marginBottom: 8,
  },
  logContainer: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 8,
  },
  logHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 4,
  },
  logTitle: {
    color: '#00ff88',
    fontSize: 14,
    fontWeight: 'bold',
  },
  clearButton: {
    color: '#ff4444',
    fontSize: 12,
  },
  packetList: {
    flex: 1,
  },
  packetItem: {
    backgroundColor: '#0f3460',
    borderRadius: 4,
    padding: 8,
    marginBottom: 4,
  },
  packetHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 2,
  },
  packetType: {
    fontSize: 12,
    fontWeight: 'bold',
    paddingHorizontal: 6,
    borderRadius: 3,
    overflow: 'hidden',
  },
  subghzLabel: {
    color: '#4ecdc4',
    backgroundColor: '#1a3a5c',
  },
  nrf24Label: {
    color: '#ff6b6b',
    backgroundColor: '#3a1a1a',
  },
  packetTime: {
    color: '#888888',
    fontSize: 10,
  },
  packetDetails: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 2,
  },
  packetInfo: {
    color: '#cccccc',
    fontSize: 11,
  },
  packetData: {
    color: '#00ff88',
    fontSize: 10,
    fontFamily: 'Courier',
  },
});