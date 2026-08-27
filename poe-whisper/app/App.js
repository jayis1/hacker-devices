// App.js - PoE Whisper companion application
// Author: jayis1
import React, { useMemo, useState } from 'react';
import { SafeAreaView, ScrollView, View, Text, Pressable, StyleSheet } from 'react-native';
import DashboardScreen from './screens/DashboardScreen';
import ProfilesScreen from './screens/ProfilesScreen';
import CaptureScreen from './screens/CaptureScreen';
import InjectorScreen from './screens/InjectorScreen';
import SafetyScreen from './screens/SafetyScreen';
import { AUTHOR, profiles, eventFeed, safetyChecklist, buildStatus } from './utils/protocol';

const tabs = ['Overview', 'Profiles', 'Capture', 'Injector', 'Safety'];

export default function App() {
  const [activeTab, setActiveTab] = useState('Overview');
  const [selectedProfile, setSelectedProfile] = useState('camera-reboot-window');
  const [toggles, setToggles] = useState({
    lldpSpoof: true,
    mpsJitter: false,
    brownout: true,
  });

  const status = useMemo(() => buildStatus(selectedProfile, toggles), [selectedProfile, toggles]);

  const handleToggle = (key) => {
    setToggles((prev) => ({ ...prev, [key]: !prev[key] }));
  };

  return (
    <SafeAreaView style={styles.safeArea}>
      <ScrollView contentContainerStyle={styles.container}>
        <Text style={styles.brand}>PoE Whisper</Text>
        <Text style={styles.byline}>Inline PoE negotiation research companion app by {AUTHOR}</Text>
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

        {activeTab === 'Overview' && <DashboardScreen status={status} />}
        {activeTab === 'Profiles' && (
          <ProfilesScreen profiles={profiles} selected={selectedProfile} onSelect={setSelectedProfile} />
        )}
        {activeTab === 'Capture' && <CaptureScreen events={eventFeed} />}
        {activeTab === 'Injector' && <InjectorScreen toggles={toggles} onToggle={handleToggle} />}
        {activeTab === 'Safety' && <SafetyScreen checklist={safetyChecklist} />}
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
  brand: {
    color: '#f8fafc',
    fontSize: 30,
    fontWeight: '900',
  },
  byline: {
    color: '#94a3b8',
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
    borderColor: '#22d3ee',
    backgroundColor: '#082f49',
  },
  tabText: {
    color: '#9ca3af',
    fontWeight: '700',
  },
  tabTextActive: {
    color: '#ecfeff',
  },
});
