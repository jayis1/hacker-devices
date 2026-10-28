# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""EL1 schema and necessary realizability bounds; not authentication."""
PAIRS=((0,1),(0,2),(0,3),(1,2),(1,3),(2,3))
FIELDS: list[str] = []
FIELDS.extend('now levels valid halted faults observed gaps missed'.split())
for i in range(6):
    FIELDS.extend(f'{key}{i}' for key in ('entries','duration','maximum','age','active'))
FIELDS.append('tx_fail')
COUNTERS=['observed','gaps','missed','tx_fail'] + [f'{k}{i}' for i in range(6) for k in ('entries','duration','maximum')]
FAULTS=[f'Overlap {i}/{j}' for i,j in PAIRS]+['Acquisition gap','Counter exhaustion','Watchdog unavailable']

def validate(s):
    def require(condition, reason):
        if not condition:
            raise ValueError(reason)
    require(all(type(s[k]) is int and 0<=s[k]<=0xffffffff for k in FIELDS),'range')
    require(s['levels']<=15 and s['valid']<=15 and s['faults']<=511,'bit fields')
    require(s['halted']<=1 and not (s['halted'] and s['valid']),'halted validity')
    require(bool(s['halted'])==bool(s['faults']&384),'halt flag')
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']),'gap accounting')
    require(bool(s['gaps'])==bool(s['faults']&64),'gap flag')
    for k,(i,j) in enumerate(PAIRS):
        entries,duration,maximum,age,active=(s[f'{name}{k}'] for name in ('entries','duration','maximum','age','active'))
        require(active<=1 and age<=maximum<=duration,'pair bounds')
        require(entries or not (duration or maximum or active),'missing entry')
        require(active or not age,'inactive age')
        require(entries!=1 or maximum==duration,'single segment maximum')
        require(maximum*entries>=duration,'segment duration bound')
        bits=(1<<i)|(1<<j)
        on=not s['halted'] and s['valid']&bits==bits and s['levels']&bits==bits
        require(bool(active)==bool(on),'active levels')
        require(bool(entries)==bool(s['faults']&(1<<k)),'pair flag')
