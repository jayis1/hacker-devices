/*
 * MCTP Wraith companion app logic
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
const state = {
  armed: false,
  safeMode: true,
  scenario: 'spdm-downgrade-probe',
  telemetry: {
    captures: 6,
    mutations: 0,
    alerts: 0,
    signedBundles: 0,
    currentMa: 142,
    tempC: 29
  },
  endpoints: [
    { eid: 8, name: 'host-root-complex', medium: 'i3c-backplane', trust: 'high' },
    { eid: 20, name: 'bmc-security-agent', medium: 'i3c-backplane', trust: 'high' },
    { eid: 33, name: 'retimer-1', medium: 'smbus-sideband', trust: 'medium' },
    { eid: 44, name: 'nic-management', medium: 'ncsi-bridge', trust: 'medium' },
    { eid: 52, name: 'nvme-enclosure', medium: 'pcie-vdm', trust: 'low' }
  ],
  routes: [
    { src: 8, dst: 20, hop: 20, ttl: 8 },
    { src: 8, dst: 33, hop: 20, ttl: 6 },
    { src: 8, dst: 44, hop: 20, ttl: 6 },
    { src: 20, dst: 52, hop: 52, ttl: 6 }
  ]
};

const statusBox = document.getElementById('statusBox');
const telemetryCards = document.getElementById('telemetryCards');
const endpointTable = document.getElementById('endpointTable');
const routeTable = document.getElementById('routeTable');
const bundleBox = document.getElementById('bundleBox');
const scenarioSelect = document.getElementById('scenario');

function renderStatus() {
  statusBox.textContent = [
    `scenario: ${state.scenario}`,
    `armed: ${state.armed}`,
    `safeMode: ${state.safeMode}`,
    `captures: ${state.telemetry.captures}`,
    `mutations: ${state.telemetry.mutations}`,
    `alerts: ${state.telemetry.alerts}`
  ].join('\n');
}

function renderTelemetry() {
  const items = [
    ['Captures', state.telemetry.captures],
    ['Mutations', state.telemetry.mutations],
    ['Alerts', state.telemetry.alerts],
    ['Signed', state.telemetry.signedBundles],
    ['Current mA', state.telemetry.currentMa],
    ['Temp C', state.telemetry.tempC]
  ];
  telemetryCards.innerHTML = items.map(([label, value]) => `
    <div class="card">
      <span>${label}</span>
      <strong>${value}</strong>
    </div>`).join('');
}

function renderEndpoints() {
  endpointTable.innerHTML = state.endpoints.map((ep) => `
    <tr>
      <td>${ep.eid}</td>
      <td>${ep.name}</td>
      <td>${ep.medium}</td>
      <td>${ep.trust}</td>
    </tr>`).join('');
}

function renderRoutes() {
  routeTable.innerHTML = state.routes.map((route) => `
    <tr>
      <td>${route.src}</td>
      <td>${route.dst}</td>
      <td>${route.hop}</td>
      <td>${route.ttl}</td>
    </tr>`).join('');
}

function updateScenario() {
  state.scenario = scenarioSelect.value;
  renderStatus();
}

function armScenario() {
  state.armed = true;
  state.safeMode = false;
  state.telemetry.mutations += 1;
  state.telemetry.signedBundles += 1;
  renderAll();
}

function triggerScenario() {
  if (!state.armed) {
    statusBox.textContent += '\ntrigger blocked: arm required';
    return;
  }
  state.telemetry.captures += 3;
  state.telemetry.currentMa += 8;
  state.telemetry.tempC += 2;
  if (state.scenario === 'route-poison') {
    state.routes[1].hop = 44;
  }
  if (state.scenario === 'sensor-ghost' || state.scenario === 'endpoint-clone') {
    state.telemetry.alerts += 1;
  }
  renderAll();
}

function safeMode() {
  state.armed = false;
  state.safeMode = true;
  renderAll();
}

function poisonRoute() {
  state.routes[1].hop = 44;
  state.telemetry.alerts += 1;
  renderAll();
}

function restoreRoute() {
  state.routes[1].hop = 20;
  renderAll();
}

function generateBundle() {
  const bundle = {
    author: 'jayis1',
    device: 'MCTP Wraith',
    authorized_use_only: true,
    scenario: state.scenario,
    timestamp_hint: 'generated locally in browser',
    telemetry: state.telemetry,
    endpoints: state.endpoints,
    routes: state.routes
  };
  bundleBox.value = JSON.stringify(bundle, null, 2);
}

async function copyBundle() {
  if (!bundleBox.value) generateBundle();
  await navigator.clipboard.writeText(bundleBox.value);
  statusBox.textContent += '\ncopy: bundle JSON copied';
}

function renderAll() {
  renderStatus();
  renderTelemetry();
  renderEndpoints();
  renderRoutes();
}

document.getElementById('armBtn').addEventListener('click', armScenario);
document.getElementById('triggerBtn').addEventListener('click', triggerScenario);
document.getElementById('safeBtn').addEventListener('click', safeMode);
document.getElementById('poisonRouteBtn').addEventListener('click', poisonRoute);
document.getElementById('restoreRouteBtn').addEventListener('click', restoreRoute);
document.getElementById('bundleBtn').addEventListener('click', generateBundle);
document.getElementById('copyBtn').addEventListener('click', copyBundle);
scenarioSelect.addEventListener('change', updateScenario);

renderAll();
generateBundle();
