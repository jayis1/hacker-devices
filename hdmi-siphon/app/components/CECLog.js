/**
 * CECLog.js — CEC event log component
 * Author: jayis1
 */

import React from 'react';
import {View, Text, StyleSheet, FlatList} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const ADDR_NAMES = {
  0: 'TV', 1: 'Rec1', 2: 'Rec2', 3: 'Tuner1', 4: 'Player',
  5: 'Audio', 6: 'Tuner2', 7: 'Tuner3', 15: 'BCast',
};

const CECLog = ({entries, maxHeight = 200}) => {
  const renderItem = ({item}) => {
    const sourceName = ADDR_NAMES[item.source_addr] || `0x${item.source_addr.toString(16)}`;
    const destName = ADDR_NAMES[item.dest_addr] || `0x${item.dest_addr.toString(16)}`;

    return (
      <View style={styles.entry}>
        <Icon name="remote" size={14} color="#78909C" />
        <Text style={styles.entryText}>
          {sourceName} → {destName}
        </Text>
        <Text style={styles.opcode}>
          0x{item.opcode.toString(16)}
        </Text>
      </View>
    );
  };

  return (
    <View style={[styles.container, {maxHeight}]}>
      {entries.length === 0 ? (
        <View style={styles.empty}>
          <Icon name="remote-off" size={20} color="#555" />
          <Text style={styles.emptyText}>No CEC activity</Text>
        </View>
      ) : (
        <FlatList
          data={entries}
          renderItem={renderItem}
          keyExtractor={(item, idx) => String(idx)}
          showsVerticalScrollIndicator={false}
        />
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#1E1E2E',
    borderRadius: 8,
    overflow: 'hidden',
  },
  entry: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 6,
    paddingHorizontal: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#2A2A3E',
  },
  entryText: {
    color: '#B0B0B0',
    fontSize: 12,
    marginLeft: 8,
    flex: 1,
  },
  opcode: {
    color: '#78909C',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  empty: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 20,
  },
  emptyText: {
    color: '#555',
    fontSize: 13,
    marginLeft: 8,
  },
});

export default CECLog;
