# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""UQ1 structural and per-slot timer bounds, not full trace reconstruction."""
FIELDS='now levels valid halted faults phase active cooling stable observed rounds attempts launched completed rejected gaps missed expired peak remaining0 remaining1 tx_fail'.split()
COUNTERS=FIELDS[9:19]+['tx_fail']
FAULTS=['early reuse','orphan return','duplicate allocation','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']

def queue(s):
    return [i for i in range(2) if s['active']&(1<<i)]

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['active']>3 or s['cooling']>3 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    depth=s['active'].bit_count()
    if s['active']&s['cooling']:
        raise ValueError('occupied slot cannot be cooling')
    for i in range(2):
        remaining=s[f'remaining{i}']
        if remaining>1000 or bool(remaining)!=bool(s['cooling']&(1<<i)):
            raise ValueError('timer bounds')
    if s['expired']+s['cooling'].bit_count()!=s['completed']:
        raise ValueError('return timer conservation')
    if s['completed']+depth!=s['launched']:
        raise ValueError('ownership conservation')
    if not depth<=s['peak']<=min(2,s['launched']) or bool(s['peak'])!=bool(s['launched']):
        raise ValueError('peak bounds')
    if s['rounds']>s['completed'] or s['rounds']+bool(depth)>s['launched']:
        raise ValueError('episode bounds')
    if s['launched'] and not depth and not s['rounds']:
        raise ValueError('missing closed episode')
    if s['peak']<2 and s['rounds']!=s['completed']:
        raise ValueError('serial occupancy must drain on each return')
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
    if a['halted']:
        return ''  # Caller checks immutability, including both timers.
    if b['phase']<a['phase']:
        return 'Baseline disappeared'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    if b['attempts']-a['attempts']>delta+1:
        return 'Too many operation edges'
    # Once tracking, every successful sample increments observed, including
    # the policy-fault endpoint. Gap/exhaustion endpoints credit no interval.
    # Both timers therefore share this exact acquisition clock on a halt.
    elapsed=b['observed']-a['observed'] if a['phase'] else delta
    if not b['halted'] and a['phase'] and elapsed!=delta:
        return 'Tracking observation clock disagrees with device time'
    need=[0,0,0]
    for i in range(2):
        bit=1<<i
        old=1 if a['active']&bit else 2 if a['cooling']&bit else 0
        new=1 if b['active']&bit else 2 if b['cooling']&bit else 0
        cursor=old
        while cursor!=new:
            need[cursor]+=1
            cursor=(cursor+1)%3
        before=a[f'remaining{i}']; after=b[f'remaining{i}']
        if old==2 and before>elapsed:
            if new!=2:
                return 'Quarantine expired before its deadline'
            if after!=before-elapsed:
                return 'Continuous timer does not match shared acquisition time'
        elif new==2:
            # A continuing timer is possible only until expiry. Otherwise a
            # new return must fit a fixed duration within the snapshot span.
            available=elapsed-before if old==2 else elapsed
            renewed=any(0<=duration-after<=available for duration in (250,1000))
            if not renewed:
                return 'New timer lacks a possible return time'
            if old==2 and b['completed']==a['completed']:
                return 'Timer restarted without return'
    extra=[b[k]-a[k]-n for k,n in zip(['launched','completed','expired'],need)]
    if min(extra)<0 or len(set(extra))!=1:
        return 'Per-slot lifecycle lacks required allocation/return/expiry'
    starts=b['launched']-a['launched']; ends=b['completed']-a['completed']
    if b['peak']>max(a['peak'],a['active'].bit_count()+starts):
        return 'Peak growth lacks allocations'
    if b['rounds']-a['rounds']>ends:
        return 'Closed episodes lack returns'
    return ''
