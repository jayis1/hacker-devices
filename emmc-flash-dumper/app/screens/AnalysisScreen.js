/**
 * AnalysisScreen.js — Flash Data Analysis & Hex Viewer
 *
 * Provides hex dump viewing, partition table analysis, SHA-256
 * hash verification, and data export capabilities for acquired
 * flash images.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext, useCallback, useMemo } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  TextInput,
  FlatList,
  Share,
  Alert,
} from 'react-native';
import { DeviceContext } from '../components/DeviceContext';
import { Buffer } from 'buffer';

/*===========================================================================
 * Constants
 *===========================================================================*/

const BYTES_PER_LINE = 16;
const LINES_PER_PAGE = 32;
const MAX_VIRTUAL_LINES = 100000;

/*===========================================================================
 * AnalysisScreen Component
 *===========================================================================*/

export default function AnalysisScreen() {
  const device = useContext(DeviceContext);
  const [hexData, setHexData] = useState(null);
  const [hexOffset, setHexOffset] = useState(0);
  const [gotoOffset, setGotoOffset] = useState('');
  const [searchHex, setSearchHex] = useState('');
  const [searchResults, setSearchResults] = useState([]);
  const [currentSearchIdx, setCurrentSearchIdx] = useState(0);
  const [hashDigest, setHashDigest] = useState(null);
  const [hashVerifying, setHashVerifying] = useState(false);
  const [partitionInfo, setPartitionInfo] = useState(null);
  const [viewMode, setViewMode] = useState('hex');  /* 'hex' | 'partitions' | 'hash' */

  /*=======================================================================
   * Hex Data Loading
   *=======================================================================*/

  const loadSampleData = useCallback(() => {
    /* Generate sample data for UI demonstration */
    const sample = Buffer.alloc(4096);
    for (let i = 0; i < sample.length; i++) {
      sample[i] = i % 256;
    }
    /* Add some recognizable patterns */
    sample.write('eMMC Flash Dumper v1.0', 0x100, 'ascii');
    sample.writeUInt32LE(0x00000200, 0x1BE);  /* Partition signature */
    setHexData(sample);
    setHexOffset(0);
  }, []);

  /*=======================================================================
   * Hex Line Rendering
   *=======================================================================*/

  const hexLines = useMemo(() => {
    if (!hexData) return [];

    const lines = [];
    const startOffset = hexOffset;
    const endOffset = Math.min(startOffset + LINES_PER_PAGE * BYTES_PER_LINE, hexData.length);

    for (let offset = startOffset; offset < endOffset; offset += BYTES_PER_LINE) {
      const lineBytes = [];
      const lineChars = [];

      for (let i = 0; i < BYTES_PER_LINE; i++) {
        const byteOffset = offset + i;
        if (byteOffset < hexData.length) {
          const byte = hexData[byteOffset];
          lineBytes.push(byte.toString(16).padStart(2, '0').toUpperCase());
          lineChars.push(byte >= 0x20 && byte < 0x7F ? String.fromCharCode(byte) : '.');
        } else {
          lineBytes.push('  ');
          lineChars.push(' ');
        }
      }

      lines.push({
        offset,
        hex: lineBytes.join(' '),
        ascii: lineChars.join(''),
        key: `line-${offset}`,
      });
    }

    return lines;
  }, [hexData, hexOffset]);

  /*=======================================================================
   * Navigation
   *=======================================================================*/

  const pageDown = () => {
    setHexOffset(Math.min(
      hexOffset + LINES_PER_PAGE * BYTES_PER_LINE,
      (hexData?.length || 0) - LINES_PER_PAGE * BYTES_PER_LINE
    ));
  };

  const pageUp = () => {
    setHexOffset(Math.max(hexOffset - LINES_PER_PAGE * BYTES_PER_LINE, 0));
  };

  const gotoAddress = () => {
    const addr = parseInt(gotoOffset, 16);
    if (!isNaN(addr) && addr >= 0 && hexData && addr < hexData.length) {
      setHexOffset(addr - (addr % BYTES_PER_LINE));
    }
  };

  /*=======================================================================
   * Search
   *=======================================================================*/

  const doSearch = useCallback(() => {
    if (!hexData || !searchHex) return;

    const searchBytes = [];
    const cleaned = searchHex.replace(/\s/g, '');
    for (let i = 0; i < cleaned.length; i += 2) {
      const byte = parseInt(cleaned.slice(i, i + 2), 16);
      if (!isNaN(byte)) searchBytes.push(byte);
    }

    if (searchBytes.length === 0) return;

    const results = [];
    for (let i = 0; i <= hexData.length - searchBytes.length; i++) {
      let match = true;
      for (let j = 0; j < searchBytes.length; j++) {
        if (hexData[i + j] !== searchBytes[j]) {
          match = false;
          break;
        }
      }
      if (match) results.push(i);
    }

    setSearchResults(results);
    setCurrentSearchIdx(0);
    if (results.length > 0) {
      setHexOffset(results[0] - (results[0] % BYTES_PER_LINE));
    } else {
      Alert.alert('Not Found', 'Pattern not found in data.');
    }
  }, [hexData, searchHex]);

  const nextSearchResult = () => {
    if (searchResults.length === 0) return;
    const next = (currentSearchIdx + 1) % searchResults.length;
    setCurrentSearchIdx(next);
    setHexOffset(searchResults[next] - (searchResults[next] % BYTES_PER_LINE));
  };

  /*=======================================================================
   * Hash Verification
   *=======================================================================*/

  const verifyHash = useCallback(async () => {
    if (!device.connected) {
      Alert.alert('Not Connected', 'Connect device to verify hash.');
      return;
    }

    setHashVerifying(true);
    try {
      const result = await device.protocol.startHash();
      setHashDigest(result.digestHex);
    } catch (e) {
      Alert.alert('Hash Failed', e.message);
    } finally {
      setHashVerifying(false);
    }
  }, [device.connected, device.protocol]);

  /*=======================================================================
   * Partition Analysis
   *=======================================================================*/

  const analyzePartitions = useCallback(() => {
    if (!hexData || hexData.length < 512) {
      Alert.alert('No Data', 'Load data first (at least 512 bytes needed for MBR).');
      return;
    }

    /* Check for MBR at offset 0 */
    const mbrSignature = hexData.readUInt16LE(0x1FE);
    if (mbrSignature === 0xAA55) {
      const partitions = [];
      for (let i = 0; i < 4; i++) {
        const entryOffset = 0x1BE + i * 16;
        const status = hexData[entryOffset];
        const type = hexData[entryOffset + 4];
        const startLba = hexData.readUInt32LE(entryOffset + 8);
        const sizeLba = hexData.readUInt32LE(entryOffset + 12);

        if (type !== 0) {
          partitions.push({
            index: i + 1,
            active: status === 0x80,
            type: type.toString(16).padStart(2, '0').toUpperCase(),
            typeName: getPartitionTypeName(type),
            startLba,
            sizeLba,
            sizeBytes: sizeLba * 512,
          });
        }
      }
      setPartitionInfo({ type: 'MBR', partitions });
    } else {
      /* Check for GPT header at LBA 1 */
      const gptSignature = hexData.toString('ascii', 512, 520);
      if (gptSignature === 'EFI PART') {
        setPartitionInfo({ type: 'GPT', partitions: [] });
      } else {
        setPartitionInfo({ type: 'Unknown', partitions: [] });
      }
    }

    setViewMode('partitions');
  }, [hexData]);

  /*=======================================================================
   * Export
   *=======================================================================*/

  const exportData = useCallback(async () => {
    if (!hexData) return;
    try {
      /* In a real app, write to file and share */
      await Share.share({
        message: `eMMC Flash Dumper — Data Export\nSize: ${hexData.length} bytes\nOffset: 0x${hexOffset.toString(16)}`,
        title: 'Export Flash Data',
      });
    } catch (e) {
      /* Ignore */
    }
  }, [hexData, hexOffset]);

  /*=======================================================================
   * Render
   *=======================================================================*/

  return (
    <View style={styles.container}>
      {/* View Mode Tabs */}
      <View style={styles.tabRow}>
        {['hex', 'partitions', 'hash'].map((mode) => (
          <TouchableOpacity
            key={mode}
            style={[styles.tab, viewMode === mode && styles.tabActive]}
            onPress={() => setViewMode(mode)}
          >
            <Text style={[styles.tabText, viewMode === mode && styles.tabTextActive]}>
              {mode.toUpperCase()}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {viewMode === 'hex' && (
        <>
          {/* Hex Navigation */}
          <View style={styles.navRow}>
            <TouchableOpacity style={styles.navBtn} onPress={pageUp}>
              <Text style={styles.navBtnText}>◀ PREV</Text>
            </TouchableOpacity>
            <View style={styles.gotoRow}>
              <TextInput
                style={styles.gotoInput}
                value={gotoOffset}
                onChangeText={setGotoOffset}
                placeholder="0x0000"
                placeholderTextColor="#444"
                keyboardType="ascii-capable"
              />
              <TouchableOpacity style={styles.gotoBtn} onPress={gotoAddress}>
                <Text style={styles.gotoBtnText}>GO</Text>
              </TouchableOpacity>
            </View>
            <TouchableOpacity style={styles.navBtn} onPress={pageDown}>
              <Text style={styles.navBtnText}>NEXT ▶</Text>
            </TouchableOpacity>
          </View>

          {/* Search */}
          <View style={styles.searchRow}>
            <TextInput
              style={styles.searchInput}
              value={searchHex}
              onChangeText={setSearchHex}
              placeholder="Search hex (e.g. AA 55)"
              placeholderTextColor="#444"
              keyboardType="ascii-capable"
            />
            <TouchableOpacity style={styles.searchBtn} onPress={doSearch}>
              <Text style={styles.searchBtnText}>FIND</Text>
            </TouchableOpacity>
            {searchResults.length > 0 && (
              <TouchableOpacity style={styles.searchBtn} onPress={nextSearchResult}>
                <Text style={styles.searchBtnText}>
                  NEXT ({currentSearchIdx + 1}/{searchResults.length})
                </Text>
              </TouchableOpacity>
            )}
          </View>

          {/* Hex Dump */}
          {hexData ? (
            <FlatList
              data={hexLines}
              keyExtractor={(item) => item.key}
              renderItem={({ item }) => (
                <View style={[
                  styles.hexLine,
                  searchResults.includes(item.offset) && styles.hexLineHighlight,
                ]}>
                  <Text style={styles.hexOffset}>
                    {item.offset.toString(16).padStart(8, '0').toUpperCase()}
                  </Text>
                  <Text style={styles.hexBytes}>{item.hex}</Text>
                  <Text style={styles.hexAscii}>{item.ascii}</Text>
                </View>
              )}
              initialNumToRender={32}
              maxToRenderPerBatch={32}
              windowSize={5}
              style={styles.hexList}
            />
          ) : (
            <View style={styles.emptyState}>
              <Text style={styles.emptyIcon}>📂</Text>
              <Text style={styles.emptyText}>No data loaded</Text>
              <TouchableOpacity style={styles.loadBtn} onPress={loadSampleData}>
                <Text style={styles.loadBtnText}>LOAD SAMPLE DATA</Text>
              </TouchableOpacity>
            </View>
          )}

          {/* Data Info Bar */}
          {hexData && (
            <View style={styles.infoBar}>
              <Text style={styles.infoBarText}>
                Size: {hexData.length.toLocaleString()} bytes |
                Range: 0x{hexOffset.toString(16)}-0x{Math.min(hexOffset + LINES_PER_PAGE * BYTES_PER_LINE - 1, hexData.length - 1).toString(16)}
              </Text>
            </View>
          )}
        </>
      )}

      {viewMode === 'partitions' && (
        <ScrollView style={styles.partitionView}>
          {partitionInfo ? (
            <>
              <Text style={styles.partitionTitle}>
                PARTITION TABLE: {partitionInfo.type}
              </Text>
              {partitionInfo.partitions.map((p) => (
                <View key={p.index} style={styles.partitionCard}>
                  <Text style={styles.partitionName}>
                    Partition {p.index} {p.active ? '[ACTIVE]' : ''}
                  </Text>
                  <InfoRow label="Type" value={`0x${p.type} (${p.typeName})`} />
                  <InfoRow label="Start LBA" value={p.startLba.toLocaleString()} />
                  <InfoRow label="Size (LBA)" value={p.sizeLba.toLocaleString()} />
                  <InfoRow label="Size" value={formatBytes(p.sizeBytes)} />
                </View>
              ))}
              {partitionInfo.partitions.length === 0 && (
                <Text style={styles.noPartitions}>No partitions found</Text>
              )}
            </>
          ) : (
            <View style={styles.emptyState}>
              <Text style={styles.emptyIcon}>📋</Text>
              <Text style={styles.emptyText}>No partition analysis</Text>
              <TouchableOpacity style={styles.loadBtn} onPress={analyzePartitions}>
                <Text style={styles.loadBtnText}>ANALYZE PARTITIONS</Text>
              </TouchableOpacity>
            </View>
          )}
        </ScrollView>
      )}

      {viewMode === 'hash' && (
        <View style={styles.hashView}>
          <Text style={styles.hashTitle}>SHA-256 VERIFICATION</Text>
          {hashDigest ? (
            <View style={styles.hashResult}>
              <Text style={styles.hashLabel}>DIGEST:</Text>
              <Text style={styles.hashValue}>{hashDigest}</Text>
            </View>
          ) : (
            <Text style={styles.hashNone}>No hash computed yet</Text>
          )}
          <TouchableOpacity
            style={[styles.hashBtn, hashVerifying && styles.btnDisabled]}
            onPress={verifyHash}
            disabled={hashVerifying || !device.connected}
          >
            <Text style={styles.hashBtnText}>
              {hashVerifying ? 'COMPUTING...' : 'VERIFY HASH'}
            </Text>
          </TouchableOpacity>
        </View>
      )}

      {/* Export Button */}
      {hexData && (
        <TouchableOpacity style={styles.exportBtn} onPress={exportData}>
          <Text style={styles.exportBtnText}>EXPORT DATA</Text>
        </TouchableOpacity>
      )}
    </View>
  );
}

/*===========================================================================
 * Helpers
 *===========================================================================*/

function InfoRow({ label, value }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
    </View>
  );
}

function formatBytes(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
}

function getPartitionTypeName(type) {
  const names = {
    0x01: 'FAT12',
    0x04: 'FAT16 <32M',
    0x06: 'FAT16',
    0x07: 'NTFS/HPFS',
    0x0B: 'FAT32 CHS',
    0x0C: 'FAT32 LBA',
    0x0E: 'FAT16 LBA',
    0x83: 'Linux',
    0x82: 'Linux Swap',
    0x8E: 'Linux LVM',
    0xA5: 'FreeBSD',
    0xEE: 'GPT Protective',
    0xEF: 'EFI System',
  };
  return names[type] || 'Unknown';
}

/*===========================================================================
 * Styles
 *===========================================================================*/

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
  },
  tabRow: {
    flexDirection: 'row',
    backgroundColor: '#111',
    borderBottomWidth: 1,
    borderBottomColor: '#222',
  },
  tab: {
    flex: 1,
    paddingVertical: 10,
    alignItems: 'center',
  },
  tabActive: {
    borderBottomWidth: 2,
    borderBottomColor: '#00ff88',
  },
  tabText: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#666',
    letterSpacing: 2,
  },
  tabTextActive: {
    color: '#00ff88',
  },
  navRow: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#111',
    gap: 8,
  },
  navBtn: {
    backgroundColor: '#1a1a1a',
    borderRadius: 4,
    paddingHorizontal: 10,
    paddingVertical: 6,
  },
  navBtnText: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#888',
  },
  gotoRow: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  gotoInput: {
    flex: 1,
    backgroundColor: '#1a1a1a',
    borderRadius: 4,
    padding: 6,
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#fff',
    borderWidth: 1,
    borderColor: '#333',
  },
  gotoBtn: {
    backgroundColor: '#1a3a1a',
    borderRadius: 4,
    paddingHorizontal: 10,
    paddingVertical: 6,
  },
  gotoBtnText: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#00ff88',
  },
  searchRow: {
    flexDirection: 'row',
    padding: 8,
    backgroundColor: '#0d0d0d',
    gap: 4,
    alignItems: 'center',
  },
  searchInput: {
    flex: 1,
    backgroundColor: '#1a1a1a',
    borderRadius: 4,
    padding: 6,
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#fff',
    borderWidth: 1,
    borderColor: '#333',
  },
  searchBtn: {
    backgroundColor: '#1a1a2e',
    borderRadius: 4,
    paddingHorizontal: 8,
    paddingVertical: 6,
  },
  searchBtnText: {
    fontFamily: 'monospace',
    fontSize: 9,
    color: '#8888ff',
  },
  hexList: {
    flex: 1,
    backgroundColor: '#0a0a0a',
  },
  hexLine: {
    flexDirection: 'row',
    paddingHorizontal: 8,
    paddingVertical: 1,
    borderBottomWidth: 0.5,
    borderBottomColor: '#111',
  },
  hexLineHighlight: {
    backgroundColor: '#1a1a0030',
    borderLeftWidth: 2,
    borderLeftColor: '#ff8800',
  },
  hexOffset: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#555',
    width: 70,
  },
  hexBytes: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#ccc',
    flex: 1,
    letterSpacing: 1,
  },
  hexAscii: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#888',
    width: 130,
  },
  emptyState: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 40,
  },
  emptyIcon: {
    fontSize: 48,
    marginBottom: 12,
  },
  emptyText: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#666',
    marginBottom: 16,
  },
  loadBtn: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderWidth: 1,
    borderColor: '#333',
  },
  loadBtnText: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#8888ff',
    letterSpacing: 1,
  },
  infoBar: {
    backgroundColor: '#111',
    padding: 8,
    borderTopWidth: 1,
    borderTopColor: '#222',
  },
  infoBarText: {
    fontFamily: 'monospace',
    fontSize: 9,
    color: '#555',
    textAlign: 'center',
  },
  partitionView: {
    flex: 1,
    padding: 16,
  },
  partitionTitle: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#00ff88',
    marginBottom: 12,
    letterSpacing: 1,
  },
  partitionCard: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
    borderLeftWidth: 3,
    borderLeftColor: '#0088ff',
  },
  partitionName: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#fff',
    marginBottom: 6,
  },
  noPartitions: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#666',
    textAlign: 'center',
    marginTop: 20,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 2,
  },
  infoLabel: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#888',
  },
  infoValue: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#ccc',
  },
  hashView: {
    flex: 1,
    padding: 16,
    alignItems: 'center',
    justifyContent: 'center',
  },
  hashTitle: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#ff8800',
    letterSpacing: 2,
    marginBottom: 16,
  },
  hashResult: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 16,
    width: '100%',
    alignItems: 'center',
  },
  hashLabel: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#666',
    marginBottom: 8,
  },
  hashValue: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#00ff88',
    textAlign: 'center',
    lineHeight: 16,
  },
  hashNone: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#666',
    marginBottom: 16,
  },
  hashBtn: {
    backgroundColor: '#1a1a00',
    borderRadius: 8,
    paddingHorizontal: 24,
    paddingVertical: 12,
    borderWidth: 1,
    borderColor: '#ff8800',
    marginTop: 12,
  },
  hashBtnText: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#ff8800',
    letterSpacing: 2,
  },
  btnDisabled: {
    opacity: 0.4,
  },
  exportBtn: {
    backgroundColor: '#003322',
    borderRadius: 8,
    padding: 12,
    margin: 16,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#00ff88',
  },
  exportBtnText: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#00ff88',
    letterSpacing: 2,
  },
});
