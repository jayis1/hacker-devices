# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Sparse snapshots must preserve first-use labels even on vacant slots."""
import unittest
from test_app import allocated
from contract import validate, transition
from console import Model, frame

class AffinityTests(unittest.TestCase):
    def test_vacant_label_cannot_change_after_drain(self):
        a=allocated()
        a.update(active=0,completed=1,rounds=1,attempts=2)
        b=dict(a,now=200,observed=197,order=5,launched=2,completed=2,rounds=2,attempts=4)
        validate(a); validate(b)
        self.assertTrue(transition(a,b,100))
        m=Model('synthetic impossible affinity change','replay')
        m.feed(frame('P',[3,1,5,2])+frame('S',a.values())+frame('S',b.values()))
        self.assertTrue(m.frozen); self.assertEqual(m.latest,a)

    def test_first_binding_other_slot_can_differ(self):
        a=allocated()
        b=dict(a,now=200,observed=197,order=14,active=3,launched=2,attempts=2,queued=1,peak=2)
        validate(a); validate(b); self.assertFalse(transition(a,b,100))

    def test_new_history_needs_allocation_and_release(self):
        a=allocated()
        a.update(launched=2,completed=1,attempts=3,rounds=1)
        b=dict(a,now=200,observed=197,order=14)
        validate(b); self.assertTrue(transition(a,b,100))
        b.update(launched=3,completed=2,attempts=5,queued=1,peak=2)
        validate(b); self.assertFalse(transition(a,b,100))

if __name__=='__main__': unittest.main()
