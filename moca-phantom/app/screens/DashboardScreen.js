// MoCA Phantom dashboard screen
// Author: jayis1

import React from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import StatCard from '../components/StatCard';
import EventList from '../components/EventList';

export default function DashboardScreen({ profile, command, safeMode, events }) {
  return (
    <ScrollView style={styles.container}>
      <Text style={styles.heading}>Engagement Summary</Text>
      <View style={styles.grid}>
        <StatCard label="Profile" value={profile} />
        <StatCard label="Mode" value={command.mode} accent="#ffc857" />
        <StatCard label="Safe Mode" value={safeMode ? 'ON' : 'OFF'} accent={safeMode ? '#65d6a6' : '#ff647c'} />
        <StatCard label="Injection" value={command.inject ? 'Armed' : 'Disabled'} accent={command.inject ? '#ff647c' : '#65d6a6'} />
      </View>
      <View style={styles.panel}>
        <Text style={styles.panelTitle}>Command Preview</Text>
        <Text style={styles.panelBody}>{command.note}</Text>
      </View>
      <Text style={styles.heading}>Recent Findings</Text>
      <EventList events={events} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  heading: {
    color: '#edf4ff',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
    marginBottom: 16,
  },
  panel: {
    backgroundColor: '#0f1d31',
    borderRadius: 14,
    padding: 16,
    marginBottom: 20,
  },
  panelTitle: {
    color: '#edf4ff',
    fontWeight: '700',
    marginBottom: 8,
  },
  panelBody: {
    color: '#9cb4d2',
    lineHeight: 20,
  },
});
