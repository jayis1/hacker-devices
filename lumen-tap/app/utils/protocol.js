// lumen-tap/app/utils/protocol.js
// LumenTap Wire Protocol — JSON-over-CDC for device control + status.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

/**
 * Command builder helpers. Each returns a JSON string terminated by \n
 * that the firmware's CDC interpreter understands.
 */
export const Commands = {
  // Laser
  setPower:   (duty)        => JSON.stringify({cmd:'set_power',  duty}) + '\n',
  arm:        ()             => JSON.stringify({cmd:'arm'}) + '\n',
  disarm:     ()             => JSON.stringify({cmd:'disarm'}) + '\n',

  // DSP
  setDemod:   (mode)        => JSON.stringify({cmd:'set_demod',  mode}) + '\n',
  setBandpass:(low, high)   => JSON.stringify({cmd:'set_bandpass', low, high}) + '\n',
  setNoise:   (depth)       => JSON.stringify({cmd:'set_noise',  depth}) + '\n',
  setBypass:  (on)          => JSON.stringify({cmd:'set_bypass', on}) + '\n',

  // SD logging
  recordStart:(operator, targetId, note) =>
                  JSON.stringify({cmd:'record_start', operator, target_id:targetId, note}) + '\n',
  recordStop: ()             => JSON.stringify({cmd:'record_stop'}) + '\n',

  // Misc
  status:     ()             => JSON.stringify({cmd:'status'}) + '\n',
};

/**
 * Status message shape (parsed from device → host):
 * {
 *   type:     'status',
 *   tick:     number,
 *   laser:    0|1,        // currently emitting
 *   pwm:      number,     // 0..255 duty (post-cap)
 *   ambient:  number,     // TSL257 count
 *   arm:      0|1,        // HW arm key present
 *   rms_in:   number,     // pre-AGC RMS
 *   rms_out:  number,     // post-DSP RMS
 *   snr_db:   number,     // estimated SNR in dB
 *   demod:    0|1,        // 0=AM, 1=FM
 *   bp_lo:    number,     // Hz
 *   bp_hi:    number,     // Hz
 *   noise:    number,     // 0..1
 *   gain:     number,     // AGC gain
 *   sd_state: number,     // 0 idle, 1 armed, 2 recording, 3 fault
 *   sd_blk:   number      // blocks written
 * }
 */
export function parseStatus(line) {
  try {
    const obj = JSON.parse(line.trim());
    if (obj.type !== 'status') return null;
    return obj;
  } catch (e) {
    return null;
  }
}

/**
 * Demodulation mode labels.
 */
export const DEMOD_LABELS = ['AM (envelope)', 'FM (quadrature)'];

/**
 * SD log state labels.
 */
export const SD_STATE_LABELS = ['Idle', 'Armed', 'Recording', 'Fault'];

/**
 * Safety verdict from a status object.
 * Returns { canEmit, reasons[] }.
 */
export function safetyVerdict(s) {
  if (!s) return { canEmit: false, reasons: ['no status'] };
  const reasons = [];
  if (!s.arm)            reasons.push('arm key absent');
  if (s.ambient >= 6000) reasons.push('ambient unsafe');
  if (s.pwm === 0)       reasons.push('power is zero');
  if (s.sd_state === 3)  reasons.push('SD fault');
  return { canEmit: reasons.length === 0, reasons };
}

/**
 * Convert a linear 0..255 PWM duty into a milliwatt estimate at 520 nm
 * given a 5 mW max diode current. (Class 1 cap = 0.39 mW.)
 */
export function dutyToMw(duty) {
  const maxMw = 5.0;
  return (duty / 255.0) * maxMw;
}

/**
 * Format an on-time in ms as M:SS.
 */
export function formatOnTime(ms) {
  const sec = Math.floor(ms / 1000);
  const m = Math.floor(sec / 60);
  const s = sec % 60;
  return `${m}:${s.toString().padStart(2, '0')}`;
}