// EtherCAT Phantom companion app
// Author: jayis1

import React, { useState } from 'react';
import { SafeAreaView, ScrollView, StatusBar, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import OverviewScreen from './screens/OverviewScreen';
import RulesScreen from './screens/RulesScreen';
import CapturesScreen from './screens/CapturesScreen';
import SafetyScreen from './screens/SafetyScreen';
import { sampleCaptures, sampleDevice, sampleRules } from './utils/protocol';

const tabs = ['overview', 'rules', 'captures', 'safety'];

export default function App() {
  const [tab, setTab] = useState('overview');

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.title}>EtherCAT Phantom</Text>
        <Text style={styles.subtitle}>Author: jayis1 · OT inline frame surgery workstation</Text>

        <View style={styles.tabBar}>
          {tabs.map((entry) => (
            <TouchableOpacity
              key={entry}
              onPress={() => setTab(entry)}
              style={[styles.tab, tab === entry && styles.tabActive]}
            >
              <Text style={[styles.tabText, tab === entry && styles.tabTextActive]}>{entry.toUpperCase()}</Text>
            </TouchableOpacity>
          ))}
        </View>

        {tab === 'overview' && <OverviewScreen device={sampleDevice} />}
        {tab === 'rules' && <RulesScreen rules={sampleRules} />}
        {tab === 'captures' && <CapturesScreen captures={sampleCaptures} />}
        {tab === 'safety' && <SafetyScreen />}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#07111c',
  },
  content: {
    padding: 18,
    paddingBottom: 40,
  },
  title: {
    color: '#eef5ff',
    fontSize: 30,
    fontWeight: '800',
  },
  subtitle: {
    color: '#8ca3c2',
    marginTop: 8,
    marginBottom: 18,
  },
  tabBar: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
    marginBottom: 18,
  },
  tab: {
    borderWidth: 1,
    borderColor: '#243449',
    borderRadius: 999,
    paddingHorizontal: 12,
    paddingVertical: 8,
  },
  tabActive: {
    backgroundColor: '#16314f',
    borderColor: '#53b2ff',
  },
  tabText: {
    color: '#7e94b3',
    fontSize: 12,
    fontWeight: '700',
  },
  tabTextActive: {
    color: '#eef6ff',
  },
  sectionTitle: {
    color: '#eaf3fe',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  metricRow: {
    flexDirection: 'row',
    gap: 12,
    flexWrap: 'wrap',
  },
  copy: {
    color: '#94a9c3',
    marginTop: 14,
    lineHeight: 21,
  },
  checkRow: {
    flexDirection: 'row',
    marginBottom: 8,
  },
  checkBullet: {
    color: '#53b2ff',
    width: 16,
    fontSize: 16,
  },
  checkText: {
    color: '#d7e4f5',
    flex: 1,
  },
});
