/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Transmit Screen — Message composition, modulation selection, Tx controls
 *
 * Allows the user to compose messages, select modulation (FSK/OOK/Whisper),
 * configure parameters, and transmit.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  TextInput,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Alert,
  Switch,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import {
  buildSetMode,
  buildSetFskParams,
  buildSetOokParams,
  buildTxMessage,
  buildSetPower,
  MODE,
  TX_POWER,
  TX_POWER_NAMES,
} from '../utils/protocol';

const SERVICE_UUID = '00001810-0000-1000-8000-00805f9b34fb';
const CHARACTERISTIC_UUID = '00002a35-0000-1000-8000-00805f9b34fb';

/**
 * Transmit screen — compose and send ultrasonic messages.
 */
export default function TransmitScreen({ bleManager, connectedDevice }) {
  const [message, setMessage] = useState('');
  const [modulation, setModulation] = useState('fsk');
  const [hexMode, setHexMode] = useState(false);
  const [markFreq, setMarkFreq] = useState('21000');
  const [spaceFreq, setSpaceFreq] = useState('19500');
  const [baudRate, setBaudRate] = useState('20');
  const [amplitude, setAmplitude] = useState('80');
  const [carrierFreq, setCarrierFreq] = useState('20000');
  const [bitDuration, setBitDuration] = useState('50000');
  const [txPower, setTxPower] = useState(TX_POWER.MED);
  const [transmitting, setTransmitting] = useState(false);

  const sendToDevice = useCallback(
    async (data) => {
      if (!connectedDevice) {
        Alert.alert('Not Connected', 'Connect to a device first on the Dashboard.');
        return;
      }
      try {
        await connectedDevice.writeCharacteristicWithResponseForService(
          SERVICE_UUID,
          CHARACTERISTIC_UUID,
          Buffer.from(data).toString('base64'),
        );
      } catch (error) {
        Alert.alert('Transmit Error', error.message);
      }
    },
    [connectedDevice],
  );

  const handleTransmit = useCallback(async () => {
    if (!message.trim()) {
      Alert.alert('Empty Message', 'Enter a message to transmit.');
      return;
    }

    setTransmitting(true);

    try {
      // 1. Set mode to TX
      let modeCmd;
      let paramsCmd;
      let msgCmd;

      if (modulation === 'fsk') {
        modeCmd = buildSetMode(MODE.TX_FSK);
        paramsCmd = buildSetFskParams(
          parseInt(markFreq, 10),
          parseInt(spaceFreq, 10),
          parseInt(baudRate, 10),
          parseInt(amplitude, 10) / 100,
        );
      } else if (modulation === 'ook') {
        modeCmd = buildSetMode(MODE.TX_OOK);
        paramsCmd = buildSetOokParams(
          parseInt(carrierFreq, 10),
          parseInt(bitDuration, 10),
          parseInt(amplitude, 10) / 100,
        );
      } else {
        modeCmd = buildSetMode(MODE.TX_WHISPER);
        paramsCmd = buildSetOokParams(20000, 200000, 0.6); // Placeholder — whisper handled separately in firmware
      }

      // 2. Set power
      const powerCmd = buildSetPower(txPower);

      // 3. Build message payload
      let msgPayload;
      if (hexMode) {
        // Parse hex string
        const hex = message.replace(/\s+/g, '');
        const bytes = [];
        for (let i = 0; i < hex.length; i += 2) {
          bytes.push(parseInt(hex.substring(i, i + 2), 16));
        }
        msgPayload = new Uint8Array(bytes);
        msgCmd = buildTxMessage(msgPayload);
      } else {
        msgCmd = buildTxMessage(message);
      }

      // Send in order: power → params → mode → message
      await sendToDevice(powerCmd);
      await sendToDevice(paramsCmd);
      await sendToDevice(modeCmd);
      await sendToDevice(msgCmd);

      Alert.alert('Transmitting', `Sending ${message.length} chars via ${modulation.toUpperCase()}`);
    } catch (error) {
      Alert.alert('Error', error.message);
    }

    setTransmitting(false);
  }, [
    message,
    modulation,
    hexMode,
    markFreq,
    spaceFreq,
    baudRate,
    amplitude,
    carrierFreq,
    bitDuration,
    txPower,
    sendToDevice,
  ]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Modulation Selection */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Modulation</Text>
        <View style={styles.modRow}>
          {['fsk', 'ook', 'whisper'].map((mod) => (
            <TouchableOpacity
              key={mod}
              style={[
                styles.modBtn,
                modulation === mod && styles.modBtnActive,
              ]}
              onPress={() => setModulation(mod)}
            >
              <Text
                style={[
                  styles.modText,
                  modulation === mod && styles.modTextActive,
                ]}
              >
                {mod.toUpperCase()}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <Text style={styles.modDesc}>
          {modulation === 'fsk'
            ? 'Frequency Shift Keying — Two-tone, robust, medium range'
            : modulation === 'ook'
            ? 'On-Off Keying — Simple carrier toggle, short range'
            : 'Spread Spectrum — Frequency-hopped whisper, longest range'}
        </Text>
      </View>

      {/* Parameters */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Parameters</Text>

        {modulation === 'fsk' && (
          <>
            <ParamRow
              label="Mark Freq (Hz)"
              value={markFreq}
              onChange={setMarkFreq}
            />
            <ParamRow
              label="Space Freq (Hz)"
              value={spaceFreq}
              onChange={setSpaceFreq}
            />
            <ParamRow
              label="Baud Rate (bps)"
              value={baudRate}
              onChange={setBaudRate}
            />
          </>
        )}

        {modulation === 'ook' && (
          <>
            <ParamRow
              label="Carrier Freq (Hz)"
              value={carrierFreq}
              onChange={setCarrierFreq}
            />
            <ParamRow
              label="Bit Duration (µs)"
              value={bitDuration}
              onChange={setBitDuration}
            />
          </>
        )}

        {modulation === 'whisper' && (
          <Text style={styles.hintText}>
            Whisper mode uses frequency hopping (19–23 kHz) with Barker
            spreading. Baud rate is fixed at ~3 bps for maximum SNR.
          </Text>
        )}

        <ParamRow
          label="Amplitude (%)"
          value={amplitude}
          onChange={setAmplitude}
        />
      </View>

      {/* Tx Power */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Tx Power</Text>
        <View style={styles.modRow}>
          {Object.keys(TX_POWER).map((key) => {
            const val = TX_POWER[key];
            return (
              <TouchableOpacity
                key={key}
                style={[
                  styles.powerBtn,
                  txPower === val && styles.powerBtnActive,
                ]}
                onPress={() => setTxPower(val)}
              >
                <Text
                  style={[
                    styles.powerText,
                    txPower === val && styles.powerTextActive,
                  ]}
                >
                  {TX_POWER_NAMES[val]}
                </Text>
              </TouchableOpacity>
            );
          })}
        </View>
      </View>

      {/* Message Input */}
      <View style={styles.card}>
        <View style={styles.msgHeader}>
          <Text style={styles.cardTitle}>Message</Text>
          <View style={styles.hexToggle}>
            <Text style={styles.hexLabel}>HEX</Text>
            <Switch
              value={hexMode}
              onValueChange={setHexMode}
              trackColor={{ false: '#333', true: '#0d7377' }}
              thumbColor={hexMode ? '#00d4ff' : '#ccc'}
            />
          </View>
        </View>

        {hexMode && (
          <Text style={styles.hexHint}>
            Enter hex bytes separated by spaces (e.g., "48 65 6C 6C 6F")
          </Text>
        )}

        <TextInput
          style={styles.textInput}
          value={message}
          onChangeText={setMessage}
          placeholder={
            hexMode ? '48 65 6C 6C 6F' : 'Type a message to transmit...'
          }
          placeholderTextColor="#555"
          multiline
          numberOfLines={3}
          autoCapitalize="none"
          autoCorrect={false}
        />

        <Text style={styles.msgInfo}>
          {hexMode
            ? `${(message.replace(/\s+/g, '').length / 2).toFixed(0)} bytes`
            : `${message.length} chars`}{' '}
          · ~{modulation === 'whisper'
            ? `${((message.length * 8) / 3).toFixed(0)}s`
            : `${((message.length * 8) / parseInt(baudRate || 20, 10)).toFixed(1)}s`}{' '}
          transmit time
        </Text>
      </View>

      {/* Transmit Button */}
      <TouchableOpacity
        style={[styles.transmitBtn, transmitting && styles.transmitBtnDisabled]}
        onPress={handleTransmit}
        disabled={transmitting || !message.trim()}
      >
        <Icon name="wifi-tethering" size={24} color="#fff" />
        <Text style={styles.transmitBtnText}>
          {transmitting ? 'Transmitting...' : 'Transmit'}
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

// Parameter input row
function ParamRow({ label, value, onChange }) {
  return (
    <View style={styles.paramRow}>
      <Text style={styles.paramLabel}>{label}</Text>
      <TextInput
        style={styles.paramInput}
        value={value}
        onChangeText={onChange}
        keyboardType="numeric"
        placeholderTextColor="#555"
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#16213e',
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
    marginBottom: 12,
  },
  modRow: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 8,
  },
  modBtn: {
    flex: 1,
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
    backgroundColor: '#16213e',
    borderWidth: 1,
    borderColor: '#333',
  },
  modBtnActive: {
    backgroundColor: '#0d7377',
    borderColor: '#00d4ff',
  },
  modText: {
    color: '#6c757d',
    fontWeight: '600',
    fontSize: 13,
  },
  modTextActive: {
    color: '#fff',
  },
  modDesc: {
    color: '#6c757d',
    fontSize: 11,
    fontStyle: 'italic',
  },
  paramRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 6,
    borderTopWidth: 1,
    borderTopColor: '#16213e',
  },
  paramLabel: {
    flex: 1,
    color: '#b0b0b0',
    fontSize: 13,
  },
  paramInput: {
    backgroundColor: '#16213e',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    color: '#e0e0e0',
    fontSize: 13,
    width: 90,
    textAlign: 'right',
    fontFamily: 'monospace',
  },
  hintText: {
    color: '#6c757d',
    fontSize: 12,
    fontStyle: 'italic',
    marginBottom: 8,
  },
  powerBtn: {
    flex: 1,
    paddingVertical: 8,
    borderRadius: 6,
    alignItems: 'center',
    backgroundColor: '#16213e',
    borderWidth: 1,
    borderColor: '#333',
  },
  powerBtnActive: {
    backgroundColor: '#7c4dff',
    borderColor: '#b388ff',
  },
  powerText: {
    color: '#6c757d',
    fontSize: 11,
    fontWeight: '600',
  },
  powerTextActive: {
    color: '#fff',
  },
  msgHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  hexToggle: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
  },
  hexLabel: {
    color: '#6c757d',
    fontSize: 12,
    fontWeight: '600',
  },
  hexHint: {
    color: '#ffaa00',
    fontSize: 11,
    marginBottom: 8,
  },
  textInput: {
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 12,
    color: '#e0e0e0',
    fontSize: 14,
    fontFamily: 'monospace',
    minHeight: 70,
    textAlignVertical: 'top',
  },
  msgInfo: {
    color: '#6c757d',
    fontSize: 11,
    marginTop: 6,
    textAlign: 'right',
  },
  transmitBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#0d7377',
    borderRadius: 12,
    padding: 16,
    gap: 10,
    marginTop: 4,
  },
  transmitBtnDisabled: {
    backgroundColor: '#333',
  },
  transmitBtnText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
});