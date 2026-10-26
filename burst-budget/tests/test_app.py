# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('console', ROOT/'app/console.py')
assert spec is not None and spec.loader is not None
app=importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/fixture')])

def packet(values):
    body=('BB1,S,'+','.join(map(str,values))).encode()
    return body+b'*'+f'{app.crc(body):04X}'.encode()+b'\n'

def live_fixture(m):
    for line in fixture().splitlines(keepends=True):
        stamp=int(line.split(b',')[2]) / 1000 if line.startswith(b'BB1,S,') else 0
        m.feed(line,stamp)

class Tests(unittest.TestCase):
    def test_fixture(self):
        m=app.Model(); m.feed(fixture(),0); m.finish()
        self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        self.assertEqual(m.status['admissions'],6)
        self.assertEqual(m.status['allowed'],4)
        self.assertEqual(m.status['excess'],2)
    def test_partial(self):
        m=app.Model(); m.feed(fixture()+b'BB1,S,',0); m.finish()
        self.assertEqual(m.errors,1)
    def test_empty(self):
        with self.assertRaises(ValueError): app.Model().finish()
    def test_resync(self):
        m=app.Model(); m.feed(b'BB1,S,123'+fixture(),0); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.status['admissions'],6)
    def test_duplicate_freshness(self):
        m=app.Model('live'); m.connected=True
        packets=fixture().splitlines(keepends=True)
        live_fixture(m)
        m.feed(packets[-1],5.3)
        self.assertFalse(m.fresh(5.6))
    def test_pause_freezes(self):
        m=app.Model('live'); m.connected=True; live_fixture(m)
        m.feed(fixture().splitlines(keepends=True)[-1],6.5)
        self.assertTrue(m.frozen)
    def test_refill_bound(self):
        m=app.Model(); m.feed(fixture(),0)
        values=list(m.status.values()); values[0]+=1; values[4]+=1; values[16]+=1; values[6]=0
        m.feed(packet(values),0)
        self.assertEqual(m.errors,1)
    def test_forward_clock(self):
        m=app.Model('live'); m.connected=True; live_fixture(m)
        values=list(m.status.values()); values[0]+=1000000
        m.feed(packet(values),4.6)
        self.assertTrue(m.frozen)
    def test_terminal_immutable(self):
        m=app.Model(); values=[0,2,0,0,0,0,0,64]+[0]*10
        m.feed(packet(values),0); values[0]=1000; values[4]=1; values[16]=1
        m.feed(packet(values),1)
        self.assertTrue(m.frozen)
    def test_private_export(self):
        m=app.Model(); m.feed(fixture(),0); m.finish()
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.json'; m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',p.read_text())
            with self.assertRaises(FileExistsError): m.export(p)
    def test_replay_special(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): app.read_replay(p)
    def test_oversize(self):
        m=app.Model(); m.feed(b'x'*10000,0)
        self.assertLess(len(m.buffer),384); self.assertGreater(m.errors,0)
    def test_impossible(self):
        m=app.Model(); m.feed(fixture(),0)
        values=list(m.status.values()); values[4]=5
        body=('BB1,S,'+','.join(map(str,values))).encode()
        frame=body+b'*'+f'{app.crc(body):04X}'.encode()+b'\n'
        m.feed(frame,0); self.assertEqual(m.errors,1)
if __name__=='__main__': unittest.main()
