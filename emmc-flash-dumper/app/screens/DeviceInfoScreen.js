/**
 * DeviceInfoScreen.js — Device Information & Diagnostics
 *
 * Displays detailed information about the connected eMMC Flash Dumper
 * hardware, including firmware version, hardware revision, self-test
 * results, FPGA status, and battery health.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  ActivityIndicator,
  Alert,
} from 'react-native';
import { DeviceContext } from '../components/DeviceContext';

/*===========================================================================
 * DeviceInfoScreen Component
 *===========================================================================*/

export default function DeviceInfoScreen() {
  const device = useContext(DeviceContext);
  const [selftestResult, setSelftestResult] = useState(null);
  const [selftestRunning, setSelftestRunning] = useState(false);
  const [fpgaStatus, setFpgaStatus] = useState(null);
  const [refreshing, setRefreshing] = useState(false);

  /*=======================================================================
   * Self-Test
   *=======================================================================*/

  const runSelftest = useCallback(async () => {
    if (!device.connected) {
      Alert.alert('Not Connected', 'Connect device to run self-test.');
      return;
    }

    setSelftestRunning(true);
    setSelftestResult(null);

    try {
      const result = await device.protocol.runSelfTest();
      setSelftestResult(result);
    } catch (e) {
      Alert.alert('Self-Test Failed', e.message);
    } finally {
      setSelftestRunning(false);
    }
  }, [device.connected, device.protocol]);

  /*=======================================================================
   * FPGA Status
   *=======================================================================*/

  const checkFpga = useCallback(async () => {
    if (!device.connected) return;

    try {
      const resp = await device.protocol.sendCommand(0x0071);  /* CMD_FPGA_STATUS */
      if (resp.valid) {
        setFpgaStatus({
          loaded: resp.payload[0] === 1,
          version: resp.payload.readUInt32LE(1),
          nandEnabled: resp.payload[5] === 1,
        });
      }
    } catch (e) {
      setFpgaStatus({ loaded: false, version: 0, nandEnabled: false });
    }
  }, [device.connected, device.protocol]);

  /*=======================================================================
   * Refresh Info
   *=======================================================================*/

  const refreshAll = useCallback(async () => {
    setRefreshing(true);
    try {
      await device.refreshInfo();
      await checkFpga();
    } catch (e) {
      /* Ignore */
    } finally {
      setRefreshing(false);
    }
  }, [device.refreshInfo, checkFpga]);

  /*=======================================================================
   * Render
   *=======================================================================*/

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Status */}
      <View style={styles.statusCard}>
        <View style={styles.statusHeader}>
          <View style={[styles.statusDot, device.connected ? styles.dotLive : styles.dotOffline]} />
          <Text style={styles.statusTitle}>
            {device.connected ? 'CONNECTED' : 'DISCONNECTED'}
          </Text>
        </View>
        {device.connected ? (
          <TouchableOpacity style={styles.disconnectBtn} onPress={device.disconnect}>
            <Text style={styles.disconnectBtnText}>DISCONNECT</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.connectBtn} onPress={device.connect}>
            {device.isScanning ? (
              <ActivityIndicator color="#fff" size="small" />
            ) : (
              <Text style={styles.connectBtnText}>SCAN FOR DEVICE</Text>
            )}
          </TouchableOpacity>
        )}
      </View>

      {/* Device Information */}
      {device.deviceInfo && (
        <View style={styles.infoCard}>
          <Text style={styles.cardTitle}>DEVICE INFORMATION</Text>
          <InfoRow label="Hardware Rev" value={`v${device.deviceInfo.hwRev}.0`} />
          <InfoRow label="Firmware" value={device.deviceInfo.fwVersion || '—'} />
          <InfoRow label="Battery" value={`${(device.batteryMv / 1000).toFixed(2)}V`} />
          <InfoRow
            label="Battery Level"
            value={getBatteryLevel(device.batteryMv)}
            valueColor={getBatteryColor(device.batteryMv)}
          />
          <InfoRow
            label="SD Card"
            value={device.deviceInfo.sdCardPresent ? 'Present' : 'Not Inserted'}
            valueColor={device.deviceInfo.sdCardPresent ? '#00ff88' : '#ff4444'}
          />
          <InfoRow
            label="USB"
            value={device.deviceInfo.usbConnected ? 'Connected' : 'Disconnected'}
          />
        </View>
      )}

      {/* FPGA Status */}
      {fpgaStatus && (
        <View style={styles.infoCard}>
          <Text style={styles.cardTitle}>FPGA STATUS</Text>
          <InfoRow
            label="Loaded"
            value={fpgaStatus.loaded ? 'Yes' : 'No'}
            valueColor={fpgaStatus.loaded ? '#00ff88' : '#ff4444'}
          />
          {fpgaStatus.loaded && (
            <>
              <InfoRow label="Version" value={`0x${fpgaStatus.version.toString(16)}`} />
              <InfoRow
                label="NAND Controller"
                value={fpgaStatus.nandEnabled ? 'Enabled' : 'Disabled'}
              />
            </>
          )}
        </View>
      )}

      {/* Self-Test Results */}
      {selftestResult && (
        <View style={styles.infoCard}>
          <Text style={styles.cardTitle}>SELF-TEST RESULTS</Text>
          <View style={styles.testSummary}>
            <Text style={[
              styles.testSummaryText,
              { color: selftestResult.errors === 0 ? '#00ff88' : '#ff4444' },
            ]}>
              {selftestResult.errors === 0 ? 'ALL TESTS PASSED' : `${selftestResult.errors} ERROR(S)`}
            </Text>
          </View>
          <TestRow label="SDRAM" passed={selftestResult.sdramOk} />
          <TestRow label="FPGA" passed={selftestResult.fpgaOk} />
          <TestRow label="OLED Display" passed={selftestResult.oledOk} />
          <TestRow label="SD Card" passed={selftestResult.sdCardOk} />
          <TestRow label="Battery" passed={selftestResult.batteryOk} />
        </View>
      )}

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.actionBtn, styles.refreshBtn]}
          onPress={refreshAll}
          disabled={refreshing || !device.connected}
        >
          {refreshing ? (
            <ActivityIndicator color="#fff" size="small" />
          ) : (
            <Text style={styles.actionBtnText}>REFRESH</Text>
          )}
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionBtn, styles.testBtn]}
          onPress={runSelftest}
          disabled={selftestRunning || !device.connected}
        >
          {selftestRunning ? (
            <ActivityIndicator color="#fff" size="small" />
          ) : (
            <Text style={styles.actionBtnText}>SELF-TEST</Text>
          )}
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionBtn, styles.fpgaBtn]}
          onPress={checkFpga}
          disabled={!device.connected}
        >
          <Text style={styles.actionBtnText}>FPGA</Text>
        </TouchableOpacity>
      </View>

      {/* Error Display */}
      {device.lastError && (
        <View style={styles.errorCard}>
          <Text style={styles.errorTitle}>LAST ERROR</Text>
          <Text style={styles.errorText}>
            Code: 0x{device.lastError.code?.toString(16)} — {device.lastError.message}
          </Text>
        </View>
      )}

      {/* Device Specs */}
      <View style={styles.specsCard}>
        <Text style={styles.cardTitle}>TECHNICAL SPECIFICATIONS</Text>
        <InfoRow label="MCU" value="STM32H743VI Cortex-M7 @ 480 MHz" />
        <InfoRow label="FPGA" value="Lattice iCE40UP5K (5280 LUTs)" />
        <InfoRow label="SDRAM" value="512 MB DDR3L (MT41K256M16TW)" />
        <InfoRow label="USB" value="USB 3.0 SuperSpeed (5 Gbps)" />
        <InfoRow label="eMMC" value="HS400, 200 MHz DDR, 8-bit" />
        <InfoRow label="NAND" value="ONFI 4.0 / Toggle 2.0, FPGA-timed" />
        <InfoRow label="SPI NOR" value="Quad SPI @ 100 MHz SDR" />
        <InfoRow label="Storage" value="microSD UHS-I SDR104" />
        <InfoRow label="Display" value="128×64 OLED (SSD1306)" />
        <InfoRow label="Battery" value="LiPo 2000 mAh, ~2.5h active" />
        <InfoRow label="PCB" value="4-layer, 85×54mm, ENIG" />
      </View>
    </ScrollView>
  );
}

/*===========================================================================
 * Sub-Components
 *===========================================================================*/

function InfoRow({ label, value, valueColor }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={[styles.infoValue, valueColor && { color: valueColor }]}>
        {value}
      </Text>
    </View>
  );
}

function TestRow({ label, passed }) {
  return (
    <View style={styles.testRow}>
      <Text style={styles.testLabel}>{label}</Text>
      <Text style={[styles.testResult, { color: passed ? '#00ff88' : '#ff4444' }]}>
        {passed ? '✓ PASS' : '✗ FAIL'}
      </Text>
    </View>
  );
}

/*===========================================================================
 * Helpers
 *===========================================================================*/

function getBatteryLevel(mv) {
  if (mv >= 4100) return 'Full';
  if (mv >= 3700) return 'Good';
  if (mv >= 3500) return 'Low';
  if (mv >= 3300) return 'Critical';
  return 'Shutdown';
}

function getBatteryColor(mv) {
  if (mv >= 3700) return '#00ff88';
  if (mv >= 3500) return '#ff8800';
  return '#ff4444';
}

/*===========================================================================
 * Styles
 *===========================================================================*/

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  statusCard: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 16,
    marginBottom: 16,
    alignItems: 'center',
  },
  statusHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 12,
  },
  statusDot: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginRight: 8,
  },
  dotLive: { backgroundColor: '#00ff88' },
  dotOffline: { backgroundColor: '#ff4444' },
  statusTitle: {
    fontFamily: 'monospace',
    fontSize: 16,
    fontWeight: 'bold',
    color: '#fff',
    letterSpacing: 2,
  },
  connectBtn: {
    backgroundColor: '#1a2e1a',
    borderRadius: 6,
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderWidth: 1,
    borderColor: '#00ff88',
  },
  connectBtnText: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#00ff88',
    letterSpacing: 1,
  },
  disconnectBtn: {
    backgroundColor: '#2e1a1a',
    borderRadius: 6,
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderWidth: 1,
    borderColor: '#ff4444',
  },
  disconnectBtnText: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#ff4444',
    letterSpacing: 1,
  },
  infoCard: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
    borderLeftWidth: 3,
    borderLeftColor: '#0088ff',
  },
  cardTitle: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#0088ff',
    letterSpacing: 2,
    marginBottom: 8,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 3,
  },
  infoLabel: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#888',
  },
  infoValue: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#fff',
  },
  testSummary: {
    backgroundColor: '#1a1a1a',
    borderRadius: 4,
    padding: 8,
    marginBottom: 8,
    alignItems: 'center',
  },
  testSummaryText: {
    fontFamily: 'monospace',
    fontSize: 13,
    fontWeight: 'bold',
    letterSpacing: 1,
  },
  testRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
    paddingHorizontal: 4,
  },
  testLabel: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#ccc',
  },
  testResult: {
    fontFamily: 'monospace',
    fontSize: 11,
    fontWeight: 'bold',
  },
  actionRow: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 16,
  },
  actionBtn: {
    flex: 1,
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    borderWidth: 1,
  },
  refreshBtn: {
    backgroundColor: '#1a1a2e',
    borderColor: '#333',
  },
  testBtn: {
    backgroundColor: '#1a2e1a',
    borderColor: '#00ff88',
  },
  fpgaBtn: {
    backgroundColor: '#2e1a1a',
    borderColor: '#ff8800',
  },
  actionBtnText: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#fff',
    letterSpacing: 1,
  },
  errorCard: {
    backgroundColor: '#1a0000',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
    borderLeftWidth: 3,
    borderLeftColor: '#ff4444',
  },
  errorTitle: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#ff4444',
    letterSpacing: 2,
    marginBottom: 4,
  },
  errorText: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#ff8888',
  },
  specsCard: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
    borderLeftWidth: 3,
    borderLeftColor: '#555',
  },
});
