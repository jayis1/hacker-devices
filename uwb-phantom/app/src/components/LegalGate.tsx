/**
 * LegalGate.tsx — mandatory authorised-use acknowledgement.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The app refuses to do anything until the operator ticks the box and
 * presses "I Agree".  The choice is persisted in AsyncStorage so it is
 * only asked once per install.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Button, Checkbox } from 'react-native-paper';
import AsyncStorage from '@react-native-async-storage/async-storage';

const KEY = 'uwb-phantom-legal-ack';

export default function LegalGate({ children }: { children: React.ReactNode }) {
  const [agreed, setAgreed] = useState(false);
  const [checked, setChecked] = useState(false);

  useEffect(() => {
    AsyncStorage.getItem(KEY).then(v => { if (v === 'yes') setAgreed(true); });
  }, []);

  if (agreed) return <>{children}</>;

  return (
    <View style={styles.root}>
      <Text style={styles.title}>UWB-PHANTOM — Legal Notice</Text>
      <Text style={styles.body}>
        This tool is for authorised security research, penetration testing
        with explicit written consent, and red-team operations on systems
        you own or have explicit permission to assess.{'\n\n'}
        Interception, relay, replay, or manipulation of UWB ranging
        exchanges on devices you do not own may violate computer-fraud
        and abuse statutes (18 U.S.C. § 1030 CFAA, ECPA), radio regulations
        (FCC Part 15 subpart F, ETSI EN 302 065), and motor-vehicle theft
        law.{'\n\n'}
        Author: jayis1. License: GPL-3.0-or-later.{'\n\n'}
        The author assumes no liability for misuse.
      </Text>
      <View style={styles.row}>
        <Checkbox status={checked ? 'checked' : 'unchecked'}
          onPress={() => setChecked(!checked)} />
        <Text style={styles.checkboxLabel}>
          I have read this notice and I am authorised to test the targets
          I will interact with.
        </Text>
      </View>
      <Button mode="contained" disabled={!checked}
        onPress={() => { AsyncStorage.setItem(KEY, 'yes'); setAgreed(true); }}>
        I Agree
      </Button>
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 24, justifyContent: 'center', backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 20, fontWeight: 'bold', marginBottom: 16 },
  body:    { color: '#cfd8dc', fontSize: 13, marginBottom: 24, lineHeight: 20 },
  row:     { flexDirection: 'row', alignItems: 'center', marginBottom: 24 },
  checkboxLabel: { color: '#cfd8dc', fontSize: 12, flex: 1, flexWrap: 'wrap' },
  footer:  { color: '#607d8b', fontSize: 10, marginTop: 24, textAlign: 'center' },
});