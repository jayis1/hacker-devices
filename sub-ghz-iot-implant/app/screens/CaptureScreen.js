/**
 * CaptureScreen.js — Live Packet Capture & Monitoring
 * Displays captured Sub-GHz packets in real-time with filtering
 */

import React, { useContext, useState, useEffect, useRef } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  ScrollView,
} from 'react-native';
import { BleContext } from '../components/BleContext';
import StatusCard from '../components/StatusCard';
import PacketList from '../components/PacketList';
import { decodePacket, protocolNames } from '../utils/protocol';

export default function CaptureScreen() {
  const {
    connectionState,
    startCapture,
    stopCapture,
    isCapturing,
    packets,
    clearPackets,
    sendCommand,
  } = useContext(BleContext);

  const [filter, setFilter] = useState('all'); // all, zigbee, zwave, subghz, ble
  const [stats, setStats] = useState({
    total: 0,
    zigbee: 0,
    zwave: 0,
    subghz: 0,
    errors: 0,
  });

  // Update stats when packets change
  useEffect(() => {
    const newStats = { total: 0, zigbee: 0, zwave: 0, subghz: 0, errors: 0 };
    packets.forEach((pkt) => {
      newStats.total++;
      if (pkt.protocol === 'zigbee') newStats.zigbee++;
      else if (pkt.protocol === 'zwave') newStats.zwave++;
      else newStats.subghz++;
      if (!pkt.crcValid) newStats.errors++;
    });
    setStats(newStats);
  }, [packets]);

  const filteredPackets = packets.filter((pkt) => {
    if (filter === 'all') return true;
    return pkt.protocol === filter;
  });

  const handleStartStop = () => {
    if (isCapturing) {
      stopCapture();
    } else {
      startCapture();
    }
  };

  const handleReplay = (packet) => {
    if (connectionState !== 'connected') return;
    // Send replay command with packet data
    sendCommand('REPLAY', {
      frequency: packet.frequency,
      modulation: packet.modulation,
      data: packet.rawData,
    });
  };

  return (
    <View style={styles.container}>
      {/* Capture Stats */}
      <View style={styles.statsContainer}>
        <TouchableOpacity
          style={[styles.statChip, filter === 'all' && styles.statChipActive]}
          onPress={() => setFilter('all')}
        >
          <Text style={[styles.statText, filter === 'all' && styles.statTextActive]}>
            All ({stats.total})
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.statChip, filter === 'zigbee' && styles.statChipActive]}
          onPress={() => setFilter('zigbee')}
        >
          <Text style={[styles.statText, filter === 'zigbee' && styles.statTextActive]}>
            Zigbee ({stats.zigbee})
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.statChip, filter === 'zwave' && styles.statChipActive]}
          onPress={() => setFilter('zwave')}
        >
          <Text style={[styles.statText, filter === 'zwave' && styles.statTextActive]}>
            Z-Wave ({stats.zwave})
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.statChip, filter === 'subghz' && styles.statChipActive]}
          onPress={() => setFilter('subghz')}
        >
          <Text style={[styles.statText, filter === 'subghz' && styles.statTextActive]}>
            Sub-GHz ({stats.subghz})
          </Text>
        </TouchableOpacity>
      </View>

      {/* Error count */}
      {stats.errors > 0 && (
        <View style={styles.errorBar}>
          <Text style={styles.errorText}>
            ⚠ {stats.errors} CRC errors
          </Text>
        </View>
      )}

      {/* Packet List */}
      <PacketList
        packets={filteredPackets}
        onReplay={handleReplay}
        style={styles.packetList}
      />

      {/* Action Buttons */}
      <View style={styles.buttonContainer}>
        <TouchableOpacity
          style={[
            styles.captureButton,
            isCapturing ? styles.stopButton : styles.startButton,
          ]}
          onPress={handleStartStop}
        >
          <Text style={styles.captureButtonText}>
            {isCapturing ? '■  Stop Capture' : '●  Start Capture'}
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={styles.clearButton}
          onPress={clearPackets}
        >
          <Text style={styles.clearButtonText}>Clear</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F23',
  },
  statsContainer: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    padding: 8,
    gap: 6,
  },
  statChip: {
    backgroundColor: '#1A1A3E',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 16,
    borderWidth: 1,
    borderColor: '#2A2A4E',
  },
  statChipActive: {
    backgroundColor: '#00E676',
    borderColor: '#00E676',
  },
  statText: {
    color: '#B0BEC5',
    fontSize: 12,
    fontWeight: '600',
  },
  statTextActive: {
    color: '#000000',
  },
  errorBar: {
    backgroundColor: '#4E342E',
    paddingHorizontal: 12,
    paddingVertical: 4,
    marginHorizontal: 8,
  },
  errorText: {
    color: '#FFC107',
    fontSize: 12,
  },
  packetList: {
    flex: 1,
  },
  buttonContainer: {
    flexDirection: 'row',
    padding: 12,
    gap: 8,
  },
  captureButton: {
    flex: 3,
    paddingVertical: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  startButton: {
    backgroundColor: '#00E676',
  },
  stopButton: {
    backgroundColor: '#F44336',
  },
  captureButtonText: {
    color: '#000000',
    fontSize: 16,
    fontWeight: '700',
  },
  clearButton: {
    flex: 1,
    paddingVertical: 14,
    borderRadius: 8,
    alignItems: 'center',
    backgroundColor: '#1A1A3E',
    borderWidth: 1,
    borderColor: '#2A2A4E',
  },
  clearButtonText: {
    color: '#B0BEC5',
    fontSize: 14,
    fontWeight: '600',
  },
});