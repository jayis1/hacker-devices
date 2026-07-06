// lumen-tap/app/components/ParamSlider.js
// Reusable labeled slider for DSP / laser parameter editing.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

import React from 'react';
import { View, Text, Slider, StyleSheet } from 'react-native';

export function ParamSlider({
  label, value, min, max, step = 1, unit = '', onValueChange,
  displayPrecision = 0, disabled = false,
}) {
  const display = (displayPrecision > 0)
    ? value.toFixed(displayPrecision)
    : Math.round(value).toString();
  return (
    <View style={[styles.row, disabled && styles.disabled]}>
      <View style={styles.labelCol}>
        <Text style={styles.label}>{label}</Text>
        <Text style={styles.value}>{display} {unit}</Text>
      </View>
      <Slider
        style={styles.slider}
        minimumValue={min}
        maximumValue={max}
        step={step}
        value={value}
        onValueChange={onValueChange}
        disabled={disabled}
        minimumTrackTintColor="#39FF14"
        maximumTrackTintColor="#333"
      />
    </View>
  );
}

const styles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 8,
    paddingHorizontal: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#222',
  },
  disabled: { opacity: 0.4 },
  labelCol: { width: 110 },
  label: { color: '#ccc', fontSize: 12, fontFamily: 'monospace' },
  value: { color: '#39FF14', fontSize: 14, fontFamily: 'monospace' },
  slider: { flex: 1, height: 40 },
});