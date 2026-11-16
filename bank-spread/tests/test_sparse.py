# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Check legal sparse paths with an independent occupancy-array oracle."""
import unittest
from test_app import snapshot, stream
from console import Model
from contract import validate, transition

class SparseTests(unittest.TestCase):
    def test_no_acquisition_cannot_invent_admission_at_halt(self):
        a=snapshot([(0,True)])
        b=snapshot([(0,True),(2,True)])
        b.update(observed=a['observed'],halted=1,valid=0,faults=32)
        validate(b)
        m=Model('synthetic impossible exhaustion endpoint','replay')
        m.feed(stream(a,b))
        self.assertTrue(m.frozen)

    def test_all_legal_sparse_prefix_pairs(self):
        checked=0
        def walk(ops,live,ancestors,left):
            nonlocal checked
            b=snapshot(ops)
            validate(b)
            for a in ancestors:
                self.assertEqual(transition(a,b,b['now']-a['now']),'',ops)
                checked+=1
            if not left:
                return
            counts=[sum(live[:2]),sum(live[2:])]
            for tag in range(4):
                if live[tag]:
                    alloc=False
                elif counts[tag//2]==min(counts):
                    alloc=True
                else:
                    continue
                next_live=live.copy(); next_live[tag]=alloc
                walk(ops+[(tag,alloc)],next_live,ancestors+[b],left-1)
        walk([], [False]*4, [], 6)
        self.assertGreater(checked,1000)
        print('legal sparse prefix pairs:',checked)

if __name__=='__main__':
    unittest.main()
