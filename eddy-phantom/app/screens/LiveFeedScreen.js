/**
 * eddy-phantom — LiveFeedScreen.js
 * Real-time reconstructed text feed from captured keystrokes.
 * Shows decoded characters with color-coded confidence scores.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, FlatList, SafeAreaView,
  StatusBar, TouchableOpacity,
} from 'react-native';

export default function LiveFeedScreen({ bleManager, connected,
  deviceState, keystrokes }) {
  const [feedData, setFeedData] = useState([]);
  const [autoScroll, setAutoScroll] = useState(true);
  const [paused, setPaused] = useState(false);
  const flatListRef = useRef(null);

  // Build text buffer from keystrokes
  const textBuffer = keystrokes.map(k => k.char).join('');
  const recentKeys = paused ? feedData : keystrokes.slice(-100);

  useEffect(() => {
    if (!paused) {
      setFeedData(keystrokes.slice(-100));
    }
  }, [keystrokes, paused]);

  // Auto-scroll to bottom
  useEffect(() => {
    if (autoScroll && flatListRef.current && recentKeys.length > 0) {
      flatListRef.current.scrollToEnd({ animated: true });
    }
  }, [recentKeys, autoScroll]);

  const confidenceColor = (conf) => {
    if (conf >= 90) return '#4ade80';
    if (conf >= 70) return '#f0a500';
    return '#ef4444';
  };

  const renderItem = ({ item, index }) => (
    <View style={styles.keyRow}>
      <Text style={styles.timestamp}>{item.time}</Text>
      <Text style={[styles.keyChar, { color: confidenceColor(item.confidence) }]}>
        {item.char === '\n' ? '⏎' : item.char === ' ' ? '␣' : item.char}
      </Text>
      <View style={styles.confBar}>
        <View style={[styles.confFill, {
          width: `${item.confidence}%`,
          backgroundColor: confidenceColor(item.confidence),
        }]} />
      </View>
      <Text style={styles.confText}>{item.confidence}%</Text>
    </View>
  );

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />

      {/* Text reconstruction preview */}
      <View style={styles.textPreviewCard}>
        <Text style={styles.previewLabel}>Reconstructed Text</Text>
        <Text style={styles.previewText}>
          {textBuffer.slice(-200) || '(waiting for keystrokes...)'}
        </Text>
      </View>

      {/* Control bar */}
      <View style={styles.controlBar}>
        <TouchableOpacity
          style={[styles.ctrlButton, paused && styles.ctrlActive]}
          onPress={() => setPaused(!paused)}
        >
          <Text style={styles.ctrlText}>{paused ? '▶ Resume' : '⏸ Pause'}</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.ctrlButton, autoScroll && styles.ctrlActive]}
          onPress={() => setAutoScroll(!autoScroll)}
        >
          <Text style={styles.ctrlText}>Auto-scroll: {autoScroll ? 'ON' : 'OFF'}</Text>
        </TouchableOpacity>
      </View>

      {/* Keystroke feed */}
      <View style={styles.feedHeader}>
        <Text style={styles.feedHeaderText}>
          Captured Keystrokes ({keystrokes.length} total)
        </Text>
      </View>

      <FlatList
        ref={flatListRef}
        data={recentKeys}
        renderItem={renderItem}
        keyExtractor={(item) => item.id.toString()}
        style={styles.feedList}
        inverted={false}
      />

      {/* Status footer */}
      <View style={styles.footer}>
        <View style={[styles.statusDot,
          { backgroundColor: deviceState === 'ARMED' ? '#e94560' : '#666' }]} />
        <Text style={styles.footerText}>
          {deviceState} | {keystrokes.length} keys captured
        </Text>
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  textPreviewCard: {
    backgroundColor: '#1a1a2e', margin: 12, borderRadius: 12,
    padding: 16, borderWidth: 1, borderColor: '#2a2a3e',
  },
  previewLabel: { color: '#e94560', fontSize: 12, fontWeight: 'bold', marginBottom: 8 },
  previewText: {
    color: '#fff', fontSize: 14, fontFamily: 'monospace',
    lineHeight: 20,
  },
  controlBar: {
    flexDirection: 'row', paddingHorizontal: 12, gap: 10, marginBottom: 8,
  },
  ctrlButton: {
    backgroundColor: '#1a1a2e', padding: 8, borderRadius: 6,
    borderWidth: 1, borderColor: '#2a2a3e',
  },
  ctrlActive: { borderColor: '#4a90d9' },
  ctrlText: { color: '#ccc', fontSize: 12 },
  feedHeader: { paddingHorizontal: 12, paddingVertical: 8 },
  feedHeaderText: { color: '#888', fontSize: 12 },
  feedList: { flex: 1, paddingHorizontal: 12 },
  keyRow: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 10,
    marginBottom: 4, borderWidth: 1, borderColor: '#2a2a3e',
  },
  timestamp: { color: '#555', fontSize: 10, width: 80 },
  keyChar: { fontSize: 18, fontWeight: 'bold', width: 50, textAlign: 'center' },
  confBar: {
    flex: 1, height: 6, backgroundColor: '#2a2a3e', borderRadius: 3,
    marginHorizontal: 8, overflow: 'hidden',
  },
  confFill: { height: '100%', borderRadius: 3 },
  confText: { color: '#888', fontSize: 10, width: 35, textAlign: 'right' },
  footer: {
    flexDirection: 'row', alignItems: 'center', padding: 12,
    backgroundColor: '#1a1a2e', borderTopWidth: 1, borderTopColor: '#2a2a3e',
  },
  statusDot: { width: 8, height: 8, borderRadius: 4, marginRight: 8 },
  footerText: { color: '#888', fontSize: 12 },
});