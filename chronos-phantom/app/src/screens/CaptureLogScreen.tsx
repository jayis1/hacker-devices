// src/screens/CaptureLogScreen.tsx — PTP/NTP frame capture log
//
// Author: jayis1
// License: GPL-2.0

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity } from 'react-native';
import { useBle } from '../ble/BleManager';
import { CMD, EVT, CapturedFrame } from '../types';
import { bytesToHex } from '../ble/protocol';
import FrameLogTable from '../components/FrameLogTable';

const MSG_TYPE_NAMES: Record<number, string> = {
  0x0: 'Sync', 0x1: 'Delay_Req', 0x2: 'Pdelay_Req', 0x3: 'Pdelay_Resp',
  0x8: 'Follow_Up', 0x9: 'Delay_Resp', 0xA: 'Pdelay_Resp_FU',
  0xB: 'Announce', 0xC: 'Signaling', 0xD: 'Management',
};

export default function CaptureLogScreen() {
  const { connected, sendCommand, onEvent } = useBle();
  const [capturing, setCapturing] = useState(false);
  const [frames, setFrames] = useState<CapturedFrame[]>([]);

  useEffect(() => {
    if (!connected) return;
    const unsub = onEvent((frame) => {
      if (frame.opcode === EVT.FRAME_CAPTURED) {
        const payload = frame.payload;
        if (payload.length < 10) return;
        const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
        const ts = Number(dv.getBigInt64(0, true));
        const frameLen = (payload[8] << 8) | payload[9];
        const frameData = payload.subarray(10, 10 + Math.min(frameLen, 128));
        // Parse Ethernet header
        if (frameData.length < 14) return;
        const dstMac = Array.from(frameData.subarray(0, 6))
          .map((b) => b.toString(16).padStart(2, '0')).join(':');
        const srcMac = Array.from(frameData.subarray(6, 12))
          .map((b) => b.toString(16).padStart(2, '0')).join(':');
        const ethType = (frameData[12] << 8) | frameData[13];
        let msgType = -1;
        let correctionField = '';
        if (ethType === 0x88F7 && frameData.length >= 14 + 34) {
          msgType = frameData[14] & 0x0F;
          const cf = Array.from(frameData.subarray(22, 30))
            .map((b) => b.toString(16).padStart(2, '0')).join('');
          correctionField = `0x${cf}`;
        }
        setFrames((prev) => [{
          timestampNs: ts,
          frameLen,
          msgType,
          srcMac,
          dstMac,
          correctionField,
          raw: bytesToHex(frameData),
        }, ...prev].slice(0, 200));
      }
    });
    return unsub;
  }, [connected, onEvent]);

  const toggleCapture = async () => {
    if (capturing) {
      await sendCommand(CMD.CAPTURE_STOP, new Uint8Array(0));
      setCapturing(false);
    } else {
      setFrames([]);
      await sendCommand(CMD.CAPTURE_START, new Uint8Array(0));
      setCapturing(true);
    }
  };

  const renderFrame = ({ item }: { item: CapturedFrame }) => (
    <View style={styles.frameCard}>
      <View style={styles.frameHeader}>
        <Text style={styles.frameType}>
          {item.msgType >= 0 ? MSG_TYPE_NAMES[item.msgType] || `PTP(0x${item.msgType.toString(16)})` : 'Non-PTP'}
        </Text>
        <Text style={styles.frameLen}>{item.frameLen} bytes</Text>
        <Text style={styles.frameTs}>{new Date(item.timestampNs / 1e6).toLocaleTimeString()}</Text>
      </View>
      <View style={styles.frameRow}>
        <Text style={styles.frameLabel}>src:</Text>
        <Text style={styles.frameValue}>{item.srcMac}</Text>
      </View>
      <View style={styles.frameRow}>
        <Text style={styles.frameLabel}>dst:</Text>
        <Text style={styles.frameValue}>{item.dstMac}</Text>
      </View>
      {item.correctionField && (
        <View style={styles.frameRow}>
          <Text style={styles.frameLabel}>cf:</Text>
          <Text style={styles.frameValue}>{item.correctionField}</Text>
        </View>
      )}
      <Text style={styles.frameRaw}>{item.raw}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Frame Capture</Text>
        <TouchableOpacity
          style={[styles.captureBtn, capturing ? styles.btnStop : styles.btnStart]}
          onPress={toggleCapture}
        >
          <Text style={styles.btnText}>
            {capturing ? '⏹ Stop' : '● Capture'}
          </Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.subtitle}>
        {capturing
          ? `${frames.length} frame${frames.length !== 1 ? 's' : ''} captured`
          : 'Tap Capture to start logging PTP/NTP frames'}
      </Text>

      <FlatList
        data={frames}
        renderItem={renderFrame}
        keyExtractor={(item, idx) => `${item.timestampNs}-${idx}`}
        contentContainerStyle={{ padding: 16, paddingBottom: 40 }}
        style={{ flex: 1 }}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No frames captured.</Text>
            <Text style={styles.emptyHint}>
              PTP frames (EtherType 0x88F7) and NTP (UDP 123) are logged with timestamps.
            </Text>
          </View>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a' },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', padding: 16 },
  title: { fontSize: 20, fontWeight: 'bold', color: '#00ff88' },
  captureBtn: { paddingHorizontal: 20, paddingVertical: 10, borderRadius: 8 },
  btnStart: { backgroundColor: '#aa3300' },
  btnStop: { backgroundColor: '#333' },
  btnText: { color: 'white', fontSize: 14, fontWeight: '600' },
  subtitle: { fontSize: 11, color: '#666', paddingHorizontal: 16, marginBottom: 8 },
  frameCard: { backgroundColor: '#1a1a1a', borderRadius: 8, padding: 12, marginBottom: 8 },
  frameHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  frameType: { color: '#00ff88', fontSize: 12, fontWeight: '600' },
  frameLen: { color: '#666', fontSize: 10 },
  frameTs: { color: '#444', fontSize: 10 },
  frameRow: { flexDirection: 'row', marginBottom: 4 },
  frameLabel: { color: '#666', fontSize: 10, width: 30, fontFamily: 'monospace' },
  frameValue: { color: '#999', fontSize: 10, fontFamily: 'monospace' },
  frameRaw: { color: '#444', fontSize: 8, fontFamily: 'monospace', marginTop: 6 },
  empty: { alignItems: 'center', marginTop: 40 },
  emptyText: { color: '#444', fontSize: 14 },
  emptyHint: { color: '#333', fontSize: 11, marginTop: 4, textAlign: 'center', paddingHorizontal: 20 },
});

// Author: jayis1
// License: GPL-2.0