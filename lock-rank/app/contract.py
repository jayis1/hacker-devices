# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""LR1 bounds for ranked acquisitions, not authentication or waveform proof."""
CLIENT='observed baselines starts solo ascending descending ambiguous'.split()
FIELDS=('now levels valid halted faults gaps missed ready'.split()+
        [side+'_'+key for side in ('a','b') for key in CLIENT]+
        'low_overlaps high_overlaps low_ticks high_ticks active tx_fail'.split())
COUNTERS=['gaps','missed']+[side+'_'+key for side in ('a','b') for key in CLIENT]+[
    'low_overlaps','high_overlaps','low_ticks','high_ticks','tx_fail']
FAULTS=['descending acquisition','simultaneous unordered acquisition',
        'same-rank ownership collision','sample gap','counter exhaustion','watchdog unavailable']
def validate(s):
    def require(ok):
        if not ok: raise ValueError('Impossible LR1 snapshot')
    require(all(type(s[k]) is int and 0<=s[k]<=0xffffffff for k in FIELDS))
    require(s['levels']<=15 and s['valid']<=15 and s['faults']<=63)
    require(s['ready']<=3 and s['active']<=3 and s['halted']<=1)
    require(bool(s['halted'])==bool(s['faults']&48))
    require(not s['halted'] or not(s['ready'] or s['valid'] or s['active']))
    for side,index in [('a',0),('b',1)]:
        c={k:s[side+'_'+k] for k in CLIENT}
        require(c['starts']==c['solo']+c['ascending']+c['descending']+c['ambiguous'])
        require(c['ambiguous']%2==0)
        require(c['baselines']<=c['observed'] and c['baselines']<=s['gaps']+1)
        require(c['starts']<=2*c['observed'] and (not c['starts'] or c['baselines']))
        pair=3<<(2*index)
        require(not(s['ready']&(1<<index)) or (s['valid']&pair==pair and c['baselines']))
    for rank,index in [('low',0),('high',1)]:
        pair=5<<index
        active=bool(s['active']&(1<<index))
        require(active==(s['valid']&pair==pair and s['levels']&pair==pair))
        require(not active or s[rank+'_overlaps'])
        require(s[rank+'_overlaps']<=s[rank+'_ticks'])
        require(bool(s[rank+'_overlaps'])==bool(s[rank+'_ticks']))
    for present,bit in [(s['a_descending'] or s['b_descending'],1),
                        (s['a_ambiguous'] or s['b_ambiguous'],2),
                        (s['low_overlaps'] or s['high_overlaps'],4),(s['gaps'],8)]:
        require(bool(present)==bool(s['faults']&bit))
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']))
def transition(p,s,delta):
    growth={k:s[k]-p[k] for k in COUNTERS}
    if growth['missed']<growth['gaps'] or (growth['missed'] and not growth['gaps']):
        return 'Missed ticks changed without matching gaps'
    if p['valid'] & ~s['valid'] and not (growth['gaps'] or s['halted']):
        return 'Qualification disappeared without gap or halt'
    for side,index in [('a',0),('b',1)]:
        g={k:growth[side+'_'+k] for k in CLIENT}
        if g['observed']+growth['missed']>delta:
            return 'Observed and missed ticks exceed elapsed device time'
        if g['starts']>2*g['observed']:
            return 'Starts grew without matching pair coverage'
        bit=1<<index
        if g['baselines']>growth['gaps']+(not(p['ready']&bit)):
            return 'Baseline appeared without loss of correlation'
        if s['ready']&bit and not(p['ready']&bit) and not g['baselines']:
            return 'Readiness appeared without baseline'
        if p['ready']&bit and not(s['ready']&bit) and not(growth['gaps'] or s['halted']):
            return 'Readiness disappeared without gap or halt'
    for rank in ('low','high'):
        if growth[rank+'_overlaps']>growth[rank+'_ticks']:
            return 'Collision segment grew without collision observation'
    return ''
