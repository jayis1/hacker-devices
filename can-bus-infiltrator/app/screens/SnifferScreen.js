/**
 * SnifferScreen.js — Real-time CAN bus frame viewer
 * Displays captured CAN frames with filtering and protocol decoding
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  View,
  Text,
  FlatList,
  StyleSheet,
  TouchableOpacity,
  TextInput,
  Switch,
  StatusBar,
} from 'react-native';
import { Buffer } from 'buffer';
import { BLE_PROTOCOL, parseCanFrame } from '../utils/protocol';

const BAUD_RATES = [
  { label: '125K', value: 0 },
  { label: '250K', value: 1 },
  { label: '500K', value: 2 },
  { label: '1M', value: 3 },
];

export default function SnifferScreen({ navigation }) {
  const [frames, setFrames] = useState([]);
  const [isSniffing, setIsSniffing] = useState(false);
  const [baudRate, setBaudRate] = useState(2); // 500K default
  const [channel, setChannel] = useState(0);
  const [filterId, setFilterId] = useState('');
  const [autoScroll, setAutoScroll] = useState(true);
  const [frameCount, setFrameCount] = useState(0);
  const [errorCount, setErrorCount] = useState(0);
  const [connected, setConnected] = useState(false);
  const flatListRef = useRef(null);
  const maxFrames = 500;

  const handleNewFrame = useCallback((frame) => {
    setFrames(prev => {
      const newFrames = [...prev, frame];
      if (newFrames.length > maxFrames) {
        return newFrames.slice(-maxFrames);
      }
      return newFrames;
    });
    setFrameCount(prev => prev + 1);
  }, []);

  const startSniffing = async () => {
    if (!connected) {
      // Attempt BLE connection first
      const result = await BLE_PROTOCOL.connect();
      if (result) setConnected(true);
      else return;
    }

    const cmd = new Uint8Array([channel, baudRate]);
    await BLE_PROTOCOL.sendCommand(0x01, cmd);
    setIsSniffing(true);
  };

  const stopSniffing = async () => {
    const cmd = new Uint8Array([channel]);
    await BLE_PROTOCOL.sendCommand(0x02, cmd);
    setIsSniffing(false);
  };

  const clearFrames = () => {
    setFrames([]);
    setFrameCount(0);
    setErrorCount(0);
  };

  const renderFrame = ({ item, index }) => {
    const isFiltered = filterId && !item.id.toString(16).toUpperCase().startsWith(filterId.toUpperCase());
    if (isFiltered) return null;

    return (
      <View style={[styles.frameRow, index % 2 === 0 ? styles.frameRowEven : null]}>
        <Text style={[styles.frameCell, styles.frameCh]}>
          CH{item.channel}
        </Text>
        <Text style={[styles.frameCell, styles.frameId]}>
          {item.idType ? 'E' : 'S'} {item.id.toString(16).toUpperCase().padStart(item.idType ? 8 : 3, '0')}
        </Text>
        <Text style={[styles.frameCell, styles.frameDlc]}>
          [{item.dlc}]
        </Text>
        <Text style={[styles.frameCell, styles.frameData]}>
          {item.data.slice(0, item.dlc).map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')}
        </Text>
        <Text style={[styles.frameCell, styles.frameTime]}>
          {(item.timestamp / 1000).toFixed(3)}s
        </Text>
        {item.rtr ? <Text style={styles.rtrBadge}>RTR</Text> : null}
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <StatusBar barStyle="light-content" />

      {/* Control Bar */}
      <View style={styles.controlBar}>
        <TouchableOpacity
          style={[styles.button, isSniffing ? styles.buttonStop : styles.buttonStart]}
          onPress={isSniffing ? stopSniffing : startSniffing}
        >
          <Text style={styles.buttonText}>
            {isSniffing ? '■ STOP' : '▶ SNIFF'}
          </Text>
        </TouchableOpacity>

        <View style={styles.baudSelector}>
          {BAUD_RATES.map(br => (
            <TouchableOpacity
              key={br.value}
              style={[styles.baudButton, baudRate === br.value ? styles.baudActive : null]}
              onPress={() => setBaudRate(br.value)}
            >
              <Text style={[styles.baudText, baudRate === br.value ? styles.baudTextActive : null]}>
                {br.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <View style={styles.channelSelector}>
          <TouchableOpacity
            style={[styles.chButton, channel === 0 ? styles.chActive : null]}
            onPress={() => setChannel(0)}
          >
            <Text style={styles.chText}>CH1</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.chButton, channel === 1 ? styles.chActive : null]}
            onPress={() => setChannel(1)}
          >
            <Text style={styles.chText}>CH2</Text>
          </TouchableOpacity>
        </View>

        <TouchableOpacity style={styles.button} onPress={clearFrames}>
          <Text style={styles.buttonText}>CLR</Text>
        </TouchableOpacity>
      </View>

      {/* Filter Bar */}
      <View style={styles.filterBar}>
        <Text style={styles.filterLabel}>ID Filter:</Text>
        <TextInput
          style={styles.filterInput}
          value={filterId}
          onChangeText={setFilterId}
          placeholder="Hex ID prefix"
          placeholderTextColor="#666"
        />
        <Text style={styles.statsText}>
          {frameCount} frames | {errorCount} errors
        </Text>
      </View>

      {/* Frame List */}
      <FlatList
        ref={flatListRef}
        data={frames}
        renderItem={renderFrame}
        keyExtractor={(item, idx) => `${item.timestamp}-${idx}`}
        style={styles.frameList}
        onContentSizeChange={() => {
          if (autoScroll) flatListRef.current?.scrollToEnd();
        }}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0D1117' },
  controlBar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#161B22',
    borderBottomWidth: 1,
    borderBottomColor: '#30363D',
  },
  button: {
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 6,
    backgroundColor: '#21262D',
    marginRight: 8,
  },
  buttonStart: { backgroundColor: '#238636' },
  buttonStop: { backgroundColor: '#DA3633' },
  buttonText: { color: '#FFFFFF', fontSize: 12, fontWeight: 'bold' },
  baudSelector: { flexDirection: 'row', marginRight: 8 },
  baudButton: {
    paddingHorizontal: 8,
    paddingVertical: 6,
    backgroundColor: '#21262D',
    borderRadius: 4,
    marginHorizontal: 2,
  },
  baudActive: { backgroundColor: '#1F6FEB' },
  baudText: { color: '#8B949E', fontSize: 10 },
  baudTextActive: { color: '#FFFFFF', fontSize: 10 },
  channelSelector: { flexDirection: 'row', marginRight: 8 },
  chButton: {
    paddingHorizontal: 8,
    paddingVertical: 6,
    backgroundColor: '#21262D',
    borderRadius: 4,
    marginHorizontal: 2,
  },
  chActive: { backgroundColor: '#8957E5' },
  chText: { color: '#C9D1D9', fontSize: 10 },
  filterBar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#161B22',
  },
  filterLabel: { color: '#8B949E', fontSize: 12, marginRight: 8 },
  filterInput: {
    flex: 1,
    height: 32,
    backgroundColor: '#0D1117',
    color: '#C9D1D9',
    borderRadius: 4,
    paddingHorizontal: 8,
    fontSize: 12,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  statsText: { color: '#8B949E', fontSize: 11, marginLeft: 8 },
  frameList: { flex: 1 },
  frameRow: {
    flexDirection: 'row',
    paddingVertical: 2,
    paddingHorizontal: 4,
    borderBottomWidth: 0.5,
    borderBottomColor: '#21262D',
  },
  frameRowEven: { backgroundColor: '#0D1117' },
  frameCell: { color: '#C9D1D9', fontSize: 11, fontFamily: 'Courier' },
  frameCh: { width: 32, color: '#8957E5' },
  frameId: { width: 80, color: '#79C0FF' },
  frameDlc: { width: 28, color: '#D2A8FF' },
  frameData: { flex: 1, color: '#7EE787' },
  frameTime: { width: 72, color: '#8B949E', textAlign: 'right' },
  rtrBadge: { color: '#F85149', fontSize: 9, marginLeft: 4 },
});