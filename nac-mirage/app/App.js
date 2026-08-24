// NAC Mirage companion app
// Author: jayis1

import React, { useMemo, useState } from 'react';
import { SafeAreaView, View, Text, Pressable, StyleSheet, ScrollView } from 'react-native';
import DashboardScreen from './screens/DashboardScreen';
import ProfilesScreen from './screens/ProfilesScreen';
import CaptureScreen from './screens/CaptureScreen';
import SafetyScreen from './screens/SafetyScreen';
import { demoStatus, profiles, eventFeed, safetyChecklist, AUTHOR } from './utils/protocol';

const tabs = ['Overview', 'Profiles', 'Capture', 'Safety'];

export default function App() {
  const [activeTab, setActiveTab] = useState('Overview');
  const [selectedProfile, setSelectedProfile] = useState('staged-relay');

  const derivedStatus = useMemo(() => ({
    ...demoStatus,
    profile: selectedProfile,
  }), [selectedProfile]);

  return (
    <SafeAreaView style={styles.safeArea}>
      <ScrollView contentContainerStyle={styles.container}>
        <Text style={styles.brand}>NAC Mirage</Text>
        <Text style={styles.byline}>Inline PoE / LLDP / NAC deception bridge companion app by {AUTHOR}</Text>
        <View style={styles.tabRow}>
          {tabs.map((tab) => (
            <Pressable
              key={tab}
              style={[styles.tab, activeTab === tab && styles.tabActive]}
              onPress={() => setActiveTab(tab)}
            >
              <Text style={[styles.tabText, activeTab === tab && styles.tabTextActive]}>{tab}</Text>
            </Pressable>
          ))}
        </View>

        {activeTab === 'Overview' && <DashboardScreen status={derivedStatus} />}
        {activeTab === 'Profiles' && (
          <ProfilesScreen profiles={profiles} selected={selectedProfile} onSelect={setSelectedProfile} />
        )}
        {activeTab === 'Capture' && <CaptureScreen events={eventFeed} />}
        {activeTab === 'Safety' && <SafetyScreen checklist={safetyChecklist} />}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#030712',
  },
  container: {
    padding: 18,
    gap: 18,
  },
  brand: {
    color: '#f9fafb',
    fontSize: 30,
    fontWeight: '900',
  },
  byline: {
    color: '#9ca3af',
    lineHeight: 20,
  },
  tabRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 10,
  },
  tab: {
    backgroundColor: '#111827',
    borderRadius: 999,
    paddingVertical: 10,
    paddingHorizontal: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
  },
  tabActive: {
    borderColor: '#3ddc97',
    backgroundColor: '#0b1b1b',
  },
  tabText: {
    color: '#9ca3af',
    fontWeight: '700',
  },
  tabTextActive: {
    color: '#ecfeff',
  },
});
