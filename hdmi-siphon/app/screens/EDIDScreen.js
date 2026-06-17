/**
 * EDIDScreen.js — EDID management
 * Author: jayis1
 */

import React, {useState} from 'react';
import {View, Text, StyleSheet, TouchableOpacity, Alert} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const EDID_PRESETS = [
  {name: '1080p60 Standard', edid: '00FFFFFFFFFFFF00...'},
  {name: '1080p60 No HDCP', edid: '00FFFFFFFFFFFF00...', noHDCP: true},
  {name: '720p60', edid: '00FFFFFFFFFFFF00...'},
  {name: '1024x768 (4:3)', edid: '00FFFFFFFFFFFF00...'},
  {name: 'Force Detect', edid: '00FFFFFFFFFFFF00...'},
];

const EDIDScreen = ({protocol}) => {
  const [currentSource, setCurrentSource] = useState('display');

  const handleInject = preset => {
    if (!protocol) return;
    if (preset.noHDCP) {
      protocol.disableHDCP(result => {
        Alert.alert(
          'EDID Updated',
          result?.status === 'ok'
            ? 'HDCP disabled in EDID'
            : 'Failed to update EDID',
        );
      });
    } else {
      Alert.alert('Info', 'EDID injection requires hex data');
    }
  };

  const handleRestorePassthrough = () => {
    if (!protocol) return;
    protocol.sendCommand('edid_set', {}, result => {
      Alert.alert(
        'EDID Restored',
        result?.status === 'ok'
          ? 'Passthrough mode restored'
          : 'Failed to restore',
      );
    });
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>EDID Manager</Text>
        <Text style={styles.cardDesc}>
          Extended Display Identification Data control
        </Text>

        <View style={styles.statusBar}>
          <Icon
            name="card-bulleted-outline"
            size={20}
            color="#42A5F5"
          />
          <Text style={styles.statusText}>
            Source: {currentSource === 'display' ? 'Display EDID' : 'Injected'}
          </Text>
        </View>
      </View>

      <Text style={styles.sectionTitle}>Presets</Text>
      {EDID_PRESETS.map((preset, idx) => (
        <TouchableOpacity
          key={idx}
          style={styles.presetItem}
          onPress={() => handleInject(preset)}>
          <View>
            <Text style={styles.presetName}>{preset.name}</Text>
            <Text style={styles.presetDesc}>
              {preset.noHDCP ? 'Disables HDCP flag' : 'Standard EDID'}
            </Text>
          </View>
          <Icon name="upload" size={20} color="#E53935" />
        </TouchableOpacity>
      ))}

      <TouchableOpacity
        style={styles.restoreButton}
        onPress={handleRestorePassthrough}>
        <Icon name="restore" size={20} color="#FFF" />
        <Text style={styles.restoreText}>Restore Passthrough</Text>
      </TouchableOpacity>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#121212',
    padding: 12,
  },
  card: {
    backgroundColor: '#1E1E2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: '700',
  },
  cardDesc: {
    color: '#B0B0B0',
    fontSize: 13,
    marginBottom: 12,
  },
  statusBar: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#2A2A3E',
    padding: 10,
    borderRadius: 8,
  },
  statusText: {
    color: '#FFF',
    fontSize: 14,
    marginLeft: 8,
  },
  sectionTitle: {
    color: '#FFF',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 8,
    marginTop: 8,
  },
  presetItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1E1E2E',
    padding: 14,
    borderRadius: 8,
    marginBottom: 6,
  },
  presetName: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '600',
  },
  presetDesc: {
    color: '#78909C',
    fontSize: 12,
    marginTop: 2,
  },
  restoreButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#546E7A',
    padding: 14,
    borderRadius: 8,
    marginTop: 12,
  },
  restoreText: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '600',
    marginLeft: 8,
  },
});

export default EDIDScreen;
