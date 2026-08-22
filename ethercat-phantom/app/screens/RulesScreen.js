// Rules screen for EtherCAT Phantom
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import RuleEditor from '../components/RuleEditor';

export default function RulesScreen({ rules }) {
  return (
    <View>
      <Text style={styles.sectionTitle}>Ruleset Composer</Text>
      <Text style={styles.copy}>
        Each rule is scoped to a specific slave and object so authorized testing remains bounded and reviewable.
      </Text>
      {rules.map((rule) => <RuleEditor key={rule.id} rule={rule} />)}
    </View>
  );
}

const styles = StyleSheet.create({
  sectionTitle: {
    color: '#eaf3fe',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  copy: {
    color: '#94a9c3',
    marginBottom: 14,
    lineHeight: 21,
  },
});
