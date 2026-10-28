# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""SA1 wire-state invariants. Internal seen mask is not transmitted."""
FIELDS = ('now levels valid ready halted faults pending occupied allocations releases retired abandoned '
          'duplicate orphan reuse collision unstable gaps missed peak tx_fail').split()
COUNTERS = FIELDS[8:]
FAULTS = ('duplicate live slot','release of free slot','reserved',
          'overlapping operations','tag setup','acquisition gap','counter exhaustion','watchdog startup')
def validate(s):
    if s['levels'] > 15 or s['valid'] > 15 or s['ready'] > 1 or s['halted'] > 1:
        raise ValueError('state bounds')
    if s['occupied'] > 15 or s['pending'] != s['occupied'].bit_count():
        raise ValueError('occupancy disagreement')
    if s['pending'] > s['peak'] or s['peak'] > 4:
        raise ValueError('capacity bounds')
    if s['faults'] > 255 or s['faults'] & 4:
        raise ValueError('reserved finding')
    if s['ready'] and (s['valid'] != 15 or s['halted'] or s['faults']):
        raise ValueError('ready without valid acquisition')
    if not s['ready'] and s['pending']:
        raise ValueError('pending outside ready segment')
    if s['retired'] + s['abandoned'] + s['pending'] != s['allocations']:
        raise ValueError('allocation conservation')
    if s['retired'] + s['orphan'] > s['releases']:
        raise ValueError('release conservation')
    if s['duplicate'] > s['abandoned'] or s['peak'] > s['allocations']:
        raise ValueError('allocation bounds')
    if s['reuse'] > s['retired'] or s['retired'] + s['pending'] > s['reuse'] + 4:
        raise ValueError('reuse bounds')
    if sum(s[k] for k in ('collision','unstable','duplicate','orphan')) > 1:
        raise ValueError('more than one terminal operation fault')
    if s['missed'] < s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')
    if bool(s['halted']) != bool(s['faults'] & 192):
        raise ValueError('halt cause')
    for bit,k in ((1,'duplicate'),(2,'orphan'),(8,'collision'),(16,'unstable'),(32,'gaps')):
        if bool(s[k]) != bool(s['faults'] & bit):
            raise ValueError('sticky fault disagreement')
