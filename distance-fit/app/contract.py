# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""DF1 maximin-distance checks, not authentication or full reconstruction."""
FIELDS='now levels valid halted faults phase active depth stable observed rounds attempts launched completed rejected gaps missed queued peak use0 use1 use2 use3 stamp0 stamp1 stamp2 stamp3 tx_fail'.split()
COUNTERS=FIELDS[9:23]+['tx_fail']
FAULTS=['closer admission despite farther vacancy','orphan release','duplicate allocation','interface/setup ambiguity','acquisition gap','counter exhaustion','watchdog startup']
POLICY=[3,1,5,4,0,1]

def distances(s):
    """Nearest-resident distance for each vacancy; None means no residents."""
    busy=[i for i in range(4) if s['active']&(1<<i)]
    return {i:min((abs(i-j) for j in busy),default=None)
            for i in range(4) if i not in busy}

def eligible(s):
    scores=distances(s)
    if not s['active']:
        return list(range(4))
    best=max((score for score in scores.values() if score is not None),default=0)
    return [i for i,score in scores.items() if score==best]

def short_path_possible(mask,target,admissions,returns):
    """Bounded count-constrained topology search, deliberately ignoring timing.

    At most eight operations gives a small finite search. Larger gaps use
    permanent-occupancy bounds in transition(), not an unbounded recursion.
    Finding one possible ordering does not certify the actual hidden ordering.
    """
    from functools import lru_cache
    if sum(admissions)+sum(returns)>8:
        return True
    @lru_cache(None)
    def visit(mask,add,drop):
        if not any(add) and not any(drop):
            return mask==target
        allowed=eligible({'active':mask})
        for tag in range(4):
            bit=1<<tag
            if add[tag] and tag in allowed:
                remaining=list(add); remaining[tag]-=1
                if visit(mask|bit,tuple(remaining),drop):
                    return True
            if drop[tag] and mask&bit:
                remaining=list(drop); remaining[tag]-=1
                if visit(mask&~bit,add,tuple(remaining)):
                    return True
        return False
    return visit(mask,tuple(admissions),tuple(returns))

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
        raise ValueError('distance finding requires a free candidate with a better rival')
    if s['faults']==4 and not s['active']&(1<<tag):
        raise ValueError('duplicate finding requires an occupied identity')
    if s['faults']==2 and s['active']&(1<<tag):
        raise ValueError('orphan finding requires a vacant identity')
    for i,n in enumerate(stamps):
        use=s['use'+str(i)]
        if bool(n)!=bool(use) or use>n:
            raise ValueError('admission history')
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
    # Every possible intermediate occupancy must preserve continuously free
    # and continuously occupied identities. Overapproximate all other slots.
    fixed_free=sum(1<<i for i in range(4) if not a['active']&(1<<i) and not changes[i])
    fixed_busy=sum(1<<i for i in range(4) if a['active']&(1<<i) and not releases[i])
    possible=set()
    for mask in range(16):
        if mask&fixed_free or mask&fixed_busy!=fixed_busy:
            continue
        possible.update(eligible({'active':mask}))
    if any(n and i not in possible for i,n in enumerate(changes)):
        return 'Persistent occupancy blocks this admission in every possible occupancy'
    if not short_path_possible(a['active'],b['active'],changes,releases):
        return 'No maximin-distance operation ordering matches sparse counts'
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
