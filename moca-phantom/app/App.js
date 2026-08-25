// MoCA Phantom Companion App
// Author: jayis1

import React, { useMemo, useState } from 'react';
import { SafeAreaView, ScrollView, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import DashboardScreen from './screens/DashboardScreen';
import NodeMapScreen from './screens/NodeMapScreen';
import ProfilesScreen from './screens/ProfilesScreen';
import CaptureScreen from './screens/CaptureScreen';
import SafetyScreen from './screens/SafetyScreen';
import { buildProfileCommand } from './utils/protocol';

const tabs = ['Dashboard', 'Node Map', 'Profiles', 'Captures', 'Safety'];

const sampleNodes = [
  { id: 1, label: 'gateway-cpe', privacyEnabled: true, rssi: -38, role: 'controller' },
  { id: 2, label: 'livingroom-stb', privacyEnabled: true, rssi: -47, role: 'endpoint' },
  { id: 3, label: 'bridge-extender', privacyEnabled: false, rssi: -53, role: 'endpoint' },
  { id: 7, label: 'adjacent-unit-leak', privacyEnabled: false, rssi: -74, role: 'unexpected' },
];

const sampleEvents = [
  { ts: '10:14:08', title: 'Privacy-disabled node discovered', severity: 'high' },
  { ts: '10:14:23', title: 'Probe path shows adjacent-unit leakage', severity: 'medium' },
  { ts: '10:15:04', title: 'Rate-cap validation rule armed in lab profile', severity: 'low' },
];

export default function App() {
  const [activeTab, setActiveTab] = useState('Dashboard');
  const [selectedProfile, setSelectedProfile] = useState('survey');
  const [safeMode, setSafeMode] = useState(true);

  const command = useMemo(() => buildProfileCommand(selectedProfile), [selectedProfile]);

  const renderScreen = () => {
    switch (activeTab) {
      case 'Dashboard':
        return <DashboardScreen profile={selectedProfile} command={command} safeMode={safeMode} events={sampleEvents} />;
      case 'Node Map':
        return <NodeMapScreen nodes={sampleNodes} />;
      case 'Profiles':
        return (
          <ProfilesScreen
            selectedProfile={selectedProfile}
            onSelectProfile={setSelectedProfile}
            safeMode={safeMode}
          />
        );
      case 'Captures':
        return <CaptureScreen events={sampleEvents} />;
      case 'Safety':
        return <SafetyScreen safeMode={safeMode} onToggleSafeMode={() => setSafeMode((value) => !value)} />;
      default:
        return null;
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>MoCA Phantom</Text>
        <Text style={styles.subtitle}>Authorized coax security assessment tool · Author: jayis1</Text>
      </View>
      <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.tabRow} contentContainerStyle={styles.tabRowContent}>
        {tabs.map((tab) => (
          <TouchableOpacity
            key={tab}
            style={[styles.tab, activeTab === tab && styles.tabActive]}
            onPress={() => setActiveTab(tab)}
          >
            <Text style={[styles.tabText, activeTab === tab && styles.tabTextActive]}>{tab}</Text>
          </TouchableOpacity>
        ))}
      </ScrollView>
      <View style={styles.screen}>{renderScreen()}</View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#08111f',
  },
  header: {
    paddingHorizontal: 18,
    paddingTop: 18,
    paddingBottom: 12,
  },
  title: {
    color: '#edf4ff',
    fontSize: 28,
    fontWeight: '700',
  },
  subtitle: {
    color: '#8aa4c8',
    fontSize: 13,
    marginTop: 4,
  },
  tabRow: {
    maxHeight: 54,
  },
  tabRowContent: {
    paddingHorizontal: 12,
    gap: 10,
  },
  tab: {
    backgroundColor: '#11223a',
    borderRadius: 18,
    paddingHorizontal: 14,
    paddingVertical: 10,
  },
  tabActive: {
    backgroundColor: '#28c0f0',
  },
  tabText: {
    color: '#bcd0ea',
    fontWeight: '600',
  },
  tabTextActive: {
    color: '#04101c',
  },
  screen: {
    flex: 1,
    padding: 16,
  },
});
