# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import binascii
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('console',ROOT/'app/console.py')
assert spec is not None and spec.loader is not None
app=importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)
def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/fixture')])
def frame(text):
    raw=text.encode()
    return raw+b'*'+f'{binascii.crc_hqx(raw,65535):04X}\n'.encode()
class ModelTests(unittest.TestCase):
    def test_fairness_encoder(self):
        data=subprocess.check_output([str(ROOT/'firmware/build/fixture'),'fairness'])
        m=app.Model('synthetic fairness'); m.feed(data,1); m.finish()
        self.assertEqual(m.errors,0)
        self.assertEqual(m.rows[-1]['starved_a'],1)
        self.assertEqual(m.rows[-1]['starved_b'],1)
        self.assertEqual(m.rows[-1]['services_b'],3)
        row=m.rows[-1].copy(); row.update(time=260,wait_a=10)
        m.feed(frame('SW1,S,'+','.join(str(row[k]) for k in app.FIELDS)),2)
        self.assertEqual(m.errors,1)
    def test_real_encoder(self):
        m=app.Model('synthetic')
        m.feed(fixture(),1)
        m.finish()
        self.assertEqual(m.errors,0)
        self.assertEqual(m.rows[-1]['switches'],1)
        self.assertEqual(m.rows[-1]['early'],1)
        self.assertEqual(m.rows[-1]['overlaps'],1)
    def test_bad_crc_tail_and_empty(self):
        m=app.Model('test')
        m.feed(b'garbage\n'+fixture()+b'SW1,S,',1)
        self.assertEqual(m.errors,1)
        with self.assertRaises(ValueError): m.finish()
        with self.assertRaises(ValueError): app.Model('empty').finish()
    def test_duplicate_and_rollback(self):
        m=app.Model('test'); m.feed(fixture(),1)
        last=fixture().splitlines(keepends=True)[-1]
        m.feed(last,2); self.assertEqual(m.received,1)
        m.feed(fixture(),3); self.assertTrue(m.frozen)
    def test_impossible(self):
        m=app.Model('test'); m.feed(fixture(),1)
        row=m.rows[-1].copy(); row['switches']=4; row['time']=110
        m.feed(frame('SW1,S,'+','.join(str(row[k]) for k in app.FIELDS)),2)
        self.assertEqual(m.errors,1)
    def test_export_preserves_qualifications(self):

        m=app.Model('synthetic'); m.feed(fixture(),1); m.errors=2
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'evidence.json'; app.export(m,path)
            value=json.loads(path.read_text())
            self.assertEqual(value['author'],'jayis1')
            self.assertEqual(value['errors'],2)
            self.assertEqual(value['source'],'synthetic')
            self.assertEqual(path.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): app.export(m,path)
    def test_minimum_integrity(self):
        m=app.Model('test'); m.feed(fixture(),1)
        row=m.rows[-1].copy()
        row.update(time=110,early=0,h0=0,h1=0,h2=0,h3=1,minimum=20)
        m.feed(frame('SW1,S,'+','.join(str(row[k]) for k in app.FIELDS)),2)
        self.assertEqual(m.errors,1)
        m=app.Model('test')
        m.feed(frame('SW1,P,20,1,100,3'),1)
        row.update(minimum=120)
        m.feed(frame('SW1,S,'+','.join(str(row[k]) for k in app.FIELDS)),1)
        row.update(time=120,minimum=110)
        m.feed(frame('SW1,S,'+','.join(str(row[k]) for k in app.FIELDS)),2)
        self.assertTrue(m.frozen)
    def test_fifo_and_large_file(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): app.read_replay(p)
            p=Path(d)/'large'; p.write_bytes(b'x'*(app.MAX_REPLAY+1))
            with self.assertRaises(ValueError): app.read_replay(p)
if __name__=='__main__': unittest.main()
