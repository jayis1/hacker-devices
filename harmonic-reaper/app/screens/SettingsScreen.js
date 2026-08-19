/**
 * SettingsScreen.js — BLE pairing, firmware update, data export, legal info
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, Button, Alert, ScrollView, Linking
} from 'react-native';
import { OPCODES } from '../utils/protocol';

export default function SettingsScreen({ ble }) {
  const [exporting, setExporting] = useState(false);

  const handleExport = () => {
    setExporting(true);
    ble.sendCommand(OPCODES.CMD_EXPORT, []);
    Alert.alert('Export', 'Exporting hit log to microSD. You will be notified when complete.');
    setTimeout(() => setExporting(false), 5000);
  };

  const handleDisconnect = () => {
    Alert.alert(
      'Disconnect',
      'Disconnect from the wand?',
      [
        { text: 'Cancel' },
        { text: 'Disconnect', onPress: () => ble.disconnect() },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>System Settings</Text>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device</Text>
        <Text style={styles.info}>Name: HR-NLJD</Text>
        <Text style={styles.info}>Model: Harmonic Reaper</Text>
        <Text style={styles.info}>Firmware: 1.0.0 (jayis1)</Text>
        <Text style={styles.info}>License: GPL-2.0</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data</Text>
        <Button
          title={exporting ? 'Exporting…' : 'Export Hit Log to SD'}
          onPress={handleExport}
          color="#00ff88"
          disabled={exporting}
        />
        <View style={{ height: 8 }} />
        <Button
          title="Disconnect from Wand"
          onPress={handleDisconnect}
          color="#ff3333"
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware Update</Text>
        <Text style={styles.hint}>
          To update firmware: connect the wand via USB-C and send "DFU" to the
          virtual serial port (115200 8N1). The wand reboots into the Nordic
          bootloader. Use nrfjprog or nRF Connect to flash the new .hex file.
        </Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.about}>
          Harmonic Reaper is an open-source Non-Linear Junction Detector
          (NLJD) for Technical Surveillance Counter-Measures (TSCM). It
          transmits a 2.4 GHz carrier and analyzes the 2nd and 3rd
          harmonics returned by semiconductor junctions to detect hidden
          electronic devices (bugs, cameras, GPS trackers) even when they
          are powered off.
        </Text>
        <Text style={styles.about}>
          Designed and authored by jayis1. Released under GPL-2.0.
        </Text>
      </View>

      <View style={styles.disclaimerBox}>
        <Text style={styles.disclaimerTitle}>⚠️ Legal & Ethical Use</Text>
        <Text style={styles.disclaimerText}>
          This device is for authorized security surveys only. Only use it
          on facilities and equipment you own or have written permission to
          inspect. The 2.4 GHz ISM band transmission must comply with local
          radio regulations (FCC §15.209 / ETSI EN 300 440). The operator
          is solely responsible for lawful operation. The author (jayis1)
          disclaims all liability for misuse.
        </Text>
      </View>
    </ScrollView>
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
    marginBottom: 16,
  },
  section: {
    marginBottom: 24,
  },
  sectionTitle: {
    color: '#00ff88',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  info: {
    color: '#888',
    fontSize: 13,
    marginBottom: 2,
  },
  hint: {
    color: '#666',
    fontSize: 12,
    lineHeight: 18,
  },
  about: {
    color: '#aaa',
    fontSize: 12,
    lineHeight: 18,
    marginBottom: 8,
  },
  disclaimerBox: {
    backgroundColor: '#1a0000',
    borderRadius: 8,
    padding: 12,
    marginBottom: 24,
    borderWidth: 1,
    borderColor: '#330000',
  },
  disclaimerTitle: {
    color: '#ff6666',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 6,
  },
  disclaimerText: {
    color: '#aa8888',
    fontSize: 11,
    lineHeight: 16,
  },
});