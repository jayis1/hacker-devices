# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import binascii
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
SPEC = importlib.util.spec_from_file_location('console', Path(__file__).resolve().parents[1]/'app/console.py')
assert SPEC is not None and SPEC.loader is not None
app = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(app)
ROOT = Path(__file__).resolve().parents[1]
def packet(body):
    data = body.encode()
    return data + f'*{binascii.crc_hqx(data,65535):04X}\n'.encode()
def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/fixture')])
class Tests(unittest.TestCase):
    def test_real_encoder_and_export(self):
        m = app.Model('synthetic', False)
        m.feed(fixture(), 0)
        m.eof()
        self.assertEqual(m.errors,0)
        self.assertEqual(m.status['invalid'],1)
        self.assertEqual(m.status['unsafe'],1)
        self.assertEqual(m.status['silent'],1)
        self.assertEqual(len(m.history),11)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'report.json'
            m.export(p,1)
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(p,2)
    def test_framing(self):
        m=app.Model('test',False)
        m.feed(b'DW1,S,12'+fixture(),0)
        self.assertEqual(m.status['now'],1000)
        self.assertGreater(m.errors,0)
        m.feed(b'garbage'*1000,0)
        self.assertLessEqual(len(m.buffer),384)
        m.feed(b'DW1,S,',0); m.eof()
        self.assertGreater(m.errors,1)
    def test_invalid_records(self):
        m=app.Model('test',False)
        m.feed(fixture(),0)
        n=m.errors
        for body in ['DW1,P,99,20','DW1,S,-1','DW1,S,4294967296','DW1,X,1']:
            m.feed(packet(body),0)
        self.assertGreaterEqual(m.errors,n+4)
        m.feed(packet('DW1,P,3,20'),0)
        v=list(m.status.values()); v[8]=99999
        m.feed(packet('DW1,S,'+','.join(map(str,v))),0)
        self.assertGreater(m.errors,n+4)
    def test_freshness_duplicates_and_reset(self):
        m=app.Model('test',True); m.connected=True
        lines=fixture().splitlines(keepends=True)
        m.feed(b''.join(lines[:4]),0)
        self.assertTrue(m.fresh(0.5))
        m.feed(lines[3],0.8)
        self.assertFalse(m.fresh(1.1))
        m.feed(lines[4],2)
        self.assertTrue(m.frozen)
        m.feed(packet('DW1,P,3,20'),2)
        self.assertTrue(m.frozen)
    def test_counter_rollback(self):
        m=app.Model('test',False); m.feed(fixture(),0)
        v=list(m.status.values()); v[0]+=100
        v[6]=v[9]=v[12]=0; v[4]&=~2
        m.feed(packet('DW1,S,'+','.join(map(str,v))),0)
        self.assertTrue(m.frozen)
    def test_replay_limits(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'pipe'; os.mkfifo(p)
            with self.assertRaises(ValueError): app.read_replay(p)
            p=Path(d)/'large'; p.write_bytes(b'x'*(app.MAX_REPLAY+1))
            with self.assertRaises(ValueError): app.read_replay(p)
            p=Path(d)/'empty'; p.write_bytes(b'')
            m=app.Model('test',False); m.feed(app.read_replay(p),0); m.eof()
            self.assertGreater(m.errors,0)
if __name__=='__main__': unittest.main()
