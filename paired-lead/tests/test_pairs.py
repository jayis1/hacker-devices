# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exhaustive small endpoint checks, separately from the C signed-walk oracle."""
import itertools
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from contract import FIELDS, POLICY, balances, eligible, validate
from console import Model, frame


def empty_pool(uses):
    count=sum(uses)
    s=dict.fromkeys(FIELDS,0)
    s.update(now=10000,valid=15,phase=1,stable=5,observed=9997,
             launched=count,completed=count,rounds=count,attempts=2*count,
             peak=int(bool(count)))
    for i,value in enumerate(uses): s['use'+str(i)]=value
    return s


class PairContract(unittest.TestCase):
    def test_small_count_cartesian_product(self):
        for uses in itertools.product(range(6),repeat=4):
            s=empty_pool(uses)
            accepted=all(-2<=uses[i]-uses[i+1]<=2 for i in (0,2))
            if accepted:
                validate(s)
                self.assertEqual(balances(s),[uses[0]-uses[1],uses[2]-uses[3]])
                expected=[]
                for tag in range(4):
                    after=list(uses); after[tag]+=1
                    if all(abs(after[i]-after[i+1])<=2 for i in (0,2)):
                        expected.append(tag)
                self.assertEqual(eligible(s),expected)
            else:
                with self.assertRaises(ValueError): validate(s)

    def test_crc_valid_cross_pair_credit_transfer(self):
        # Every global conservation equation is satisfied. Aggregate balance
        # cannot pay the individual pair's debt, in either direction.
        for uses in ([3,0,0,3],[0,3,3,0]):
            m=Model('synthetic impossible endpoint','replay')
            s=empty_pool(uses)
            m.feed(frame('P',POLICY)+frame('S',[s[k] for k in FIELDS]))
            self.assertEqual(m.errors,1)
            self.assertIsNone(m.latest)

    def test_wrong_policy_cannot_authorize_status(self):
        m=Model('wrong pair map','replay')
        m.feed(frame('P',[3,1,5,4,2,2]))
        s=empty_pool([0,0,0,0])
        m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertTrue(m.frozen)
        self.assertIsNone(m.latest)

    def test_halted_evidence_is_immutable(self):
        a=empty_pool([2,0,0,0])
        a.update(halted=1,valid=0,faults=1,rejected=1,attempts=5)
        b=dict(a,now=10100,observed=10097)
        m=Model('synthetic halted change','replay')
        m.feed(frame('P',POLICY))
        for s in (a,b): m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertEqual(m.latest,a)
        self.assertIn('Halted evidence changed',m.frozen)

    def test_no_catchup_credit_from_release(self):
        s=empty_pool([2,0,0,0])
        self.assertNotIn(0,eligible(s))
        # A legal admission by the partner, rather than its release, restores room.
        s=empty_pool([2,1,0,0])
        self.assertIn(0,eligible(s))


if __name__=='__main__': unittest.main()
