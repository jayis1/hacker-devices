# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""QO1 queue integrity constraints; CRC and invariants are not authentication."""
FIELDS='now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['tag overtook queue head','orphan dequeue','duplicate live tag','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']

def queue(s):
    return [(s['order']>>(2*i))&3 for i in range(s['active'].bit_count())]

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['active']>15 or s['order']>255 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    q=queue(s)
    if len(set(q))!=len(q) or sum(1<<tag for tag in q)!=s['active'] or s['order']>>(2*len(q)):
        raise ValueError('packed queue identity mismatch')
    if not len(q)<=s['peak']<=min(4,s['launched']) or bool(s['peak'])!=bool(s['launched']):
        raise ValueError('peak bounds')
    if s['peak'] and s['peak']-1>s['queued']:
        raise ValueError('peak requires queued admissions')
    if s['queued'] and s['peak']<2:
        raise ValueError('queued admission requires overlap coverage')
    if s['completed']+len(q)!=s['launched']:
        raise ValueError('queue conservation')
    if s['rounds']+bool(q)+s['queued']!=s['launched'] or s['rounds']>s['completed']:
        raise ValueError('episode conservation')
    if s['launched']+s['completed']+s['rejected']!=s['attempts']:
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
    """Necessary sparse-record bounds; not reconstruction of unseen events."""
    if b['phase']<a['phase']:
        return 'Baseline disappeared'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    if b['attempts']-a['attempts']>delta+1:
        return 'Too many operation edges'
    starts=b['launched']-a['launched']
    ends=b['completed']-a['completed']
    rounds=b['rounds']-a['rounds']
    old,new=queue(a),queue(b)
    # FIFO departures remove only a prefix. Surviving old tags must lead every
    # new admission, including a tag reused after its own earlier departure.
    survivors=old[min(ends,len(old)):]
    if new[:len(survivors)]!=survivors:
        return 'Queue survivor order changed without head departures'
    if len(new)-len(survivors)>starts:
        return 'New queue entries lack enqueues'
    if rounds:
        if ends<len(old)+rounds-1:
            return 'Empty boundaries lack dequeues'
        if starts<rounds-bool(old)+len(new):
            return 'Empty boundaries lack enqueues'
    elif old and ends>=len(old) and not starts and new:
        return 'Queue replacement lacks admissions'
    if b['peak']>max(a['peak'],len(old)+starts):
        return 'Peak growth lacks admissions'
    return ''
