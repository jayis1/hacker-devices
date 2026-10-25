# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
import os
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('console',root/'app/console.py')
assert spec is not None and spec.loader is not None
app=importlib.util.module_from_spec(spec); spec.loader.exec_module(app)

def frame(kind, *values):
    s=('RD1,'+kind+','+','.join(map(str,values))).encode()
    return s+b'*'+f'{app.crc(s):04X}\n'.encode()

class Tests(unittest.TestCase):
    def test_encoder(self):
        m=app.Model('synthetic fixture')
        m.feed(subprocess.check_output([root/'firmware/build/fixture']))
        m.finish()
        self.assertFalse(m.error)
        self.assertEqual(m.latest['completed'],1)
        self.assertEqual(m.latest['faults'],0)
    def test_crc_and_truncated(self):
        m=app.Model('test'); m.feed(b'bad\n'); self.assertEqual(m.errors,1)
        m.feed(frame('P',20,500)+b'RD1,S,123'); m.finish()
        self.assertTrue(m.error)
    def test_resync(self):
        m=app.Model('test'); m.feed(b'RD1,S,12'+frame('P',20,500))
        self.assertTrue(m.policy); self.assertEqual(m.errors,1)
    def test_impossible(self):
        m=app.Model('test'); m.feed(frame('P',20,500))
        m.feed(frame('S',1,0,0,0,1,0,0,0,0,0,0,0,0,0))
        self.assertTrue(m.error)
    def test_duplicate_and_rollback(self):
        m=app.Model('test'); m.feed(frame('P',20,500))
        good=frame('S',1,1,0,0,0,0,0,0,0,0,0,0,0,0)
        m.feed(good); arrival=m.arrival; m.feed(good)
        self.assertEqual(arrival,m.arrival)
        m.feed(frame('S',1,1,0,0,0,0,0,0,1,0,0,0,0,0))
        self.assertTrue(m.error)
    def test_bounded_regular_replay_and_private_export(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): app.read_replay(p)
            p=Path(td)/'large'; p.write_bytes(b'x'*(app.LIMIT+1))
            with self.assertRaises(ValueError): app.read_replay(p)
            p=Path(td)/'report.json'; m=app.Model('test')
            app.export(m,p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): app.export(m,p)
if __name__=='__main__': unittest.main()
