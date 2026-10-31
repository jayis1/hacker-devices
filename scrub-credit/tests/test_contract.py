# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from contract import FIELDS, validate, transition

def state(active=0,clean=0,alloc=0,release=0,scrubs=0,rounds=0,peak=0):
    s=dict.fromkeys(FIELDS,0)
    s.update(now=100,valid=15,phase=1,stable=5,observed=97,active=active,clean=clean,
             launched=alloc,completed=release,scrubbed=scrubs,rounds=rounds,peak=peak,
             attempts=alloc+release+scrubs)
    validate(s)
    return s

class ContractTests(unittest.TestCase):
    def test_independent_credit_migration_rejected(self):
        a=state(clean=1,scrubs=1)
        b=state(clean=2,scrubs=1)
        self.assertTrue(transition(a,b,100))
    def test_reuse_requires_new_scrub(self):
        a=state(active=1,alloc=1,scrubs=1,peak=1)
        b=dict(a,launched=2,completed=1,rounds=1,attempts=4)
        with self.assertRaises(ValueError): validate(b)
        b.update(scrubbed=2,attempts=5); validate(b)
        self.assertFalse(transition(a,b,100))
    def test_swap_live_clean_requires_full_cycle(self):
        a=state(active=1,clean=2,alloc=1,scrubs=2,peak=1)
        b=dict(a,active=2,clean=1)
        self.assertTrue(transition(a,b,100))
        b.update(launched=2,completed=1,scrubbed=3,attempts=6,rounds=1)
        validate(b); self.assertFalse(transition(a,b,100))
    def test_all_slot_states_and_operation_words(self):
        # Separate list-state oracle; explore all legal interleavings to depth 7.
        a=state(); seen=0
        def walk(states,s,depth):
            nonlocal seen
            validate(s); self.assertFalse(transition(a,s,1000)); seen+=1
            if not depth: return
            for slot in range(2):
                t=dict(s); bits=list(states); old=bits[slot]; bits[slot]=(old+1)%3
                key=['scrubbed','launched','completed'][old]; t[key]+=1; t['attempts']+=1
                t['active']=sum(1<<i for i in range(2) if bits[i]==2)
                t['clean']=sum(1<<i for i in range(2) if bits[i]==1)
                if old==2 and not t['active']: t['rounds']+=1
                t['peak']=max(t['peak'],t['active'].bit_count())
                self.assertFalse(transition(s,t,100))
                walk(bits,t,depth-1)
        walk([0,0],a,7); self.assertGreater(seen,200)
    def test_invalid(self):
        for key,value in [('clean',4),('active',4),('scrubbed',1),('peak',2),('rounds',1),('faults',3)]:
            a=state(); a[key]=value
            with self.assertRaises(ValueError): validate(a)

if __name__=='__main__': unittest.main()
