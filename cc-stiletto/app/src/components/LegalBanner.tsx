// src/components/LegalBanner.tsx — one-time legal/ethics acknowledgment.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React, { useState } from 'react';
import { View, Text, Modal, Pressable, StyleSheet, Linking } from 'react-native';

const KEY = 'cc_stiletto_legal_ack_v1';

export function LegalBanner() {
  const [visible, setVisible] = useState(() => {
    // In a real app this would read AsyncStorage; we default to showing it.
    return true;
  });

  return (
    <Modal visible={visible} transparent animationType="fade" onRequestClose={() => {}}>
      <View style={styles.overlay}>
        <View style={styles.card}>
          <Text style={styles.title}>CC-Stiletto — Legal &amp; Ethical Notice</Text>
          <Text style={styles.body}>
            CC-Stiletto is a professional security research instrument for
            authorized penetration testing of hardware you own or have explicit
            written permission to test. USB-C Power Delivery can negotiate up
            to 48 V / 5 A; deliberate mis-negotiation can cause thermal damage,
            fire, or injury. Misuse may violate computer-fraud and product-safety
            laws.
          </Text>
          <Text style={styles.body}>
            The author (jayis1) disclaims all liability. Use only with
            appropriate electrical safety precautions.
          </Text>
          <Pressable onPress={() => Linking.openURL('https://hermes-agent.nousresearch.com')}>
            <Text style={styles.link}>Full disclaimer in README.md</Text>
          </Pressable>
          <Pressable style={styles.btn} onPress={() => setVisible(false)}>
            <Text style={styles.btnText}>I understand — continue</Text>
          </Pressable>
        </View>
      </View>
    </Modal>
  );
}

const styles = StyleSheet.create({
  overlay: { flex: 1, backgroundColor: 'rgba(0,0,0,0.7)', justifyContent: 'center', padding: 24 },
  card:    { backgroundColor: '#1b1b1b', borderRadius: 10, padding: 20, borderColor: '#d62828', borderWidth: 2 },
  title:   { color: '#d62828', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  body:    { color: '#ddd', fontSize: 13, marginBottom: 10, lineHeight: 18 },
  link:    { color: '#4cc9f0', fontSize: 12, marginBottom: 16, textDecorationLine: 'underline' },
  btn:     { backgroundColor: '#d62828', borderRadius: 6, padding: 12, alignItems: 'center' },
  btnText: { color: 'white', fontWeight: 'bold' },
});