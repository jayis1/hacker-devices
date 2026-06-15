/**
 * SnifferScreen — BLE packet capture and display
 *
 * Captures BLE packets from Radio A/B via USB and displays
 * them in a scrolling list with filtering and export options.
 */

import React, { useState, useEffect, useRef, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  Switch,
  TextInput,
  Alert,
} from 'react-native';
import { ConnectionContext } from '../components/ConnectionContext';

// BLE ADV types
const ADV_TYPES = {
  0x00: 'ADV_IND',
  0x01: 'ADV_DIRECT_IND',
  0x02: 'ADV_NONCONN_IND',
  0x03: 'SCAN_REQ',
  0x04: 'SCAN_RSP',
  0x05: 'ADV_EXT_IND',
};

// BLE channel names
const CHANNELS = {
  37: 'ADV 37',
  38: 'ADV 38',
  39: 'ADV 39',
};

export default function SnifferScreen() {
  const { connected, sendCommand, packetStream } = useContext(ConnectionContext);
  const [capturing, setCapturing] = useState(false);
  const [packets, setPackets] = useState([]);
  const [filterChannel, setFilterChannel] = useState('all');
  const [filterAddr, setFilterAddr] = useState('');
  const [showData, setShowData] = useState(true);
  const [radioSource, setRadioSource] = useState('both'); // 'A', 'B', 'both'
  const packetCountRef = useRef(0);
  const flatListRef = useRef(null);

  useEffect(() => {
    if (!capturing) return;

    const unsubscribe = packetStream.subscribe(packet => {
      const parsed = parsePacket(packet);
      if (parsed) {
        setPackets(prev => {
          const updated = [...prev, parsed];
          // Keep max 500 packets
          if (updated.length > 500) return updated.slice(-500);
          return updated;
        });
        packetCountRef.current++;
      }
    });

    return () => unsubscribe();
  }, [capturing, packetStream]);

  const parsePacket = (raw) => {
    if (!raw || raw.length < 4) return null;

    const radioId = raw[0];     // 0x01 = Radio A, 0x02 = Radio B
    const cmd = raw[1];
    const len = (raw[2] << 8) | raw[3];
    const data = raw.slice(4, 4 + len);

    return {
      id: packetCountRef.current++,
      radio: radioId === 0x01 ? 'A' : 'B',
      timestamp: Date.now(),
      channel: data[0] || 0,
      rssi: data[1] ? -(256 - data[1]) : 0,
      pduType: data[2] || 0,
      length: data[3] || 0,
      addr: data.slice(4, 10).map(b => b.toString(16).padStart(2, '0')).join(':'),
      data: data.slice(10),
      raw: raw,
    };
  };

  const startCapture = () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to BLE Phantom first');
      return;
    }
    // Put device in sniff mode and start scanning
    sendCommand([0xA0, 0x01]) // Set mode to SNIFF
      .then(() => {
        if (radioSource === 'A' || radioSource === 'both') {
          sendCommand([0xA2, 0x00]); // Start Radio A
        }
        if (radioSource === 'B' || radioSource === 'both') {
          sendCommand([0xA2, 0x01]); // Start Radio B
        }
        setCapturing(true);
      });
  };

  const stopCapture = () => {
    setCapturing(false);
    sendCommand([0xA3, 0x00]); // Stop Radio A
    sendCommand([0xA3, 0x01]); // Stop Radio B
  };

  const clearPackets = () => {
    setPackets([]);
    packetCountRef.current = 0;
  };

  const filteredPackets = packets.filter(pkt => {
    if (filterChannel !== 'all' && pkt.channel !== parseInt(filterChannel)) return false;
    if (filterAddr && !pkt.addr.includes(filterAddr.toLowerCase())) return false;
    return true;
  });

  const renderPacket = ({ item }) => (
    <View style={[
      styles.packetRow,
      item.radio === 'A' ? styles.packetRadioA : styles.packetRadioB,
    ]}>
      <View style={styles.packetHeader}>
        <Text style={[styles.packetRadio, item.radio === 'A' ? styles.radioAText : styles.radioBText]}>
          R{item.radio}
        </Text>
        <Text style={styles.packetTime}>
          {new Date(item.timestamp).toLocaleTimeString('en-US', { hour12: false })}
        </Text>
        <Text style={styles.packetChannel}>
          {CHANNELS[item.channel] || `CH${item.channel}`}
        </Text>
        <Text style={styles.packetRSSI}>
          {item.rssi} dBm
        </Text>
      </View>
      <Text style={styles.packetAddr}>{item.addr}</Text>
      <Text style={styles.packetType}>
        {ADV_TYPES[item.pduType] || `Type 0x${item.pduType.toString(16)}`}
      </Text>
      {showData && item.data.length > 0 && (
        <Text style={styles.packetData}>
          {Array.from(item.data).map(b => b.toString(16).padStart(2, '0')).join(' ')}
        </Text>
      )}
    </View>
  );

  return (
    <View style={styles.container}>
      {/* Control Bar */}
      <View style={styles.controlBar}>
        <TouchableOpacity
          style={[styles.captureButton, capturing && styles.captureButtonActive]}
          onPress={capturing ? stopCapture : startCapture}
        >
          <Text style={styles.captureButtonText}>
            {capturing ? '■ STOP' : '▶ CAPTURE'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.clearButton} onPress={clearPackets}>
          <Text style={styles.clearButtonText}>CLEAR</Text>
        </TouchableOpacity>
        <Text style={styles.packetCount}>{filteredPackets.length} pkts</Text>
      </View>

      {/* Filter Bar */}
      <View style={styles.filterBar}>
        <TextInput
          style={styles.filterInput}
          placeholder="Filter MAC..."
          placeholderTextColor="#666666"
          value={filterAddr}
          onChangeText={setFilterAddr}
          autoCapitalize="none"
        />
        <View style={styles.toggleContainer}>
          <Text style={styles.toggleLabel}>Show Data</Text>
          <Switch
            value={showData}
            onValueChange={setShowData}
            trackColor={{ false: '#333333', true: '#006644' }}
            thumbColor={showData ? '#00FF88' : '#666666'}
          />
        </View>
      </View>

      {/* Packet List */}
      <FlatList
        ref={flatListRef}
        data={filteredPackets}
        renderItem={renderPacket}
        keyExtractor={item => item.id.toString()}
        style={styles.packetList}
        inverted={false}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D0D1A',
  },
  controlBar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#1A1A2E',
  },
  captureButton: {
    backgroundColor: '#006644',
    paddingVertical: 8,
    paddingHorizontal: 16,
    borderRadius: 6,
    marginRight: 8,
  },
  captureButtonActive: {
    backgroundColor: '#662222',
  },
  captureButtonText: {
    color: '#00FF88',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
  },
  clearButton: {
    backgroundColor: '#333344',
    paddingVertical: 8,
    paddingHorizontal: 12,
    borderRadius: 6,
    marginRight: 8,
  },
  clearButtonText: {
    color: '#AAAAAA',
    fontFamily: 'monospace',
    fontSize: 12,
  },
  packetCount: {
    color: '#AAAAAA',
    fontFamily: 'monospace',
    fontSize: 12,
    marginLeft: 'auto',
  },
  filterBar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#111122',
  },
  filterInput: {
    flex: 1,
    backgroundColor: '#222244',
    color: '#FFFFFF',
    padding: 6,
    borderRadius: 4,
    fontFamily: 'monospace',
    fontSize: 12,
    marginRight: 8,
  },
  toggleContainer: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  toggleLabel: {
    color: '#AAAAAA',
    fontSize: 12,
    marginRight: 4,
  },
  packetList: {
    flex: 1,
  },
  packetRow: {
    padding: 8,
    marginVertical: 1,
    marginHorizontal: 4,
    borderRadius: 4,
  },
  packetRadioA: {
    backgroundColor: '#0A1A2E',
    borderLeftWidth: 2,
    borderLeftColor: '#00AAFF',
  },
  packetRadioB: {
    backgroundColor: '#1A1A0A',
    borderLeftWidth: 2,
    borderLeftColor: '#FFAA00',
  },
  packetHeader: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  packetRadio: {
    fontSize: 10,
    fontFamily: 'monospace',
    fontWeight: 'bold',
    width: 24,
  },
  radioAText: {
    color: '#00AAFF',
  },
  radioBText: {
    color: '#FFAA00',
  },
  packetTime: {
    color: '#888888',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 72,
  },
  packetChannel: {
    color: '#00FF88',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 60,
  },
  packetRSSI: {
    color: '#FF8888',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  packetAddr: {
    color: '#CCCCCC',
    fontSize: 11,
    fontFamily: 'monospace',
    marginTop: 2,
  },
  packetType: {
    color: '#AAAAAA',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  packetData: {
    color: '#666666',
    fontSize: 9,
    fontFamily: 'monospace',
    marginTop: 2,
  },
});