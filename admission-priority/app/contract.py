# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""AP1 evidence validation. Necessary plausibility, not authentication."""
FIELDS=('now levels valid halted faults gaps missed ready observed starts accepted '
        'orphans inversions collided high low overlaps overlap_ticks collision_active baselines tx_fail').split()
COUNTERS=('gaps missed observed starts accepted orphans inversions collided high low '
          'overlaps overlap_ticks baselines tx_fail').split()
FAULTS=['orphan admission','priority inversion','collision or direct handover',
        'sample gap','counter exhaustion','watchdog unavailable']
def validate(s):
    def require(ok):
        if not ok: raise ValueError('Impossible AP1 snapshot')
    require(all(type(s[k]) is int and 0<=s[k]<=0xffffffff for k in FIELDS))
    require(s['levels']<=15 and s['valid']<=15 and s['faults']<=63)
    require(s['ready']<=1 and s['halted']<=1 and s['collision_active']<=1)
    require(bool(s['halted'])==bool(s['faults']&48))
    require(not s['halted'] or not(s['ready'] or s['valid'] or s['collision_active']))
    require(not s['ready'] or (s['valid']==15 and s['baselines']))
    if not s['halted']:
        require(bool(s['collision_active'])==((s['valid']&12)==12 and (s['levels']&12)==12))
    for field,bit in [('orphans',1),('inversions',2),('gaps',8)]:
        require(bool(s[field])==bool(s['faults']&bit))
    require(bool(s['collided'] or s['overlaps'])==bool(s['faults']&4))
    require(s['starts']==sum(s[k] for k in ['accepted','orphans','inversions','collided']))
    require(s['starts']==s['high']+s['low'])
    require(s['inversions']<=s['low'])
    require(s['overlaps']<=s['overlap_ticks'])
    require(bool(s['overlaps'])==bool(s['overlap_ticks']))
    require(not s['collision_active'] or s['overlaps'])
    require(s['baselines']<=s['observed'] and s['baselines']<=s['gaps']+1)
    require(not s['starts'] or s['baselines'])
    require(s['starts']<=2*s['observed'])
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']))
def transition(p,s,delta):
    growth={k:s[k]-p[k] for k in COUNTERS}
    if growth['inversions']>growth['low']:
        return 'Priority findings grew without low admissions'
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
