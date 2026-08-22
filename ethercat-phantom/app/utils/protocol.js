// EtherCAT Phantom app protocol helpers
// Author: jayis1

export const AUTHOR = 'jayis1';

export const sampleDevice = {
  name: 'EtherCAT Phantom',
  status: 'linked',
  cycleBudgetNs: 700,
  timingMarginNs: 190,
  batteryMv: 3988,
  safeMode: false,
};

export const sampleRules = [
  { id: 'r1', label: 'Clamp conveyor', object: '0x7000:01', action: 'Clamp 0..1400', enabled: true },
  { id: 'r2', label: 'Derate heater', object: '0x7000:02', action: 'Offset -10', enabled: true },
  { id: 'r3', label: 'Spoof servo target', object: '0x607A:00', action: 'Force 42000', enabled: true },
];

export const sampleCaptures = [
  { cycle: 1, slave: 2, object: '0x7000:01', before: 1200, after: 1200, modified: false },
  { cycle: 3, slave: 4, object: '0x607A:00', before: 50000, after: 42000, modified: true },
  { cycle: 7, slave: 7, object: '0x7010:01', before: 0, after: 1, modified: true },
  { cycle: 12, slave: 5, object: '0x7020:01', before: 82, after: 82, modified: false },
];
