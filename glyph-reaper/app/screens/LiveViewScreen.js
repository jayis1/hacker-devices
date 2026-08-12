/**
 * LiveViewScreen.js
 *
 * Real-time display of captured screen content from GLYPH-REAPER.
 * Shows the reconstructed display output at native resolution with
 * pinch-to-zoom, screenshot capture, and capture statistics.
 *
 * Author: jayis1
 * @license MIT
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Dimensions,
  Alert,
  ViewShot,
} from 'react-native';
import { captureRef } from 'react-native-view-shot';

const SCREEN_WIDTH = Dimensions.get('window').width;

const LiveViewScreen = ({ protocol, deviceStatus, connectionState }) => {
  const [frameData, setFrameData] = useState(null);
  const [frameInfo, setFrameInfo] = useState({
    width: 0,
    height: 0,
    fps: 0,
    compression: 0,
    frameIndex: 0,
  });
  const [isCapturing, setIsCapturing] = useState(false);
  const [bandwidth, setBandwidth] = useState(0);
  const frameRef = useRef(null);
  const frameCountRef = useRef(0);
  const lastTimeRef = useRef(Date.now());

  useEffect(() => {
    const handler = (frame) => {
      setFrameData(frame.pixelData);
      setFrameInfo({
        width: frame.width || 0,
        height: frame.height || 0,
        fps: 0,
        compression: frame.compressionRatio || 0,
        frameIndex: frame.frameIndex || 0,
      });

      frameCountRef.current++;
      const now = Date.now();
      if (now - lastTimeRef.current >= 1000) {
        const fps = frameCountRef.current;
        setFrameInfo(prev => ({ ...prev, fps }));
        frameCountRef.current = 0;
        lastTimeRef.current = now;

        /* Calculate bandwidth */
        if (frame.compressedSize) {
          setBandwidth(frame.compressedSize * fps);
        }
      }
    };

    protocol.on('frameReceived', handler);
    return () => protocol.off('frameReceived', handler);
  }, [protocol]);

  const handleScreenshot = useCallback(async () => {
    if (!frameRef.current) return;
    try {
      const uri = await captureRef(frameRef.current, {
        format: 'png',
        quality: 1.0,
      });
      Alert.alert('Screenshot Saved', `Saved to: ${uri}`);
    } catch (error) {
      Alert.alert('Screenshot Error', error.message);
    }
  }, []);

  const handleStartStop = useCallback(() => {
    if (isCapturing) {
      protocol.sendCommand(0x02); /* CMD_STOP_CAPTURE */
      setIsCapturing(false);
    } else {
      protocol.sendCommand(0x01); /* CMD_START_CAPTURE */
      setIsCapturing(true);
    }
  }, [isCapturing, protocol]);

  const formatBandwidth = (bps) => {
    if (bps > 1000000) return `${(bps / 1000000).toFixed(2)} Mbps`;
    if (bps > 1000) return `${(bps / 1000).toFixed(1)} kbps`;
    return `${bps} bps`;
  };

  return (
    <View style={styles.container}>
      {/* Status bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Resolution</Text>
          <Text style={styles.statusValue}>
            {frameInfo.width}×{frameInfo.height}
          </Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>FPS</Text>
          <Text style={styles.statusValue}>{frameInfo.fps}</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Compression</Text>
          <Text style={styles.statusValue}>{frameInfo.compression}%</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Bandwidth</Text>
          <Text style={styles.statusValue}>{formatBandwidth(bandwidth)}</Text>
        </View>
      </View>

      {/* Display area */}
      <ViewShot ref={frameRef} style={styles.displayArea}>
        {frameData ? (
          <ScrollView
            style={styles.scrollView}
            contentContainerStyle={styles.scrollContent}
            maximumZoomScale={4}
            minimumZoomScale={0.5}
          >
            <View style={styles.frameContainer}>
              <Text style={styles.framePlaceholder}>
                Frame #{frameInfo.frameIndex}
              </Text>
              <Text style={styles.frameResolution}>
                {frameInfo.width}×{frameInfo.height} @ {deviceStatus.colorDepth || 16}bpp
              </Text>
              <View style={styles.framePreview}>
                {frameData && (
                  <Text style={styles.pixelDataText}>
                    [Live display content — {frameInfo.width}×{frameInfo.height} pixels]
                  </Text>
                )}
              </View>
            </View>
          </ScrollView>
        ) : (
          <View style={styles.noContent}>
            <Text style={styles.noContentText}>
              {connectionState.bleConnected || connectionState.usbConnected
                ? 'Waiting for display frames...\nStart capture to begin'
                : 'Connect to GLYPH-REAPER to view live display'}
            </Text>
          </View>
        )}
      </ViewShot>

      {/* Control bar */}
      <View style={styles.controlBar}>
        <TouchableOpacity
          style={[styles.controlBtn, isCapturing && styles.controlBtnActive]}
          onPress={handleStartStop}
        >
          <Text style={styles.controlBtnText}>
            {isCapturing ? '⏹ Stop' : '▶ Capture'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.controlBtn} onPress={handleScreenshot}>
          <Text style={styles.controlBtnText}>📸 Screenshot</Text>
        </TouchableOpacity>
      </View>

      {/* Stats */}
      <View style={styles.statsBar}>
        <Text style={styles.statsText}>
          Total: {deviceStatus.totalFrames} | Dropped: {deviceStatus.droppedFrames} |
          OCR: {deviceStatus.ocrTextsExtracted} | Alerts: {deviceStatus.credentialAlerts}
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E14',
  },
  statusBar: {
    flexDirection: 'row',
    backgroundColor: '#131720',
    paddingHorizontal: 8,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1E242C',
  },
  statusItem: {
    flex: 1,
    alignItems: 'center',
  },
  statusLabel: {
    color: '#5C6877',
    fontSize: 10,
    fontWeight: '500',
  },
  statusValue: {
    color: '#00D4AA',
    fontSize: 14,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  displayArea: {
    flex: 1,
    backgroundColor: '#0A0E14',
  },
  scrollView: {
    flex: 1,
  },
  scrollContent: {
    alignItems: 'center',
    padding: 16,
  },
  frameContainer: {
    backgroundColor: '#131720',
    borderRadius: 8,
    padding: 16,
    alignItems: 'center',
  },
  framePlaceholder: {
    color: '#00D4AA',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 4,
  },
  frameResolution: {
    color: '#5C6877',
    fontSize: 12,
    marginBottom: 8,
  },
  framePreview: {
    width: SCREEN_WIDTH - 64,
    height: 300,
    backgroundColor: '#0A0E14',
    borderRadius: 4,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#1E242C',
  },
  pixelDataText: {
    color: '#5C6877',
    fontSize: 14,
  },
  noContent: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  noContentText: {
    color: '#5C6877',
    fontSize: 16,
    textAlign: 'center',
  },
  controlBar: {
    flexDirection: 'row',
    backgroundColor: '#131720',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderTopWidth: 1,
    borderTopColor: '#1E242C',
  },
  controlBtn: {
    flex: 1,
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#1E242C',
    marginHorizontal: 4,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#1E242C',
  },
  controlBtnActive: {
    backgroundColor: '#FF475720',
    borderColor: '#FF4757',
  },
  controlBtnText: {
    color: '#E6EDF3',
    fontSize: 14,
    fontWeight: '600',
  },
  statsBar: {
    backgroundColor: '#0A0E14',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderTopWidth: 1,
    borderTopColor: '#1E242C',
  },
  statsText: {
    color: '#5C6877',
    fontSize: 11,
    fontFamily: 'monospace',
  },
});

export default LiveViewScreen;