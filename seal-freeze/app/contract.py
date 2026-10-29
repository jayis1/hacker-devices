# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""SF1 snapshot contract: independent checks, never authentication."""
FIELDS = ('now levels valid halted faults gaps missed state captured seals resets '
          'rejected coverage mismatch unsealed t0 t1 e0 e1 c0 c1 x0 x1 tx_fail').split()
COUNTERS = 'gaps missed seals resets rejected coverage t0 t1 e0 e1 x0 x1 tx_fail'.split()
FAULTS = ['configuration changed', 'seal withdrawn', 'sample gap', 'counter exhausted',
          'watchdog unavailable', 'rejected sealing']

def validate(s):
    def require(condition):
        if not condition:
            raise ValueError('impossible SF1 snapshot')
    require(all(isinstance(s[k],int) and 0 <= s[k] <= 0xffffffff for k in FIELDS))
    require(s['levels'] <= 15 and s['valid'] <= 15 and s['faults'] <= 63)
    require(s['state'] <= 2 and s['captured'] <= 3 and s['halted'] <= 1)
    require(bool(s['halted']) == bool(s['faults'] & 24))
    require(not s['halted'] or not (s['valid'] or s['state']))
    require(not s['state'] or (s['valid'] == 15 and not s['levels'] & 8 and s['resets']))
    require(s['state'] == 2 or not s['captured'])
    require(s['state'] != 2 or (s['seals'] and s['coverage']))
    require(s['seals'] <= s['resets'] and s['seals'] <= s['coverage'])
    require(not s['coverage'] or s['seals'])
    require(s['missed'] >= s['gaps'] and (s['gaps'] or not s['missed']))
    require(bool(s['gaps']) == bool(s['faults'] & 4))
    require(bool(s['rejected']) == bool(s['faults'] & 32))
    mismatch = s['state'] == 2 and s['levels'] & 3 != s['captured']
    unsealed = s['state'] == 2 and not s['levels'] & 4
    require(s['mismatch'] == int(mismatch) and s['unsealed'] == int(unsealed))
    for i, active in enumerate([mismatch,unsealed]):
        t,e,c,x = (s[f'{k}{i}'] for k in 'tecx')
        require(bool(e) == bool(t) == bool(x))
        require(bool(c) == active)
        require(e <= t <= s['coverage'] and c <= x <= t)
        require(not e or x + e - 1 <= t)
        require(t <= e*x)
        require(e != 1 or (x == t and (not active or c == t)))
        require(bool(t) == bool(s['faults'] & (1<<i)))
