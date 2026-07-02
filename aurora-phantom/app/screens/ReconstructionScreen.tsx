// screens/ReconstructionScreen.tsx — live coarse image + brightness trace
// Author: jayis1   License: GPL-2.0
import React, { useEffect, useRef } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { useStore } from '../src/store';
import { ble, FramePacket } from '../src/ble';

const W = 32; // upsampled preview width (device sends 16x16 -> 2x = 32)

export default function ReconstructionScreen() {
  const { frames, pushFrame } = useStore();
  const seqRef = useRef(0);

  useEffect(() => {
    ble.onFrame((p: FramePacket) => {
      if (p.kind === 'data') {
        pushFrame({
          seq: seqRef.current++,
          width: W,
          height: W,
          mags: p.bytes,
        });
      }
    });
  }, [pushFrame]);

  const latest = frames.length ? frames[frames.length - 1] : null;

  return (
    <ScrollView style={s.container}>
      <Text style={s.title}>Reconstruction</Text>
      <Text style={s.hint}>Coarse 32×32 preview — structure, not readable text</Text>
      {latest ? (
        <PixelGrid frame={latest} />
      ) : (
        <Text style={s.empty}>No frames received. Run a capture or rendezvous.</Text>
      )}
      <Text style={s.section}>Recent brightness trace</Text>
      <BrightnessTrace frames={frames.slice(-30)} />
    </ScrollView>
  );
}

function PixelGrid({ frame }: { frame: { width: number; height: number; mags: Uint8Array } }) {
  const cells: React.ReactNode[] = [];
  for (let y = 0; y < frame.height; y++) {
    for (let x = 0; x < frame.width; x++) {
      const idx = y * frame.width + x;
      const v = frame.mags[idx] ?? 0;
      const c = `rgb(${v},${v},${Math.min(255, v + 40)})`;
      cells.push(
        <View key={`${x}-${y}`} style={{ width: 8, height: 8, backgroundColor: c }} />
      );
    }
  }
  return (
    <View style={{
      flexDirection: 'row', flexWrap: 'wrap', width: frame.width * 8,
      backgroundColor: '#000', alignSelf: 'center', marginBottom: 16,
    }}>
      {cells}
    </View>
  );
}

function BrightnessTrace({ frames }: { frames: { mags: Uint8Array }[] }) {
  if (!frames.length) return <Text style={s.empty}>No trace data.</Text>;
  const avgs = frames.map(f => {
    let s = 0;
    for (let i = 0; i < f.mags.length; i++) s += f.mags[i];
    return s / (f.mags.length || 1);
  });
  const maxA = Math.max(...avgs, 1);
  return (
    <View style={s.traceWrap}>
      {avgs.map((a, i) => {
        const h = (a / maxA) * 100;
        return <View key={i} style={[s.traceBar, { height: `${h}%` }]} />;
      })}
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#7fd4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 8 },
  hint: { color: '#888', fontSize: 12, marginBottom: 16 },
  section: { color: '#7fd4ff', fontSize: 16, fontWeight: '600', marginTop: 8, marginBottom: 8 },
  empty: { color: '#555', textAlign: 'center', marginTop: 20 },
  traceWrap: { flexDirection: 'row', alignItems: 'flex-end', height: 120, backgroundColor: '#0e0e18', borderRadius: 6, padding: 8 },
  traceBar: { flex: 1, marginHorizontal: 1, backgroundColor: '#5cff8a', borderRadius: 2 },
});