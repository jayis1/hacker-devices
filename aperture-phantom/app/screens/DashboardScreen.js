/**
 * AperturePhantom — DashboardScreen
 *
 * Shows the live device status (mode, armed, frame/crc counters, battery,
 * SD free space) and offers mode switching, arm/disarm, and a quick
 * navigation hub to the other screens.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useEffect } from 'react';
import { View, Text, Button, StyleSheet, TouchableOpacity } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { MODE } from '../utils/protocol';

const MODE_NAMES = ['PASSTHROUGH', 'CAPTURE', 'INJECT', 'REPLAY', 'FUZZ'];
const MODE_COLORS = ['#60a060', '#6060e0', '#e06060', '#e060e0', '#e0e060'];

export default function DashboardScreen({ navigation }) {
  const { status, linkInfo, arm, setMode, log, send } = useDevice();

  useEffect(() => {
    /* refresh status every 2 s */
    const t = setInterval(() => send(0x02 /* GET_STATUS */, null), 2000);
    return () => clearInterval(t);
  }, [send]);

  const modeName = status ? MODE_NAMES[status.mode] : '—';
  const modeColor = status ? MODE_COLORS[status.mode] : '#607080';

  return (
    <View style={styles.container}>
      <View style={styles.statusCard}>
        <View style={[styles.modePill, { backgroundColor: modeColor }]}>
          <Text style={styles.modeText}>{modeName}</Text>
        </View>
        <Text style={styles.armedText}>
          {status && status.armed ? '● ARMED' : '○ DISARMED'}
        </Text>
        {status && (
          <View style={styles.grid}>
            <Stat label="Frames" value={status.frameCount} />
            <Stat label="CRC errs" value={status.crcErrors} />
            <Stat label="Status" value={'0x' + (status.status >>> 0).toString(16)} />
          </View>
        )}
        {linkInfo && (
          <View style={styles.grid}>
            <Stat label="Lanes" value={linkInfo.lanes} />
            <Stat label="Rate" value={(linkInfo.rateMbps || 0) + ' Mbps'} />
            <Stat label="DT" value={'0x' + (linkInfo.dataType || 0).toString(16)} />
            <Stat label="Sensor" value={'0x' + (linkInfo.sensorAddr || 0).toString(16)} />
            <Stat label="Battery" value={(linkInfo.battery || 0) + '%'} />
            <Stat label="SD free" value={(linkInfo.sdFreeKB || 0) + ' KB'} />
          </View>
        )}
      </View>

      <View style={styles.actions}>
        <Button
          title={status && status.armed ? 'Disarm' : 'Arm'}
          color={status && status.armed ? '#a04040' : '#40a060'}
          onPress={() => arm(!(status && status.armed))}
        />
      </View>

      <View style={styles.modeRow}>
        {MODE_NAMES.map((m, i) => (
          <TouchableOpacity key={m} onPress={() => setMode(i)}
            style={[styles.modeBtn,
                   status && status.mode === i && styles.modeBtnActive]}>
            <Text style={styles.modeBtnLabel}>{m}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.navGrid}>
        <NavBtn title="Live View" onPress={() => navigation.navigate('LiveView')} />
        <NavBtn title="Replay"    onPress={() => navigation.navigate('Replay')} />
        <NavBtn title="Inject"    onPress={() => navigation.navigate('Inject')} />
        <NavBtn title="Script"    onPress={() => navigation.navigate('Script')} />
        <NavBtn title="Sensor"    onPress={() => navigation.navigate('Sensor')} />
        <NavBtn title="Fuzzer"    onPress={() => navigation.navigate('Fuzzer')} />
      </View>

      <Text style={styles.author}>AperturePhantom · author: jayis1 · GPL-2.0</Text>
    </View>
  );
}

function Stat({ label, value }) {
  return (
    <View style={styles.statBox}>
      <Text style={styles.statLabel}>{label}</Text>
      <Text style={styles.statValue}>{String(value)}</Text>
    </View>
  );
}

function NavBtn({ title, onPress }) {
  return (
    <TouchableOpacity style={styles.navBtn} onPress={onPress}>
      <Text style={styles.navLabel}>{title}</Text>
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 12, backgroundColor: '#101418' },
  statusCard: { backgroundColor: '#181d24', borderRadius: 8, padding: 12,
                alignItems: 'center', marginBottom: 12 },
  modePill: { paddingHorizontal: 16, paddingVertical: 6, borderRadius: 12,
              marginBottom: 6 },
  modeText: { color: '#0a0c10', fontWeight: '700', fontSize: 14 },
  armedText: { color: '#d0e0f0', fontSize: 13, marginBottom: 8 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'center',
          gap: 8, marginVertical: 6 },
  statBox: { backgroundColor: '#202830', borderRadius: 6, padding: 8,
              minWidth: 80, alignItems: 'center' },
  statLabel: { color: '#8aa0b8', fontSize: 10 },
  statValue: { color: '#d0e0f0', fontSize: 13, fontWeight: '600', marginTop: 2 },
  actions: { marginVertical: 8 },
  modeRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6, marginBottom: 12 },
  modeBtn: { backgroundColor: '#202830', paddingHorizontal: 10, paddingVertical: 6,
              borderRadius: 6 },
  modeBtnActive: { backgroundColor: '#405060' },
  modeBtnLabel: { color: '#d0e0f0', fontSize: 11, fontWeight: '600' },
  navGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8 },
  navBtn: { backgroundColor: '#283038', padding: 14, borderRadius: 8,
            minWidth: 100, alignItems: 'center' },
  navLabel: { color: '#a0b0c0', fontSize: 13, fontWeight: '600' },
  author: { color: '#5a7088', fontSize: 10, marginTop: 16, textAlign: 'center' },
});