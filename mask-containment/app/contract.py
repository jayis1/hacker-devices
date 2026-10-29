# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""MC1 independent permission containment: necessary evidence bounds."""
FIELDS='now levels valid halted faults gaps missed excess denied'.split()
FIELDS += [f'b{i}' for i in range(8)]
FIELDS += [f'{prefix}{i}' for prefix in 'ecx' for i in range(4)]
FIELDS += ['tx_fail']
COUNTERS='gaps missed tx_fail'.split()+[f'b{i}' for i in range(8)]+[f'{p}{i}' for p in 'ex' for i in range(4)]
FAULTS=['Excess privilege 0','Excess privilege 1','Acquisition gap','Counter exhaustion','Watchdog unavailable']

def validate(s):
    def require(condition, reason):
        if not condition:
            raise ValueError(reason)
    require(all(type(s[k]) is int and 0<=s[k]<=0xffffffff for k in FIELDS),'range')
    require(s['levels']<=15 and s['valid']<=15 and s['faults']<=31,'bit fields')
    require(s['halted']<=1 and s['excess']<=3 and s['denied']<=3,'flags')
    require(bool(s['halted'])==bool(s['faults']&24),'halt flag')
    require(not s['halted'] or not (s['valid'] or s['excess'] or s['denied']),'halt state')
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']),'gap accounting')
    require(bool(s['gaps'])==bool(s['faults']&4),'sticky gap')
    excess=denied=0
    for lane in range(2):
        mask=5<<lane
        valid=s['valid']&mask==mask
        state=((s['levels']>>lane)&1)|(((s['levels']>>(lane+2))&1)<<1)
        high=valid and state==2
        low=valid and state==1
        if high: excess|=1<<lane
        if low: denied|=1<<lane
        require(not valid or s[f'b{4*lane+state}']>0,'unrecorded current state')
        for index,kind,active in ((lane,2,high),(lane+2,1,low)):
            total=s[f'b{4*lane+kind}']
            count,current,maximum=(s[f'{p}{index}'] for p in 'ecx')
            require(bool(count)==bool(total)==bool(maximum),'episode presence')
            require(count<=total and current<=maximum<=total,'episode bounds')
            require(bool(current)==bool(active),'active streak')
            require(not count or maximum+count-1<=total,'maximum feasibility')
            require(count!=1 or maximum==total,'single episode maximum')
            require(count!=1 or not active or current==total,'single active episode')
        require(bool(s[f'e{lane}'])==bool(s['faults']&(1<<lane)),'sticky excess')
    require(s['excess']==excess and s['denied']==denied,'containment flags')
