# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""RS1 ownership and cohort conservation; not authentication."""
FIELDS='now levels valid halted faults phase active seen stable observed rounds attempts launched completed rejected gaps missed readers writers shared tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['reader/writer overlap','orphan release','duplicate admission','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']

def validate(s):
    if set(s)!=set(FIELDS) or any(type(s[k]) is not int or not 0<=s[k]<=0xffffffff for k in FIELDS):
        raise ValueError('fields or unsigned bounds')
    if s['levels']>15 or s['valid']>15 or s['faults']>127:
        raise ValueError('masks')
    if s['phase']>1 or s['active']>15 or s['seen']>15 or s['stable']>5 or s['halted']>1:
        raise ValueError('state bounds')
    if bool(s['halted'])!=bool(s['faults']) or s['faults']&(s['faults']-1):
        raise ValueError('terminal reason')
    if s['halted'] and s['valid']:
        raise ValueError('halted qualification')
    if not s['halted'] and s['phase'] and s['valid']!=15:
        raise ValueError('baseline qualification')
    active,seen=s['active'],s['seen']
    if active&8 and active&7:
        raise ValueError('incompatible active owners')
    if active&seen!=active or (not active and seen) or (seen&8 and seen!=8):
        raise ValueError('cohort membership')
    if s['completed']+active.bit_count()!=s['launched']:
        raise ValueError('ownership conservation')
    if s['readers']+s['writers']!=s['launched'] or s['shared']>s['readers']:
        raise ValueError('admission classes')
    if s['rounds']+bool(active)+s['shared']!=s['launched'] or s['rounds']>s['completed']:
        raise ValueError('cohort conservation')
    if (seen&7).bit_count()>s['readers'] or (active&8 and not s['writers']):
        raise ValueError('identity without admission')
    if max(0,seen.bit_count()-1)>s['shared']:
        raise ValueError('cohort sharing unsupported')
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
    starts=b['launched']-a['launched']
    ends=b['completed']-a['completed']
    rounds=b['rounds']-a['rounds']
    if (b['active']&~a['active']).bit_count()>starts or (a['active']&~b['active']).bit_count()>ends:
        return 'Owner changes lack operations'
    if not rounds:
        if b['seen']&a['seen']!=a['seen']:
            return 'Cohort membership forgotten before last release'
        if (b['seen']&~a['seen']).bit_count()>starts:
            return 'New cohort identities lack admissions'
        if a['active'] and bool(a['active']&8)!=bool(b['active']&8):
            return 'Reader/writer change without empty boundary'
    else:
        if ends<a['active'].bit_count()+rounds-1:
            return 'Cohort boundaries lack releases'
        if starts<rounds-1+b['seen'].bit_count():
            return 'Cohort boundaries lack admissions'
    if (b['seen']&7&~a['seen']).bit_count()>b['readers']-a['readers']:
        return 'Reader identities lack reader admissions'
    if b['active']&8 and not a['active']&8 and b['writers']==a['writers']:
        return 'Writer ownership lacks writer admission'
    return ''
