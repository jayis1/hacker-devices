// lumen-tap/app/screens/SafetyScreen.js
// Interlock status, ambient-light reading, Class indicator, kill switch.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

import React, { useState, useContext } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert } from 'react-native';
import { Commands, safetyVerdict, dutyToMw } from '../utils/protocol';
import { DeviceContext } from '../App';

export function SafetyScreen() {
  const { send, status } = useContext(DeviceContext);
  const verdict = safetyVerdict(status);
  const pwm = status?.pwm ?? 0;
  const mw = dutyToMw(pwm);
  const classLabel = mw <= 0.39 ? 'CLASS 1 (eye-safe)' : (mw <= 1.0 ? 'CLASS 1M' : 'CLASS 3R');
  const classColor = mw <= 0.39 ? '#39FF14' : (mw <= 1.0 ? '#FFA500' : '#FF4040');

  const confirmKill = () => {
    Alert.alert(
      'LASER KILL',
      'Immediately cut laser emission and disarm?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'KILL', style: 'destructive',
          onPress: () => { send(Commands.disarm()); send(Commands.setPower(0)); } },
      ]
    );
  };

  const confirmArm = () => {
    Alert.alert(
      'ARM LASER',
      'Authorise laser emission? Verify target is in view and area is clear.',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'ARM', style: 'destructive',
          onPress: () => send(Commands.arm()) },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.h1}>SAFETY</Text>

      <View style={[styles.classBox, {borderColor: classColor}]}>
        <Text style={[styles.classLabel, {color: classColor}]}>{classLabel}</Text>
        <Text style={styles.classPower}>≈ {mw.toFixed(2)} mW @ 520 nm</Text>
        <Text style={styles.classNote}>PWM {pwm}/255  (cap 39 = Class 1)</Text>
      </View>

      <Text style={styles.section}>INTERLOCK STATUS</Text>
      <Interlock label="Keyed arm switch (HW)" ok={!!status?.arm} />
      <Interlock label="Ambient light safe"    ok={(status?.ambient ?? 0) < 6000} />
      <Interlock label="Software Class 1 cap"  ok={pwm <= 39} />
      <Interlock label="Watchdog"              ok={!!status?.arm} /* proxy */ />
      <Interlock label="SD not in fault"       ok={(status?.sd_state ?? 0) !== 3} />

      <View style={[styles.verdict, {borderColor: verdict.canEmit ? '#39FF14' : '#FF4040'}]}>
        <Text style={{color: verdict.canEmit ? '#39FF14' : '#FF4040', fontFamily:'monospace'}}>
          {verdict.canEmit ? 'EMISSION PERMITTED' : 'BLOCKED: ' + verdict.reasons.join(', ')}
        </Text>
      </View>

      <Text style={styles.section}>CONTROLS</Text>
      <View style={{flexDirection:'row'}}>
        <TouchableOpacity style={[styles.btn, styles.btnGo]} onPress={confirmArm}>
          <Text style={[styles.btnTxt, styles.btnGoTxt]}>ARM</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.btn, styles.btnDanger]} onPress={confirmKill}>
          <Text style={styles.btnTxt}>KILL</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.section}>AMBIENT LIGHT</Text>
      <Text style={styles.label}>TSL257 reading: {status?.ambient ?? '--'} / 16383</Text>
      <Text style={styles.label}>Block threshold: 6000 (heuristic eye-in-path detector)</Text>

      <Text style={styles.section}>⚠️ LEGAL & ETHICAL</Text>
      <Text style={styles.legal}>
        Pointing a laser at a window, person, vehicle, or aircraft you do not
        own or have written authorization to test is illegal in most
        jurisdictions. LumenTap ships with hardware and software interlocks;
        bypassing them is the sole responsibility of the operator. Always
        operate in Class 1 mode unless a qualified laser safety officer has
        approved higher power. The author (jayis1) disclaims all liability
        for misuse.
      </Text>
    </ScrollView>
  );
}

function Interlock({label, ok}) {
  return (
    <View style={styles.interlockRow}>
      <Text style={{color: ok ? '#39FF14' : '#FF4040', fontFamily:'monospace', marginRight: 8}}>
        {ok ? '✓' : '✗'}
      </Text>
      <Text style={{color: '#ccc', fontFamily:'monospace', fontSize: 12, flex: 1}}>{label}</Text>
      <Text style={{color: ok ? '#39FF14' : '#FF4040', fontFamily:'monospace', fontSize: 11}}>
        {ok ? 'OK' : 'FAIL'}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  h1: { color: '#39FF14', fontSize: 20, fontFamily: 'monospace', marginBottom: 8 },
  section: { color: '#888', fontSize: 11, fontFamily: 'monospace',
             marginTop: 14, marginBottom: 4, borderBottomWidth: 1, borderBottomColor: '#222' },
  classBox: { borderWidth: 2, borderRadius: 6, padding: 12, alignItems: 'center', marginVertical: 8 },
  classLabel: { fontFamily: 'monospace', fontSize: 16, fontWeight: 'bold' },
  classPower: { color: '#ccc', fontFamily: 'monospace', fontSize: 13, marginTop: 4 },
  classNote: { color: '#666', fontFamily: 'monospace', fontSize: 10, marginTop: 2 },
  verdict: { borderWidth: 1, borderRadius: 4, padding: 10, marginVertical: 10, alignItems: 'center' },
  interlockRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 4 },
  btn: { flex: 1, padding: 14, borderRadius: 4, alignItems: 'center', marginHorizontal: 4, backgroundColor: '#222' },
  btnGo: { backgroundColor: '#39FF14' },
  btnDanger: { backgroundColor: '#FF4040' },
  btnTxt: { color: '#39FF14', fontFamily: 'monospace' },
  btnGoTxt: { color: '#000', fontWeight: 'bold' },
  label: { color: '#ccc', fontFamily: 'monospace', fontSize: 12, marginVertical: 2 },
  legal: { color: '#999', fontFamily: 'monospace', fontSize: 10, marginTop: 6, lineHeight: 16 },
});