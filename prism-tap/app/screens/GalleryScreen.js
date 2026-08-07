/**
 * screens/GalleryScreen.js — Captured Frame Gallery
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, FlatList, Image, Alert, Share } from 'react-native';
import { useDevice } from '../utils/deviceContext';
import { CMD } from '../utils/protocol';

const GalleryScreen = () => {
  const { connected, sendCommand, sdPresent } = useDevice();
  const [frames, setFrames] = useState([]);
  const [selectedFrame, setSelectedFrame] = useState(null);
  const [loading, setLoading] = useState(false);

  const loadFrameList = useCallback(async () => {
    if (!connected) return;
    setLoading(true);
    // Request frame list from device
    await sendCommand(CMD.GET_FRAME_LIST);
    // In a real implementation, the response would populate frames[]
    // For now, show placeholder data
    setFrames([
      { id: 0, name: 'cap_0.ptf', size: '2.3 MB', ts: '14:22:01', w: 1920, h: 1080, fmt: 'RAW10' },
      { id: 1, name: 'cap_1.ptf', size: '2.3 MB', ts: '14:22:02', w: 1920, h: 1080, fmt: 'RAW10' },
      { id: 2, name: 'cap_2.ptf', size: '2.3 MB', ts: '14:22:03', w: 1920, h: 1080, fmt: 'RAW10' },
      { id: 3, name: 'cap_3.ptf', size: '2.3 MB', ts: '14:22:04', w: 1920, h: 1080, fmt: 'RAW10' },
    ]);
    setLoading(false);
  }, [connected, sendCommand]);

  useEffect(() => {
    loadFrameList();
  }, [loadFrameList]);

  const handleRequestThumbnail = async (frameId) => {
    if (!connected) return;
    await sendCommand(CMD.GET_FRAME_THUMB, [frameId & 0xFF, (frameId >> 8) & 0xFF]);
    Alert.alert('Thumbnail', 'Requested thumbnail from device');
  };

  const handleExportFrames = async () => {
    if (!connected) return;
    Alert.alert(
      'Export Frames',
      'Export all captured frames via USB-C?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Export', onPress: async () => {
          await sendCommand(CMD.EXPORT_FRAMES);
          Alert.alert('Export', 'Frame export started. Connect USB-C to download.');
        }},
      ]
    );
  };

  const handleEraseFrames = async () => {
    Alert.alert(
      'Erase All Frames',
      'Delete all captured frames from SD card?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Delete All', style: 'destructive', onPress: async () => {
          await sendCommand(CMD.ERASE_FRAMES);
          setFrames([]);
          Alert.alert('Done', 'All frames erased');
        }},
      ]
    );
  };

  const handleShare = async (frame) => {
    try {
      await Share.share({
        message: `Prism-Tap frame: ${frame.name} (${frame.w}x${frame.h} ${frame.fmt}, ${frame.size})`,
      });
    } catch (error) {
      Alert.alert('Error', error.message);
    }
  };

  const renderFrame = ({ item }) => (
    <TouchableOpacity
      style={styles.frameItem}
      onPress={() => handleRequestThumbnail(item.id)}
      onLongPress={() => handleShare(item)}
    >
      <View style={styles.frameThumb}>
        <Text style={styles.thumbText}>{item.fmt}</Text>
      </View>
      <View style={styles.frameInfo}>
        <Text style={styles.frameName}>{item.name}</Text>
        <Text style={styles.frameDetails}>
          {item.w}x{item.h} • {item.fmt} • {item.size}
        </Text>
        <Text style={styles.frameTs}>{item.ts}</Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Captured Frames</Text>
        <Text style={styles.subtitle}>
          {sdPresent ? `${frames.length} frames on SD card` : 'No SD card'}
        </Text>
      </View>

      <View style={styles.actionBar}>
        <TouchableOpacity style={styles.barButton} onPress={loadFrameList}>
          <Text style={styles.barText}>Refresh</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.barButton} onPress={handleExportFrames}>
          <Text style={styles.barText}>Export USB</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.barButton, styles.dangerButton]} onPress={handleEraseFrames}>
          <Text style={[styles.barText, styles.dangerText]}>Erase All</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        data={frames}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderFrame}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>
              {loading ? 'Loading...' : 'No frames captured yet.\nStart a capture from the Capture tab.'}
            </Text>
          </View>
        }
        contentContainerStyle={frames.length === 0 ? styles.emptyList : null}
        style={styles.frameList}
      />

      <View style={styles.infoBox}>
        <Text style={styles.infoText}>
          Tap a frame to request a thumbnail. Long press to share details.
          Export via USB-C for bulk download of full resolution frames.
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1e',
  },
  header: {
    padding: 15,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  title: {
    color: '#00d9ff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  subtitle: {
    color: '#666',
    fontSize: 12,
    marginTop: 2,
  },
  actionBar: {
    flexDirection: 'row',
    padding: 10,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  barButton: {
    flex: 1,
    padding: 8,
    borderRadius: 5,
    backgroundColor: '#0a2e3e',
    marginHorizontal: 4,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#00d9ff',
  },
  dangerButton: {
    borderColor: '#ff4444',
    backgroundColor: '#2e1a1a',
  },
  barText: {
    color: '#00d9ff',
    fontSize: 12,
  },
  dangerText: {
    color: '#ff4444',
  },
  frameList: {
    flex: 1,
    padding: 10,
  },
  frameItem: {
    flexDirection: 'row',
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    marginBottom: 8,
    padding: 10,
    borderWidth: 1,
    borderColor: '#333',
  },
  frameThumb: {
    width: 60,
    height: 40,
    backgroundColor: '#0a0a14',
    borderRadius: 4,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  thumbText: {
    color: '#555',
    fontSize: 9,
  },
  frameInfo: {
    flex: 1,
    marginLeft: 10,
  },
  frameName: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  frameDetails: {
    color: '#888',
    fontSize: 11,
    marginTop: 2,
  },
  frameTs: {
    color: '#555',
    fontSize: 10,
    marginTop: 2,
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  emptyList: {
    flex: 1,
  },
  emptyText: {
    color: '#555',
    fontSize: 14,
    textAlign: 'center',
    marginTop: 50,
  },
  infoBox: {
    margin: 15,
    padding: 10,
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  infoText: {
    color: '#666',
    fontSize: 10,
    lineHeight: 14,
  },
});

export default GalleryScreen;