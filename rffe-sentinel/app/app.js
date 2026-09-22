// RFFE Sentinel companion application
// Author: jayis1
// SPDX-License-Identifier: MIT

export const MAGIC = 0x50435352;
export const VERSION = 1;

export function crc32c(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) {
      const mask = -(crc & 1);
      crc = (crc >>> 1) ^ (0x82f63b78 & mask);
    }
  }
  return (~crc) >>> 0;
}

export function validateRule(rule) {
  const usid = Number(rule.usid);
  const first = Number(rule.first);
  const last = Number(rule.last);
  const active = ['deny', 'substitute', 'delay'].includes(rule.action);
  if (!Number.isInteger(usid) || usid < 0 || usid > 15) return 'USID must be 0–15';
  if (!Number.isInteger(first) || !Number.isInteger(last) || first < 0 || last > 0xffff || first > last) return 'Invalid register range';
  if (active && rule.usid === '*') return 'Active rules require an explicit USID';
  if (rule.action === 'delay' && (Number(rule.delay) < 0 || Number(rule.delay) > 2000)) return 'Delay must be 0–2000 ns';
  return '';
}

export function filterEvents(events, query) {
  const text = query.trim().toLowerCase();
  if (!text) return events;
  return events.filter((event) => Object.values(event).some((value) => String(value).toLowerCase().includes(text)));
}

const state = {
  screen: 'dashboard',
  mode: 'PASSIVE_CAPTURE',
  lease: 0,
  rules: [{ usid: 2, first: 16, last: 31, action: 'alert', delay: 0 }],
  events: [
    { time: '00:00.001204', usid: 2, command: 'REG_WRITE', register: '0x10', value: '0x44', action: 'forward', flags: 'OK' },
    { time: '00:00.001688', usid: 2, command: 'REG_WRITE', register: '0x12', value: '0xf3', action: 'alert', flags: 'OK' },
    { time: '00:00.002041', usid: 7, command: 'EXT_READ', register: '0x0110', value: '0x08', action: 'forward', flags: 'PARK' },
    { time: '00:00.003775', usid: 2, command: 'REG_WRITE', register: '0x16', value: '0x81', action: 'forward', flags: 'PARITY' }
  ]
};

function dashboard() {
  return `<h1>Dashboard</h1>
    <div class="banner good card">Bypass path verified. Intervention is not armed.</div>
    <div class="grid">
      <section class="card"><h3>Operating mode</h3><div class="value">${state.mode}</div></section>
      <section class="card"><h3>Target VIO</h3><div class="value">1.800 V</div></section>
      <section class="card"><h3>Target current</h3><div class="value">7.8 mA</div></section>
      <section class="card"><h3>Frames / second</h3><div class="value">12,481</div></section>
      <section class="card"><h3>Parity failures</h3><div class="value">1</div></section>
      <section class="card"><h3>FPGA watchdog</h3><div class="value good">Healthy</div></section>
    </div>
    <div class="actions"><button class="primary" id="captureToggle">Pause passive capture</button><button class="dangerButton" id="bypassNow">Return to bypass</button></div>
    <section class="card"><h3>Trust boundary</h3><p>The demo transport generates synthetic RFFE metadata locally. No radio samples, packet payloads, or subscriber data are collected.</p></section>`;
}

function timeline() {
  const rows = state.events.map((event) => `<tr>${Object.values(event).map((value) => `<td>${value}</td>`).join('')}</tr>`).join('');
  return `<h1>Evidence timeline</h1>
    <label>Filter events<input id="eventFilter" placeholder="USID, register, action, flag"></label>
    <div class="actions"><button class="primary" id="exportEvents">Export redacted JSON</button><label class="primary">Import evidence<input id="importEvents" type="file" accept="application/json" hidden></label><button class="dangerButton" id="deleteEvents">Delete local events</button></div>
    <table><thead><tr><th>Time</th><th>USID</th><th>Command</th><th>Register</th><th>Value</th><th>Action</th><th>Flags</th></tr></thead><tbody id="eventRows">${rows}</tbody></table>`;
}

function policy() {
  const rules = state.rules.map((rule, index) => `<tr><td>${index + 1}</td><td>${rule.usid}</td><td>0x${rule.first.toString(16)}–0x${rule.last.toString(16)}</td><td>${rule.action}</td><td><button data-remove="${index}">Remove</button></td></tr>`).join('');
  return `<h1>Policy editor</h1>
    <div class="banner warning">Active rules are inert until the physical ARM button is pressed and a time-limited lease is granted.</div>
    <section class="card"><h3>Add explicit rule</h3><div class="rule">
      <label>USID<input id="usid" type="number" min="0" max="15" value="2"></label>
      <label>First register<input id="first" value="16"></label>
      <label>Last register<input id="last" value="31"></label>
      <label>Action<select id="action"><option>alert</option><option>deny</option><option>substitute</option><option>delay</option></select></label>
    </div><button class="primary" id="addRule">Validate and add</button><p id="ruleError" class="danger"></p></section>
    <h2>Compiled rules</h2><table><thead><tr><th>#</th><th>USID</th><th>Range</th><th>Action</th><th></th></tr></thead><tbody>${rules}</tbody></table>`;
}

function safety() {
  return `<h1>Safety &amp; authorization</h1>
    <div class="banner danger"><strong>Authorized laboratory use only.</strong> Active RFFE intervention can change radio emissions and damage front-end components. Confirm written scope, shielding, dummy loads, and emergency power removal.</div>
    <section class="card"><h3>Arm checklist</h3>
      <label><input class="check" type="checkbox"> I have written authorization for this target.</label>
      <label><input class="check" type="checkbox"> The target is in a compliant shielded setup.</label>
      <label><input class="check" type="checkbox"> I verified VIO, current limit, and bypass on a scope.</label>
      <label><input class="check" type="checkbox"> I will press the physical ARM button within ten seconds.</label>
      <button class="amber" id="requestLease">Request 60-second lease</button>
      <button class="dangerButton" id="cancelLease">Cancel intervention</button>
      <p id="leaseStatus">No active lease.</p>
    </section>`;
}

function render() {
  const root = document.querySelector('#app');
  root.innerHTML = ({ dashboard, timeline, policy, safety }[state.screen])();
  document.querySelectorAll('aside button').forEach((button) => button.classList.toggle('active', button.dataset.screen === state.screen));
  bindScreen();
}

function bindScreen() {
  document.querySelector('#bypassNow')?.addEventListener('click', () => { state.mode = 'SAFE_BYPASS'; state.lease = 0; render(); });
  document.querySelector('#captureToggle')?.addEventListener('click', () => { state.mode = state.mode === 'PASSIVE_CAPTURE' ? 'SAFE_BYPASS' : 'PASSIVE_CAPTURE'; render(); });
  document.querySelector('#eventFilter')?.addEventListener('input', (event) => {
    const rows = filterEvents(state.events, event.target.value).map((item) => `<tr>${Object.values(item).map((value) => `<td>${value}</td>`).join('')}</tr>`).join('');
    document.querySelector('#eventRows').innerHTML = rows;
  });
  document.querySelector('#deleteEvents')?.addEventListener('click', () => { state.events = []; render(); });
  document.querySelector('#exportEvents')?.addEventListener('click', () => {
    const blob = new Blob([JSON.stringify({ author: 'jayis1', protocol: 'RSCP/1', events: state.events }, null, 2)], { type: 'application/json' });
    const anchor = Object.assign(document.createElement('a'), { href: URL.createObjectURL(blob), download: 'rffe-sentinel-redacted.json' });
    anchor.click(); URL.revokeObjectURL(anchor.href);
  });
  document.querySelector('#importEvents')?.addEventListener('change', async (event) => {
    const file = event.target.files[0];
    if (!file || file.size > 1024 * 1024) return;
    try {
      const imported = JSON.parse(await file.text());
      if (imported.author !== 'jayis1' || imported.protocol !== 'RSCP/1' || !Array.isArray(imported.events)) return;
      state.events = imported.events.slice(0, 4096).filter((item) => item && typeof item === 'object');
      render();
    } catch {
      // Invalid evidence is ignored; it never changes the current timeline.
    }
  });
  document.querySelector('#addRule')?.addEventListener('click', () => {
    const rule = { usid: Number(document.querySelector('#usid').value), first: Number(document.querySelector('#first').value), last: Number(document.querySelector('#last').value), action: document.querySelector('#action').value, delay: 0 };
    const error = validateRule(rule);
    if (error) { document.querySelector('#ruleError').textContent = error; return; }
    state.rules.push(rule); render();
  });
  document.querySelectorAll('[data-remove]').forEach((button) => button.addEventListener('click', () => { state.rules.splice(Number(button.dataset.remove), 1); render(); }));
  document.querySelector('#requestLease')?.addEventListener('click', () => {
    const checked = [...document.querySelectorAll('.check')].every((item) => item.checked);
    document.querySelector('#leaseStatus').textContent = checked ? 'Checklist complete. Press the physical ARM button; software alone cannot arm.' : 'Complete every checklist item first.';
  });
  document.querySelector('#cancelLease')?.addEventListener('click', () => { state.mode = 'SAFE_BYPASS'; state.lease = 0; render(); });
}

if (typeof document !== 'undefined') {
  document.querySelectorAll('aside button').forEach((button) => button.addEventListener('click', () => { state.screen = button.dataset.screen; render(); }));
  render();
}
