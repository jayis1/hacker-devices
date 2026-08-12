/**
 * CaptureScreen.js
 *
 * Configuration of capture parameters — display protocol selection,
 * resolution, color depth, capture rate, and trigger modes.
 *
 * Author: jayis1
 * @license MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  TextInput,
  Switch,
  StyleSheet,
  ScrollView,
  Alert,
} from 'react-native';

const PROTOCOLS = [
  { id: 1, label: 'MIPI DSI', desc: '1-4 lanes, up to 1.5 Gbps' },
  { id: 2, label: 'RGB Parallel', desc: '8/16/18/24-bit, 100 MHz' },
  { id: 3, label: 'SPI LCD', desc: 'ILI9341/ST7789/SSD1306' },
  { id: 4, label: 'LVDS', desc: 'Single/dual link, 135 MHz' },
];

const TRIGGER_MODES = [
  { id: 0, label: 'Continuous', desc: 'Capture every frame' },
  { id: 1, label: 'On Change', desc: 'Only when content changes' },
  { id: 2, label: 'On Motion', desc: 'Accelerometer triggered' },
  { id: 3, label: 'Scheduled', desc: 'At regular intervals' },
];

const CaptureScreen = ({ protocol, deviceStatus, connectionState }) => {
  const [selectedProtocol, setSelectedProtocol] = useState(2);
  const [width, setWidth] = useState('480');
  const [height, setHeight] = useState('320');
  const [colorDepth, setColorDepth] = useState(16);
  const [captureFps, setCaptureFps] = useState(15);
  const [triggerMode, setTriggerMode] = useState(0);
  const [ocrEnabled, setOcrEnabled] = useState(true);
  const [patternEnabled, setPatternEnabled] = useState(true);
  const [qrEnabled, setQrEnabled] = useState(true);
  const [isCapturing, setIsCapturing] = useState(false);

  const handleStartCapture = useCallback(() => {
    /* Send configuration commands */
    protocol.sendCommand(0x03, [selectedProtocol]); /* CMD_SET_PROTOCOL */
    protocol.sendCommand(0x04, [
      (parseInt(width) >> 8) & 0xFF,
      parseInt(width) & 0xFF,
      (parseInt(height) >> 8) & 0xFF,
      parseInt(height) & 0xFF,
    ]); /* CMD_SET_RESOLUTION */
    protocol.sendCommand(0x05, [colorDepth]); /* CMD_SET_COLOR_DEPTH */
    protocol.sendCommand(0x06, [captureFps]); /* CMD_SET_CAPTURE_FPS */
    protocol.sendCommand(0x07, [triggerMode]); /* CMD_SET_TRIGGER_MODE */
    protocol.sendCommand(0x0A, [ocrEnabled ? 1 : 0]); /* CMD_SET_OCR_MODE */

    /* Start capture */
    protocol.sendCommand(0x01, [selectedProtocol]); /* CMD_START_CAPTURE */
    setIsCapturing(true);
  }, [protocol, selectedProtocol, width, height, colorDepth, captureFps,
      triggerMode, ocrEnabled]);

  const handleStopCapture = useCallback(() => {
    protocol.sendCommand(0x02); /* CMD_STOP_CAPTURE */
    setIsCapturing(false);
  }, [protocol]);

  return (
    <ScrollView style={styles.container}>
      {/* Protocol selection */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display Protocol</Text>
        {PROTOCOLS.map(proto => (
          <TouchableOpacity
            key={proto.id}
            style={[
              styles.optionRow,
              selectedProtocol === proto.id && styles.optionRowActive,
            ]}
            onPress={() => setSelectedProtocol(proto.id)}
          >
            <View style={styles.optionInfo}>
              <Text style={[
                styles.optionLabel,
                selectedProtocol === proto.id && styles.optionLabelActive,
              ]}>
                {proto.label}
              </Text>
              <Text style={styles.optionDesc}>{proto.desc}</Text>
            </View>
            <View style={[
              styles.radioButton,
              selectedProtocol === proto.id && styles.radioButtonActive,
            ]} />
          </TouchableOpacity>
        ))}
      </View>

      {/* Resolution */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Resolution</Text>
        <View style={styles.inputRow}>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Width (px)</Text>
            <TextInput
              style={styles.textInput}
              value={width}
              onChangeText={setWidth}
              keyboardType="numeric"
              editable={!isCapturing}
            />
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Height (px)</Text>
            <TextInput
              style={styles.textInput}
              value={height}
              onChangeText={setHeight}
              keyboardType="numeric"
              editable={!isCapturing}
            />
          </View>
        </View>
        <View style={styles.inputRow}>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Color Depth</Text>
            <View style={styles.segmentControl}>
              {[16, 24].map(depth => (
                <TouchableOpacity
                  key={depth}
                  style={[
                    styles.segmentBtn,
                    colorDepth === depth && styles.segmentBtnActive,
                  ]}
                  onPress={() => setColorDepth(depth)}
                >
                  <Text style={[
                    styles.segmentBtnText,
                    colorDepth === depth && styles.segmentBtnTextActive,
                  ]}>
                    {depth}-bit
                  </Text>
                </TouchableOpacity>
              ))}
            </View>
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Max FPS</Text>
            <View style={styles.segmentControl}>
              {[1, 5, 15, 30].map(fps => (
                <TouchableOpacity
                  key={fps}
                  style={[
                    styles.segmentBtn,
                    captureFps === fps && styles.segmentBtnActive,
                  ]}
                  onPress={() => setCaptureFps(fps)}
                >
                  <Text style={[
                    styles.segmentBtnText,
                    captureFps === fps && styles.segmentBtnTextActive,
                  ]}>
                    {fps}
                  </Text>
                </TouchableOpacity>
              ))}
            </View>
          </View>
        </View>
      </View>

      {/* Trigger mode */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Trigger Mode</Text>
        {TRIGGER_MODES.map(mode => (
          <TouchableOpacity
            key={mode.id}
            style={[
              styles.optionRow,
              triggerMode === mode.id && styles.optionRowActive,
            ]}
            onPress={() => setTriggerMode(mode.id)}
          >
            <View style={styles.optionInfo}>
              <Text style={[
                styles.optionLabel,
                triggerMode === mode.id && styles.optionLabelActive,
              ]}>
                {mode.label}
              </Text>
              <Text style={styles.optionDesc}>{mode.desc}</Text>
            </View>
            <View style={[
              styles.radioButton,
              triggerMode === mode.id && styles.radioButtonActive,
            ]} />
          </TouchableOpacity>
        ))}
      </View>

      {/* Processing options */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>On-Device Processing</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>OCR Text Extraction</Text>
          <Switch
            value={ocrEnabled}
            onValueChange={setOcrEnabled}
            trackColor={{ false: '#1E242C', true: '#00D4AA' }}
            thumbColor={ocrEnabled ? '#FFFFFF' : '#5C6877'}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Credential Detection</Text>
          <Switch
            value={patternEnabled}
            onValueChange={setPatternEnabled}
            trackColor={{ false: '#1E242C', true: '#00D4AA' }}
            thumbColor={patternEnabled ? '#FFFFFF' : '#5C6877'}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>QR/Barcode Decode</Text>
          <Switch
            value={qrEnabled}
            onValueChange={setQrEnabled}
            trackColor={{ false: '#1E242C', true: '#00D4AA' }}
            thumbColor={qrEnabled ? '#FFFFFF' : '#5C6877'}
          />
        </View>
      </View>

      {/* Start/Stop button */}
      <View style={styles.actionSection}>
        <TouchableOpacity
          style={[styles.actionBtn, isCapturing && styles.actionBtnStop]}
          onPress={isCapturing ? handleStopCapture : handleStartCapture}
          disabled={!connectionState.bleConnected && !connectionState.usbConnected}
        >
          <Text style={styles.actionBtnText}>
            {isCapturing ? '⏹ Stop Capture' : '▶ Start Capture'}
          </Text>
        </TouchableOpacity>
        {(!connectionState.bleConnected && !connectionState.usbConnected) && (
          <Text style={styles.warningText}>
            Connect to GLYPH-REAPER first
          </Text>
        )}
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E14',
  },
  section: {
    backgroundColor: '#131720',
    marginHorizontal: 12,
    marginVertical: 6,
    borderRadius: 10,
    padding: 16,
  },
  sectionTitle: {
    color: '#00D4AA',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 12,
  },
  optionRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 10,
    paddingHorizontal: 12,
    borderRadius: 8,
    marginBottom: 4,
    backgroundColor: '#0A0E14',
    borderWidth: 1,
    borderColor: '#1E242C',
  },
  optionRowActive: {
    borderColor: '#00D4AA',
    backgroundColor: '#00D4AA10',
  },
  optionInfo: {
    flex: 1,
  },
  optionLabel: {
    color: '#E6EDF3',
    fontSize: 15,
    fontWeight: '600',
  },
  optionLabelActive: {
    color: '#00D4AA',
  },
  optionDesc: {
    color: '#5C6877',
    fontSize: 12,
    marginTop: 2,
  },
  radioButton: {
    width: 20,
    height: 20,
    borderRadius: 10,
    borderWidth: 2,
    borderColor: '#5C6877',
  },
  radioButtonActive: {
    borderColor: '#00D4AA',
    backgroundColor: '#00D4AA',
  },
  inputRow: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  inputGroup: {
    flex: 1,
    marginHorizontal: 4,
  },
  inputLabel: {
    color: '#5C6877',
    fontSize: 12,
    marginBottom: 6,
  },
  textInput: {
    backgroundColor: '#0A0E14',
    borderWidth: 1,
    borderColor: '#1E242C',
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 10,
    color: '#E6EDF3',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  segmentControl: {
    flexDirection: 'row',
    backgroundColor: '#0A0E14',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#1E242C',
    overflow: 'hidden',
  },
  segmentBtn: {
    flex: 1,
    paddingVertical: 8,
    alignItems: 'center',
  },
  segmentBtnActive: {
    backgroundColor: '#00D4AA20',
  },
  segmentBtnText: {
    color: '#5C6877',
    fontSize: 13,
    fontWeight: '600',
  },
  segmentBtnTextActive: {
    color: '#00D4AA',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 10,
  },
  switchLabel: {
    color: '#E6EDF3',
    fontSize: 14,
    fontWeight: '500',
  },
  actionSection: {
    padding: 16,
    alignItems: 'center',
  },
  actionBtn: {
    width: '100%',
    paddingVertical: 16,
    alignItems: 'center',
    backgroundColor: '#00D4AA',
    borderRadius: 12,
  },
  actionBtnStop: {
    backgroundColor: '#FF4757',
  },
  actionBtnText: {
    color: '#0A0E14',
    fontSize: 18,
    fontWeight: '700',
  },
  warningText: {
    color: '#FF4757',
    fontSize: 14,
    marginTop: 8,
  },
});

export default CaptureScreen;