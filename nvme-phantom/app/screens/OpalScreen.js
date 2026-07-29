/**
 * screens/OpalScreen.js — TCG Opal security command console
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';

export default function OpalScreen({ ble }) {
  const [password, setPassword] = useState('');
  const [psid, setPsid] = useState('');
  const [log, setLog] = useState([]);
  const [busy, setBusy] = useState(false);

  const addLog = (msg) => {
    setLog(prev => [...prev, { time: new Date().toLocaleTimeString(), msg }]);
  };

  const handleUnlock = async () => {
    if (!password) { Alert.alert('Enter password'); return; }
    setBusy(true);
    addLog('Sending ENTER_LEARNED_MODE (UNLOCK)...');
    try {
      await ble.opalSend(password);
      addLog('Unlock command sent. Monitor capture for response.');
    } catch (e) { addLog('Error: ' + e.message); }
    setBusy(false);
  };

  const handleRevert = async () => {
    Alert.alert(
      '⚠️ DANGER: Revert SED',
      'This will REVERT the SED to factory defaults, wiping the Data Encryption Key. ALL DATA WILL BE PERMANENTLY LOST. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'REVERT (destructive)', style: 'destructive', onPress: async () => {
          setBusy(true);
          addLog('Sending REVERT (destructive)...');
          try {
            await ble.opalSend(psid || password);
            addLog('Revert command sent.');
          } catch (e) { addLog('Error: ' + e.message); }
          setBusy(false);
        }}
      ]
    );
  };

  const handleGenKey = async () => {
    setBusy(true);
    addLog('Sending GENKEY (rekey Locking SP)...');
    try {
      await ble.opalSend(password);
      addLog('GenKey command sent.');
    } catch (e) { addLog('Error: ' + e.message); }
    setBusy(false);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>TCG Opal Console</Text>
      <Text style={styles.warning}>⚠️ Opal commands modify SED locking state.{'\n'}Use only on drives you own.</Text>

      <Text style={styles.label}>User Password (for UNLOCK)</Text>
      <TextInput style={styles.input} value={password} onChangeText={setPassword}
        secureTextEntry placeholder="Enter Opal user password"/>

      <Text style={styles.label}>PSID (for REVERT)</Text>
      <TextInput style={styles.input} value={psid} onChangeText={setPsid}
        secureTextEntry placeholder="Enter PSID (printed on drive label)"/>

      <View style={styles.buttonRow}>
        <TouchableOpacity style={styles.btnGreen} onPress={handleUnlock} disabled={busy}>
          <Text style={styles.btnText}>Unlock</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btnYellow} onPress={handleGenKey} disabled={busy}>
          <Text style={styles.btnText}>GenKey</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btnRed} onPress={handleRevert} disabled={busy}>
          <Text style={styles.btnText}>Revert</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.logTitle}>Session Log</Text>
      <View style={styles.logBox}>
        {log.length === 0 ? (
          <Text style={styles.empty}>No Opal commands sent yet.</Text>
        ) : (
          log.map((entry, i) => (
            <View key={i} style={styles.logRow}>
              <Text style={styles.logTime}>{entry.time}</Text>
              <Text style={styles.logMsg}>{entry.msg}</Text>
            </View>
          ))
        )}
      </View>

      <Text style={styles.footer}>TCG Opal SSC 2.01 — jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 5 },
  warning: { color: '#FF6600', fontSize: 11, marginBottom: 15 },
  label: { color: '#aaa', fontSize: 14, marginTop: 10, marginBottom: 5 },
  input: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, padding: 10, fontSize: 14, marginBottom: 5 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 15, marginBottom: 15 },
  btnGreen: { backgroundColor: '#00AA00', padding: 12, borderRadius: 6, width: '31%', alignItems: 'center' },
  btnYellow: { backgroundColor: '#cca300', padding: 12, borderRadius: 6, width: '31%', alignItems: 'center' },
  btnRed: { backgroundColor: '#cc3300', padding: 12, borderRadius: 6, width: '31%', alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 13, fontWeight: 'bold' },
  logTitle: { fontSize: 16, color: '#888', marginBottom: 8 },
  logBox: { backgroundColor: '#0d1117', borderRadius: 6, padding: 10, minHeight: 150 },
  empty: { color: '#555', textAlign: 'center', marginTop: 30 },
  logRow: { flexDirection: 'row', paddingVertical: 3 },
  logTime: { color: '#666', fontSize: 11, width: 80 },
  logMsg: { color: '#aaa', fontSize: 12, flex: 1, fontFamily: 'monospace' },
  footer: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 10 },
});