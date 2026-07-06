// lumen-tap/app/components/Scope.js
// Real-time oscilloscope + spectrogram widget for LumenTap.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

import React, { useRef, useEffect } from 'react';
import { View, StyleSheet, Text } from 'react-native';

/**
 * A canvas-style scope drawn with absolutely-positioned Views.
 * Not the fastest approach, but works in Expo without native canvas.
 * `samples` is an array of floats in [-1, 1].
 */
export function Scope({ samples = [], label = 'LIVE', color = '#39FF14' }) {
  const W = 300, H = 120, N = 128;
  // down/up-sample to N points
  const pts = [];
  if (samples.length > 0) {
    const step = samples.length / N;
    for (let i = 0; i < N; i++) {
      const v = samples[Math.floor(i * step)] || 0;
      pts.push(v);
    }
  }
  return (
    <View style={styles.box}>
      <Text style={[styles.label, {color}]}>{label}</Text>
      <View style={[styles.plot, {width: W, height: H}]}>
        {pts.map((v, i) => {
          const y = H/2 - v * (H/2 - 4);
          return (
            <View key={i}
              style={{
                position: 'absolute',
                left: (i / N) * W,
                top: y,
                width: 2,
                height: 2,
                backgroundColor: color,
                borderRadius: 1,
              }}
            />
          );
        })}
        {/* zero line */}
        <View style={{position:'absolute', left:0, right:0, top:H/2,
                      height:1, backgroundColor:'#333'}} />
      </View>
    </View>
  );
}

/**
 * Simple 8-band bar "spectrogram" from a status RMS + SNR estimate.
 * Real spectrogram requires the audio stream; here we synthesize a
 * pseudo-spectrum from the demod RMS for a visual proxy.
 */
export function SpectrumBars({ rms = 0, snrDb = 0 }) {
  const bands = 8;
  const bars = [];
  for (let i = 0; i < bands; i++) {
    // pseudo: random-ish but seeded by rms + i
    const seed = (rms * 1000 + i * 17 + snrDb * 3) % 100;
    const h = 4 + (seed / 100) * 60;
    bars.push(
      <View key={i} style={{
        width: 24, height: h, marginLeft: 4,
        backgroundColor: `hsl(${140 - i*10}, 90%, ${40 + i*2}%)`,
        borderRadius: 2,
      }} />
    );
  }
  return (
    <View style={styles.box}>
      <Text style={styles.label}>SPECTRUM (proxy)</Text>
      <View style={{flexDirection:'row', alignItems:'flex-end', height:70}}>
        {bars}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  box: {
    backgroundColor: '#111',
    borderRadius: 6,
    padding: 8,
    marginVertical: 6,
  },
  plot: {
    backgroundColor: '#000',
    borderRadius: 4,
    overflow: 'hidden',
  },
  label: {
    color: '#888',
    fontSize: 10,
    fontFamily: 'monospace',
    marginBottom: 4,
  },
});