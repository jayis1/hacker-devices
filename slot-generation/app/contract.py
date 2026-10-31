# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""SG1 integrity constraints; these checks do not authenticate evidence."""
FIELDS='now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['generation mismatch or non-toggling reuse','orphan release','duplicate live slot','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']

def queue(s):
    """Compatibility name for the UI: result is a slot set, never FIFO order."""
    return [(i,(s['order']>>i)&1) for i in range(2) if s['active']&(1<<i)]

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['active']>3 or s['order']>15 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    seen=s['order']>>2
    if s['active']&~seen or (s['order']&3)&~seen:
        raise ValueError('unseen generation or occupancy')
    if seen.bit_count()>s['launched'] or bool(seen)!=bool(s['launched']):
        raise ValueError('seen allocation history')
    depth=s['active'].bit_count()
    if not depth<=s['peak']<=min(2,s['launched']) or bool(s['peak'])!=bool(s['launched']):
        raise ValueError('peak bounds')
    if s['peak'] and s['peak']-1>s['queued']:
        raise ValueError('peak requires overlapping admissions')
    if s['queued'] and s['peak']<2:
        raise ValueError('overlap coverage')
    if s['completed']+depth!=s['launched']:
        raise ValueError('ownership conservation')
    if s['rounds']+bool(depth)+s['queued']!=s['launched'] or s['rounds']>s['completed']:
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
    """Necessary lower bounds for hidden operations between sparse snapshots."""
    if b['phase']<a['phase']:
        return 'Baseline disappeared'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    if b['attempts']-a['attempts']>delta+1:
        return 'Too many operation edges'
    old_seen,new_seen=a['order']>>2,b['order']>>2
    if old_seen&~new_seen:
        return 'Generation history disappeared'
    starts=b['launched']-a['launched']
    ends=b['completed']-a['completed']
    need_starts=need_ends=0
    for i in range(2):
        bit=1<<i
        old_live=bool(a['active']&bit)
        new_live=bool(b['active']&bit)
        changed=bool((a['order']^b['order'])&bit)
        first=bool(new_seen&bit) and not bool(old_seen&bit)
        # A changed generation needs an allocation, which needs release of a
        # previous live owner. An unseen slot first becoming seen also needs
        # an allocation even if its initial generation is zero.
        alloc=int(changed or first or (new_live and not old_live))
        releases=alloc+int(old_live)-int(new_live)
        need_starts+=alloc
        need_ends+=releases
    if starts<need_starts or ends<need_ends:
        return 'Generation or occupancy changed without ownership cycle'
    if b['peak']>max(a['peak'],a['active'].bit_count()+starts):
        return 'Peak growth lacks allocations'
    return ''
