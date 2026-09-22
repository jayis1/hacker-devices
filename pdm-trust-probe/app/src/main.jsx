// PDM Trust Probe browser companion
// Author: jayis1
import React, { useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { armPayload, commands, encodeFrame, patternPayload } from './protocol.js';
import { ProbeTransport } from './transport.js';
import './style.css';
const initialEvents = [
  { id: 1, time: '00:00:14.220', channel: 3, type: 'Stuck-low', severity: 'critical', detail: '0/2048 one-bits' },
  { id: 2, time: '00:00:16.004', channel: 1, type: 'Duplicate stream', severity: 'warn', detail: 'Matched CH0 for 8 windows' },
  { id: 3, time: '00:00:20.117', channel: 0, type: 'Clock drift', severity: 'warn', detail: '3.42 MHz' }
];
function Meter({ label, value, suffix, state='good' }) { return <div className={`meter ${state}`}><span>{label}</span><strong>{value}{suffix}</strong><div><i style={{width:`${Math.min(100,value)}%`}} /></div></div>; }
function App() {
  const [connected, setConnected] = useState(false); const [connectionName, setConnectionName] = useState('Disconnected'); const [capture, setCapture] = useState(false); const [tab, setTab] = useState('Dashboard'); const [events, setEvents] = useState(initialEvents); const [armed, setArmed] = useState(false); const [error, setError] = useState('');
  const transport = useRef(null); const sequence = useRef(0); const softwareArmed = useRef(false);
  const channels = useMemo(() => [{n:0,d:51,t:50,s:'good'},{n:1,d:51,t:50,s:'warn'},{n:2,d:48,t:34,s:'good'},{n:3,d:0,t:0,s:'bad'}], []);
  function exportReport(){ const blob=new Blob([JSON.stringify({author:'jayis1',device:'PDM Trust Probe',events},null,2)],{type:'application/json'}); const a=document.createElement('a'); a.href=URL.createObjectURL(blob); a.download='pdm-trust-report.json'; a.click(); URL.revokeObjectURL(a.href); }
  async function connect(mock){ try { setError(''); transport.current=new ProbeTransport(mock); const info=await transport.current.connect(); sequence.current=0; softwareArmed.current=false; setConnected(true); setConnectionName(info.productName); } catch(e){ setError(e.message); setConnected(false); } }
  async function send(command,payload=new Uint8Array()){ if(!transport.current) throw new Error('Connect a device first'); sequence.current=(sequence.current+1)>>>0; if(sequence.current===0) throw new Error('Sequence exhausted; reconnect'); return transport.current.send(encodeFrame(command,sequence.current,payload)); }
  async function toggleCapture(){ try { await send(capture?commands.stop:commands.start); setCapture(!capture); } catch(e){ setError(e.message); } }
  async function runPattern(){ try { if(!softwareArmed.current){ await send(commands.arm,armPayload(0xa5c31f27)); softwareArmed.current=true; } await send(commands.pattern,patternPayload(0,1,25)); setError('25 ms alternating pattern acknowledged; hardware returned to bypass'); } catch(e){ setError(e.message); } }
  return <main><header><div><small>jayis1 / authorized lab console</small><h1>PDM Trust Probe</h1></div><div className="actions"><button onClick={()=>connect(false)} className={connected?'connected':''}>Connect WebUSB</button><button onClick={()=>connect(true)}>Use mock device</button></div></header>{error&&<aside>{error}</aside>}<p>{connectionName}</p>
   <nav>{['Dashboard','Channels','Events','Policy'].map(x=><button key={x} className={tab===x?'active':''} onClick={()=>setTab(x)}>{x}</button>)}</nav>
   {tab==='Dashboard'&&<section><div className="hero"><div><span>Mode</span><b>{capture?'Observing':'Bypass'}</b></div><div><span>PDM clock</span><b>3.072 MHz</b></div><div><span>Privacy</span><b>Statistics only</b></div><div><span>Violations</span><b>{events.length}</b></div></div><div className="actions"><button disabled={!connected} onClick={toggleCapture}>{capture?'Stop capture':'Start capture'}</button><button onClick={exportReport}>Export evidence</button></div><aside>Raw microphone samples are reduced to density and transition statistics in hardware and are not retained by the app.</aside></section>}
   {tab==='Channels'&&<section className="grid">{channels.map(c=><article key={c.n}><h2>Channel {c.n}</h2><Meter label="One density" value={c.d} suffix="%" state={c.s}/><Meter label="Transitions" value={c.t} suffix="%" state={c.s}/><p>{c.s==='bad'?'Signal stuck low':c.s==='warn'?'Possible mirrored stream':'Within learned envelope'}</p></article>)}</section>}
   {tab==='Events'&&<section><div className="actions"><button onClick={()=>setEvents([])}>Clear local view</button><button onClick={exportReport}>Export JSON</button></div><table><thead><tr><th>Time</th><th>Channel</th><th>Finding</th><th>Detail</th></tr></thead><tbody>{events.map(e=><tr key={e.id} className={e.severity}><td>{e.time}</td><td>CH{e.channel}</td><td>{e.type}</td><td>{e.detail}</td></tr>)}</tbody></table></section>}
   {tab==='Policy'&&<section className="policy"><h2>Safe intervention policy</h2><label><input type="checkbox" checked={armed} onChange={e=>setArmed(e.target.checked)}/> I have physically pressed and continue to hold ARM</label><label>Clock envelope<input defaultValue="1.0–3.5 MHz" /></label><label>Density envelope<input defaultValue="12–88%" /></label><label>Maximum synthetic test pattern<input defaultValue="250 ms" /></label><button disabled={!armed||!connected} onClick={runPattern}>Run 25 ms non-speech alternating pattern</button><p>Active output requires the hardware ARM button and expires after 60 seconds. The firmware rejects arbitrary sample uploads.</p></section>}
   <footer>Reference console by jayis1 · no cloud service · no audio playback</footer></main>;
}
createRoot(document.getElementById('root')).render(<App/>);
