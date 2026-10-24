# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('console', ROOT/'app/ledger.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

def frame(body):
    data=body.encode()
    return data+b'*'+f'{app.crc(data):04X}'.encode()+b'\n'

class AppTests(unittest.TestCase):
    def test_crc(self):
        self.assertEqual(app.crc(b'123456789'),0x29b1)

    def test_resync(self):
        m=app.Model('synthetic')
        good=frame('GL1,B,0,1000,3')
        m.feed(b'GL1,E,3,broken'+good)
        self.assertEqual(m.boot,[0,1000,3])
        self.assertGreater(m.errors,0)

    def test_validation(self):
        m=app.Model('synthetic')
        for s in ['GL1,B,0,999,3','GL1,E,0,0,99,0','GL1,S,0',
                  'GL1,B,4294967296,1000,3','GL1,B,-1,1000,3']:
            m.feed(frame(s))
        m.feed(b'GL1,B,0,1000,3*0000\n')
        self.assertEqual(m.errors,6)
        self.assertIsNone(m.boot)

    def test_bounded(self):
        m=app.Model('synthetic')
        m.feed(b'x'*10000)
        self.assertLessEqual(len(m.buffer),384)
        for i in range(300): m.feed(frame(f'GL1,E,{i},{i},1,0'))
        self.assertEqual(len(m.events),256)

    def test_status_and_export(self):
        m=app.Model('synthetic')
        m.connected=True
        status=[10,0,0,1]+[0]*15
        m.feed(frame('GL1,S,'+','.join(map(str,status))))
        self.assertTrue(m.fresh())
        m.received-=2
        self.assertFalse(m.fresh())
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'e.json'
            m.export(path)
            result=json.loads(path.read_text())
            self.assertEqual(result['author'],'jayis1')
            self.assertEqual(os.stat(path).st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(path)

    def test_restart_and_replay(self):
        m=app.Model('replay',replay=True)
        m.connected=True
        for t in [100,50]:
            m.feed(frame('GL1,S,'+','.join(map(str,[t,0,0,1]+[0]*15))))
        self.assertTrue(m.restart)
        self.assertFalse(m.fresh())
        self.assertEqual(m.status[0],100)
        m.feed(frame('GL1,B,0,1000,3'))
        self.assertFalse(m.restart)
        self.assertIsNone(m.status)

    def test_named_inputs(self):
        m=app.Model('synthetic')
        m.status=[10,1,0,1]+[0]*15
        lines='\n'.join(app.screen_lines(m,1))
        self.assertIn('REQUEST=1 GRANT=0 USE=0 REVOKE=0',lines)

    def test_incompatible_policy(self):
        m=app.Model('synthetic')
        m.connected=True
        m.feed(frame('GL1,B,0,50,3'))
        m.feed(frame('GL1,S,'+','.join(map(str,[10,0,0,1]+[0]*15))))
        self.assertFalse(m.fresh())
        self.assertIsNone(m.status)
        self.assertTrue(m.incompatible)
        m.feed(frame('GL1,B,0,1000,3'))
        self.assertFalse(m.incompatible)

    def test_status_wrap_and_delayed_event(self):
        m=app.Model('synthetic')
        for t in [0xfffffffe,2]:
            m.feed(frame('GL1,S,'+','.join(map(str,[t,0,0,1]+[0]*15))))
        m.feed(frame('GL1,E,0,4294967293,1,0'))
        self.assertFalse(m.restart)
        self.assertEqual(m.status[0],2)

if __name__=='__main__': unittest.main()
