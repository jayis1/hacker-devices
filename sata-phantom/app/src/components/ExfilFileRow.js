/**
 * ExfilFileRow.js — Exfiltrated File Row Component
 * Author: jayis1
 */

import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const FIS_LABELS = {
  '27': 'CMD',
  '34': 'STATUS',
  '46': 'DATA',
  '41': 'DMA SETUP',
};

const ExfilFileRow = ({ file, onDownload, downloading }) => {
  const fisLabel = FIS_LABELS[file.fisType?.toString(16).padStart(2, '0')] || `0x${file.fisType?.toString(16) || '??'}`;
  const timeStr = file.timestamp ? new Date(file.timestamp * 1000).toLocaleTimeString() : '--:--:--';

  return (
    <View style={styles.container}>
      <View style={styles.row}>
        <Icon name="file-document-outline" size={22} color="#48f" />
        <View style={styles.info}>
          <Text style={styles.fileName} numberOfLines={1}>{file.name || `Capture #${file.id}`}</Text>
          <View style={styles.metaRow}>
            <Text style={styles.metaBadge}>{fisLabel}</Text>
            <Text style={styles.metaText}>LBA: 0x{(file.lba || 0).toString(16).padStart(12, '0')}</Text>
          </View>
          <Text style={styles.sizeText}>{file.size || 0} bytes • {timeStr}</Text>
        </View>
        <TouchableOpacity
          style={[styles.downloadBtn, downloading && styles.downloadBtnDisabled]}
          onPress={onDownload}
          disabled={downloading}
        >
          <Icon
            name={downloading ? 'loading' : 'download'}
            size={20}
            color={downloading ? '#555' : '#00ff88'}
          />
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#12122a',
    borderRadius: 10,
    padding: 12,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#2a2a4a',
  },
  row: { flexDirection: 'row', alignItems: 'center' },
  info: { flex: 1, marginLeft: 10 },
  fileName: { color: '#e0e0e0', fontSize: 14, fontWeight: '500' },
  metaRow: { flexDirection: 'row', alignItems: 'center', marginTop: 4, gap: 8 },
  metaBadge: {
    backgroundColor: '#1a2a4a', color: '#48f', fontSize: 10,
    paddingHorizontal: 6, paddingVertical: 2, borderRadius: 4,
    fontWeight: 'bold', overflow: 'hidden',
  },
  metaText: { color: '#888', fontSize: 11, fontFamily: 'monospace' },
  sizeText: { color: '#666', fontSize: 11, marginTop: 2 },
  downloadBtn: { padding: 8 },
  downloadBtnDisabled: { opacity: 0.5 },
});

export default ExfilFileRow;
