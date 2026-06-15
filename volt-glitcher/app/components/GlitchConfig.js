import React, { useState } from 'react';
import { View, Text, TextInput, StyleSheet } from 'react-native';

const GlitchConfig = () => {
  const [width, setWidth] = useState('50');
  const [delay, setDelay] = useState('1000');

  return (
    <View style={styles.card}>
      <Text style={styles.label}>Glitch Width (ns):</Text>
      <TextInput
        style={styles.input}
        value={width}
        onChangeText={setWidth}
        keyboardType="numeric"
      />
      <Text style={styles.label}>Delay (ns):</Text>
      <TextInput
        style={styles.input}
        value={delay}
        onChangeText={setDelay}
        keyboardType="numeric"
      />
    </View>
  );
};

const styles = StyleSheet.create({
  card: { padding: 15, backgroundColor: '#1e1e1e', borderRadius: 8, marginBottom: 20 },
  label: { color: '#bbb', marginBottom: 5 },
  input: { backgroundColor: '#333', color: '#fff', padding: 10, borderRadius: 4, marginBottom: 15 }
});

export default GlitchConfig;
