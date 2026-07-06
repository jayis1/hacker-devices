// lumen-tap/app/App.js
// LumenTap companion app — navigation + device connection + status polling.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

import React, { createContext, useState, useEffect, useRef } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

import { LiveScreen }    from './screens/LiveScreen';
import { TuneScreen }    from './screens/TuneScreen';
import { RecordScreen }  from './screens/RecordScreen';
import { SafetyScreen }  from './screens/SafetyScreen';
import { Commands, parseStatus } from './utils/protocol';

// ---- Device context --------------------------------------------------
export const DeviceContext = createContext({
  status: null,
  send: () => {},
  syntheticSamples: [],
  connected: false,
  connect: () => {},
  disconnect: () => {},
});

const Tab = createBottomTabNavigator();

function AppInner() {
  const [status, setStatus]           = useState(null);
  const [connected, setConnected]     = useState(false);
  const [syntheticSamples, setSamps]  = useState([]);
  const lineBuffer = useRef('');
  // We don't have a real USB-serial hook in this sandbox; we synthesize
  // a status stream so the UI is exercised. In production, replace
  // `fakePoll` with the expo-usb-serial on-data callback.
  const fakeRef = useRef(0);

  // Synthesize a periodic status object so the UI is alive
  useEffect(() => {
    const id = setInterval(() => {
      if (!connected) return;
      fakeRef.current++;
      const t = fakeRef.current;
      const synth = {
        type: 'status',
        tick: t * 250,
        laser: (t % 20 < 10) ? 1 : 0,
        pwm: 28,
        ambient: 800 + (Math.sin(t/3) * 200),
        arm: 1,
        rms_in: 0.02 + Math.random() * 0.01,
        rms_out: 0.3 + Math.random() * 0.1,
        snr_db: 18 + Math.sin(t/5) * 4,
        demod: 1,
        bp_lo: 20, bp_hi: 16000,
        noise: 0.6,
        gain: 12.3,
        sd_state: 2,
        sd_blk: t * 4,
      };
      setStatus(synth);
      // generate synthetic waveform samples
      const arr = new Array(128);
      for (let i = 0; i < 128; i++) {
        arr[i] = Math.sin((i + t * 4) * 0.3) * 0.6 +
                 Math.sin((i + t * 2) * 0.07) * 0.3 +
                 (Math.random() - 0.5) * 0.1;
      }
      setSamps(arr);
    }, 250);
    return () => clearInterval(id);
  }, [connected]);

  const send = (line) => {
    // In production: write `line` to the USB CDC endpoint.
    // Here we just log it.
    // eslint-disable-next-line no-console
    console.log('[LumenTap → device]', line.trim());
  };

  const connect = () => { setConnected(true); send(Commands.status()); };
  const disconnect = () => { setConnected(false); };

  const ctx = { status, send, syntheticSamples, connected, connect, disconnect };

  if (!connected) {
    return (
      <View style={styles.connect}>
        <Text style={styles.title}>LumenTap</Text>
        <Text style={styles.sub}>Laser-Doppler Acoustic Eavesdropping Tool</Text>
        <Text style={styles.author}>by jayis1 — MIT License</Text>
        <Text style={styles.warn}>
          ⚖️ Authorized security research only. Pointing a laser at any
          surface you do not own or have written authorization to test is
          illegal in most jurisdictions.
        </Text>
        <TouchableOpacity style={styles.btn} onPress={connect}>
          <Text style={styles.btnTxt}>CONNECT TO DEVICE</Text>
        </TouchableOpacity>
      </View>
    );
  }

  return (
    <DeviceContext.Provider value={ctx}>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            tabBarStyle: { backgroundColor: '#111' },
            tabBarActiveTintColor: '#39FF14',
            tabBarInactiveTintColor: '#555',
            headerStyle: { backgroundColor: '#111' },
            headerTintColor: '#39FF14',
            headerTitleStyle: { fontFamily: 'monospace' },
          }}>
          <Tab.Screen name="Live"    component={LiveScreen}    options={{title:'LIVE'}} />
          <Tab.Screen name="Tune"    component={TuneScreen}    options={{title:'TUNE'}} />
          <Tab.Screen name="Record"  component={RecordScreen}  options={{title:'RECORD'}} />
          <Tab.Screen name="Safety"  component={SafetyScreen}  options={{title:'SAFETY'}} />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceContext.Provider>
  );
}

export default function App() {
  return <AppInner />;
}

const styles = StyleSheet.create({
  connect: { flex: 1, backgroundColor: '#0a0a0a', alignItems: 'center', justifyContent: 'center', padding: 24 },
  title: { color: '#39FF14', fontSize: 36, fontFamily: 'monospace', fontWeight: 'bold' },
  sub: { color: '#888', fontFamily: 'monospace', fontSize: 12, marginTop: 4 },
  author: { color: '#555', fontFamily: 'monospace', fontSize: 10, marginTop: 2 },
  warn: { color: '#999', fontFamily: 'monospace', fontSize: 11, marginTop: 24, textAlign: 'center', lineHeight: 18 },
  btn: { backgroundColor: '#39FF14', padding: 14, borderRadius: 6, marginTop: 32 },
  btnTxt: { color: '#000', fontFamily: 'monospace', fontWeight: 'bold' },
});