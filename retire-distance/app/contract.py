# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""RT1 necessary endpoint checks, not authentication or full reconstruction."""
FIELDS='now levels valid halted faults phase active depth stable observed rounds attempts launched completed rejected gaps missed queued peak use0 use1 use2 use3 stamp0 stamp1 stamp2 stamp3 tx_fail'.split()
COUNTERS=FIELDS[9:23]+['tx_fail']
FAULTS=['release before two later admissions','orphan release','duplicate allocation','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']
POLICY=[3,1,5,4,2,1]

def eligible(s):
    return [i for i in range(4) if s['active']&(1<<i)
            and s['launched']-s['stamp'+str(i)]>=2]

def queue(s):
    return [(i,s['use'+str(i)]) for i in range(4) if s['active']&(1<<i)]

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
    if s['depth']!=s['active'].bit_count():
        raise ValueError('occupancy count')
    tag=s['levels']&3
    if s['faults']==1 and (not s['active']&(1<<tag) or tag in eligible(s)):
        raise ValueError('early release requires an immature resident')
    if s['faults']==2 and s['active']&(1<<tag):
        raise ValueError('orphan names a resident')
    if s['faults']==4 and not s['active']&(1<<tag):
        raise ValueError('duplicate names a vacancy')
    if s['rounds']:
        raise ValueError('youngest residents prevent full drain')
    stamps=[s['stamp'+str(i)] for i in range(4)]
    seen=[n for n in stamps if n]
    if len(seen)!=len(set(seen)) or max(stamps)!=s['launched']:
        raise ValueError('last ordinal ownership')
    for i,n in enumerate(stamps):
        use=s['use'+str(i)]
        if bool(n)!=bool(use) or use>n or (use and 1+3*(use-1)>n):
            raise ValueError('admission history')
        if n and not s['active']&(1<<i) and s['launched']-n<2:
            raise ValueError('immature resident disappeared')
    for age in range(min(2,s['launched'])):
        if not any(s['active']&(1<<i) and n==s['launched']-age for i,n in enumerate(stamps)):
            raise ValueError('latest two admissions must remain resident')
    if not s['depth']<=s['peak']<=4 or bool(s['peak'])!=bool(s['launched']) or s['peak']>s['launched']:
        raise ValueError('peak bounds')
    if (s['queued'] and s['peak']<2) or (s['peak']>1 and s['queued']<s['peak']-1):
        raise ValueError('overlap coverage')
    if sum(s['use'+str(i)] for i in range(4))!=s['launched'] or any(not use for _,use in queue(s)):
        raise ValueError('use conservation')
    if s['completed']+s['depth']!=s['launched']:
        raise ValueError('resource conservation')
    if s['rounds']+bool(s['active'])+s['queued']!=s['launched'] or s['rounds']>s['completed']:
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
    changes=[b['use'+str(i)]-a['use'+str(i)] for i in range(4)]
    if any(n<0 for n in changes) or sum(changes)!=starts:
        return 'Cumulative use history changed without admissions'
    if starts<added.bit_count() or ends<removed.bit_count():
        return 'Membership changed without allocation/release'
    releases=[n+bool(a['active']&(1<<i))-bool(b['active']&(1<<i)) for i,n in enumerate(changes)]
    if any(n<0 or n>ends for n in releases):
        return 'Per-tag lifetime cannot explain membership'
    # Bound first renewal by old resident debt, then each renewal by three
    # ordinal positions. These necessary bounds do not reconstruct ordering.
    for i,n in enumerate(changes):
        old=a['stamp'+str(i)]
        if releases[i] and a['active']&(1<<i) and b['launched']-old<2:
            return 'Release lacks two later admissions'
        if n:
            first=max(a['launched']+1,old+3) if a['active']&(1<<i) else a['launched']+1
            if b['stamp'+str(i)]<first+3*(n-1):
                return 'Renewal precedes resident maturity'
    if b['peak']>max(a['peak'],a['depth']+starts):
        return 'Peak growth lacks admissions'
    if not starts and b['peak']!=a['peak']:
        return 'Peak changed without admission'
    if b['rounds']!=a['rounds']:
        return 'Pipeline cannot drain'
    for i,n in enumerate(changes):
        old=a['stamp'+str(i)]; new=b['stamp'+str(i)]
        if not n and new!=old:
            return 'Last ordinal changed without admission'
        if n and not a['launched']+n<=new<=b['launched']:
            return 'New admission ordinals impossible'
    return ''
