/**
 * TextExtractionScreen.js
 *
 * Shows OCR-extracted text from the captured display in real time.
 * Organized by frame with timestamps. Includes credential detection
 * alerts highlighting detected patterns (API keys, passwords, OTPs, URLs).
 *
 * Author: jayis1
 * @license MIT
 */

import React, { useState, useEffect, useCallback, useMemo } from 'react';
import {
  View,
  Text,
  FlatList,
  TouchableOpacity,
  TextInput,
  StyleSheet,
  Alert,
} from 'react-native';

const PATTERN_LABELS = {
  1: 'API_KEY',
  2: 'JWT_TOKEN',
  3: 'PASSWORD',
  4: 'OTP_CODE',
  5: 'URL',
  6: 'EMAIL',
  7: 'QR_CODE',
  8: 'BARCODE',
  9: 'CREDIT_CARD',
  10: 'IP_ADDR',
  11: 'MAC_ADDR',
  12: 'PHONE',
  13: 'CUSTOM',
};

const PATTERN_COLORS = {
  1: '#FF6B00',   /* API key - orange */
  2: '#FF4757',   /* JWT - red */
  3: '#FF4757',   /* Password - red */
  4: '#FFD700',   /* OTP - gold */
  5: '#00D4AA',   /* URL - green */
  6: '#00D4AA',   /* Email - green */
  7: '#7B68EE',   /* QR - purple */
  8: '#7B68EE',   /* Barcode - purple */
  9: '#FF4757',   /* Credit card - red */
  10: '#5C6877',  /* IP - gray */
  11: '#5C6877',  /* MAC - gray */
  12: '#5C6877',  /* Phone - gray */
  13: '#FFD700',  /* Custom - gold */
};

const TextEntry = React.memo(({ entry, isExpanded, onToggle }) => {
  const time = new Date(entry.timestamp).toLocaleTimeString();
  const patternLabel = entry.alertType ? PATTERN_LABELS[entry.alertType] : null;
  const patternColor = entry.alertType ? PATTERN_COLORS[entry.alertType] : null;

  return (
    <TouchableOpacity
      style={[styles.entryItem, entry.alertType && styles.entryItemAlert]}
      onPress={onToggle}
      activeOpacity={0.7}
    >
      <View style={styles.entryHeader}>
        <Text style={styles.entryTime}>{time}</Text>
        <Text style={styles.entryFrame}>Frame #{entry.frameIndex}</Text>
        {patternLabel && (
          <View style={[styles.alertBadge, { backgroundColor: patternColor + '20' }]}>
            <Text style={[styles.alertBadgeText, { color: patternColor }]}>
              ⚠ {patternLabel}
            </Text>
          </View>
        )}
      </View>
      <Text style={styles.entryText} numberOfLines={isExpanded ? 0 : 2}>
        {entry.text}
      </Text>
      {isExpanded && entry.alertType && (
        <View style={styles.alertDetail}>
          <Text style={styles.alertDetailText}>
            Confidence: {entry.confidence || 'N/A'}%
          </Text>
          <Text style={styles.alertDetailText}>
            Location: ({entry.x || 0}, {entry.y || 0})
          </Text>
          {entry.text.length > 50 && (
            <Text style={styles.alertFullText}>{entry.text}</Text>
          )}
        </View>
      )}
    </TouchableOpacity>
  );
});

const TextExtractionScreen = ({ protocol, connectionState }) => {
  const [entries, setEntries] = useState([]);
  const [filterText, setFilterText] = useState('');
  const [showAlertsOnly, setShowAlertsOnly] = useState(false);
  const [expandedId, setExpandedId] = useState(null);
  const maxEntries = 5000;

  useEffect(() => {
    const ocrHandler = (data) => {
      const newEntries = data.text.split('\n').filter(t => t.trim()).map((text, i) => ({
        id: `${data.frameIndex}-${i}-${Date.now()}`,
        frameIndex: data.frameIndex,
        timestamp: Date.now(),
        text: text.trim(),
        alertType: null,
      }));

      if (newEntries.length > 0) {
        setEntries(prev => {
          const updated = [...newEntries.reverse(), ...prev];
          return updated.slice(0, maxEntries);
        });
      }
    };

    const alertHandler = (alert) => {
      const entry = {
        id: `alert-${alert.frameIndex}-${Date.now()}`,
        frameIndex: alert.frameIndex,
        timestamp: Date.now(),
        text: alert.text,
        alertType: alert.patternType,
        confidence: alert.confidence,
        x: alert.x,
        y: alert.y,
      };
      setEntries(prev => [entry, ...prev].slice(0, maxEntries));

      /* Show popup for high-severity patterns */
      if (alert.patternType === 2 || alert.patternType === 3 ||
          alert.patternType === 9) {
        Alert.alert(
          '🔴 Critical Credential Detected',
          `Type: ${PATTERN_LABELS[alert.patternType]}\nText: ${alert.text.substring(0, 60)}`,
          [{ text: 'OK' }]
        );
      }
    };

    protocol.on('ocrText', ocrHandler);
    protocol.on('credentialAlert', alertHandler);
    return () => {
      protocol.off('ocrText', ocrHandler);
      protocol.off('credentialAlert', alertHandler);
    };
  }, [protocol]);

  const filteredEntries = useMemo(() => {
    return entries.filter(entry => {
      if (showAlertsOnly && !entry.alertType) return false;
      if (filterText && !entry.text.toLowerCase().includes(filterText.toLowerCase()))
        return false;
      return true;
    });
  }, [entries, filterText, showAlertsOnly]);

  const handleClear = useCallback(() => {
    setEntries([]);
  }, []);

  const handleExport = useCallback(() => {
    const json = JSON.stringify(entries, null, 2);
    Alert.alert('Export', `Would export ${entries.length} entries (${json.length} bytes)`);
  }, [entries]);

  const renderEntry = useCallback(({ item }) => (
    <TextEntry
      entry={item}
      isExpanded={expandedId === item.id}
      onToggle={() => setExpandedId(prev => prev === item.id ? null : item.id)}
    />
  ), [expandedId]);

  return (
    <View style={styles.container}>
      {/* Filter bar */}
      <View style={styles.filterBar}>
        <TextInput
          style={styles.filterInput}
          placeholder="Search text..."
          placeholderTextColor="#5C6877"
          value={filterText}
          onChangeText={setFilterText}
        />
        <TouchableOpacity
          style={[styles.filterBtn, showAlertsOnly && styles.filterBtnActive]}
          onPress={() => setShowAlertsOnly(!showAlertsOnly)}
        >
          <Text style={styles.filterBtnText}>
            {showAlertsOnly ? '⚠ Alerts' : 'All Text'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Action bar */}
      <View style={styles.actionBar}>
        <Text style={styles.entryCount}>
          {filteredEntries.length} entries
          {showAlertsOnly && ` (${entries.filter(e => e.alertType).length} alerts)`}
        </Text>
        <View style={styles.actionRow}>
          <TouchableOpacity style={styles.actionBtn} onPress={handleExport}>
            <Text style={styles.actionBtnText}>📤 Export</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionBtn} onPress={handleClear}>
            <Text style={styles.actionBtnText}>🗑 Clear</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Entry list */}
      <FlatList
        data={filteredEntries}
        renderItem={renderEntry}
        keyExtractor={item => item.id}
        style={styles.entryList}
        contentContainerStyle={styles.entryListContent}
        initialNumToRender={20}
        maxToRenderPerBatch={10}
        windowSize={5}
        removeClippedSubviews={true}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>
              {connectionState.bleConnected || connectionState.usbConnected
                ? 'Waiting for OCR text...\nEnable OCR in Capture settings'
                : 'Connect to GLYPH-REAPER to extract text'}
            </Text>
          </View>
        }
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E14',
  },
  filterBar: {
    flexDirection: 'row',
    backgroundColor: '#131720',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1E242C',
  },
  filterInput: {
    flex: 1,
    backgroundColor: '#0A0E14',
    borderWidth: 1,
    borderColor: '#1E242C',
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 8,
    color: '#E6EDF3',
    fontSize: 14,
    marginRight: 8,
  },
  filterBtn: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    backgroundColor: '#1E242C',
    borderRadius: 8,
    justifyContent: 'center',
  },
  filterBtnActive: {
    backgroundColor: '#FF475720',
  },
  filterBtnText: {
    color: '#E6EDF3',
    fontSize: 13,
    fontWeight: '600',
  },
  actionBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#131720',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#1E242C',
  },
  entryCount: {
    color: '#5C6877',
    fontSize: 12,
  },
  actionRow: {
    flexDirection: 'row',
  },
  actionBtn: {
    paddingHorizontal: 12,
    paddingVertical: 4,
    backgroundColor: '#1E242C',
    borderRadius: 6,
    marginLeft: 8,
  },
  actionBtnText: {
    color: '#E6EDF3',
    fontSize: 12,
  },
  entryList: {
    flex: 1,
  },
  entryListContent: {
    padding: 8,
  },
  entryItem: {
    backgroundColor: '#131720',
    borderRadius: 8,
    padding: 12,
    marginBottom: 6,
    borderWidth: 1,
    borderColor: '#1E242C',
  },
  entryItemAlert: {
    borderColor: '#FF475740',
  },
  entryHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 6,
  },
  entryTime: {
    color: '#5C6877',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  entryFrame: {
    color: '#5C6877',
    fontSize: 11,
    marginLeft: 8,
    fontFamily: 'monospace',
  },
  alertBadge: {
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 4,
    marginLeft: 8,
  },
  alertBadgeText: {
    fontSize: 10,
    fontWeight: '700',
  },
  entryText: {
    color: '#E6EDF3',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  alertDetail: {
    marginTop: 8,
    paddingTop: 8,
    borderTopWidth: 1,
    borderTopColor: '#1E242C',
  },
  alertDetailText: {
    color: '#5C6877',
    fontSize: 12,
    marginBottom: 4,
  },
  alertFullText: {
    color: '#FF4757',
    fontSize: 13,
    fontFamily: 'monospace',
    marginTop: 4,
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingTop: 100,
  },
  emptyText: {
    color: '#5C6877',
    fontSize: 16,
    textAlign: 'center',
  },
});

export default TextExtractionScreen;