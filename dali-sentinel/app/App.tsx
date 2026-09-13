// DALI Sentinel companion application
// Author: jayis1
// Copyright (c) 2026 jayis1
import React, {useMemo, useReducer, useState} from 'react';
import {SafeAreaView, ScrollView, StyleSheet, Text, TouchableOpacity, View} from 'react-native';

type Side = 'CTRL' | 'GEAR' | 'UNKNOWN';
type Screen = 'Live' | 'Findings' | 'Topology' | 'Lab' | 'Export';
type Severity = 'info' | 'low' | 'medium' | 'high' | 'critical';
type Frame = {id: number; timestampMs: number; side: Side; bits: number; raw: number; quality: number; collision: number; valid: boolean};
type Finding = {id: number; timestampMs: number; severity: Severity; code: string; evidenceRaw: number; explanation: string};
type Gear = {address: number; responses: number; lastSeenMs: number; confidence: number};
type Status = {connected: boolean; mode: 'OBSERVE'|'INLINE'|'LAB_ISOLATE'|'FAULT'; bypassClosed: boolean; armed: boolean; busMv: [number, number]; temperatureC: number; captureLoss: number};
type State = {status: Status; frames: Frame[]; findings: Finding[]; gear: Gear[]};
type Action = {type: 'frame'; frame: Frame} | {type: 'finding'; finding: Finding} | {type: 'status'; status: Partial<Status>} | {type: 'clear'};

export interface Transport {
  open(): Promise<void>;
  close(): Promise<void>;
  write(packet: Uint8Array): Promise<void>;
  subscribe(listener: (packet: Uint8Array) => void): () => void;
}

const initial: State = {
  status: {connected: true, mode: 'OBSERVE', bypassClosed: true, armed: false, busMv: [16120, 15980], temperatureC: 29, captureLoss: 0},
  frames: [
    {id: 1, timestampMs: 120, side: 'CTRL', bits: 16, raw: 0x0190, quality: 96, collision: 0, valid: true},
    {id: 2, timestampMs: 136, side: 'GEAR', bits: 8, raw: 0xff, quality: 91, collision: 0, valid: true},
    {id: 3, timestampMs: 940, side: 'CTRL', bits: 16, raw: 0xff05, quality: 94, collision: 0, valid: true},
  ],
  findings: [{id: 1, timestampMs: 940, severity: 'medium', code: 'BROADCAST_MAX', evidenceRaw: 0xff05, explanation: 'Broadcast RECALL MAX LEVEL reached the observed field bus.'}],
  gear: [{address: 0, responses: 4, lastSeenMs: 136, confidence: 82}],
};

function reducer(state: State, action: Action): State {
  if (action.type === 'status') return {...state, status: {...state.status, ...action.status}};
  if (action.type === 'clear') return {...state, frames: [], findings: []};
  if (action.type === 'finding') return {...state, findings: [action.finding, ...state.findings].slice(0, 500)};
  const frames = [action.frame, ...state.frames].slice(0, 2000);
  if (action.frame.bits !== 8 || action.frame.side !== 'GEAR') return {...state, frames};
  const query = state.frames.find(frame => frame.bits === 16 && frame.timestampMs <= action.frame.timestampMs);
  if (!query) return {...state, frames};
  const address = (query.raw >> 9) & 0x3f;
  const existing = state.gear.find(item => item.address === address);
  const gear = existing
    ? state.gear.map(item => item.address === address ? {...item, responses: item.responses + 1, lastSeenMs: action.frame.timestampMs, confidence: Math.min(99, item.confidence + 2)} : item)
    : [...state.gear, {address, responses: 1, lastSeenMs: action.frame.timestampMs, confidence: 45}];
  return {...state, frames, gear};
}

export function crc32c(data: Uint8Array): number {
  let crc = 0xffffffff;
  for (const byte of data) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit++) crc = (crc >>> 1) ^ ((crc & 1) ? 0x82f63b78 : 0);
  }
  return (~crc) >>> 0;
}

export function decodeEnvelope(packet: Uint8Array): Action | null {
  if (packet.length < 12) return null;
  const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
  const version = packet[0];
  const type = packet[1];
  const payloadLength = view.getUint16(4, true);
  if (version !== 1 || payloadLength + 10 !== packet.length) return null;
  const expected = view.getUint32(packet.length - 4, true);
  if (crc32c(packet.slice(0, -4)) !== expected) return null;
  if (type === 1 && payloadLength >= 13) {
    return {type: 'frame', frame: {
      id: view.getUint16(2, true), timestampMs: view.getUint32(6, true),
      side: packet[10] === 0 ? 'CTRL' : packet[10] === 1 ? 'GEAR' : 'UNKNOWN',
      bits: packet[11], raw: view.getUint32(12, true), quality: packet[16],
      collision: packet[17], valid: packet[18] !== 0,
    }};
  }
  return null;
}

function Pill({label, tone = '#1b2430'}: {label: string; tone?: string}) {
  return <View style={[styles.pill, {backgroundColor: tone}]}><Text style={styles.pillText}>{label}</Text></View>;
}

function Header({status}: {status: Status}) {
  return <View style={styles.header}>
    <View><Text style={styles.title}>DALI Sentinel</Text><Text style={styles.byline}>by jayis1 · authorized use only</Text></View>
    <Pill label={status.connected ? status.mode : 'OFFLINE'} tone={status.mode === 'FAULT' ? '#7f1d1d' : '#164e63'} />
  </View>;
}

function Metric({label, value}: {label: string; value: string}) {
  return <View style={styles.metric}><Text style={styles.metricValue}>{value}</Text><Text style={styles.muted}>{label}</Text></View>;
}

function LiveScreen({state}: {state: State}) {
  const valid = state.frames.filter(frame => frame.valid).length;
  const collisions = state.frames.filter(frame => frame.collision >= 3).length;
  return <ScrollView>
    <View style={styles.metrics}>
      <Metric label="frames" value={String(state.frames.length)} />
      <Metric label="valid" value={String(valid)} />
      <Metric label="collisions" value={String(collisions)} />
      <Metric label="loss" value={String(state.status.captureLoss)} />
    </View>
    <View style={styles.card}><Text style={styles.cardTitle}>Electrical state</Text>
      <Text style={styles.body}>CTRL {(state.status.busMv[0] / 1000).toFixed(2)} V · GEAR {(state.status.busMv[1] / 1000).toFixed(2)} V</Text>
      <Text style={styles.body}>Bypass {state.status.bypassClosed ? 'closed (fail-safe)' : 'open'} · {state.status.temperatureC} °C</Text>
    </View>
    <Text style={styles.section}>Timeline</Text>
    {state.frames.map(frame => <View key={frame.id} style={styles.row}>
      <Pill label={frame.side} tone={frame.side === 'CTRL' ? '#1d4ed8' : '#047857'} />
      <View style={styles.flex}><Text style={styles.mono}>0x{frame.raw.toString(16).padStart(frame.bits / 4, '0').toUpperCase()}</Text><Text style={styles.muted}>{frame.timestampMs} ms · {frame.bits} bit · Q{frame.quality}</Text></View>
      {!frame.valid && <Pill label="BAD" tone="#991b1b" />}
    </View>)}
  </ScrollView>;
}

const severityColor: Record<Severity, string> = {info:'#334155', low:'#365314', medium:'#854d0e', high:'#9a3412', critical:'#991b1b'};
function FindingsScreen({findings}: {findings: Finding[]}) {
  return <ScrollView><Text style={styles.section}>Explainable findings</Text>
    {findings.length === 0 && <Text style={styles.empty}>No findings in this capture.</Text>}
    {findings.map(item => <View key={item.id} style={styles.card}>
      <View style={styles.rowHead}><Text style={styles.cardTitle}>{item.code}</Text><Pill label={item.severity.toUpperCase()} tone={severityColor[item.severity]} /></View>
      <Text style={styles.body}>{item.explanation}</Text>
      <Text style={styles.monoSmall}>evidence 0x{item.evidenceRaw.toString(16).toUpperCase()} · {item.timestampMs} ms</Text>
    </View>)}
  </ScrollView>;
}

function TopologyScreen({gear}: {gear: Gear[]}) {
  return <ScrollView><Text style={styles.section}>Observed topology</Text><Text style={styles.notice}>Passive inventory is incomplete by design. Confidence reflects correlated query/reply observations.</Text>
    {gear.map(item => <View key={item.address} style={styles.row}>
      <View style={styles.address}><Text style={styles.addressText}>{item.address}</Text></View>
      <View style={styles.flex}><Text style={styles.cardTitle}>Short address {item.address}</Text><Text style={styles.muted}>{item.responses} responses · last {item.lastSeenMs} ms</Text></View>
      <Pill label={`${item.confidence}%`} tone="#334155" />
    </View>)}
  </ScrollView>;
}

function LabScreen({status}: {status: Status}) {
  const permitted = status.mode === 'LAB_ISOLATE' && status.armed && !status.bypassClosed;
  return <ScrollView><Text style={styles.section}>Guarded lab controls</Text>
    <View style={[styles.card, {borderColor: permitted ? '#166534' : '#9a3412'}]}>
      <Text style={styles.cardTitle}>{permitted ? 'Locally authorized' : 'Outputs locked'}</Text>
      <Text style={styles.body}>{permitted ? 'The physical arm control, jumper, isolation, and expiry are valid.' : 'Fit the authorization jumper and press ARM on the device in an isolated laboratory setup.'}</Text>
    </View>
    {['Endpoint: GEAR','Frame: QUERY STATUS (address 0)','Repetitions: 1','Expiry: 30 seconds','Broadcast high-impact: denied'].map(value => <View key={value} style={styles.option}><Text style={styles.body}>{value}</Text></View>)}
    <TouchableOpacity disabled={!permitted} style={[styles.button, !permitted && styles.disabled]}><Text style={styles.buttonText}>Queue bounded test</Text></TouchableOpacity>
    <Text style={styles.notice}>Opening this screen never changes device mode. The firmware independently enforces current, temperature, bypass feedback, rate, and ten-minute session limits.</Text>
  </ScrollView>;
}

function ExportScreen({state, clear}: {state: State; clear: () => void}) {
  const [redact, setRedact] = useState(true);
  const digest = useMemo(() => crc32c(new TextEncoder().encode(JSON.stringify({frames: state.frames, findings: state.findings}))).toString(16).padStart(8, '0'), [state.frames, state.findings]);
  return <ScrollView><Text style={styles.section}>Evidence export</Text>
    <View style={styles.card}><Text style={styles.cardTitle}>Capture summary</Text><Text style={styles.body}>{state.frames.length} frames · {state.findings.length} findings</Text><Text style={styles.monoSmall}>CRC-32C preview {digest}</Text></View>
    <TouchableOpacity style={styles.option} onPress={() => setRedact(!redact)}><Text style={styles.body}>Operator-label redaction: {redact ? 'ON' : 'OFF'}</Text></TouchableOpacity>
    <TouchableOpacity style={styles.button}><Text style={styles.buttonText}>Export JSON + manifest</Text></TouchableOpacity>
    <TouchableOpacity style={styles.dangerButton} onPress={clear}><Text style={styles.buttonText}>Clear local timeline</Text></TouchableOpacity>
    <Text style={styles.notice}>Exports stay on this device unless the operator explicitly shares them. Protect captures as building-security data.</Text>
  </ScrollView>;
}

export default function App() {
  const [state, dispatch] = useReducer(reducer, initial);
  const [screen, setScreen] = useState<Screen>('Live');
  return <SafeAreaView style={styles.safe}><Header status={state.status} /><View style={styles.content}>
    {screen === 'Live' && <LiveScreen state={state} />}
    {screen === 'Findings' && <FindingsScreen findings={state.findings} />}
    {screen === 'Topology' && <TopologyScreen gear={state.gear} />}
    {screen === 'Lab' && <LabScreen status={state.status} />}
    {screen === 'Export' && <ExportScreen state={state} clear={() => dispatch({type:'clear'})} />}
  </View><View style={styles.tabs}>{(['Live','Findings','Topology','Lab','Export'] as Screen[]).map(tab => <TouchableOpacity key={tab} onPress={() => setScreen(tab)} style={[styles.tab, screen === tab && styles.activeTab]}><Text style={styles.tabText}>{tab}</Text></TouchableOpacity>)}</View></SafeAreaView>;
}

const styles = StyleSheet.create({
  safe:{flex:1,backgroundColor:'#07111d'}, header:{padding:16,flexDirection:'row',justifyContent:'space-between',alignItems:'center',borderBottomWidth:1,borderColor:'#243447'}, title:{color:'#f8fafc',fontSize:24,fontWeight:'700'}, byline:{color:'#94a3b8',fontSize:11}, content:{flex:1,padding:14}, tabs:{flexDirection:'row',borderTopWidth:1,borderColor:'#243447'}, tab:{flex:1,paddingVertical:14,alignItems:'center'}, activeTab:{backgroundColor:'#123047'}, tabText:{color:'#dbeafe',fontSize:11}, section:{color:'#e2e8f0',fontWeight:'700',fontSize:18,marginBottom:10}, card:{backgroundColor:'#111c2b',borderWidth:1,borderColor:'#26374b',borderRadius:10,padding:14,marginBottom:10}, cardTitle:{color:'#f1f5f9',fontWeight:'700'}, body:{color:'#cbd5e1',lineHeight:21}, muted:{color:'#94a3b8',fontSize:12}, mono:{color:'#a7f3d0',fontFamily:'monospace',fontSize:16}, monoSmall:{color:'#a7f3d0',fontFamily:'monospace',fontSize:12,marginTop:8}, row:{flexDirection:'row',alignItems:'center',gap:10,backgroundColor:'#0d1826',padding:11,borderBottomWidth:1,borderColor:'#1e293b'}, rowHead:{flexDirection:'row',justifyContent:'space-between',alignItems:'center',marginBottom:8}, flex:{flex:1}, pill:{borderRadius:12,paddingHorizontal:9,paddingVertical:4}, pillText:{color:'#f8fafc',fontSize:10,fontWeight:'700'}, metrics:{flexDirection:'row',gap:7,marginBottom:12}, metric:{flex:1,backgroundColor:'#111c2b',padding:10,borderRadius:8}, metricValue:{color:'#67e8f9',fontSize:19,fontWeight:'700'}, notice:{color:'#fbbf24',backgroundColor:'#2b2110',padding:12,borderRadius:8,lineHeight:19,marginBottom:10}, empty:{color:'#94a3b8',padding:20,textAlign:'center'}, address:{width:42,height:42,borderRadius:21,backgroundColor:'#164e63',alignItems:'center',justifyContent:'center'}, addressText:{color:'white',fontWeight:'700'}, option:{backgroundColor:'#111c2b',padding:15,borderBottomWidth:1,borderColor:'#26374b'}, button:{backgroundColor:'#0369a1',padding:15,borderRadius:8,alignItems:'center',marginTop:14,marginBottom:10}, disabled:{opacity:0.35}, dangerButton:{backgroundColor:'#9f1239',padding:15,borderRadius:8,alignItems:'center',marginBottom:10}, buttonText:{color:'white',fontWeight:'700'},
});
