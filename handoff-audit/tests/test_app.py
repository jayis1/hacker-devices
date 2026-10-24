# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('audit',ROOT/'app/audit.py')
assert spec is not None and spec.loader is not None
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def frame(body):
    data=body.encode('ascii')
    return data+f'*{audit.crc(data):04X}\n'.encode('ascii')

BOOT=frame('HA1,B,0,20,50,3')
STATUS=frame('HA1,S,'+','.join(map(str,[123,5,1,1,20,2,0,0,0,0,0,0,0,0,0])))

class Tests(unittest.TestCase):
    def test_late_attachment_policy_preserves_evidence(self):
        m=audit.Model('late')
        m.connected=True
        m.feed(STATUS)
        self.assertFalse(m.fresh())
        m.feed(frame('HA1,P,20,50,3'))
        self.assertTrue(m.fresh())
        self.assertEqual(m.status[0],123)
        self.assertEqual(m.sessions,0)
        m.feed(frame('HA1,P,21,50,3'))
        self.assertFalse(m.fresh())
        m.feed(frame('HA1,P,20,50,3')+STATUS)
        self.assertTrue(m.fresh())

    def test_live_and_replay(self):
        m=audit.Model('test')
        m.connected=True
        m.feed(STATUS)
        self.assertFalse(m.fresh())
        m.feed(BOOT+STATUS)
        self.assertTrue(m.fresh())
        m.received-=2
        self.assertFalse(m.fresh())
        m.replay=True
        m.feed(STATUS)
        self.assertFalse(m.fresh())

    def test_bad_and_split(self):
        m=audit.Model('test')
        for b in BOOT+STATUS: m.feed(bytes([b]))
        self.assertEqual(m.status[0],123)
        m.feed(b'brokenHA1,S,12'+STATUS)
        self.assertEqual(m.errors,1)
        for body in ['HA2,B,0,20,50,3','HA1,S,-1','HA1,S,4294967296','HA1,E,0,1,99,0']:
            m.feed(frame(body))
        m.feed(b'x'*2000+b'\n')
        self.assertGreater(m.errors,4)
        self.assertLessEqual(len(m.buffer),384)

    def test_incompatible_and_restart(self):
        m=audit.Model('test')
        m.connected=True
        m.feed(BOOT+STATUS)
        m.feed(frame('HA1,B,0,21,50,3')+STATUS)
        self.assertTrue(m.incompatible)
        self.assertFalse(m.fresh())
        m.feed(BOOT+STATUS)
        old=STATUS.replace(b'123',b'122')
        # CRC must be recomputed for the rollback test.
        m.feed(frame(old.split(b'*')[0].decode()))
        self.assertTrue(m.restart)
        self.assertFalse(m.fresh())
        m.feed(BOOT)
        self.assertIsNone(m.status)
        self.assertFalse(m.restart)

    def test_export_screens(self):
        m=audit.Model('synthetic',True)
        m.feed(BOOT+STATUS)
        self.assertIn('HANDOFF OVERVIEW', '\n'.join(audit.screen_lines(m,1)))
        self.assertIn('EVENT HISTORY','\n'.join(audit.screen_lines(m,2)))
        self.assertIn('HEALTH','\n'.join(audit.screen_lines(m,3)))
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'evidence.json'
            m.export(p)
            self.assertEqual(os.stat(p).st_mode&0o777,0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(p)

if __name__=='__main__': unittest.main()
