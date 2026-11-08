# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""AD1 necessary snapshot/transition checks, not a full hidden-history proof."""
FIELDS = ('now levels valid halted faults phase active depth stable observed rounds '
          'attempts launched completed rejected gaps missed queued peak '
          'use0 use1 use2 use3 stamp0 stamp1 stamp2 stamp3 ref0 ref1 ref2 ref3 tx_fail').split()
COUNTERS = FIELDS[9:23] + ['tx_fail']
FAULTS = ['reference ceiling exceeded', 'reference underflow', 'reserved',
          'interface/setup ambiguity', 'acquisition gap', 'counter exhaustion', 'watchdog startup']
POLICY = [3, 1, 5, 4, 3, 1]


def references(s):
    return [s['ref'+str(i)] for i in range(4)]


def eligible(s):
    return [i for i, n in enumerate(references(s)) if n < 3]


def queue(s):
    return [(i, s['use'+str(i)]) for i in range(4) if s['active'] & (1 << i)]


def validate(s):
    if set(s) != set(FIELDS) or any(type(s[k]) is not int or not 0 <= s[k] <= 0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels'] > 15 or s['valid'] > 15 or s['faults'] > 127 or s['faults'] & 4:
        raise ValueError('masks')
    if s['phase'] > 1 or s['active'] > 15 or s['stable'] > 5 or s['halted'] > 1:
        raise ValueError('state bounds')
    if bool(s['halted']) != bool(s['faults']) or s['faults'] & (s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid'] != 15:
        raise ValueError('baseline qualification')
    refs = references(s)
    if any(n > 3 or n > s['use'+str(i)] for i, n in enumerate(refs)):
        raise ValueError('per-object reference bounds')
    if s['active'] != sum((1 << i) for i, n in enumerate(refs) if n):
        raise ValueError('live object membership')
    if s['depth'] != sum(refs):
        raise ValueError('reference total')
    stamps = [s['stamp'+str(i)] for i in range(4)]
    seen = [n for n in stamps if n]
    if len(seen) != len(set(seen)) or max(stamps) != s['launched']:
        raise ValueError('last ordinal ownership')
    tag = s['levels'] & 3
    if s['faults'] == 1 and refs[tag] != 3:
        raise ValueError('ceiling finding requires three retained references')
    if s['faults'] == 2 and refs[tag]:
        raise ValueError('underflow finding requires zero retained references')
    for i, n in enumerate(stamps):
        use = s['use'+str(i)]
        if bool(n) != bool(use) or use > n:
            raise ValueError('retain history')
    if not s['depth'] <= s['peak'] <= 12 or bool(s['peak']) != bool(s['launched']) or s['peak'] > s['launched']:
        raise ValueError('peak bounds')
    if (s['queued'] and s['peak'] < 2) or (s['peak'] > 1 and s['queued'] < s['peak']-1):
        raise ValueError('overlap coverage')
    if sum(s['use'+str(i)] for i in range(4)) != s['launched']:
        raise ValueError('retain conservation')
    if s['completed'] + s['depth'] != s['launched']:
        raise ValueError('reference conservation')
    if s['rounds'] + bool(s['active']) + s['queued'] != s['launched'] or s['rounds'] > s['completed']:
        raise ValueError('pool episode conservation')
    if s['launched'] + s['completed'] + s['rejected'] != s['attempts']:
        raise ValueError('attempt conservation')
    if not s['phase'] and s['attempts']:
        raise ValueError('pre-baseline attempts')
    if s['attempts'] > 2*s['observed']:
        raise ValueError('insufficient observations')
    if s['rejected'] > 2 or bool(s['rejected']) != bool(s['faults'] & 15):
        raise ValueError('rejection finding')
    if s['rejected'] == 2 and s['faults'] != 8:
        raise ValueError('two-edge ambiguity')
    if s['gaps'] > 1 or bool(s['gaps']) != bool(s['faults'] & 16):
        raise ValueError('gap finding')
    if s['missed'] < s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')


def transition(a, b, delta):
    if b['phase'] < a['phase']:
        return 'Baseline disappeared'
    if not b['halted'] and b['valid'] & a['valid'] != a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed'] + b['missed']-a['missed'] > delta:
        return 'Observed and missed growth exceed device time'
    if b['attempts']-a['attempts'] > delta+1:
        return 'Too many operation edges'
    retains = b['launched']-a['launched']
    drops = b['completed']-a['completed']
    changes = [b['use'+str(i)]-a['use'+str(i)] for i in range(4)]
    if any(n < 0 for n in changes) or sum(changes) != retains:
        return 'Cumulative retain history changed without operations'
    released = [n+a['ref'+str(i)]-b['ref'+str(i)] for i, n in enumerate(changes)]
    if any(n < 0 for n in released) or sum(released) != drops:
        return 'Per-object reference changes cannot explain drops'
    if b['peak'] > max(a['peak'], a['depth']+retains):
        return 'Peak growth lacks retains'
    if not retains and b['peak'] != a['peak']:
        return 'Peak changed without retain'
    if not a['depth'] and not b['depth'] and retains and b['rounds'] == a['rounds']:
        return 'Hidden cycle lacks pool drain'
    if b['rounds']-a['rounds'] > drops or b['queued']-a['queued'] > retains:
        return 'Pool episode increments lack corresponding operations'
    for i, n in enumerate(changes):
        old = a['stamp'+str(i)]; new = b['stamp'+str(i)]
        if not n and new != old:
            return 'Last ordinal changed without retain'
        if n and not a['launched']+n <= new <= b['launched']:
            return 'New retain ordinals impossible'
    return ''
