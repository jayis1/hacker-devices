/**
 * LogViewerScreen.tsx - Real-time event log viewer
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState, useMemo } from 'react';
import { View, FlatList, StyleSheet, TouchableOpacity } from 'react-native';
import { Card, Title, Text, Button, SegmentedButtons } from 'react-native-paper';

interface LogViewerScreenProps {
  phantom: any;
}

interface LogEntry {
  timestamp: number;
  level: 'info' | 'warn' | 'error';
  message: string;
}

export default function LogViewerScreen({ phantom }: LogViewerScreenProps) {
  const [filter, setFilter] = useState('all');
  const [autoScroll, setAutoScroll] = useState(true);
  const { logs } = phantom;

  const filteredLogs = useMemo(() => {
    if (filter === 'all') return logs;
    return logs.filter((log: LogEntry) => log.level === filter);
  }, [logs, filter]);

  const formatTime = (ts: number): string => {
    const d = new Date(ts);
    return `${d.getHours().toString().padStart(2, '0')}:` +
           `${d.getMinutes().toString().padStart(2, '0')}:` +
           `${d.getSeconds().toString().padStart(2, '0')}.` +
           `${d.getMilliseconds().toString().padStart(3, '0')}`;
  };

  const getLevelColor = (level: string): string => {
    switch (level) {
      case 'info': return '#2196F3';
      case 'warn': return '#FF9800';
      case 'error': return '#F44336';
      default: return '#888';
    }
  };

  const getLevelIcon = (level: string): string => {
    switch (level) {
      case 'info': return 'ℹ';
      case 'warn': return '⚠';
      case 'error': return '✖';
      default: return '•';
    }
  };

  const exportLogs = () => {
    const logText = logs.map((log: LogEntry) =>
      `[${formatTime(log.timestamp)}] ${log.level.toUpperCase()}: ${log.message}`
    ).join('\n');
    
    // In production, use Share API or file system
    phantom.addLog('info', `Exported ${logs.length} log entries`);
  };

  const renderLogEntry = ({ item, index }: { item: LogEntry; index: number }) => {
    const color = getLevelColor(item.level);
    return (
      <View style={[styles.logEntry, { borderLeftColor: color }]}>
        <Text style={styles.logTime}>{formatTime(item.timestamp)}</Text>
        <Text style={[styles.logIcon, { color }]}>{getLevelIcon(item.level)}</Text>
        <Text style={styles.logMessage}>{item.message}</Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.headerRow}>
            <Title>Event Log</Title>
            <Text style={styles.logCount}>{logs.length} entries</Text>
          </View>
          <SegmentedButtons
            value={filter}
            onValueChange={setFilter}
            buttons={[
              { label: 'All', value: 'all' },
              { label: 'Info', value: 'info' },
              { label: 'Warn', value: 'warn' },
              { label: 'Error', value: 'error' },
            ]}
            style={styles.filter}
          />
          <View style={styles.actionRow}>
            <Button
              mode="outlined"
              onPress={exportLogs}
              style={styles.actionBtn}
              icon="download"
            >
              Export
            </Button>
            <Button
              mode="outlined"
              onPress={() => setAutoScroll(!autoScroll)}
              style={styles.actionBtn}
              icon={autoScroll ? 'arrow-down' : 'arrow-down-bold'}
            >
              {autoScroll ? 'Auto' : 'Manual'}
            </Button>
          </View>
        </Card.Content>
      </Card>

      <FlatList
        data={filteredLogs}
        renderItem={renderLogEntry}
        keyExtractor={(item, index) => `${item.timestamp}-${index}`}
        style={styles.logList}
        inverted={!autoScroll}
      />

      <Card style={styles.statsCard}>
        <Card.Content>
          <View style={styles.statsRow}>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Info</Text>
              <Text style={[styles.statValue, { color: '#2196F3' }]}>
                {logs.filter((l: LogEntry) => l.level === 'info').length}
              </Text>
            </View>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Warnings</Text>
              <Text style={[styles.statValue, { color: '#FF9800' }]}>
                {logs.filter((l: LogEntry) => l.level === 'warn').length}
              </Text>
            </View>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Errors</Text>
              <Text style={[styles.statValue, { color: '#F44336' }]}>
                {logs.filter((l: LogEntry) => l.level === 'error').length}
              </Text>
            </View>
          </View>
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    padding: 8,
  },
  card: {
    marginBottom: 8,
    backgroundColor: '#16213e',
  },
  headerRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  logCount: {
    color: '#888',
    fontSize: 12,
  },
  filter: {
    marginVertical: 8,
  },
  actionRow: {
    flexDirection: 'row',
    gap: 8,
  },
  actionBtn: {
    flex: 1,
  },
  logList: {
    flex: 1,
  },
  logEntry: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    padding: 8,
    marginVertical: 2,
    backgroundColor: '#0f172a',
    borderRadius: 4,
    borderLeftWidth: 3,
  },
  logTime: {
    color: '#666',
    fontSize: 11,
    fontFamily: 'monospace',
    marginRight: 8,
    marginTop: 2,
  },
  logIcon: {
    fontSize: 14,
    marginRight: 8,
    marginTop: 2,
  },
  logMessage: {
    color: '#e0e0e0',
    fontSize: 13,
    flex: 1,
  },
  statsCard: {
    marginTop: 8,
    backgroundColor: '#16213e',
  },
  statsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  statItem: {
    alignItems: 'center',
  },
  statLabel: {
    color: '#888',
    fontSize: 11,
    textTransform: 'uppercase',
  },
  statValue: {
    fontSize: 24,
    fontWeight: 'bold',
  },
});

// Author: jayis1