# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Real encoder integration plus independently forged endpoint regressions."""
import copy
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from console import Model, frame, read_replay
from contract import FIELDS, eligible, validate, transition
ROOT=Path(__file__).resolve().parents[1]

class ContractTests(unittest.TestCase):
    def fixture(self,mode='legal'):
        data=subprocess.check_output([str(ROOT/'firmware/build/fixture'),mode])
        model=Model('synthetic C fixture','replay'); model.feed(data); model.finish()
        self.assertEqual(model.errors,0); self.assertFalse(model.frozen)
        return model,data
    def test_real_encoder(self):
        model,_=self.fixture()
        self.assertEqual(model.latest['launched'],7)
        self.assertEqual(model.latest['completed'],3)
        self.assertEqual(eligible(model.latest),[1,3])
        model,_=self.fixture('early')
        self.assertEqual(model.latest['faults'],1)
    def test_all_sparse_pairs(self):
        model,_=self.fixture()
        rows=list(model.history)
        for i,a in enumerate(rows):
            for b in rows[i+1:]:
                self.assertEqual(transition(a,b,b['now']-a['now']),'')
    def test_young_resident_cannot_disappear(self):
        model,_=self.fixture(); a=model.history[1]; b=copy.deepcopy(a)
        b.update(now=a['now']+100,active=0,depth=0,completed=1,attempts=2,rounds=1)
        with self.assertRaises(ValueError): validate(b)
    def test_sparse_early_return_and_renewal(self):
        model,_=self.fixture(); a=copy.deepcopy(model.latest)
        # Swap latest admission ownership while preserving endpoint validity.
        b=copy.deepcopy(a); b['now']+=100; b['launched']+=1
        b['completed']+=1; b['queued']+=1; b['attempts']+=2
        b['observed']+=100; b['use0']+=1; b['stamp0']=b['launched']
        with self.assertRaises(ValueError): validate(b)
        self.assertTrue(transition(a,b,100))
    def test_corruption_tail_and_private_export(self):
        model,data=self.fixture()
        damaged=Model('synthetic','replay'); damaged.feed(b'RT1,S,12'+data); damaged.finish()
        self.assertGreater(damaged.errors,0); self.assertIsNotNone(damaged.latest)
        damaged.feed(b'RT1,S'); damaged.finish(); self.assertGreater(damaged.errors,1)
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'evidence.json'; damaged.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): damaged.export(path)
            fifo=Path(directory)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
    def test_crc_valid_invalid_status(self):
        model,_=self.fixture(); bad=dict(model.latest); bad['rounds']=1
        model.feed(frame('S',[bad[k] for k in FIELDS]))
        self.assertEqual(model.errors,1)

if __name__=='__main__': unittest.main()
