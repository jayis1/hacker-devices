/**
 * TeslaPhantom — Scan Map Screen
 * Displays 2D fault sensitivity heat map from cartography scan.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useRef } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Dimensions } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const { width } = Dimensions.get('window');
const CELL_SIZE = Math.min(20, (width - 32) / 30);

const faultColors = {
  0: '#2c3e50',  // NONE — dark blue
  1: '#f39c12',  // DATA_CORRUPT — orange
  2: '#e74c3c',  // SKIP — red
  3: '#c0392b',  // CRASH — dark red
  4: '#8e44ad',  // UNKNOWN — purple
};

const faultLabels = {
  0: 'No Fault',
  1: 'Data Corrupt',
  2: 'Skip (Branch)',
  3: 'Crash/Reset',
  4: 'Unknown',
};

export default function ScanMapScreen() {
  const { scanMap, sendCommand } = useDevice();
  const [selectedCell, setSelectedCell] = useState(null);
  const scrollRef = useRef(null);

  // Parse scan map binary data
  const parseMap = (data) => {
    if (!data || data.length < 8) return null;
    const width_mm = data.charCodeAt(0) | (data.charCodeAt(1) << 8);
    const height_mm = data.charCodeAt(2) | (data.charCodeAt(3) << 8);
    const step_mm = data.charCodeAt(4) | (data.charCodeAt(5) << 8);
    const count = data.charCodeAt(6) | (data.charCodeAt(7) << 8);

    const points = [];
    let offset = 8;
    for (let i = 0; i < count && offset + 7 <= data.length; i++) {
      const x = data.charCodeAt(offset) | (data.charCodeAt(offset + 1) << 8);
      const y = data.charCodeAt(offset + 2) | (data.charCodeAt(offset + 3) << 8);
      const fault = data.charCodeAt(offset + 4);
      const hv = data.charCodeAt(offset + 5) | (data.charCodeAt(offset + 6) << 8);
      points.push({ x: x / 100, y: y / 100, fault, hv });
      offset += 7;
    }

    return { width_mm, height_mm, step_mm: step_mm / 100, points };
  };

  const map = parseMap(scanMap);

  // Build grid for display
  const buildGrid = (mapData) => {
    if (!mapData || mapData.points.length === 0) return [];
    const cols = Math.floor(mapData.width_mm / mapData.step_mm) + 1;
    const rows = Math.floor(mapData.height_mm / mapData.step_mm) + 1;
    const grid = Array(rows).fill(null).map(() => Array(cols).fill(0));

    for (const p of mapData.points) {
      const col = Math.round(p.x / mapData.step_mm);
      const row = Math.round(p.y / mapData.step_mm);
      if (row < rows && col < cols) {
        grid[row][col] = Math.max(grid[row][col], p.fault);
      }
    }
    return grid;
  };

  const grid = map ? buildGrid(map) : [];

  // Statistics
  const stats = map ? map.points.reduce((acc, p) => {
    acc.total++;
    acc[p.fault] = (acc[p.fault] || 0) + 1;
    return acc;
  }, { total: 0 }) : { total: 0 };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Fault Sensitivity Map</Text>

      {!map ? (
        <View style={styles.emptyBox}>
          <Text style={styles.emptyText}>No scan data available</Text>
          <Text style={styles.emptyHint}>Run a scan from the Scan Setup tab</Text>
        </View>
      ) : (
        <>
          <View style={styles.mapInfo}>
            <Text style={styles.mapInfoText}>
              Area: {map.width_mm/100}×{map.height_mm/100} mm, Step: {map.step_mm} mm
            </Text>
            <Text style={styles.mapInfoText}>Points: {map.points.length}</Text>
          </View>

          <ScrollView horizontal={true} ref={scrollRef}>
            <View style={styles.gridContainer}>
              {grid.map((row, rowIdx) => (
                <View key={rowIdx} style={styles.gridRow}>
                  {row.map((fault, colIdx) => (
                    <TouchableOpacity
                      key={colIdx}
                      style={[styles.cell, { backgroundColor: faultColors[fault] || '#333' }]}
                      onPress={() => setSelectedCell({
                        x: colIdx * map.step_mm,
                        y: rowIdx * map.step_mm,
                        fault: fault
                      })}
                    />
                  ))}
                </View>
              ))}
            </View>
          </ScrollView>

          {selectedCell && (
            <View style={styles.detailBox}>
              <Text style={styles.detailTitle}>Selected Point</Text>
              <Text style={styles.detailText}>X: {selectedCell.x.toFixed(2)} mm</Text>
              <Text style={styles.detailText}>Y: {selectedCell.y.toFixed(2)} mm</Text>
              <Text style={styles.detailText}>Fault: {faultLabels[selectedCell.fault]}</Text>
            </View>
          )}

          <View style={styles.legendBox}>
            <Text style={styles.legendTitle}>Fault Classification:</Text>
            {Object.entries(faultLabels).map(([key, label]) => (
              <View key={key} style={styles.legendRow}>
                <View style={[styles.legendSwatch, { backgroundColor: faultColors[key] }]} />
                <Text style={styles.legendText}>{label}: {stats[key] || 0}</Text>
              </View>
            ))}
          </View>

          <View style={styles.statsBox}>
            <Text style={styles.statsTitle}>Summary</Text>
            <Text style={styles.statsText}>Total points: {stats.total}</Text>
            <Text style={styles.statsText}>
              Fault rate: {(((stats.total - (stats[0] || 0)) / Math.max(1, stats.total)) * 100).toFixed(1)}%
            </Text>
          </View>
        </>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', color: '#e74c3c', marginTop: 16, marginBottom: 12 },
  emptyBox: { alignItems: 'center', paddingVertical: 60 },
  emptyText: { color: '#95a5a6', fontSize: 16 },
  emptyHint: { color: '#555', fontSize: 12, marginTop: 8 },
  mapInfo: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  mapInfoText: { color: '#95a5a6', fontSize: 12 },
  gridContainer: { backgroundColor: '#111', borderRadius: 4, padding: 4 },
  gridRow: { flexDirection: 'row' },
  cell: { width: CELL_SIZE, height: CELL_SIZE, margin: 1, borderRadius: 2 },
  detailBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginVertical: 12 },
  detailTitle: { color: '#3498db', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  detailText: { color: '#bbb', fontSize: 13, paddingVertical: 2 },
  legendBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginVertical: 12 },
  legendTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  legendRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 4 },
  legendSwatch: { width: 16, height: 16, borderRadius: 3, marginRight: 8 },
  legendText: { color: '#bbb', fontSize: 12 },
  statsBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginBottom: 30 },
  statsTitle: { color: '#27ae60', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  statsText: { color: '#bbb', fontSize: 13, paddingVertical: 2 },
});