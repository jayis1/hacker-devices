/**
 * screens/ScheduleEditor.js — BC minor/major frame CSV editor + templates
 *
 * Author: jayis1
 * License: MIT
 *
 * A table editor for the Bus Controller schedule. Each row is one 1553
 * message: ch,rt,tx,sa,wc,d0..d31,gap_us. Templates give the user a
 * known-good starting point (normal poll, mode-code sweep, fuzz).
 */

import React, { useState } from 'react';
import { View, Text, TextInput, FlatList, StyleSheet, Alert } from 'react-native';
import { Card, Row, Label, Value, Button, COLORS } from '../components/Themed';
import { useCdc } from '../utils/cdc';

const TEMPLATES = {
  'Normal poll': [
    '0,1,0,3,4,0,0,0,0,100',
    '0,2,0,5,2,0,0,100',
    '0,3,1,7,8,0,0,0,0,0,0,0,0,0,100',
  ],
  'Mode-code sweep': [
    '0,1,1,0,0,100',
    '0,1,1,31,0,100',
    '0,2,1,0,0,100',
  ],
  'Fuzz campaign': [
    '0,7,0,3,4,4242,0,0,0,50',
    '0,7,0,3,31,0,0,0,0,50',
    '0,7,0,0,4,0,0,0,0,50',
  ],
};

export default function ScheduleEditor() {
  const { send } = useCdc();
  const [rows, setRows] = useState([...TEMPLATES['Normal poll']]);
  const [editing, setEditing] = useState(null);   /* index being edited */

  const loadTemplate = (name) => setRows([...TEMPLATES[name]]);
  const addRow = () => setRows([...rows, '0,1,0,3,4,0,0,0,0,100']);
  const removeRow = (i) => setRows(rows.filter((_, idx) => idx !== i));

  const sendTo = () => {
    for (const r of rows) {
      const csv = r.split(',').map((x) => x.trim()).join(',');
      send(`bc load csv ${csv}`);
    }
    Alert.alert('Schedule loaded', `${rows.length} messages sent to device. Tap "Run" to start.`);
  };

  const renderItem = ({ item, index }) => (
    <View style={styles.row}>
      <TextInput
        style={styles.input}
        value={item}
        onChangeText={(t) => {
          const next = [...rows]; next[index] = t; setRows(next);
        }}
        placeholder="ch,rt,tx,sa,wc,d0..d31,gap_us"
        placeholderTextColor={COLORS.muted}
      />
      <Button title="✕" onPress={() => removeRow(index)} color={COLORS.danger} />
    </View>
  );

  return (
    <View style={styles.container}>
      <Card>
        <Label>Templates</Label>
        <View style={styles.btnRow}>
          {Object.keys(TEMPLATES).map((name) => (
            <Button key={name} title={name} onPress={() => loadTemplate(name)} />
          ))}
        </View>
      </Card>

      <Card>
        <Label>Schedule ({rows.length} messages)</Label>
        <FlatList
          data={rows}
          keyExtractor={(i) => String(i)}
          renderItem={renderItem}
          style={{ maxHeight: 300 }}
        />
        <View style={styles.btnRow}>
          <Button title="+ Add row" onPress={addRow} color={COLORS.accent} />
          <Button title="Load to device" onPress={sendTo} color={COLORS.warn} />
        </View>
      </Card>

      <Card>
        <Label>Bus Controller</Label>
        <View style={styles.btnRow}>
          <Button title="Run" onPress={() => send('bc run')} color={COLORS.ok} />
          <Button title="Stop" onPress={() => send('bc stop')} color={COLORS.danger} />
          <Button
            title="Fuzz: parity"
            onPress={() => send('bc fuzz 1')}
            color={COLORS.warn}
          />
          <Button
            title="Fuzz: bad len"
            onPress={() => send('bc fuzz 4')}
            color={COLORS.warn}
          />
          <Button
            title="Fuzz: mode code"
            onPress={() => send('bc fuzz 20')}
            color={COLORS.warn}
          />
          <Button title="Fuzz: clear" onPress={() => send('bc fuzz 0')} />
        </View>
      </Card>
      <Text style={styles.footer}>by jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: COLORS.bg, padding: 12 },
  btnRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6, marginVertical: 6 },
  row: { flexDirection: 'row', alignItems: 'center', marginVertical: 3 },
  input: {
    flex: 1,
    backgroundColor: COLORS.bg,
    color: COLORS.text,
    borderColor: COLORS.border,
    borderWidth: 1,
    borderRadius: 4,
    padding: 6,
    fontFamily: 'monospace',
    fontSize: 11,
  },
  footer: { color: COLORS.muted, fontSize: 10, textAlign: 'center', paddingVertical: 8 },
});