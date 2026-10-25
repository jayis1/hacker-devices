# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import binascii
import importlib.util
from pathlib import Path
import tempfile
import os
import subprocess
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('console', ROOT/'app/console.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

def frame(kind, values):
    data = ('DB1,'+kind+','+','.join(map(str,values))).encode()
    return data + f'*{binascii.crc_hqx(data,65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def test_fixture(self):
        m = app.Model('synthetic')
        m.feed(subprocess.check_output([str(ROOT/'firmware/build/fixture')]))
        self.assertEqual(m.errors,0)
        self.assertEqual(len(m.rows),121)
        self.assertEqual(m.rows[-1]['total_active'],3000)
        self.assertEqual(m.rows[-1]['faults'],9)
        self.assertTrue(m.policy)
    def test_resync(self):
        m=app.Model('synthetic')
        m.feed(b'DB1,S,0,broken'+frame('P',[10000,2000,20]))
        self.assertTrue(m.policy)
        self.assertGreater(m.errors,0)
    def test_bounds(self):
        m=app.Model('synthetic')
        m.feed(b'x'*10000)
        self.assertLessEqual(len(m.buffer),384)
        m.feed(frame('P',[10000,2000,20]))
        m.feed(frame('S',[0,1,0,2,0,0,0,0,0,0,0,0,0,0]))
        self.assertEqual(len(m.rows),0)
        self.assertGreater(m.errors,0)
    def test_reset_duplicate(self):
        m=app.Model('synthetic')
        m.feed(frame('P',[10000,2000,20]))
        values=[100,100,0,0,0,0,0,0,0,0,0,0,0,0]
        m.feed(frame('S',values))
        stamp=m.updated
        m.feed(frame('S',values))
        self.assertEqual(m.updated,stamp)
        values[2]=2
        m.feed(frame('S',values))
        self.assertTrue(m.frozen)
    def test_files(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.json'
            m=app.Model('synthetic')
            m.export(p,False)
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p,False)
            fifo=Path(d)/'fifo'
            os.mkfifo(fifo)
            with self.assertRaises(ValueError): app.read_replay(fifo)
            large=Path(d)/'large'
            large.write_bytes(b'x'*(1048577))
            with self.assertRaises(ValueError): app.read_replay(large)

if __name__=='__main__': unittest.main()
