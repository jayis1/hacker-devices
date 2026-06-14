/**
 * StatusCard.js — Reusable card component for Substation app
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

export default function StatusCard({ title, icon, color, children, style }) {
  return (
    <View style={[styles.card, style]}>
      <View style={styles.header}>
        <Icon name={icon} size={20} color={color || '#FFFFFF'} />
        <Text style={[styles.title, color && { color }]}>{title}</Text>
      </View>
      <View style={styles.content}>{children}</View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1A1A2E',
    marginHorizontal: 12,
    marginVertical: 6,
    borderRadius: 10,
    borderWidth: 1,
    borderColor: '#2A2A4E',
    overflow: 'hidden',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#2A2A4E',
  },
  title: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '700',
    marginLeft: 8,
  },
  content: {
    paddingHorizontal: 16,
    paddingVertical: 8,
  },
});