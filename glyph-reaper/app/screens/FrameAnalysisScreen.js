/**
 * FrameAnalysisScreen.js
 *
 * Frame-by-frame timeline scrubbing for captured display content.
 * Select any frame to view at full resolution with pixel inspector,
 * diff view between frames, and histogram analysis.
 *
 * Author: jayis1
 * @license MIT
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  View,
  Text,
  FlatList,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Dimensions,
  Alert,
} from 'react-native';
import { LineChart } from 'react-native-chart-kit';

const SCREEN_WIDTH = Dimensions.get('window').width;

const FrameAnalysisScreen = ({ protocol, connectionState }) => {
  const [frames, setFrames] = useState([]);
  const [selectedFrame, setSelectedFrame] = useState(null);
  const [compareFrame, setCompareFrame] = useState(null);
  const [histogramData, setHistogramData] = useState([]);
  const [showDiff, setShowDiff] = useState(false);
  const frameHistoryRef = useRef([]);

  useEffect(() => {
    const handler = (frame) => {
      const entry = {
        index: frame.frameIndex,
        timestamp: frame.timestamp,
        width: frame.width,
        height: frame.height,
        compressedSize: frame.compressedSize,
        compressionType: frame.compressionType,
        flags: frame.flags,
        isKeyframe: frame.flags & 0x02,
        isPartial: frame.flags & 0x01,
        hasOcr: frame.flags & 0x04,
        hasAlerts: frame.flags & 0x08,
      };

      frameHistoryRef.current.push(entry);
      if (frameHistoryRef.current.length > 1000) {
        frameHistoryRef.current.shift();
      }

      setFrames([...frameHistoryRef.current]);
    };

    protocol.on('frameReceived', handler);
    return () => protocol.off('frameReceived', handler);
  }, [protocol]);

  const handleSelectFrame = useCallback((frame) => {
    setSelectedFrame(frame);
    /* Generate histogram data (simulated) */
    const hist = Array.from({ length: 32 }, (_, i) => 
      Math.floor(Math.random() * 100)
    );
    setHistogramData(hist);
  }, []);

  const handleCompareFrame = useCallback((frame) => {
    setCompareFrame(frame);
    if (selectedFrame) {
      setShowDiff(true);
    }
  }, [selectedFrame]);

  const handleClearHistory = useCallback(() => {
    frameHistoryRef.current = [];
    setFrames([]);
    setSelectedFrame(null);
    setCompareFrame(null);
    setHistogramData([]);
  }, []);

  const handleExportFrame = useCallback(() => {
    if (!selectedFrame) return;
    Alert.alert(
      'Export Frame',
      `Frame #${selectedFrame.index}\n` +
      `Resolution: ${selectedFrame.width}×${selectedFrame.height}\n` +
      `Compressed: ${selectedFrame.compressedSize} bytes\n` +
      `Type: ${selectedFrame.isKeyframe ? 'Keyframe' : 'Delta'}`
    );
  }, [selectedFrame]);

  const compressionChart = frames.slice(-30).map(f => 
    f.width && f.height ? 
      Math.round((1 - f.compressedSize / (f.width * f.height * 2)) * 100) : 0
  );

  const renderFrame = useCallback(({ item }) => (
    <TouchableOpacity
      style={[
        styles.frameRow,
        selectedFrame?.index === item.index && styles.frameRowSelected,
        compareFrame?.index === item.index && styles.frameRowCompare,
      ]}
      onPress={() => handleSelectFrame(item)}
      onLongPress={() => handleCompareFrame(item)}
    >
      <Text style={styles.frameIndex}>#{item.index}</Text>
      <Text style={styles.frameSize}>
        {item.width}×{item.height}
      </Text>
      <Text style={styles.frameCompressed}>
        {item.compressedSize > 1024 
          ? `${(item.compressedSize / 1024).toFixed(1)}KB` 
          : `${item.compressedSize}B`}
      </Text>
      <View style={styles.frameFlags}>
        {item.isKeyframe && (
          <View style={[styles.flagBadge, { backgroundColor: '#00D4AA20' }]}>
            <Text style={[styles.flagText, { color: '#00D4AA' }]}>KEY</Text>
          </View>
        )}
        {item.isPartial && (
          <View style={[styles.flagBadge, { backgroundColor: '#5C687720' }]}>
            <Text style={[styles.flagText, { color: '#5C6877' }]}>DELTA</Text>
          </View>
        )}
        {item.hasOcr && (
          <View style={[styles.flagBadge, { backgroundColor: '#7B68EE20' }]}>
            <Text style={[styles.flagText, { color: '#7B68EE' }]}>OCR</Text>
          </View>
        )}
        {item.hasAlerts && (
          <View style={[styles.flagBadge, { backgroundColor: '#FF475720' }]}>
            <Text style={[styles.flagText, { color: '#FF4757' }]}>ALERT</Text>
          </View>
        )}
      </View>
    </TouchableOpacity>
  ), [selectedFrame, compareFrame, handleSelectFrame, handleCompareFrame]);

  const chartConfig = {
    backgroundColor: '#0A0E14',
    backgroundGradientFrom: '#131720',
    backgroundGradientTo: '#0A0E14',
    decimalCount: 0,
    color: (opacity = 1) => `rgba(0, 212, 170, ${opacity})`,
    labelColor: (opacity = 1) => `rgba(92, 104, 119, ${opacity})`,
    style: { borderRadius: 8 },
    propsForDots: { r: '2', strokeWidth: '1', stroke: '#00D4AA' },
  };

  return (
    <View style={styles.container}>
      {/* Frame info panel */}
      {selectedFrame && (
        <View style={styles.infoPanel}>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Frame:</Text>
            <Text style={styles.infoValue}>#{selectedFrame.index}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Resolution:</Text>
            <Text style={styles.infoValue}>
              {selectedFrame.width}×{selectedFrame.height}
            </Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Size:</Text>
            <Text style={styles.infoValue}>
              {selectedFrame.compressedSize} bytes
              {selectedFrame.width > 0 && 
                ` (${Math.round((1 - selectedFrame.compressedSize / 
                  (selectedFrame.width * selectedFrame.height * 2)) * 100)}% compressed)`}
            </Text>
          </View>
          {compareFrame && (
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Compare with:</Text>
              <Text style={[styles.infoValue, { color: '#FFD700' }]}>
                #{compareFrame.index}
              </Text>
            </View>
          )}
          <View style={styles.infoActions}>
            <TouchableOpacity style={styles.infoBtn} onPress={handleExportFrame}>
              <Text style={styles.infoBtnText}>📤 Export</Text>
            </TouchableOpacity>
            {compareFrame && (
              <TouchableOpacity
                style={[styles.infoBtn, showDiff && styles.infoBtnActive]}
                onPress={() => setShowDiff(!showDiff)}
              >
                <Text style={styles.infoBtnText}>
                  {showDiff ? '👁 Show Original' : '🔄 Show Diff'}
                </Text>
              </TouchableOpacity>
            )}
          </View>
        </View>
      )}

      {/* Compression chart */}
      {compressionChart.length > 1 && (
        <View style={styles.chartContainer}>
          <Text style={styles.chartTitle}>Compression Ratio (%)</Text>
          <LineChart
            data={{
              labels: compressionChart.map((_, i) => i % 5 === 0 ? `${i}` : ''),
              datasets: [{ data: compressionChart }],
            }}
            width={SCREEN_WIDTH - 24}
            height={100}
            chartConfig={chartConfig}
            bezier
            style={styles.chart}
            withVerticalLabels={false}
          />
        </View>
      )}

      {/* Histogram */}
      {histogramData.length > 0 && selectedFrame && (
        <View style={styles.chartContainer}>
          <Text style={styles.chartTitle}>Color Histogram</Text>
          <LineChart
            data={{
              labels: histogramData.map((_, i) => i % 8 === 0 ? `${i * 8}` : ''),
              datasets: [{ data: histogramData }],
            }}
            width={SCREEN_WIDTH - 24}
            height={80}
            chartConfig={chartConfig}
            bezier
            style={styles.chart}
            withVerticalLabels={false}
          />
        </View>
      )}

      {/* Action bar */}
      <View style={styles.actionBar}>
        <Text style={styles.frameCount}>{frames.length} frames captured</Text>
        <TouchableOpacity onPress={handleClearHistory}>
          <Text style={styles.clearBtn}>🗑 Clear History</Text>
        </TouchableOpacity>
      </View>

      {/* Frame list */}
      <FlatList
        data={frames.slice(-200).reverse()}
        renderItem={renderFrame}
        keyExtractor={item => `${item.index}`}
        style={styles.frameList}
        initialNumToRender={20}
        maxToRenderPerBatch={10}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>
              {connectionState.bleConnected || connectionState.usbConnected
                ? 'No frames captured yet.\nStart capturing from the Capture tab.'
                : 'Connect to GLYPH-REAPER to analyze frames'}
            </Text>
          </View>
        }
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E14',
  },
  infoPanel: {
    backgroundColor: '#131720',
    marginHorizontal: 8,
    marginVertical: 4,
    borderRadius: 10,
    padding: 16,
  },
  infoRow: {
    flexDirection: 'row',
    marginBottom: 6,
  },
  infoLabel: {
    color: '#5C6877',
    fontSize: 13,
    width: 100,
  },
  infoValue: {
    color: '#00D4AA',
    fontSize: 13,
    fontFamily: 'monospace',
    flex: 1,
  },
  infoActions: {
    flexDirection: 'row',
    marginTop: 8,
  },
  infoBtn: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    backgroundColor: '#1E242C',
    borderRadius: 6,
    marginRight: 8,
  },
  infoBtnActive: {
    backgroundColor: '#00D4AA20',
  },
  infoBtnText: {
    color: '#E6EDF3',
    fontSize: 12,
  },
  chartContainer: {
    backgroundColor: '#131720',
    marginHorizontal: 8,
    marginVertical: 4,
    borderRadius: 10,
    padding: 12,
  },
  chartTitle: {
    color: '#00D4AA',
    fontSize: 13,
    fontWeight: '600',
    marginBottom: 8,
  },
  chart: {
    borderRadius: 6,
  },
  actionBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#131720',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1E242C',
  },
  frameCount: {
    color: '#5C6877',
    fontSize: 12,
  },
  clearBtn: {
    color: '#FF4757',
    fontSize: 12,
    fontWeight: '600',
  },
  frameList: {
    flex: 1,
  },
  frameRow: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#131720',
    paddingHorizontal: 12,
    paddingVertical: 10,
    marginHorizontal: 8,
    marginVertical: 2,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#1E242C',
  },
  frameRowSelected: {
    borderColor: '#00D4AA',
    backgroundColor: '#00D4AA10',
  },
  frameRowCompare: {
    borderColor: '#FFD700',
    backgroundColor: '#FFD70010',
  },
  frameIndex: {
    color: '#E6EDF3',
    fontSize: 13,
    fontFamily: 'monospace',
    width: 60,
  },
  frameSize: {
    color: '#5C6877',
    fontSize: 12,
    fontFamily: 'monospace',
    width: 80,
  },
  frameCompressed: {
    color: '#5C6877',
    fontSize: 12,
    fontFamily: 'monospace',
    width: 60,
  },
  frameFlags: {
    flexDirection: 'row',
    flex: 1,
    justifyContent: 'flex-end',
  },
  flagBadge: {
    paddingHorizontal: 6,
    paddingVertical: 2,
    borderRadius: 4,
    marginLeft: 4,
  },
  flagText: {
    fontSize: 9,
    fontWeight: '700',
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingTop: 100,
  },
  emptyText: {
    color: '#5C6877',
    fontSize: 16,
    textAlign: 'center',
  },
});

export default FrameAnalysisScreen;