// data.js - EDID Phantom mobile companion data model
// Author: jayis1
// Copyright (c) 2026 jayis1
export const AUTHOR = 'jayis1';

export const profiles = [
  {
    key: 'conference-mirror',
    title: 'Conference Mirror',
    summary: 'Inline HDMI reconnaissance with guarded HPD pulses and adaptive EDID shaping.',
    risk: 'Medium',
  },
  {
    key: 'kiosk-reality-check',
    title: 'Kiosk Reality Check',
    summary: 'Emulated sink mode for kiosk validation, display spoofing research, and rollback testing.',
    risk: 'High',
  },
  {
    key: 'boardroom-observer',
    title: 'Boardroom Observer',
    summary: 'Passive sniff-only mode for low-disturbance conference-room AV telemetry collection.',
    risk: 'Low',
  },
  {
    key: 'training-lab-sandbox',
    title: 'Training Lab Sandbox',
    summary: 'Aggressive but rate-limited scenario for lab-only CEC and HPD orchestration exercises.',
    risk: 'High',
  },
];

export const captureFeed = [
  { time: '09:42:11', bus: 'DDC', detail: '0x50 read offset 0x36 len 0x10 checksum OK' },
  { time: '09:42:14', bus: 'CEC', detail: 'Opcode 0x82 Active Source from logical address 0x1' },
  { time: '09:42:19', bus: 'Policy', detail: 'Vendor block preserved, HDR descriptor modified to test fallback' },
  { time: '09:42:26', bus: 'HPD', detail: 'Pulse window armed with 80 ms spacing and rollback timer' },
  { time: '09:42:33', bus: 'Analytics', detail: 'Source retrained TMDS clock after EDID luminance change' },
];

export const cecActions = [
  {
    title: 'Guarded Active Source',
    detail: 'Sends opcode 0x82 only when target policy enables injected CEC traffic.',
  },
  {
    title: 'Display Power Snapshot',
    detail: 'Polls opcode 0x90 response trends to observe display wake/sleep state changes.',
  },
  {
    title: 'Soft-Key Navigation Macro',
    detail: 'Lab-only sequence that tests kiosk menu lockouts without touching DDC signaling.',
  },
];

export function buildMutationReport(profileKey, toggles) {
  const enabled = Object.entries(toggles)
    .filter(([, value]) => value)
    .map(([key]) => key)
    .join(', ');

  const severity = toggles.spoofHdr || toggles.injectLatency ? 'elevated' : 'guarded';
  const seed = profileKey.split('').reduce((sum, char) => sum + char.charCodeAt(0), 0) +
    Object.values(toggles).filter(Boolean).length * 17;

  return {
    profileKey,
    enabled,
    severity,
    seed: `0x${seed.toString(16).toUpperCase()}`,
    preview: [
      'CTA extension hints altered to study source parser resilience.',
      toggles.preserveVendor ? 'Vendor-specific block pinned to reduce display breakage.' : 'Vendor-specific block released for full adversarial fuzzing.',
      toggles.injectLatency ? 'DDC response shaping adds sub-millisecond jitter to emulate marginal cabling.' : 'DDC timing remains stable and reference-like.',
    ],
  };
}
