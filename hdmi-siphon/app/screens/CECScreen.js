/**
 * CECScreen.js — CEC bus monitor and command injector
 * Author: jayis1
 */

import React, {useState, useEffect, useRef} from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  FlatList,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const CEC_COMMANDS = [
  {name: 'Power Off TV', address: 0, opcode: 0x36, payload: []},
  {name: 'Active Source', address: 15, opcode: 0x82, payload: [0x10, 0x00]},
  {name: 'Volume Up', address: 5, opcode: 0x44, payload: [0x41]},
  {name: 'Volume Down', address: 5, opcode: 0x44, payload: [0x42]},
  {name: 'Mute', address: 5, opcode: 0x44, payload: [0x43]},
  {name: 'Play', address: 4, opcode: 0x44, payload: [0x44]},
  {name: 'Pause', address: 4, opcode: 0x44, payload: [0x46]},
  {name: 'Stop', address: 4, opcode: 0x44, payload: [0x47]},
  {name: 'Menu', address: 0, opcode: 0x8D, payload: [0x00]},
  {name: 'Exit', address: 0, opcode: 0x44, payload: [0x0D]},
];

const ADDR_NAMES = {
  0: 'TV', 1: 'Rec1', 2: 'Rec2', 3: 'Tuner1', 4: 'Player',
  5: 'Audio', 6: 'Tuner2', 7: 'Tuner3', 15: 'Broadcast',
};

const CECScreen = ({protocol}) => {
  const [log, setLog] = useState([]);
  const [logCount, setLogCount] = useState(0);

  useEffect(() => {
    // Poll for CEC log updates
    const interval = setInterval(() => {
      if (protocol) {
        protocol.sendCommand('status', {}, response => {
          if (response?.cec_msg_count !== logCount) {
            setLogCount(response?.cec_msg_count || 0);
          }
        });
      }
    }, 2000);
    return () => clearInterval(interval);
  }, []);

  const handleSendCEC = (cmd) => {
    if (!protocol) return;
    protocol.sendCEC(cmd.address, cmd.opcode, cmd.payload, result => {
      const newEntry = {
        id: Date.now(),
        time: new Date().toLocaleTimeString(),
        cmd: cmd.name,
        status: result?.status === 'ok' ? 'SENT' : 'FAILED',
      };
      setLog(prev => [newEntry, ...prev].slice(0, 100));
    });
  };

  const renderCECItem = ({item}) => (
    <TouchableOpacity
      style={styles.cecItem}
      onPress={() => handleSendCEC(item)}>
      <Icon name="remote" size={20} color="#E53935" />
      <View style={styles.cecItemInfo}>
        <Text style={styles.cecItemName}>{item.name}</Text>
        <Text style={styles.cecItemDetail}>
          → {ADDR_NAMES[item.address] || `0x${item.address.toString(16)}`} (op: 0x{item.opcode.toString(16)})
        </Text>
      </View>
      <Icon name="send" size={18} color="#78909C" />
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      {/* Commands */}
      <Text style={styles.sectionTitle}>Send CEC Command</Text>
      <FlatList
        data={CEC_COMMANDS}
        renderItem={renderCECItem}
        keyExtractor={item => item.name}
        style={styles.commandList}
        horizontal={false}
        numColumns={1}
      />

      {/* Activity Log */}
      <View style={styles.logSection}>
        <Text style={styles.sectionTitle}>
          Activity Log ({logCount} total)
        </Text>
        {log.length === 0 ? (
          <View style={styles.logEmpty}>
            <Icon name="remote-off" size={32} color="#555" />
            <Text style={styles.logEmptyText}>No CEC activity yet</Text>
          </View>
        ) : (
          log.map(entry => (
            <View key={entry.id} style={styles.logEntry}>
              <Text style={styles.logTime}>{entry.time}</Text>
              <Text style={styles.logCmd}>{entry.cmd}</Text>
              <Text
                style={[
                  styles.logStatus,
                  entry.status === 'SENT'
                    ? styles.logStatusSent
                    : styles.logStatusFailed,
                ]}>
                {entry.status}
              </Text>
            </View>
          ))
        )}
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#121212',
    padding: 12,
  },
  sectionTitle: {
    color: '#FFF',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 8,
  },
  commandList: {
    maxHeight: 300,
    marginBottom: 12,
  },
  cecItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1E1E2E',
    padding: 12,
    borderRadius: 8,
    marginBottom: 4,
  },
  cecItemInfo: {
    flex: 1,
    marginLeft: 10,
  },
  cecItemName: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '600',
  },
  cecItemDetail: {
    color: '#78909C',
    fontSize: 12,
    marginTop: 2,
  },
  logSection: {
    flex: 1,
  },
  logEmpty: {
    alignItems: 'center',
    paddingTop: 20,
  },
  logEmptyText: {
    color: '#555',
    fontSize: 14,
    marginTop: 8,
  },
  logEntry: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1E1E2E',
    padding: 8,
    borderRadius: 6,
    marginBottom: 3,
  },
  logTime: {
    color: '#78909C',
    fontSize: 11,
    width: 70,
  },
  logCmd: {
    color: '#FFF',
    fontSize: 13,
    flex: 1,
  },
  logStatus: {
    fontSize: 11,
    fontWeight: '600',
    width: 50,
    textAlign: 'right',
  },
  logStatusSent: {
    color: '#66BB6A',
  },
  logStatusFailed: {
    color: '#EF5350',
  },
});

export default CECScreen;
