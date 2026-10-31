# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Domain changes require pool drain, even across sparse valid snapshots."""
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from contract import validate, transition
from console import Model, frame
from test_app import allocated

class CohortTests(unittest.TestCase):
    def test_switch_requires_closed_cohort(self):
        a = allocated()
        a.update(active=3, order=12, attempts=2, launched=2, queued=1, peak=2)
        b = dict(a, now=200, observed=197, order=15, attempts=6,
                 launched=4, completed=2, queued=3)
        validate(a)
        validate(b)
        self.assertTrue(transition(a, b, 100))
        m = Model('synthetic impossible domain migration', 'replay')
        m.feed(frame('P', [3,1,5,2]) + frame('S', a.values()) + frame('S', b.values()))
        self.assertTrue(m.frozen)
        self.assertEqual(m.latest, a)
        b.update(rounds=1, queued=2)
        validate(b)
        self.assertFalse(transition(a, b, 100))

    def test_same_domain_rolling_reuse_needs_no_drain(self):
        a = allocated()
        a.update(active=3, order=12, attempts=2, launched=2, queued=1, peak=2)
        b = dict(a, now=200, observed=197, attempts=6,
                 launched=4, completed=2, queued=3)
        validate(a)
        validate(b)
        self.assertFalse(transition(a, b, 100))

if __name__ == '__main__':
    unittest.main()
