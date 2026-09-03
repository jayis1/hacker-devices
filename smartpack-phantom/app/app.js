// SmartPack Phantom companion app
// Author: jayis1
// Copyright (c) 2026 jayis1
const state = {
  status: {
    profile: 'enterprise-laptop',
    soc: 79,
    tempC: 31,
    auth: 'passthrough',
    health: 'nominal',
    mutations: 0,
    chargeLimit: 3200,
    scenario: 'idle'
  },
  timeline: [
    { time: '00:00.100', type: 'boot', detail: 'Passive proxy mode enabled' },
    { time: '00:00.250', type: 'smbus', detail: 'Host queried voltage/current/status' },
    { time: '00:00.500', type: 'auth', detail: 'Challenge-response observed' }
  ],
  queuedAuth: []
};

function renderStatus() {
  const stats = document.getElementById('stats');
  stats.innerHTML = '';
  const entries = [
    ['Profile', state.status.profile],
    ['SoC', `${state.status.soc}%`],
    ['Temperature', `${state.status.tempC} °C`],
    ['Auth', state.status.auth],
    ['Health', state.status.health],
    ['Charge Limit', `${state.status.chargeLimit} mA`],
    ['Scenario', state.status.scenario],
    ['Mutations', String(state.status.mutations)]
  ];
  for (const [label, value] of entries) {
    const div = document.createElement('div');
    div.className = 'stat';
    div.innerHTML = `<div class="label">${label}</div><div class="value">${value}</div>`;
    stats.appendChild(div);
  }
}

function renderTimeline() {
  const tbody = document.getElementById('timeline');
  tbody.innerHTML = '';
  for (const row of state.timeline) {
    const tr = document.createElement('tr');
    tr.innerHTML = `<td>${row.time}</td><td>${row.type}</td><td>${row.detail}</td>`;
    tbody.appendChild(tr);
  }
}

function renderAuthQueue() {
  const list = document.getElementById('auth-events');
  list.innerHTML = '';
  state.queuedAuth.forEach((item) => {
    const li = document.createElement('li');
    li.textContent = item;
    list.appendChild(li);
  });
}

function addTimeline(type, detail) {
  const seconds = String(state.timeline.length).padStart(2, '0');
  state.timeline.unshift({ time: `00:${seconds}.000`, type, detail });
  state.timeline = state.timeline.slice(0, 16);
  renderTimeline();
}

document.getElementById('refresh-status').addEventListener('click', () => {
  renderStatus();
  addTimeline('ui', 'Status refreshed');
});

document.getElementById('arm-scenario').addEventListener('click', () => {
  const scenario = document.getElementById('scenario').value;
  state.status.scenario = `${scenario} (armed)`;
  document.getElementById('scenario-log').textContent = `Armed ${scenario}`;
  addTimeline('scenario', `Armed ${scenario}`);
  renderStatus();
});

document.getElementById('trigger-scenario').addEventListener('click', () => {
  const scenario = document.getElementById('scenario').value;
  state.status.scenario = `${scenario} (triggered)`;
  state.status.mutations += 3;
  if (scenario === 'thermal-trip-spoof') {
    state.status.health = 'thermal-warning';
    state.status.tempC = 78;
  }
  if (scenario === 'low-soc-bait') {
    state.status.soc = 2;
    state.status.health = 'near-empty';
  }
  if (scenario === 'charger-limit-swing') {
    state.status.chargeLimit = 500;
    state.status.health = 'limited';
  }
  document.getElementById('scenario-log').textContent = `Triggered ${scenario}`;
  addTimeline('scenario', `Triggered ${scenario}`);
  renderStatus();
});

document.getElementById('apply-profile').addEventListener('click', () => {
  state.status.profile = document.getElementById('profile-kind').value;
  state.status.soc = Number(document.getElementById('soc').value);
  state.status.tempC = Number(document.getElementById('temp').value);
  state.status.chargeLimit = Number(document.getElementById('charge-limit').value);
  state.status.health = state.status.tempC > 55 ? 'thermal-warning' : state.status.soc < 5 ? 'near-empty' : 'nominal';
  addTimeline('profile', `Applied ${state.status.profile} overlay`);
  renderStatus();
});

document.getElementById('queue-auth').addEventListener('click', () => {
  const mode = document.getElementById('auth-mode').value;
  state.status.auth = mode;
  state.queuedAuth.unshift(`Queued auth mode: ${mode}`);
  state.queuedAuth = state.queuedAuth.slice(0, 8);
  addTimeline('auth', `Set auth mode to ${mode}`);
  renderAuthQueue();
  renderStatus();
});

document.getElementById('export-json').addEventListener('click', () => {
  const payload = {
    author: 'jayis1',
    device: 'SmartPack Phantom',
    status: state.status,
    timeline: state.timeline,
    queuedAuth: state.queuedAuth
  };
  document.getElementById('export-output').value = JSON.stringify(payload, null, 2);
  addTimeline('export', 'Generated evidence bundle');
});

renderStatus();
renderTimeline();
renderAuthQueue();
