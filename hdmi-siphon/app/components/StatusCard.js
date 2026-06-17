/**
 * StatusCard.js — Reusable status indicator card component
 * Author: jayis1
 */

import React from 'react';
import {View, Text, StyleSheet} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const StatusCard = ({icon, label, value, color = '#78909C'}) => {
  return (
    <View style={styles.card}>
      <Icon name={icon} size={24} color={color} />
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, {color}]} numberOfLines={1}>
        {value}
      </Text>
    </View>
  );
};

const styles = StyleSheet.create({
  card: {
    flex: 1,
    backgroundColor: '#1E1E2E',
    borderRadius: 10,
    padding: 12,
    alignItems: 'center',
    marginHorizontal: 3,
    minHeight: 80,
  },
  label: {
    color: '#78909C',
    fontSize: 10,
    marginTop: 6,
    textTransform: 'uppercase',
    letterSpacing: 0.5,
  },
  value: {
    color: '#FFF',
    fontSize: 13,
    fontWeight: '700',
    marginTop: 2,
    textAlign: 'center',
  },
});

export default StatusCard;
