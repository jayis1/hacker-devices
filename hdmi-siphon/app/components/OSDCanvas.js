/**
 * OSDCanvas.js — OSD overlay drag/drop editor canvas component
 * Author: jayis1
 */

import React, {useState, useRef} from 'react';
import {View, Text, StyleSheet, PanResponder} from 'react-native';

const OSDCanvas = ({text = 'OSD Text', color = '#FF0000', fontSize = 18, onPositionChange}) => {
  const [position, setPosition] = useState({x: 20, y: 20});
  const posRef = useRef(position);

  const panResponder = useRef(
    PanResponder.create({
      onStartShouldSetPanResponder: () => true,
      onMoveShouldSetPanResponder: () => true,
      onPanResponderMove: (_, gesture) => {
        const newPos = {
          x: Math.max(0, posRef.current.x + gesture.dx),
          y: Math.max(0, posRef.current.y + gesture.dy),
        };
        setPosition(newPos);
      },
      onPanResponderRelease: (_, gesture) => {
        posRef.current = {
          x: Math.max(0, posRef.current.x + gesture.dx),
          y: Math.max(0, posRef.current.y + gesture.dy),
        };
        if (onPositionChange) {
          onPositionChange(posRef.current);
        }
      },
    }),
  ).current;

  return (
    <View style={styles.canvas}>
      <View
        style={[
          styles.overlayText,
          {left: position.x, top: position.y},
        ]}
        {...panResponder.panHandlers}>
        <Text style={[styles.text, {color, fontSize}]}>{text}</Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  canvas: {
    flex: 1,
    backgroundColor: '#000',
    borderRadius: 8,
    overflow: 'hidden',
    minHeight: 200,
    position: 'relative',
  },
  overlayText: {
    position: 'absolute',
    padding: 4,
  },
  text: {
    fontWeight: 'bold',
    textShadowColor: 'rgba(0,0,0,0.8)',
    textShadowOffset: {width: 1, height: 1},
    textShadowRadius: 2,
  },
});

export default OSDCanvas;
