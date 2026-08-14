/**
 * screens/RtMitmConsole.js — RT substore editor + MITM match/replace rules
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, Text, TextInput, FlatList, StyleSheet, Alert } from 'react-native';
import { Card, Row, Label, Value, Button, COLORS } from '../components/Themed';
import { useCdc } from '../utils/cdc';

export default function RtMitmConsole() {
  const { send } = useCdc();
  const [rtAddr, setRtAddr] = useState('7');
  const [sa, setSa] = useState('3');
  const [data, setData] = useState('0000 1111 2222 3333');
  const [faultMask, setFaultMask] = useState('0');
  const [rules, setRules] = useState([
    { ch: '0', match: '0C07', repl: '0C07', kind: 'repl' },
  ]);

  const emulateRt = () => send(`rt set ${rtAddr}`);
  const loadSubstore = () => {
    const hex = data.split(/\s+/).join(' ');
    send(`rt tx ${sa} ${hex}`);
  };
  const setFault = () => send(`rt fault ${faultMask}`);

  const addRule = () => setRules([...rules, { ch: '0', match: '0000', repl: '0000', kind: 'repl' }]);
  const removeRule = (i) => setRules(rules.filter((_, idx) => idx !== i));
  const sendRule = (r) => {
    if (r.kind === 'repl') send(`mitm add ${r.ch} ${r.match} ${r.repl}`);
    else if (r.kind === 'drop') send(`mitm drop ${r.ch} ${r.match}`);
    else send(`mitm inject ${r.ch} 0 ${r.repl}`);
  };
  const sendAll = () => {
    send('mitm clear');
    rules.forEach(sendRule);
    Alert.alert('MITM rules pushed', `${rules.length} rules active.`);
  };

  const renderRule = ({ item, index }) => (
    <View style={styles.ruleRow}>
      <TextInput style={[styles.input, { width: 30 }]} value={item.ch}
        onChangeText={(t) => { const n = [...rules]; n[index].ch = t; setRules(n); }} />
      <TextInput style={[styles.input, { width: 60 }]} value={item.match}
        onChangeText={(t) => { const n = [...rules]; n[index].match = t; setRules(n); }} />
      <TextInput style={[styles.input, { width: 60 }]} value={item.repl}
        onChangeText={(t) => { const n = [...rules]; n[index].repl = t; setRules(n); }} />
      <Button title="→" onPress={() => sendRule(item)} color={COLORS.accent} />
      <Button title="✕" onPress={() => removeRule(index)} color={COLORS.danger} />
    </View>
  );

  return (
    <View style={styles.container}>
      <Card>
        <Label>RT emulator</Label>
        <Row><Label>RT address (0–30)</Label><Value>{rtAddr}</Value></Row>
        <TextInput style={styles.input} value={rtAddr} onChangeText={setRtAddr} keyboardType="numeric" />
        <Button title="Emulate this RT" onPress={emulateRt} color={COLORS.warn} />

        <Label>Subaddress / TX data</Label>
        <TextInput style={styles.input} value={sa} onChangeText={setSa} keyboardType="numeric" />
        <TextInput style={styles.input} value={data} onChangeText={setData}
          placeholder="hex words separated by spaces" placeholderTextColor={COLORS.muted} />
        <Button title="Load TX substore" onPress={loadSubstore} color={COLORS.accent} />

        <Label>Status-word fault mask</Label>
        <TextInput style={styles.input} value={faultMask} onChangeText={setFaultMask} />
        <Button title="Set fault mask" onPress={setFault} color={COLORS.danger} />
      </Card>

      <Card>
        <Label>MITM rules</Label>
        <Text style={styles.hint}>ch · match · replace (or drop/inject)</Text>
        <FlatList data={rules} keyExtractor={(i) => String(i)} renderItem={renderRule} />
        <View style={styles.btnRow}>
          <Button title="+ Rule" onPress={addRule} color={COLORS.accent} />
          <Button title="Clear all" onPress={() => send('mitm clear')} color={COLORS.danger} />
          <Button title="Push all" onPress={sendAll} color={COLORS.warn} />
        </View>
      </Card>
      <Text style={styles.footer}>by jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: COLORS.bg, padding: 12 },
  input: {
    backgroundColor: COLORS.bg,
    color: COLORS.text,
    borderColor: COLORS.border,
    borderWidth: 1,
    borderRadius: 4,
    padding: 6,
    marginVertical: 4,
    fontFamily: 'monospace',
  },
  btnRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6, marginVertical: 6 },
  ruleRow: { flexDirection: 'row', alignItems: 'center', gap: 4, marginVertical: 3 },
  hint: { color: COLORS.muted, fontSize: 11, marginTop: 2 },
  footer: { color: COLORS.muted, fontSize: 10, textAlign: 'center', paddingVertical: 8 },
});