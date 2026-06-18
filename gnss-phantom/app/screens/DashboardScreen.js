/*
 * screens/DashboardScreen.js — Main dashboard for GNSS-Phantom
 *
 * Shows device connection status, engine state, battery, active SV count,
 * and provides quick action buttons for arm/disarm/start/stop spoofing.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert,
} from 'react-native';
import StatusIndicator from '../components/StatusIndicator';
import SafetyInterlock from '../components/SafetyInterlock';
import { bleManager } from '../utils/ble';
import {
  cmdGetStatus, cmdArm, cmdDisarm,
  cmdStartSpoof, cmdStopSpoof,
  ENGINE_STATE,
} from '../utils/protocol';

export default function DashboardScreen({ navigation }) {
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState(null);
  const [showInterlock, setShowInterlock] = useState(false);
  const [pendingAction, setPendingAction] = useState(null);

  useEffect(() => {
    bleManager.onStatus((newStatus) => {
      setStatus(newStatus);
      setConnected(bleManager.connected);
    });

    bleManager.startStatusPolling(2000);

    return () => {
      bleManager.stopStatusPolling();
    };
  }, []);

  const handleAction = (action) => {
    setPendingAction(action);
    setShowInterlock(true);
  };

  const onInterlockConfirm = async () => {
    setShowInterlock(false);
    if (pendingAction === 'start') {
      await bleManager.send(cmdStartSpoof());
    } else if (pendingAction === 'arm') {
      await bleManager.send(cmdArm());
    }
    setPendingAction(null);
  };

  const refreshStatus = async () => {
    if (bleManager.connected) {
      await bleManager.send(cmdGetStatus());
    }
  };

  const engineState = status ? status.engineStateName : 'DISCONNECTED';
  const isTransmitting = engineState === 'TRANSMITTING';
  const isArmed = engineState === 'ARMED' || isTransmitting;

  return (
    <ScrollView style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>GNSS-Phantom</Text>
        <Text style={styles.subtitle}>GPX-1 · jayis1</Text>
      </View>

      {/* Connection status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CONNECTION</Text>
        <View style={styles.statusRow}>
          <StatusIndicator
            label={connected ? 'BLE Connected' : 'Disconnected'}
            color={connected ? 'green' : 'red'}
            active={connected}
          />
        </View>
      </View>

      {/* Engine state */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>ENGINE STATE</Text>
        <View style={styles.stateCard}>
          <Text style={styles.stateLabel}>State:</Text>
          <Text style={[styles.stateValue,
            { color: isTransmitting ? '#ef4444' : (isArmed ? '#eab308' : '#22c55e') }]}>
            {engineState}
          </Text>
        </View>
      </View>

      {/* Telemetry */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>TELEMETRY</Text>
        <View style={styles.telemetryRow}>
          <Text style={styles.tLabel}>Active SVs:</Text>
          <Text style={styles.tValue}>{status?.svCount ?? '--'}</Text>
        </View>
        <View style={styles.telemetryRow}>
          <Text style={styles.tLabel}>Battery:</Text>
          <Text style={[styles.tValue,
            { color: (status?.batteryMv ?? 4200) < 3300 ? '#ef4444' : '#22c55e' }]}>
            {status ? `${(status.batteryMv / 1000).toFixed(2)}V` : '--'}
          </Text>
        </View>
        <View style={styles.telemetryRow}>
          <Text style={styles.tLabel}>Temperature:</Text>
          <Text style={styles.tValue}>{status ? `${status.tempC}°C` : '--'}</Text>
        </View>
        <View style={styles.telemetryRow}>
          <Text style={styles.tLabel}>Safety Switch:</Text>
          <Text style={[styles.tValue,
            { color: status?.safetyEngaged ? '#ef4444' : '#22c55e' }]}>
            {status ? (status.safetyEngaged ? 'LIVE' : 'SAFE') : '--'}
          </Text>
        </View>
        <View style={styles.telemetryRow}>
          <Text style={styles.tLabel}>Armed:</Text>
          <Text style={[styles.tValue, { color: status?.armed ? '#eab308' : '#888' }]}>
            {status ? (status.armed ? 'YES' : 'NO') : '--'}
          </Text>
        </View>
      </View>

      {/* Quick actions */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>QUICK ACTIONS</Text>

        {!isArmed ? (
          <TouchableOpacity
            style={[styles.actionBtn, { backgroundColor: '#eab308' }]}
            onPress={() => handleAction('arm')}
            disabled={!connected}
          >
            <Text style={styles.actionText}>⚠ ARM DEVICE</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity
            style={[styles.actionBtn, { backgroundColor: '#6b7280' }]}
            onPress={async () => { await bleManager.send(cmdDisarm()); }}
            disabled={!connected}
          >
            <Text style={styles.actionText}>DISARM</Text>
          </TouchableOpacity>
        )}

        {isArmed && !isTransmitting && (
          <TouchableOpacity
            style={[styles.actionBtn, { backgroundColor: '#ef4444' }]}
            onPress={() => handleAction('start')}
            disabled={!connected}
          >
            <Text style={styles.actionText}>⛔ START SPOOFING</Text>
          </TouchableOpacity>
        )}

        {isTransmitting && (
          <TouchableOpacity
            style={[styles.actionBtn, { backgroundColor: '#22c55e' }]}
            onPress={async () => { await bleManager.send(cmdStopSpoof()); }}
            disabled={!connected}
          >
            <Text style={styles.actionText}>STOP SPOOFING</Text>
          </TouchableOpacity>
        )}

        <TouchableOpacity style={styles.refreshBtn} onPress={refreshStatus}>
          <Text style={styles.refreshText}>↻ Refresh Status</Text>
        </TouchableOpacity>
      </View>

      {/* Navigation */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CONFIGURE</Text>
        <TouchableOpacity
          style={styles.navBtn}
          onPress={() => navigation.navigate('Satellites')}
        >
          <Text style={styles.navText}>📡 Satellite Configuration →</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navBtn}
          onPress={() => navigation.navigate('Trajectory')}
        >
          <Text style={styles.navText}>🗺️ Trajectory & Position →</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navBtn}
          onPress={() => navigation.navigate('RFSettings')}
        >
          <Text style={styles.navText}>📶 RF Settings →</Text>
        </TouchableOpacity>
      </View>

      {/* Disclaimer */}
      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠ AUTHORIZED SECURITY RESEARCH USE ONLY{'\n'}
          GNSS spoofing outside authorized RF-shielded{'\n'}
          environments is ILLEGAL and dangerous.{'\n'}
          User assumes all legal responsibility.{'\n'}
          © jayis1 — v1.0.0
        </Text>
      </View>

      <SafetyInterlock
        visible={showInterlock}
        onConfirm={onInterlockConfirm}
        onCancel={() => { setShowInterlock(false); setPendingAction(null); }}
        steps={[
          'I am in an authorized RF-shielded environment',
          'I confirm all required legal permissions are obtained',
          'I understand this will transmit GNSS spoofing signals',
          'PROCEED WITH TRANSMISSION',
        ]}
      />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#3b82f6', fontSize: 24, fontWeight: 'bold', fontFamily: 'monospace' },
  subtitle: { color: '#888', fontSize: 12, marginTop: 4 },
  section: { margin: 10, padding: 15, backgroundColor: '#1a1a2e', borderRadius: 8 },
  sectionTitle: { color: '#3b82f6', fontSize: 11, fontWeight: 'bold', marginBottom: 10, letterSpacing: 1 },
  statusRow: { flexDirection: 'row', flexWrap: 'wrap' },
  stateCard: { flexDirection: 'row', justifyContent: 'space-between', padding: 10 },
  stateLabel: { color: '#888', fontSize: 14 },
  stateValue: { fontSize: 16, fontWeight: 'bold', fontFamily: 'monospace' },
  telemetryRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  tLabel: { color: '#aaa', fontSize: 13 },
  tValue: { color: '#fff', fontSize: 13, fontFamily: 'monospace' },
  actionBtn: { padding: 15, borderRadius: 8, marginVertical: 5, alignItems: 'center' },
  actionText: { color: '#fff', fontSize: 16, fontWeight: 'bold', fontFamily: 'monospace' },
  refreshBtn: { padding: 10, marginTop: 5, alignItems: 'center' },
  refreshText: { color: '#3b82f6', fontSize: 14 },
  navBtn: { padding: 12, backgroundColor: '#2a2a3e', borderRadius: 6, marginVertical: 4 },
  navText: { color: '#e0e0e0', fontSize: 14 },
  disclaimer: { margin: 10, padding: 15, backgroundColor: '#2a0a0a', borderRadius: 8, borderWidth: 1, borderColor: '#ef4444' },
  disclaimerText: { color: '#fbbf24', fontSize: 10, textAlign: 'center', lineHeight: 16, fontFamily: 'monospace' },
});