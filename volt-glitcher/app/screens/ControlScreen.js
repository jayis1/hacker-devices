import React, { useState } from 'react';
import { View, Text, TextInput, Button, StyleSheet } from 'react-native';
import GlitchConfig from '../components/GlitchConfig';

const ControlScreen = ({ navigation }) => {
  const [status, setStatus] = useState('Disconnected');

  return (
    <View style={styles.container}>
      <Text style={styles.status}>Status: {status}</Text>
      <GlitchConfig />
      <Button title="Arm Trigger" onPress={() => setStatus('Armed')} />
      <Button title="View Trace" onPress={() => navigation.navigate('Analysis')} />
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#121212' },
  status: { color: '#00FF00', fontSize: 18, marginBottom: 20 },
});

export default ControlScreen;
