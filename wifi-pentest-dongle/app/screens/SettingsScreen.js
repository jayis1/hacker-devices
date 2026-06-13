/**
 * WFP-100 Settings Screen — Channel, band, and injection configuration
 *
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, TextInput } from 'react-native';
import { buildFrame, CMD_SET_CHANNEL, CMD_SET_BAND, CMD_SET_FILTER, CMD_WIFI_KILL, BAND_2GHZ, BAND_5GHZ, BAND_6GHZ } from '../utils/protocol';

const CHANNELS = {
  [BAND_2GHZ]: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14],
  [BAND_5GHZ]: [36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 149, 153, 157, 161, 165, 169, 173, 177],
  [BAND_6GHZ]: [1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233],
};

const BAND_NAMES = {
  [BAND_2GHZ]: '2.4 GHz',
  [BAND_5GHZ]: '5 GHz',
  [BAND_6GHZ]: '6 GHz',
};

export default function SettingsScreen() {
  const [band, setBand] = useState(BAND_2GHZ);
  const [channel, setChannel] = useState(6);
  const [filterBssid, setFilterBssid] = useState('');
  const [wifiKill, setWifiKill] = useState(false);

  const sendCommand = (cmd, payload) => {
    const frame = buildFrame(cmd, payload);
    console.log(`TX [${cmd.toString(16).padStart(2, '0')}]: ${Array.from(frame).map(b => b.toString(16).padStart(2, '0')).join(' ')}`);
  };

  const handleBandChange = (newBand) => {
    setBand(newBand);
    const payload = new Uint8Array(1);
    payload[0] = newBand;
    sendCommand(CMD_SET_BAND, payload);
    // Reset channel to first in the new band
    setChannel(CHANNELS[newBand][0]);
  };

  const handleChannelChange = (ch) => {
    setChannel(ch);
    const payload = new Uint8Array(3);
    payload[0] = ch & 0xFF;
    payload[1] = (ch >> 8) & 0xFF;
    payload[2] = band;
    sendCommand(CMD_SET_CHANNEL, payload);
  };

  const handleWifiKill = () => {
    const newState = !wifiKill;
    setWifiKill(newState);
    sendCommand(CMD_WIFI_KILL, new Uint8Array([newState ? 1 : 0]));
  };

  const handleSetFilter = () => {
    if (filterBssid.length === 17) {  // xx:xx:xx:xx:xx:xx format
      const macBytes = filterBssid.split(':').map(h => parseInt(h, 16));
      sendCommand(CMD_SET_FILTER, new Uint8Array(macBytes));
    }
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.sectionTitle}>Band Selection</Text>
      <View style={styles.bandRow}>
        {[BAND_2GHZ, BAND_5GHZ, BAND_6GHZ].map((b) => (
          <TouchableOpacity
            key={b}
            style={[styles.bandButton, band === b && styles.bandButtonActive]}
            onPress={() => handleBandChange(b)}
          >
            <Text style={[styles.bandButtonText, band === b && styles.bandButtonTextActive]}>
              {BAND_NAMES[b]}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={styles.sectionTitle}>Channel (Band: {BAND_NAMES[band]})</Text>
      <View style={styles.channelGrid}>
        {CHANNELS[band].slice(0, 20).map((ch) => (
          <TouchableOpacity
            key={ch}
            style={[styles.channelButton, channel === ch && styles.channelButtonActive]}
            onPress={() => handleChannelChange(ch)}
          >
            <Text style={[styles.channelButtonText, channel === ch && styles.channelButtonTextActive]}>
              {ch}
            </Text>
          </TouchableOpacity>
        ))}
        {CHANNELS[band].length > 20 && (
          <Text style={styles.moreChannels}>+{CHANNELS[band].length - 20} more channels</Text>
        )}
      </View>

      <Text style={styles.sectionTitle}>BSSID Filter (optional)</Text>
      <TextInput
        style={styles.input}
        value={filterBssid}
        onChangeText={setFilterBssid}
        placeholder="aa:bb:cc:dd:ee:ff"
        placeholderTextColor="#666"
        autoCapitalize="none"
        maxLength={17}
      />
      <TouchableOpacity style={styles.filterButton} onPress={handleSetFilter}>
        <Text style={styles.filterButtonText}>Apply Filter</Text>
      </TouchableOpacity>

      <Text style={styles.sectionTitle}>RF Kill Switch</Text>
      <TouchableOpacity
        style={[styles.killButton, wifiKill ? styles.killButtonActive : styles.killButtonInactive]}
        onPress={handleWifiKill}
      >
        <Text style={styles.killButtonText}>
          {wifiKill ? 'WiFi KILLED — Radio Off' : 'WiFi ACTIVE — Radio On'}
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#16213e' },
  content: { padding: 16 },
  sectionTitle: { fontSize: 16, fontWeight: '600', color: '#00ff88', marginTop: 16, marginBottom: 8 },
  bandRow: { flexDirection: 'row', gap: 8 },
  bandButton: { flex: 1, paddingVertical: 12, borderRadius: 8, backgroundColor: '#1a1a2e', alignItems: 'center' },
  bandButtonActive: { backgroundColor: '#0ea5e9' },
  bandButtonText: { color: '#888', fontSize: 14, fontWeight: '600' },
  bandButtonTextActive: { color: '#1a1a2e' },
  channelGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  channelButton: { width: 44, height: 36, borderRadius: 6, backgroundColor: '#1a1a2e', justifyContent: 'center', alignItems: 'center' },
  channelButtonActive: { backgroundColor: '#00ff88' },
  channelButtonText: { color: '#888', fontSize: 12 },
  channelButtonTextActive: { color: '#1a1a2e', fontWeight: '700' },
  moreChannels: { color: '#666', fontSize: 11, marginTop: 4 },
  input: { backgroundColor: '#1a1a2e', color: '#fff', borderRadius: 8, padding: 12, fontSize: 14, fontFamily: 'monospace' },
  filterButton: { backgroundColor: '#8b5cf6', paddingVertical: 12, borderRadius: 8, marginTop: 8, alignItems: 'center' },
  filterButtonText: { color: '#1a1a2e', fontWeight: '600', fontSize: 14 },
  killButton: { paddingVertical: 16, borderRadius: 8, alignItems: 'center', marginTop: 8 },
  killButtonInactive: { backgroundColor: '#22c55e' },
  killButtonActive: { backgroundColor: '#ef4444' },
  killButtonText: { color: '#fff', fontWeight: '700', fontSize: 16 },
});