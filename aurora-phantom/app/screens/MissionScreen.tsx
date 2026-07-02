// screens/MissionScreen.tsx — compose and arm a capture mission
// Author: jayis1   License: GPL-2.0
import React from 'react';
import { View, Text, TextInput, TouchableOpacity, Switch, StyleSheet, ScrollView } from 'react-native';
import { useStore } from '../src/store';
import { ble } from '../src/ble';

export default function MissionScreen() {
  const { mission, setMission, connected } = useStore();

  const arm = async () => {
    const json = JSON.stringify({
      lo_freq_hz: mission.loFreqHz,
      int_window_us: mission.intWindowUs,
      frame_period_us: mission.framePeriodUs,
      duration_s: mission.durationS,
      wavelength_nm: mission.wavelengthNm,
      bias_trim: mission.biasTrim,
      tdc_res: mission.tdcRes,
      exfil_policy: mission.exfilPolicy,
      stream_events: mission.streamEvents ? 1 : 0,
      name: mission.name,
    });
    if (connected) {
      await ble.sendCommand(json);
      useStore.getState().pushLog('MISS', 'armed: ' + mission.name);
    }
  };

  return (
    <ScrollView style={s.container}>
      <Text style={s.title}>Mission Composer</Text>
      <Field label="Mission name" value={mission.name}
        onChange={(v) => setMission({ name: v })} />
      <NumField label="LO freq (Hz, 0=auto)" value={String(mission.loFreqHz)}
        onChange={(v) => setMission({ loFreqHz: +v })} />
      <NumField label="Integration window (µs)" value={String(mission.intWindowUs)}
        onChange={(v) => setMission({ intWindowUs: +v })} />
      <NumField label="Frame period (µs)" value={String(mission.framePeriodUs)}
        onChange={(v) => setMission({ framePeriodUs: +v })} />
      <NumField label="Duration (s)" value={String(mission.durationS)}
        onChange={(v) => setMission({ durationS: +v })} />
      <NumField label="Bandpass (nm)" value={String(mission.wavelengthNm)}
        onChange={(v) => setMission({ wavelengthNm: +v })} />
      <NumField label="SPAD bias trim (0-4095)" value={String(mission.biasTrim)}
        onChange={(v) => setMission({ biasTrim: +v })} />
      <NumField label="TDC resolution (0,1,2)" value={String(mission.tdcRes)}
        onChange={(v) => setMission({ tdcRes: +v })} />
      <View style={s.row}>
        <Text style={s.label}>Stream raw events to SD</Text>
        <Switch value={mission.streamEvents}
          onValueChange={(v) => setMission({ streamEvents: v })} />
      </View>
      <View style={s.row}>
        <Text style={s.label}>Exfil policy (0=rendezvous, 1=live)</Text>
        <Switch value={mission.exfilPolicy === 1}
          onValueChange={(v) => setMission({ exfilPolicy: v ? 1 : 0 })} />
      </View>
      <TouchableOpacity style={s.btn} onPress={arm}>
        <Text style={s.btnText}>Arm mission</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

function Field({ label, value, onChange }: { label: string; value: string; onChange: (v: string) => void }) {
  return (
    <View style={s.field}>
      <Text style={s.label}>{label}</Text>
      <TextInput style={s.input} value={value} onChangeText={onChange}
        placeholderTextColor="#555" />
    </View>
  );
}
function NumField({ label, value, onChange }: { label: string; value: string; onChange: (v: string) => void }) {
  return <Field label={label} value={value} onChange={onChange} />;
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#7fd4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 16 },
  field: { marginBottom: 14 },
  label: { color: '#888', fontSize: 13, marginBottom: 4 },
  input: { backgroundColor: '#141422', color: '#ddd', borderRadius: 6, padding: 10, fontSize: 15 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 10 },
  btn: { backgroundColor: '#1a3a5c', padding: 14, borderRadius: 8, marginTop: 20, alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
});