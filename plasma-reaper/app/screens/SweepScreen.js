/**
 * SweepScreen.js — 2-D parameter sweep configurator + live heat-map
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: MIT
 *
 * Configure a 2-D parameter sweep (X × Y), launch it, and view
 * a live heat-map of results as the sweep progresses.
 */

import React, { useState, useContext, useRef, useEffect } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet,
  ScrollView, ActivityIndicator, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { DeviceContext } from '../utils/deviceContext';
import { CMD_START_SWEEP, CMD_STOP_SWEEP, RESP_SWEEP_PROGRESS, RESP_SWEEP_COMPLETE } from '../utils/protocol';

const SWEEP_PARAMS = [
  { value: 'offset', label: 'Trigger Offset (ns)', code: 0 },
  { value: 'width', label: 'Glitch Width (ns)', code: 1 },
  { value: 'depth', label: 'Power Depth (mV)', code: 2 },
  { value: 'emv', label: 'EM Voltage (mV)', code: 3 },
  { value: 'emw', label: 'EM Width (ns)', code: 4 },
  { value: 'clk', label: 'Clock Cycle Offset', code: 5 },
  { value: 'sr', label: 'Series R Index', code: 6 },
];

export default function SweepScreen() {
  const { connected, sendCommand, status } = useContext(DeviceContext);

  const [xParam, setXParam] = useState(0); // offset
  const [yParam, setYParam] = useState(1); // width
  const [xStart, setXStart] = useState('500');
  const [xEnd, setXEnd] = useState('5000');
  const [xStep, setXStep] = useState('100');
  const [yStart, setYStart] = useState('50');
  const [yEnd, setYEnd] = useState('500');
  const [yStep, setYStep] = useState('10');
  const [shotsPerCell, setShotsPerCell] = useState('1');
  const [adaptive, setAdaptive] = useState(false);
  const [heatmap, setHeatmap] = useState([]);
  const [running, setRunning] = useState(false);

  // Calculate grid dimensions
  const xCount = Math.floor((parseInt(xEnd) - parseInt(xStart)) / parseInt(xStep)) + 1;
  const yCount = Math.floor((parseInt(yEnd) - parseInt(yStart)) / parseInt(yStep)) + 1;

  const handleStart = async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to PlasmaReaper first.');
      return;
    }

    // Initialize heatmap
    const newMap = [];
    for (let y = 0; y < yCount; y++) {
      newMap.push(new Array(xCount).fill(0)); // 0 = no result
    }
    setHeatmap(newMap);
    setRunning(true);

    // Build sweep config payload
    const config = {
      xParam: xParam,
      xStart: parseInt(xStart),
      xEnd: parseInt(xEnd),
      xStep: parseInt(xStep),
      yParam: yParam,
      yStart: parseInt(yStart),
      yEnd: parseInt(yEnd),
      yStep: parseInt(yStep),
      zParam: 7, // unused
      zStart: 0, zEnd: 0, zStep: 0,
      shotsPerCell: parseInt(shotsPerCell),
      adaptive: adaptive,
      baseParams: {
        vectorMask: 0x01, // power glitch by default
        triggerOffsetNs: 1000,
        glitchWidthNs: 200,
        powerDepthMv: 900,
        powerSeriesR: 3,
      },
    };

    // Serialize and send
    const payload = new Uint8Array(64);
    payload[0] = config.xParam;
    writeU32(payload, 1, config.xStart);
    writeU32(payload, 5, config.xEnd);
    writeU32(payload, 9, config.xStep);
    payload[13] = config.yParam;
    writeU32(payload, 14, config.yStart);
    writeU32(payload, 18, config.yEnd);
    writeU32(payload, 22, config.yStep);
    payload[26] = config.shotsPerCell;
    payload[27] = config.adaptive ? 1 : 0;
    // Base params start at offset 28
    payload[28] = config.baseParams.vectorMask;
    writeU32(payload, 29, config.baseParams.triggerOffsetNs);
    writeU32(payload, 33, config.baseParams.glitchWidthNs);
    writeU16(payload, 37, config.baseParams.powerDepthMv);
    payload[39] = config.baseParams.powerSeriesR;

    await sendCommand(CMD_START_SWEEP, payload);
  };

  const handleStop = async () => {
    await sendCommand(CMD_STOP_SWEEP, null);
    setRunning(false);
  };

  // Update heatmap from sweep progress
  useEffect(() => {
    if (!status || !status.sweepRunning) {
      if (running && status && !status.sweepRunning) {
        setRunning(false);
      }
      return;
    }
    // The status polling in App.js updates status.sweepProgress
    // Here we would process individual sweep progress responses
  }, [status, running]);

  // Get color for a heatmap cell
  const getCellColor = (value) => {
    if (value === 0) return '#eee';      // no result
    if (value === 1) return '#4CAF50';   // success
    if (value === 2) return '#F44336';   // failure
    if (value === 3) return '#FF9800';   // no response
    if (value === 4) return '#9C27B0';   // invalid
    return '#ccc';
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Sweep Configuration</Text>

        <Text style={styles.subLabel}>X-Axis Parameter</Text>
        <View style={styles.paramSelector}>
          {SWEEP_PARAMS.map(p => (
            <TouchableOpacity
              key={p.code}
              style={[styles.paramChip, xParam === p.code ? styles.paramChipActive : null]}
              onPress={() => setXParam(p.code)}
            >
              <Text style={[styles.paramChipText, xParam === p.code ? styles.paramChipTextActive : null]}>
                {p.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <View style={styles.rangeRow}>
          <RangeInput label="Start" value={xStart} onChange={setXStart} />
          <RangeInput label="End" value={xEnd} onChange={setXEnd} />
          <RangeInput label="Step" value={xStep} onChange={setXStep} />
        </View>

        <Text style={styles.subLabel}>Y-Axis Parameter</Text>
        <View style={styles.paramSelector}>
          {SWEEP_PARAMS.map(p => (
            <TouchableOpacity
              key={p.code}
              style={[styles.paramChip, yParam === p.code ? styles.paramChipActive : null]}
              onPress={() => setYParam(p.code)}
            >
              <Text style={[styles.paramChipText, yParam === p.code ? styles.paramChipTextActive : null]}>
                {p.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <View style={styles.rangeRow}>
          <RangeInput label="Start" value={yStart} onChange={setYStart} />
          <RangeInput label="End" value={yEnd} onChange={setYEnd} />
          <RangeInput label="Step" value={yStep} onChange={setYStep} />
        </View>

        <View style={styles.rangeRow}>
          <RangeInput label="Shots/Cell" value={shotsPerCell} onChange={setShotsPerCell} />
        </View>

        <View style={styles.adaptiveRow}>
          <Text style={styles.adaptiveLabel}>Adaptive (hill-climbing)</Text>
          <TouchableOpacity
            style={[styles.toggle, adaptive ? styles.toggleOn : null]}
            onPress={() => setAdaptive(!adaptive)}
          >
            <View style={[styles.toggleKnob, adaptive ? styles.toggleKnobOn : null]} />
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>
          Grid: {xCount} × {yCount} = {xCount * yCount} cells
        </Text>

        {running ? (
          <TouchableOpacity style={styles.stopBtn} onPress={handleStop}>
            <Icon name="stop" size={20} color="#fff" />
            <Text style={styles.stopBtnText}>STOP SWEEP</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.startBtn} onPress={handleStart} disabled={!connected}>
            <Icon name="play" size={20} color="#fff" />
            <Text style={styles.startBtnText}>START SWEEP</Text>
          </TouchableOpacity>
        )}

        {status && status.sweepRunning && (
          <View style={styles.progressContainer}>
            <Text style={styles.progressText}>
              Progress: {status.completedCells || 0} / {status.totalCells || 0} cells
            </Text>
            <Text style={styles.progressText}>
              Successes: {status.successCount || 0}
            </Text>
            <View style={styles.progressBar}>
              <View style={[styles.progressBarFill, {
                width: `${status.totalCells ? ((status.completedCells / status.totalCells) * 100) : 0}%`
              }]} />
            </View>
          </View>
        )}
      </View>

      {heatmap.length > 0 && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Heat-Map</Text>
          <ScrollView horizontal>
            <View>
              {/* Y-axis label */}
              <View style={styles.heatmapRow}>
                <Text style={styles.axisLabel}>{SWEEP_PARAMS[yParam].label} →</Text>
                {Array.from({ length: xCount }, (_, i) => (
                  <Text key={i} style={styles.columnHeader}>
                    {parseInt(xStart) + i * parseInt(xStep)}
                  </Text>
                ))}
              </View>
              {/* Heatmap rows */}
              {heatmap.map((row, y) => (
                <View key={y} style={styles.heatmapRow}>
                  <Text style={styles.rowLabel}>
                    {parseInt(yStart) + y * parseInt(yStep)}
                  </Text>
                  {row.map((cell, x) => (
                    <View key={x} style={[styles.heatmapCell, { backgroundColor: getCellColor(cell) }]} />
                  ))}
                </View>
              ))}
            </View>
          </ScrollView>

          {/* Legend */}
          <View style={styles.legendRow}>
            <LegendItem color="#eee" label="Pending" />
            <LegendItem color="#4CAF50" label="Success" />
            <LegendItem color="#F44336" label="Failure" />
            <LegendItem color="#FF9800" label="No Response" />
            <LegendItem color="#9C27B0" label="Invalid" />
          </View>
        </View>
      )}

      <Text style={styles.footer}>Author: jayis1 · MIT License</Text>
    </ScrollView>
  );
}

function RangeInput({ label, value, onChange }) {
  return (
    <View style={styles.rangeInput}>
      <Text style={styles.rangeInputLabel}>{label}</Text>
      <TextInput
        style={styles.rangeTextInput}
        value={value}
        onChangeText={onChange}
        keyboardType="numeric"
      />
    </View>
  );
}

function LegendItem({ color, label }) {
  return (
    <View style={styles.legendItem}>
      <View style={[styles.legendColor, { backgroundColor: color }]} />
      <Text style={styles.legendText}>{label}</Text>
    </View>
  );
}

function writeU32(arr, off, val) {
  arr[off] = val & 0xFF;
  arr[off+1] = (val >> 8) & 0xFF;
  arr[off+2] = (val >> 16) & 0xFF;
  arr[off+3] = (val >> 24) & 0xFF;
}
function writeU16(arr, off, val) {
  arr[off] = val & 0xFF;
  arr[off+1] = (val >> 8) & 0xFF;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  section: { backgroundColor: '#fff', margin: 8, borderRadius: 8, padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  subLabel: { fontSize: 14, fontWeight: '600', color: '#555', marginTop: 8, marginBottom: 4 },
  paramSelector: { flexDirection: 'row', flexWrap: 'wrap', marginBottom: 8 },
  paramChip: { paddingHorizontal: 12, paddingVertical: 6, borderWidth: 1, borderColor: '#ddd', borderRadius: 4, margin: 2 },
  paramChipActive: { backgroundColor: '#e91e63', borderColor: '#e91e63' },
  paramChipText: { fontSize: 12, color: '#333' },
  paramChipTextActive: { color: '#fff' },
  rangeRow: { flexDirection: 'row', marginBottom: 8 },
  rangeInput: { flex: 1, marginHorizontal: 4 },
  rangeInputLabel: { fontSize: 10, color: '#888' },
  rangeTextInput: { borderWidth: 1, borderColor: '#ddd', borderRadius: 4, padding: 6, fontSize: 12 },
  adaptiveRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginTop: 8 },
  adaptiveLabel: { fontSize: 14 },
  toggle: { width: 44, height: 24, borderRadius: 12, backgroundColor: '#ccc', padding: 2 },
  toggleOn: { backgroundColor: '#4CAF50' },
  toggleKnob: { width: 20, height: 20, borderRadius: 10, backgroundColor: '#fff' },
  toggleKnobOn: { transform: [{ translateX: 20 }] },
  startBtn: { backgroundColor: '#4CAF50', padding: 16, borderRadius: 8, flexDirection: 'row', justifyContent: 'center', alignItems: 'center' },
  startBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 16, marginLeft: 8 },
  stopBtn: { backgroundColor: '#f44336', padding: 16, borderRadius: 8, flexDirection: 'row', justifyContent: 'center', alignItems: 'center' },
  stopBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 16, marginLeft: 8 },
  progressContainer: { marginTop: 12 },
  progressText: { fontSize: 14, color: '#333' },
  progressBar: { height: 8, backgroundColor: '#e0e0e0', borderRadius: 4, marginTop: 4 },
  progressBarFill: { height: 8, backgroundColor: '#e91e63', borderRadius: 4 },
  heatmapRow: { flexDirection: 'row', alignItems: 'center' },
  axisLabel: { fontSize: 10, color: '#888', width: 60 },
  columnHeader: { fontSize: 8, color: '#666', width: 16, textAlign: 'center' },
  rowLabel: { fontSize: 8, color: '#666', width: 40, textAlign: 'right', paddingRight: 4 },
  heatmapCell: { width: 16, height: 16, margin: 0.5, borderRadius: 2 },
  legendRow: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 12 },
  legendItem: { flexDirection: 'row', alignItems: 'center', marginRight: 12, marginBottom: 4 },
  legendColor: { width: 12, height: 12, borderRadius: 2, marginRight: 4 },
  legendText: { fontSize: 10, color: '#666' },
  footer: { textAlign: 'center', color: '#999', fontSize: 12, padding: 16 },
});