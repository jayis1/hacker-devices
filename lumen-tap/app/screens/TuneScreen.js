// lumen-tap/app/screens/TuneScreen.js
// DSP parameter editing — demod mode, bandpass, noise depth, AGC target.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

import React, { useState, useContext } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Switch } from 'react-native';
import { ParamSlider } from '../components/ParamSlider';
import { Commands, DEMOD_LABELS } from '../utils/protocol';
import { DeviceContext } from '../App';

export function TuneScreen() {
  const { send, status } = useContext(DeviceContext);
  const [demod, setDemod]   = useState(1);
  const [bpLo, setBpLo]     = useState(20);
  const [bpHi, setBpHi]     = useState(16000);
  const [noise, setNoise]   = useState(0.6);
  const [bypass, setBypass] = useState(false);

  // local commit helpers (debounce-ish: send on release)
  const commitBandpass = (lo, hi) => send(Commands.setBandpass(lo, hi));

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.h1}>TUNE DSP</Text>

      <Text style={styles.section}>DEMODULATION MODE</Text>
      <View style={styles.row}>
        {DEMOD_LABELS.map((lbl, i) => (
          <TouchableOpacity key={i}
            style={[styles.modeBtn, demod === i && styles.modeBtnActive]}
            onPress={() => { setDemod(i); send(Commands.setDemod(i)); }}>
            <Text style={[styles.modeBtnTxt, demod === i && styles.modeBtnTxtActive]}>{lbl}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={styles.section}>BANDPASS (Hz)</Text>
      <ParamSlider label="Low cut"   value={bpLo}  min={10}   max={2000}  step={10}
                   unit="Hz" onValueChange={setBpLo}
                   onValueChange={() => {}} />
      <ParamSlider label="High cut"  value={bpHi}  min={2000} max={90000} step={500}
                   unit="Hz"
                   onValueChange={(v) => { setBpHi(v); commitBandpass(bpLo, v); }} />
      <TouchableOpacity style={styles.btn} onPress={() => commitBandpass(bpLo, bpHi)}>
        <Text style={styles.btnTxt}>APPLY BANDPASS</Text>
      </TouchableOpacity>

      <Text style={styles.section}>NOISE SUPPRESSION</Text>
      <ParamSlider label="Depth" value={noise} min={0} max={1} step={0.05}
                   unit="" displayPrecision={2}
                   onValueChange={(v) => { setNoise(v); send(Commands.setNoise(v)); }} />

      <Text style={styles.section}>DEBUG</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Bypass DSP (raw ADC out)</Text>
        <Switch value={bypass}
          onValueChange={(v) => { setBypass(v); send(Commands.setBypass(v ? 1 : 0)); }} />
      </View>

      <Text style={styles.foot}>
        Changes apply immediately via CDC. AGC target is fixed at firmware
        default (RMS ≈ 30000) — adjust in firmware if needed.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  h1: { color: '#39FF14', fontSize: 20, fontFamily: 'monospace', marginBottom: 8 },
  section: { color: '#888', fontSize: 11, fontFamily: 'monospace',
             marginTop: 14, marginBottom: 4, borderBottomWidth: 1, borderBottomColor: '#222' },
  row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
         paddingVertical: 6 },
  label: { color: '#ccc', fontFamily: 'monospace', fontSize: 12, flex: 1 },
  modeBtn: { flex: 1, padding: 10, backgroundColor: '#1a1a1a', borderRadius: 4, marginRight: 6,
             alignItems: 'center' },
  modeBtnActive: { backgroundColor: '#39FF14' },
  modeBtnTxt: { color: '#888', fontFamily: 'monospace', fontSize: 11 },
  modeBtnTxtActive: { color: '#000', fontWeight: 'bold' },
  btn: { backgroundColor: '#222', padding: 10, borderRadius: 4, alignItems: 'center', marginTop: 8 },
  btnTxt: { color: '#39FF14', fontFamily: 'monospace' },
  foot: { color: '#555', fontSize: 10, fontFamily: 'monospace', marginTop: 16, fontStyle: 'italic' },
});