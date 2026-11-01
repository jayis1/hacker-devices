# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Coverage-set oracle, independent of the cumulative-count implementation."""
import itertools
from pathlib import Path
import subprocess
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from contract import FIELDS, POLICY, frontier, covered, eligible, validate, transition
from console import Model, frame

class CoverageTests(unittest.TestCase):
    def test_all_permuted_round_pairs(self):
        for first in itertools.permutations(range(4)):
            for second in itertools.permutations(range(4)):
                state=dict.fromkeys(FIELDS,0)
                state.update(valid=15,phase=1,stable=5,now=20,observed=17)
                mask=set(); epoch=0
                for tag in first+second:
                    self.assertIn(tag,eligible(state))
                    mask.add(tag)
                    if len(mask)==4: mask.clear(); epoch+=1
                    # A synthetic allocate/release cycle, not physical capture.
                    state['use'+str(tag)]+=1
                    for k,n in [('now',100),('observed',100),('launched',1),
                                ('completed',1),('attempts',2),('rounds',1)]: state[k]+=n
                    state['peak']=1
                    validate(state)
                    self.assertEqual(frontier(state),epoch)
                    self.assertEqual(set(covered(state)),mask)
                    self.assertEqual(set(eligible(state)),set(range(4))-mask)
    def test_busy_laggard_blocks_every_free_tag(self):
        s=dict.fromkeys(FIELDS,0)
        s.update(valid=15,phase=1,stable=5,now=500,observed=497,
                 use0=2,use1=2,use2=2,use3=1,active=8,depth=1,
                 peak=4,launched=7,completed=6,attempts=13,queued=6)
        validate(s)
        self.assertEqual(eligible(s),[])
        self.assertEqual(covered(s),[0,1,2])
    def test_balanced_forgery_is_not_coverage(self):
        s=dict.fromkeys(FIELDS,0)
        s.update(valid=15,phase=1,observed=100,peak=1,
                 use0=2,launched=2,completed=2,attempts=4,rounds=2)
        with self.assertRaises(ValueError): validate(s)
        m=Model('synthetic impossible snapshot','replay')
        m.feed(frame('P',POLICY)+frame('S',[s[k] for k in FIELDS]))
        self.assertEqual(m.errors,1)
        self.assertIsNone(m.latest)
    def test_c_encoder(self):
        m=Model('synthetic C encoder','replay')
        m.feed(subprocess.check_output([ROOT/'firmware/build/fixture']))
        m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual(covered(m.latest),[0,2])
        self.assertEqual(frontier(m.latest),0)
    def test_history_cannot_swap_covered_members(self):
        a=dict.fromkeys(FIELDS,0)
        a.update(valid=15,phase=1,observed=100,peak=1,
                 use0=1,launched=1,completed=1,attempts=2,rounds=1)
        b=dict(a,use0=0,use1=1,now=100,observed=200)
        validate(a); validate(b)
        self.assertTrue(transition(a,b,100))

    def test_successful_rounds_and_busy_c_encoder(self):
        for mode,expected_rounds,expected_covered,expected_eligible in [
                ('rounds',2,[],[]),('busy',1,[0,1,2],[])]:
            with self.subTest(mode=mode):
                m=Model('synthetic C '+mode,'replay')
                m.feed(subprocess.check_output([ROOT/'firmware/build/fixture',mode]))
                m.finish()
                self.assertEqual((m.errors,m.frozen),(0,''))
                self.assertEqual(m.latest['halted'],0)
                self.assertEqual(frontier(m.latest),expected_rounds)
                self.assertEqual(covered(m.latest),expected_covered)
                self.assertEqual(eligible(m.latest),expected_eligible)

if __name__=='__main__': unittest.main()
