// MessageLog.js — scrolling log of device messages for AxleTap
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React, { useEffect, useRef } from 'react';
import { View, Text, ScrollView, StyleSheet } from 'react-native';

export default function MessageLog({ messages }) {
  const scrollRef = useRef(null);
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollToEnd({ animated: true });
    }
  }, [messages]);

  return (
    <View style={styles.container}>
      <ScrollView ref={scrollRef} style={styles.scroll}>
        {messages.map((m, i) => (
          <Text key={i} style={[styles.line, m.level === 'err' ? styles.err : styles.info]}>
            [{m.time}] {m.text}
          </Text>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d0d0d',
    borderTopWidth: 1,
    borderTopColor: '#333',
    padding: 8,
  },
  scroll: { flex: 1 },
  line: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#ccc',
    marginBottom: 2,
  },
  info: { color: '#8fa9' },
  err:  { color: '#ff6677' },
});