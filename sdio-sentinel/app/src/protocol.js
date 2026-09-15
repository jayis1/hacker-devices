// SDIO Sentinel protocol helpers. Author: jayis1.
export const PROTOCOL_VERSION = 1;
export const SCOPE_STATEMENT = 'I HAVE AUTHORIZATION FOR THIS SD/SDIO TARGET';

export const Modes = Object.freeze({
  SAFE_BYPASS: 0,
  PASSIVE_OBSERVE: 1,
  ENFORCE_POLICY: 2,
  LAB_EMULATION: 3,
  FAULT: 4,
});

export const modeName = (mode) => [
  'Safe bypass', 'Passive observe', 'Policy enforcement', 'Lab emulation', 'Fault',
][mode] ?? 'Unknown';

export function commandName(command) {
  const commands = {
    0: 'GO_IDLE_STATE', 5: 'IO_SEND_OP_COND', 8: 'SEND_IF_COND',
    11: 'VOLTAGE_SWITCH', 13: 'SEND_STATUS', 17: 'READ_SINGLE_BLOCK',
    18: 'READ_MULTIPLE_BLOCK', 24: 'WRITE_BLOCK', 25: 'WRITE_MULTIPLE_BLOCK',
    27: 'PROGRAM_CSD', 32: 'ERASE_WR_BLK_START', 33: 'ERASE_WR_BLK_END',
    38: 'ERASE', 42: 'LOCK_UNLOCK', 52: 'IO_RW_DIRECT', 53: 'IO_RW_EXTENDED',
  };
  return commands[command] ?? `CMD${command}`;
}

export function decodeStatus(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < 28) {
    throw new Error('Status payload must contain 28 bytes');
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return {
    boardId: view.getUint32(0, true),
    uptimeMs: view.getUint32(4, true),
    framesSeen: view.getUint32(8, true),
    blocked: view.getUint32(12, true),
    anomalies: view.getUint32(16, true),
    clockHz: view.getUint32(20, true),
    mode: bytes[24],
    cardState: bytes[25],
    armed: bytes[26] === 1,
    cardPresent: bytes[27] === 1,
  };
}

export function validateRule(rule) {
  if (!Number.isInteger(rule.command) || rule.command < 0 || rule.command > 63) {
    return 'Command must be an integer from 0 through 63';
  }
  if (!['log', 'block', 'require-arm'].includes(rule.action)) {
    return 'Unknown policy action';
  }
  if (!/^0x[0-9a-f]{8}$/i.test(rule.argumentMask)) {
    return 'Argument mask must be eight hexadecimal digits prefixed by 0x';
  }
  return null;
}

export function exportAudit(entries) {
  const safe = entries.map(({ rawData, ...entry }) => ({
    ...entry,
    exportedBy: 'jayis1',
    rawDataIncluded: false,
  }));
  return JSON.stringify({ format: 'sdio-sentinel-audit-v1', entries: safe }, null, 2);
}
