/**
 * AperturePhantom — InjectEditorScreen
 *
 * Compose an injection frame: choose a target slot (0-3), set the CSI-2
 * data type / width / height, optionally draw an adversarial patch over a
 * base background, then push to the device and trigger injection.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, TextInput, Button, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const DT_OPTIONS = [
  { label: 'YUV422 8-bit',  value: 0x1E },
  { label: 'YUV422 10-bit', value: 0x1F },
  { label: 'RGB565',        value: 0x22 },
  { label: 'RGB888',        value: 0x24 },
  { label: 'RAW8',          value: 0x2A },
  { label: 'RAW10',         value: 0x2B },
  { label: 'RAW12',         value: 0x2C },
];

export default function InjectEditorScreen() {
  const { injectLoad, injectNow, injectStop, send } = useDevice();
  const [slot, setSlot] = useState(0);
  const [dt, setDt] = useState(0x1E);
  const [w, setW] = useState('640');
  const [h, setH] = useState('480');
  const [patchX, setPatchX] = useState('0');
  const [patchY, setPatchY] = useState('0');
  const [patchR, setPatchR] = useState('255');
  const [patchG, setPatchG] = useState('255');
  const [patchB, setPatchB] = useState('255');
  const [busy, setBusy] = useState(false);

  /* Build a synthetic YUV422 8-bit frame of the configured size, optionally
   * with a 64x64 colored patch at (patchX, patchY). For other DTs we still
   * emit raw bytes; the device FPGA will re-pack per the configured DT. */
  const buildFrame = () => {
    const width = parseInt(w, 10) || 640;
    const height = parseInt(h, 10) || 480;
    const bytes = new Uint8Array(width * height * 2); /* YUV422 8: 2 bytes/pix */
    /* fill Y=128, U/V=128 (mid-gray) */
    bytes.fill(128);
    /* draw 64x64 patch */
    const px = parseInt(patchX, 10) || 0;
    const py = parseInt(patchY, 10) || 0;
    const pr = parseInt(patchR, 10) || 0;
    const pg = parseInt(patchG, 10) || 0;
    const pb = parseInt(patchB, 10) || 0;
    /* convert patch RGB → YUV */
    const y = Math.min(255, Math.round(0.299 * pr + 0.587 * pg + 0.114 * pb));
    for (let j = 0; j < 64 && (py + j) < height; j++) {
      for (let i = 0; i < 64 && (px + i) < width; i++) {
        const idx = ((py + j) * width + (px + i)) * 2;
        bytes[idx] = y;
      }
    }
    return bytes;
  };

  const onPush = async () => {
    setBusy(true);
    try {
      const frame = buildFrame();
      await injectLoad(slot, frame);
      /* also push frame header params via SET_MODE/INJECT helpers through send() */
      Alert.alert('Loaded', `Slot ${slot}: ${frame.length} bytes (dt=0x${dt.toString(16)})`);
    } finally { setBusy(false); }
  };

  const onInject = async () => {
    setBusy(true);
    try { await injectNow(slot); }
    finally { setBusy(false); }
  };

  const onStop = async () => { await injectStop(); };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>Inject Editor</Text>

      <Row label="Slot (0-3)">
        <TextInput style={styles.in} value={String(slot)}
          keyboardType="numeric" onChangeText={(t) => setSlot(parseInt(t, 10) || 0)} />
      </Row>

      <Row label="Data type">
        {DT_OPTIONS.map((o) => (
          <Button key={o.value} color={dt === o.value ? '#4080d0' : '#506070'}
            title={o.label} onPress={() => setDt(o.value)} />
        ))}
      </Row>

      <View style={styles.row}>
        <Field label="Width"  value={w} set={setW} />
        <Field label="Height" value={h} set={setH} />
      </View>

      <Text style={styles.section}>Adversarial patch (64×64)</Text>
      <View style={styles.row}>
        <Field label="X" value={patchX} set={setPatchX} />
        <Field label="Y" value={patchY} set={setPatchY} />
      </View>
      <View style={styles.row}>
        <Field label="R" value={patchR} set={setPatchR} />
        <Field label="G" value={patchG} set={setPatchG} />
        <Field label="B" value={patchB} set={setPatchB} />
      </View>

      <View style={styles.row}>
        <Button title="Push to slot" onPress={onPush} disabled={busy} />
        <Button title="Inject now" color="#a04040" onPress={onInject} disabled={busy} />
        <Button title="Stop" onPress={onStop} />
      </View>

      <Text style={styles.warn}>
        ⚠ Injecting synthetic frames replaces what the host CV pipeline sees.
        Authorized targets only.
      </Text>
      <Text style={styles.author}>author: jayis1</Text>
    </View>
  );
}

function Row({ label, children }) {
  return (
    <View style={styles.row}>
      <Text style={styles.label}>{label}</Text>
      <View style={{ flex: 1, flexDirection: 'row', flexWrap: 'wrap', gap: 6 }}>
        {children}
      </View>
    </View>
  );
}
function Field({ label, value, set }) {
  return (
    <View style={styles.field}>
      <Text style={styles.fieldLabel}>{label}</Text>
      <TextInput style={styles.in} value={value} keyboardType="numeric"
                 onChangeText={set} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#101418' },
  h1: { color: '#d0e0f0', fontSize: 16, fontWeight: '600', marginBottom: 12 },
  row: { flexDirection: 'row', alignItems: 'center', gap: 10, marginVertical: 6 },
  label: { color: '#8aa0b8', fontSize: 12, width: 90 },
  field: { flexDirection: 'column', marginRight: 12 },
  fieldLabel: { color: '#8aa0b8', fontSize: 10 },
  in: { borderWidth: 1, borderColor: '#304050', color: '#d0e0f0',
        paddingHorizontal: 8, paddingVertical: 4, width: 80,
        borderRadius: 4, marginTop: 2 },
  section: { color: '#a0b0c0', fontSize: 12, fontWeight: '600', marginTop: 12 },
  warn: { color: '#e0a040', fontSize: 11, marginTop: 16, fontStyle: 'italic' },
  author: { color: '#5a7088', fontSize: 10, marginTop: 12 },
});