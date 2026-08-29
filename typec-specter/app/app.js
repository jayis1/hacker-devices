// Type-C Specter companion application
// Author: jayis1
// Copyright (c) 2026 jayis1
const state = {
  ethicsAccepted: false,
  attachment: {
    target: 'Laptop target detected',
    peer: 'Peer dock emulation attached',
    role: 'Sink ↔ Source',
    orientation: 'CC1 active, USB2 pass-through'
  },
  telemetry: {
    targetVBUS: '5.08 V',
    peerVBUS: '9.00 V',
    current: '1.43 A',
    temperature: '43 C'
  },
  safety: 'Passive monitor safe mode is active. Mutation actions require operator acknowledgment.',
  scenarios: [
    { name: 'dock_identity_flip', detail: 'Proxy a legitimate dock, then pivot to a forged identity during steady-state attachment.', risk: 'Tests trust persistence after identity changes.' },
    { name: 'late_vconn_claim', detail: 'Introduce sideband isolation and altered identity timing to study VCONN policy handling.', risk: 'Exercises unusual power/sideband transitions.' },
    { name: 'role_swap_race', detail: 'Request role swaps near host policy transitions and observe reset behavior.', risk: 'Can reveal unstable firmware rollback logic.' },
    { name: 'debug_accessory_probe', detail: 'Briefly assert debug accessory conditions during attach windows.', risk: 'Targets hidden or privileged service modes.' },
    { name: 'power_starve_then_recover', detail: 'Throttle available current, then restore normal behavior under trace.', risk: 'Finds maintenance and brownout fallbacks.' }
  ],
  profiles: [
    ['transparent-proxy', 'Baseline', 'No mutation; observe only', 'Default safe mode'],
    ['corp-dock-shadow', 'Dock emulation', 'Stable identity flip after attach', 'May trigger driver reload paths'],
    ['late-vconn-claim', 'Cable/power anomaly', 'Shifts assumptions around VCONN timing', 'Use only on lab-approved hardware'],
    ['debug-service-bait', 'Accessory lure', 'Tests debug accessory handling', 'High value for service-port research']
  ],
  logs: [
    '[00.000] BOOT  passive monitor enabled',
    '[00.300] ARM   scenario dock_identity_flip',
    '[00.700] PD    identity profile -> corp-dock-shadow',
    '[01.300] MUX   USB2 isolated for scripted perturbation',
    '[01.800] MUX   USB2 restored',
    '[02.200] PD    mutation disabled',
    '[02.400] DONE  scenario complete'
  ]
};

const q = (sel) => document.querySelector(sel);
const qa = (sel) => [...document.querySelectorAll(sel)];

function renderDashboard() {
  q('#attachment-list').innerHTML = Object.entries(state.attachment).map(([k, v]) => `<li><strong>${k}</strong>: ${v}</li>`).join('');
  q('#telemetry-list').innerHTML = Object.entries(state.telemetry).map(([k, v]) => `<li><strong>${k}</strong>: ${v}</li>`).join('');
  q('#safety-state').textContent = state.safety;
}

function renderScenarios() {
  q('#scenario-cards').innerHTML = state.scenarios.map((scenario, idx) => `
    <article class="card">
      <span class="badge">Scenario ${idx + 1}</span>
      <h3>${scenario.name}</h3>
      <p>${scenario.detail}</p>
      <p class="warn">${scenario.risk}</p>
      <button data-arm="${scenario.name}">Arm Scenario</button>
    </article>`).join('');

  qa('[data-arm]').forEach(btn => btn.addEventListener('click', () => {
    if (!state.ethicsAccepted) {
      state.logs.push(`[BLOCK] Cannot arm ${btn.dataset.arm} before ethics acknowledgement.`);
      renderLogs();
      alert('Acknowledge authorized-use gate first.');
      return;
    }
    state.safety = `Scenario ${btn.dataset.arm} armed. Mutation timer bounded and audit logging active.`;
    state.logs.push(`[ARM] ${btn.dataset.arm} armed by operator`);
    renderDashboard();
    renderLogs();
  }));
}

function renderProfiles() {
  q('#profiles-body').innerHTML = state.profiles.map(profile => `
    <tr><td>${profile[0]}</td><td>${profile[1]}</td><td>${profile[2]}</td><td>${profile[3]}</td></tr>`).join('');
}

function renderLogs() {
  q('#log-output').textContent = state.logs.join('\n');
}

function exportJson() {
  const blob = new Blob([JSON.stringify(state, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'typec-specter-session.json';
  a.click();
  URL.revokeObjectURL(url);
}

function bindNavigation() {
  qa('.nav-btn').forEach(btn => btn.addEventListener('click', () => {
    qa('.nav-btn').forEach(b => b.classList.remove('active'));
    qa('.screen').forEach(s => s.classList.remove('active'));
    btn.classList.add('active');
    q(`#${btn.dataset.screen}`).classList.add('active');
  }));
}

function bindEthics() {
  q('#ethics-check').addEventListener('change', (event) => {
    state.ethicsAccepted = event.target.checked;
    q('#ethics-status').textContent = state.ethicsAccepted
      ? 'Acknowledged. Scenario arming unlocked for authorized testing.'
      : 'Mutation controls locked until acknowledged.';
  });
}

function bindToolbar() {
  q('#export-json').addEventListener('click', exportJson);
  q('#replay-log').addEventListener('click', () => {
    state.logs.push('[REPLAY] Demo timeline replayed for operator review.');
    renderLogs();
  });
}

renderDashboard();
renderScenarios();
renderProfiles();
renderLogs();
bindNavigation();
bindEthics();
bindToolbar();
