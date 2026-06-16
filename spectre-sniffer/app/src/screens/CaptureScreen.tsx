//==============================================================================
// CaptureScreen.tsx — Capture File Manager
// Author: jayis1
// Description: Browse, download, and manage IQ capture files stored on the
//              Spectre-Sniffer's SD card.
//==============================================================================

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  Alert,
  ActivityIndicator,
} from 'react-native';
import { SpectreAPI } from '../services/api';
import { CaptureFile } from '../types';

const CaptureScreen: React.FC = () => {
  const [files, setFiles] = useState<CaptureFile[]>([]);
  const [loading, setLoading] = useState(true);

  const loadFiles = useCallback(async () => {
    setLoading(true);
    try {
      const data = await SpectreAPI.listCaptures();
      setFiles(data);
    } catch (err) {
      // Show empty state
      setFiles([]);
    }
    setLoading(false);
  }, []);

  useEffect(() => {
    loadFiles();
  }, [loadFiles]);

  const deleteFile = (captureId: string) => {
    Alert.alert('Delete Capture', 'Are you sure?', [
      { text: 'Cancel', style: 'cancel' },
      {
        text: 'Delete',
        style: 'destructive',
        onPress: async () => {
          try {
            await SpectreAPI.deleteCapture(captureId);
            loadFiles();
          } catch (err) {
            Alert.alert('Error', 'Failed to delete file');
          }
        },
      },
    ]);
  };

  const formatSize = (bytes: number): string => {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  };

  const renderItem = ({ item }: { item: CaptureFile }) => (
    <View style={styles.fileItem}>
      <View style={styles.fileInfo}>
        <Text style={styles.fileName}>{item.name}</Text>
        <Text style={styles.fileDetails}>
          {formatSize(item.sizeBytes)} |{' '}
          {(item.centerFrequencyHz / 1e6).toFixed(1)} MHz |{' '}
          {(item.sampleRateHz / 1e6).toFixed(1)} MSps
        </Text>
        <Text style={styles.fileDate}>
          {new Date(item.timestamp * 1000).toLocaleString()}
        </Text>
      </View>
      <View style={styles.fileActions}>
        <TouchableOpacity
          style={styles.downloadButton}
          onPress={async () => {
            try {
              await SpectreAPI.downloadCapture(item.name);
              Alert.alert('Download', 'Capture downloaded to device');
            } catch (err) {
              Alert.alert('Error', 'Download failed');
            }
          }}
        >
          <Text style={styles.actionText}>DL</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.deleteButton}
          onPress={() => deleteFile(item.name)}
        >
          <Text style={styles.actionText}>Del</Text>
        </TouchableOpacity>
      </View>
    </View>
  );

  if (loading) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" color="#00E676" />
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Capture Files</Text>
        <TouchableOpacity style={styles.refreshButton} onPress={loadFiles}>
          <Text style={styles.refreshText}>Refresh</Text>
        </TouchableOpacity>
      </View>

      {files.length === 0 ? (
        <View style={styles.emptyState}>
          <Text style={styles.emptyText}>No captures found</Text>
          <Text style={styles.emptySubtext}>
            Start a capture from the Dashboard to see files here
          </Text>
        </View>
      ) : (
        <FlatList
          data={files}
          renderItem={renderItem}
          keyExtractor={(item) => item.name}
          style={styles.fileList}
        />
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  loadingContainer: {
    flex: 1,
    backgroundColor: '#0f0f23',
    justifyContent: 'center',
    alignItems: 'center',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  title: {
    color: '#fff',
    fontSize: 20,
    fontWeight: 'bold',
  },
  refreshButton: {
    backgroundColor: '#1a1a2e',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  refreshText: {
    color: '#00E676',
    fontSize: 14,
    fontWeight: '600',
  },
  emptyState: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  emptyText: {
    color: '#888',
    fontSize: 18,
    fontWeight: '600',
  },
  emptySubtext: {
    color: '#666',
    fontSize: 14,
    marginTop: 8,
    textAlign: 'center',
  },
  fileList: {
    flex: 1,
  },
  fileItem: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  fileInfo: {
    flex: 1,
  },
  fileName: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
    marginBottom: 4,
  },
  fileDetails: {
    color: '#888',
    fontSize: 12,
  },
  fileDate: {
    color: '#666',
    fontSize: 11,
    marginTop: 2,
  },
  fileActions: {
    flexDirection: 'row',
  },
  downloadButton: {
    backgroundColor: '#448AFF',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
    marginRight: 8,
  },
  deleteButton: {
    backgroundColor: '#FF5252',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
  },
  actionText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: 'bold',
  },
});

export default CaptureScreen;
