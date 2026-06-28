/**
 * TemperatureGauge.tsx - Circular temperature gauge component
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Text, Surface } from 'react-native-paper';

interface TemperatureGaugeProps {
  label: string;
  value: number;
  target?: number;
  unit?: string;
  min?: number;
  max?: number;
  color?: string;
}

export default function TemperatureGauge({
  label,
  value,
  target,
  unit = '°C',
  min = -20,
  max = 130,
  color = '#FF5722',
}: TemperatureGaugeProps) {
  const range = max - min;
  const pct = Math.max(0, Math.min(100, ((value - min) / range) * 100));
  
  // Color based on temperature
  const getColor = (temp: number): string => {
    if (temp < 0) return '#00BCD4';   // Cyan for cold
    if (temp < 25) return '#4CAF50';   // Green for normal
    if (temp < 60) return '#FFEB3B';   // Yellow for warm
    if (temp < 90) return '#FF9800';   // Orange for hot
    return '#F44336';                   // Red for very hot
  };

  const valueColor = getColor(value);
  
  return (
    <Surface style={styles.container} elevation={2}>
      <Text style={styles.label}>{label}</Text>
      <View style={styles.gaugeContainer}>
        <Text style={[styles.value, { color: valueColor }]}>
          {value.toFixed(1)}
        </Text>
        <Text style={styles.unit}>{unit}</Text>
      </View>
      {target !== undefined && (
        <Text style={styles.target}>Target: {target.toFixed(1)}{unit}</Text>
      )}
      <View style={styles.barContainer}>
        <View style={styles.barBackground}>
          <View
            style={[
              styles.barFill,
              {
                width: `${pct}%`,
                backgroundColor: valueColor,
              },
            ]}
          />
        </View>
      </View>
      <View style={styles.scaleContainer}>
        <Text style={styles.scaleLabel}>{min}°</Text>
        <Text style={styles.scaleLabel}>{max}°</Text>
      </View>
    </Surface>
  );
}

const styles = StyleSheet.create({
  container: {
    padding: 16,
    borderRadius: 12,
    backgroundColor: '#16213e',
    margin: 8,
    alignItems: 'center',
    minWidth: 140,
  },
  label: {
    fontSize: 14,
    color: '#aaa',
    marginBottom: 8,
  },
  gaugeContainer: {
    flexDirection: 'row',
    alignItems: 'baseline',
  },
  value: {
    fontSize: 36,
    fontWeight: 'bold',
  },
  unit: {
    fontSize: 16,
    color: '#888',
    marginLeft: 4,
  },
  target: {
    fontSize: 12,
    color: '#888',
    marginTop: 4,
  },
  barContainer: {
    width: '100%',
    marginTop: 12,
  },
  barBackground: {
    height: 6,
    backgroundColor: '#333',
    borderRadius: 3,
    overflow: 'hidden',
  },
  barFill: {
    height: '100%',
    borderRadius: 3,
  },
  scaleContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    width: '100%',
    marginTop: 4,
  },
  scaleLabel: {
    fontSize: 10,
    color: '#666',
  },
});

// Author: jayis1