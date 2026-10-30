# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""DD1 graph and conservation checks; not authentication or waveform proof."""
FIELDS='now levels valid halted faults phase active done stable observed rounds attempts launched completed rejected gaps missed tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['start before prerequisite completion','orphan completion','duplicate job start','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']
PREREQUISITES=(0,1,1,6)

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['active']>15 or s['done']>7 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    if s['active']&s['done']:
        raise ValueError('job both active and complete')
    seen=s['active']|s['done']
    for tag,required in enumerate(PREREQUISITES):
        if seen&(1<<tag) and s['done']&required!=required:
            raise ValueError('job without finished prerequisites')
    if 4*s['rounds']+seen.bit_count()!=s['launched']:
        raise ValueError('launch conservation')
    if 4*s['rounds']+s['done'].bit_count()!=s['completed']:
        raise ValueError('completion conservation')
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
    if b['phase']<a['phase']:
        return 'Baseline disappeared'
    if not b['halted'] and b['valid']&a['valid']!=a['valid']:
        return 'Qualification disappeared without halt'
    if b['observed']-a['observed']+b['missed']-a['missed']>delta:
        return 'Observed and missed growth exceed device time'
    if b['attempts']-a['attempts']>delta+1:
        return 'Too many operation edges'
    rounds=b['rounds']-a['rounds']
    old_seen=a['active']|a['done']
    new_seen=b['active']|b['done']
    if not rounds:
        if b['done']&a['done']!=a['done'] or new_seen&old_seen!=old_seen:
            return 'Job identity forgotten within graph'
        if (new_seen&~old_seen).bit_count()!=b['launched']-a['launched']:
            return 'Membership changes disagree with launches'
        if (b['done']&~a['done']).bit_count()!=b['completed']-a['completed']:
            return 'Finished set disagrees with completions'
    else:
        needed=4-old_seen.bit_count()+4*(rounds-1)+new_seen.bit_count()
        if b['launched']-a['launched']!=needed:
            return 'Launches do not support graph boundaries'
        needed=4-a['done'].bit_count()+4*(rounds-1)+b['done'].bit_count()
        if b['completed']-a['completed']!=needed:
            return 'Completions do not support graph boundaries'
    return ''
