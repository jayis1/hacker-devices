/**
 * components/Themed.js — small themed primitives for the 1553-Phantom app
 *
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

const COLORS = {
  bg: '#0d1117',
  surface: '#161b22',
  border: '#30363d',
  text: '#c9d1d9',
  muted: '#8b949e',
  accent: '#58a6ff',
  danger: '#f85149',
  ok: '#3fb950',
  warn: '#d29922',
};

export function Card({ children, style }) {
  return <View style={[styles.card, style]}>{children}</View>;
}

export function Row({ children, style }) {
  return <View style={[styles.row, style]}>{children}</View>;
}

export function Label({ children }) {
  return <Text style={styles.label}>{children}</Text>;
}

export function Value({ children, color }) {
  return <Text style={[styles.value, color ? { color } : null]}>{children}</Text>;
}

export function Button({ title, onPress, color }) {
  return (
    <TouchableOpacity
      onPress={onPress}
      style={[styles.btn, color ? { borderColor: color } : null]}
    >
      <Text style={[styles.btnText, color ? { color } : null]}>{title}</Text>
    </TouchableOpacity>
  );
}

export function Bar({ label, pct, color }) {
  return (
    <View style={styles.barWrap}>
      <Text style={styles.barLabel}>{label}</Text>
      <View style={styles.barOuter}>
        <View
          style={[styles.barInner, {
            width: `${Math.max(0, Math.min(100, pct))}%`,
            backgroundColor: color || COLORS.accent,
          }]}
        />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: COLORS.surface,
    borderColor: COLORS.border,
    borderWidth: 1,
    borderRadius: 8,
    padding: 12,
    marginVertical: 6,
  },
  row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', paddingVertical: 4 },
  label: { color: COLORS.muted, fontSize: 12, fontWeight: '600', textTransform: 'uppercase', letterSpacing: 0.5 },
  value: { color: COLORS.text, fontSize: 16, fontWeight: '500' },
  btn: {
    backgroundColor: COLORS.surface,
    borderColor: COLORS.border,
    borderWidth: 1,
    borderRadius: 6,
    paddingVertical: 8,
    paddingHorizontal: 14,
    marginVertical: 4,
  },
  btnText: { color: COLORS.accent, fontSize: 14, fontWeight: '600', textAlign: 'center' },
  barWrap: { marginVertical: 4 },
  barLabel: { color: COLORS.muted, fontSize: 11, marginBottom: 2 },
  barOuter: { height: 8, backgroundColor: COLORS.bg, borderRadius: 4, borderWidth: 1, borderColor: COLORS.border },
  barInner: { height: 8, borderRadius: 4 },
});

export { COLORS };