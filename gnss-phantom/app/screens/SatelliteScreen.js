/*
 * screens/SatelliteScreen.js — Satellite configuration screen
 *
 * Allows adding/removing simulated satellites, setting per-SV power,
 * selecting constellation, and loading ephemeris data.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  ScrollView, Alert, Modal, FlatList,
} from 'react-native';
import { bleManager } from '../utils/ble';
import { cmdSetSV, CONSTELLATION, RF_FREQS } from '../utils/protocol';

const CONSTELLATIONS = [
  { id: CONSTELLATION.GPS,      label: 'GPS',      freq: RF_FREQS.GPS_L1 },
  { id: CONSTELLATION.GLONASS,  label: 'GLONASS',  freq: RF_FREQS.GLONASS_L1 },
  { id: CONSTELLATION.GALILEO,  label: 'Galileo',  freq: RF_FREQS.GALILEO_E1 },
  { id: CONSTELLATION.BEIDOU,   label: 'BeiDou',   freq: RF_FREQS.BEIDOU_B1 },
];

export default function SatelliteScreen() {
  const [activeSVs, setActiveSVs] = useState([]);
  const [showAdd, setShowAdd] = useState(false);
  const [selConstellation, setSelConstellation] = useState(CONSTELLATION.GPS);
  const [prnInput, setPrnInput] = useState('7');
  const [powerInput, setPowerInput] = useState('0');

  const addSV = async () => {
    const prn = parseInt(prnInput, 10);
    const power = parseInt(powerInput, 10);
    if (isNaN(prn) || prn < 1 || prn > 32) {
      Alert.alert('Invalid PRN', 'PRN must be 1-32');
      return;
    }
    await bleManager.send(cmdSetSV(selConstellation, prn, power));
    const consLabel = CONSTELLATIONS.find(c => c.id === selConstellation)?.label || '?';
    setActiveSVs([...activeSVs, { prn, cons: consLabel, power }]);
    setShowAdd(false);
  };

  const removeSV = (prn) => {
    setActiveSVs(activeSVs.filter(sv => sv.prn !== prn));
    // Note: firmware doesn't have a remove-SV command in this version;
    // would need CMD_REMOVE_SV in a production firmware
  };

  const renderSV = ({ item }) => (
    <View style={styles.svRow}>
      <View style={styles.svInfo}>
        <Text style={styles.svPRN}>PRN {item.prn}</Text>
        <Text style={styles.svCons}>{item.cons}</Text>
      </View>
      <Text style={styles.svPower}>{item.power} dB</Text>
      <TouchableOpacity onPress={() => removeSV(item.prn)}>
        <Text style={styles.removeBtn}>✕</Text>
      </TouchableOpacity>
    </View>
  );

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Satellite Configuration</Text>
      </View>

      {/* Active SVs */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>
          ACTIVE SATELLITES ({activeSVs.length})
        </Text>
        {activeSVs.length === 0 ? (
          <Text style={styles.emptyText}>No satellites configured</Text>
        ) : (
          <FlatList
            data={activeSVs}
            keyExtractor={(item) => `${item.cons}-${item.prn}`}
            renderItem={renderSV}
            scrollEnabled={false}
          />
        )}
      </View>

      {/* Add SV button */}
      <TouchableOpacity
        style={styles.addBtn}
        onPress={() => setShowAdd(true)}
      >
        <Text style={styles.addBtnText}>+ Add Satellite</Text>
      </TouchableOpacity>

      {/* Constellation info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CONSTELLATIONS</Text>
        {CONSTELLATIONS.map(c => (
          <View key={c.id} style={styles.consRow}>
            <Text style={styles.consLabel}>{c.label}</Text>
            <Text style={styles.consFreq}>{c.freq.toFixed(3)} MHz</Text>
          </View>
        ))}
      </View>

      {/* Add SV Modal */}
      <Modal visible={showAdd} transparent animationType="slide">
        <View style={styles.modalOverlay}>
          <View style={styles.modalDialog}>
            <Text style={styles.modalTitle}>Add Satellite Vehicle</Text>

            <Text style={styles.inputLabel}>Constellation</Text>
            <View style={styles.consSelector}>
              {CONSTELLATIONS.map(c => (
                <TouchableOpacity
                  key={c.id}
                  style={[
                    styles.consOption,
                    selConstellation === c.id ? styles.consSelected : {},
                  ]}
                  onPress={() => setSelConstellation(c.id)}
                >
                  <Text style={styles.consOptionText}>{c.label}</Text>
                </TouchableOpacity>
              ))}
            </View>

            <Text style={styles.inputLabel}>PRN (1-32)</Text>
            <TextInput
              style={styles.input}
              keyboardType="numeric"
              value={prnInput}
              onChangeText={setPrnInput}
              placeholder="7"
              placeholderTextColor="#555"
            />

            <Text style={styles.inputLabel}>Power Offset (dB)</Text>
            <TextInput
              style={styles.input}
              keyboardType="numeric"
              value={powerInput}
              onChangeText={setPowerInput}
              placeholder="0"
              placeholderTextColor="#555"
            />

            <View style={styles.modalButtons}>
              <TouchableOpacity
                style={[styles.modalBtn, { backgroundColor: '#6b7280' }]}
                onPress={() => setShowAdd(false)}
              >
                <Text style={styles.modalBtnText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.modalBtn, { backgroundColor: '#22c55e' }]}
                onPress={addSV}
              >
                <Text style={styles.modalBtnText}>Add</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#3b82f6', fontSize: 20, fontWeight: 'bold', fontFamily: 'monospace' },
  section: { margin: 10, padding: 15, backgroundColor: '#1a1a2e', borderRadius: 8 },
  sectionTitle: { color: '#3b82f6', fontSize: 11, fontWeight: 'bold', marginBottom: 10, letterSpacing: 1 },
  emptyText: { color: '#888', fontSize: 13, fontStyle: 'italic' },
  svRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 10, borderBottomWidth: 1, borderBottomColor: '#2a2a3e' },
  svInfo: { flex: 1 },
  svPRN: { color: '#fff', fontSize: 14, fontFamily: 'monospace' },
  svCons: { color: '#888', fontSize: 11 },
  svPower: { color: '#eab308', fontSize: 13, marginRight: 15 },
  removeBtn: { color: '#ef4444', fontSize: 18, padding: 5 },
  addBtn: { margin: 10, padding: 15, backgroundColor: '#1a3a1a', borderRadius: 8, alignItems: 'center', borderWidth: 1, borderColor: '#22c55e' },
  addBtnText: { color: '#22c55e', fontSize: 16, fontFamily: 'monospace' },
  consRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  consLabel: { color: '#aaa', fontSize: 13 },
  consFreq: { color: '#3b82f6', fontSize: 12, fontFamily: 'monospace' },
  modalOverlay: { flex: 1, backgroundColor: 'rgba(0,0,0,0.85)', justifyContent: 'center', alignItems: 'center' },
  modalDialog: { width: '90%', backgroundColor: '#1a1a2e', borderRadius: 12, padding: 20 },
  modalTitle: { color: '#3b82f6', fontSize: 18, fontWeight: 'bold', marginBottom: 20, textAlign: 'center' },
  inputLabel: { color: '#888', fontSize: 12, marginTop: 10, marginBottom: 5 },
  consSelector: { flexDirection: 'row', flexWrap: 'wrap', marginVertical: 5 },
  consOption: { padding: 8, margin: 4, backgroundColor: '#2a2a3e', borderRadius: 6 },
  consSelected: { backgroundColor: '#3b82f6' },
  consOptionText: { color: '#fff', fontSize: 12 },
  input: { backgroundColor: '#2a2a3e', color: '#fff', borderRadius: 6, padding: 10, fontSize: 14, fontFamily: 'monospace' },
  modalButtons: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 20 },
  modalBtn: { padding: 12, borderRadius: 8, width: '40%', alignItems: 'center' },
  modalBtnText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
});