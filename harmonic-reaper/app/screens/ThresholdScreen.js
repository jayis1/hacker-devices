/**
 * ThresholdScreen.js — TX power, ratio thresholds, pulse mode, quiet mode
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, Slider, Switch, Button, Picker } from 'react-native';
import { OPCODES, MODES } from '../utils/protocol';

export default function ThresholdScreen({ ble }) {
  const [txPower, setTxPower] = useState(0);
  const [semiThresh, setSemiThresh] = useState(6);
  const [metalThresh, setMetalThresh] = useState(-6);
  const [pulsePrf, setPulsePrf] = useState(4000);
  const [pulseWidth, setPulseWidth] = useState(200);
  const [quietMode, setQuietMode] = useState(true);
  const [mode, setMode] = useState(MODES.SWEEP_PULSED);

  const applyMode = (m) => {
    setMode(m);
    ble.sendCommand(OPCODES.CMD_SET_MODE, [m]);
  };

  const applyTxPower = (v) => {
    setTxPower(v);
    ble.sendCommand(OPCODES.CMD_SET_TX_POWER, [v & 0xFF]);
  };

  const applyPulse = () => {
    ble.sendCommand(OPCODES.CMD_SET_PULSE, [pulsePrf & 0xFF, (pulsePrf >> 8) & 0xFF, pulseWidth & 0xFF]);
  };

  const applyThresh = () => {
    ble.sendCommand(OPCODES.CMD_SET_THRESH, [semiThresh & 0xFF, metalThresh & 0xFF]);
  };

  const applyQuiet = (v) => {
    setQuietMode(v);
    ble.sendCommand(OPCODES.CMD_SET_QUIET, [v ? 1 : 0]);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Sweep Parameters</Text>

      <View style={styles.section}>
        <Text style={styles.label}>Operating Mode</Text>
        <Picker
          selectedValue={mode}
          onValueChange={applyMode}
          style={styles.picker}
        >
          <Picker.Item label="Idle (TX off)" value={MODES.IDLE} />
          <Picker.Item label="Quiet RX (passive scan)" value={MODES.QUIET_RX} />
          <Picker.Item label="CW Sweep" value={MODES.SWEEP_CW} />
          <Picker.Item label="Pulsed Sweep (default)" value={MODES.SWEEP_PULSED} />
          <Picker.Item label="Calibrate" value={MODES.CALIBRATE} />
        </Picker>
      </View>

      <View style={styles.section}>
        <Text style={styles.label}>TX Power: {txPower} dBm (−10 to +15)</Text>
        <Slider
          minimumValue={-10}
          maximumValue={15}
          step={1}
          value={txPower}
          onSlidingComplete={applyTxPower}
          minimumTrackTintColor="#00ff88"
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.label}>Semiconductor Threshold: +{semiThresh} dB</Text>
        <Slider
          minimumValue={0}
          maximumValue={20}
          step={1}
          value={semiThresh}
          onSlidingComplete={(v) => { setSemiThresh(v); }}
          minimumTrackTintColor="#ff3333"
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.label}>Dissimilar Metal Threshold: {metalThresh} dB</Text>
        <Slider
          minimumValue={-20}
          maximumValue={0}
          step={1}
          value={metalThresh}
          onSlidingComplete={(v) => { setMetalThresh(v); }}
          minimumTrackTintColor="#888888"
        />
      </View>
      <Button title="Apply Thresholds" onPress={applyThresh} color="#00ff88" />

      <View style={styles.section}>
        <Text style={styles.label}>Pulse PRF: {pulsePrf} Hz</Text>
        <Slider
          minimumValue={1000}
          maximumValue={10000}
          step={500}
          value={pulsePrf}
          onSlidingComplete={(v) => setPulsePrf(v)}
          minimumTrackTintColor="#00aaff"
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.label}>Pulse Width: {pulseWidth} ns</Text>
        <Slider
          minimumValue={50}
          maximumValue={10000}
          step={50}
          value={pulseWidth}
          onSlidingComplete={(v) => setPulseWidth(v)}
          minimumTrackTintColor="#00aaff"
        />
      </View>
      <Button title="Apply Pulse Settings" onPress={applyPulse} color="#00aaff" />

      <View style={styles.section}>
        <View style={styles.row}>
          <Text style={styles.label}>Quiet Mode (randomized PRF)</Text>
          <Switch value={quietMode} onValueChange={applyQuiet} trackColor={{ true: '#00ff88' }} />
        </View>
        <Text style={styles.hint}>
          Spreads the spectral signature so an adversary's RF activity monitor
          sees noise rather than a stable pulse train.
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
    padding: 16,
  },
  title: {
    color: '#00ff88',
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  section: {
    marginBottom: 20,
  },
  label: {
    color: '#aaa',
    fontSize: 13,
    marginBottom: 4,
  },
  picker: {
    color: '#fff',
    backgroundColor: '#1a1a2e',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  hint: {
    color: '#555',
    fontSize: 10,
    marginTop: 4,
  },
});