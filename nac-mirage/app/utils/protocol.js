// NAC Mirage operator protocol helpers
// Author: jayis1

export const AUTHOR = 'jayis1';

export const demoStatus = {
  profile: 'staged-relay',
  uptimeMs: 4000,
  framesSeen: 10,
  framesMutated: 4,
  framesDropped: 2,
  alerts: 2,
  observedVlan: 120,
  advertisedVlan: 222,
  poeBudgetMw: 7000,
  radioConnected: true,
  bypassEngaged: false,
};

export const profiles = [
  {
    id: 'transparent',
    title: 'Transparent Observe',
    risk: 'Low',
    summary: 'Records LLDP-MED, PoE draw, and EAPOL timing without altering traffic.',
  },
  {
    id: 'voice-vlan-decoy',
    title: 'Voice VLAN Decoy',
    risk: 'Medium',
    summary: 'Advertises handset-friendly VLAN and power values to study NAC and provisioning drift.',
  },
  {
    id: 'nac-delay',
    title: 'NAC Delay Relay',
    risk: 'High',
    summary: 'Temporarily stalls EAPOL and endpoint link-up to expose race conditions.',
  },
  {
    id: 'staged-relay',
    title: 'Staged Relay',
    risk: 'High',
    summary: 'Combines LLDP mutation, delayed relay opening, and PoE budget shaping.',
  },
];

export const eventFeed = [
  { id: '1', ts: '00:00.400', tag: 'LLDP', text: 'Switch advertisement parsed with voice VLAN 310 and dot1x requirement.' },
  { id: '2', ts: '00:00.800', tag: 'DROP', text: 'Intercepted switch-originated EAPOL request per staged-relay policy.' },
  { id: '3', ts: '00:01.200', tag: 'DHCP', text: 'Endpoint requested voice handset address class before relay fully opened.' },
  { id: '4', ts: '00:02.000', tag: 'POE', text: 'PoE budget reduced after simulated peak draw threshold crossed.' },
  { id: '5', ts: '00:03.600', tag: 'ALERT', text: 'Observed SIP registration opportunity on mirrored traffic.' },
];

export const safetyChecklist = [
  'Use only on infrastructure you are explicitly authorized to test.',
  'Confirm fail-open bypass is physically validated before live insertion.',
  'Coordinate with network defenders before manipulating PoE budgets or VLAN advertisements.',
  'Never deploy against emergency phones, life-safety controllers, or production medical devices.',
];
