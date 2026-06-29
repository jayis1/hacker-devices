/*
 * GlitchControlScreen.js — Contactless power-glitch injection controls
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, Alert } from 'react-native';
import { Card, Title, Paragraph, Button, TextInput, Divider, ToggleButton } from 'react-native-paper';
import { useDevice } from '../components/DeviceContext';

export default function GlitchControlScreen() {
  const { connected, sendCommand, telemetry } = useDevice();
  const [delay, setDelay] = useState('4200');   // µs
  const [width, setWidth] = useState('3');      // µs
  const [repeat, setRepeat] = useState('1');
  const [ramp, setRamp] = useState('0');
  const [trigger, setTrigger] = useState('timer');

  const arm = () => {
    const trigMap = { timer:TRIG_TIMER, gpio:TRIG_GPIO, qi:TRIG_QI, adc:TRIG_ADC };
    sendCommand(`glitch arm ${delay} ${width} ${repeat} ${ramp}\n`);
  };
  const fire = () => {
    Alert.alert(
      'Confirm glitch',
      `Fire a ${width}µs power drop after ${delay}µs?`,
      [
        { text:'Cancel', style:'cancel' },
        { text:'Fire', onPress: () => sendCommand('glitch fire\n'), style:'destructive' },
      ]
    );
  };
  const disarm = () => sendCommand('glitch disarm\n');

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Contactless Glitch</Title>
          <Paragraph style={styles.warn}>⚠ Drops target power. Authorized targets only.</Paragraph>
          <Paragraph>Mode: {telemetry.mode} · armed: {telemetry.glitchArmed ? 'yes' : 'no'}</Paragraph>
          <Paragraph>Fires: {telemetry.glitchFires}</Paragraph>
          <Divider style={styles.div}/>

          <Title style={styles.h2}>Trigger source</Title>
          <ToggleButton.Row onValueChange={setTrigger} value={trigger}>
            <ToggleButton icon="timer"   value="timer" />
            <ToggleButton icon="gpio"     value="gpio" />
            <ToggleButton icon="qi"      value="qi" />
            <ToggleButton icon="adc"     value="adc" />
          </ToggleButton.Row>
          <Text style={styles.help}>timer: delay-then-fire · gpio: wait on TRIG_IN · qi: match Qi packet · adc: current threshold</Text>

          <Divider style={styles.div}/>
          <View style={styles.row}>
            <TextInput label="Delay µs" value={delay} onChangeText={setDelay} keyboardType="numeric" style={styles.input}/>
            <TextInput label="Width µs" value={width} onChangeText={setWidth} keyboardType="numeric" style={styles.input}/>
          </View>
          <View style={styles.row}>
            <TextInput label="Repeat"  value={repeat} onChangeText={setRepeat} keyboardType="numeric" style={styles.input}/>
            <TextInput label="Ramp µs"  value={ramp}   onChangeText={setRamp}   keyboardType="numeric" style={styles.input}/>
          </View>
          <Divider style={styles.div}/>
          <View style={styles.row}>
            <Button mode="contained" onPress={arm}>Arm</Button>
            <Button mode="contained" onPress={fire} color="#d73a49">Fire</Button>
            <Button mode="outlined" onPress={disarm}>Disarm</Button>
          </View>
        </Card.Content>
      </Card>
    </View>
  );
}

const TRIG_TIMER='timer', TRIG_GPIO='gpio', TRIG_QI='qi', TRIG_ADC='adc';

const styles = StyleSheet.create({
  container: { flex:1, padding:12, backgroundColor:'#0d1117' },
  card: { marginBottom:12, backgroundColor:'#161b22' },
  row: { flexDirection:'row', alignItems:'center', marginVertical:6, gap:8 },
  input: { flex:1, backgroundColor:'#0d1117', maxWidth:120 },
  div: { marginVertical:8 },
  h2: { fontSize:14, color:'#8b949e' },
  warn: { color:'#d73a49', marginBottom:8 },
  help: { color:'#8b949e', fontSize:11, marginTop:4 },
});