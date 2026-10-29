# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""BO1 transfer matrix and necessary evidence bounds, not authentication."""
FIELDS='now levels valid halted faults observed gaps missed anchors forward skip reverse repeat collisions last ready idle collision_active'.split()+[f'm{i}' for i in range(16)]+['tx_fail']
COUNTERS='observed gaps missed anchors forward skip reverse repeat collisions tx_fail'.split()+[f'm{i}' for i in range(16)]
FAULTS=['Repeated owner','Skipped owner','Reverse turn','Multiple owners','Acquisition gap','Counter exhaustion','Watchdog unavailable']

def validate(s):
    def require(condition, reason):
        if not condition: raise ValueError(reason)
    require(all(type(s[k]) is int and 0<=s[k]<=0xffffffff for k in FIELDS),'range')
    require(s['levels']<=15 and s['valid']<=15 and s['faults']<=127,'bit fields')
    require(all(s[k]<=1 for k in ('halted','ready','idle','collision_active')),'boolean')
    require(s['last']<=4,'owner')
    require(bool(s['halted'])==bool(s['faults']&96),'halt flag')
    require(not s['halted'] or not any(s[k] for k in ('valid','ready','idle','collision_active')),'halt state')
    require(s['ready'] or (s['last']==4 and not s['idle']),'lost phase')
    if s['ready']:
        require(s['valid']==15 and s['levels'].bit_count()<=1,'ready vector')
        require(bool(s['idle'])==(s['levels']==0),'idle vector')
        require(s['idle'] or s['levels']==1<<s['last'],'owner vector')
    require(s['last']==4 or s['anchors']>0,'missing anchor')
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']),'gap accounting')
    for name,bit in [('repeat',1),('skip',2),('reverse',4),('collisions',8),('gaps',16)]:
        require(bool(s[name])==bool(s['faults']&bit),'sticky '+name)
    collision=not s['halted'] and (s['levels']&s['valid']).bit_count()>1
    require(bool(s['collision_active'])==collision,'collision state')
    sums=[0]*4
    for i in range(4):
        for j in range(4): sums[(j-i)%4]+=s[f'm{4*i+j}']
    require(sums==[s[k] for k in ('repeat','forward','skip','reverse')],'matrix aggregates')
    require(s['anchors'] or not any(sums),'unanchored transfers')
