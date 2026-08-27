// protocol.js - PoE Whisper app protocol model
// Author: jayis1
export const AUTHOR = 'jayis1';

export const profiles = [
  {
    id: 'camera-reboot-window',
    name: 'Camera Reboot Window',
    description: 'Class-4 profile with bounded 120 ms brownout during camera codec initialization.',
    meta: '24.5 W request · LLDP spoof enabled · safe rollback armed',
  },
  {
    id: 'phone-class-downgrade',
    name: 'Phone Class Downgrade',
    description: 'Reduce advertised class and add MPS jitter to observe handset behavior under constrained power.',
    meta: '6.5 W request · LLDP spoof enabled · no voltage sag',
  },
  {
    id: 'badge-reader-mps-jitter',
    name: 'Badge Reader MPS Jitter',
    description: 'Apply bounded keepalive irregularity and short droop for physical-security controller testing.',
    meta: '12.5 W request · MPS jitter enabled · 80 ms droop',
  },
];

export const eventFeed = [
  { ts: '000ms', code: 'BOOT', message: 'PoE Whisper booted in authorized research mode.' },
  { ts: '006ms', code: 'PSE', message: 'Fingerprint matched Cisco Catalyst bt-capable source.' },
  { ts: '030ms', code: 'LLDP', message: 'Captured endpoint power request advertisement at 24.5 W.' },
  { ts: '042ms', code: 'SPOOF', message: 'Injected LLDP identity override with +5.0 W request delta.' },
  { ts: '057ms', code: 'DROOP', message: 'Armed bounded 120 ms brownout to 36.5 V during boot window.' },
  { ts: '072ms', code: 'SIG', message: 'Current signature labeled motor-or-ir-burst, possible camera IR engage.' },
];

export const safetyChecklist = [
  'Operate only with written authorization and an agreed rollback window.',
  'Verify the endpoint is non-life-safety critical before active power manipulation.',
  'Confirm thermal cutoff, bypass relay, and watchdog rollback before insertion.',
  'Record baseline behavior in passive mode prior to enabling profiles.',
  'Do not exceed bounded droop windows or defeat hardware interlocks.',
];

export const buildStatus = (selectedProfile, toggles) => ({
  power: selectedProfile === 'phone-class-downgrade' ? '6.5' : selectedProfile === 'badge-reader-mps-jitter' ? '12.5' : '24.5',
  poeClass: selectedProfile === 'phone-class-downgrade' ? 'Class 2' : selectedProfile === 'badge-reader-mps-jitter' ? 'Class 3' : 'Class 4',
  voltage: toggles.brownout ? '36.5' : '52.0',
  tempMargin: toggles.brownout ? '32' : '37',
  summary: `Profile ${selectedProfile} loaded. LLDP spoof ${toggles.lldpSpoof ? 'active' : 'inactive'}, MPS jitter ${toggles.mpsJitter ? 'active' : 'inactive'}, brownout ${toggles.brownout ? 'armed' : 'off'}.`,
});
