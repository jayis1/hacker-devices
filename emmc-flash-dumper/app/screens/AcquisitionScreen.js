/**
 * AcquisitionScreen.js — Flash Acquisition Control Interface
 *
 * Provides UI for selecting flash type (eMMC/NAND/SPI NOR), configuring
 * acquisition parameters, starting/stopping acquisition, and monitoring
 * real-time progress with data rate and ETA display.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useContext, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  ActivityIndicator,
  TextInput,
  Switch,
  Alert,
} from 'react-native';
import { DeviceContext } from '../components/DeviceContext';
import { CMD, ERR, ERROR_STRINGS } from '../utils/protocol';

/*===========================================================================
 * Flash Type Definitions
 *===========================================================================*/

const FLASH_TYPES = [
  {
    id: 'emmc',
    label: 'eMMC 5.1',
    icon: '💾',
    description: 'JESD84-B51 eMMC flash via HS400 8-bit DDR bus',
    color: '#00ff88',
  },
  {
    id: 'nand',
    label: 'NAND Flash',
    icon: '🧩',
    description: 'ONFI 4.0 / Toggle 2.0 parallel NAND via FPGA',
    color: '#ff8800',
  },
  {
    id: 'spinor',
    label: 'SPI NOR',
    icon: '📌',
    description: 'Quad SPI NOR flash at 100 MHz SDR',
    color: '#0088ff',
  },
];

const ACQUISITION_MODES = [
  { id: 'full', label: 'Full Dump', description: 'Read entire flash device' },
  { id: 'partial', label: 'Partial', description: 'Read specific block range' },
  { id: 'boot', label: 'Boot Partitions', description: 'Read boot0/boot1 only' },
];

/*===========================================================================
 * AcquisitionScreen Component
 *===========================================================================*/

export default function AcquisitionScreen() {
  const device = useContext(DeviceContext);
  const [selectedFlash, setSelectedFlash] = useState('emmc');
  const [selectedMode, setSelectedMode] = useState('full');
  const [flashInfo, setFlashInfo] = useState(null);
  const [detecting, setDetecting] = useState(false);
  const [acquiring, setAcquiring] = useState(false);
  const [progress, setProgress] = useState(0);
  const [bytesReceived, setBytesReceived] = useState(0);
  const [totalBytes, setTotalBytes] = useState(0);
  const [dataRate, setDataRate] = useState(0);
  const [eta, setEta] = useState('--');
  const [startBlock, setStartBlock] = useState('0');
  const [blockCount, setBlockCount] = useState('0');
  const [saveToSd, setSaveToSd] = useState(false);
  const [verifyHash, setVerifyHash] = useState(true);
  const [errorMessage, setErrorMessage] = useState(null);
  const [startTime, setStartTime] = useState(null);
  const [lastUpdateTime, setLastUpdateTime] = useState(null);
  const [lastBytes, setLastBytes] = useState(0);

  /*=======================================================================
   * Flash Detection
   *=======================================================================*/

  const detectFlash = useCallback(async () => {
    if (!device.connected) {
      Alert.alert('Not Connected', 'Please connect the eMMC Flash Dumper first.');
      return;
    }

    setDetecting(true);
    setFlashInfo(null);
    setErrorMessage(null);

    try {
      let info;
      switch (selectedFlash) {
        case 'emmc':
          info = await device.protocol.detectEmmc();
          break;
        case 'nand':
          info = await device.protocol.detectNand();
          break;
        case 'spinor':
          info = await device.protocol.detectSpiNor();
          break;
      }
      setFlashInfo(info);
    } catch (e) {
      setErrorMessage(e.message || 'Detection failed');
      Alert.alert('Detection Failed', e.message);
    } finally {
      setDetecting(false);
    }
  }, [device.connected, device.protocol, selectedFlash]);

  /*=======================================================================
   * Acquisition Start/Stop
   *=======================================================================*/

  const startAcquisition = useCallback(async () => {
    if (!device.connected || !flashInfo) {
      Alert.alert('Not Ready', 'Detect flash first.');
      return;
    }

    setAcquiring(true);
    setProgress(0);
    setBytesReceived(0);
    setErrorMessage(null);
    setStartTime(Date.now());
    setLastUpdateTime(Date.now());
    setLastBytes(0);

    const onProgress = (p) => {
      setBytesReceived(p.bytesReceived);
      setTotalBytes(p.totalBlocks * 512);
      setProgress(p.totalBlocks > 0
        ? (p.blocksReceived / p.totalBlocks * 100)
        : 0);

      /* Calculate data rate */
      const now = Date.now();
      const elapsed = (now - lastUpdateTime) / 1000;
      if (elapsed > 0.5) {
        const newBytes = p.bytesReceived - lastBytes;
        setDataRate(newBytes / elapsed);
        setLastUpdateTime(now);
        setLastBytes(p.bytesReceived);

        /* Calculate ETA */
        if (p.totalBlocks > 0 && p.blocksReceived > 0) {
          const totalElapsed = (now - startTime) / 1000;
          const bytesPerSec = p.bytesReceived / totalElapsed;
          const remaining = (p.totalBlocks * 512) - p.bytesReceived;
          const etaSec = remaining / bytesPerSec;
          if (etaSec < 60) {
            setEta(`${Math.ceil(etaSec)}s`);
          } else if (etaSec < 3600) {
            setEta(`${Math.ceil(etaSec / 60)}m`);
          } else {
            setEta(`${(etaSec / 3600).toFixed(1)}h`);
          }
        }
      }
    };

    try {
      let result;
      switch (selectedFlash) {
        case 'emmc': {
          const start = parseInt(startBlock) || 0;
          const count = selectedMode === 'full'
            ? flashInfo.capacityBlocks
            : (parseInt(blockCount) || flashInfo.capacityBlocks);
          result = await device.protocol.startEmmcAcquire(start, count, onProgress);
          break;
        }
        case 'nand':
          result = await device.protocol.startNandAcquire(onProgress);
          break;
        case 'spinor':
          /* SPI NOR acquisition */
          result = { data: null, sha256: null, totalBytes: 0 };
          break;
      }

      /* Verify hash if requested */
      if (verifyHash && result.sha256) {
        const hashResult = await device.protocol.startHash();
        const match = Buffer.from(result.sha256).equals(Buffer.from(hashResult.digest));
        if (!match) {
          Alert.alert('Hash Mismatch', 'SHA-256 verification failed! Data may be corrupted.');
        }
      }

      /* Save to SD if requested */
      if (saveToSd && result.data) {
        await device.protocol.sendCommand(CMD.SDCARD_START);
        /* Data streaming to SD handled by firmware */
      }

      Alert.alert(
        'Acquisition Complete',
        `${(result.totalBytes / (1024 * 1024)).toFixed(1)} MB acquired\n` +
        `SHA-256: ${result.sha256 ? Buffer.from(result.sha256).toString('hex').slice(0, 16) + '...' : 'N/A'}`
      );
    } catch (e) {
      setErrorMessage(e.message || 'Acquisition failed');
      Alert.alert('Acquisition Failed', e.message);
    } finally {
      setAcquiring(false);
    }
  }, [device.connected, device.protocol, flashInfo, selectedFlash, selectedMode,
      startBlock, blockCount, verifyHash, saveToSd, startTime, lastUpdateTime, lastBytes]);

  const abortAcquisition = useCallback(async () => {
    try {
      await device.protocol.abortEmmcAcquire();
      setAcquiring(false);
    } catch (e) {
      /* Ignore */
    }
  }, [device.protocol]);

  /*=======================================================================
   * Format Helpers
   *=======================================================================*/

  const formatBytes = (bytes) => {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
    return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
  };

  const formatRate = (bytesPerSec) => {
    if (bytesPerSec < 1024) return `${bytesPerSec.toFixed(0)} B/s`;
    if (bytesPerSec < 1024 * 1024) return `${(bytesPerSec / 1024).toFixed(1)} KB/s`;
    return `${(bytesPerSec / (1024 * 1024)).toFixed(1)} MB/s`;
  };

  /*=======================================================================
   * Render
   *=======================================================================*/

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Status */}
      <View style={styles.statusBar}>
        <View style={[styles.statusDot, device.connected ? styles.dotLive : styles.dotOffline]} />
        <Text style={styles.statusText}>
          {device.connected ? 'DEVICE CONNECTED' : 'DEVICE OFFLINE'}
        </Text>
        {device.connected && (
          <Text style={styles.batteryText}>
            {(device.batteryMv / 1000).toFixed(2)}V
          </Text>
        )}
      </View>

      {/* Flash Type Selection */}
      <Text style={styles.sectionTitle}>FLASH TYPE</Text>
      <View style={styles.flashTypeRow}>
        {FLASH_TYPES.map((ft) => (
          <TouchableOpacity
            key={ft.id}
            style={[
              styles.flashTypeBtn,
              selectedFlash === ft.id && { borderColor: ft.color, backgroundColor: ft.color + '20' },
            ]}
            onPress={() => { setSelectedFlash(ft.id); setFlashInfo(null); }}
            disabled={acquiring}
          >
            <Text style={styles.flashIcon}>{ft.icon}</Text>
            <Text style={[styles.flashLabel, selectedFlash === ft.id && { color: ft.color }]}>
              {ft.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Acquisition Mode */}
      <Text style={styles.sectionTitle}>MODE</Text>
      <View style={styles.modeRow}>
        {ACQUISITION_MODES.map((mode) => (
          <TouchableOpacity
            key={mode.id}
            style={[styles.modeBtn, selectedMode === mode.id && styles.modeBtnActive]}
            onPress={() => setSelectedMode(mode.id)}
            disabled={acquiring}
          >
            <Text style={[styles.modeLabel, selectedMode === mode.id && styles.modeLabelActive]}>
              {mode.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Partial Range Inputs */}
      {selectedMode === 'partial' && (
        <View style={styles.rangeRow}>
          <View style={styles.rangeInput}>
            <Text style={styles.rangeLabel}>Start Block</Text>
            <TextInput
              style={styles.textInput}
              value={startBlock}
              onChangeText={setStartBlock}
              keyboardType="numeric"
              placeholder="0"
              editable={!acquiring}
            />
          </View>
          <View style={styles.rangeInput}>
            <Text style={styles.rangeLabel}>Block Count</Text>
            <TextInput
              style={styles.textInput}
              value={blockCount}
              onChangeText={setBlockCount}
              keyboardType="numeric"
              placeholder="All"
              editable={!acquiring}
            />
          </View>
        </View>
      )}

      {/* Options */}
      <Text style={styles.sectionTitle}>OPTIONS</Text>
      <View style={styles.optionsRow}>
        <View style={styles.optionItem}>
          <Text style={styles.optionLabel}>Save to SD</Text>
          <Switch
            value={saveToSd}
            onValueChange={setSaveToSd}
            disabled={acquiring || !device.deviceInfo?.sdCardPresent}
            trackColor={{ false: '#333', true: '#00ff8840' }}
            thumbColor={saveToSd ? '#00ff88' : '#666'}
          />
        </View>
        <View style={styles.optionItem}>
          <Text style={styles.optionLabel}>Verify Hash</Text>
          <Switch
            value={verifyHash}
            onValueChange={setVerifyHash}
            disabled={acquiring}
            trackColor={{ false: '#333', true: '#00ff8840' }}
            thumbColor={verifyHash ? '#00ff88' : '#666'}
          />
        </View>
      </View>

      {/* Flash Info */}
      {flashInfo && (
        <View style={styles.infoCard}>
          <Text style={styles.infoTitle}>DEVICE INFORMATION</Text>
          {selectedFlash === 'emmc' && (
            <>
              <InfoRow label="Capacity" value={formatBytes(flashInfo.capacityBytes)} />
              <InfoRow label="Blocks" value={flashInfo.capacityBlocks?.toLocaleString()} />
              <InfoRow label="HS400" value={flashInfo.hs400Supported ? 'Yes' : 'No'} />
              <InfoRow label="Boot0" value={formatBytes(flashInfo.boot0Size * 512)} />
              <InfoRow label="Boot1" value={formatBytes(flashInfo.boot1Size * 512)} />
              <InfoRow label="RPMB" value={formatBytes(flashInfo.rpmbSize * 512)} />
              <InfoRow label="Life Est." value={`${flashInfo.lifeEstA} / ${flashInfo.lifeEstB}`} />
            </>
          )}
          {selectedFlash === 'nand' && (
            <>
              <InfoRow label="Mfr ID" value={`0x${flashInfo.manufacturerId?.toString(16)}`} />
              <InfoRow label="Page Size" value={formatBytes(flashInfo.pageSize)} />
              <InfoRow label="OOB Size" value={`${flashInfo.oobSize} B`} />
              <InfoRow label="Pages/Block" value={flashInfo.pagesPerBlock?.toLocaleString()} />
              <InfoRow label="Total Size" value={formatBytes(flashInfo.totalSizeBytes)} />
              <InfoRow label="ONFI" value={flashInfo.onfiCompliant ? 'Yes' : 'No'} />
            </>
          )}
          {selectedFlash === 'spinor' && (
            <>
              <InfoRow label="Mfr ID" value={`0x${flashInfo.manufacturerId?.toString(16)}`} />
              <InfoRow label="Capacity" value={formatBytes(flashInfo.sizeBytes)} />
              <InfoRow label="Quad SPI" value={flashInfo.quadSpiSupported ? 'Yes' : 'No'} />
            </>
          )}
        </View>
      )}

      {/* Progress */}
      {acquiring && (
        <View style={styles.progressCard}>
          <Text style={styles.progressTitle}>ACQUISITION IN PROGRESS</Text>
          <View style={styles.progressBarBg}>
            <View style={[styles.progressBarFill, { width: `${Math.min(progress, 100)}%` }]} />
          </View>
          <Text style={styles.progressPercent}>{progress.toFixed(1)}%</Text>
          <View style={styles.progressStats}>
            <StatItem label="Received" value={formatBytes(bytesReceived)} />
            <StatItem label="Total" value={formatBytes(totalBytes)} />
            <StatItem label="Rate" value={formatRate(dataRate)} />
            <StatItem label="ETA" value={eta} />
          </View>
        </View>
      )}

      {/* Error */}
      {errorMessage && (
        <View style={styles.errorCard}>
          <Text style={styles.errorTitle}>ERROR</Text>
          <Text style={styles.errorText}>{errorMessage}</Text>
        </View>
      )}

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        {!acquiring ? (
          <>
            <TouchableOpacity
              style={[styles.actionBtn, styles.detectBtn]}
              onPress={detectFlash}
              disabled={detecting || !device.connected}
            >
              {detecting ? (
                <ActivityIndicator color="#fff" size="small" />
              ) : (
                <Text style={styles.actionBtnText}>DETECT</Text>
              )}
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.actionBtn, styles.acquireBtn, !flashInfo && styles.btnDisabled]}
              onPress={startAcquisition}
              disabled={!flashInfo || !device.connected}
            >
              <Text style={styles.actionBtnText}>ACQUIRE</Text>
            </TouchableOpacity>
          </>
        ) : (
          <TouchableOpacity
            style={[styles.actionBtn, styles.abortBtn]}
            onPress={abortAcquisition}
          >
            <Text style={styles.actionBtnText}>ABORT</Text>
          </TouchableOpacity>
        )}
      </View>
    </ScrollView>
  );
}

/*===========================================================================
 * Sub-Components
 *===========================================================================*/

function InfoRow({ label, value }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
    </View>
  );
}

function StatItem({ label, value }) {
  return (
    <View style={styles.statItem}>
      <Text style={styles.statLabel}>{label}</Text>
      <Text style={styles.statValue}>{value}</Text>
    </View>
  );
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
  statusBar: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
    marginBottom: 16,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  dotLive: { backgroundColor: '#00ff88' },
  dotOffline: { backgroundColor: '#ff4444' },
  statusText: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#ccc',
    flex: 1,
  },
  batteryText: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#888',
  },
  sectionTitle: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#666',
    letterSpacing: 2,
    marginBottom: 8,
    marginTop: 16,
  },
  flashTypeRow: {
    flexDirection: 'row',
    gap: 8,
  },
  flashTypeBtn: {
    flex: 1,
    backgroundColor: '#111',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#222',
    padding: 12,
    alignItems: 'center',
  },
  flashIcon: {
    fontSize: 24,
    marginBottom: 4,
  },
  flashLabel: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#888',
  },
  modeRow: {
    flexDirection: 'row',
    gap: 8,
  },
  modeBtn: {
    flex: 1,
    backgroundColor: '#111',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#222',
    padding: 10,
    alignItems: 'center',
  },
  modeBtnActive: {
    borderColor: '#00ff88',
    backgroundColor: '#00ff8810',
  },
  modeLabel: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#888',
  },
  modeLabelActive: {
    color: '#00ff88',
  },
  rangeRow: {
    flexDirection: 'row',
    gap: 12,
    marginTop: 8,
  },
  rangeInput: {
    flex: 1,
  },
  rangeLabel: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#666',
    marginBottom: 4,
  },
  textInput: {
    backgroundColor: '#111',
    borderWidth: 1,
    borderColor: '#333',
    borderRadius: 6,
    padding: 8,
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#fff',
  },
  optionsRow: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
  },
  optionItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
  },
  optionLabel: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#ccc',
  },
  infoCard: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
    marginTop: 16,
    borderLeftWidth: 3,
    borderLeftColor: '#00ff88',
  },
  infoTitle: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#00ff88',
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
  progressCard: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
    marginTop: 16,
    borderLeftWidth: 3,
    borderLeftColor: '#ff8800',
  },
  progressTitle: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#ff8800',
    letterSpacing: 2,
    marginBottom: 8,
  },
  progressBarBg: {
    height: 8,
    backgroundColor: '#222',
    borderRadius: 4,
    overflow: 'hidden',
    marginBottom: 4,
  },
  progressBarFill: {
    height: '100%',
    backgroundColor: '#00ff88',
    borderRadius: 4,
  },
  progressPercent: {
    fontFamily: 'monospace',
    fontSize: 24,
    color: '#00ff88',
    textAlign: 'center',
    marginVertical: 4,
  },
  progressStats: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
    marginTop: 8,
  },
  statItem: {
    flex: 1,
    minWidth: '45%',
    backgroundColor: '#1a1a1a',
    borderRadius: 6,
    padding: 8,
    alignItems: 'center',
  },
  statLabel: {
    fontFamily: 'monospace',
    fontSize: 9,
    color: '#666',
  },
  statValue: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#fff',
    marginTop: 2,
  },
  errorCard: {
    backgroundColor: '#1a0000',
    borderRadius: 8,
    padding: 12,
    marginTop: 16,
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
    fontSize: 11,
    color: '#ff8888',
  },
  actionRow: {
    flexDirection: 'row',
    gap: 12,
    marginTop: 20,
  },
  actionBtn: {
    flex: 1,
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
  },
  detectBtn: {
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#333',
  },
  acquireBtn: {
    backgroundColor: '#003322',
    borderWidth: 1,
    borderColor: '#00ff88',
  },
  abortBtn: {
    backgroundColor: '#330000',
    borderWidth: 1,
    borderColor: '#ff4444',
  },
  btnDisabled: {
    opacity: 0.4,
  },
  actionBtnText: {
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
    color: '#fff',
    letterSpacing: 2,
  },
});
