# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import itertools
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from contract import eligible, remaining, validate, transition, FIELDS

class QuotaTests(unittest.TestCase):
    def test_all_remaining_and_eligibility(self):
        for used in itertools.product(range(2),range(3),range(4),range(5)):
            for mask in range(16):
                s=dict.fromkeys(FIELDS,0)
                s.update(active=mask,**{'use'+str(i):u for i,u in enumerate(used)})
                self.assertEqual(remaining(s),[i+1-used[i] for i in range(4)])
                self.assertEqual(eligible(s),[i for i in range(4) if not mask&(1<<i) and used[i]<i+1])
    def test_budget_cannot_migrate_or_refund(self):
        a=dict.fromkeys(FIELDS,0)
        a.update(now=100,valid=15,phase=1,stable=5,observed=97,rounds=1,
                 launched=1,completed=1,attempts=2,peak=1,use0=1)
        validate(a)
        b=dict(a,now=200,observed=197,use0=0,use1=1)
        validate(b)
        self.assertTrue(transition(a,b,100))
        b=dict(a,now=200,observed=197,use0=2,launched=2,completed=2,attempts=4,rounds=2)
        with self.assertRaises(ValueError): validate(b)
    def test_another_resource_retains_its_own_budget(self):
        a=dict.fromkeys(FIELDS,0)
        a.update(now=100,valid=15,phase=1,stable=5,observed=97,rounds=1,
                 launched=1,completed=1,attempts=2,peak=1,use0=1)
        b=dict(a,now=200,observed=197,use1=1,launched=2,attempts=3,depth=1,active=2)
        validate(a); validate(b)
        self.assertFalse(transition(a,b,100))

if __name__=='__main__': unittest.main()
