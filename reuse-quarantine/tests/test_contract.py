# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from contract import FIELDS, validate, transition

def baseline():
    s=dict.fromkeys(FIELDS,0)
    s.update(now=20,valid=15,phase=1,stable=5,observed=17)
    return s

def cooling():
    s=baseline()
    s.update(now=100,observed=97,attempts=2,launched=1,completed=1,
             rounds=1,peak=1,cooling=1,remaining0=250)
    return s

class ContractTests(unittest.TestCase):
    def test_halt_uses_one_acquisition_clock_for_both_timers(self):
        a=cooling()
        a.update(cooling=3,remaining0=200,remaining1=980,launched=2,
                 completed=2,attempts=4,rounds=1,peak=2)
        b=dict(a,now=120,observed=107,halted=1,valid=0,faults=16,
               gaps=1,missed=9,remaining0=190,remaining1=960)
        validate(a); validate(b)
        self.assertTrue(transition(a,b,20))
        b['remaining1']=970
        self.assertFalse(transition(a,b,20))
    def test_renewal_must_wait_for_old_timer(self):
        a=cooling()
        b=dict(a,now=400,observed=397,remaining0=100,launched=2,
               completed=2,expired=1,rounds=2,attempts=4)
        validate(b)
        self.assertTrue(transition(a,b,300))
        b['remaining0']=220
        self.assertFalse(transition(a,b,300))
    def test_early_expiry(self):
        a=cooling(); b=dict(a,now=349,observed=346,cooling=0,remaining0=0,expired=1)
        validate(a); validate(b)
        self.assertTrue(transition(a,b,249))
        b.update(now=350,observed=347)
        self.assertFalse(transition(a,b,250))
    def test_remaining_clock(self):
        a=cooling(); b=dict(a,now=120,observed=117,remaining0=229)
        self.assertTrue(transition(a,b,20))
        b['remaining0']=230
        self.assertFalse(transition(a,b,20))
    def test_timer_migration(self):
        a=cooling(); b=dict(a,now=120,observed=117,cooling=2,remaining0=0,remaining1=230)
        self.assertTrue(transition(a,b,20))
    def test_invariants(self):
        for changes in [dict(active=1),dict(remaining0=0),dict(expired=1),dict(remaining1=2)]:
            with self.assertRaises(ValueError): validate(dict(cooling(),**changes))
    def test_fault_freezes_before_gap_interval(self):
        a=cooling(); b=dict(a,now=120,halted=1,valid=0,faults=16,gaps=1,missed=19)
        validate(b)
        self.assertFalse(transition(a,b,20))
    def test_new_timer_requires_return_and_time(self):
        a=baseline(); b=cooling()
        self.assertFalse(transition(a,b,80))
        b['remaining0']=999
        self.assertFalse(transition(a,b,80))
        b['remaining0']=600
        self.assertTrue(transition(a,b,80))

if __name__=='__main__': unittest.main()
