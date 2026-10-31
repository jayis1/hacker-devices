# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""SC1 per-resource scrub-credit invariants. Not authentication or erasure proof."""
FIELDS='now levels valid halted faults phase active clean stable observed rounds attempts launched completed rejected gaps missed scrubbed peak tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['missing, stacked or live scrub credit','orphan return','duplicate allocation','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']

def queue(s):
    return [i for i in range(2) if s['active']&(1<<i)]

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['active']>3 or s['clean']>3 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    depth=s['active'].bit_count()
    if s['active']&s['clean']:
        raise ValueError('occupied slot cannot retain credit')
    if s['launched']+s['clean'].bit_count()!=s['scrubbed']:
        raise ValueError('scrub credit conservation')
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
    if s['launched']+s['completed']+s['scrubbed']+s['rejected']!=s['attempts']:
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
    """Check cyclic per-slot reachability, plus timing and aggregate bounds.

    Every legal slot follows dirty -> clean -> occupied -> dirty. After
    the shortest path, additional hidden work must consist of full cycles;
    these contribute exactly one scrub, allocation and return each.
    Timing and episode bounds remain necessary, not complete reconstruction.
    """
    if b['phase']<a['phase']:
        return 'Baseline disappeared'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    if b['attempts']-a['attempts']>delta+1:
        return 'Too many operation edges'
    need=[0,0,0]
    for i in range(2):
        bit=1<<i
        old=2 if a['active']&bit else int(bool(a['clean']&bit))
        new=2 if b['active']&bit else int(bool(b['clean']&bit))
        while old!=new:
            need[old]+=1
            old=(old+1)%3
    keys=['scrubbed','launched','completed']
    extra=[b[k]-a[k]-n for k,n in zip(keys,need)]
    if min(extra)<0 or len(set(extra))!=1:
        return 'Per-slot credit changed without required scrub/allocation/return cycle'
    starts=b['launched']-a['launched']
    ends=b['completed']-a['completed']
    if b['peak']>max(a['peak'],a['active'].bit_count()+starts):
        return 'Peak growth lacks allocations'
    if b['rounds']-a['rounds']>ends:
        return 'Closed episodes lack returns'
    return ''
