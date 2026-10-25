# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
spec=importlib.util.spec_from_file_location('audit',Path(__file__).resolve().parents[1]/'app/audit.py')
assert spec is not None and spec.loader is not None
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def frame(kind,values):
    body=('EW1,'+kind+','+','.join(map(str,values))).encode()
    return body+b'*'+f'{audit.crc(body):04X}'.encode()+b'\n'
def status(t=100,completed=0):
    return [t,0,1,1,0,0,completed,0,0,0,0,0,0,0,0]
class TestApp(unittest.TestCase):
    def model(self):
        m=audit.Model('synthetic'); m.connected=True
        m.feed(frame('P',[100,500,5000,3])); return m
    def test_fixture(self):
        m=self.model()
        data=subprocess.check_output([str(Path(__file__).resolve().parents[1]/'firmware/build/fixture')])
        for byte in data: m.feed(bytes([byte]))
        self.assertEqual(m.errors,0)
        self.assertEqual(m.status[6],1)
        self.assertEqual(m.status[8],1)
        self.assertTrue(m.fresh())
    def test_late_policy_and_freshness(self):
        m=audit.Model('synthetic'); m.connected=True
        m.feed(frame('S',status())); self.assertFalse(m.fresh())
        m.feed(frame('P',[100,500,5000,3])); self.assertTrue(m.fresh())
        m.received=time.monotonic()-2; self.assertFalse(m.fresh())
        m.received=time.monotonic(); m.replay=True; self.assertFalse(m.fresh())
    def test_partial_and_oversized(self):
        m=self.model(); m.feed(b'EW1,S,12'+frame('S',status()))
        self.assertEqual(m.status[0],100); self.assertGreater(m.errors,0)
        m.feed(b'x'*10000+frame('S',status(101)))
        self.assertEqual(m.status[0],101); self.assertLessEqual(len(m.buffer),384)
    def test_bounds_and_crc(self):
        m=self.model()
        for record in [b'EW1,S,1*0000\n',frame('S',[1]),frame('S',status(2**32)),
                       frame('E',[0,0,9,0]),frame('S',[100,16]+status()[2:]),
                       frame('S',[100,0,1,6]+status()[4:]),frame('S',['-1']+status()[1:])]:
            before=m.errors; m.feed(record); self.assertEqual(m.errors,before+1)
    def test_reset_and_policy(self):
        m=self.model(); m.feed(frame('S',status(100,2)))
        m.feed(frame('S',status(110,1))); self.assertTrue(m.restart)
        m.feed(frame('P',[100,500,5000,3])); self.assertTrue(m.restart)
        m.feed(frame('B',[0,100,500,5000,3])); self.assertFalse(m.restart)
        self.assertIsNone(m.status)
        m.feed(frame('S',status(50))); m.feed(frame('S',status(40)))
        self.assertTrue(m.restart)
    def test_incompatible(self):
        m=self.model(); m.feed(frame('P',[100,500,1000,3]))
        m.feed(frame('S',status())); self.assertIsNone(m.status)
        self.assertTrue(m.incompatible); self.assertFalse(m.fresh())
    def test_wrap_and_events(self):
        m=self.model(); m.feed(frame('S',status(0xfffffffe)))
        m.feed(frame('S',status(2))); self.assertFalse(m.restart)
        for i in range(300): m.feed(frame('E',[i,0,1,0]))
        self.assertEqual(len(m.events),256); self.assertFalse(m.restart)
    def test_export_and_screens(self):
        m=self.model(); m.feed(frame('S',status())); m.replay=True; m.connected=False
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'e.json'; m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            self.assertIn('"replay": true',p.read_text())
        for page,label in [(1,'ERASE HANDSHAKE'),(2,'EVENT HISTORY'),(3,'HEALTH')]:
            self.assertIn(label,'\n'.join(audit.screen_lines(m,page)))
if __name__=='__main__': unittest.main()
