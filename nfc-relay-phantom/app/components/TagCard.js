/**
 * NFC Relay Phantom — TagCard Component
 * Reusable card component for displaying NFC/RFID tag data
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Text, Card, Chip, IconButton } from 'react-native-paper';

export default function TagCard({ tag, onEmulate, onSave, onSelect }) {
  const formatUID = (uid) => {
    if (!uid) return 'Unknown';
    return uid.match(/.{2}/g)?.join(':') || uid;
  };

  const getProtocolColor = (protocol) => {
    switch (protocol) {
      case 'NFC-A':
      case 'EM4100':
        return '#2196F3';
      case 'NFC-B':
      case 'HID_PROX':
        return '#4CAF50';
      case 'NFC-F':
      case 'AWID':
        return '#FF9800';
      case 'NFC-V':
      case 'IO_PROX':
        return '#9C27B0';
      default:
        return '#666';
    }
  };

  return (
    <Card style={styles.card}>
      <Card.Content>
        <View style={styles.header}>
          <Chip
            mode="outlined"
            textStyle={{ color: getProtocolColor(tag.protocol), fontSize: 11 }}
            style={[styles.chip, { borderColor: getProtocolColor(tag.protocol) }]}
          >
            {tag.protocol || 'Unknown'}
          </Chip>
          <Text style={styles.timestamp}>{tag.timestamp || 'Just now'}</Text>
        </View>
        <Text style={styles.uidLabel}>UID</Text>
        <Text style={styles.uid}>{formatUID(tag.uid)}</Text>
        {tag.atqa && (
          <Text style={styles.detail}>ATQA: {tag.atqa} | SAK: {tag.sak || 'N/A'}</Text>
        )}
        {tag.facility_code !== undefined && (
          <Text style={styles.detail}>
            Facility: {tag.facility_code} | Card: {tag.card_number}
          </Text>
        )}
      </Card.Content>
      <Card.Actions>
        {onEmulate && (
          <IconButton icon="nfc-variant" onPress={() => onEmulate(tag)} size={20} />
        )}
        {onSave && (
          <IconButton icon="content-save" onPress={() => onSave(tag)} size={20} />
        )}
        {onSelect && (
          <IconButton icon="check-circle" onPress={() => onSelect(tag)} size={20} />
        )}
      </Card.Actions>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    marginVertical: 4,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 4,
  },
  chip: {
    height: 24,
  },
  timestamp: {
    fontSize: 11,
    color: '#999',
  },
  uidLabel: {
    fontSize: 10,
    color: '#999',
    textTransform: 'uppercase',
  },
  uid: {
    fontFamily: 'monospace',
    fontSize: 16,
    color: '#6200EE',
    letterSpacing: 1,
    marginVertical: 2,
  },
  detail: {
    fontSize: 12,
    color: '#666',
  },
});