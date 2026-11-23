# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""WN1 necessary endpoint checks, not authentication or full reconstruction."""
FIELDS=('now levels valid halted faults phase active depth stable observed rounds attempts launched completed rejected gaps missed queued peak use0 use1 use2 use3 stamp0 stamp1 stamp2 stamp3 '
        'old0 new0 old1 new1 old2 new2 old3 new3 tx_fail').split()
COUNTERS=FIELDS[9:23]+['tx_fail']
FAULTS=['third admission inside rolling window','orphan release','duplicate allocation','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']
POLICY=[3,1,5,4,2,1000]

def timers(s):
    return [(s['old'+str(i)],s['new'+str(i)]) for i in range(4)]

def eligible(s):
    return [tag for tag in range(4) if not s['active']&(1<<tag)
            and s['old'+str(tag)]==0]

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
    stamps=[s['stamp'+str(i)] for i in range(4)]
    seen=[n for n in stamps if n]
    if len(seen)!=len(set(seen)) or max(stamps)!=s['launched']:
        raise ValueError('last ordinal ownership')
    tag=s['levels']&3
    if s['faults']==1 and (s['active']&(1<<tag) or tag in eligible(s)):
        raise ValueError('window finding requires two unexpired admissions')
    for i,n in enumerate(stamps):
        use=s['use'+str(i)]
        if bool(n)!=bool(use) or use>n:
            raise ValueError('admission history')
        old,new=timers(s)[i]
        if not 0<=old<=new<=1000 or (old and old==new):
            raise ValueError('deadline ordering')
        if (not use and new) or (use<2 and old):
            raise ValueError('deadline without admission')
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
    acquired=b['observed']-a['observed']
    if starts+ends>acquired:
        return 'Accepted operations exceed acquired observations'
    if b['attempts']-a['attempts']>2*acquired:
        return 'Attempt growth exceeds acquired observations'
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
    # Timers advance by acquired observations, never by terminal wall time.
    # Once history exists qualification is already established permanently.
    if a['phase'] and not b['halted'] and acquired!=delta:
        return 'Continuous tracking omitted acquired ticks'
    for i,n in enumerate(changes):
        old,new=timers(a)[i]
        later_old,later_new=timers(b)[i]
        if not n:
            if (later_old,later_new)!=(max(0,old-acquired),max(0,new-acquired)):
                return 'Deadline changed without matching admission'
        else:
            # Two deadline lanes: earliest kth admission needs the corresponding
            # old lane to expire, then one whole window per subsequent reuse.
            lane=(n-1)%2
            earliest=max(1,(old,new)[lane])+((n-1)//2)*1000
            if earliest>acquired:
                return 'Admissions exceed rolling-window capacity'
            # A retained deadline also locates the admission within the gap.
            # Zero gives an upper bound (expired), not an unknown free pass.
            if acquired+later_new-1000<earliest:
                return 'Newest deadline predates permitted renewal'
            if n>=2:
                prior_lane=(n-2)%2
                prior_earliest=max(1,(old,new)[prior_lane])+((n-2)//2)*1000
                if acquired+later_old-1000<prior_earliest:
                    return 'Older deadline predates permitted renewal'
            if later_new<max(0,1001-acquired):
                return 'New admission deadline too old'
            if n==1 and later_old!=max(0,new-acquired):
                return 'Previous newest deadline lost or migrated'
            if n>=2 and later_old<max(0,1001-acquired):
                return 'Penultimate admission deadline too old'
    if b['peak']>max(a['peak'],a['depth']+starts):
        return 'Peak growth lacks admissions'
    if not starts and b['peak']!=a['peak']:
        return 'Peak changed without admission'
    if not a['active'] and not b['active'] and starts and b['rounds']==a['rounds']:
        return 'Hidden cycle lacks episode closure'
    for i,n in enumerate(changes):
        old=a['stamp'+str(i)]; new=b['stamp'+str(i)]
        if not n and new!=old:
            return 'Last ordinal changed without admission'
        if n and not a['launched']+n<=new<=b['launched']:
            return 'New admission ordinals impossible'
    return ''
