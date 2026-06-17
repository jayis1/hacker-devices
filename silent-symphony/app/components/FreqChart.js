/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Frequency Chart Component — SVG-based spectrum display
 *
 * Renders a real-time spectrum analyzer visualization using SVG.
 * Shows frequency (x-axis) vs. energy (y-axis) for ultrasonic range.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Line, Rect, Text as SvgText, G } from 'react-native-svg';

/**
 * Frequency spectrum chart component.
 *
 * @param {number[]} data     - Array of 256 frequency bin energy values (0-1)
 * @param {number}   width    - Chart width in pixels
 * @param {number}   height   - Chart height in pixels
 * @param {number}   minFreq  - Minimum displayed frequency (Hz)
 * @param {number}   maxFreq  - Maximum displayed frequency (Hz)
 */
export default function FreqChart({ data, width, height, minFreq, maxFreq }) {
  if (!data || data.length === 0) {
    return (
      <View style={[styles.placeholder, { width, height }]}>
        <Text style={styles.placeholderText}>No data</Text>
      </View>
    );
  }

  const barWidth = width / data.length;
  const padding = { top: 10, bottom: 20, left: 35, right: 5 };
  const chartW = width - padding.left - padding.right;
  const chartH = height - padding.top - padding.bottom;

  // Frequency labels
  const freqLabels = [];
  const labelStep = Math.round((maxFreq - minFreq) / 4 / 1000) * 1000;
  for (let f = minFreq; f <= maxFreq; f += labelStep) {
    const x =
      padding.left + ((f - minFreq) / (maxFreq - minFreq)) * chartW;
    freqLabels.push({ x, label: `${(f / 1000).toFixed(0)}k` });
  }

  // Energy labels (y-axis)
  const energyLabels = [
    { y: padding.top, label: '-0dB' },
    { y: padding.top + chartH * 0.5, label: '-6dB' },
    { y: padding.top + chartH, label: '-12dB' },
  ];

  return (
    <Svg width={width} height={height}>
      {/* Background */}
      <Rect
        x={0}
        y={0}
        width={width}
        height={height}
        fill="#0a0a1a"
        rx={4}
      />

      {/* Grid lines */}
      {freqLabels.map((f, i) => (
        <Line
          key={`grid-${i}`}
          x1={f.x}
          y1={padding.top}
          x2={f.x}
          y2={padding.top + chartH}
          stroke="#1a1a2e"
          strokeWidth={0.5}
        />
      ))}

      {/* Frequency bars */}
      {data.map((val, idx) => {
        const x = padding.left + (idx / data.length) * chartW;
        const barH = Math.max(1, val * chartH);
        const y = padding.top + chartH - barH;

        // Color gradient: green → yellow → red based on amplitude
        let r = Math.floor(val * 255 * 4);
        let g = Math.floor(255 - val * 255 * 4);
        if (r > 255) r = 255;
        if (g < 0) g = 0;
        const color = `rgb(${r}, ${g}, ${Math.floor(255 - val * 255)})`;

        return (
          <Rect
            key={idx}
            x={x}
            y={y}
            width={Math.max(1, barWidth - 0.5)}
            height={barH}
            fill={color}
            opacity={0.8}
          />
        );
      })}

      {/* Frequency axis labels */}
      {freqLabels.map((f, i) => (
        <SvgText
          key={`flabel-${i}`}
          x={f.x}
          y={height - 4}
          fill="#6c757d"
          fontSize={8}
          textAnchor="middle"
        >
          {f.label}
        </SvgText>
      ))}

      {/* Energy axis labels */}
      {energyLabels.map((e, i) => (
        <SvgText
          key={`elabel-${i}`}
          x={8}
          y={e.y + 3}
          fill="#6c757d"
          fontSize={7}
          textAnchor="start"
        >
          {e.label}
        </SvgText>
      ))}

      {/* Border */}
      <Rect
        x={0}
        y={0}
        width={width}
        height={height}
        fill="none"
        stroke="#16213e"
        strokeWidth={1}
        rx={4}
      />
    </Svg>
  );
}

const styles = StyleSheet.create({
  placeholder: {
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#16213e',
  },
  placeholderText: {
    color: '#6c757d',
    fontSize: 12,
    fontStyle: 'italic',
  },
});