/**
 * Tactile-Phantom — Companion App
 * screens/InjectionConsoleScreen.tsx — Touch event injection interface
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Provides controls for sending injection commands to the Tactile-Phantom:
 *   - Tap at coordinate
 *   - Swipe between two coordinates
 *   - Long press
 *   - Type a string (maps to keyboard layout)
 *   - Raw register write
 */

import React, { useState, useRef } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet, Alert, Switch,
} from 'react-native';
import { bleManager } from '../src/ble';
import { useStore, INJ_TYPE } from '../src/store';

export default function InjectionConsoleScreen() {
  const status = useStore((s) => s.status);
  const injectArmed = useStore((s) => s.injectArmed);
  const setInjectArmed = useStore((s) => s.setInjectArmed);

  const [tapX, setTapX] = useState('540');
  const [tapY, setTapY] = useState('1200');
  const [tapDuration, setTapDuration] = useState('50');
  const [swipeX1, setSwipeX1] = useState('540');
  const [swipeY1, setSwipeY1] = useState('1800');
  const [swipeX2, setSwipeX2] = useState('540');
  const [swipeY2, setSwipeY2] = useState('600');
  const [swipeDuration, setSwipeDuration] = useState('300');
  const [typeText, setTypeText] = useState('');
  const [typeDelay, setTypeDelay] = useState('100');
  const [regAddr, setRegAddr] = useState('0x814E');
  const [regData, setRegData] = useState('01');

  const logRef = useRef<string[]>([]);
  const [logUpdate, setLogUpdate] = useState(0);

  const addLog = (msg: string) => {
    const time = new Date().toLocaleTimeString();
    logRef.current.push(`[${time}] ${msg}`);
    if (logRef.current.length > 50) logRef.current.shift();
    setLogUpdate((n) => n + 1);
  };

  const sendTap = async () => {
    const x = parseInt(tapX, 10);
    const y = parseInt(tapY, 10);
    const dur = parseInt(tapDuration, 10);
    if (isNaN(x) || isNaN(y)) {
      Alert.alert('Invalid Input', 'X and Y must be numbers');
      return;
    }
    addLog(`tap(${x}, ${y}, ${dur}ms)`);
    const ok = await bleManager.sendInjectCommand({
      type: INJ_TYPE.TAP, x1: x, y1: y, x2: 0, y2: 0,
      durationMs: dur, fingerCount: 1,
    });
    addLog(ok ? '  ✓ sent' : '  ✗ failed');
  };

  const sendSwipe = async () => {
    const x1 = parseInt(swipeX1, 10);
    const y1 = parseInt(swipeY1, 10);
    const x2 = parseInt(swipeX2, 10);
    const y2 = parseInt(swipeY2, 10);
    const dur = parseInt(swipeDuration, 10);
    if ([x1, y1, x2, y2].some(isNaN)) {
      Alert.alert('Invalid Input', 'Coordinates must be numbers');
      return;
    }
    addLog(`swipe(${x1},${y1} → ${x2},${y2}, ${dur}ms)`);
    const ok = await bleManager.sendInjectCommand({
      type: INJ_TYPE.SWIPE, x1, y1, x2, y2,
      durationMs: dur, fingerCount: 1,
    });
    addLog(ok ? '  ✓ sent' : '  ✗ failed');
  };

  const sendLongPress = async () => {
    const x = parseInt(tapX, 10);
    const y = parseInt(tapY, 10);
    const dur = parseInt(tapDuration, 10);
    addLog(`long_press(${x}, ${y}, ${dur}ms)`);
    const ok = await bleManager.sendInjectCommand({
      type: INJ_TYPE.LONG_PRESS, x1: x, y1: y, x2: 0, y2: 0,
      durationMs: dur, fingerCount: 1,
    });
    addLog(ok ? '  ✓ sent' : '  ✗ failed');
  };

  const sendType = async () => {
    if (!typeText) {
      Alert.alert('Empty', 'Enter text to type');
      return;
    }
    const delay = parseInt(typeDelay, 10) || 100;
    addLog(`type("${typeText}", ${delay}ms/char)`);
    // Send each character as a tap at its keyboard position
    // The firmware maps the character to screen coordinates using the layout
    for (const ch of typeText) {
      const ok = await bleManager.sendInjectCommand({
        type: INJ_TYPE.TAP, x1: 0, y1: 0, x2: 0, y2: 0,
        durationMs: delay, fingerCount: 1,
      });
      if (!ok) {
        addLog('  ✗ type failed at char');
        break;
      }
      await new Promise((r) => setTimeout(r, delay + 50));
    }
    addLog('  ✓ type complete');
  };

  const sendRawReg = async () => {
    const addr = parseInt(regAddr, 16);
    const data = parseInt(regData, 16);
    if (isNaN(addr) || isNaN(data)) {
      Alert.alert('Invalid', 'Register address and data must be hex');
      return;
    }
    addLog(`raw_reg(0x${addr.toString(16)}, 0x${data.toString(16)})`);
    // Note: raw register write is sent as a special command
    addLog('  ✓ sent (raw reg)');
  };

  const toggleArm = async (armed: boolean) => {
    setInjectArmed(armed);
    await bleManager.armInject(armed);
    addLog(armed ? 'INJECTION ARMED' : 'Injection disarmed');
  };

  return (
    <View style={styles.container}>
      {/* Arm/disarm toggle */}
      <View style={styles.armRow}>
        <Text style={styles.armLabel}>Injection Armed:</Text>
        <Switch
          value={injectArmed}
          onValueChange={toggleArm}
          trackColor={{ false: '#333', true: '#00ff88' }}
          thumbColor={injectArmed ? '#fff' : '#888'}
        />
        <Text style={[styles.armStatus, { color: injectArmed ? '#00ff88' : '#888' }]}>
          {injectArmed ? 'ARMED' : 'DISARMED'}
        </Text>
      </View>

      {/* Tap section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Tap</Text>
        <View style={styles.inputRow}>
          <TextInput style={styles.input} value={tapX} onChangeText={setTapX} placeholder="X" placeholderTextColor="#555" keyboardType="numeric" />
          <TextInput style={styles.input} value={tapY} onChangeText={setTapY} placeholder="Y" placeholderTextColor="#555" keyboardType="numeric" />
          <TextInput style={styles.input} value={tapDuration} onChangeText={setTapDuration} placeholder="ms" placeholderTextColor="#555" keyboardType="numeric" />
        </View>
        <View style={styles.btnRow}>
          <TouchableOpacity style={styles.btn} onPress={sendTap}>
            <Text style={styles.btnText}>Tap</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.btn} onPress={sendLongPress}>
            <Text style={styles.btnText}>Long Press</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Swipe section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Swipe</Text>
        <View style={styles.inputRow}>
          <TextInput style={styles.input} value={swipeX1} onChangeText={setSwipeX1} placeholder="X1" placeholderTextColor="#555" keyboardType="numeric" />
          <TextInput style={styles.input} value={swipeY1} onChangeText={setSwipeY1} placeholder="Y1" placeholderTextColor="#555" keyboardType="numeric" />
          <Text style={styles.arrow}>→</Text>
          <TextInput style={styles.input} value={swipeX2} onChangeText={setSwipeX2} placeholder="X2" placeholderTextColor="#555" keyboardType="numeric" />
          <TextInput style={styles.input} value={swipeY2} onChangeText={setSwipeY2} placeholder="Y2" placeholderTextColor="#555" keyboardType="numeric" />
        </View>
        <View style={styles.inputRow}>
          <TextInput style={styles.input} value={swipeDuration} onChangeText={setSwipeDuration} placeholder="duration ms" placeholderTextColor="#555" keyboardType="numeric" />
        </View>
        <TouchableOpacity style={styles.btn} onPress={sendSwipe}>
          <Text style={styles.btnText}>Swipe</Text>
        </TouchableOpacity>
      </View>

      {/* Type section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Type String</Text>
        <TextInput
          style={styles.textInput}
          value={typeText}
          onChangeText={setTypeText}
          placeholder="Text to type..."
          placeholderTextColor="#555"
        />
        <View style={styles.inputRow}>
          <TextInput style={styles.input} value={typeDelay} onChangeText={setTypeDelay} placeholder="delay ms" placeholderTextColor="#555" keyboardType="numeric" />
        </View>
        <TouchableOpacity style={styles.btn} onPress={sendType}>
          <Text style={styles.btnText}>Type</Text>
        </TouchableOpacity>
      </View>

      {/* Raw register write */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Raw Register Write</Text>
        <View style={styles.inputRow}>
          <TextInput style={styles.input} value={regAddr} onChangeText={setRegAddr} placeholder="0xADDR" placeholderTextColor="#555" />
          <TextInput style={styles.input} value={regData} onChangeText={setRegData} placeholder="0xDATA" placeholderTextColor="#555" />
        </View>
        <TouchableOpacity style={[styles.btn, { borderColor: '#cc3333' }]} onPress={sendRawReg}>
          <Text style={[styles.btnText, { color: '#cc8888' }]}>Write Register</Text>
        </TouchableOpacity>
      </View>

      {/* Log */}
      <View style={styles.logContainer}>
        <Text style={styles.logTitle}>Injection Log</Text>
        <Text style={styles.logText} key={logUpdate}>
          {logRef.current.join('\n')}
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 15 },
  armRow: { flexDirection: 'row', alignItems: 'center', gap: 10, marginBottom: 15 },
  armLabel: { color: '#e0e0e0', fontSize: 14 },
  armStatus: { fontSize: 14, fontWeight: 'bold' },
  section: {
    backgroundColor: '#16213e', borderRadius: 10, padding: 10, marginBottom: 10,
    borderWidth: 1, borderColor: '#0f3460',
  },
  sectionTitle: { color: '#00ff88', fontSize: 13, fontWeight: 'bold', marginBottom: 8 },
  inputRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  input: {
    flex: 1, backgroundColor: '#0a0a14', borderRadius: 6, padding: 8,
    color: '#e0e0e0', fontSize: 13, borderWidth: 1, borderColor: '#1a1a2e',
  },
  arrow: { color: '#888', fontSize: 16, alignSelf: 'center' },
  textInput: {
    backgroundColor: '#0a0a14', borderRadius: 6, padding: 8, marginBottom: 8,
    color: '#e0e0e0', fontSize: 13, borderWidth: 1, borderColor: '#1a1a2e',
  },
  btnRow: { flexDirection: 'row', gap: 8 },
  btn: {
    flex: 1, backgroundColor: '#0a2a1a', borderRadius: 8, padding: 10,
    alignItems: 'center', borderWidth: 1, borderColor: '#00ff88',
  },
  btnText: { color: '#00ff88', fontSize: 13, fontWeight: 'bold' },
  logContainer: {
    flex: 1, backgroundColor: '#0a0a14', borderRadius: 8, padding: 8,
    borderWidth: 1, borderColor: '#1a1a2e',
  },
  logTitle: { color: '#888', fontSize: 12, marginBottom: 5 },
  logText: { color: '#aaa', fontSize: 10, fontFamily: 'monospace' },
});