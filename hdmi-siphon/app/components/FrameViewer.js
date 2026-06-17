/**
 * FrameViewer.js — Full-screen captured frame viewer
 * Author: jayis1
 */

import React from 'react';
import {View, Text, StyleSheet, Image, TouchableOpacity} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const FrameViewer = ({filename, size, onClose}) => {
  const host = '192.168.4.1';
  const imageUrl = `http://${host}/frame/${filename}`;

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.filename}>{filename}</Text>
        <TouchableOpacity onPress={onClose}>
          <Icon name="close" size={24} color="#FFF" />
        </TouchableOpacity>
      </View>
      <View style={styles.imageContainer}>
        <Image
          source={{uri: imageUrl}}
          style={styles.image}
          resizeMode="contain"
        />
      </View>
      <View style={styles.info}>
        <Text style={styles.infoText}>
          Size: {size > 1024 * 1024
            ? `${(size / (1024 * 1024)).toFixed(1)} MB`
            : size > 1024
            ? `${(size / 1024).toFixed(0)} KB`
            : `${size} B`}
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#000',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#1A1A2E',
  },
  filename: {
    color: '#FFF',
    fontSize: 16,
    fontWeight: '600',
  },
  imageContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  image: {
    width: '100%',
    height: '100%',
  },
  info: {
    padding: 16,
    backgroundColor: '#1A1A2E',
    alignItems: 'center',
  },
  infoText: {
    color: '#B0B0B0',
    fontSize: 14,
  },
});

export default FrameViewer;
