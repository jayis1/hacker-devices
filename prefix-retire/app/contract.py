# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""PR1 necessary invariants; neither a proof of realizability nor authentication."""
FIELDS='now levels valid halted faults phase ready next stable observed rounds attempts prepared retired rejected gaps missed tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['non-prefix retirement','retirement before ready','duplicate readiness','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['ready']>15 or s['next']>3 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    prefix=(1<<s['next'])-1
    if s['ready']&prefix!=prefix:
        raise ValueError('retired prefix lacks readiness')
    if 4*s['rounds']+s['ready'].bit_count()!=s['prepared']:
        raise ValueError('readiness conservation')
    if 4*s['rounds']+s['next']!=s['retired']:
        raise ValueError('prefix conservation')
    if s['prepared']+s['retired']+s['rejected']!=s['attempts']:
        raise ValueError('attempt conservation')
    if not s['phase'] and s['attempts']:
        raise ValueError('pre-baseline attempts')
    if s['attempts']>2*s['observed']:
        raise ValueError('insufficient observations')
    if s['rejected']>2 or bool(s['rejected'])!=bool(s['faults']&15):
        raise ValueError('rejection finding')
    if s['rejected']==2 and s['faults']!=8:
        raise ValueError('two-edge ambiguity')
    if s['gaps']>1 or bool(s['gaps'])!=bool(s['faults']&16):
        raise ValueError('gap finding')
    if s['missed']<s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')

def transition(a,b,delta):
    if b['phase']<a['phase']:
        return 'Baseline disappeared'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    if b['attempts']-a['attempts']>delta+1:
        return 'Too many operation edges'
    rounds=b['rounds']-a['rounds']
    prepared=b['prepared']-a['prepared']
    if not rounds:
        if b['ready']&a['ready']!=a['ready']:
            return 'Readiness forgotten within batch'
        if (b['ready']&~a['ready']).bit_count()!=prepared:
            return 'Readiness changes disagree with preparations'
        if b['next']<a['next']:
            return 'Prefix regressed without completing batch'
    else:
        # Complete old readiness, then each intermediate batch, then final set.
        needed=4-a['ready'].bit_count()+4*(rounds-1)+b['ready'].bit_count()
        if prepared!=needed:
            return 'Preparations do not support batch boundaries'
    return ''
