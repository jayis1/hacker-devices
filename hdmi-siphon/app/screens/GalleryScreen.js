/**
 * GalleryScreen.js — Captured frame gallery
 * Author: jayis1
 */

import React, {useState, useEffect} from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  Image,
  Dimensions,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const {width} = Dimensions.get('window');
const NUM_COLUMNS = 3;
const ITEM_SIZE = (width - 24 - (NUM_COLUMNS - 1) * 6) / NUM_COLUMNS;

const GalleryScreen = ({protocol}) => {
  const [files, setFiles] = useState([]);
  const [loading, setLoading] = useState(false);

  const loadGallery = () => {
    setLoading(true);
    if (protocol) {
      protocol.sendCommand('gallery', {}, response => {
        if (response?.files) {
          setFiles(response.files);
        }
        setLoading(false);
      });
    } else {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadGallery();
  }, []);

  const renderItem = ({item}) => (
    <TouchableOpacity style={styles.thumbContainer}>
      <View style={styles.thumb}>
        <Icon name="file-image" size={32} color="#78909C" />
      </View>
      <Text style={styles.thumbName} numberOfLines={1}>
        {item.name}
      </Text>
      <Text style={styles.thumbSize}>
        {item.size > 1024
          ? `${(item.size / 1024).toFixed(0)} KB`
          : `${item.size} B`}
      </Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Captured Frames</Text>
        <TouchableOpacity style={styles.refreshBtn} onPress={loadGallery}>
          <Icon name="refresh" size={22} color="#FFF" />
        </TouchableOpacity>
      </View>

      {files.length === 0 && !loading ? (
        <View style={styles.empty}>
          <Icon name="image-off" size={64} color="#555" />
          <Text style={styles.emptyText}>No captures yet</Text>
          <Text style={styles.emptyHint}>
            Use the Capture tab to take frames
          </Text>
        </View>
      ) : (
        <FlatList
          data={files}
          renderItem={renderItem}
          keyExtractor={item => item.name}
          numColumns={NUM_COLUMNS}
          refreshing={loading}
          onRefresh={loadGallery}
          contentContainerStyle={styles.list}
        />
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#121212',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 12,
  },
  title: {
    color: '#FFF',
    fontSize: 20,
    fontWeight: '700',
  },
  refreshBtn: {
    padding: 8,
    backgroundColor: '#1E1E2E',
    borderRadius: 8,
  },
  list: {
    padding: 6,
  },
  thumbContainer: {
    width: ITEM_SIZE,
    margin: 3,
    backgroundColor: '#1E1E2E',
    borderRadius: 8,
    padding: 8,
    alignItems: 'center',
  },
  thumb: {
    width: ITEM_SIZE - 16,
    height: ITEM_SIZE - 16,
    backgroundColor: '#2A2A3E',
    borderRadius: 6,
    justifyContent: 'center',
    alignItems: 'center',
  },
  thumbName: {
    color: '#B0B0B0',
    fontSize: 10,
    marginTop: 4,
    textAlign: 'center',
  },
  thumbSize: {
    color: '#78909C',
    fontSize: 9,
  },
  empty: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  emptyText: {
    color: '#666',
    fontSize: 18,
    marginTop: 12,
  },
  emptyHint: {
    color: '#555',
    fontSize: 13,
    marginTop: 4,
  },
});

export default GalleryScreen;
