/**
 * OSDScreen.js — OSD overlay text editor
 * Author: jayis1
 */

import React, {useState} from 'react';
import {
  View,
  Text,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const OSDScreen = ({protocol}) => {
  const [text, setText] = useState('');
  const [xPos, setXPos] = useState('100');
  const [yPos, setYPos] = useState('100');
  const [color, setColor] = useState('#FF0000');
  const [alpha, setAlpha] = useState('200');

  const handleApply = () => {
    if (!protocol) return;
    if (!text.trim()) {
      Alert.alert('Error', 'Please enter text to display');
      return;
    }

    protocol.setOSD(
      text,
      parseInt(xPos) || 100,
      parseInt(yPos) || 100,
      color,
      parseInt(alpha) || 200,
      result => {
        if (result?.status === 'ok') {
          Alert.alert('OSD Applied', `Text "${text}" displayed on screen`);
        } else {
          Alert.alert('Error', 'Failed to apply OSD overlay');
        }
      },
    );
  };

  const handleClear = () => {
    setText('');
    if (protocol) {
      protocol.sendCommand('osd_text', {
        text: '',
        x: 0,
        y: 0,
        color: '#000000',
        alpha: 0,
      });
    }
  };

  const PRESET_COLORS = [
    '#FF0000', '#00FF00', '#0000FF', '#FFFF00',
    '#FF00FF', '#00FFFF', '#FFFFFF', '#FF8800',
  ];

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>On-Screen Display Overlay</Text>
        <Text style={styles.cardDesc}>
          Inject text onto the video stream in real-time
        </Text>

        <Text style={styles.inputLabel}>Overlay Text</Text>
        <TextInput
          style={styles.textInput}
          value={text}
          onChangeText={setText}
          placeholder="Enter text to display..."
          placeholderTextColor="#555"
          maxLength={64}
        />

        <View style={styles.row}>
          <View style={styles.halfInput}>
            <Text style={styles.inputLabel}>X Position</Text>
            <TextInput
              style={styles.smallInput}
              value={xPos}
              onChangeText={setXPos}
              keyboardType="numeric"
              placeholderTextColor="#555"
            />
          </View>
          <View style={styles.halfInput}>
            <Text style={styles.inputLabel}>Y Position</Text>
            <TextInput
              style={styles.smallInput}
              value={yPos}
              onChangeText={setYPos}
              keyboardType="numeric"
              placeholderTextColor="#555"
            />
          </View>
        </View>

        <Text style={styles.inputLabel}>Color</Text>
        <View style={styles.colorRow}>
          {PRESET_COLORS.map(c => (
            <TouchableOpacity
              key={c}
              style={[
                styles.colorSwatch,
                {backgroundColor: c},
                color === c && styles.colorSelected,
              ]}
              onPress={() => setColor(c)}
            />
          ))}
        </View>

        <Text style={styles.inputLabel}>Alpha (0-255): {alpha}</Text>
        <TextInput
          style={styles.smallInput}
          value={alpha}
          onChangeText={setAlpha}
          keyboardType="numeric"
          placeholderTextColor="#555"
        />

        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.button, styles.applyButton]}
            onPress={handleApply}>
            <Icon name="check" size={20} color="#FFF" />
            <Text style={styles.buttonText}>APPLY</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.clearButton]}
            onPress={handleClear}>
            <Icon name="close" size={20} color="#FFF" />
            <Text style={styles.buttonText}>CLEAR</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.preview}>
        <Text style={styles.previewTitle}>Preview</Text>
        <View style={styles.previewBox}>
          <Text style={[styles.previewText, {color: color}]}>
            {text || 'Preview Text'}
          </Text>
        </View>
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
  card: {
    backgroundColor: '#1E1E2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: '700',
    marginBottom: 4,
  },
  cardDesc: {
    color: '#B0B0B0',
    fontSize: 13,
    marginBottom: 12,
  },
  inputLabel: {
    color: '#B0B0B0',
    fontSize: 13,
    marginBottom: 4,
    marginTop: 8,
  },
  textInput: {
    backgroundColor: '#2A2A3E',
    color: '#FFF',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
  },
  smallInput: {
    backgroundColor: '#2A2A3E',
    color: '#FFF',
    borderRadius: 8,
    padding: 10,
    fontSize: 14,
    textAlign: 'center',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  halfInput: {
    flex: 1,
    marginHorizontal: 4,
  },
  colorRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginTop: 4,
  },
  colorSwatch: {
    width: 32,
    height: 32,
    borderRadius: 16,
    margin: 4,
    borderWidth: 2,
    borderColor: 'transparent',
  },
  colorSelected: {
    borderColor: '#FFF',
  },
  buttonRow: {
    flexDirection: 'row',
    marginTop: 16,
  },
  button: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 12,
    borderRadius: 8,
    marginHorizontal: 4,
  },
  applyButton: {
    backgroundColor: '#E53935',
  },
  clearButton: {
    backgroundColor: '#546E7A',
  },
  buttonText: {
    color: '#FFF',
    fontSize: 14,
    fontWeight: '700',
    marginLeft: 6,
  },
  preview: {
    backgroundColor: '#1E1E2E',
    borderRadius: 12,
    padding: 16,
  },
  previewTitle: {
    color: '#B0B0B0',
    fontSize: 13,
    marginBottom: 8,
  },
  previewBox: {
    backgroundColor: '#000',
    borderRadius: 8,
    padding: 20,
    minHeight: 100,
    justifyContent: 'center',
    alignItems: 'center',
  },
  previewText: {
    fontSize: 24,
    fontWeight: 'bold',
  },
});

export default OSDScreen;
