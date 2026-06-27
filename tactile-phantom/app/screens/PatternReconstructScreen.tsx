/**
 * Tactile-Phantom — Companion App
 * screens/PatternReconstructScreen.tsx — Unlock pattern visualization & replay
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Reconstructs Android unlock patterns from captured touch events.
 * The Android unlock pattern uses a 3x3 grid; this screen visualizes
 * the sequence of grid nodes touched and allows replay via injection.
 */

import React, { useState, useMemo, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Dimensions, Alert,
} from 'react-native';
import { useStore } from '../src/store';
import { bleManager } from '../src/ble';
import { INJ_TYPE } from '../src/store';

const SCREEN_W = Dimensions.get('window').width;
const GRID_SIZE = 280;
const CELL = GRID_SIZE / 3;

export default function PatternReconstructScreen() {
  const events = useStore((s) => s.events);
  const status = useStore((s) => s.status);
  const [patternNodes, setPatternNodes] = useState<number[]>([]);
  const [allPatterns, setAllPatterns] = useState<number[][]>([]);

  // Extract patterns from touch events
  // A pattern is a sequence of touch-down events where coordinates
  // map to grid nodes, terminated by a release or timeout.
  useMemo(() => {
    const patterns: number[][] = [];
    let currentPattern: number[] = [];
    let patternActive = false;
    let lastTime = 0;

    for (const ev of events) {
      if (ev.type === 1 && ev.fingers.length > 0) {  // TOUCH
        const f = ev.fingers[0];
        const node = coordToNode(f.x, f.y, status?.xResolution || 1080,
                                  status?.yResolution || 2400);
        if (node >= 0) {
          if (!patternActive || (ev.timestamp - lastTime) > 500000) {
            // New pattern starts
            if (currentPattern.length > 0) patterns.push(currentPattern);
            currentPattern = [node];
            patternActive = true;
          } else {
            if (!currentPattern.includes(node)) {
              currentPattern.push(node);
            }
          }
          lastTime = ev.timestamp;
        }
      } else if (ev.type === 2) {  // RELEASE
        if (patternActive && currentPattern.length > 0) {
          patterns.push([...currentPattern]);
          currentPattern = [];
          patternActive = false;
        }
      }
    }
    if (currentPattern.length > 0) patterns.push(currentPattern);
    setAllPatterns(patterns);
    if (patterns.length > 0) setPatternNodes(patterns[patterns.length - 1]);
  }, [events, status]);

  // Replay the selected pattern via injection
  const replayPattern = async () => {
    if (patternNodes.length === 0) {
      Alert.alert('No Pattern', 'No pattern to replay');
      return;
    }
    // Convert grid nodes to coordinates
    const points = patternNodes.map((node) => {
      const row = Math.floor(node / 3);
      const col = node % 3;
      const x = Math.round((col + 0.5) * (status?.xResolution || 1080) / 3);
      const y = Math.round((row + 0.5) * (status?.yResolution || 2400) * 0.8 / 3 +
                 (status?.yResolution || 2400) * 0.1);
      return { x, y };
    });

    // Send each point as a tap in sequence
    for (const pt of points) {
      await bleManager.sendInjectCommand({
        type: INJ_TYPE.TAP,
        x1: pt.x, y1: pt.y, x2: 0, y2: 0,
        durationMs: 100, fingerCount: 1,
      });
      await new Promise((r) => setTimeout(r, 150));
    }
    Alert.alert('Replay Sent', `Pattern: ${patternNodes.join('-')}`);
  };

  const selectPattern = (idx: number) => {
    setPatternNodes(allPatterns[idx] || []);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Pattern Reconstruction</Text>
      <Text style={styles.subtitle}>
        Android 3×3 unlock grid — {allPatterns.length} pattern(s) captured
      </Text>

      {/* Pattern grid visualization */}
      <View style={styles.gridContainer}>
        <View style={styles.grid}>
          {/* Grid dots */}
          {[0, 1, 2, 3, 4, 5, 6, 7, 8].map((node) => {
            const row = Math.floor(node / 3);
            const col = node % 3;
            const isActive = patternNodes.includes(node);
            return (
              <View
                key={node}
                style={[
                  styles.gridDot,
                  {
                    left: col * CELL + CELL / 2 - 12,
                    top: row * CELL + CELL / 2 - 12,
                  },
                  isActive && styles.gridDotActive,
                ]}
              >
                {isActive && (
                  <Text style={styles.dotLabel}>
                    {patternNodes.indexOf(node) + 1}
                  </Text>
                )}
              </View>
            );
          })}

          {/* Connection lines */}
          {patternNodes.length > 1 && (
            <SvgLines
              nodes={patternNodes}
              cellSize={CELL}
            />
          )}
        </View>
      </View>

      {/* Pattern sequence text */}
      <View style={styles.sequenceRow}>
        <Text style={styles.sequenceLabel}>Sequence:</Text>
        <Text style={styles.sequenceText}>
          {patternNodes.length > 0
            ? patternNodes.map((n) => n + 1).join(' → ')
            : '(none)'}
        </Text>
      </View>

      {/* Captured patterns list */}
      <View style={styles.patternListContainer}>
        <Text style={styles.patternListTitle}>Captured Patterns:</Text>
        {allPatterns.length === 0 ? (
          <Text style={styles.emptyText}>No patterns captured yet.</Text>
        ) : (
          allPatterns.slice(-10).reverse().map((p, i) => (
            <TouchableOpacity
              key={i}
              style={styles.patternItem}
              onPress={() => selectPattern(allPatterns.length - 1 - i)}
            >
              <Text style={styles.patternItemText}>
                {i === 0 ? '▶ ' : '  '}
                {p.map((n) => n + 1).join('-')}
              </Text>
              <Text style={styles.patternItemLen}>{p.length} nodes</Text>
            </TouchableOpacity>
          ))
        )}
      </View>

      {/* Replay button */}
      <TouchableOpacity style={styles.replayBtn} onPress={replayPattern}>
        <Text style={styles.replayBtnText}>▶ Replay via Injection</Text>
      </TouchableOpacity>
    </View>
  );
}

// Convert coordinates to grid node (0-8)
function coordToNode(x: number, y: number, xRes: number, yRes: number): number {
  const cellW = xRes / 3;
  const cellH = (yRes * 0.8) / 3;
  const offsetY = yRes * 0.1;
  const col = Math.floor(x / cellW);
  const row = Math.floor((y - offsetY) / cellH);
  if (col < 0 || col > 2 || row < 0 || row > 2) return -1;
  // Check proximity to center
  const cx = col * cellW + cellW / 2;
  const cy = row * cellH + cellH / 2 + offsetY;
  const dx = Math.abs(x - cx);
  const dy = Math.abs(y - cy);
  if (dx > cellW * 0.4 || dy > cellH * 0.4) return -1;
  return row * 3 + col;
}

// Inline SVG-like line rendering using View borders
function SvgLines({ nodes, cellSize }: { nodes: number[]; cellSize: number }) {
  // Render lines between consecutive nodes
  // Each line is a thin rotated View
  const lines: any[] = [];
  for (let i = 0; i < nodes.length - 1; i++) {
    const r1 = Math.floor(nodes[i] / 3);
    const c1 = nodes[i] % 3;
    const r2 = Math.floor(nodes[i + 1] / 3);
    const c2 = nodes[i + 1] % 3;
    const x1 = c1 * cellSize + cellSize / 2;
    const y1 = r1 * cellSize + cellSize / 2;
    const x2 = c2 * cellSize + cellSize / 2;
    const y2 = r2 * cellSize + cellSize / 2;
    const dx = x2 - x1;
    const dy = y2 - y1;
    const len = Math.sqrt(dx * dx + dy * dy);
    const angle = Math.atan2(dy, dx) * (180 / Math.PI);
    lines.push(
      <View
        key={i}
        style={{
          position: 'absolute',
          left: x1,
          top: y1,
          width: len,
          height: 2,
          backgroundColor: '#00ff88',
          transform: [{ rotate: `${angle}deg` }],
          transformOrigin: '0 0',
        }}
      />
    );
  }
  return <>{lines}</>;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 15 },
  title: { color: '#e0e0e0', fontSize: 22, fontWeight: 'bold', textAlign: 'center' },
  subtitle: { color: '#888', fontSize: 12, textAlign: 'center', marginBottom: 15 },
  gridContainer: { alignItems: 'center', marginBottom: 15 },
  grid: {
    width: GRID_SIZE, height: GRID_SIZE, position: 'relative',
    backgroundColor: '#0a0a14', borderRadius: 12, borderWidth: 1, borderColor: '#1a1a2e',
  },
  gridDot: {
    position: 'absolute', width: 24, height: 24, borderRadius: 12,
    backgroundColor: '#1a1a2e', borderWidth: 2, borderColor: '#333',
    alignItems: 'center', justifyContent: 'center',
  },
  gridDotActive: {
    backgroundColor: '#00ff88', borderColor: '#00ff88',
    shadowColor: '#00ff88', shadowOpacity: 0.5, shadowRadius: 6,
  },
  dotLabel: { color: '#000', fontSize: 12, fontWeight: 'bold' },
  sequenceRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 15, gap: 10 },
  sequenceLabel: { color: '#888', fontSize: 14 },
  sequenceText: { color: '#00ff88', fontSize: 18, fontWeight: 'bold' },
  patternListContainer: { flex: 1 },
  patternListTitle: { color: '#888', fontSize: 14, marginBottom: 8 },
  patternItem: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
    backgroundColor: '#16213e', borderRadius: 8, padding: 10, marginBottom: 5,
    borderWidth: 1, borderColor: '#0f3460',
  },
  patternItemText: { color: '#e0e0e0', fontSize: 14 },
  patternItemLen: { color: '#666', fontSize: 11 },
  emptyText: { color: '#555', textAlign: 'center', marginTop: 20 },
  replayBtn: {
    backgroundColor: '#0a2a1a', borderRadius: 10, padding: 15,
    alignItems: 'center', borderWidth: 1, borderColor: '#00ff88',
    marginTop: 10,
  },
  replayBtnText: { color: '#00ff88', fontSize: 16, fontWeight: 'bold' },
});