// screens/SettingsScreen.tsx — device settings + calibration
// Author: jayis1   License: GPL-2.0
import React from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useStore } from '../src/store';
import { ble } from '../src/ble';

export default function SettingsScreen() {
  const { connected } = useStore();
  const [tcxo, setTcxo] = React.useState('0.5');
  const [bandCal, setBandCal] = React.useState('550');

  const send = async (cmd: string) => {
    if (connected) await ble.sendCommand(cmd);
  };

  return (
    <ScrollView style={s.container}>
      <Text style={s.title}>Settings</Text>

      <Text style={s.section}>Device</Text>
      <Row label="Author" value="jayis1" />
      <Row label="Firmware" value="1.0.0" />
      <Row label="Board" value="aurora-phantom" />

      <Text style={s.section}>TCXO trim (ppm)</Text>
      <TextInput style={s.input} value={tcxo} onChangeText={setTcxo}
        placeholderTextColor="#555" keyboardType="numeric" />
      <TouchableOpacity style={s.btn} onPress={() => send(`{"cmd":"tcxo_trim","ppm":${tcxo}}`)}>
        <Text style={s.btnText}>Apply TCXO trim</Text>
      </TouchableOpacity>

      <Text style={s.section}>Optical bandpass calibration (nm)</Text>
      <TextInput style={s.input} value={bandCal} onChangeText={setBandCal}
        placeholderTextColor="#555" keyboardType="numeric" />
      <TouchableOpacity style={s.btn} onPress={() => send(`{"cmd":"bandpass_cal","nm":${bandCal}}`)}>
        <Text style={s.btnText}>Calibrate bandpass</Text>
      </TouchableOpacity>

      <Text style={s.section}>Diagnostics</Text>
      <TouchableOpacity style={s.btn} onPress={() => send('{"cmd":"regs"}')}>
        <Text style={s.btnText}>Dump FPGA registers</Text>
      </TouchableOpacity>
      <TouchableOpacity style={s.btn} onPress={() => send('{"cmd":"rate"}')}>
        <Text style={s.btnText}>Show aggregate rate</Text>
      </TouchableOpacity>
      <TouchableOpacity style={s.btnDanger} onPress={() => send('{"cmd":"standby"}')}>
        <Text style={s.btnText}>Force standby</Text>
      </TouchableOpacity>

      <Text style={s.legal}>
        ⚠ Aurora-Phantom is for authorized security research only. Interception
        of optical emanations from equipment you do not own may be illegal.
        Author: jayis1. License: GPL-2.0.
      </Text>
    </ScrollView>
  );
}

function Row({ label, value }: { label: string; value: string }) {
  return (
    <View style={s.row}>
      <Text style={s.label}>{label}</Text>
      <Text style={s.value}>{value}</Text>
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#7fd4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 16 },
  section: { color: '#7fd4ff', fontSize: 16, fontWeight: '600', marginTop: 16, marginBottom: 8 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  label: { color: '#888', fontSize: 14 },
  value: { color: '#ddd', fontSize: 14 },
  input: { backgroundColor: '#141422', color: '#ddd', borderRadius: 6, padding: 10, fontSize: 15 },
  btn: { backgroundColor: '#1a3a5c', padding: 12, borderRadius: 8, marginTop: 8, alignItems: 'center' },
  btnDanger: { backgroundColor: '#5c1a1a', padding: 12, borderRadius: 8, marginTop: 8, alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 15, fontWeight: '600' },
  legal: { color: '#885555', fontSize: 11, marginTop: 24, lineHeight: 16 },
});