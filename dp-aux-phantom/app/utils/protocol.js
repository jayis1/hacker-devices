// DP AUX Phantom demo protocol model
// Author: jayis1

export const AUTHOR = 'jayis1';

export const demoStatus = {
  profile: 'dock-emulator',
  sinkName: 'Conference Dock Emulator',
  vendor: 'JY1',
  serial: 'MASKED-JAY1',
  linkRate: 'HBR2',
  lanes: 4,
  auxReads: 6,
  auxWrites: 4,
  i2cReads: 2,
  alerts: 1,
  ruleHits: 4,
  hpd: 'stable',
  radio: 'connected',
};

export const profiles = [
  {
    id: 'transparent',
    title: 'Transparent',
    summary: 'Passive telemetry only. Capture AUX, HPD, and EDID without mutation.',
  },
  {
    id: 'edid-mask',
    title: 'EDID Mask',
    summary: 'Replace serial and asset tags to assess host trust decisions bound to display identity.',
  },
  {
    id: 'lt-slowroll',
    title: 'LT Slowroll',
    summary: 'Intentionally slow link-training to characterize driver timeout handling and recovery.',
  },
  {
    id: 'dock-emulator',
    title: 'Dock Emulator',
    summary: 'Present a dock-like sink profile for conference-room and hot-desk attack path testing.',
  },
  {
    id: 'hpd-bounce',
    title: 'HPD Bounce',
    summary: 'Pulse HPD under policy control to trigger re-enumeration and monitor software behavior.',
  },
  {
    id: 'aux-fuzz',
    title: 'AUX Fuzz',
    summary: 'Apply deterministic mutations to low-risk lab traffic for parser hardening research.',
  },
];

export const eventFeed = [
  {
    time: '00:00.050',
    severity: 'info',
    title: 'Sink attached',
    detail: 'USB-C alt-mode entered; sink DPCD rev 0x14 observed.',
  },
  {
    time: '00:00.160',
    severity: 'info',
    title: 'Link rate request intercepted',
    detail: 'Host requested HBR3; profile forced HBR2 for dock emulation.',
  },
  {
    time: '00:00.470',
    severity: 'info',
    title: 'EDID page rewritten',
    detail: 'Dock emulator header injected and monitor serial field replaced.',
  },
  {
    time: '00:00.550',
    severity: 'alert',
    title: 'Sink count probe tagged',
    detail: 'Possible MST topology enumeration; operator note recorded.',
  },
];

export const safetyChecklist = [
  'Use only on systems and displays you are explicitly authorized to assess.',
  'Disable HDCP and content playback before conducting timing or re-enumeration experiments.',
  'Prefer transparent mode first to establish baseline interoperability.',
  'Keep inline power-path bypass closed if the DUT is a production presentation room dock.',
  'Record exact profile, target firmware, and cable orientation for reproducibility.',
];
