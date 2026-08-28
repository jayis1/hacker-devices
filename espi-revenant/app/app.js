// app.js - eSPI Revenant companion app
// Author: jayis1
const AUTHOR = 'jayis1';

const profiles = [
  {
    id: 'resume-glitch-window',
    title: 'Resume Glitch Window',
    effect: 'Bounded flash completion delay during resume-trigger sequence.',
    risk: 'Medium',
  },
  {
    id: 'vw-contradiction-lab',
    title: 'VW Contradiction Lab',
    effect: 'One contradictory HOST_RST_WARN pulse after valid wake progression.',
    risk: 'Medium',
  },
  {
    id: 'ec-maintenance-probe',
    title: 'EC Maintenance Probe',
    effect: 'Replays maintenance-style peripheral transactions in a narrow window.',
    risk: 'High',
  },
];

const telemetry = {
  mode: 'Passive Mirror',
  activeProfile: 'resume-glitch-window',
  armed: false,
  stats: [
    { label: 'Target Voltage', value: '1.79 V' },
    { label: 'EC Current', value: '141 mA' },
    { label: 'Board Temp', value: '41.5 C' },
    { label: 'Anomaly Score', value: '37' },
  ],
  traces: [
    { ts: '00:00.105', channel: 'vwire', text: 'SLP_S3 cleared, resume window opened, awake=1', note: 'Baseline trace by jayis1' },
    { ts: '00:00.110', channel: 'flash', text: 'Read-ID completion delayed by 96 ns in bounded profile', note: 'Guarded active profile' },
    { ts: '00:00.115', channel: 'peripheral', text: 'Port 0x61 observed with maintenance hint absent', note: 'No replay yet' },
    { ts: '00:00.120', channel: 'oob', text: 'Capabilities advertised: vwire + flash injection enabled', note: 'Control-plane mirror' },
  ],
  safety: [
    'Written authorization collected and validated.',
    'Spare motherboard or bench target prepared.',
    'Passive baseline captured before active mode.',
    'Watchdog and rollback path verified.',
    'Operator understands that sideband manipulation can corrupt firmware state.',
  ],
};

const overviewStats = document.getElementById('overviewStats');
const profilesRoot = document.getElementById('profiles');
const traceFeed = document.getElementById('traceFeed');
const safetyList = document.getElementById('safetyList');
const statusBadge = document.getElementById('statusBadge');
const armButton = document.getElementById('armButton');
const injectButton = document.getElementById('injectButton');
const rollbackButton = document.getElementById('rollbackButton');

function renderStats() {
  overviewStats.innerHTML = '';
  telemetry.stats.forEach((stat) => {
    const card = document.createElement('article');
    card.className = 'stat-card';
    card.innerHTML = `<div class="stat-label">${stat.label}</div><div class="stat-value">${stat.value}</div>`;
    overviewStats.appendChild(card);
  });
}

function renderProfiles() {
  profilesRoot.innerHTML = '';
  profiles.forEach((profile) => {
    const card = document.createElement('article');
    card.className = 'profile-card';
    const active = telemetry.activeProfile === profile.id;
    card.innerHTML = `
      <h3>${profile.title}</h3>
      <p>${profile.effect}</p>
      <div class="profile-meta">Risk: ${profile.risk} · ${active ? 'Selected' : 'Available'} · Author: ${AUTHOR}</div>
      <button data-profile="${profile.id}">${active ? 'Selected Profile' : 'Load Profile'}</button>
    `;
    card.querySelector('button').addEventListener('click', () => {
      telemetry.activeProfile = profile.id;
      telemetry.mode = telemetry.armed ? 'Guarded Trigger Mode' : 'Passive Mirror';
      telemetry.traces.unshift({
        ts: new Date().toLocaleTimeString(),
        channel: 'control',
        text: `Profile changed to ${profile.id}`,
        note: `Operator action by ${AUTHOR}`,
      });
      renderProfiles();
      renderTrace();
    });
    profilesRoot.appendChild(card);
  });
}

function renderTrace() {
  traceFeed.innerHTML = '';
  telemetry.traces.forEach((item) => {
    const row = document.createElement('article');
    row.className = 'trace-item';
    row.innerHTML = `<strong>${item.ts}</strong> · <span>${item.channel}</span><p>${item.text}</p><small>${item.note}</small>`;
    traceFeed.appendChild(row);
  });
  statusBadge.textContent = telemetry.mode;
}

function renderSafety() {
  safetyList.innerHTML = '';
  telemetry.safety.forEach((item) => {
    const li = document.createElement('li');
    li.textContent = item;
    safetyList.appendChild(li);
  });
}

armButton.addEventListener('click', () => {
  telemetry.armed = !telemetry.armed;
  telemetry.mode = telemetry.armed ? 'Guarded Trigger Mode' : 'Passive Mirror';
  telemetry.traces.unshift({
    ts: new Date().toLocaleTimeString(),
    channel: 'control',
    text: telemetry.armed ? 'Active mode armed with hardware switch confirmation.' : 'Interposer returned to passive mirror mode.',
    note: `Safety state updated by ${AUTHOR}`,
  });
  renderTrace();
});

injectButton.addEventListener('click', () => {
  telemetry.traces.unshift({
    ts: new Date().toLocaleTimeString(),
    channel: 'vwire',
    text: telemetry.armed
      ? 'Simulated HOST_RST_WARN contradiction pulse queued under bounded rules.'
      : 'Injection blocked because the device is not armed.',
    note: telemetry.armed ? 'Profile-enforced single-shot event' : 'Passive-only guard active',
  });
  renderTrace();
});

rollbackButton.addEventListener('click', () => {
  telemetry.armed = false;
  telemetry.mode = 'Fail-Safe Bypass';
  telemetry.traces.unshift({
    ts: new Date().toLocaleTimeString(),
    channel: 'safety',
    text: 'Watchdog rollback asserted; direct host-to-EC path restored.',
    note: `Rollback record generated by ${AUTHOR}`,
  });
  renderTrace();
});

renderStats();
renderProfiles();
renderTrace();
renderSafety();
