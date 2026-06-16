/**
 * ExfilScreen.js — Exfiltration Browser
 * Author: jayis1
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, StyleSheet,
  ActivityIndicator, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import ExfilFileRow from '../components/ExfilFileRow';
import { getExfilList, downloadExfil } from '../services/api';

const ExfilScreen = () => {
  const [files, setFiles] = useState([]);
  const [loading, setLoading] = useState(false);
  const [downloading, setDownloading] = useState(null);

  const loadFiles = useCallback(async () => {
    setLoading(true);
    try {
      const list = await getExfilList();
      setFiles(list);
    } catch (e) {
      console.error('Exfil list error:', e);
    } finally {
      setLoading(false);
    }
  }, []);

  React.useEffect(() => { loadFiles(); }, [loadFiles]);

  const handleDownload = async (file) => {
    setDownloading(file.id);
    try {
      const data = await downloadExfil(file.id);
      Alert.alert('Downloaded', `${file.name} (${file.size} bytes) saved to device`);
    } catch (e) {
      Alert.alert('Error', 'Download failed: ' + e.message);
    } finally {
      setDownloading(null);
    }
  };

  const renderItem = ({ item }) => (
    <ExfilFileRow
      file={item}
      onDownload={() => handleDownload(item)}
      downloading={downloading === item.id}
    />
  );

  return (
    <View style={styles.container}>
      <View style={styles.headerBar}>
        <Text style={styles.headerTitle}>Captured SATA Data</Text>
        <TouchableOpacity style={styles.refreshBtn} onPress={loadFiles}>
          <Icon name="refresh" size={20} color="#00ff88" />
        </TouchableOpacity>
      </View>

      <FlatList
        data={files}
        renderItem={renderItem}
        keyExtractor={(item) => String(item.id)}
        contentContainerStyle={styles.listContent}
        ListEmptyComponent={
          <View style={styles.emptyState}>
            {loading ? (
              <ActivityIndicator size="large" color="#00ff88" />
            ) : (
              <>
                <Icon name="file-hidden" size={48} color="#555" />
                <Text style={styles.emptyText}>No captured data</Text>
                <Text style={styles.emptySubtext}>
                  Set capture rules on the Rules tab. Exfiltrated data appears here.
                </Text>
              </>
            )}
          </View>
        }
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a' },
  headerBar: {
    flexDirection: 'row', alignItems: 'center',
    paddingHorizontal: 16, paddingVertical: 12,
    borderBottomWidth: 1, borderBottomColor: '#2a2a4a',
  },
  headerTitle: { flex: 1, color: '#e0e0e0', fontSize: 16, fontWeight: 'bold' },
  refreshBtn: { padding: 8 },
  listContent: { padding: 12 },
  emptyState: { alignItems: 'center', paddingVertical: 60 },
  emptyText: { color: '#888', fontSize: 16, marginTop: 16 },
  emptySubtext: { color: '#555', fontSize: 12, marginTop: 8, textAlign: 'center', paddingHorizontal: 40 },
});

export default ExfilScreen;
