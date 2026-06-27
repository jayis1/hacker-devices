/**
 * Tactile-Phantom — Companion App
 * screens/LayoutManagerScreen.tsx — Configure screen layout for keystroke inference
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet, Alert, ScrollView,
} from 'react-native';
import { bleManager } from '../src/ble';
import { useStore } from '../src/store';

export default function LayoutManagerScreen() {
  const screenRes = useStore((s) => s.screenResolution);
  const setScreenResolution = useStore((s) => s.setScreenResolution);
  const setLayout = useStore((s) => s.setLayout);

  const [width, setWidth] = useState(String(screenRes.width));
  const [height, setHeight] = useState(String(screenRes.height));
  const [layoutType, setLayoutType] = useState<'qwerty' | 'dialer' | 'custom'>('qwerty');

  // Generate a default QWERTY layout for the given resolution
  const generateQwerty = (w: number, h: number) => {
    const keys: { x: number; y: number; width: number; height: number; label: string }[] = [];
    const kbTop = Math.round(h * 0.70);
    const kbH = Math.round(h * 0.25);
    const keyW = Math.round(w / 10);
    const keyH = Math.round(kbH / 4);
    let y = kbTop;

    const rows = ['QWERTYUIOP', 'ASDFGHJKL', 'ZXCVBNM'];
    for (let r = 0; r < rows.length; r++) {
      const row = rows[r];
      const offset = r === 0 ? 0 : r === 1 ? keyW / 2 : keyW * 1.5;
      for (let c = 0; c < row.length; c++) {
        keys.push({
          x: Math.round(offset + c * keyW + keyW / 2),
          y: y + Math.round(keyH / 2),
          width: keyW, height: keyH,
          label: row[c],
        });
      }
      y += keyH;
    }
    // Space
    keys.push({
      x: Math.round(w / 2), y: y + Math.round(keyH / 2),
      width: Math.round(w * 0.6), height: keyH, label: ' ',
    });
    return keys;
  };

  // Generate a dialer pad layout (1-9, *, 0, #)
  const generateDialer = (w: number, h: number) => {
    const keys: { x: number; y: number; width: number; height: number; label: string }[] = [];
    const padTop = Math.round(h * 0.30);
    const padH = Math.round(h * 0.50);
    const cellW = Math.round(w / 3);
    const cellH = Math.round(padH / 4);
    const dialer = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '0', '#'];
    for (let i = 0; i < 12; i++) {
      const row = Math.floor(i / 3);
      const col = i % 3;
      keys.push({
        x: col * cellW + Math.round(cellW / 2),
        y: padTop + row * cellH + Math.round(cellH / 2),
        width: cellW, height: cellH,
        label: dialer[i],
      });
    }
    return keys;
  };

  const sendLayoutToDevice = async () => {
    const w = parseInt(width, 10);
    const h = parseInt(height, 10);
    if (isNaN(w) || isNaN(h) || w <= 0 || h <= 0) {
      Alert.alert('Invalid', 'Screen dimensions must be positive numbers');
      return;
    }

    let keys;
    if (layoutType === 'qwerty') keys = generateQwerty(w, h);
    else if (layoutType === 'dialer') keys = generateDialer(w, h);
    else keys = [];

    setLayout(keys);
    setScreenResolution(w, h);

    // Serialize and send to device via BLE
    // Format: [2 bytes count] + [8 bytes per key (x,y,w,h)] + [1 byte label per key]
    const buf = new Uint8Array(2 + keys.length * 9);
    buf[0] = keys.length & 0xFF;
    buf[1] = (keys.length >> 8) & 0xFF;
    let pos = 2;
    for (const k of keys) {
      buf[pos++] = k.x & 0xFF;
      buf[pos++] = (k.x >> 8) & 0xFF;
      buf[pos++] = k.y & 0xFF;
      buf[pos++] = (k.y >> 8) & 0xFF;
      buf[pos++] = k.width & 0xFF;
      buf[pos++] = (k.width >> 8) & 0xFF;
      buf[pos++] = k.height & 0xFF;
      buf[pos++] = (k.height >> 8) & 0xFF;
      buf[pos++] = k.label.charCodeAt(0);
    }

    const ok = await bleManager.sendLayout(buf);
    Alert.alert(ok ? 'Layout Sent' : 'Send Failed',
      ok ? `${keys.length} keys for ${w}x${h} (${layoutType})` : 'Check BLE connection');
  };

  return (
    <View style={styles.container}>
      <ScrollView>
        <Text style={styles.title}>Layout Manager</Text>
        <Text style={styles.subtitle}>
          Configure target screen layout for keystroke inference.
          The layout model maps touch coordinates to typed characters.
        </Text>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Screen Resolution</Text>
          <View style={styles.inputRow}>
            <TextInput style={styles.input} value={width} onChangeText={setWidth} placeholder="Width" placeholderTextColor="#555" keyboardType="numeric" />
            <Text style={styles.times}>×</Text>
            <TextInput style={styles.input} value={height} onChangeText={setHeight} placeholder="Height" placeholderTextColor="#555" keyboardType="numeric" />
          </View>
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Layout Type</Text>
          <View style={styles.typeRow}>
            {(['qwerty', 'dialer', 'custom'] as const).map((t) => (
              <TouchableOpacity
                key={t}
                style={[styles.typeBtn, layoutType === t && styles.typeBtnActive]}
                onPress={() => setLayoutType(t)}
              >
                <Text style={[styles.typeBtnText, layoutType === t && styles.typeBtnTextActive]}>
                  {t.toUpperCase()}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
          <Text style={styles.typeDesc}>
            {layoutType === 'qwerty' && 'Standard QWERTY keyboard (bottom 25% of screen)'}
            {layoutType === 'dialer' && 'Numeric dialer pad (3x4 grid)'}
            {layoutType === 'custom' && 'Custom layout (configure via coordinate editor)'}
          </Text>
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Preview</Text>
          <Text style={styles.previewText}>
            Resolution: {width}×{height}{'\n'}
            Layout: {layoutType.toUpperCase()}{'\n'}
            Keys: {layoutType === 'qwerty' ? '26 letters + space' :
                   layoutType === 'dialer' ? '12 digits (0-9, *, #)' : 'custom'}
          </Text>
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Common Resolutions</Text>
          {[
            { name: 'Phone (1080×2400)', w: '1080', h: '2400' },
            { name: 'Phone (720×1600)', w: '720', h: '1600' },
            { name: 'Tablet (1200×1920)', w: '1200', h: '1920' },
            { name: 'Tablet (800×1280)', w: '800', h: '1280' },
            { name: 'POS (600×1024)', w: '600', h: '1024' },
          ].map((preset) => (
            <TouchableOpacity
              key={preset.name}
              style={styles.presetBtn}
              onPress={() => { setWidth(preset.w); setHeight(preset.h); }}
            >
              <Text style={styles.presetText}>{preset.name}</Text>
            </TouchableOpacity>
          ))}
        </View>

        <TouchableOpacity style={styles.sendBtn} onPress={sendLayoutToDevice}>
          <Text style={styles.sendBtnText}>Send Layout to Device</Text>
        </TouchableOpacity>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 15 },
  title: { color: '#e0e0e0', fontSize: 22, fontWeight: 'bold', textAlign: 'center', marginBottom: 5 },
  subtitle: { color: '#888', fontSize: 12, textAlign: 'center', marginBottom: 20 },
  section: { backgroundColor: '#16213e', borderRadius: 10, padding: 12, marginBottom: 12, borderWidth: 1, borderColor: '#0f3460' },
  sectionTitle: { color: '#00ff88', fontSize: 13, fontWeight: 'bold', marginBottom: 8 },
  inputRow: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  input: { flex: 1, backgroundColor: '#0a0a14', borderRadius: 6, padding: 8, color: '#e0e0e0', fontSize: 13, borderWidth: 1, borderColor: '#1a1a2e' },
  times: { color: '#888', fontSize: 16 },
  typeRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  typeBtn: { flex: 1, backgroundColor: '#0a0a14', borderRadius: 6, padding: 8, alignItems: 'center', borderWidth: 1, borderColor: '#1a1a2e' },
  typeBtnActive: { borderColor: '#00ff88', backgroundColor: '#0a2a1a' },
  typeBtnText: { color: '#888', fontSize: 12 },
  typeBtnTextActive: { color: '#00ff88' },
  typeDesc: { color: '#666', fontSize: 11 },
  previewText: { color: '#aaa', fontSize: 12, fontFamily: 'monospace' },
  presetBtn: { backgroundColor: '#0a0a14', borderRadius: 6, padding: 8, marginBottom: 5, borderWidth: 1, borderColor: '#1a1a2e' },
  presetText: { color: '#aaa', fontSize: 12 },
  sendBtn: { backgroundColor: '#0a2a1a', borderRadius: 10, padding: 15, alignItems: 'center', borderWidth: 1, borderColor: '#00ff88', marginTop: 5 },
  sendBtnText: { color: '#00ff88', fontSize: 16, fontWeight: 'bold' },
});