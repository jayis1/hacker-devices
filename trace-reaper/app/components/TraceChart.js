/**
 * TRACE-REAPER — Power-trace chart component
 * A lightweight canvas-free SVG-ish trace renderer using flex divs.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';

/**
 * Renders an int16 sample array as a vertical bar sparkline.
 * Each bar's height is proportional to |sample|/max.
 */
export default function TraceChart({ samples, color = '#00e5ff', height = 80 }) {
  if (!samples || samples.length === 0) {
    return <View style={[styles.box, { height }]} />;
  }
  const n = samples.length;
  const max = 1 << 11; /* 12-bit ADC full scale / 2 */
  const bars = [];
  for (let i = 0; i < n; i++) {
    const v = Math.abs(samples[i]);
    const h = Math.min(1, v / max);
    bars.push(
      <View key={i}
        style={{
          flex: 1,
          backgroundColor: color,
          height: `${h * 100}%`,
          marginRight: 0.5,
          alignSelf: 'center',
          opacity: 0.85,
        }}
      />
    );
  }
  return <View style={[styles.box, { height }]}>{bars}</View>;
}

const styles = StyleSheet.create({
  box: {
    flexDirection: 'row',
    alignItems: 'stretch',
    backgroundColor: '#0b1622',
    borderRadius: 4,
    padding: 2,
    width: '100%',
  },
});