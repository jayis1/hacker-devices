/**
 * WFP-100 Capture Screen — Live packet capture display
 *
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, FlatList, StyleSheet, TouchableOpacity } from 'react-native';
import { buildFrame, CMD_STOP_CAPTURE, CMD_SET_CHANNEL, parsePacketPayload, RESP_PACKET, BAND_2GHZ } from '../utils/protocol';
import PacketList from '../components/PacketList';

export default function CaptureScreen({ navigation }) {
  const [packets, setPackets] = useState([]);
  const [capturing, setCapturing] = useState(true);
  const [channel, setChannel] = useState(6);
  const [band, setBand] = useState(BAND_2GHZ);
  const [stats, setStats] = useState({ total: 0, mgmt: 0, data: 0, ctrl: 0 });
  const packetIdRef = useRef(0);

  useEffect(() => {
    // In production, this would listen on the USB/BLE serial stream
    // and parse incoming RESP_PACKET frames
    return () => {
      if (capturing) {
        sendCommand(CMD_STOP_CAPTURE, null);
      }
    };
  }, []);

  const sendCommand = (cmd, payload) => {
    const frame = buildFrame(cmd, payload);
    console.log(`TX [${cmd.toString(16).padStart(2, '0')}]: ${Array.from(frame).map(b => b.toString(16).padStart(2, '0')).join(' ')}`);
  };

  const handleStop = () => {
    sendCommand(CMD_STOP_CAPTURE, null);
    setCapturing(false);
    navigation.goBack();
  };

  const handleChannelHop = () => {
    // Cycle through common 2.4 GHz channels: 1, 6, 11
    const channels = [1, 6, 11];
    const nextIdx = (channels.indexOf(channel) + 1) % channels.length;
    const newChannel = channels[nextIdx];
    setChannel(newChannel);
    const payload = new Uint8Array(3);
    payload[0] = newChannel & 0xFF;
    payload[1] = (newChannel >> 8) & 0xFF;
    payload[2] = band;
    sendCommand(CMD_SET_CHANNEL, payload);
  };

  const getFrameType = (frame) => {
    if (!frame || frame.length < 2) return 'Unknown';
    const type = (frame[0] >> 2) & 0x03;
    const subtype = (frame[0] >> 4) & 0x0F;
    if (type === 0) {
      const names = {
        0: 'Assoc Req', 1: 'Assoc Resp', 2: 'Reassoc Req', 3: 'Reassoc Resp',
        4: 'Probe Req', 5: 'Probe Resp', 8: 'Beacon', 10: 'Disassoc', 11: 'Auth',
        12: 'Deauth', 13: 'Action'
      };
      return names[subtype] || `Mgmt(${subtype})`;
    }
    if (type === 1) return 'Ctrl';
    if (type === 2) return 'Data';
    return 'Unknown';
  };

  const renderPacket = ({ item }) => (
    <View style={styles.packetRow}>
      <Text style={styles.packetId}>#{item.id}</Text>
      <Text style={styles.packetType}>{item.type}</Text>
      <Text style={styles.packetInfo}>Ch {item.channel} | {item.rssi} dBm | {item.len} B</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <TouchableOpacity style={styles.stopButton} onPress={handleStop}>
          <Text style={styles.stopButtonText}>Stop</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.hopButton} onPress={handleChannelHop}>
          <Text style={styles.hopButtonText}>Hop → Ch {channel}</Text>
        </TouchableOpacity>
        <Text style={styles.statsText}>
          {stats.total} packets | {stats.mgmt} mgmt | {stats.data} data
        </Text>
      </View>

      <FlatList
        data={packets}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderPacket}
        style={styles.packetList}
        inverted={false}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a4a',
  },
  stopButton: {
    backgroundColor: '#ff4444',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 6,
    marginRight: 12,
  },
  stopButtonText: {
    color: '#fff',
    fontWeight: '600',
    fontSize: 14,
  },
  hopButton: {
    backgroundColor: '#0ea5e9',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 6,
    marginRight: 12,
  },
  hopButtonText: {
    color: '#1a1a2e',
    fontWeight: '600',
    fontSize: 14,
  },
  statsText: {
    color: '#00ff88',
    fontSize: 12,
    flex: 1,
    textAlign: 'right',
  },
  packetList: {
    flex: 1,
  },
  packetRow: {
    flexDirection: 'row',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderBottomWidth: 0.5,
    borderBottomColor: '#1a1a2e',
  },
  packetId: {
    color: '#666',
    fontSize: 11,
    width: 50,
  },
  packetType: {
    color: '#00ff88',
    fontSize: 12,
    fontWeight: '600',
    width: 90,
  },
  packetInfo: {
    color: '#888',
    fontSize: 11,
    flex: 1,
  },
});