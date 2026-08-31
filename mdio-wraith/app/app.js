// MDIO Wraith companion app
// Author: jayis1
const AUTHOR = 'jayis1';

const state = {
  armed: false,
  mode: 'Passive Mirror',
  activeProfile: 'phy-fingerprint',
  status: [
    { label: 'Author', value: AUTHOR },
    { label: 'Target Voltage', value: '3.29 V' },
    { label: 'Target Current', value: '188.4 mA' },
    { label: 'Board Temp', value: '36.4 C' },
    { label: 'Anomalies', value: '1' },
    { label: 'Mode', value: 'Passive Mirror' },
  ],
  phys: [
    { addr: 0, label: 'uplink-phy', clause45: true, link: true, note: 'Core WAN/uplink management transceiver.' },
    { addr: 1, label: 'mgmt-phy', clause45: true, link: false, note: 'Out-of-band management path candidate.' },
    { addr: 2, label: 'poe-sidecar', clause45: false, link: true, note: 'Power negotiation adjacent PHY.' },
    { addr: 7, label: 'backplane-phy', clause45: true, link: true, note: 'High-speed backplane port.' },
  ],
  profiles: [
    { id: 'phy-fingerprint', risk: 'Low', description: 'Passive PHY census with Clause 22/45 snapshots and vendor-page inventory.' },
    { id: 'isolated-link-drop', risk: 'Medium', description: 'Guarded isolate-bit assertion after link-up detection to validate alerting and failover.' },
    { id: 'loopback-diversion', risk: 'Medium', description: 'One-time loopback and restart-autoneg write for control-plane deception research.' },
    { id: 'strap-shadow', risk: 'High', description: 'Vendor-page and strap-bank shadowing on selected PHYs with strict write budget.' },
  ],
  captures: [
    { ts: '00:00.040', channel: 'mdio', text: 'PHY 0 ID read 0x2000:0xA231', note: 'Baseline discovery by jayis1' },
    { ts: '00:00.070', channel: 'mdio', text: 'PHY 1 ANER=0x0001, Clause 45 path available', note: 'Management sidecar may support extended pages' },
    { ts: '00:00.120', channel: 'power', text: '3.3 V stable, 188.4 mA target draw', note: 'Inline monitor normal' },
  ],
  ethics: [
    'Use only on infrastructure you own or are explicitly authorized to assess.',
    'Capture a passive baseline before any active write profile.',
    'Treat PoE-powered and industrial targets as safety-relevant systems.',
    'Have rollback access to the target switch, router, or controller before using active mode.',
    'Record written authorization in the engagement package.',
  ],
};

const statusGrid = document.getElementById('statusGrid');
const phyTable = document.getElementById('phyTable');
const telemetryList = document.getElementById('telemetryList');
const profilesRoot = document.getElementById('profilesRoot');
const captureFeed = document.getElementById('captureFeed');
const ethicsList = document.getElementById('ethicsList');
const armButton = document.getElementById('armButton');
const rollbackButton = document.getElementById('rollbackButton');
const injectButton = document.getElementById('injectButton');
const registerForm = document.getElementById('registerForm');
const registerResult = document.getElementById('registerResult');

document.querySelectorAll('.tab').forEach((tabButton) => {
  tabButton.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach((node) => node.classList.remove('active'));
    document.querySelectorAll('.panel').forEach((node) => node.classList.remove('active'));
    tabButton.classList.add('active');
    document.getElementById(tabButton.dataset.tab).classList.add('active');
  });
});

function syncStatusMode() {
  state.status = state.status.map((item) => (item.label === 'Mode' ? { ...item, value: state.mode } : item));
}

function renderStatus() {
  statusGrid.innerHTML = '';
  state.status.forEach((item) => {
    const card = document.createElement('article');
    card.className = 'status-card';
    card.innerHTML = `<div class="label">${item.label}</div><div class="value">${item.value}</div>`;
    statusGrid.appendChild(card);
  });
}

function renderPhys() {
  phyTable.innerHTML = '';
  telemetryList.innerHTML = '';
  state.phys.forEach((phy) => {
    const row = document.createElement('article');
    row.className = 'phy-row';
    row.innerHTML = `<strong>PHY ${phy.addr} — ${phy.label}</strong><p>${phy.note}</p><small>Clause 45: ${phy.clause45 ? 'yes' : 'no'} · Link: <span class="${phy.link ? 'tag-ok' : 'tag-warn'}">${phy.link ? 'up' : 'down'}</span> · Author: ${AUTHOR}</small>`;
    phyTable.appendChild(row);

    const item = document.createElement('li');
    item.textContent = `PHY ${phy.addr} ${phy.label}: ${phy.link ? 'link up' : 'link down'}, extended pages ${phy.clause45 ? 'present' : 'absent'}`;
    telemetryList.appendChild(item);
  });
}

function renderProfiles() {
  profilesRoot.innerHTML = '';
  state.profiles.forEach((profile) => {
    const card = document.createElement('article');
    card.className = 'profile-card';
    const selected = state.activeProfile === profile.id;
    card.innerHTML = `
      <h3>${profile.id}</h3>
      <p>${profile.description}</p>
      <small>Risk: ${profile.risk} · Author: ${AUTHOR}</small>
      <div class="hero-actions" style="margin-top:0.75rem;">
        <button data-profile="${profile.id}">${selected ? 'Selected' : 'Load Profile'}</button>
      </div>
    `;
    card.querySelector('button').addEventListener('click', () => {
      state.activeProfile = profile.id;
      state.captures.unshift({
        ts: new Date().toLocaleTimeString(),
        channel: 'control',
        text: `Profile changed to ${profile.id}`,
        note: `Operator selection recorded by ${AUTHOR}`,
      });
      renderProfiles();
      renderCaptures();
    });
    profilesRoot.appendChild(card);
  });
}

function renderCaptures() {
  captureFeed.innerHTML = '';
  state.captures.forEach((capture) => {
    const row = document.createElement('article');
    row.className = 'capture-item';
    row.innerHTML = `<strong>${capture.ts}</strong> · <span>${capture.channel}</span><p>${capture.text}</p><small>${capture.note}</small>`;
    captureFeed.appendChild(row);
  });
}

function renderEthics() {
  ethicsList.innerHTML = '';
  state.ethics.forEach((entry) => {
    const item = document.createElement('li');
    item.textContent = entry;
    ethicsList.appendChild(item);
  });
}

armButton.addEventListener('click', () => {
  state.armed = !state.armed;
  state.mode = state.armed ? 'Guarded Trigger Mode' : 'Passive Mirror';
  syncStatusMode();
  state.captures.unshift({
    ts: new Date().toLocaleTimeString(),
    channel: 'control',
    text: state.armed ? 'Guarded mode armed after authorization check.' : 'Returned to passive mirror mode.',
    note: `Safety state updated by ${AUTHOR}`,
  });
  renderStatus();
  renderCaptures();
});

rollbackButton.addEventListener('click', () => {
  state.armed = false;
  state.mode = 'Fail-Safe Bypass';
  syncStatusMode();
  state.captures.unshift({
    ts: new Date().toLocaleTimeString(),
    channel: 'safety',
    text: 'Bypass asserted and future writes blocked until re-armed.',
    note: `Rollback path invoked by ${AUTHOR}`,
  });
  renderStatus();
  renderCaptures();
});

injectButton.addEventListener('click', () => {
  state.captures.unshift({
    ts: new Date().toLocaleTimeString(),
    channel: 'mdio',
    text: state.armed
      ? `Queued one guarded write under ${state.activeProfile}`
      : 'Write simulation blocked because the device is not armed.',
    note: state.armed ? 'Budget and thermal checks still apply.' : 'Passive safety gate enforced.',
  });
  renderCaptures();
});

registerForm.addEventListener('submit', (event) => {
  event.preventDefault();
  const phy = document.getElementById('phyInput').value;
  const reg = document.getElementById('regInput').value;
  const value = document.getElementById('valueInput').value;
  registerResult.textContent = [
    `author=${AUTHOR}`,
    `profile=${state.activeProfile}`,
    `armed=${state.armed}`,
    `simulated_target=phy${phy}`,
    `register=${reg}`,
    `value=${value}`,
    `decision=${state.armed ? 'staged-for-guarded-execution' : 'blocked-until-armed'}`,
  ].join('\n');
});

syncStatusMode();
renderStatus();
renderPhys();
renderProfiles();
renderCaptures();
renderEthics();
