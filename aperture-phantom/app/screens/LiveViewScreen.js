/**
 * AperturePhantom — LiveViewScreen
 *
 * Streams downscaled frames from the device over BLE and renders them as a
 * pseudo-image using a simple YUV→RGB conversion. Also offers a snapshot
 * button that saves the current frame into a capture slot on the device.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useEffect, useRef, useState } from 'react';
import { View, Text, Button, StyleSheet, ViewStyle } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function LiveViewScreen() {
  const { liveFrame, streamFrames, snapshot, send } = useDevice();
  const [streaming, setStreaming] = useState(false);
  const lastSeq = useRef(0);

  useEffect(() => {
    /* start streaming on mount */
    setStreaming(true);
    streamFrames(true);
    return () => streamFrames(false);
  }, []);

  const toggleStream = () => {
    const on = !streaming;
    setStreaming(on);
    streamFrames(on);
  };

  /* Render the latest frame as a grid of colored cells. The frame payload
   * from the device is a 16x16 RGB thumbnail (768 bytes). */
  const renderThumb = () => {
    if (!liveFrame || !liveFrame.data || liveFrame.data.length < 16) {
      return <Text style={styles.placeholder}>no frames yet…</Text>;
    }
    const d = liveFrame.data;
    const N = 16;
    const cellSize = 14;
    const cells = [];
    for (let i = 0; i < N * N; i++) {
      const r = d[i * 3] || 0;
      const g = d[i * 3 + 1] || 0;
      const b = d[i * 3 + 2] || 0;
      cells.push(
        <View key={i} style={{
          width: cellSize, height: cellSize,
          backgroundColor: `rgb(${r},${g},${b})`,
        }} />
      );
    }
    return (
      <View style={{ flexDirection: 'row', flexWrap: 'wrap',
                     width: N * cellSize, height: N * cellSize }}>
        {cells}
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>Live View (sensor-side, downscaled)</Text>
      <View style={styles.frameBox}>{renderThumb()}</View>
      {liveFrame && (
        <Text style={styles.meta}>
          dt=0x{liveFrame.dt.toString(16)} seq={liveFrame.seq} len={liveFrame.len}
        </Text>
      )}
      <View style={styles.row}>
        <Button title={streaming ? 'Pause' : 'Stream'} onPress={toggleStream} />
        <Button title="Snapshot" onPress={snapshot} />
      </View>
      <Text style={styles.note}>
        Full-resolution frames are captured to the SD card on the device; the
        phone preview is a 16×16 downscaled thumbnail for low-bandwidth BLE.
      </Text>
      <Text style={styles.author}>author: jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#101418',
               alignItems: 'center' },
  h1: { color: '#d0e0f0', fontSize: 16, fontWeight: '600', marginBottom: 12 },
  frameBox: { backgroundColor: '#000', padding: 8, borderRadius: 6,
              marginVertical: 12, minHeight: 270, minWidth: 270,
              justifyContent: 'center', alignItems: 'center' },
  placeholder: { color: '#607080', fontSize: 12 },
  meta: { color: '#8aa0b8', fontSize: 10, fontFamily: 'monospace' },
  row: { flexDirection: 'row', gap: 12, marginVertical: 12 },
  note: { color: '#607080', fontSize: 11, marginTop: 16, textAlign: 'center' },
  author: { color: '#5a7088', fontSize: 10, marginTop: 16 },
});