/**
 * InjectScreen.js — WriteProperty console with life-safety interlock banner.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * Lets the operator issue a WriteProperty to an analog-value (type 2) or
 * binary-output (type 4) object. The life-safety blocklist is enforced in
 * firmware (watchdog task); the app surfaces a red banner for blocked
 * object types so the operator does not waste an ACK cycle on a frame the
 * Phantom will never emit.
 */
import React, { useState } from 'react';
import { View, Text, TextInput, Button, StyleSheet, Alert, Slider } from 'react-native';
import { usePhantom } from '../src/PhantomContext';

const LIFESAFETY_TYPES = { 18: 'life-safety-point', 19: 'life-safety-zone',
                           24: 'pulse-converter', 21: 'program',
                           39: 'life-safety-array' };

const OBJECT_TYPES = {
  0: 'analog-input', 1: 'analog-output', 2: 'analog-value',
  3: 'binary-input', 4: 'binary-output', 5: 'binary-value',
  14: 'multi-state-value',
};

export default function InjectScreen() {
  const { sendCommand, connected } = usePhantom();
  const [objType, setObjType] = useState('2');     /* analog-value default */
  const [objInst, setObjInst] = useState('1');
  const [propId, setPropId] = useState('85');     /* 85 = present-value */
  const [value, setValue] = useState(22.5);

  const blocked = LIFESAFETY_TYPES[parseInt(objType, 10)];

  const fire = async () => {
    if (!connected) { Alert.alert('Not connected'); return; }
    if (blocked) { Alert.alert('Blocked', `${blocked} is on the life-safety blocklist`); return; }
    const ok = await sendCommand({
      op: 'writeprop',
      a1: parseInt(objType, 10),
      a2: parseInt(objInst, 10),
      a3: parseInt(propId, 10),
      f: value,
    });
    Alert.alert(ok ? 'sent' : 'failed', ok ? 'WriteProperty emitted — watchdog armed' : 'BLE write failed');
  };

  return (
    <View style={s.wrap}>
      <Text style={s.h2}>Inject</Text>
      {blocked && (
        <View style={s.banner}>
          <Text style={s.bannerText}>
            ⛔ {blocked} — life-safety blocklist. Firmware will refuse this write
            regardless of jumper state.
          </Text>
        </View>
      )}
      <Text style={s.label}>Object type ({Object.keys(OBJECT_TYPES).map((k)=>OBJECT_TYPES[k]).join(', ')})</Text>
      <TextInput style={s.in} value={objType} onChangeText={setObjType} keyboardType="numeric" />
      <Text style={s.label}>Object instance</Text>
      <TextInput style={s.in} value={objInst} onChangeText={setObjInst} keyboardType="numeric" />
      <Text style={s.label}>Property ID (85 = present-value)</Text>
      <TextInput style={s.in} value={propId} onChangeText={setPropId} keyboardType="numeric" />
      <Text style={s.label}>Value: {value.toFixed(2)}</Text>
      <Slider minimumValue={-20} maximumValue={120} step={0.5} value={value}
              onValueChange={setValue} style={s.slider} />
      <Button title="WriteProperty" color="#e63946" onPress={fire} disabled={!connected || !!blocked} />
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, padding: 20, backgroundColor: '#0f1115' },
  h2: { fontSize: 20, fontWeight: 'bold', color: '#e63946', marginTop: 8, marginBottom: 8 },
  banner: { backgroundColor: '#5a0a0a', padding: 10, borderRadius: 4, marginBottom: 12,
            borderWidth: 1, borderColor: '#e63946' },
  bannerText: { color: '#ffc0c0', fontSize: 12 },
  label: { color: '#aaa', marginTop: 10, fontSize: 12 },
  in: { color: '#fff', borderBottomWidth: 1, borderColor: '#333', padding: 8, marginTop: 4 },
  slider: { height: 40, marginTop: 8 },
});