/**
 * ACOUSTIC-PHANTOM — Spectrogram Canvas Component
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Canvas-based real-time spectrogram renderer. Displays the power
 * spectrum as a color-coded waterfall: time scrolls left-to-right,
 * frequency is vertical (low at bottom, high at top), magnitude is
 * color (blue=low, green=mid, red=high).
 *
 * Uses React Native View-based rendering (no canvas dependency) —
 * each spectrogram frame is rendered as a column of colored cells.
 * This is less performant than a true canvas but avoids native
 * module dependencies. For production, replace with react-native-canvas
 * or a Skia-based renderer.
 */

import React, { useMemo } from 'react';
import { View, StyleSheet } from 'react-native';

const MAX_FRAMES = 100;
const BANDS = 32;

// Map energy [0..1] to a color
function energyToColor(energy) {
  // Normalize and clamp
  const e = Math.min(1, Math.max(0, energy * 3));

  if (e < 0.25) {
    // Black → Blue
    const t = e / 0.25;
    const r = 0;
    const g = 0;
    const b = Math.round(t * 255);
    return `rgb(${r},${g},${b})`;
  } else if (e < 0.5) {
    // Blue → Cyan → Green
    const t = (e - 0.25) / 0.25;
    const r = 0;
    const g = Math.round(t * 255);
    const b = Math.round(255 - t * 255);
    return `rgb(${r},${g},${b})`;
  } else if (e < 0.75) {
    // Green → Yellow
    const t = (e - 0.5) / 0.25;
    const r = Math.round(t * 255);
    const g = 255;
    const b = 0;
    return `rgb(${r},${g},${b})`;
  } else {
    // Yellow → Red → White
    const t = (e - 0.75) / 0.25;
    const r = 255;
    const g = Math.round(255 - t * 200);
    const b = Math.round(t * 200);
    return `rgb(${r},${g},${b})`;
  }
}

export default function SpectrogramCanvas({ data, zoom = 1, style }) {
  // Render the most recent frames
  const frames = data.slice(-MAX_FRAMES);

  // Pre-compute colors for each cell
  const cells = useMemo(() => {
    const result = [];
    for (let f = 0; f < frames.length; f++) {
      const frame = frames[f];
      if (!frame) continue;
      for (let b = 0; b < BANDS; b++) {
        result.push({
          key: `${f}-${b}`,
          color: energyToColor(frame[b] || 0),
          frameIdx: f,
          bandIdx: b,
        });
      }
    }
    return result;
  }, [frames]);

  const colWidth = Math.max(2, (100 / MAX_FRAMES) * zoom);
  const bandHeight = 100 / BANDS;

  return (
    <View style={[styles.container, style]}>
      {/* Render spectrogram as a grid of colored Views */}
      <View style={styles.grid}>
        {cells.map(cell => (
          <View
            key={cell.key}
            style={{
              position: 'absolute',
              left: cell.frameIdx * colWidth,
              top: cell.bandIdx * bandHeight,
              width: colWidth + 0.5,  // slight overlap to avoid gaps
              height: bandHeight + 0.5,
              backgroundColor: cell.color,
            }}
          />
        ))}
      </View>

      {/* Grid lines (frequency markers) */}
      <View style={styles.gridLines}>
        {[0.25, 0.5, 0.75].map(pos => (
          <View
            key={pos}
            style={{
              position: 'absolute',
              left: 0,
              right: 0,
              top: `${pos * 100}%`,
              height: 1,
              backgroundColor: 'rgba(255,255,255,0.1)',
            }}
          />
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#000',
    overflow: 'hidden',
    borderRadius: 4,
  },
  grid: {
    flex: 1,
    position: 'relative',
  },
  gridLines: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
  },
});