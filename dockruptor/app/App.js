// Dockruptor React Native App
// Author: jayis1
// SPDX-License-Identifier: MIT

import React, { useMemo, useState } from 'react';
import { SafeAreaView, ScrollView, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import DashboardScreen from './screens/DashboardScreen';
import PolicyScreen from './screens/PolicyScreen';
import IdentityScreen from './screens/IdentityScreen';
import CaptureScreen from './screens/CaptureScreen';
import SafetyScreen from './screens/SafetyScreen';
import { buildMockSession } from './utils/protocol';

const tabs = ['Dashboard', 'Policy', 'Identity', 'Capture', 'Safety'];

export default function App() {
  const [activeTab, setActiveTab] = useState('Dashboard');
  const [session, setSession] = useState(buildMockSession());

  const screenProps = useMemo(
    () => ({
      session,
      onMutatePolicy: (nextPolicy) => {
        setSession((current) => ({
          ...current,
          policy: nextPolicy,
          events: [
            {
              id: `evt-${current.events.length + 1}`,
              type: 'policy',
              direction: 'internal',
              summary: 'Policy profile updated locally by authorized operator',
              time: '09:31:48',
            },
            ...current.events,
          ],
        }));
      },
      onAcknowledgeSafety: () => {
        setSession((current) => ({
          ...current,
          safety: { ...current.safety, acknowledged: true },
        }));
      },
    }),
    [session]
  );

  const renderScreen = () => {
    switch (activeTab) {
      case 'Policy':
        return <PolicyScreen {...screenProps} />;
      case 'Identity':
        return <IdentityScreen {...screenProps} />;
      case 'Capture':
        return <CaptureScreen {...screenProps} />;
      case 'Safety':
        return <SafetyScreen {...screenProps} />;
      case 'Dashboard':
      default:
        return <DashboardScreen {...screenProps} />;
    }
  };

  return (
    <SafeAreaView style={styles.safeArea}>
      <View style={styles.header}>
        <View>
          <Text style={styles.title}>Dockruptor</Text>
          <Text style={styles.subtitle}>Authorized USB-C policy manipulation research platform</Text>
        </View>
        <View style={styles.badge}>
          <Text style={styles.badgeText}>author: jayis1</Text>
        </View>
      </View>

      <View style={styles.tabRow}>
        {tabs.map((tab) => (
          <TouchableOpacity
            key={tab}
            style={[styles.tabButton, activeTab === tab && styles.tabButtonActive]}
            onPress={() => setActiveTab(tab)}
          >
            <Text style={[styles.tabText, activeTab === tab && styles.tabTextActive]}>{tab}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <ScrollView contentContainerStyle={styles.body}>{renderScreen()}</ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#08111f',
  },
  header: {
    paddingTop: 16,
    paddingHorizontal: 16,
    paddingBottom: 12,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
  },
  title: {
    color: '#e5f1ff',
    fontSize: 30,
    fontWeight: '800',
  },
  subtitle: {
    color: '#8ca2c6',
    marginTop: 4,
    maxWidth: 260,
  },
  badge: {
    backgroundColor: '#14304d',
    borderRadius: 999,
    paddingHorizontal: 12,
    paddingVertical: 8,
  },
  badgeText: {
    color: '#7fd1ff',
    fontWeight: '700',
    fontSize: 12,
  },
  tabRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    paddingHorizontal: 12,
    gap: 8,
  },
  tabButton: {
    paddingVertical: 10,
    paddingHorizontal: 14,
    borderRadius: 12,
    backgroundColor: '#102338',
  },
  tabButtonActive: {
    backgroundColor: '#1d5cff',
  },
  tabText: {
    color: '#a9bfdc',
    fontWeight: '700',
  },
  tabTextActive: {
    color: '#ffffff',
  },
  body: {
    padding: 16,
    gap: 16,
  },
});
