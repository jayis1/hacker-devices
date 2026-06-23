/**
 * ACOUSTIC-PHANTOM — Beam Indicator Component
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Compass-style widget showing the beamforming source bearing.
 * Displays the current steering direction and allows the operator
 * to manually adjust the beam angle by dragging.
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, PanResponder, Dimensions,
} from 'react-native';

const COMPASS_SIZE = 180;

export default function BeamIndicator({ bearing = 0, onSteer }) {
  const [angle, setAngle] = useState(bearing);

  // Pan responder for manual beam steering
  const panResponder = PanResponder.create({
    onStartShouldSetPanResponder: () => true,
    onMoveShouldSetPanResponder: () => true,
    onPanResponderMove: (_, gestureState) => {
      // Calculate angle from center of compass
      const center = COMPASS_SIZE / 2;
      // Use move distance to adjust angle
      const newAngle = Math.max(-60, Math.min(60, angle + gestureState.dx * 0.3));
      setAngle(newAngle);
    },
    onPanResponderRelease: () => {
      if (onSteer) {
        onSteer(Math.round(angle));
      }
    },
  });

  // The beam line extends from center to the edge at the given angle
  const angleRad = (angle * Math.PI) / 180;
  const lineLength = COMPASS_SIZE / 2 - 10;
  const endX = COMPASS_SIZE / 2 + Math.sin(angleRad) * lineLength;
  const endY = COMPASS_SIZE / 2 - Math.cos(angleRad) * lineLength;

  return (
    <View style={styles.container}>
      <View
        style={styles.compass}
        {...panResponder.panHandlers}
      >
        {/* Compass circle */}
        <View style={styles.circle} />

        {/* Direction markers */}
        <Text style={styles.markerN}>0°</Text>
        <Text style={styles.markerL}>-45°</Text>
        <Text style={styles.markerR}>+45°</Text>

        {/* Beam line (simulated with View) */}
        <View
          style={[
            styles.beamLine,
            {
              width: lineLength,
              transform: [
                { rotate: `${angle}deg` },
                { translateY: -lineLength / 2 },
              ],
            },
          ]}
        />

        {/* Center dot */}
        <View style={styles.centerDot} />
      </View>

      <Text style={styles.bearingText}>
        Bearing: {Math.round(angle)}°
      </Text>
      <Text style={styles.hintText}>
        Drag to steer beam (-60° to +60°)
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: 'center', padding: 16 },
  compass: {
    width: COMPASS_SIZE,
    height: COMPASS_SIZE,
    justifyContent: 'center',
    alignItems: 'center',
    position: 'relative',
  },
  circle: {
    position: 'absolute',
    width: COMPASS_SIZE,
    height: COMPASS_SIZE,
    borderRadius: COMPASS_SIZE / 2,
    borderWidth: 2,
    borderColor: '#333',
    backgroundColor: 'rgba(0, 0, 0, 0.3)',
  },
  markerN: {
    position: 'absolute',
    top: 2,
    color: '#888',
    fontSize: 10,
  },
  markerL: {
    position: 'absolute',
    left: 2,
    top: COMPASS_SIZE / 2 - 6,
    color: '#666',
    fontSize: 9,
  },
  markerR: {
    position: 'absolute',
    right: 2,
    top: COMPASS_SIZE / 2 - 6,
    color: '#666',
    fontSize: 9,
  },
  beamLine: {
    position: 'absolute',
    top: COMPASS_SIZE / 2,
    left: COMPASS_SIZE / 2,
    height: 3,
    backgroundColor: '#00AAFF',
    transformOrigin: 'left center',
    borderRadius: 2,
    shadowColor: '#00AAFF',
    shadowOffset: { width: 0, height: 0 },
    shadowOpacity: 0.6,
    shadowRadius: 4,
  },
  centerDot: {
    position: 'absolute',
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: '#FFF',
    top: COMPASS_SIZE / 2 - 4,
    left: COMPASS_SIZE / 2 - 4,
  },
  bearingText: {
    color: '#CCC',
    fontSize: 18,
    fontWeight: 'bold',
    marginTop: 12,
    fontFamily: 'monospace',
  },
  hintText: {
    color: '#555',
    fontSize: 12,
    marginTop: 4,
  },
});