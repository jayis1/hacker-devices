/**
 * RelayScreen.tsx — UWB relay attack test bench control.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The relay is the canonical test of UWB anti-relay claims.  This screen
 * exposes the three modes (transparent, shrink, jitter), the target
 * distance to shrink to, and an arming button that requires confirmation.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, Alert } from 'react-native';
import { Button, SegmentedButtons, Slider, TextInput } from 'react-native-paper';
import { ble, UwbEvent } from '../ble/UwbPhantomBle';

type Mode = 'transparent' | 'shrink' | 'jitter';

export default function RelayScreen(): JSX.Element {
  const [mode, setMode]           = useState<Mode>('shrink');
  const [targetCm, setTargetCm]   = useState(5);
  const [delayMs, setDelayMs]     = useState(1);
  const [jitterUs, setJitterUs]   = useState(0);
  const [armed, setArmed]         = useState(false);
  const [forwarded, setForwarded] = useState(0);
  const [dropped, setDropped]     = useState(0);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'relay_stats') {
        setForwarded(e.forwarded); setDropped(e.dropped);
      }
    });
    return () => unsub();
  }, []);

  const arm = () => {
    Alert.alert(
      'Confirm relay arm',
      `Mode: ${mode}\nTarget: ${targetCm} cm\nDelay: ${delayMs} ms\nJitter: ±${jitterUs} µs\n\n` +
      `Ensure you are authorised for the target. Proceed?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Arm', style: 'destructive', onPress: doArm },
      ]);
  };

  const doArm = async () => {
    await ble.sendCmd({
      v: 'relay_arm', mode, target_cm: targetCm,
      delay_ms: delayMs, jitter_us: jitterUs,
    });
    setArmed(true);
  };

  const disarm = async () => {
    await ble.sendCmd({ v: 'relay_disarm' });
    setArmed(false);
  };

  return (
    <View style={styles.root}>
      <Text style={styles.title}>UWB Relay Engine</Text>
      <Text style={styles.warn}>⚠ Authorized targets only. Relay attacks on systems you do not own may be illegal.</Text>
      <Text style={styles.label}>Mode</Text>
      <SegmentedButtons value={mode} onValueChange={v => setMode(v as Mode)}
        buttons={[
          { value: 'transparent', label: 'Transparent' },
          { value: 'shrink',      label: 'Shrink' },
          { value: 'jitter',      label: 'Jitter' },
        ]} />
      <Text style={styles.label}>Target distance: {targetCm} cm</Text>
      <Slider value={targetCm} onValueChange={v => setTargetCm(Math.round(v))}
        minimumValue={0} maximumValue={50} step={1} />
      <Text style={styles.label}>Forwarding delay: {delayMs} ms</Text>
      <Slider value={delayMs} onValueChange={v => setDelayMs(Math.round(v))}
        minimumValue={0} maximumValue={50} step={1} />
      <Text style={styles.label}>Jitter: ±{jitterUs} µs</Text>
      <Slider value={jitterUs} onValueChange={v => setJitterUs(Math.round(v))}
        minimumValue={0} maximumValue={500} step={10} />
      <View style={styles.buttons}>
        {!armed ? (
          <Button mode="contained" onPress={arm} buttonColor="#d32f2f">Arm relay</Button>
        ) : (
          <Button mode="contained" onPress={disarm} buttonColor="#388e3c">Disarm</Button>
        )}
      </View>
      <Text style={styles.stats}>Forwarded: {forwarded}  Dropped: {dropped}</Text>
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  warn:    { color: '#ffab40', fontSize: 11, marginBottom: 16 },
  label:   { color: '#90a4ae', fontSize: 12, marginTop: 12, marginBottom: 4 },
  buttons: { marginTop: 24, alignItems: 'center' },
  stats:   { color: '#b0bec5', fontSize: 13, marginTop: 24, fontFamily: 'monospace' },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 16 },
});