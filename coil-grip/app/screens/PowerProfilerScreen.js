/*
 * PowerProfilerScreen.js — Live power-profile trace & device fingerprinting
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import { Card, Title, Paragraph, Button } from 'react-native-paper';
import { LineChart } from 'react-native-chart-kit';
import { useDevice } from '../components/DeviceContext';

const screenWidth = Dimensions.get('window').width;

export default function PowerProfilerScreen() {
  const { connected, sendCommand, telemetry, log } = useDevice();
  const [trace, setTrace] = useState(Array(30).fill(0));
  const [matchName, setMatchName] = useState('—');
  const [conf, setConf] = useState(0);

  // Poll classifier result
  useEffect(() => {
    if (!connected) return;
    const id = setInterval(() => { sendCommand('profiler class\n'); }, 1500);
    return () => clearInterval(id);
  }, [connected, sendCommand]);

  // Parse log for classifier result
  useEffect(() => {
    const last = [...log].reverse().find(l => l.msg && l.msg.startsWith('state='));
    if (last) {
      const m = last.msg.match(/state=(\d+) conf=(\d+)/);
      if (m) { setMatchName(`fingerprint #${m[1]}`); setConf(parseInt(m[2])/1000); }
    }
    // Also feed current into the trace
    setTrace((t) => [...t.slice(1), telemetry.currentA]);
  }, [log, telemetry.currentA]);

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Power Profiler</Title>
          <Paragraph>Live current draw from the Qi field</Paragraph>
          <Paragraph>Instant: {telemetry.currentA.toFixed(3)} A</Paragraph>
          <Paragraph>Coil temp: {telemetry.coilTempC} °C</Paragraph>
          <Button mode="contained" onPress={() => sendCommand('mode profile\n')}>Start profiling</Button>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Live trace</Title>
          <LineChart
            data={{ labels: trace.map((_,i)=>i%5===0?`${i}`:''), datasets:[{ data: trace }] }}
            width={screenWidth - 48}
            height={180}
            chartConfig={chartConfig}
            bezier
            style={styles.chart}
          />
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Classification</Title>
          <Paragraph>Best match: {matchName}</Paragraph>
          <Paragraph>Confidence: {(conf*100).toFixed(1)}%</Paragraph>
        </Card.Content>
      </Card>
    </View>
  );
}

const chartConfig = {
  backgroundGradientFrom: '#161b22',
  backgroundGradientTo: '#161b22',
  color: (opacity=1) => `rgba(88,166,255,${opacity})`,
  labelColor: (opacity=1) => `rgba(139,148,158,${opacity})`,
  strokeWidth: 2,
};

const styles = StyleSheet.create({
  container: { flex:1, padding:12, backgroundColor:'#0d1117' },
  card: { marginBottom:12, backgroundColor:'#161b22' },
  chart: { marginVertical:8, borderRadius:8 },
});