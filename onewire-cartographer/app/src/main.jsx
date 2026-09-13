// OneWire Cartographer interactive console. Author: jayis1. SPDX-License-Identifier: MIT
import React, { useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { CartographerTransport } from './transport.js';
import { COMMAND, expectedResponse, parseEvent } from './protocol.js';
import './style.css';

const names = ['Reset', 'Presence', 'Bit', 'Byte', 'ROM', 'Timing anomaly', 'Voltage anomaly', 'Contention', 'Policy denial'];
const demo = [
  { type: 0, port: 0, timestamp: 1200, duration: 481, millivolts: 3291, value: 0, flags: 0 },
  { type: 1, port: 1, timestamp: 1745, duration: 83, millivolts: 3278, value: 0, flags: 0 },
  { type: 3, port: 0, timestamp: 2500, duration: 0, millivolts: 3302, value: 0x33, flags: 0 },
];

function Metric({ label, value }) { return <div className="metric"><small>{label}</small><strong>{value}</strong></div>; }
function App() {
  const [tab, setTab] = useState('Live');
  const [connected, setConnected] = useState(false);
  const [capturing, setCapturing] = useState(false);
  const [events, setEvents] = useState(demo);
  const [notice, setNotice] = useState('Demo data loaded; connect hardware for live capture.');
  const [armed, setArmed] = useState(false);
  const [roms] = useState([
    { rom: '01-2A-00-00-00-00-00-7D', family: 'DS1990A token', count: 41, jitter: 3, crc: true },
    { rom: '28-91-A4-6B-12-00-00-52', family: 'DS18B20 sensor', count: 14, jitter: 7, crc: true },
  ]);
  const transport = useRef(null);
  const anomalies = useMemo(() => events.filter(event => event.type >= 5).length, [events]);

  const connect = async () => {
    try {
      transport.current = new CartographerTransport(bytes => setEvents(old => [...old.slice(-499), parseEvent(bytes)]));
      await transport.current.connect(); setConnected(true); setNotice('USB device connected. Passive monitor is fail-open.');
    } catch (error) { setNotice(error.message); }
  };
  const toggleCapture = async () => {
    try { await transport.current?.send(capturing ? COMMAND.STOP : COMMAND.START); setCapturing(!capturing); }
    catch (error) { setNotice(error.message); }
  };
  const armLab = async () => {
    if (!connected) return setNotice('Connect hardware and move its physical ARM switch first.');
    const challenge = crypto.getRandomValues(new Uint32Array(1))[0];
    const payload = new Uint8Array(8); const view = new DataView(payload.buffer);
    view.setUint32(0, challenge, true); view.setUint32(4, expectedResponse(challenge), true);
    try { await transport.current.send(COMMAND.ARM, payload); setArmed(true); setNotice('Lab session requested; verify the hardware ARM indicator.'); }
    catch (error) { setNotice(error.message); }
  };

  return <main>
    <header><div><p className="eyebrow">JAYIS1 LAB INSTRUMENTS</p><h1>OneWire Cartographer</h1></div><button onClick={connected ? () => transport.current.disconnect().then(() => setConnected(false)) : connect}>{connected ? 'Disconnect' : 'Connect USB'}</button></header>
    <nav>{['Live', 'Topology', 'Lab', 'Export'].map(name => <button className={tab === name ? 'active' : ''} onClick={() => setTab(name)} key={name}>{name}</button>)}</nav>
    <aside className="notice">{notice}</aside>
    {tab === 'Live' && <section>
      <div className="metrics"><Metric label="LINK" value={connected ? 'ONLINE' : 'DEMO'} /><Metric label="EVENTS" value={events.length} /><Metric label="ANOMALIES" value={anomalies} /><Metric label="MODE" value={armed ? 'LAB ARMED' : 'PASSIVE'} /></div>
      <div className="panel"><div className="panel-title"><h2>Decoded timeline</h2><button onClick={toggleCapture}>{capturing ? 'Stop capture' : 'Start capture'}</button></div>
      <table><thead><tr><th>Time µs</th><th>Segment</th><th>Event</th><th>Value</th><th>Width</th><th>Voltage</th></tr></thead><tbody>{events.slice().reverse().map((e, i) => <tr className={e.type >= 5 ? 'warn' : ''} key={`${e.timestamp}-${i}`}><td>{e.timestamp}</td><td>{e.port ? 'device' : 'master'}</td><td>{names[e.type] ?? `Type ${e.type}`}</td><td>0x{e.value.toString(16).padStart(2, '0')}</td><td>{e.duration} µs</td><td>{e.millivolts} mV</td></tr>)}</tbody></table></div>
    </section>}
    {tab === 'Topology' && <section className="panel"><h2>Observed bus population</h2><p>Passive ROM observations are correlated with presence-pulse timing, producing an inventory without transmitting probes.</p><div className="cards">{roms.map(device => <article key={device.rom}><span className="pill">CRC {device.crc ? 'VALID' : 'BAD'}</span><h3>{device.family}</h3><code>{device.rom}</code><p>{device.count} observations · timing jitter {device.jitter} µs</p></article>)}</div></section>}
    {tab === 'Lab' && <section className="panel danger"><h2>Isolated lab drive</h2><p>Transmission is disabled until the physical ARM switch and a five-minute challenge session are both active. Use only on equipment you own or are explicitly authorized to test.</p><button onClick={armLab} disabled={armed}>{armed ? 'Session requested' : 'Request armed session'}</button><fieldset disabled={!armed}><label>Target segment<select><option>Isolated device side</option><option>Isolated master side</option></select></label><label>Byte (hex)<input defaultValue="CC" pattern="[0-9A-Fa-f]{2}" /></label><button onClick={() => setNotice('Drive action requires acknowledgement returned by hardware.')}>Queue one byte</button></fieldset></section>}
    {tab === 'Export' && <section className="panel"><h2>Evidence export</h2><p>Export the current normalized event stream for case notes and reproducible timing analysis.</p><button onClick={() => { const blob = new Blob([JSON.stringify({ author: 'jayis1', device: 'OneWire Cartographer', events }, null, 2)], { type: 'application/json' }); const a = document.createElement('a'); a.href = URL.createObjectURL(blob); a.download = 'onewire-capture.json'; a.click(); URL.revokeObjectURL(a.href); }}>Download JSON</button></section>}
    <footer>Designed and authored by jayis1 · Authorized security research only</footer>
  </main>;
}

createRoot(document.getElementById('root')).render(<App />);
