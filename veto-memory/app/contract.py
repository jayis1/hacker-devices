# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""VM1 necessary evidence invariants; checksums do not authenticate a source."""
FIELDS='now levels valid halted faults phase veto seen observed baselines rounds accepted early vetoed cancelled abandoned commits repeated veto_rounds withdrawals gaps missed tx_fail'.split()
COUNTERS=FIELDS[8:]
FAULTS=['early commit','commit after veto','repeated commit','acquisition gap','counter exhaustion','watchdog startup']

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>63:
        raise ValueError('masks')
    if s['phase']>3 or s['veto']>1 or s['seen']>3 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']&56):
        raise ValueError('halt reason')
    if s['halted'] and any(s[k] for k in ('valid','phase','seen','veto')):
        raise ValueError('halted state')
    if s['phase'] and (s['valid']!=15 or not s['baselines']):
        raise ValueError('baseline evidence')
    if s['phase']<=1 and (s['seen'] or s['veto']):
        raise ValueError('idle round state')
    if s['phase']==1 and s['levels']:
        raise ValueError('ready levels')
    if s['phase']==2 and s['levels']&4:
        raise ValueError('active commit')
    if s['veto'] and not s['veto_rounds']:
        raise ValueError('unrecorded veto')
    first=s['accepted']+s['early']+s['vetoed']
    if s['rounds']!=first+s['cancelled']+s['abandoned']+(s['phase']==2):
        raise ValueError('round conservation')
    if s['commits']!=first+s['repeated']:
        raise ValueError('commit conservation')
    if (s['phase']==3 or s['repeated']) and not first:
        raise ValueError('unrecorded decision')
    if not s['vetoed']<=s['veto_rounds']<=s['rounds']:
        raise ValueError('veto count')
    if s['withdrawals'] and not s['veto_rounds']:
        raise ValueError('withdrawal without veto')
    if any(s[k]>s['observed'] for k in ('baselines','rounds','commits')):
        raise ValueError('insufficient observations')
    if s['gaps']>1 or s['abandoned']>1 or s['baselines']>s['rounds']+1:
        raise ValueError('segment accounting')
    if s['withdrawals']>2*s['observed']:
        raise ValueError('withdrawal bound')
    for field,bit in [('early',1),('vetoed',2),('repeated',4),('gaps',8)]:
        if bool(s[field])!=bool(s['faults']&bit):
            raise ValueError('sticky findings')
    if s['missed']<s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')

def transition(a,b,delta):
    # Additional checks beyond individual snapshot validity. A hidden
    # baseline is necessary to clear a veto, even when both endpoints active.
    if a['veto'] and not b['veto'] and not b['halted'] and b['baselines']==a['baselines']:
        return 'Veto disappeared without a baseline'
    if b['rounds']==a['rounds'] and b['baselines']==a['baselines'] and not b['halted']:
        if b['seen']&a['seen']!=a['seen']:
            return 'Preparation history disappeared within a round'
        if a['phase']==3 and b['phase']!=3:
            return 'Drain ended without a baseline'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observation and missed-tick growth exceed elapsed time'
    if b['baselines']-a['baselines']>b['rounds']-a['rounds']+1:
        return 'Too many baseline entries'
    return ''
