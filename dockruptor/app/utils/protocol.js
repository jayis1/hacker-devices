// Dockruptor protocol utilities
// Author: jayis1
// SPDX-License-Identifier: MIT

export const buildMockSession = () => ({
  metrics: {
    contract: '9V @ 2A',
    hostMode: 'USB fallback pending DP alt-mode release',
    battery: '84%',
    cable: 'Passive USB2 spoof profile',
    bypass: 'No',
    temperature: '36.8C',
  },
  policy: {
    powerClamp: true,
    altModeDelayTicks: 5,
    chargeOnly: false,
    cableSpoof: 'Passive USB2 cable, no alt-mode assurance',
    softResetTick: 9,
  },
  identities: {
    host: 'Secure laptop host port',
    dock: 'Managed dock with DP alt-mode',
    cableOriginal: 'USB4 active cable with eMarker',
    cableSpoofed: 'Passive USB2 cable, no alt-mode assurance',
    svids: ['DisplayPort', 'Vendor Dock Mode'],
  },
  safety: {
    relayBypass: false,
    safeMode: false,
    acknowledged: false,
    advisory:
      'Authorized use only. If the endpoint becomes unstable or the battery reserve falls below the red threshold, assert bypass immediately.',
  },
  events: [
    { id: 'evt-1', type: 'policy', direction: 'internal', summary: 'DisplayPort alt-mode release delayed until tick 5', time: '09:31:10' },
    { id: 'evt-2', type: 'pd', direction: 'dock>host', summary: 'Discover Identity VDM includes managed dock certificate tag', time: '09:31:05' },
    { id: 'evt-3', type: 'power', direction: 'internal', summary: 'Battery 84%, temp 36.8C, relay bypass inactive', time: '09:31:03' },
    { id: 'evt-4', type: 'pd', direction: 'host>dock', summary: 'Soft_Reset injected for recovery behavior testing', time: '09:30:58' },
    { id: 'evt-5', type: 'info', direction: 'internal', summary: 'Authorized conference-room dock engagement loaded', time: '09:30:55' },
  ],
});
