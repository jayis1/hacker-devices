# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""QC1 necessary evidence invariants, not source authentication."""
FIELDS='now levels valid halted faults phase cohort owed observed baselines rounds accepted early orphan abandoned reclaims overlap captured released discarded gaps missed tx_fail'.split()
COUNTERS=FIELDS[8:]
FAULTS=['early reclaim','orphan reclaim','overlapping retirement','acquisition gap','counter exhaustion','watchdog startup']

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>63:
        raise ValueError('masks')
    if s['phase']>3 or s['cohort']>3 or s['owed']>3 or s['halted']>1:
        raise ValueError('state bounds')
    if s['owed'] & ~s['cohort']:
        raise ValueError('owed outside retirement cohort')
    if bool(s['halted'])!=bool(s['faults']&60):
        raise ValueError('halt reason')
    if s['halted'] and any(s[k] for k in ('valid','phase','cohort','owed')):
        raise ValueError('halted state')
    if s['phase'] and (s['valid']!=15 or not s['baselines']):
        raise ValueError('baseline evidence')
    if s['phase']!=2 and (s['cohort'] or s['owed']):
        raise ValueError('inactive cohort')
    closed=s['accepted']+s['early']
    if s['rounds']!=closed+s['abandoned']+(s['phase']==2):
        raise ValueError('round conservation')
    if s['reclaims']!=closed+s['orphan']:
        raise ValueError('reclaim conservation')
    if s['captured']!=s['released']+s['discarded']+s['owed'].bit_count():
        raise ValueError('reader conservation')
    if s['captured']>2*s['rounds']:
        raise ValueError('reader population')
    if s['phase']==2 and s['captured']<s['cohort'].bit_count():
        raise ValueError('unrecorded cohort')
    if s['phase']==3 and not closed:
        raise ValueError('unrecorded decision')
    if any(s[k]>s['observed'] for k in ('baselines','rounds','reclaims')):
        raise ValueError('insufficient observations')
    if any(s[k]>1 for k in ('gaps','overlap','abandoned')) or s['baselines']>closed+1:
        raise ValueError('segment accounting')
    for field,bit in [('early',1),('orphan',2),('overlap',4),('gaps',8)]:
        if bool(s[field])!=bool(s['faults']&bit):
            raise ValueError('sticky findings')
    if s['missed']<s['gaps'] or (not s['gaps'] and s['missed']):
        raise ValueError('gap accounting')

def transition(a,b,delta):
    if b['rounds']==a['rounds'] and b['phase']==a['phase']==2:
        if a['cohort']!=b['cohort']:
            return 'Retirement cohort changed within an active round'
        if b['owed'] & ~a['owed']:
            return 'A later reader reacquired an old obligation'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    if b['captured']-a['captured']>2*(b['rounds']-a['rounds']):
        return 'Captured readers lack new retirement rounds'
    if b['baselines']-a['baselines']>b['accepted']-a['accepted']+b['early']-a['early']+1:
        return 'Too many baseline entries'
    return ''
