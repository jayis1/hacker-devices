/**
 * AperturePhantom — ScriptEditorScreen
 *
 * Edit and upload the on-device bytecode script that runs autonomously
 * (phone-less). Operators compose sequences like:
 *   WAIT_FRAMES 5
 *   IF_TRIGGER 0
 *     INJECT 0
 *   END
 * The app serializes the program to the OP_* bytecode defined in
 * firmware/registers.h and uploads it via CMD_SCRIPT_LOAD.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, TextInput, Button, StyleSheet, Alert, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';

/* OP_* values (must match firmware/registers.h) */
const OP = {
  NOP: 0x00, WAIT_MS: 0x01, WAIT_FRAMES: 0x02, CAPTURE: 0x03, INJECT: 0x04,
  REPLAY: 0x05, SENSOR_WRITE: 0x06, SENSOR_READ: 0x07, SET_MODE: 0x08,
  IF_TRIGGER: 0x09, TRIGGER_ARM: 0x0A, TRIGGER_DISARM: 0x0B, BLE_NOTIFY: 0x0C,
  LOG: 0x0D, LED: 0x0E, MOTOR_PULSE: 0x0F, LOOP: 0x10, END_LOOP: 0x11,
  JUMP: 0x12, HALT: 0x13,
};

export default function ScriptEditorScreen() {
  const { scriptLoad, scriptRun, scriptStop, send } = useDevice();
  const [text, setText] = useState(
`# aperture-phantom script (jayis1)
# low-and-slow: only inject when the scene changes enough
WAIT_FRAMES 5
TRIGGER_ARM
LOOP 0
  IF_TRIGGER 0
    INJECT 0
    WAIT_MS 200
  END_LOOP
END_LOOP
HALT
`);

  /* Tiny line-assembler: each non-comment, non-blank line is one mnemonic
   * followed by decimal args. Produces a Uint8Array of bytecode. */
  const assemble = () => {
    const lines = text.split('\n');
    const out = [];
    for (const raw of lines) {
      const line = raw.replace(/#.*$/, '').trim();
      if (!line) continue;
      const parts = line.split(/\s+/);
      const mn = parts[0].toUpperCase();
      const args = parts.slice(1).map((a) => parseInt(a, 10) || 0);
      if (!(mn in OP)) throw new Error('unknown mnemonic: ' + mn);
      out.push(OP[mn]);
      switch (OP[mn]) {
      case OP.WAIT_MS: case OP.WAIT_FRAMES: case OP.LOOP: case OP.JUMP:
      case OP.CAPTURE: case OP.REPLAY:
        out.push((args[0] >> 8) & 0xFF, args[0] & 0xFF);
        break;
      case OP.INJECT: case OP.SENSOR_WRITE: case OP.SENSOR_READ:
      case OP.SET_MODE: case OP.IF_TRIGGER: case OP.BLE_NOTIFY:
      case OP.LOG: case OP.LED: case OP.MOTOR_PULSE:
        out.push(args[0] & 0xFF);
        if (OP[mn] === OP.SENSOR_WRITE) out.push((args[1] >> 8) & 0xFF, args[1] & 0xFF);
        break;
      case OP.NOP: case OP.TRIGGER_ARM: case OP.TRIGGER_DISARM:
      case OP.END_LOOP: case OP.HALT:
        break;
      default: break;
      }
    }
    return new Uint8Array(out);
  };

  const onUpload = async () => {
    try {
      const bc = assemble();
      await scriptLoad(bc);
      Alert.alert('Uploaded', bc.length + ' bytes of bytecode');
    } catch (e) {
      Alert.alert('Assemble failed', e.message);
    }
  };

  const onRun = async () => { await scriptRun(); };
  const onStop = async () => { await scriptStop(); };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>Script Editor (on-device bytecode)</Text>
      <Text style={styles.help}>
        Mnemonics: WAIT_MS WAIT_FRAMES CAPTURE INJECT REPLAY SENSOR_WRITE
        SENSOR_READ SET_MODE IF_TRIGGER TRIGGER_ARM TRIGGER_DISARM
        BLE_NOTIFY LOG LED MOTOR_PULSE LOOP END_LOOP JUMP HALT
      </Text>
      <ScrollView style={styles.scroll}>
        <TextInput
          style={styles.editor}
          multiline
          textAlignVertical="top"
          value={text}
          onChangeText={setText}
          fontFamily="monospace"
        />
      </ScrollView>
      <View style={styles.row}>
        <Button title="Upload" onPress={onUpload} />
        <Button title="Run"    color="#40a060" onPress={onRun} />
        <Button title="Stop"   color="#a04040" onPress={onStop} />
      </View>
      <Text style={styles.author}>author: jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#101418' },
  h1: { color: '#d0e0f0', fontSize: 16, fontWeight: '600', marginBottom: 4 },
  help: { color: '#708090', fontSize: 10, marginBottom: 8 },
  scroll: { flex: 1, borderColor: '#304050', borderWidth: 1, borderRadius: 4,
            marginBottom: 8 },
  editor: { color: '#d0e0f0', fontSize: 12, padding: 8, minHeight: 260 },
  row: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  author: { color: '#5a7088', fontSize: 10, textAlign: 'center' },
});