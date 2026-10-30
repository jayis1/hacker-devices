# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""RS1 necessary ledger invariants. Not authentication or waveform proof."""
FIELDS='now levels valid halted faults phase seen stable observed sessions ended admissions fresh duplicate invalid gaps missed tx_fail'.split()
COUNTERS=FIELDS[8:]
FAULTS=['duplicate identity','invalid session or setup','acquisition gap','counter exhaustion','watchdog startup']

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>31:
        raise ValueError('masks')
    if s['phase']>2 or s['seen']>15 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']&30):
        raise ValueError('halt reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    if s['ended']+(s['phase']==2)!=s['sessions']:
        raise ValueError('session conservation')
    if s['fresh']+s['duplicate']+s['invalid']!=s['admissions']:
        raise ValueError('admission conservation')
    if s['fresh']>4*s['sessions'] or s['seen'].bit_count()>s['fresh']:
        raise ValueError('identity bounds')
    if not s['sessions'] and s['seen']:
        raise ValueError('unrecorded set')
    if not s['phase'] and (s['sessions'] or s['admissions']):
        raise ValueError('pre-baseline activity')
    if max(s['admissions'],s['sessions'])>s['observed']:
        raise ValueError('insufficient observations')
    if bool(s['duplicate'])!=bool(s['faults']&1) or (s['duplicate'] and not s['fresh']):
        raise ValueError('duplicate finding')
    if s['invalid']>1 or (s['invalid'] and not s['faults']&2):
        raise ValueError('invalid finding')
    if s['gaps']>1 or bool(s['gaps'])!=bool(s['faults']&4):
        raise ValueError('gap finding')
    if s['missed']<s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')
    if not s['halted'] and s['phase']==2 and not s['levels']&8:
        raise ValueError('session level')
    if not s['halted'] and s['phase']==1 and s['levels']&12:
        raise ValueError('idle levels')

def transition(a,b,delta):
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    new_sessions=b['sessions']-a['sessions']
    new_fresh=b['fresh']-a['fresh']
    if not new_sessions:
        if b['seen']&a['seen']!=a['seen']:
            return 'Identity forgotten without a new session'
        if (b['seen']&~a['seen']).bit_count()!=new_fresh:
            return 'Membership changes disagree with fresh admissions'
    else:
        # A closed old session contributes no capacity. Opening a new one
        # clears all membership, so every retained bit needs a fresh edge.
        remaining=4-a['seen'].bit_count() if a['phase']==2 else 0
        if new_fresh>4*new_sessions+remaining:
            return 'Fresh admissions exceed remaining session capacity'
        if b['seen'].bit_count()>new_fresh:
            return 'New session membership lacks fresh admissions'
    return ''
