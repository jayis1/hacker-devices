/**
 * Tactile-Phantom — Companion App
 * screens/LiveCaptureScreen.tsx — Real-time touch visualization
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, Dimensions,
} from 'react-native';
import { useStore, EV_TYPE_NAMES, VENDOR_NAMES } from '../src/store';
import { TouchEvent } from '../src/ble';

const SCREEN_W = Dimensions.get('window').width;
const CANVAS_H = 500;

export default function LiveCaptureScreen() {
  const events = useStore((s) => s.events);
  const status = useStore((s) => s.status);
  const [currentTouches, setCurrentTouches] = useState<TouchEvent | null>(null);
  const lastEventRef = useRef<number>(0);

  // Update current touches from latest event
  useEffect(() => {
    if (events.length === 0) return;
    const latest = events[events.length - 1];
    if (latest.timestamp !== lastEventRef.current) {
      lastEventRef.current = latest.timestamp;
      if (latest.type === 1) {  // TOUCH
        setCurrentTouches(latest);
      } else if (latest.type === 2) {  // RELEASE
        setCurrentTouches(null);
      }
    }
  }, [events]);

  // Scale touch coordinates to canvas
  const scaleX = (x: number) => {
    const res = status?.xResolution || 1080;
    return (x / res) * (SCREEN_W - 40) + 20;
  };
  const scaleY = (y: number) => {
    const res = status?.yResolution || 2400;
    return (y / res) * CANVAS_H + 20;
  };

  const recentEvents = events.slice(-20).reverse();

  return (
    <View style={styles.container}>
      {/* Status bar */}
      <View style={styles.statusBar}>
        <Text style={styles.statusText}>
          {status?.attached ? '● CAPTURING' : '○ IDLE'}
        </Text>
        <Text style={styles.statusDetail}>
          {status ? `${VENDOR_NAMES[status.vendor] || '?'} ` +
           `${status.mode === 1 ? 'I2C' : 'SPI'} @0x${status.i2cAddr.toString(16)}` : 'No bus'}
        </Text>
        <Text style={styles.statusCounters}>
          TXN: {status?.totalTransactions.toLocaleString() || 0} ·
          EV: {status?.totalEvents.toLocaleString() || 0}
        </Text>
        <Text style={styles.statusBattery}>
          BAT: {status ? (status.batteryMv / 1000).toFixed(2) : '0.00'}V
        </Text>
      </View>

      {/* Touch canvas */}
      <View style={styles.canvasContainer}>
        <View style={styles.canvas}>
          {/* Device outline */}
          <View style={styles.deviceOutline} />

          {/* Touch points */}
          {currentTouches?.fingers.map((f, i) => (
            <View
              key={i}
              style={[
                styles.touchPoint,
                {
                  left: scaleX(f.x) - 12,
                  top: scaleY(f.y) - 12,
                },
              ]}
            >
              <Text style={styles.touchLabel}>{f.id}</Text>
            </View>
          ))}
        </View>
        <Text style={styles.canvasLabel}>
          {currentTouches
            ? `${currentTouches.fingerCount} finger(s)`
            : 'No touch'}
        </Text>
      </View>

      {/* Recent events */}
      <View style={styles.eventList}>
        <Text style={styles.eventListTitle}>Recent Events</Text>
        {recentEvents.slice(0, 10).map((ev, i) => (
          <View key={i} style={styles.eventRow}>
            <Text style={styles.eventType}>
              {EV_TYPE_NAMES[ev.type] || `T${ev.type}`}
            </Text>
            <Text style={styles.eventCoords}>
              {ev.fingers.length > 0
                ? `(${ev.fingers[0].x}, ${ev.fingers[0].y})`
                : ev.gestureId !== undefined
                ? `G:${ev.gestureId}`
                : '-'}
            </Text>
            <Text style={styles.eventTime}>
              {(ev.timestamp / 1000000).toFixed(2)}s
            </Text>
          </View>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a' },
  statusBar: {
    flexDirection: 'row', flexWrap: 'wrap', alignItems: 'center',
    padding: 10, backgroundColor: '#16213e', gap: 15,
  },
  statusText: { color: '#00ff88', fontSize: 13, fontWeight: 'bold' },
  statusDetail: { color: '#aaa', fontSize: 11 },
  statusCounters: { color: '#888', fontSize: 10 },
  statusBattery: { color: '#ffcc00', fontSize: 10 },
  canvasContainer: { alignItems: 'center', padding: 10 },
  canvas: {
    width: SCREEN_W - 20, height: CANVAS_H,
    backgroundColor: '#0a0a14', borderRadius: 10, position: 'relative',
    borderWidth: 1, borderColor: '#1a1a2e',
  },
  deviceOutline: {
    position: 'absolute', top: 20, left: 20,
    width: SCREEN_W - 60, height: CANVAS_H - 40,
    borderWidth: 2, borderColor: '#333', borderRadius: 15, borderStyle: 'dashed',
  },
  touchPoint: {
    position: 'absolute', width: 24, height: 24, borderRadius: 12,
    backgroundColor: '#00ff88', alignItems: 'center', justifyContent: 'center',
    shadowColor: '#00ff88', shadowOffset: { width: 0, height: 0 },
    shadowOpacity: 0.6, shadowRadius: 8, elevation: 8,
  },
  touchLabel: { color: '#000', fontSize: 10, fontWeight: 'bold' },
  canvasLabel: { color: '#666', fontSize: 12, marginTop: 8 },
  eventList: { flex: 1, padding: 15 },
  eventListTitle: { color: '#888', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  eventRow: {
    flexDirection: 'row', alignItems: 'center', gap: 15,
    paddingVertical: 4, borderBottomWidth: 0.5, borderBottomColor: '#1a1a2e',
  },
  eventType: { color: '#00ff88', fontSize: 11, fontWeight: 'bold', width: 80 },
  eventCoords: { color: '#aaa', fontSize: 11, flex: 1 },
  eventTime: { color: '#666', fontSize: 10 },
});