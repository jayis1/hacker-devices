// BootROM Banshee companion app
// Author: jayis1
// Copyright (c) 2026 jayis1
const state = {
  ethicsAccepted: false,
  currentLimit: 700,
  activeOverlay: 'transparent',
  banner: 'Passive proxy mode. Baseline capture is active and mutation controls are locked until authorized-use acknowledgement.',
  runtime: {
    scenario: 'header-ghost',
    busMode: 'qspi',
    target: 'Target SoC detected via interposer harness',
    flash: 'Boot flash online at 1.8 V domain',
    strap: 'normal-boot',
    mutation: 'disabled'
  },
  telemetry: {
    targetVccio: '1.82 V',
    flashVccio: '1.80 V',
    current: '342 mA',
    temperature: '46 C'
  },
  scenarios: [
    {
      id: 'rollback-shadow',
      title: 'Rollback Shadow',
      detail: 'Overlay stale anti-rollback metadata on selected reads without rewriting the physical flash.',
      timing: '100-1200 ms bounded mutation window',
      effect: 'Verifies whether ROM and stage-1 reject downgrade metadata independently.'
    },
    {
      id: 'jedec-masquerade',
      title: 'JEDEC Masquerade',
      detail: 'Serve a synthetic JEDEC ID and altered SFDP personality during discovery.',
      timing: '100-1000 ms bounded mutation window',
      effect: 'Tests flash identity trust and parser assumptions.'
    },
    {
      id: 'late-ready-stall',
      title: 'Late Ready Stall',
      detail: 'Hold the flash busy across a chosen retry window, then release clean reads.',
      timing: '100-1300 ms bounded mutation window',
      effect: 'Maps timeout handling and recovery behavior.'
    },
    {
      id: 'header-ghost',
      title: 'Header Ghost',
      detail: 'Replace only the first boot header slice and leave later reads untouched.',
      timing: '100-1200 ms bounded mutation window',
      effect: 'Identifies which early bytes actually anchor trust decisions.'
    }
  ],
  overlays: [
    { name: 'transparent', base: '0x00000000', purpose: 'Observe only; no read changes.', sample: '54 52 41 4E 53 50 41 52' },
    { name: 'rollback-shadow', base: '0x00001000', purpose: 'Downgrade and anti-rollback field mutation.', sample: '42 42 52 4F 00 00 00 01' },
    { name: 'header-ghost', base: '0x00000000', purpose: 'Synthetic header magic and manifest selector.', sample: '47 48 4F 53 54 48 44 52' },
    { name: 'manifest-delta', base: '0x00002000', purpose: 'Partition and manifest metadata mutation.', sample: '4D 41 4E 49 46 45 53 54' }
  ],
  captures: [
    '[00.000] 9F  JEDEC read -> 20 BA 19',
    '[00.100] 5A  SFDP read -> 53 46 44 50',
    '[00.200] EB  addr 0x00000000 len 32 -> header slice',
    '[00.300] 05  status read -> ready',
    '[00.400] 6B  addr 0x00001000 len 24 -> manifest slice',
    '[00.500] EB  addr 0x00002000 len 32 -> secondary header'
  ],
  audit: [
    '[BOOT] Passive proxy engaged',
    '[SAFE] Mutation controls locked by ethics gate',
    '[NOTE] Demo session loaded by jayis1 reference app'
  ]
};

const q = (sel) => document.querySelector(sel);
const qa = (sel) => [...document.querySelectorAll(sel)];

function renderOverview() {
  q('#status-list').innerHTML = Object.entries(state.runtime)
    .map(([key, value]) => `<li><strong>${key}</strong>: ${value}</li>`)
    .join('');
  q('#telemetry-list').innerHTML = Object.entries(state.telemetry)
    .map(([key, value]) => `<li><strong>${key}</strong>: ${value}</li>`)
    .join('');
  q('#banner-text').textContent = state.banner;
}

function armScenario(id) {
  if (!state.ethicsAccepted) {
    state.audit.push(`[BLOCK] Attempted to arm ${id} before authorized-use acknowledgement.`);
    renderAudit();
    return alert('Authorized-use acknowledgement is required before active manipulation.');
  }
  const scenario = state.scenarios.find(item => item.id === id);
  state.runtime.scenario = scenario.id;
  state.runtime.mutation = 'armed';
  state.banner = `Scenario ${scenario.title} armed. Trigger only during a controlled reset in an authorized environment.`;
  state.audit.push(`[ARM] ${scenario.id} armed with current limit ${state.currentLimit} mA.`);
  renderOverview();
  renderAudit();
}

function renderScenarios() {
  q('#scenario-cards').innerHTML = state.scenarios.map((scenario, index) => `
    <article class="card">
      <span class="badge">Scenario ${index + 1}</span>
      <h3>${scenario.title}</h3>
      <p>${scenario.detail}</p>
      <p class="warn">Timing: ${scenario.timing}</p>
      <p>${scenario.effect}</p>
      <button data-arm="${scenario.id}">Arm Scenario</button>
    </article>
  `).join('');

  qa('[data-arm]').forEach(button => button.addEventListener('click', () => armScenario(button.dataset.arm)));
}

function applyOverlay(name) {
  const overlay = state.overlays.find(item => item.name === name);
  state.activeOverlay = overlay.name;
  state.runtime.mutation = state.ethicsAccepted ? 'armed-overlay' : 'disabled';
  state.audit.push(`[OVERLAY] Selected ${overlay.name} at ${overlay.base}.`);
  renderOverlays();
  renderAudit();
}

function renderOverlays() {
  q('#overlay-body').innerHTML = state.overlays.map(overlay => `
    <tr>
      <td>${overlay.name}</td>
      <td>${overlay.base}</td>
      <td>${overlay.purpose}</td>
      <td><button data-overlay="${overlay.name}">Use</button></td>
    </tr>
  `).join('');

  const active = state.overlays.find(item => item.name === state.activeOverlay);
  q('#overlay-detail').textContent = [
    `name: ${active.name}`,
    `base: ${active.base}`,
    `purpose: ${active.purpose}`,
    `sample-bytes: ${active.sample}`,
    `selected-by: jayis1 companion app demo`
  ].join('\n');

  qa('[data-overlay]').forEach(button => button.addEventListener('click', () => applyOverlay(button.dataset.overlay)));
}

function renderCaptures() {
  q('#capture-log').textContent = state.captures.join('\n');
  q('#heatmap').innerHTML = state.overlays.map(overlay => `
    <div class="heat">
      <strong>${overlay.base}</strong>
      <span>${overlay.name}</span>
      <p>${overlay.purpose}</p>
    </div>
  `).join('');
}

function renderAudit() {
  q('#audit-log').textContent = state.audit.join('\n');
}

function exportJson() {
  const blob = new Blob([JSON.stringify(state, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'bootrom-banshee-session.json';
  a.click();
  URL.revokeObjectURL(url);
}

function bindTabs() {
  qa('.tab').forEach(tab => tab.addEventListener('click', () => {
    qa('.tab').forEach(node => node.classList.remove('active'));
    qa('.screen').forEach(node => node.classList.remove('active'));
    tab.classList.add('active');
    q(`#${tab.dataset.screen}`).classList.add('active');
  }));
}

function bindSafety() {
  q('#ethics-check').addEventListener('change', (event) => {
    state.ethicsAccepted = event.target.checked;
    q('#ethics-status').textContent = state.ethicsAccepted
      ? 'Acknowledged. Active scenario arming is now unlocked for authorized work.'
      : 'Active manipulation is locked until acknowledged.';
    state.audit.push(state.ethicsAccepted ? '[ETHICS] Authorized-use gate acknowledged.' : '[ETHICS] Authorized-use gate cleared.');
    renderAudit();
  });

  q('#current-limit').addEventListener('input', (event) => {
    state.currentLimit = Number(event.target.value);
    q('#current-limit-label').textContent = `${state.currentLimit} mA`;
    state.audit.push(`[SAFE] Current limit set to ${state.currentLimit} mA.`);
    renderAudit();
  });

  q('#abort-btn').addEventListener('click', () => {
    state.runtime.mutation = 'disabled';
    state.banner = 'Scenario aborted. Device returned to passive proxy mode.';
    state.audit.push('[ABORT] Scenario aborted by operator. Passive mode restored.');
    renderOverview();
    renderAudit();
  });
}

function bindToolbar() {
  q('#export-json').addEventListener('click', exportJson);
  q('#replay-demo').addEventListener('click', () => {
    state.captures.push('[00.600] DEMO replay -> overlay compare complete');
    state.audit.push('[REPLAY] Demo capture replay executed.');
    renderCaptures();
    renderAudit();
  });
}

renderOverview();
renderScenarios();
renderOverlays();
renderCaptures();
renderAudit();
bindTabs();
bindSafety();
bindToolbar();
