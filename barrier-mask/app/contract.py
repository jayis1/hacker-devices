# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Public BM1 state invariants; CRC alone cannot establish realizability."""
FIELDS = ('now levels valid ready halted faults mask active age rounds arrivals releases '
          'accepted completed abandoned discarded duplicate early collision unstable timeouts '
          'gaps missed tx_fail').split()
COUNTERS = FIELDS[9:]
FAULTS = ('duplicate participant', 'early release', 'round timeout', 'overlapping strobes',
          'tag setup', 'acquisition gap', 'counter exhaustion', 'watchdog startup')
def validate(s):
    if s['levels'] > 15 or s['valid'] > 15 or s['mask'] > 15:
        raise ValueError('mask bounds')
    if any(s[k] > 1 for k in ('ready','halted','active')) or s['faults'] > 255:
        raise ValueError('state bounds')
    if s['age'] > 1001 or bool(s['mask']) != bool(s['active']):
        raise ValueError('active membership')
    if s['active'] and (not s['ready'] or s['age'] > 1000):
        raise ValueError('active without ready or overdue')
    if s['ready'] and (s['valid'] != 15 or s['halted'] or s['faults']):
        raise ValueError('ready without valid acquisition')
    if s['completed'] + s['abandoned'] + s['active'] != s['rounds']:
        raise ValueError('round conservation')
    if 4*s['completed'] + s['discarded'] + s['mask'].bit_count() != s['accepted']:
        raise ValueError('participant conservation')
    if s['accepted'] > s['arrivals'] or s['completed'] > s['releases']:
        raise ValueError('edge accounting')
    if not s['abandoned'] <= s['discarded'] <= 4*s['abandoned']:
        raise ValueError('abandoned membership')
    if s['duplicate'] and s['arrivals'] == s['accepted']:
        raise ValueError('missing duplicate attempt')
    if s['early'] and s['releases'] == s['completed']:
        raise ValueError('missing early attempt')
    if max(s['timeouts'],s['duplicate']) > s['abandoned']:
        raise ValueError('fault without active round')
    if sum(s[k] for k in ('duplicate','early','collision','unstable','timeouts')) > 1:
        raise ValueError('more than one terminal operation fault')
    if s['missed'] < s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')
    if bool(s['halted']) != bool(s['faults'] & 192):
        raise ValueError('halt cause')
    for bit,k in ((1,'duplicate'),(2,'early'),(4,'timeouts'),(8,'collision'),(16,'unstable'),(32,'gaps')):
        if bool(s[k]) != bool(s['faults'] & bit):
            raise ValueError('sticky fault disagreement')
