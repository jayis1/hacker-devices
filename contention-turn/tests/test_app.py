# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
import unittest
import os
import pty
import tempfile
import subprocess
import tkinter as tk
import time
from unittest.mock import patch
from console import Model, Console, frame, read_replay
from contract import FIELDS
ROOT=Path(__file__).resolve().parents[1]
class Tests(unittest.TestCase):
    def fixture(self):
        return subprocess.check_output([str(ROOT/'firmware/build/fixture')])
    def model(self):
        m=Model('synthetic C fixture','replay'); m.feed(self.fixture()); m.finish(); return m
    def test_fixture(self):
        m=self.model(); self.assertFalse(m.frozen); self.assertEqual(m.errors,0)
        self.assertEqual(len(m.history),40); self.assertEqual(m.latest['starts'],8)
        self.assertEqual(m.latest['accepted'],4); self.assertEqual(m.latest['unfair'],1)
        self.assertEqual(m.latest['collided'],2)
        self.assertEqual(m.latest['orphans'],1); self.assertEqual(m.latest['seeds'],3)
        self.assertFalse(m.live())
    def test_c_gap_reseed_halt(self):
        data=subprocess.check_output([str(ROOT/'firmware/build/fixture'),'recovery'])
        m=Model('synthetic C recovery fixture','replay'); m.feed(data); m.finish()
        self.assertFalse(m.frozen); self.assertEqual(m.errors,0)
        self.assertEqual(m.latest['gaps'],1); self.assertEqual(m.latest['baselines'],2)
        self.assertEqual(m.latest['seeds'],2); self.assertEqual(m.latest['halted'],1)
        self.assertEqual(m.latest['turn'],0)
    def test_corruption_resync_tail(self):
        m=Model('broken','replay'); m.feed(b'CT1,S,123'+self.fixture()+b'CT1,S,1')
        m.finish(); self.assertEqual(m.errors,2); self.assertIsNotNone(m.latest)
    def test_impossible_crc_valid(self):
        m=self.model(); s=dict(m.latest); s['starts']+=1
        with self.assertRaises(ValueError): m.accept(frame('S',[s[k] for k in FIELDS]).strip())
    def test_rollback_and_boot(self):
        m=self.model(); s=dict(m.latest); s['now']-=1
        m.accept(frame('S',[s[k] for k in FIELDS]).strip()); self.assertIn('rollback',m.frozen)
        m=self.model(); m.accept(frame('B',[1]).strip()); self.assertIn('Boot',m.frozen)
    def test_exports_and_files(self):
        m=self.model()
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'evidence.json'; m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',p.read_text())
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(large)
    def test_live_pause_and_duplicate(self):
        rows=self.fixture().splitlines(True)
        m=Model('fake transport','live'); m.connected=True
        with patch('console.time.monotonic',return_value=1): m.feed(rows[0]+rows[1])
        with patch('console.time.monotonic',return_value=1.02): m.feed(rows[2]); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=3):
            m.feed(rows[2]); self.assertFalse(m.live())
            m.feed(rows[3]); self.assertIn('pause',m.frozen)
    def test_real_tk_pty(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect(); self.assertIsNotNone(app.port)
            original=app.port; app.connect(); self.assertIs(app.port,original)
            for line in self.fixture().splitlines(True):
                os.write(master,line)
                until=time.monotonic()+.02
                while time.monotonic()<until: root.update(); time.sleep(.001)
            until=time.monotonic()+.1
            while time.monotonic()<until: root.update(); time.sleep(.001)
            self.assertEqual(len(app.model.history),40)
            self.assertIn('unfair',app.summary.get('1.0','end'))
            self.assertIn('collided',app.history.get('1.0','end'))
            self.assertEqual(app.tabs.index('end'),3)
            os.close(master); master=None
            until=time.monotonic()+.1
            while time.monotonic()<until: root.update(); time.sleep(.005)
            self.assertIsNone(app.port); self.assertFalse(app.model.connected)
            self.assertEqual(app.model.origin,'live')
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)
if __name__=='__main__': unittest.main()
