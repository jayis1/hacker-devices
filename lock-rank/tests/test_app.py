# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import os
from pathlib import Path
import pty
import stat
import subprocess
import sys
import tempfile
import termios
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from console import Model, Console, Serial, frame, read_replay
from contract import FIELDS, validate

def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/fixture')])

class Evidence(unittest.TestCase):
    def test_real_encoder(self):
        m=Model('synthetic C fixture','replay'); m.feed(fixture()); m.finish()
        self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        self.assertEqual(m.latest['a_descending'],1)
        self.assertEqual(m.latest['b_ambiguous'],2)
        self.assertEqual(m.latest['gaps'],1)
        self.assertEqual(m.latest['halted'],1)
        self.assertFalse(m.live())
    def test_bad_crc_bounds_and_tail(self):
        m=Model('synthetic','replay'); m.feed(fixture())
        m.feed(b'LR1,S,1*0000\n'); self.assertEqual(m.errors,1)
        m.feed(b'LR1,S,'); m.finish(); self.assertEqual(m.errors,2)
        m.feed(b'x'*800); self.assertGreater(m.errors,2)
    def test_resynchronize(self):
        m=Model('synthetic','replay'); m.feed(b'LR1,S,12'+fixture())
        m.finish(); self.assertEqual(m.errors,1); self.assertFalse(m.frozen)
    def test_impossible_crc_valid(self):
        m=Model('synthetic','replay'); m.feed(fixture())
        s=m.latest.copy(); s['a_starts']+=1
        with self.assertRaises(ValueError): validate(s)
        m.feed(frame('S',[s[k] for k in FIELDS])); self.assertEqual(m.errors,1)
    def test_reset_and_policy(self):
        m=Model('synthetic','replay'); m.feed(fixture())
        m.feed(frame('B',[9])); self.assertTrue(m.frozen); self.assertEqual(m.later_reset,9)
        m.feed(frame('P',[3,1,2,1])); self.assertTrue(m.frozen)
    def test_private_export_replay_limits(self):
        m=Model('synthetic','replay'); m.feed(fixture())
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.json'; m.export(p)
            self.assertEqual(stat.S_IMODE(p.stat().st_mode),0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(large)
    def test_duplicate_and_pause(self):
        records=fixture().splitlines(keepends=True)
        m=Model('pty','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10): m.feed(b''.join(records[:3]))
        with patch('console.time.monotonic',return_value=10.1): m.feed(records[3])
        with patch('console.time.monotonic',return_value=10.2):
            self.assertTrue(m.live()); m.feed(records[3]); self.assertEqual(m.received,10.1)
        with patch('console.time.monotonic',return_value=12):
            m.feed(records[4]); self.assertFalse(m.live()); self.assertTrue(m.frozen)
    def test_impossible_gap_coverage(self):
        m=Model('synthetic','replay')
        m.feed(b''.join(fixture().splitlines(keepends=True)[:3]))
        s=m.latest.copy(); s['now']+=100
        s['gaps']+=1; s['missed']+=1000000; s['faults']|=8
        s['a_observed']+=100; s['b_observed']+=100
        m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertTrue(m.frozen)
    def test_unexplained_qualification_loss(self):
        m=Model('synthetic','replay'); m.feed(frame('P',[3,1,2,1]))
        s=dict.fromkeys(FIELDS,0); s['valid']=1; s['now']=10
        m.feed(frame('S',[s[k] for k in FIELDS]))
        s['valid']=0; s['now']=110
        m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertTrue(m.frozen)
    def test_serial_restore(self):
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            s=Serial(os.ttyname(slave)); s.close()
            self.assertEqual(termios.tcgetattr(slave),old)
            with patch('console.termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),old)
        finally: os.close(master); os.close(slave)
    def test_actual_gui_pty(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            port=app.port; app.connect(); self.assertIs(app.port,port)
            # Replay is deliberate: this burst must not be mislabeled real-time.
            app.model.origin='synthetic-pty'
            os.write(master,fixture()); app.poll(); root.update()
            self.assertEqual(app.model.errors,0); self.assertFalse(app.model.frozen)
            self.assertIn('a_descending: 1',app.summary.get('1.0','end'))
            self.assertIn('b_ambiguous: 2',app.summary.get('1.0','end'))
            self.assertIn('synthetic-pty',app.integrity.get('1.0','end'))
            os.close(master); master=None; app.poll(); root.update()
            self.assertIsNone(app.port); self.assertFalse(app.model.connected)
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'trace'; p.write_bytes(fixture()); app.load(p)
                self.assertEqual(app.model.origin,'replay')
                self.assertIn('descending',app.history.get('1.0','end'))
        finally:
            app.close(); root.destroy()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
