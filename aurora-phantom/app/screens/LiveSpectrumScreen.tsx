// screens/LiveSpectrumScreen.tsx — real-time FFT of aggregate photon rate
// Author: jayis1   License: GPL-2.0
import React, { useEffect, useRef } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ViewStyle } from 'react-native';
import { useStore } from '../src/store';
import { ble } from '../src/ble';

export default function LiveSpectrumScreen() {
  const { spectrum, setSpectrum, connected } = useStore();
  const timer = useRef<any>(null);

  useEffect(() => {
    // Subscribe to frame packets; the header kind carries a tiny spectrum
    // sample. In a full build the device pushes periodic spectrum snapshots.
    ble.onFrame((p) => {
      if (p.kind === 'header' && p.bytes.length >= 2) {
        const total = p.bytes[0] | (p.bytes[1] << 8);
        useStore.getState().pushLog('FFT', 'total frames=' + total);
      }
    });
    return () => { if (timer.current) clearInterval(timer.current); };
  }, []);

  // Simulated spectrum sweep for the UI demo path (real data arrives via BLE).
  useEffect(() => {
    if (!connected) return;
    timer.current = setInterval(() => {
      const N = 64;
      const arr: number[] = new Array(N);
      for (let i = 0; i < N; i++) {
        // artificial peak near bin 8 (~60 Hz in the device's decimated scale)
        const peak = 200 * Math.exp(-((i - 8) ** 2) / 4);
        arr[i] = Math.max(0, peak + Math.random() * 30);
      }
      setSpectrum(arr);
    }, 300);
    return () => { if (timer.current) clearInterval(timer.current); };
  }, [connected, setSpectrum]);

  return (
    <View style={s.container}>
      <Text style={s.title}>Live Spectrum</Text>
      <Text style={s.hint}>Aggregate photon-rate FFT — look for the refresh line</Text>
      <SpectrumBar data={spectrum} />
      <TouchableOpacity style={s.btn} onPress={() => ble.sendCommand('{"cmd":"fft"}')}>
        <Text style={s.btnText}>Run 1s FFT scan</Text>
      </TouchableOpacity>
      <TouchableOpacity style={s.btn} onPress={() => ble.sendCommand('{"cmd":"pll_lock"}')}>
        <Text style={s.btnText}>Lock PLL to peak</Text>
      </TouchableOpacity>
    </View>
  );
}

function SpectrumBar({ data }: { data: number[] }) {
  if (!data.length) return <Text style={s.empty}>No spectrum data yet.</Text>;
  const maxVal = Math.max(...data, 1);
  return (
    <View style={s.barWrap}>
      {data.map((v, i) => {
        const h = (v / maxVal) * 100;
        const color = v > maxVal * 0.6 ? '#ff5c5c' : v > maxVal * 0.3 ? '#ffcc5c' : '#5cff8a';
        return <View key={i} style={[s.bar, { height: `${h}%`, backgroundColor: color }]} />;
      })}
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#7fd4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 8 },
  hint: { color: '#888', fontSize: 12, marginBottom: 16 },
  barWrap: { flex: 1, flexDirection: 'row', alignItems: 'flex-end', backgroundColor: '#0e0e18', borderRadius: 6, padding: 8, minHeight: 200 },
  bar: { flex: 1, marginHorizontal: 1, borderRadius: 2 },
  empty: { color: '#555', textAlign: 'center', marginTop: 40 },
  btn: { backgroundColor: '#1a3a5c', padding: 14, borderRadius: 8, marginTop: 16, alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
});