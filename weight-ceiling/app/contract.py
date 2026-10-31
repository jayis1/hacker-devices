# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""WC1 necessary consistency constraints, not proof of authentic measurements."""
FIELDS='now levels valid halted faults phase active load stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['weighted budget exceeded','orphan release','duplicate allocation','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']

def queue(s):
    """Active (tag, weight) pairs in tag order, not retirement order."""
    return [(i,i+1) for i in range(4) if s['active']&(1<<i)]

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['active']>15 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    if s['load']!=sum(cost for _,cost in queue(s)) or s['load']>5:
        raise ValueError('weighted occupancy')
    if not s['load']<=s['peak']<=5 or bool(s['peak'])!=bool(s['launched']):
        raise ValueError('peak bounds')
    if s['peak']>4*s['launched'] or (s['peak']==5 and not s['queued']):
        raise ValueError('peak lacks allocations')
    if s['queued'] and s['peak']<3:
        raise ValueError('overlap coverage')
    depth=s['active'].bit_count()
    if s['completed']+depth!=s['launched']:
        raise ValueError('resource conservation')
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
    """Necessary bounds for sparse evidence; hidden full cycles remain possible."""
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
    added=b['active']&~a['active']
    removed=a['active']&~b['active']
    if starts<added.bit_count() or ends<removed.bit_count():
        return 'Membership changed without allocation/release'
    if b['peak']>max(a['peak'],a['load']+4*starts):
        return 'Weighted peak growth lacks allocations'
    if not starts and b['peak']!=a['peak']:
        return 'Peak changed without an allocation'
    # An empty-to-empty interval with admissions must have closed an episode.
    if not a['active'] and not b['active'] and starts and b['rounds']==a['rounds']:
        return 'Hidden cycle lacks episode closure'
    return ''
