# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""TM1 wire contract. Necessary invariants, not cryptographic authenticity."""
FIELDS='now levels valid ready halted faults captured active age starts acks completed abandoned setup_fail hold overtake orphan overlap timeouts gaps missed max_latency tx_fail'.split()
COUNTERS=FIELDS[9:]
FAULTS=['Insufficient setup','Payload changed','Request overtook acknowledgment',
        'Unsolicited acknowledgment','Simultaneous request and acknowledgment',
        'Acknowledgment timeout','Acquisition gap','Counter exhaustion','Watchdog unavailable']

def validate(s):
    def require(condition, reason):
        if not condition:
            raise ValueError(reason)
    require(all(isinstance(s[k],int) and 0<=s[k]<=0xffffffff for k in FIELDS),'range')
    require(s['levels']<=15 and s['valid']<=15 and s['captured']<=3,'bit fields')
    require(all(s[k]<=1 for k in ('ready','halted','active')) and s['faults']<=511,'state fields')
    equal=((s['levels']>>2)&1)==((s['levels']>>3)&1)
    if s['active']:
        require(s['ready'] and not equal and s['age']<=1000,'active phase')
        require((s['levels']&3)==s['captured'],'active payload')
    else:
        require(s['age']==0,'inactive age')
    if s['ready']:
        require(s['valid']==15 and not s['halted'] and not s['faults'],'readiness')
        require(s['active'] or equal,'idle phase')
    require(s['starts']==s['completed']+s['abandoned']+s['active'],'conservation')
    require(s['completed']<=s['acks'] and s['max_latency']<=1000,'completion bounds')
    require(bool(s['completed'])==bool(s['max_latency']),'latency evidence')
    require(s['missed']>=s['gaps'] and (s['gaps'] or not s['missed']),'gap accounting')
    require(all(s[k]<=s['abandoned'] for k in ('setup_fail','hold','timeouts','overlap')),'abandonment bounds')
    require(2*s['overtake']<=s['abandoned'],'overtake accounting')
    require(s['orphan']<=s['acks']-s['completed'],'orphan accounting')
    failures=['setup_fail','hold','overtake','orphan','overlap','timeouts','gaps']
    require(sum(s[k] for k in failures[:-1])<=1,'correlation stops on first failure')
    for i,key in enumerate(failures):
        require(bool(s[key])==bool(s['faults']&(1<<i)),'finding consistency')
    require(bool(s['halted'])==bool(s['faults']&384),'halt consistency')
