# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""BR1 necessary snapshot and transition invariants; not authentication."""
FIELDS = ('now levels valid halted faults gaps missed state grade remaining age '
          'attempts failures successes abandoned early malformed timeouts resets coverage tx_fail').split()
COUNTERS = 'gaps missed attempts failures successes abandoned early malformed timeouts resets coverage tx_fail'.split()
FAULTS = ['early retry','ambiguous event','sample gap','counter exhausted',
          'watchdog unavailable','response timeout']

def validate(s):
    def require(value):
        if not value:
            raise ValueError('impossible BR1 snapshot')
    require(all(isinstance(s[k],int) and 0 <= s[k] <= 0xffffffff for k in FIELDS))
    require(s['levels'] <= 15 and s['valid'] <= 15 and s['faults'] <= 63)
    require(s['state'] <= 2 and s['grade'] <= 7 and s['halted'] <= 1)
    require(bool(s['halted']) == bool(s['faults'] & 24))
    require(not s['halted'] or not (s['valid'] or s['state']))
    require(not s['state'] or (s['valid']==15 and not s['levels'] & 8 and s['resets']))
    require(s['state'] or not (s['grade'] or s['remaining'] or s['age']))
    delay = (10 << (s['grade']-1)) if s['grade'] else 0
    require(s['remaining'] <= delay and s['grade'] <= s['failures'])
    require(s['state']==2 or not s['age'])
    require(s['age'] <= 1000)
    require(s['attempts']==s['failures']+s['successes']+s['abandoned']+(s['state']==2))
    require(s['early'] <= s['attempts'] <= s['coverage'])
    require(s['timeouts'] <= s['abandoned'] and s['malformed'] <= s['coverage'])
    require(not s['early'] or s['failures'])
    require(s['missed'] >= s['gaps'] and (s['gaps'] or not s['missed']))
    for key,bit in [('early',1),('malformed',2),('gaps',4),('timeouts',32)]:
        require(bool(s[key]) == bool(s['faults'] & bit))

def transition(p,s,delta):
    """Sparse records need necessary bounds, not an invented exact event trace."""
    growth={k:s[k]-p[k] for k in COUNTERS}
    if growth['abandoned'] > growth['attempts']+(p['state']==2):
        return 'Abandonment exceeds available attempts'
    if growth['early'] > growth['attempts']:
        return 'Early retries exceed new attempts'
    if growth['attempts'] > growth['coverage']:
        return 'Attempts exceed covered observations'
    stable = (s['state'] and p['state'] and not growth['gaps'] and
              not growth['resets'] and not growth['abandoned'] and not s['halted'])
    if stable:
        if not growth['failures'] and not growth['successes']:
            if s['grade'] != p['grade'] or s['remaining'] != max(0,p['remaining']-delta):
                return 'Backoff changed without an outcome'
        if not growth['successes'] and s['grade'] != min(7,p['grade']+growth['failures']):
            return 'Escalation inconsistent with failures'
        if not growth['attempts'] and p['state']==2 and s['state']==2 and s['age']!=p['age']+delta:
            return 'Pending age inconsistent with elapsed time'
    return ''
