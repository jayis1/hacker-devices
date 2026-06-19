/**
 * FaultHeatmap.tsx — SVG fault-class heatmap for the scan progress view
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Rect, Text as SvgText } from 'react-native-svg';

interface Props {
  grid: Record<string, number>;   // "x,y" → fault class
  size: number;
}

const CLASS_COLORS: Record<number, string> = {
  0: '#1a1a2e',   // no change (dark)
  1: '#4ecca3',   // single bit (green)
  2: '#e94560',   // single byte (red — DFA gold)
  3: '#ff9f1c',   // multi byte (orange)
  4: '#666',      // crash (grey)
  5: '#ff0',      // reset (yellow)
  6: '#f0f',      // timeout (magenta)
};

export function FaultHeatmap({ grid, size }: Props) {
  const keys = Object.keys(grid);
  if (keys.length === 0) {
    return (
      <View style={[styles.empty, { width: size, height: size * 0.6 }]}>
        <Text style={styles.emptyText}>No faults yet. Start a scan.</Text>
      </View>
    );
  }

  // Parse the grid to find bounds.
  const pts = keys.map((k) => {
    const [x, y] = k.split(',').map(Number);
    return { x, y, c: grid[k] };
  });
  const xs = pts.map((p) => p.x);
  const ys = pts.map((p) => p.y);
  const xmin = Math.min(...xs), xmax = Math.max(...xs);
  const ymin = Math.min(...ys), ymax = Math.max(...ys);
  const xRange = Math.max(1, xmax - xmin);
  const yRange = Math.max(1, ymax - ymin);
  const cellW = size / (xRange + 1);
  const cellH = (size * 0.6) / (yRange + 1);

  return (
    <View style={styles.wrap}>
      <Svg width={size} height={size * 0.6}>
        {pts.map((p, i) => {
          const px = (p.x - xmin) * cellW;
          const py = (p.y - ymin) * cellH;
          return (
            <Rect
              key={i}
              x={px}
              y={py}
              width={cellW + 1}
              height={cellH + 1}
              fill={CLASS_COLORS[p.c] ?? '#222'}
              stroke="#0f0f1e"
              strokeWidth={0.5}
            />
          );
        })}
        <SvgText x={4} y={14} fill="#666" fontSize={10}>
          {xmin},{ymin} → {xmax},{ymax} µm
        </SvgText>
      </Svg>
      <View style={styles.legendRow}>
        {Object.entries(CLASS_COLORS).map(([c, col]) => (
          <View key={c} style={styles.legendItem}>
            <View style={[styles.swatch, { backgroundColor: col }]} />
            <Text style={styles.legendText}>{c}</Text>
          </View>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { alignItems: 'center' },
  empty: { backgroundColor: '#16213e', borderRadius: 8, justifyContent: 'center', alignItems: 'center' },
  emptyText: { color: '#666', fontSize: 13 },
  legendRow: { flexDirection: 'row', marginTop: 8, flexWrap: 'wrap' },
  legendItem: { flexDirection: 'row', alignItems: 'center', marginRight: 10, marginTop: 4 },
  swatch: { width: 12, height: 12, borderRadius: 2, marginRight: 4 },
  legendText: { color: '#a0a0c0', fontSize: 10 },
});