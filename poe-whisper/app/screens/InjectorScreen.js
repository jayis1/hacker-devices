// InjectorScreen.js - PoE Whisper active control screen
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import ToggleCard from '../components/ToggleCard';

export default function InjectorScreen({ toggles, onToggle }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Power Manipulation Controls</Text>
      <ToggleCard
        title="LLDP Identity Override"
        description="Advertise alternate power requests and endpoint identity for authorized validation."
        active={toggles.lldpSpoof}
        onPress={() => onToggle('lldpSpoof')}
      />
      <ToggleCard
        title="Maintain-Power Jitter"
        description="Introduce bounded MPS irregularity to evaluate PSE keepalive behavior."
        active={toggles.mpsJitter}
        onPress={() => onToggle('mpsJitter')}
      />
      <ToggleCard
        title="Brownout Window"
        description="Arm a short voltage sag during the selected boot phase with hardware rollback."
        active={toggles.brownout}
        onPress={() => onToggle('brownout')}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 12 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
});
