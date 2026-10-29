# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""TH1 snapshot and sparse-transition plausibility; not authentication."""
FIELDS=('now levels valid halted faults gaps missed state settle mismatch coverage '
        'good bad_high bad_low episodes streak longest rises falls tx_fail').split()
COUNTERS='gaps missed coverage good bad_high bad_low episodes longest rises falls tx_fail'.split()
FAULTS=['missing high alarm','spurious high alarm','sample gap','counter exhaustion','watchdog unavailable']
def validate(s):
    def require(ok):
        if not ok: raise ValueError('Impossible TH1 snapshot')
    require(all(type(s[k]) is int and 0<=s[k]<=0xffffffff for k in FIELDS))
    require(s['levels']<=15 and s['valid']<=15 and s['faults']<=31)
    require(s['state']<=2 and s['halted']<=1 and s['mismatch']<=1 and s['settle']<=10)
    require(bool(s['halted'])==bool(s['faults']&24))
    require(not s['halted'] or not (s['state'] or s['valid']))
    require(not s['state'] or (s['valid']==15 and not s['levels']&8))
    require(s['state'] or not(s['settle'] or s['mismatch'] or s['streak']))
    require(not s['settle'] or not(s['mismatch'] or s['streak']))
    require((s['valid']&3) in (0,3))
    if s['state'] and not s['settle']:
        require(bool(s['mismatch'])==((s['state']==2)!=bool(s['levels']&4)))
    bad=s['bad_high']+s['bad_low']
    require(s['good']+bad==s['coverage'])
    require(s['episodes']<=bad and s['longest']<=bad)
    require(bool(bad)==bool(s['episodes'])==bool(s['longest']))
    require(s['longest']+s['episodes']<=bad+1)
    require(s['episodes']*s['longest']>=bad)
    require(s['streak']<=s['longest'] and bool(s['streak'])==bool(s['mismatch']))
    require(not s['mismatch'] or ((s['state']==2)!=bool(s['levels']&4)))
    require(s['state']!=2 or s['rises'])
    require(s['state']!=1 or s['falls'])
    require(not s['state'] or (s['levels']&3)!=0 or s['state']==1)
    require(not s['state'] or (s['levels']&3)!=3 or s['state']==2)
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']))
    for field,bit in [('bad_high',1),('bad_low',2),('gaps',4)]:
        require(bool(s[field])==bool(s['faults']&bit))
def transition(p,s,delta):
    growth={k:s[k]-p[k] for k in COUNTERS}
    if growth['episodes']>growth['bad_high']+growth['bad_low']:
        return 'New episodes exceed mismatched observations'
    if s['longest']>p['longest']+growth['bad_high']+growth['bad_low']:
        return 'Longest episode grew without mismatched observations'
    if s['state']!=p['state'] and s['state'] and not growth['rises' if s['state']==2 else 'falls']:
        return 'Obligation changed without a threshold entry'
    return ''
