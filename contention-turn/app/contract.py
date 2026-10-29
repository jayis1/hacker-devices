# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""CT1 necessary evidence bounds; not authentication or a waveform proof."""
FIELDS=('now levels valid halted faults gaps missed ready observed starts accepted '
        'orphans unfair collided side_a side_b overlaps overlap_ticks collision_active '
        'baselines turn seeds contended tx_fail').split()
COUNTERS=('gaps missed observed starts accepted orphans unfair collided side_a side_b '
          'overlaps overlap_ticks baselines seeds contended tx_fail').split()
FAULTS=['orphan admission','unfair contended turn','collision or direct handover',
        'sample gap','counter exhaustion','watchdog unavailable']
def validate(s):
    def require(ok):
        if not ok: raise ValueError('Impossible CT1 snapshot')
    require(all(type(s[k]) is int and 0<=s[k]<=0xffffffff for k in FIELDS))
    require(s['levels']<=15 and s['valid']<=15 and s['faults']<=63)
    require(s['ready']<=1 and s['halted']<=1 and s['collision_active']<=1)
    require(s['turn']<=2 and (not s['turn'] or (s['ready'] and s['seeds'])))
    require(bool(s['halted'])==bool(s['faults']&48))
    require(not s['halted'] or not(s['ready'] or s['valid'] or s['collision_active'] or s['turn']))
    require(not s['ready'] or (s['valid']==15 and s['baselines']))
    if not s['halted']:
        both=(s['valid']&12)==12 and (s['levels']&12)==12
        require(bool(s['collision_active'])==both and (not both or not s['turn']))
    for field,bit in [('orphans',1),('unfair',2),('gaps',8)]:
        require(bool(s[field])==bool(s['faults']&bit))
    require(bool(s['collided'] or s['overlaps'])==bool(s['faults']&4))
    require(s['starts']==sum(s[k] for k in ['accepted','orphans','unfair','collided']))
    require(s['starts']==s['side_a']+s['side_b'])
    clean=s['accepted']+s['unfair']
    require(s['unfair']<=s['contended']<=clean)
    require(s['seeds']<=s['accepted'] and bool(clean)==bool(s['seeds']))
    require(s['seeds']<=1+s['gaps']+s['orphans']+s['collided'])
    require(s['overlaps']<=s['overlap_ticks'])
    require(bool(s['overlaps'])==bool(s['overlap_ticks']))
    require(not s['collision_active'] or s['overlaps'])
    require(s['baselines']<=s['observed'] and s['baselines']<=s['gaps']+1)
    require(not s['starts'] or s['baselines'])
    require(s['starts']<=2*s['observed'])
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']))
def transition(p,s,delta):
    growth={k:s[k]-p[k] for k in COUNTERS}
    clean=growth['accepted']+growth['unfair']
    if not growth['unfair']<=growth['contended']<=clean:
        return 'Contention findings grew without matching admissions'
    if growth['seeds']>growth['accepted']:
        return 'Seed appeared without accepted admission'
    if growth['seeds']>growth['gaps']+growth['orphans']+growth['collided']+(not p['turn']):
        return 'Preference reseeded without loss of correlation'
    if s['turn']!=p['turn']:
        if s['turn'] and not clean:
            return 'Preference changed without clean admission'
        if not s['turn'] and not (growth['gaps'] or growth['orphans'] or growth['collided'] or s['halted']):
            return 'Preference disappeared without loss of correlation'
    if growth['overlaps']>growth['overlap_ticks']:
        return 'Overlap episodes grew without overlap observations'
    if growth['starts']>2*growth['observed']:
        return 'Admissions exceed newly covered observations'
    if growth['baselines']>growth['gaps']+(not p['ready']):
        return 'Baseline restored without continuity loss'
    if s['ready'] and not p['ready'] and not growth['baselines']:
        return 'Readiness appeared without baseline'
    if p['ready'] and not s['ready'] and not growth['gaps'] and not s['halted']:
        return 'Readiness disappeared without gap or halt'
    return ''
