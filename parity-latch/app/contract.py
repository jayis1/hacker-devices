# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""PL1 necessary evidence bounds, shared by the desktop and parser tests."""
FIELDS = ('now levels valid active halted faults captured age starts completed good bad '
          'aborted parity setup hold collisions gaps missed bin0 bin1 bin2 bin3 tx_fail').split()
COUNTERS = FIELDS[8:]
FAULTS = ('odd parity', 'setup interval', 'hold interval', 'overlapping strobes',
          'acquisition gap', 'counter exhaustion', 'watchdog startup')

def validate(s):
    if s['levels'] > 15 or s['valid'] > 15 or s['captured'] > 7 or s['age'] >= 10:
        raise ValueError('measurement bounds')
    if s['active'] > 1 or s['halted'] > 1 or s['faults'] > 127:
        raise ValueError('state bounds')
    if s['good'] + s['bad'] != s['completed']:
        raise ValueError('completion conservation')
    if s['completed'] + s['aborted'] + s['active'] != s['starts']:
        raise ValueError('start conservation')
    if sum(s[f'bin{i}'] for i in range(4)) != s['good']:
        raise ValueError('codeword histogram conservation')
    if not s['active'] and s['age']:
        raise ValueError('inactive timer')
    if s['active'] and (s['valid'] != 15 or s['halted']):
        raise ValueError('active without acquisition')
    for name, bit in [('parity',1), ('setup',2), ('hold',4), ('collisions',8), ('gaps',16)]:
        if bool(s[name]) != bool(s['faults'] & bit):
            raise ValueError('missing sticky fault')
    if s['missed'] < s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')
    if bool(s['halted']) != bool(s['faults'] & 96):
        raise ValueError('halt cause')
    if any(s[k] > s['starts'] for k in ('parity','setup','hold')):
        raise ValueError('more failures than transfers')
    if any(s[k] > s['bad'] + s['aborted'] + s['active'] for k in ('parity','setup','hold')):
        raise ValueError('failure attributed to good transfer')
    if 2 * s['collisions'] > s['aborted']:
        raise ValueError('collision conservation')
    if s['bad'] > s['parity'] + s['setup'] + s['hold']:
        raise ValueError('bad transfer without cause')
