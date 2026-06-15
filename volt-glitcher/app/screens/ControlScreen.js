/**
 * ControlScreen.js — Main glitch control screen
 * Arm/disarm/fire controls and real-time status display
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext, useCallback, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Alert, Vibration, Platform,
} from 'react-native';
import { DeviceContext } from '../App';
import { DEVICE_STATES, EVENT_TYPES } from '../utils/protocol';

/* ============================================================================
 * Status Card Component
 * ============================================================================ */

function StatusCard({ title, value, unit = '', color = '#fff' }) {
  return (
    <View style={styles.statusCard}>
      <Text style={styles.statusCardTitle}>{title}</Text>
      <Text style={[styles.statusCardValue, { color }]}>{value}</Text>
      {unit ? <Text style={styles.statusCardUnit}>{unit}</Text> : null}
    </View>
  );
}

/* ============================================================================
 * Event Log Component
 * ============================================================================ */

function EventLog({ events }) {
  const recentEvents = events.slice(-20);

  const getEventLabel = (type) => {
    switch (type) {
      case EVENT_TYPES.TRIGGER_DETECTED: return '🎯 Trigger';
      case EVENT_TYPES.GLITCH_FIRED:    return '⚡ Glitch';
      case EVENT_TYPES.FAULT:           return '⚠️ Fault';
      case EVENT_TYPES.CALIBRATION_DONE: return '✅ Cal Done';
      case EVENT_TYPES.OVERCURRENT:     return '🔴 Overcurrent';
      case EVENT_TYPES.OVERTEMP:        return '🌡 Overtemp';
      case EVENT_TYPES.FPGA_READY:      return '💚 FPGA Ready';
      default: return `❓ Event ${type}`;
    }
  };

  return (
    <View style={styles.eventSection}>
      <Text style={styles.sectionTitle}>Event Log</Text>
      <ScrollView style={styles.eventScroll} nestedScrollEnabled>
        {recentEvents.length === 0 && (
          <Text style={styles.eventEmpty}>No events yet</Text>
        )}
        {recentEvents.map((evt, i) => (
          <View key={i} style={styles.eventRow}>
            <Text style={styles.eventLabel}>{getEventLabel(evt.type)}</Text>
            <Text style={styles.eventTime}>
              {evt.timestamp ? (evt.timestamp / 1000).toFixed(1) + 's' : '--'}
            </Text>
          </View>
        ))}
      </ScrollView>
    </View>
  );
}

/* ============================================================================
 * Control Buttons
 * ============================================================================ */

function ControlButtons() {
  const { deviceState, actions } = useContext(DeviceContext);

  const isConnected = deviceState !== DEVICE_STATES.DISCONNECTED;
  const isArmed = deviceState === DEVICE_STATES.ARMED;
  const isIdle = deviceState === DEVICE_STATES.IDLE || deviceState === DEVICE_STATES.CONNECTED;

  return (
    <View style={styles.buttonGrid}>
      {/* ARM button */}
      <TouchableOpacity
        style={[
          styles.armBtn,
          !isIdle && styles.btnDisabled,
          isArmed && styles.btnDisabled,
        ]}
        onPress={actions.arm}
        disabled={!isIdle}
      >
        <Text style={styles.armBtnText}>
          {isArmed ? '🟡 ARMED' : '🟢 ARM'}
        </Text>
      </TouchableOpacity>

      {/* DISARM button */}
      <TouchableOpacity
        style={[
          styles.disarmBtn,
          !isArmed && styles.btnDisabled,
        ]}
        onPress={actions.disarm}
        disabled={!isArmed}
      >
        <Text style={styles.disarmBtnText}>🔴 DISARM</Text>
      </TouchableOpacity>

      {/* FIRE button */}
      <TouchableOpacity
        style={[
          styles.fireBtn,
          !isArmed && styles.btnDisabled,
        ]}
        onPress={() => {
          Vibration.vibrate(50);
          actions.fire();
        }}
        disabled={!isArmed}
      >
        <Text style={styles.fireBtnText}>⚡ FIRE</Text>
      </TouchableOpacity>

      {/* MARK SUCCESS button */}
      <TouchableOpacity
        style={[styles.successBtn, !isArmed && styles.btnDisabled]}
        onPress={actions.markSuccess}
        disabled={!isArmed}
      >
        <Text style={styles.successBtnText}>✅ Success</Text>
      </TouchableOpacity>

      {/* CALIBRATE button */}
      <TouchableOpacity
        style={[styles.calBtn, !isIdle && styles.btnDisabled]}
        onPress={actions.calibrate}
        disabled={!isIdle}
      >
        <Text style={styles.calBtnText}>🔧 Calibrate</Text>
      </TouchableOpacity>

      {/* RESET button */}
      <TouchableOpacity
        style={styles.resetBtn}
        onPress={() => {
          Alert.alert(
            'Reset Device',
            'This will disarm and reset all outputs. Continue?',
            [
              { text: 'Cancel', style: 'cancel' },
              { text: 'Reset', style: 'destructive', onPress: actions.resetDevice },
            ]
          );
        }}
      >
        <Text style={styles.resetBtnText}>🔄 Reset</Text>
      </TouchableOpacity>
    </View>
  );
}

/* ============================================================================
 * Main ControlScreen Component
 * ============================================================================ */

function ControlScreen() {
  const { deviceState, results, events, actions, deviceInfo } = useContext(DeviceContext);
  const [adcValues, setAdcValues] = useState({ vcc: 0, current: 0, em: 0 });

  /* Periodically read ADC values for display */
  useEffect(() => {
    const interval = setInterval(async () => {
      if (deviceState === DEVICE_STATES.DISCONNECTED) return;
      try {
        const vcc = await actions.readADC(0);
        const current = await actions.readADC(6);
        const em = await actions.readADC(7);
        if (vcc) setAdcValues((prev) => ({ ...prev, vcc: vcc.millivolts }));
        if (current) setAdcValues((prev) => ({ ...prev, current: current.raw }));
        if (em) setAdcValues((prev) => ({ ...prev, em: em.millivolts }));
      } catch (e) { /* Ignore read errors */ }
    }, 1000);

    return () => clearInterval(interval);
  }, [deviceState, actions]);

  /* Vibrate on glitch events */
  useEffect(() => {
    if (events.length > 0) {
      const last = events[events.length - 1];
      if (last.type === EVENT_TYPES.GLITCH_FIRED) {
        Vibration.vibrate(100);
      }
      if (last.type === EVENT_TYPES.OVERCURRENT || last.type === EVENT_TYPES.FAULT) {
        Vibration.vibrate([200, 100, 200]);
      }
    }
  }, [events]);

  const isConnected = deviceState !== DEVICE_STATES.DISCONNECTED;

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.scrollContent}>
      {/* Connection status */}
      {!isConnected && (
        <TouchableOpacity style={styles.connectBtn} onPress={actions.connect}>
          <Text style={styles.connectBtnText}>🔌 Connect Device</Text>
        </TouchableOpacity>
      )}

      {isConnected && (
        <>
          {/* Status dashboard */}
          <View style={styles.dashboard}>
            <Text style={styles.dashTitle}>Device Status</Text>

            <View style={styles.statusRow}>
              <StatusCard
                title="FPGA"
                value={deviceInfo?.fpgaReady ? 'Ready' : 'N/A'}
                color={deviceInfo?.fpgaReady ? '#55efc4' : '#ff6b6b'}
              />
              <StatusCard
                title="State"
                value={deviceState === DEVICE_STATES.ARMED ? 'ARMED' : 'IDLE'}
                color={deviceState === DEVICE_STATES.ARMED ? '#ffeaa7' : '#a0a0b0'}
              />
              <StatusCard
                title="Temp"
                value={results?.temperature ?? '--'}
                unit="°C"
                color={(results?.temperature ?? 0) > 70 ? '#ff6b6b' : '#55efc4'}
              />
            </View>

            <View style={styles.statusRow}>
              <StatusCard
                title="Target VCC"
                value={adcValues.vcc || results?.targetVccMv || '--'}
                unit="mV"
                color="#00cec9"
              />
              <StatusCard
                title="Current"
                value={results?.shuntCurrentMa ?? '--'}
                unit="mA"
                color={Math.abs(results?.shuntCurrentMa ?? 0) > 400 ? '#ff6b6b' : '#00cec9'}
              />
              <StatusCard
                title="Faults"
                value={results?.faultFlags ?? 0}
                color={results?.faultFlags ? '#ff6b6b' : '#55efc4'}
              />
            </View>
          </View>

          {/* Glitch counters */}
          <View style={styles.counterSection}>
            <Text style={styles.sectionTitle}>Glitch Counters</Text>
            <View style={styles.counterRow}>
              <View style={styles.counter}>
                <Text style={styles.counterValue}>{results?.glitchCount ?? 0}</Text>
                <Text style={styles.counterLabel}>Total Glitches</Text>
              </View>
              <View style={styles.counter}>
                <Text style={styles.counterValue}>{results?.triggerCount ?? 0}</Text>
                <Text style={styles.counterLabel}>Triggers</Text>
              </View>
              <View style={styles.counter}>
                <Text style={[styles.counterValue, { color: '#55efc4' }]}>
                  {results?.successCount ?? 0}
                </Text>
                <Text style={styles.counterLabel}>Successes</Text>
              </View>
            </View>

            {/* Success rate */}
            {results && results.glitchCount > 0 && (
              <Text style={styles.successRate}>
                Success rate: {((results.successCount / results.glitchCount) * 100).toFixed(1)}%
              </Text>
            )}
          </View>

          {/* Control buttons */}
          <ControlButtons />

          {/* Event log */}
          <EventLog events={events} />
        </>
      )}
    </ScrollView>
  );
}

/* ============================================================================
 * Styles
 * ============================================================================ */

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0f' },
  scrollContent: { padding: 16, paddingBottom: 40 },
  connectBtn: {
    backgroundColor: '#6c5ce7', borderRadius: 12, padding: 20,
    alignItems: 'center', marginVertical: 40,
  },
  connectBtnText: { color: '#fff', fontSize: 18, fontWeight: '700' },
  dashboard: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16, marginBottom: 12,
  },
  dashTitle: { color: '#6c5ce7', fontSize: 16, fontWeight: '700', marginBottom: 12 },
  statusRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  statusCard: {
    flex: 1, backgroundColor: '#252540', borderRadius: 8,
    padding: 10, marginHorizontal: 3, alignItems: 'center',
  },
  statusCardTitle: { color: '#666', fontSize: 11, marginBottom: 4 },
  statusCardValue: { fontSize: 16, fontWeight: '700' },
  statusCardUnit: { color: '#555', fontSize: 10, marginTop: 2 },
  counterSection: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16, marginBottom: 12,
  },
  sectionTitle: { color: '#6c5ce7', fontSize: 16, fontWeight: '700', marginBottom: 12 },
  counterRow: { flexDirection: 'row', justifyContent: 'space-around' },
  counter: { alignItems: 'center' },
  counterValue: { color: '#fff', fontSize: 24, fontWeight: '700' },
  counterLabel: { color: '#666', fontSize: 11, marginTop: 4 },
  successRate: { color: '#55efc4', fontSize: 14, textAlign: 'center', marginTop: 8 },
  buttonGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8, marginBottom: 12 },
  armBtn: {
    flex: 1, minWidth: '45%', backgroundColor: '#27ae60', borderRadius: 10,
    paddingVertical: 16, alignItems: 'center',
  },
  armBtnText: { color: '#fff', fontSize: 16, fontWeight: '700' },
  disarmBtn: {
    flex: 1, minWidth: '45%', backgroundColor: '#c0392b', borderRadius: 10,
    paddingVertical: 16, alignItems: 'center',
  },
  disarmBtnText: { color: '#fff', fontSize: 16, fontWeight: '700' },
  fireBtn: {
    flex: 1, minWidth: '45%', backgroundColor: '#e74c3c', borderRadius: 10,
    paddingVertical: 20, alignItems: 'center', borderWidth: 2, borderColor: '#ff6b6b',
  },
  fireBtnText: { color: '#fff', fontSize: 20, fontWeight: '900' },
  successBtn: {
    flex: 1, minWidth: '45%', backgroundColor: '#2ecc71', borderRadius: 10,
    paddingVertical: 14, alignItems: 'center',
  },
  successBtnText: { color: '#fff', fontSize: 14, fontWeight: '600' },
  calBtn: {
    flex: 1, minWidth: '45%', backgroundColor: '#2980b9', borderRadius: 10,
    paddingVertical: 14, alignItems: 'center',
  },
  calBtnText: { color: '#fff', fontSize: 14, fontWeight: '600' },
  resetBtn: {
    flex: 1, minWidth: '45%', backgroundColor: '#555', borderRadius: 10,
    paddingVertical: 14, alignItems: 'center',
  },
  resetBtnText: { color: '#ccc', fontSize: 14 },
  btnDisabled: { opacity: 0.3 },
  eventSection: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16, marginBottom: 12,
  },
  eventScroll: { maxHeight: 200 },
  eventEmpty: { color: '#444', fontSize: 13, textAlign: 'center', padding: 20 },
  eventRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    paddingVertical: 4, borderBottomWidth: 1, borderBottomColor: '#252540',
  },
  eventLabel: { color: '#ccc', fontSize: 13 },
  eventTime: { color: '#666', fontSize: 12 },
});

export default ControlScreen;