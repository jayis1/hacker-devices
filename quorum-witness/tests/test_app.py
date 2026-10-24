# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import time
import unittest

spec = importlib.util.spec_from_file_location('audit', Path(__file__).resolve().parents[1]/'app/audit.py')
assert spec is not None and spec.loader is not None
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def frame(kind, values):
    body = ('QW1,'+kind+','+','.join(map(str, values))).encode()
    return body + f'*{audit.crc(body):04X}\n'.encode()

def status(now=100):
    return [now, 11, 1, 1, 4, 16, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 100, 0, 0, 100]

class Tests(unittest.TestCase):
    def setUp(self):
        self.model = audit.Model('synthetic')
        self.model.connected = True

    def test_policy_and_status(self):
        self.model.feed(frame('S', status()))
        self.assertFalse(self.model.fresh())
        self.model.feed(frame('P', [50,100,3]))
        self.assertTrue(self.model.fresh())
        self.assertEqual(self.model.snapshot()['status']['dissent_c'], 1)

    def test_fragmentation_and_partial_recovery(self):
        data = b'QW1,S,12' + frame('P', [50,100,3])
        for byte in data:
            self.model.feed(bytes([byte]))
        self.assertTrue(self.model.policy_known)
        self.assertEqual(self.model.errors, 1)

    def test_bounds_and_crc(self):
        for data in [b'bad\n', frame('S',[0]*22), frame('S',[-1]*23),
                     frame('S',[2**32]*23), frame('E',[0,0,9,0]),
                     frame('P',[50,100,3]).replace(b'50',b'51')]:
            self.model.feed(data)
        self.assertEqual(self.model.errors, 6)
        self.assertIsNone(self.model.status)
        self.model.feed(b'x'*100000)
        self.assertLessEqual(len(self.model.buffer),384)

    def test_semantic_bounds(self):
        for index, value in [(1,16),(2,2),(3,2),(4,8),(5,32)]:
            data=status()
            data[index]=value
            self.model.feed(frame('S',data))
        self.assertEqual(self.model.errors,5)

    def test_reset_and_policy(self):
        self.model.feed(frame('P',[50,100,3])+frame('S',status(1000)))
        self.model.feed(frame('S',status(2)))
        self.assertTrue(self.model.restart)
        self.model.feed(frame('P',[50,100,3]))
        self.assertFalse(self.model.fresh())
        self.model.feed(frame('B',[0,50,100,3])+frame('S',status(3)))
        self.assertTrue(self.model.fresh())
        self.assertEqual(self.model.sessions,1)

    def test_wrap(self):
        self.model.feed(frame('P',[50,100,3])+frame('S',status(0xfffffff0)))
        self.model.feed(frame('S',status(10)))
        self.assertFalse(self.model.restart)

    def test_incompatible(self):
        self.model.feed(frame('P',[1,2,3])+frame('S',status()))
        self.assertTrue(self.model.incompatible)
        self.assertIsNone(self.model.status)
        self.model.feed(frame('P',[50,100,3])+frame('S',status()))
        self.assertTrue(self.model.fresh())

    def test_freshness_and_disconnect(self):
        self.model.feed(frame('P',[50,100,3])+frame('S',status()))
        self.model.received=time.monotonic()-2
        self.assertFalse(self.model.fresh())
        self.model.feed(frame('S',status(101)))
        self.model.connected=False
        snap=self.model.snapshot()
        self.assertFalse(snap['fresh'])
        self.assertFalse(snap['replay'])
        self.assertIsNotNone(snap['status'])

    def test_replay_never_live(self):
        self.model.replay=True
        self.model.feed(frame('P',[50,100,3])+frame('S',status()))
        self.assertFalse(self.model.fresh())

    def test_events_bounded(self):
        for i in range(1000):
            self.model.feed(frame('E',[i,i,1,0]))
        self.assertEqual(len(self.model.events),256)
        self.assertEqual(self.model.events[0][0],744)

    def test_screens(self):
        self.model.feed(frame('P',[50,100,3])+frame('S',status()))
        self.assertIn('QUORUM OVERVIEW','\n'.join(audit.screen_lines(self.model,1)))
        self.assertIn('VOTE_C=0','\n'.join(audit.screen_lines(self.model,1)))
        self.assertIn('EVENT HISTORY','\n'.join(audit.screen_lines(self.model,2)))
        self.assertIn('HEALTH','\n'.join(audit.screen_lines(self.model,3)))

    def test_export(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'evidence.json'
            self.model.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.assertEqual(json.loads(path.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): self.model.export(path)
            link=Path(directory)/'link'
            os.symlink(path,link)
            with self.assertRaises(FileExistsError): self.model.export(link)

if __name__=='__main__': unittest.main()
