/**
 * screens/Dashboard.js — role selector, channel activity, armed state, battery
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Card, Row, Label, Value, Button, Bar, COLORS } from '../components/Themed';
import { useCdc } from '../utils/cdc';

const ROLES = ['BM', 'RT', 'BC', 'MITM'];

export default function Dashboard() {
  const { connected, mockMode, connect, send, onLine } = useCdc();
  const [role, setRole] = useState('BM');
  const [armed, setArmed] = useState(false);
  const [injecting, setInjecting] = useState(false);
  const [cap, setCap] = useState(0);
  const [chAerr, setChAerr] = useState(0);
  const [chBerr, setChBerr] = useState(0);
  const [actA, setActA] = useState(0);
  const [actB, setActB] = useState(0);
  const [batt, setBatt] = useState(85);

  useEffect(() => {
    if (!connected) connect();
  }, [connected, connect]);

  useEffect(() => {
    const off = onLine((line) => {
      const m = line.match(/role=(\w+)\s+armed=(\d+)\s+inj=(\d+)\s+cap=(\d+)\s+chA_err=(\d+)\s+chB_err=(\d+)/);
      if (m) {
        setRole(m[1]);
        setArmed(m[2] === '1');
        setInjecting(m[3] === '1');
        setCap(parseInt(m[4], 10));
        setChAerr(parseInt(m[5], 10));
        setChBerr(parseInt(m[6], 10));
      }
      if (line.match(/^\d+,0,/)) setActA((a) => Math.min(100, a + 5));
      if (line.match(/^\d+,1,/)) setActB((b) => Math.min(100, b + 5));
    });
    const timer = setInterval(() => {
      setActA((a) => Math.max(0, a - 2));
      setActB((b) => Math.max(0, b - 2));
    }, 250);
    return () => { off(); clearInterval(timer); };
  }, [onLine]);

  const refresh = () => send('status');
  const pickRole = (r) => {
    if (r !== 'BM' && !armed) {
      send('arm');      /* request arm; user must long-press button too */
    }
    send(`role ${r.toLowerCase()}`);
  };
  const arm = () => send('arm');
  const disarm = () => send('disarm');

  return (
    <View style={styles.container}>
      <Card>
        <Row>
          <Label>Connection</Label>
          <Value color={connected ? COLORS.ok : COLORS.danger}>
            {connected ? (mockMode ? 'MOCK' : 'USB CDC') : 'Disconnected'}
          </Value>
        </Row>
        <Row>
          <Label>Role</Label>
          <Value color={armed ? COLORS.warn : COLORS.text}>{role}</Value>
        </Row>
        <Row>
          <Label>Armed</Label>
          <Value color={armed ? COLORS.danger : COLORS.muted}>
            {armed ? (injecting ? 'INJECTING' : 'ARMED') : 'safe'}
          </Value>
        </Row>
        <Row>
          <Label>Battery</Label>
          <Value color={batt > 20 ? COLORS.ok : COLORS.danger}>{batt}%</Value>
        </Row>
        <Button title="Refresh status" onPress={refresh} color={COLORS.accent} />
      </Card>

      <Card>
        <Label>Role</Label>
        <View style={styles.roleRow}>
          {ROLES.map((r) => (
            <Button
              key={r}
              title={r}
              onPress={() => pickRole(r)}
              color={role === r ? COLORS.accent : COLORS.muted}
            />
          ))}
        </View>
        {!armed && role !== 'BM' ? (
          <Text style={styles.warn}>
            Switching to a TX-capable role requires arm. Tap "Arm" then long-press the device button.
          </Text>
        ) : null}
        <View style={styles.roleRow}>
          <Button title="Arm" onPress={arm} color={COLORS.warn} />
          <Button title="Disarm" onPress={disarm} color={COLORS.danger} />
        </View>
      </Card>

      <Card>
        <Label>Channel activity</Label>
        <Bar label="Channel A (primary)" pct={actA} color={COLORS.ok} />
        <Bar label="Channel B (redundant)" pct={actB} color={COLORS.accent} />
        <Row>
          <Label>Ch A errors</Label>
          <Value color={chAerr ? COLORS.danger : COLORS.text}>{chAerr}</Value>
        </Row>
        <Row>
          <Label>Ch B errors</Label>
          <Value color={chBerr ? COLORS.danger : COLORS.text}>{chBerr}</Value>
        </Row>
        <Row>
          <Label>Captured words</Label>
          <Value>{cap}</Value>
        </Row>
      </Card>

      <Text style={styles.footer}>1553-Phantom · by jayis1 · CERN-OHL-S v2 · authorized use only</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: COLORS.bg, padding: 12 },
  roleRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6, marginVertical: 6 },
  warn: { color: COLORS.warn, fontSize: 12, marginTop: 6 },
  footer: { color: COLORS.muted, fontSize: 11, textAlign: 'center', marginTop: 12, marginBottom: 24 },
});