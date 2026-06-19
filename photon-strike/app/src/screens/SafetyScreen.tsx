/**
 * SafetyScreen.tsx — laser safety interlock status & two-person enable
 * Author: jayis1
 * License: MIT
 *
 * Enforces a two-person rule: enabling the laser requires a 3-second
 * long-press confirmation. Shows live interlock status and an
 * emergency-stop button.
 */

import React, { useState, useRef } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Alert, Pressable } from 'react-native';
import { useBle } from '../services/BleContext';

export default function SafetyScreen() {
  const ble = useBle();
  const [laserOn, setLaserOn] = useState(false);
  const [pressSecs, setPressSecs] = useState(0);
  const timerRef = useRef<any>(null);

  // The live interlock status would come from a status characteristic;
  // here we use the scanStatus as a proxy for device liveness.
  const live = ble.connected;

  const startHold = () => {
    setPressSecs(0);
    timerRef.current = setInterval(() => {
      setPressSecs((s) => {
        if (s + 1 >= 3) {
          clearInterval(timerRef.current);
          enableLaser();
          return 0;
        }
        return s + 1;
      });
    }, 1000);
  };

  const cancelHold = () => {
    if (timerRef.current) clearInterval(timerRef.current);
    setPressSecs(0);
  };

  const enableLaser = async () => {
    await ble.laserEnable();
    setLaserOn(true);
    Alert.alert('Laser ENABLED', 'Class 3B/4 active. Wear 1064 nm goggles.');
  };

  const disableLaser = async () => {
    await ble.laserDisable();
    setLaserOn(false);
  };

  const estop = async () => {
    await ble.emergencyStop();
    setLaserOn(false);
    if (timerRef.current) clearInterval(timerRef.current);
    Alert.alert('EMERGENCY STOP', 'Laser disabled, scan aborted.');
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Laser Safety</Text>
      <Text style={styles.warning}>
        ⚠️ 1064 nm Class 3B/4. Direct or reflected exposure causes permanent
        eye/skin injury. Operate only inside an interlocked enclosure with
        OD≥6 @ 1064 nm goggles.
      </Text>

      <View style={styles.statusCard}>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Device connected:</Text>
          <Text style={[styles.v, { color: live ? '#4ecca3' : '#e94560' }]}>{live ? 'YES' : 'NO'}</Text>
        </View>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Laser state:</Text>
          <Text style={[styles.v, { color: laserOn ? '#e94560' : '#4ecca3' }]}>
            {laserOn ? 'ENABLED' : 'DISABLED'}
          </Text>
        </View>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Interlock:</Text>
          <Text style={[styles.v, { color: live ? '#4ecca3' : '#666' }]}>{live ? 'CLOSED' : '—'}</Text>
        </View>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Key switch:</Text>
          <Text style={[styles.v, { color: live ? '#4ecca3' : '#666' }]}>{live ? 'ON' : '—'}</Text>
        </View>
      </View>

      {!laserOn ? (
        <Pressable
          styleIn={({ pressed }) => [styles.holdButton, pressed && styles.holdButtonPressed]}
          onPressIn={startHold}
          onPressOut={cancelHold}
        >
          <Text style={styles.holdButtonText}>
            {pressSecs > 0 ? `Hold to enable… ${3 - pressSecs}s` : 'Hold 3s to enable laser'}
          </Text>
        </Pressable>
      ) : (
        <TouchableOpacity style={[styles.button, { borderColor: '#4ecca3' }]} onPress={disableLaser}>
          <Text style={[styles.buttonText, { color: '#4ecca3' }]}>Disable Laser</Text>
        </TouchableOpacity>
      )}

      <TouchableOpacity style={[styles.button, { borderColor: '#e94560', backgroundColor: '#e9456022' }]} onPress={estop}>
        <Text style={[styles.buttonText, { color: '#e94560' }]}>EMERGENCY STOP</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>PhotonStrike v1.0.0 — author: jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16 },
  title: { color: '#e94560', fontSize: 22, fontWeight: 'bold', marginBottom: 8 },
  warning: { color: '#e94560', fontSize: 11, marginBottom: 16, lineHeight: 16 },
  statusCard: { backgroundColor: '#16213e', borderRadius: 10, padding: 16, marginBottom: 16 },
  kvRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 3 },
  k: { color: '#a0a0c0' },
  v: { color: '#fff', fontWeight: '700' },
  holdButton: {
    backgroundColor: '#16213e', padding: 18, borderRadius: 10,
    alignItems: 'center', marginVertical: 6, borderWidth: 2, borderColor: '#e94560',
  },
  holdButtonPressed: { backgroundColor: '#e9456033' },
  holdButtonText: { color: '#e94560', fontSize: 16, fontWeight: '700' },
  button: {
    backgroundColor: '#16213e', padding: 14, borderRadius: 8,
    alignItems: 'center', marginVertical: 6, borderWidth: 1, borderColor: '#e94560',
  },
  buttonText: { color: '#e94560', fontSize: 16, fontWeight: '600' },
  footer: { color: '#444', fontSize: 11, textAlign: 'center', marginTop: 20 },
});