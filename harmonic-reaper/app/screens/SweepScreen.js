/**
 * SweepScreen.js — polar map of hits by IMU heading + power
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Renders a polar/radial plot of detected hits. Each hit is placed at
 * (yaw_deg, power) on a 360° circle centered on the operator. Hit color
 * encodes classification (red = semiconductor, gray = metal).
 */

import React, { useRef, useEffect } from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import Svg, { Circle, Line, Text as SvgText, Path } from 'react-native-svg';
import { CLASSIFY } from '../utils/protocol';

const SCREEN_W = Dimensions.get('window').width;
const PLOT_SIZE = Math.min(SCREEN_W - 32, 320);
const CENTER = PLOT_SIZE / 2;
const RADIUS = PLOT_SIZE / 2 - 20;

export default function SweepScreen({ hits }) {
  const recentHits = hits.slice(-50);  // show last 50

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Sweep Map</Text>
      <Text style={styles.subtitle}>Polar plot — operator at center</Text>

      <Svg width={PLOT_SIZE} height={PLOT_SIZE} style={styles.plot}>
        {/* Concentric range rings */}
        <Circle cx={CENTER} cy={CENTER} r={RADIUS} fill="none" stroke="#222" strokeWidth="1" />
        <Circle cx={CENTER} cy={CENTER} r={RADIUS * 0.66} fill="none" stroke="#1a1a1a" strokeWidth="1" />
        <Circle cx={CENTER} cy={CENTER} r={RADIUS * 0.33} fill="none" stroke="#1a1a1a" strokeWidth="1" />

        {/* Cross hairs */}
        <Line x1={CENTER - RADIUS} y1={CENTER} x2={CENTER + RADIUS} y2={CENTER} stroke="#1a1a1a" strokeWidth="1" />
        <Line x1={CENTER} y1={CENTER - RADIUS} x2={CENTER} y2={CENTER + RADIUS} stroke="#1a1a1a" strokeWidth="1" />

        {/* Cardinal labels */}
        <SvgText x={CENTER} y={8} fill="#555" fontSize="10" textAnchor="middle">0°</SvgText>
        <SvgText x={PLOT_SIZE - 12} y={CENTER + 4} fill="#555" fontSize="10" textAnchor="middle">90°</SvgText>
        <SvgText x={CENTER} y={PLOT_SIZE - 4} fill="#555" fontSize="10" textAnchor="middle">180°</SvgText>
        <SvgText x={12} y={CENTER + 4} fill="#555" fontSize="10" textAnchor="middle">270°</SvgText>

        {/* Center point */}
        <Circle cx={CENTER} cy={CENTER} r="3" fill="#00ff88" />

        {/* Plot hits */}
        {recentHits.map((hit, i) => {
          const angleRad = (hit.yaw_deg * Math.PI) / 180;
          // Map P2 power (-60..0 dBFS) to radial distance (0.2..1.0 × RADIUS)
          const norm = Math.max(0, Math.min(1, (hit.p2_dbfs + 60) / 60));
          const r = RADIUS * (0.2 + 0.8 * norm);
          const x = CENTER + r * Math.sin(angleRad);
          const y = CENTER - r * Math.cos(angleRad);
          const color = hit.classify === CLASSIFY.SEMI ? '#ff3333' :
                        hit.classify === CLASSIFY.METAL ? '#888888' : '#ffaa00';
          return (
            <Circle
              key={i}
              cx={x}
              cy={y}
              r="5"
              fill={color}
              opacity="0.8"
            />
          );
        })}
      </Svg>

      <View style={styles.legend}>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#ff3333' }]} />
          <Text style={styles.legendText}>Semiconductor</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#888888' }]} />
          <Text style={styles.legendText}>Dissimilar Metal</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#ffaa00' }]} />
          <Text style={styles.legendText}>Ambiguous</Text>
        </View>
      </View>

      <Text style={styles.count}>Total hits: {hits.length}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
    alignItems: 'center',
    padding: 16,
  },
  title: {
    color: '#00ff88',
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 4,
  },
  subtitle: {
    color: '#555',
    fontSize: 12,
    marginBottom: 12,
  },
  plot: {
    marginBottom: 12,
  },
  legend: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    width: '100%',
    marginBottom: 8,
  },
  legendItem: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  legendDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 4,
  },
  legendText: {
    color: '#888',
    fontSize: 10,
  },
  count: {
    color: '#666',
    fontSize: 14,
  },
});