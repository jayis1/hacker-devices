# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Synthetic production-encoder and hostile-stream tests, no physical device."""
import binascii
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit', ROOT/'app/audit.py')
assert spec is not None and spec.loader is not None
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def record(kind, values):
    body = ('EW1,' + kind + ',' + ','.join(map(str, values))).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

def fixture():
    return subprocess.check_output([ROOT/'firmware/build/fixture'])

class ModelTests(unittest.TestCase):
    def model(self):
        m = audit.Model('synthetic')
        m.feed(fixture())
        self.assertEqual(m.errors, 0)
        self.assertFalse(m.blocked)
        return m

    def test_encoder_and_partial_reads(self):
        m = audit.Model('synthetic')
        for byte in fixture():
            m.feed(bytes([byte]))
        self.assertEqual(m.latest['highest'], 2)
        self.assertEqual(m.latest['accepted'], 1)
        self.assertEqual(m.latest['downgrades'], 1)
        self.assertGreater(m.latest['execute_ms'], 0)
        self.assertEqual(m.latest['faults'], 48)
        self.assertTrue(m.events)
        self.assertEqual(m.errors, 0)

    def test_crc_bounds_and_resync(self):
        m = self.model()
        m.feed(b'x'*10000)
        m.feed(b'EW1,S,1,2' + record('P',audit.POLICY))
        self.assertLess(len(m.buffer), 384)
        self.assertGreater(m.errors, 0)
        m.feed(b'EW1,P,10,10,1000,3,20*0000\n')
        self.assertFalse(m.blocked)
        self.assertTrue(m.compatible)

    def test_duplicates_do_not_refresh(self):
        m = self.model()
        before = m.updated
        with patch.object(audit.time,'monotonic',return_value=before+3):
            m.feed(record('S', list(m.latest.values())))
            self.assertTrue(m.snapshot()['stale'])
            self.assertEqual(m.updated,before)

    def test_regressions_freeze(self):
        for key in ('accepted','downgrades','highest','faults','time'):
            with self.subTest(key=key):
                m=self.model(); v=m.latest.copy(); v['time']+=100
                if key=='time': v[key]=0
                else: v[key]=0
                m.feed(record('S',list(v.values())))
                self.assertTrue(m.blocked)

    def test_policy_mismatch_and_boot(self):
        for frame in (record('P',[0]*5),record('B',[0])):
            m=self.model(); m.feed(frame)
            self.assertTrue(m.blocked)
            m.feed(record('P',audit.POLICY))
            self.assertTrue(m.blocked)

    def test_late_attach(self):
        m=audit.Model('synthetic')
        m.feed(fixture().split(b'\n',1)[1])
        self.assertIsNotNone(m.latest)
        self.assertFalse(m.blocked)

    def test_contradictory_events(self):
        m=audit.Model('synthetic'); m.feed(record('P',audit.POLICY))
        m.feed(record('E',[1,10,3,2]))
        m.feed(record('E',[1,10,3,2]))
        self.assertFalse(m.blocked)
        m.feed(record('E',[1,11,4,1]))
        self.assertTrue(m.blocked)

    def test_impossible_valid_status_and_event(self):
        m=self.model(); v=m.latest.copy(); v['time']+=10
        v['valid']=1; v['current']=v['highest']; v['levels']=4
        m.feed(record('S',list(v.values())))
        self.assertEqual(m.errors,1)
        for kind,value in [(2,4),(3,0xffffffff),(4,4),(5,4),(6,256),(6,0),(7,0)]:
            m.feed(record('E',[100,250,kind,value]))
        self.assertEqual(m.errors,8)

    def test_impossible_exposure_and_ranges(self):
        for key,value in [('execute_ms',10000),('highest',4),('phase',3),('valid',1)]:
            m=self.model(); v=m.latest.copy(); v['time']+=1; v[key]=value
            # valid=1 contradicts current highest only if current is changed.
            if key=='valid': v['current']=0
            m.feed(record('S',list(v.values())))
            self.assertEqual(m.errors,1)

    def test_private_export_and_replay_limits(self):
        m=self.model()
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'report.json'; m.export(p)
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): audit.read_replay(fifo)
            big=Path(tmp)/'large'; big.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): audit.read_replay(big)
            p=Path(tmp)/'trace'; p.write_bytes(fixture())
            self.assertEqual(audit.read_replay(p),fixture())

if __name__=='__main__':
    unittest.main()
