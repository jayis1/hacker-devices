/**
 * SnifferScreen.js
 *
 * Real-time CAN bus frame monitor with filtering, DBC decoding,
 * and signal graphing capabilities.
 *
 * Displays live CAN frames received from the CAN Creeper device
 * with hardware-timestamped precision. Supports filtering by
 * CAN ID, channel selection, and DBC-based signal decoding.
 */

import React, { useState, useEffect, useCallback, useRef, useMemo } from 'react';
import {
  View,
  Text,
  FlatList,
  TouchableOpacity,
  TextInput,
  Switch,
  StyleSheet,
  ScrollView,
  Dimensions,
  Animated,
} from 'react-native';
import { LineChart } from 'react-native-chart-kit';

const SCREEN_WIDTH = Dimensions.get('window').width;

/* CAN frame display structure */
const FrameItem = React.memo(({ frame, isExpanded, onToggle, dbcDecoded }) => {
  const idStr = frame.ide
    ? `0x${frame.id.toString(16).toUpperCase().padStart(8, '0')}`
    : `0x${frame.id.toString(16).toUpperCase().padStart(3, '0')}`;

  const dlcToLen = (dlc) => {
    const map = [0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64];
    return map[dlc] || 0;
  };

  const dataHex = frame.data.slice(0, dlcToLen(frame.dlc))
    .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
    .join(' ');

  const flags = [];
  if (frame.fd) flags.push('FD');
  if (frame.brs) flags.push('BRS');
  if (frame.esi) flags.push('ESI');
  if (frame.rtr) flags.push('RTR');
  if (frame.ide) flags.push('EXT');

  const timestampMs = (frame.timestamp / 1000).toFixed(3);

  return (
    <TouchableOpacity
      style={styles.frameItem}
      onPress={onToggle}
      activeOpacity={0.7}
    >
      <View style={styles.frameHeader}>
        <View style={styles.frameIdContainer}>
          <Text style={styles.frameId}>{idStr}</Text>
          {flags.length > 0 && (
            <View style={styles.flagsContainer}>
              {flags.map((flag, i) => (
                <View key={i} style={styles.flagBadge}>
                  <Text style={styles.flagText}>{flag}</Text>
                </View>
              ))}
            </View>
          )}
        </View>
        <View style={styles.frameMeta}>
          <Text style={styles.frameChannel}>
            CH{frame.channel}
          </Text>
          <Text style={styles.frameTimestamp}>
            {timestampMs}ms
          </Text>
          <Text style={styles.frameDlc}>
            DLC:{frame.dlc}
          </Text>
        </View>
      </View>

      <View style={styles.frameDataRow}>
        <Text style={styles.frameDataHex} numberOfLines={2}>
          {dataHex}
        </Text>
      </View>

      {isExpanded && dbcDecoded && (
        <View style={styles.dbcDecodedContainer}>
          <Text style={styles.dbcTitle}>DBC Decoded:</Text>
          {dbcDecoded.signals.map((signal, idx) => (
            <View key={idx} style={styles.signalRow}>
              <Text style={styles.signalName}>{signal.name}:</Text>
              <Text style={styles.signalValue}>
                {signal.value.toFixed(signal.precision)} {signal.unit}
              </Text>
            </View>
          ))}
        </View>
      )}

      {isExpanded && !dbcDecoded && (
        <View style={styles.rawDataContainer}>
          <Text style={styles.rawDataTitle}>Raw Data:</Text>
          {frame.data.slice(0, dlcToLen(frame.dlc)).map((byte, idx) => (
            <View key={idx} style={styles.byteRow}>
              <Text style={styles.byteIndex}>Byte {idx}:</Text>
              <Text style={styles.byteHex}>0x{byte.toString(16).toUpperCase().padStart(2, '0')}</Text>
              <Text style={styles.byteDec}>({byte})</Text>
              <Text style={styles.byteBin}>
                {byte.toString(2).padStart(8, '0')}
              </Text>
            </View>
          ))}
        </View>
      )}
    </TouchableOpacity>
  );
});

/**
 * Sniffer Screen Component
 */
const SnifferScreen = ({ protocol, deviceStatus, connectionState }) => {
  const [frames, setFrames] = useState([]);
  const [filterId, setFilterId] = useState('');
  const [filterMask, setFilterMask] = useState('');
  const [selectedChannel, setSelectedChannel] = useState(0xFF); /* 0xFF = both */
  const [expandedFrameId, setExpandedFrameId] = useState(null);
  const [paused, setPaused] = useState(false);
  const [showGraph, setShowGraph] = useState(false);
  const [graphSignal, setGraphSignal] = useState(null);
  const [graphData, setGraphData] = useState([]);
  const [dbcFile, setDbcFile] = useState(null);
  const flatListRef = useRef(null);
  const pausedFrames = useRef([]);
  const maxFrames = 10000;

  /* Subscribe to frame events */
  useEffect(() => {
    const handler = (frame) => {
      if (paused) {
        pausedFrames.current.push(frame);
        if (pausedFrames.current.length > maxFrames) {
          pausedFrames.current.shift();
        }
        return;
      }

      /* Apply channel filter */
      if (selectedChannel !== 0xFF && frame.channel !== selectedChannel) {
        return;
      }

      /* Apply ID filter */
      if (filterId) {
        const filterIdNum = parseInt(filterId, 16);
        const filterMaskNum = filterMask ? parseInt(filterMask, 16) : 0xFFFFFFFF;
        if ((frame.id & filterMaskNum) !== (filterIdNum & filterMaskNum)) {
          return;
        }
      }

      setFrames(prev => {
        const updated = [frame, ...prev];
        if (updated.length > maxFrames) {
          updated.pop();
        }
        return updated;
      });

      /* Update graph data if graphing */
      if (showGraph && graphSignal && dbcFile) {
        const decoded = decodeFrameWithDBC(frame, dbcFile);
        if (decoded) {
          const signal = decoded.signals.find(s => s.name === graphSignal);
          if (signal) {
            setGraphData(prev => {
              const updated = [...prev, signal.value];
              if (updated.length > 200) updated.shift();
              return updated;
            });
          }
        }
      }
    };

    protocol.on('frameReceived', handler);
    return () => protocol.off('frameReceived', handler);
  }, [paused, selectedChannel, filterId, filterMask, showGraph, graphSignal, dbcFile]);

  /* Resume from pause */
  const handleResume = useCallback(() => {
    setPaused(false);
    /* Flush paused frames */
    const buffered = pausedFrames.current;
    pausedFrames.current = [];
    setFrames(prev => {
      const updated = [...buffered, ...prev];
      if (updated.length > maxFrames) {
        return updated.slice(0, maxFrames);
      }
      return updated;
    });
  }, []);

  /* Clear all frames */
  const handleClear = useCallback(() => {
    setFrames([]);
    pausedFrames.current = [];
    setGraphData([]);
  }, []);

  /* Toggle frame expansion */
  const handleToggleFrame = useCallback((frameId) => {
    setExpandedFrameId(prev => prev === frameId ? null : frameId);
  }, []);

  /* Decode frame with DBC (simplified) */
  const decodeFrameWithDBC = (frame, dbc) => {
    if (!dbc || !dbc.messages) return null;
    const msg = dbc.messages.find(m => m.id === frame.id);
    if (!msg) return null;

    const signals = msg.signals.map(sig => {
      /* Extract bits from frame data */
      let rawValue = 0;
      const startBit = sig.startBit;
      const length = sig.length;
      const byteIdx = Math.floor(startBit / 8);
      const bitOffset = startBit % 8;

      for (let i = 0; i < length; i++) {
        const totalBit = startBit + i;
        const bIdx = Math.floor(totalBit / 8);
        const bOff = totalBit % 8;
        if (bIdx < frame.data.length) {
          if (frame.data[bIdx] & (1 << bOff)) {
            rawValue |= (1 << i);
          }
        }
      }

      /* Apply scaling and offset */
      const value = rawValue * sig.factor + sig.offset;
      return {
        name: sig.name,
        value,
        unit: sig.unit || '',
        precision: sig.precision || 1,
      };
    });

    return { signals };
  };

  /* Memoized frame list with DBC decoding */
  const frameList = useMemo(() => {
    return frames.map((frame, index) => ({
      ...frame,
      _key: `${frame.timestamp}-${frame.id}-${index}`,
      dbcDecoded: dbcFile ? decodeFrameWithDBC(frame, dbcFile) : null,
    }));
  }, [frames, dbcFile]);

  /* Render frame item */
  const renderFrame = useCallback(({ item }) => (
    <FrameItem
      frame={item}
      isExpanded={expandedFrameId === item._key}
      onToggle={() => handleToggleFrame(item._key)}
      dbcDecoded={item.dbcDecoded}
    />
  ), [expandedFrameId, handleToggleFrame]);

  /* Graph configuration */
  const chartConfig = {
    backgroundColor: '#0D1117',
    backgroundGradientFrom: '#161B22',
    backgroundGradientTo: '#0D1117',
    decimalCount: 2,
    color: (opacity = 1) => `rgba(255, 107, 0, ${opacity})`,
    labelColor: (opacity = 1) => `rgba(230, 237, 243, ${opacity})`,
    style: { borderRadius: 8 },
    propsForDots: { r: '2', strokeWidth: '1', stroke: '#FF6B00' },
  };

  return (
    <View style={styles.container}>
      {/* Control bar */}
      <View style={styles.controlBar}>
        <View style={styles.channelSelector}>
          <TouchableOpacity
            style={[styles.channelBtn, selectedChannel === 0xFF && styles.channelBtnActive]}
            onPress={() => setSelectedChannel(0xFF)}
          >
            <Text style={[styles.channelBtnText, selectedChannel === 0xFF && styles.channelBtnTextActive]}>
              Both
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.channelBtn, selectedChannel === 0 && styles.channelBtnActive]}
            onPress={() => setSelectedChannel(0)}
          >
            <Text style={[styles.channelBtnText, selectedChannel === 0 && styles.channelBtnTextActive]}>
              CH0
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.channelBtn, selectedChannel === 1 && styles.channelBtnActive]}
            onPress={() => setSelectedChannel(1)}
          >
            <Text style={[styles.channelBtnText, selectedChannel === 1 && styles.channelBtnTextActive]}>
              CH1
            </Text>
          </TouchableOpacity>
        </View>

        <View style={styles.filterRow}>
          <TextInput
            style={styles.filterInput}
            placeholder="ID (hex)"
            placeholderTextColor="#484F58"
            value={filterId}
            onChangeText={setFilterId}
            autoCapitalize="none"
            autoCorrect={false}
          />
          <TextInput
            style={styles.filterInput}
            placeholder="Mask (hex)"
            placeholderTextColor="#484F58"
            value={filterMask}
            onChangeText={setFilterMask}
            autoCapitalize="none"
            autoCorrect={false}
          />
        </View>

        <View style={styles.actionRow}>
          <TouchableOpacity
            style={[styles.actionBtn, paused && styles.actionBtnActive]}
            onPress={paused ? handleResume : () => setPaused(true)}
          >
            <Text style={styles.actionBtnText}>
              {paused ? '▶ Resume' : '⏸ Pause'}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionBtn} onPress={handleClear}>
            <Text style={styles.actionBtnText}>🗑 Clear</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.actionBtn, showGraph && styles.actionBtnActive]}
            onPress={() => setShowGraph(!showGraph)}
          >
            <Text style={styles.actionBtnText}>📈 Graph</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Frame counter */}
      <View style={styles.counterBar}>
        <Text style={styles.counterText}>
          Frames: {frames.length} | CH0: {deviceStatus.can0FrameCount} | CH1: {deviceStatus.can1FrameCount}
        </Text>
        {(deviceStatus.can0Overflow > 0 || deviceStatus.can1Overflow > 0) && (
          <Text style={styles.overflowWarning}>
            ⚠ OVF: CH0={deviceStatus.can0Overflow} CH1={deviceStatus.can1Overflow}
          </Text>
        )}
      </View>

      {/* Signal graph */}
      {showGraph && (
        <View style={styles.graphContainer}>
          <Text style={styles.graphTitle}>Signal Graph</Text>
          {graphData.length > 0 ? (
            <LineChart
              data={{
                labels: graphData.slice(-10).map((_, i) => `${i}`),
                datasets: [{ data: graphData.slice(-50) }],
              }}
              width={SCREEN_WIDTH - 32}
              height={180}
              chartConfig={chartConfig}
              bezier
              style={styles.chart}
            />
          ) : (
            <Text style={styles.noGraphData}>
              Waiting for matching CAN frames...
            </Text>
          )}
        </View>
      )}

      {/* Frame list */}
      <FlatList
        ref={flatListRef}
        data={frameList}
        renderItem={renderFrame}
        keyExtractor={item => item._key}
        style={styles.frameList}
        contentContainerStyle={styles.frameListContent}
        initialNumToRender={20}
        maxToRenderPerBatch={10}
        windowSize={5}
        removeClippedSubviews={true}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>
              {connectionState.bleConnected || connectionState.usbConnected
                ? 'Waiting for CAN frames...'
                : 'Connect to CAN Creeper to start sniffing'}
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
    backgroundColor: '#0D1117',
  },
  controlBar: {
    backgroundColor: '#161B22',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#30363D',
  },
  channelSelector: {
    flexDirection: 'row',
    marginBottom: 8,
  },
  channelBtn: {
    flex: 1,
    paddingVertical: 6,
    alignItems: 'center',
    backgroundColor: '#21262D',
    marginHorizontal: 2,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  channelBtnActive: {
    backgroundColor: '#FF6B0020',
    borderColor: '#FF6B00',
  },
  channelBtnText: {
    color: '#8B949E',
    fontSize: 13,
    fontWeight: '600',
  },
  channelBtnTextActive: {
    color: '#FF6B00',
  },
  filterRow: {
    flexDirection: 'row',
    marginBottom: 8,
  },
  filterInput: {
    flex: 1,
    backgroundColor: '#0D1117',
    borderWidth: 1,
    borderColor: '#30363D',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    color: '#E6EDF3',
    fontSize: 13,
    marginHorizontal: 2,
    fontFamily: 'monospace',
  },
  actionRow: {
    flexDirection: 'row',
  },
  actionBtn: {
    flex: 1,
    paddingVertical: 6,
    alignItems: 'center',
    backgroundColor: '#21262D',
    marginHorizontal: 2,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  actionBtnActive: {
    backgroundColor: '#FF6B0020',
    borderColor: '#FF6B00',
  },
  actionBtnText: {
    color: '#E6EDF3',
    fontSize: 12,
    fontWeight: '600',
  },
  counterBar: {
    backgroundColor: '#161B22',
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderBottomWidth: 1,
    borderBottomColor: '#30363D',
  },
  counterText: {
    color: '#8B949E',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  overflowWarning: {
    color: '#FF4444',
    fontSize: 11,
    fontFamily: 'monospace',
    marginTop: 2,
  },
  graphContainer: {
    backgroundColor: '#161B22',
    padding: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#30363D',
  },
  graphTitle: {
    color: '#E6EDF3',
    fontSize: 14,
    fontWeight: '600',
    marginBottom: 8,
  },
  chart: {
    borderRadius: 8,
  },
  noGraphData: {
    color: '#8B949E',
    fontSize: 13,
    textAlign: 'center',
    paddingVertical: 20,
  },
  frameList: {
    flex: 1,
  },
  frameListContent: {
    paddingBottom: 20,
  },
  frameItem: {
    backgroundColor: '#161B22',
    marginHorizontal: 8,
    marginTop: 4,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#30363D',
    padding: 10,
  },
  frameHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  frameIdContainer: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  frameId: {
    color: '#FF6B00',
    fontSize: 15,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  flagsContainer: {
    flexDirection: 'row',
    marginLeft: 6,
  },
  flagBadge: {
    backgroundColor: '#FF6B0030',
    borderRadius: 4,
    paddingHorizontal: 4,
    paddingVertical: 1,
    marginLeft: 3,
  },
  flagText: {
    color: '#FF6B00',
    fontSize: 9,
    fontWeight: '700',
  },
  frameMeta: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  frameChannel: {
    color: '#58A6FF',
    fontSize: 12,
    fontWeight: '600',
    marginRight: 8,
  },
  frameTimestamp: {
    color: '#8B949E',
    fontSize: 11,
    fontFamily: 'monospace',
    marginRight: 8,
  },
  frameDlc: {
    color: '#8B949E',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  frameDataRow: {
    marginTop: 6,
  },
  frameDataHex: {
    color: '#7EE787',
    fontSize: 12,
    fontFamily: 'monospace',
    lineHeight: 18,
  },
  dbcDecodedContainer: {
    marginTop: 8,
    backgroundColor: '#0D1117',
    borderRadius: 6,
    padding: 8,
  },
  dbcTitle: {
    color: '#E6EDF3',
    fontSize: 12,
    fontWeight: '600',
    marginBottom: 4,
  },
  signalRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 2,
  },
  signalName: {
    color: '#8B949E',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  signalValue: {
    color: '#E6EDF3',
    fontSize: 11,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  rawDataContainer: {
    marginTop: 8,
    backgroundColor: '#0D1117',
    borderRadius: 6,
    padding: 8,
  },
  rawDataTitle: {
    color: '#E6EDF3',
    fontSize: 12,
    fontWeight: '600',
    marginBottom: 4,
  },
  byteRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 1,
  },
  byteIndex: {
    color: '#8B949E',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 50,
  },
  byteHex: {
    color: '#7EE787',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 45,
  },
  byteDec: {
    color: '#E6EDF3',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 40,
  },
  byteBin: {
    color: '#8B949E',
    fontSize: 9,
    fontFamily: 'monospace',
  },
  emptyContainer: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 60,
  },
  emptyText: {
    color: '#484F58',
    fontSize: 15,
    textAlign: 'center',
  },
});

export default SnifferScreen;
