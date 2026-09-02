// I3C Poltergeist companion app by jayis1
const state = {
  armed: false,
  bypass: false,
  mode: 'observe',
  profile: 'inventory-passive',
  budget: 2,
  anomalies: 0,
  telemetry: {
    temp: 42.1,
    current: 88.3,
    voltage: 1.8,
    link: 'BLE + USB'
  },
  targets: [
    { index: 0, name: 'imu-array', dynamic: '0x12', legacy: true, secure: false, role: 'sensor' },
    { index: 1, name: 'touch-bridge', dynamic: '0x18', legacy: true, secure: false, role: 'touch' },
    { index: 2, name: 'pmic-shadow', dynamic: '0x1C', legacy: false, secure: true, role: 'pmic' },
    { index: 3, name: 'ec-proxy', dynamic: '0x22', legacy: true, secure: true, role: 'ec' },
    { index: 4, name: 'secure-haptics', dynamic: '0x29', legacy: true, secure: true, role: 'haptics' }
  ],
  profiles: [
    { name: 'inventory-passive', risk: 'low', desc: 'Observe ENTDAA, CCC sequence, and address assignment with no target-visible writes.' },
    { name: 'downgrade-probe', risk: 'medium', desc: 'Test whether selected peripherals still answer legacy I²C transactions after DAA.' },
    { name: 'hotjoin-ghost', risk: 'high', desc: 'Inject a bounded synthetic hot-join to evaluate controller admission and logging.' },
    { name: 'ibi-shadow', risk: 'high', desc: 'Replay an in-band interrupt vector to map host trust assumptions.' },
    { name: 'ccc-eclipse', risk: 'high', desc: 'Suppress or mirror one CCC to test retry and failover behavior.' }
  ],
  timeline: [
    { t: '00:005', label: 'RSTDAA broadcast observed', risk: 'low' },
    { t: '00:010', label: 'ENTDAA broadcast observed', risk: 'low' },
    { t: '00:020', label: 'touch-bridge assigned dynamic address 0x18', risk: 'low' },
    { t: '00:040', label: 'legacy fallback window detected on ec-proxy', risk: 'medium' }
  ],
  ethics: [
    'Use only with explicit written authorization.',
    'Confirm target rail voltage before attaching harnesses.',
    'Start in observe mode and capture a baseline before active profiles.',
    'Maintain a rollback path and power-cycle plan for the target.'
  ]
};

const qs = (selector) => document.querySelector(selector);
const qsa = (selector) => Array.from(document.querySelectorAll(selector));

function renderStatus() {
  const metrics = [
    ['Mode', state.bypass ? 'bypass' : state.mode],
    ['Profile', state.profile],
    ['Armed', state.armed ? 'yes' : 'no'],
    ['Write Budget', String(state.budget)],
    ['Anomalies', String(state.anomalies)],
    ['Board Temp', `${state.telemetry.temp.toFixed(1)} °C`],
    ['Rail Current', `${state.telemetry.current.toFixed(1)} mA`],
    ['Rail Voltage', `${state.telemetry.voltage.toFixed(2)} V`],
    ['Operator Link', state.telemetry.link]
  ];

  qs('#statusGrid').innerHTML = metrics
    .map(([label, value]) => `<article class="metric"><h3>${label}</h3><div>${value}</div></article>`)
    .join('');
}

function renderTargets() {
  qs('#targetsRoot').innerHTML = state.targets
    .map(
      (target) => `
        <article class="target-card">
          <h3>${target.index}: ${target.name}</h3>
          <p>Role: ${target.role}</p>
          <p>Dynamic Address: ${target.dynamic}</p>
          <p>Legacy Fallback: ${target.legacy ? 'present' : 'none observed'}</p>
          <p>Secure Role: ${target.secure ? 'yes' : 'no'}</p>
        </article>`
    )
    .join('');
}

function renderTelemetry() {
  const items = [
    `Board temperature trend: ${state.telemetry.temp.toFixed(1)} °C`,
    `Target rail current: ${state.telemetry.current.toFixed(1)} mA`,
    `Target rail voltage: ${state.telemetry.voltage.toFixed(2)} V`,
    `Management links: ${state.telemetry.link}`,
    `Capture count: ${state.timeline.length}`
  ];
  qs('#telemetryRoot').innerHTML = items.map((item) => `<li>${item}</li>`).join('');
}

function renderProfiles() {
  qs('#profilesRoot').innerHTML = state.profiles
    .map(
      (profile) => `
        <article class="profile-card">
          <h3>${profile.name}</h3>
          <div class="badge ${profile.risk}">${profile.risk.toUpperCase()} RISK</div>
          <p>${profile.desc}</p>
          <button data-profile="${profile.name}">Load Profile</button>
        </article>`
    )
    .join('');

  qsa('[data-profile]').forEach((button) => {
    button.addEventListener('click', () => {
      state.profile = button.dataset.profile;
      state.mode = state.profile === 'inventory-passive' ? 'observe' : 'guarded';
      state.armed = false;
      appendTimeline(`profile ${state.profile} loaded`, 'low');
      repaint();
    });
  });
}

function renderTimeline() {
  qs('#timelineRoot').innerHTML = state.timeline
    .map(
      (event) => `
        <article class="timeline-entry">
          <div class="stamp">${event.t}</div>
          <div>${event.label}</div>
          <div class="badge ${event.risk}">${event.risk}</div>
        </article>`
    )
    .join('');
}

function renderEthics() {
  qs('#ethicsRoot').innerHTML = state.ethics.map((item) => `<li>${item}</li>`).join('');
}

function appendTimeline(label, risk) {
  const seconds = String(state.timeline.length).padStart(2, '0');
  state.timeline.unshift({ t: `00:${seconds}`, label, risk });
  state.timeline = state.timeline.slice(0, 12);
}

function repaint() {
  renderStatus();
  renderTargets();
  renderTelemetry();
  renderProfiles();
  renderTimeline();
  renderEthics();
}

qsa('.tab').forEach((tab) => {
  tab.addEventListener('click', () => {
    qsa('.tab').forEach((node) => node.classList.remove('active'));
    qsa('.panel').forEach((node) => node.classList.remove('active'));
    tab.classList.add('active');
    qs(`#${tab.dataset.tab}`).classList.add('active');
  });
});

qs('#armBtn').addEventListener('click', () => {
  state.armed = true;
  state.mode = state.bypass ? 'bypass' : 'active';
  appendTimeline(`profile ${state.profile} armed`, 'medium');
  repaint();
});

qs('#bypassBtn').addEventListener('click', () => {
  state.bypass = true;
  state.armed = false;
  state.mode = 'bypass';
  appendTimeline('fail-safe bypass engaged by operator', 'high');
  repaint();
});

qs('#simulateBtn').addEventListener('click', () => {
  state.telemetry.temp += 0.7;
  state.telemetry.current += 6.2;
  state.anomalies += state.profile === 'inventory-passive' ? 0 : 1;
  appendTimeline(`capture burst under ${state.profile}`, state.profile === 'inventory-passive' ? 'low' : 'medium');
  repaint();
});

qs('#commandForm').addEventListener('submit', (event) => {
  event.preventDefault();
  const command = qs('#commandSelect').value;
  const targetIndex = Number(qs('#targetInput').value);
  const payload = qs('#payloadInput').value;

  const result = {
    author: 'jayis1',
    command,
    targetIndex,
    payload,
    armed: state.armed,
    bypass: state.bypass,
    accepted: state.armed && !state.bypass,
    profile: state.profile
  };

  if (result.accepted) {
    state.budget = Math.max(0, state.budget - 1);
    appendTimeline(`staged ${command} for target ${targetIndex} with payload ${payload}`, 'high');
  } else {
    state.anomalies += 1;
    appendTimeline(`command rejected: arm required before ${command}`, 'medium');
  }

  qs('#commandOutput').textContent = JSON.stringify(result, null, 2);
  repaint();
});

repaint();
