# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import binascii
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('console',ROOT/'app/console.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

def frame(body):
    body = body.encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

def policy(model):
    model.feed(frame('LP1,P,1000,10000,32,5'))

def status(model, **kwargs):
    data = dict(time=100,state=1,levels=4,pairs=1,sum_a=10,sum_b=15,
                a_slower=0,b_slower=1,equal=0,large=1,max_delta=5,
                rejected=0,gaps=0,tx_fail=0,faults=0)
    data.update(kwargs)
    model.feed(frame('LP1,S,'+','.join(str(data[k]) for k in app.FIELDS)))

class Tests(unittest.TestCase):
    def test_policy_and_status(self):
        m=app.Model('test'); status(m); self.assertFalse(m.rows)
        policy(m); status(m); self.assertEqual(m.rows[-1]['pairs'],1)
    def test_corruption_and_resync(self):
        m=app.Model('test'); m.feed(b'LP1,S,partial'); policy(m); status(m)
        self.assertEqual(len(m.rows),1); self.assertEqual(m.errors,1)
        m.feed(b'LP1,B,0*0000\n'); self.assertEqual(m.errors,2)
    def test_impossible(self):
        for bad in [dict(pairs=33),dict(sum_a=0),dict(a_slower=2),dict(large=0),
                    dict(max_delta=1000),dict(gaps=1),dict(state=3),
                    dict(faults=256),dict(sum_b=1001),dict(rejected=1),
                    dict(faults=128),dict(equal=1)]:
            m=app.Model('test'); policy(m); status(m,**bad)
            self.assertEqual(m.errors,1,bad); self.assertFalse(m.rows)
    def test_cross_field_impossibility(self):
        for bad in [dict(b_slower=0,equal=1),dict(max_delta=999),
                    dict(state=2,levels=0),dict(state=1,levels=5),
                    dict(a_slower=1,b_slower=0)]:
            m=app.Model('test'); policy(m); status(m,**bad)
            self.assertEqual(m.errors,1,bad)
            self.assertFalse(m.rows)
    def test_monotonic_and_duplicate(self):
        m=app.Model('test'); policy(m); status(m); stamp=m.updated
        status(m); self.assertEqual(m.updated,stamp)
        status(m,levels=6); self.assertTrue(m.frozen)
        m=app.Model('test'); policy(m); status(m,time=200)
        status(m,time=100); self.assertTrue(m.frozen)
        m=app.Model('test'); policy(m); status(m)
        status(m,time=200,sum_a=9,sum_b=14); self.assertTrue(m.frozen)
    def test_eof_and_export(self):
        m=app.Model('replay: test'); policy(m); status(m); m.feed(b'LP1,S,')
        m.finish(); self.assertEqual(m.errors,1); self.assertTrue(m.frozen)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'export.json'; m.export(p,False)
            evidence=json.loads(p.read_text())
            self.assertEqual(evidence['author'],'jayis1')
            self.assertTrue(evidence['provenance'].startswith('replay:'))
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p,False)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): app.read_replay(fifo)
    def test_real_encoder(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m=app.Model('synthetic C fixture'); m.feed(data); m.finish()
        self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        self.assertEqual(m.rows[-1]['pairs'],32)
        self.assertEqual(m.rows[-1]['large'],32)
        self.assertEqual(m.rows[-1]['state'],3)
    def test_policy_mismatch_and_reset(self):
        m=app.Model('test'); policy(m); status(m)
        m.feed(frame('LP1,B,0')); self.assertTrue(m.frozen)
        m=app.Model('test'); m.feed(frame('LP1,P,1,2,3,4'))
        self.assertTrue(m.frozen)

if __name__=='__main__': unittest.main()
