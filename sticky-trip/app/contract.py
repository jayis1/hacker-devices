# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""ST1 necessary snapshot consistency bounds; not source authentication."""
FIELDS='now levels valid halted faults gaps missed known expected sets clears rejected coverage missing spurious'.split()
FIELDS += [f'{p}{i}' for p in 'tecx' for i in range(2)] + ['tx_fail']
COUNTERS='gaps missed sets clears rejected coverage tx_fail'.split()+[f'{p}{i}' for p in 'tex' for i in range(2)]
FAULTS=['Missing latch','Spurious latch','Acquisition gap','Counter exhaustion','Watchdog unavailable','Rejected clear edge']

def validate(s):
    def require(ok, reason):
        if not ok:
            raise ValueError(reason)
    require(all(type(s[k]) is int and 0 <= s[k] <= 0xffffffff for k in FIELDS),'integer range')
    require(s['levels'] <= 15 and s['valid'] <= 15 and s['faults'] <= 63,'bit fields')
    require(all(s[k] <= 1 for k in ['known','expected','halted']),'booleans')
    require(bool(s['halted']) == bool(s['faults'] & 24),'halt flags')
    require(not s['halted'] or not (s['valid'] or s['known']),'halt state')
    require(not s['known'] or (s['valid']==15 and s['coverage']>0),'known coverage')
    require(s['known'] or not s['expected'],'unknown obligation')
    require(not (s['known'] and s['levels']&1) or s['expected'],'set precedence')
    require(not s['expected'] or s['sets'],'set provenance')
    require(not (s['known'] and not s['expected']) or s['clears'],'clear provenance')
    require(not s['coverage'] or s['sets'] or s['clears'],'coverage provenance')
    require(s['sets']+s['clears'] <= s['coverage'],'transition coverage')
    require(s['t0']+s['t1'] <= s['coverage'],'mismatch coverage')
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']),'gap accounting')
    require(bool(s['gaps'])==bool(s['faults']&4),'gap flag')
    require(bool(s['rejected'])==bool(s['faults']&32),'rejected flag')
    missing=bool(s['known'] and s['expected'] and not s['levels']&8)
    spurious=bool(s['known'] and not s['expected'] and s['levels']&8)
    require(s['missing']==missing and s['spurious']==spurious,'current mismatch')
    for i,active in enumerate([missing,spurious]):
        total,n,current,maximum=(s[f'{p}{i}'] for p in 'tecx')
        require(bool(n)==bool(total)==bool(maximum),'episode presence')
        require(bool(current)==active,'active streak')
        require(n<=total and current<=maximum<=total,'episode bounds')
        require(not n or maximum+n-1<=total,'maximum feasibility')
        require(total<=n*maximum,'episode capacity')
        require(n!=1 or (maximum==total and (not active or current==total)),'single episode')
        require(bool(total)==bool(s['faults']&(1<<i)),'sticky mismatch')
