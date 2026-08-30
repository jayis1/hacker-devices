// App.js - EDID Phantom companion app
// Author: jayis1
// Copyright (c) 2026 jayis1
import React, { useMemo, useState } from 'react';
import { SafeAreaView, ScrollView, View, Text, Pressable, StyleSheet } from 'react-native';
import OverviewScreen from './screens/OverviewScreen';
import CaptureScreen from './screens/CaptureScreen';
import MutationStudioScreen from './screens/MutationStudioScreen';
import SafetyScreen from './screens/SafetyScreen';
import CECConsoleScreen from './screens/CECConsoleScreen';
import { AUTHOR, profiles, captureFeed, cecActions, buildMutationReport } from './utils/data';

const tabs = ['Overview', 'Capture', 'Mutation Studio', 'CEC Console', 'Safety'];

export default function App() {
  const [activeTab, setActiveTab] = useState('Overview');
  const [selectedProfile, setSelectedProfile] = useState('conference-mirror');
  const [toggles, setToggles] = useState({
    preserveVendor: true,
    injectLatency: true,
    spoofHdr: false,
    cecGuard: true,
  });

  const mutationReport = useMemo(
    () => buildMutationReport(selectedProfile, toggles),
    [selectedProfile, toggles]
  );

  const handleToggle = (key) => {
    setToggles((prev) => ({ ...prev, [key]: !prev[key] }));
  };

  return (
    <SafeAreaView style={styles.safeArea}>
      <ScrollView contentContainerStyle={styles.container}>
        <Text style={styles.title}>EDID Phantom</Text>
        <Text style={styles.subtitle}>Display trust-boundary research console by {AUTHOR}</Text>
        <View style={styles.pillRow}>
          {tabs.map((tab) => (
            <Pressable
              key={tab}
              style={[styles.pill, activeTab === tab && styles.pillActive]}
              onPress={() => setActiveTab(tab)}
            >
              <Text style={[styles.pillText, activeTab === tab && styles.pillTextActive]}>{tab}</Text>
            </Pressable>
          ))}
        </View>

        {activeTab === 'Overview' && (
          <OverviewScreen
            profiles={profiles}
            selectedProfile={selectedProfile}
            onSelectProfile={setSelectedProfile}
            report={mutationReport}
          />
        )}
        {activeTab === 'Capture' && <CaptureScreen events={captureFeed} />}
        {activeTab === 'Mutation Studio' && (
          <MutationStudioScreen toggles={toggles} onToggle={handleToggle} report={mutationReport} />
        )}
        {activeTab === 'CEC Console' && <CECConsoleScreen actions={cecActions} />}
        {activeTab === 'Safety' && <SafetyScreen toggles={toggles} />}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#020617',
  },
  container: {
    padding: 18,
    gap: 18,
  },
  title: {
    color: '#f8fafc',
    fontSize: 30,
    fontWeight: '900',
  },
  subtitle: {
    color: '#94a3b8',
    lineHeight: 20,
  },
  pillRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 10,
  },
  pill: {
    backgroundColor: '#111827',
    borderRadius: 999,
    paddingVertical: 10,
    paddingHorizontal: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
  },
  pillActive: {
    backgroundColor: '#0f172a',
    borderColor: '#22d3ee',
  },
  pillText: {
    color: '#94a3b8',
    fontWeight: '800',
  },
  pillTextActive: {
    color: '#ecfeff',
  },
});
