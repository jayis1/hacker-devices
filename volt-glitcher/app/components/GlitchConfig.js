/**
 * GlitchConfig.js — Glitch configuration component
 * Full-featured parameter editor for all glitch modes
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext, useCallback, useEffect } from 'react';
import {
  View, Text, StyleSheet, ScrollView, Switch, TextInput,
  TouchableOpacity, Alert, Platform,
} from 'react-native';
import { DeviceContext } from '../App';
import {
  GLITCH_MODES, TRIGGER_MODES, GLITCH_SHAPES, getDefaultConfig,
} from '../utils/protocol';

/* ============================================================================
 * Mode Selector
 * ============================================================================ */

function ModeSelector({ value, onChange }) {
  const modes = [
    { key: GLITCH_MODES.VOLTAGE_GLITCH, label: '⚡ Voltage', desc: 'VCC shunt via MOSFET' },
    { key: GLITCH_MODES.EM_GLITCH, label: '🧲 EM Pulse', desc: 'Electromagnetic coil' },
    { key: GLITCH_MODES.CLOCK_GLITCH, label: '⏱ Clock', desc: 'Clock stretch/insert' },
  ];

  return (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>Glitch Mode</Text>
      <View style={styles.modeRow}>
        {modes.map((m) => (
          <TouchableOpacity
            key={m.key}
            style={[styles.modeButton, value === m.key && styles.modeButtonActive]}
            onPress={() => onChange(m.key)}
          >
            <Text style={[styles.modeLabel, value === m.key && styles.modeLabelActive]}>
              {m.label}
            </Text>
            <Text style={styles.modeDesc}>{m.desc}</Text>
          </TouchableOpacity>
        ))}
      </View>
    </View>
  );
}

/* ============================================================================
 * Parameter Slider (integer input with label and range)
 * ============================================================================ */

function ParamInput({ label, value, onChange, unit = '', min = 0, max = 65535, step = 1 }) {
  const [text, setText] = useState(String(value));

  useEffect(() => {
    setText(String(value));
  }, [value]);

  const handleSubmit = useCallback(() => {
    const num = parseInt(text, 10);
    if (!isNaN(num) && num >= min && num <= max) {
      onChange(num);
    } else {
      setText(String(value));
    }
  }, [text, min, max, onChange, value]);

  return (
    <View style={styles.paramRow}>
      <Text style={styles.paramLabel}>{label}</Text>
      <View style={styles.paramInputWrap}>
        <TextInput
          style={styles.paramInput}
          value={text}
          onChangeText={setText}
          onBlur={handleSubmit}
          onSubmitEditing={handleSubmit}
          keyboardType="numeric"
          returnKeyType="done"
        />
        <Text style={styles.paramUnit}>{unit}</Text>
      </View>
      <Text style={styles.paramRange}>{min}–{max}</Text>
    </View>
  );
}

/* ============================================================================
 * Trigger Mode Selector
 * ============================================================================ */

function TriggerSelector({ value, onChange, onEdgeChange, edgeValue }) {
  const triggers = [
    { key: TRIGGER_MODES.NONE, label: 'None' },
    { key: TRIGGER_MODES.GPIO, label: 'GPIO' },
    { key: TRIGGER_MODES.UART_PATTERN, label: 'UART Pattern' },
    { key: TRIGGER_MODES.JTAG_STATE, label: 'JTAG State' },
    { key: TRIGGER_MODES.MANUAL, label: 'Manual' },
    { key: TRIGGER_MODES.FPGA_PATTERN, label: 'FPGA' },
  ];

  const edges = ['Rising', 'Falling', 'Both'];

  return (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>Trigger Source</Text>
      <View style={styles.triggerGrid}>
        {triggers.map((t) => (
          <TouchableOpacity
            key={t.key}
            style={[styles.triggerBtn, value === t.key && styles.triggerBtnActive]}
            onPress={() => onChange(t.key)}
          >
            <Text style={[styles.triggerLabel, value === t.key && styles.triggerLabelActive]}>
              {t.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Edge selection (for GPIO/external triggers) */}
      {(value === TRIGGER_MODES.GPIO || value === TRIGGER_MODES.FPGA_PATTERN) && (
        <View style={styles.edgeRow}>
          <Text style={styles.paramLabel}>Edge:</Text>
          {edges.map((e, i) => (
            <TouchableOpacity
              key={i}
              style={[styles.edgeBtn, edgeValue === i && styles.edgeBtnActive]}
              onPress={() => onEdgeChange(i)}
            >
              <Text style={styles.edgeText}>{e}</Text>
            </TouchableOpacity>
          ))}
        </View>
      )}
    </View>
  );
}

/* ============================================================================
 * UART Pattern Editor
 * ============================================================================ */

function UARTPatternEditor({ pattern, mask, onPatternChange, onMaskChange }) {
  return (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>UART Trigger Pattern</Text>
      <View style={styles.uartRow}>
        {pattern.map((byte, i) => (
          <View key={i} style={styles.uartByteWrap}>
            <Text style={styles.uartLabel}>B{i}</Text>
            <TextInput
              style={styles.uartInput}
              value={byte.toString(16).padStart(2, '0').toUpperCase()}
              onChangeText={(t) => {
                const val = parseInt(t, 16);
                if (!isNaN(val) && val >= 0 && val <= 255) {
                  const newPat = [...pattern];
                  newPat[i] = val;
                  onPatternChange(newPat);
                }
              }}
              maxLength={2}
              keyboardType="numeric"
            />
          </View>
        ))}
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Match Mask</Text>
        <TextInput
          style={styles.paramInput}
          value={'0x' + mask.toString(16).padStart(2, '0').toUpperCase()}
          onChangeText={(t) => {
            const val = parseInt(t.replace('0x', ''), 16);
            if (!isNaN(val)) onMaskChange(val & 0xFF);
          }}
          maxLength={4}
        />
      </View>
    </View>
  );
}

/* ============================================================================
 * Main GlitchConfig Component
 * ============================================================================ */

function GlitchConfig() {
  const { glitchConfig, actions, colors } = useContext(DeviceContext);
  const [config, setConfig] = useState(glitchConfig || getDefaultConfig());
  const [dirty, setDirty] = useState(false);

  useEffect(() => {
    if (glitchConfig) setConfig(glitchConfig);
  }, [glitchConfig]);

  const updateConfig = useCallback((key, value) => {
    setConfig((prev) => ({ ...prev, [key]: value }));
    setDirty(true);
  }, []);

  const applyConfig = useCallback(async () => {
    try {
      await actions.updateConfig(config);
      setDirty(false);
    } catch (e) {
      Alert.alert('Error', 'Failed to apply configuration');
    }
  }, [config, actions]);

  const resetConfig = useCallback(() => {
    setConfig(getDefaultConfig());
    setDirty(true);
  }, []);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
      {/* Mode Selector */}
      <ModeSelector
        value={config.mode}
        onChange={(m) => updateConfig('mode', m)}
      />

      {/* Pulse Shape */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Pulse Shape</Text>
        <View style={styles.shapeRow}>
          {['Rectangle', 'Triangle', 'Gaussian', 'Sawtooth', 'Custom'].map((s, i) => (
            <TouchableOpacity
              key={i}
              style={[styles.shapeBtn, config.shape === i && styles.shapeBtnActive]}
              onPress={() => updateConfig('shape', i)}
            >
              <Text style={[styles.shapeText, config.shape === i && styles.shapeTextActive]}>
                {s}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Timing Parameters */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Timing</Text>
        <ParamInput
          label="Delay" value={config.delayNs} unit="ns"
          min={0} max={16700000}
          onChange={(v) => updateConfig('delayNs', v)}
        />
        <ParamInput
          label="Width" value={config.widthNs} unit="ns"
          min={1} max={65000000}
          onChange={(v) => updateConfig('widthNs', v)}
        />
        <ParamInput
          label="Repeat Count" value={config.repeatCount} unit="×"
          min={1} max={255}
          onChange={(v) => updateConfig('repeatCount', v)}
        />
        <ParamInput
          label="Repeat Delay" value={config.repeatDelayNs} unit="ns"
          min={0} max={16700000}
          onChange={(v) => updateConfig('repeatDelayNs', v)}
        />
      </View>

      {/* Mode-specific parameters */}
      {config.mode === GLITCH_MODES.VOLTAGE_GLITCH && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Voltage Glitch Settings</Text>
          <ParamInput
            label="Target VCC" value={config.vccTargetMv} unit="mV"
            min={500} max={5000}
            onChange={(v) => updateConfig('vccTargetMv', v)}
          />
          <ParamInput
            label="Max Current" value={config.maxCurrentMa} unit="mA"
            min={50} max={3000}
            onChange={(v) => updateConfig('maxCurrentMa', v)}
          />
        </View>
      )}

      {config.mode === GLITCH_MODES.EM_GLITCH && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>EM Glitch Settings</Text>
          <ParamInput
            label="Amplitude (DAC)" value={config.emAmplitude} unit=""
            min={0} max={1023}
            onChange={(v) => updateConfig('emAmplitude', v)}
          />
        </View>
      )}

      {config.mode === GLITCH_MODES.CLOCK_GLITCH && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Clock Glitch Settings</Text>
          <ParamInput
            label="Phase Offset" value={config.clkPhaseOffset} unit="ps"
            min={0} max={10000}
            onChange={(v) => updateConfig('clkPhaseOffset', v)}
          />
        </View>
      )}

      {/* Trigger Configuration */}
      <TriggerSelector
        value={config.triggerMode}
        onChange={(m) => updateConfig('triggerMode', m)}
        edgeValue={config.triggerEdge}
        onEdgeChange={(e) => updateConfig('triggerEdge', e)}
      />

      {config.triggerMode === TRIGGER_MODES.UART_PATTERN && (
        <UARTPatternEditor
          pattern={config.uartPattern}
          mask={config.uartMask}
          onPatternChange={(p) => updateConfig('uartPattern', p)}
          onMaskChange={(m) => updateConfig('uartMask', m)}
        />
      )}

      {config.triggerMode === TRIGGER_MODES.JTAG_STATE && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>JTAG TAP State</Text>
          <ParamInput
            label="TAP State" value={config.jtagState} unit=""
            min={0} max={15}
            onChange={(v) => updateConfig('jtagState', v)}
          />
        </View>
      )}

      {/* Safety & Options */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Options</Text>
        <View style={styles.switchRow}>
          <Text style={styles.paramLabel}>Auto-Arm</Text>
          <Switch
            value={config.autoArm}
            onValueChange={(v) => updateConfig('autoArm', v)}
            trackColor={{ false: '#333', true: '#6c5ce7' }}
            thumbColor="#fff"
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.paramLabel}>Safety Limit</Text>
          <Switch
            value={config.safetyLimit}
            onValueChange={(v) => updateConfig('safetyLimit', v)}
            trackColor={{ false: '#333', true: '#6c5ce7' }}
            thumbColor="#fff"
          />
        </View>
      </View>

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.applyBtn, !dirty && styles.applyBtnDisabled]}
          onPress={applyConfig}
          disabled={!dirty}
        >
          <Text style={styles.applyBtnText}>Apply Config</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.resetBtn} onPress={resetConfig}>
          <Text style={styles.resetBtnText}>Reset Defaults</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

/* ============================================================================
 * Styles
 * ============================================================================ */

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0f' },
  scrollContent: { padding: 16, paddingBottom: 100 },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  sectionTitle: {
    color: '#6c5ce7',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 12,
  },
  modeRow: { flexDirection: 'row', justifyContent: 'space-between' },
  modeButton: {
    flex: 1, backgroundColor: '#252540', borderRadius: 8,
    padding: 12, marginHorizontal: 4, alignItems: 'center',
  },
  modeButtonActive: { backgroundColor: '#6c5ce7' },
  modeLabel: { color: '#a0a0b0', fontSize: 14, fontWeight: '600' },
  modeLabelActive: { color: '#fff' },
  modeDesc: { color: '#666', fontSize: 11, marginTop: 4 },
  paramRow: {
    flexDirection: 'row', alignItems: 'center',
    justifyContent: 'space-between', marginBottom: 8,
  },
  paramLabel: { color: '#ccc', fontSize: 14, flex: 1 },
  paramInputWrap: { flexDirection: 'row', alignItems: 'center', flex: 1 },
  paramInput: {
    backgroundColor: '#252540', color: '#fff', borderRadius: 6,
    padding: 8, fontSize: 14, width: 80, textAlign: 'center',
  },
  paramUnit: { color: '#666', fontSize: 12, marginLeft: 4 },
  paramRange: { color: '#444', fontSize: 10, width: 60, textAlign: 'right' },
  triggerGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 4 },
  triggerBtn: {
    backgroundColor: '#252540', borderRadius: 6, paddingHorizontal: 12,
    paddingVertical: 8, margin: 2,
  },
  triggerBtnActive: { backgroundColor: '#6c5ce7' },
  triggerLabel: { color: '#a0a0b0', fontSize: 13 },
  triggerLabelActive: { color: '#fff' },
  edgeRow: { flexDirection: 'row', alignItems: 'center', marginTop: 8, gap: 4 },
  edgeBtn: { backgroundColor: '#252540', borderRadius: 4, paddingHorizontal: 8, paddingVertical: 4 },
  edgeBtnActive: { backgroundColor: '#4834d4' },
  edgeText: { color: '#ccc', fontSize: 12 },
  uartRow: { flexDirection: 'row', justifyContent: 'space-around' },
  uartByteWrap: { alignItems: 'center' },
  uartLabel: { color: '#666', fontSize: 10, marginBottom: 2 },
  uartInput: {
    backgroundColor: '#252540', color: '#00cec9', borderRadius: 4,
    padding: 6, fontSize: 14, fontFamily: 'monospace', width: 48, textAlign: 'center',
  },
  shapeRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 4 },
  shapeBtn: { backgroundColor: '#252540', borderRadius: 6, paddingHorizontal: 10, paddingVertical: 6 },
  shapeBtnActive: { backgroundColor: '#6c5ce7' },
  shapeText: { color: '#a0a0b0', fontSize: 12 },
  shapeTextActive: { color: '#fff' },
  switchRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', marginBottom: 8,
  },
  actionRow: { flexDirection: 'row', gap: 12, marginTop: 8 },
  applyBtn: {
    flex: 1, backgroundColor: '#6c5ce7', borderRadius: 8,
    paddingVertical: 14, alignItems: 'center',
  },
  applyBtnDisabled: { backgroundColor: '#333' },
  applyBtnText: { color: '#fff', fontSize: 16, fontWeight: '700' },
  resetBtn: {
    backgroundColor: '#333', borderRadius: 8,
    paddingVertical: 14, paddingHorizontal: 20, alignItems: 'center',
  },
  resetBtnText: { color: '#a0a0b0', fontSize: 14 },
});

export default GlitchConfig;