# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""CS1 snapshot invariants, mirrored from the C encoder's validity gate."""
FIELDS = ('now levels valid ready halted faults depth top pushes pops matched abandoned '
          'mismatch underflow overflow collision unstable gaps missed peak tx_fail').split()
COUNTERS = FIELDS[8:]
FAULTS = ('return tag mismatch','return without entry','nesting overflow',
          'overlapping operations','tag setup','acquisition gap','counter exhaustion','watchdog startup')
def validate(s):
    if s['levels'] > 15 or s['valid'] > 15 or s['ready'] > 1 or s['halted'] > 1:
        raise ValueError('state bounds')
    if s['depth'] > 8 or s['peak'] > 8 or s['depth'] > s['peak']:
        raise ValueError('stack bounds')
    if s['top'] > 3 or (not s['depth'] and s['top']):
        raise ValueError('top bounds')
    if s['faults'] > 255 or (s['ready'] and (s['valid'] != 15 or s['halted'] or s['faults'])):
        raise ValueError('ready without valid acquisition')
    if not s['ready'] and s['depth']:
        raise ValueError('pending stack outside ready segment')
    if s['matched'] + s['abandoned'] + s['depth'] != s['pushes']:
        raise ValueError('entry conservation')
    if s['matched'] + s['mismatch'] + s['underflow'] > s['pops']:
        raise ValueError('return conservation')
    if s['overflow'] > s['pushes'] or s['peak'] > s['pushes']:
        raise ValueError('entry bounds')
    if sum(s[k] for k in ('collision','unstable','overflow','mismatch','underflow')) > 1:
        raise ValueError('more than one terminal operation fault')
    if s['missed'] < s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')
    if bool(s['halted']) != bool(s['faults'] & 192):
        raise ValueError('halt cause')
    for i,k in enumerate(('mismatch','underflow','overflow','collision','unstable','gaps')):
        if bool(s[k]) != bool(s['faults'] & (1 << i)):
            raise ValueError('sticky fault disagreement')
