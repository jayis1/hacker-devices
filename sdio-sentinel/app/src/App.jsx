// SDIO Sentinel operational console. Author: jayis1.
import React, { useEffect, useMemo, useState } from 'react';
import { DemoTransport } from './transport.js';
import { exportAudit, modeName, Modes, SCOPE_STATEMENT, validateRule } from './protocol.js';

const navigation = ['Overview', 'Live trace', 'Policy', 'Audit export'];
const emptyStatus = {
  boardId: 0, uptimeMs: 0, framesSeen: 0, blocked: 0, anomalies: 0,
  clockHz: 0, mode: Modes.SAFE_BYPASS, cardState: 0, armed: false, cardPresent: false,
};

function Metric({ label, value, tone = '' }) {
  return <div className={`metric ${tone}`}><span>{label}</span><strong>{value}</strong></div>;
}

function Banner({ kind, children }) {
  return <div role="status" className={`banner ${kind}`}>{children}</div>;
}

function Overview({ connected, status, frames }) {
  const recent = frames.slice(0, 5);
  return <section>
    <div className="section-heading">
      <div><p className="eyebrow">Trust boundary</p><h2>Removable-media posture</h2></div>
      <span className={`connection ${connected ? 'online' : ''}`}>{connected ? 'ONLINE / DEMO' : 'OFFLINE'}</span>
    </div>
    <div className="metrics">
      <Metric label="Observed frames" value={status.framesSeen.toLocaleString()} />
      <Metric label="Anomalies" value={status.anomalies} tone={status.anomalies ? 'amber' : ''} />
      <Metric label="Blocked" value={status.blocked} tone={status.blocked ? 'red' : ''} />
      <Metric label="Clock" value={`${(status.clockHz / 1_000_000).toFixed(1)} MHz`} />
    </div>
    <div className="grid two">
      <article className="panel">
        <p className="eyebrow">Current state</p>
        <h3>{modeName(status.mode)}</h3>
        <dl className="status-list">
          <div><dt>Card presence</dt><dd>{status.cardPresent ? 'Detected' : 'No card'}</dd></div>
          <div><dt>Physical arm</dt><dd>{status.armed ? 'Active' : 'Not armed'}</dd></div>
          <div><dt>Bus state</dt><dd>{status.cardState === 5 ? 'Transfer' : `State ${status.cardState}`}</dd></div>
          <div><dt>Board identity</dt><dd>{status.boardId ? `0x${status.boardId.toString(16)}` : '—'}</dd></div>
        </dl>
      </article>
      <article className="panel">
        <p className="eyebrow">Latest findings</p>
        <div className="finding-list">
          {recent.length === 0 && <p className="muted">Connect demo mode to generate a safe simulated trace.</p>}
          {recent.map((frame) => <div className="finding" key={frame.sequence}>
            <span className={`dot ${frame.severity}`} />
            <div><strong>{frame.name}</strong><small>{frame.summary}</small></div>
            <code>#{frame.sequence}</code>
          </div>)}
        </div>
      </article>
    </div>
  </section>;
}

function Trace({ frames, paused, setPaused, clearFrames }) {
  return <section>
    <div className="section-heading">
      <div><p className="eyebrow">Protocol telemetry</p><h2>Live command trace</h2></div>
      <div className="actions">
        <button className="secondary" onClick={() => setPaused(!paused)}>{paused ? 'Resume' : 'Pause'}</button>
        <button className="ghost" onClick={clearFrames}>Clear</button>
      </div>
    </div>
    <div className="panel table-wrap">
      <table><thead><tr><th>Seq</th><th>Time</th><th>Command</th><th>Argument</th><th>Score</th><th>Decision</th></tr></thead>
        <tbody>{frames.map((frame) => <tr key={frame.sequence}>
          <td>#{frame.sequence}</td><td>{frame.timeMs} ms</td><td><strong>{frame.name}</strong><small>{frame.direction}</small></td>
          <td><code>0x{frame.argument.toString(16).padStart(8, '0')}</code></td>
          <td><span className={`score ${frame.severity}`}>{frame.score}</span></td>
          <td><span className={`decision ${frame.action}`}>{frame.action}</span></td>
        </tr>)}</tbody>
      </table>
      {frames.length === 0 && <p className="empty">No transactions captured.</p>}
    </div>
  </section>;
}

function Policy({ status, setMode, rules, addRule }) {
  const [scope, setScope] = useState(false);
  const [command, setCommand] = useState('24');
  const [action, setAction] = useState('block');
  const [mask, setMask] = useState('0xffffffff');
  const [error, setError] = useState('');

  async function submit(event) {
    event.preventDefault();
    const candidate = { command: Number(command), action, argumentMask: mask };
    const issue = validateRule(candidate);
    if (issue) return setError(issue);
    setError('');
    await addRule(candidate);
  }

  return <section>
    <div className="section-heading"><div><p className="eyebrow">Fail-closed controls</p><h2>Policy workspace</h2></div></div>
    <Banner kind="warning"><strong>Active enforcement is gated.</strong> The device also requires a 1.5-second physical ARM hold. This console cannot bypass that control.</Banner>
    <div className="grid two">
      <article className="panel">
        <h3>Operating mode</h3>
        <div className="mode-stack">
          {[Modes.SAFE_BYPASS, Modes.PASSIVE_OBSERVE, Modes.ENFORCE_POLICY].map((mode) =>
            <button key={mode} className={`mode ${status.mode === mode ? 'selected' : ''}`}
              disabled={mode > Modes.PASSIVE_OBSERVE && !scope}
              onClick={() => setMode(mode, scope)}>
              <span>{modeName(mode)}</span><small>{mode === 0 ? 'Transparent electrical bypass' : mode === 1 ? 'Analyze without intervention' : 'Apply approved block rules'}</small>
            </button>)}
        </div>
        <label className="consent"><input type="checkbox" checked={scope} onChange={(event) => setScope(event.target.checked)} />
          <span>I confirm written authorization and an in-scope test fixture.<small>{SCOPE_STATEMENT}</small></span>
        </label>
      </article>
      <article className="panel">
        <h3>Add bounded rule</h3>
        <form onSubmit={submit} className="rule-form">
          <label>SD command<input value={command} onChange={(event) => setCommand(event.target.value)} inputMode="numeric" /></label>
          <label>Action<select value={action} onChange={(event) => setAction(event.target.value)}><option value="block">Block</option><option value="log">Log</option><option value="require-arm">Require physical arm</option></select></label>
          <label>Argument mask<input value={mask} onChange={(event) => setMask(event.target.value)} spellCheck="false" /></label>
          {error && <p className="form-error">{error}</p>}
          <button className="primary" type="submit">Add policy rule</button>
        </form>
      </article>
    </div>
    <article className="panel rules"><h3>Session rules</h3>{rules.map((rule) => <div key={rule.id} className="rule-row"><code>CMD{rule.command}</code><span>{rule.action}</span><code>{rule.argumentMask}</code><b>enabled</b></div>)}</article>
  </section>;
}

function Audit({ frames }) {
  const risky = frames.filter((frame) => frame.severity !== 'info');
  function download() {
    const blob = new Blob([exportAudit(risky)], { type: 'application/json' });
    const link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = 'sdio-sentinel-redacted-audit.json';
    link.click();
    URL.revokeObjectURL(link.href);
  }
  return <section><div className="section-heading"><div><p className="eyebrow">Data minimization</p><h2>Redacted audit export</h2></div></div>
    <div className="grid two"><article className="panel"><h3>{risky.length} findings ready</h3><p>Exports contain command metadata, policy decisions, and timestamps. Raw sector contents are intentionally excluded.</p><button className="primary" disabled={!risky.length} onClick={download}>Export JSON</button></article>
      <article className="panel"><h3>Retention boundary</h3><p>The device keeps a bounded 64-entry audit ring. Closing the browser clears this demo session. Production deployments should set an engagement-specific retention period.</p></article></div></section>;
}

export default function App() {
  const transport = useMemo(() => new DemoTransport(), []);
  const [screen, setScreen] = useState('Overview');
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState(emptyStatus);
  const [frames, setFrames] = useState([]);
  const [rules, setRules] = useState([{ id: 'default-boot', command: 24, action: 'require-arm', argumentMask: '0xffffe000', enabled: true }]);
  const [paused, setPaused] = useState(false);
  const [notice, setNotice] = useState('Demo transport is isolated from real hardware.');

  useEffect(() => transport.subscribe((frame) => {
    if (!paused) setFrames((current) => [frame, ...current].slice(0, 100));
  }), [transport, paused]);

  useEffect(() => {
    if (!connected) return undefined;
    const timer = setInterval(() => transport.status().then(setStatus).catch((error) => setNotice(error.message)), 800);
    return () => clearInterval(timer);
  }, [connected, transport]);

  async function toggleConnection() {
    try {
      if (connected) { await transport.disconnect(); setConnected(false); setStatus(emptyStatus); }
      else { await transport.connect(); setConnected(true); setStatus(await transport.status()); }
    } catch (error) { setNotice(error.message); }
  }

  async function setMode(mode, scope) {
    try { setStatus(await transport.setMode(mode, scope)); setNotice(`Mode changed to ${modeName(mode)}.`); }
    catch (error) { setNotice(error.message); }
  }

  async function addRule(rule) {
    const created = await transport.addRule(rule);
    setRules((current) => [...current, created]);
  }

  return <div className="shell">
    <aside><div className="brand"><div className="mark">SD</div><div><strong>SDIO SENTINEL</strong><small>by jayis1</small></div></div>
      <nav>{navigation.map((item) => <button key={item} className={screen === item ? 'active' : ''} onClick={() => setScreen(item)}>{item}</button>)}</nav>
      <div className="safety"><span className="shield">✓</span><div><strong>Authorized use only</strong><small>Passive and fail-safe by default</small></div></div>
    </aside>
    <main><header><div><p>DEVICE / <b>{connected ? 'DEMO-ONLINE' : 'NOT CONNECTED'}</b></p><small>{notice}</small></div><button className={connected ? 'secondary' : 'primary'} onClick={toggleConnection}>{connected ? 'Disconnect' : 'Connect demo'}</button></header>
      {screen === 'Overview' && <Overview connected={connected} status={status} frames={frames} />}
      {screen === 'Live trace' && <Trace frames={frames} paused={paused} setPaused={setPaused} clearFrames={() => setFrames([])} />}
      {screen === 'Policy' && <Policy status={status} setMode={setMode} rules={rules} addRule={addRule} />}
      {screen === 'Audit export' && <Audit frames={frames} />}
      <footer>SDIO Sentinel console v1.0.0 · Author jayis1 · No captured sector data leaves the browser</footer>
    </main>
  </div>;
}
