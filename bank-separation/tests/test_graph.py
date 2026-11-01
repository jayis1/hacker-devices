# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Independent graph-policy checks, including sparse hidden admissions."""
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from contract import FIELDS, validate, transition, eligible

def state(active=0, uses=(0,0,0,0), completed=0, rounds=0, queued=0, peak=0):
    s=dict.fromkeys(FIELDS,0)
    s.update(now=100,valid=15,phase=1,stable=5,observed=97,active=active,
             depth=active.bit_count(),launched=sum(uses),completed=completed,
             rounds=rounds,queued=queued,peak=peak,attempts=sum(uses)+completed)
    s.update({'use'+str(i):n for i,n in enumerate(uses)})
    return s

class GraphTests(unittest.TestCase):
    def test_independent_sets(self):
        legal={0,1,2,4,8,5,9,10}
        for mask in range(16):
            s=state(mask,tuple(int(bool(mask&(1<<i))) for i in range(4)),
                    queued=max(mask.bit_count()-1,0),peak=mask.bit_count())
            if mask in legal: validate(s)
            else:
                with self.assertRaises(ValueError): validate(s)
    def test_candidates(self):
        self.assertEqual(eligible(1),[2,3])
        self.assertEqual(eligible(5),[])
        self.assertEqual(eligible(8),[0,1])
    def test_hidden_conflict_with_continuous_incumbent(self):
        a=state(1,(1,0,0,0),peak=1)
        # Tag 1 allegedly enters and leaves while tag 0 never releases.
        b=state(1,(1,1,0,0),completed=1,queued=1,peak=2)
        validate(a); validate(b)
        self.assertTrue(transition(a,b,100))
    def test_release_blocker_then_allocate_is_permitted(self):
        a=state(1,(1,0,0,0),peak=1)
        b=state(2,(1,1,0,0),completed=1,rounds=1,peak=1)
        validate(a); validate(b)
        self.assertFalse(transition(a,b,100))
    def test_repeat_use_not_fairness(self):
        a=state(0,(1,0,0,0),completed=1,rounds=1,peak=1)
        b=state(1,(2,0,0,0),completed=1,rounds=1,peak=1)
        validate(a); validate(b)
        self.assertFalse(transition(a,b,100))

if __name__=='__main__': unittest.main()
