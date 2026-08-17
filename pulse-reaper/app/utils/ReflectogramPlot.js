/**
 * ReflectogramPlot.js — simple SVG reflectogram plot
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * A lightweight line plot of the TDR reflectogram samples. Uses
 * react-native-svg to draw the trace. The x-axis is sample index
 * (≈ distance in mm at 3.35 mm/sample), the y-axis is amplitude
 * (signed 16-bit, scaled to fit).
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Polyline, Line, Text as SvgText } from 'react-native-svg';

export function ReflectogramPlot({ data, width = 320, height = 140 }) {
  if (!data || data.length === 0) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyText}>No reflectogram data yet</Text>
      </View>
    );
  }

  // Find the max absolute amplitude for scaling
  let maxAbs = 1;
  for (const s of data) {
    const a = Math.abs(s);
    if (a > maxAbs) maxAbs = a;
  }

  // Build the polyline points
  const n = data.length;
  const stepX = width / Math.max(n - 1, 1);
  const midY = height / 2;
  const points = data.map((s, i) => {
    const x = i * stepX;
    const y = midY - (s / maxAbs) * (midY - 6);
    return `${x.toFixed(1)},${y.toFixed(1)}`;
  }).join(' ');

  return (
    <View style={styles.container}>
      <Svg width={width} height={height}>
        {/* Grid lines */}
        <Line x1="0" y1={midY} x2={width} y2={midY} stroke="#30363d" strokeWidth="0.5" />
        <Line x1="0" y1="0" x2="0" y2={height} stroke="#30363d" strokeWidth="0.5" />
        <Line x1={width - 1} y1="0" x2={width - 1} y2={height} stroke="#30363d" strokeWidth="0.5" />

        {/* Reflectogram trace */}
        <Polyline points={points} fill="none" stroke="#ff6b35" strokeWidth="1.2" />

        {/* Axis labels */}
        <SvgText x="4" y="12" fill="#6e7681" fontSize="9">amplitude</SvgText>
        <SvgText x={width - 60} y={height - 4} fill="#6e7681" fontSize="9">
          {((n * 3.35) / 1000).toFixed(1)} m
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: 'center', justifyContent: 'center' },
  empty: { height: 140, alignItems: 'center', justifyContent: 'center' },
  emptyText: { color: '#6e7681', fontSize: 13 },
});