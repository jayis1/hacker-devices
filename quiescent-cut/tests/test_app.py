# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import json
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from console import Console, Model, Serial, frame, read_replay
from contract import FIELDS, validate, transition

def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/fixture')])

def model():
    m=Model('synthetic production C encoder','replay')
    m.feed(fixture()); m.finish()
    return m

class ContractTests(unittest.TestCase):
    def test_fixture(self):
        m=model()
        self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        self.assertEqual(m.latest['accepted'],1)
        self.assertEqual(m.latest['early'],1)
        self.assertEqual(m.latest['abandoned'],1)
        self.assertTrue(m.latest['halted']); self.assertFalse(m.live())

    def test_invalid_snapshots(self):
        s=model().latest
        for key in ('rounds','captured','released','reclaims','early'):
            bad=dict(s); bad[key]+=1
            with self.subTest(key=key), self.assertRaises(ValueError): validate(bad)
        bad=dict(s); bad['owed']=1
        with self.assertRaises(ValueError): validate(bad)

    def test_frames_limits_and_resync(self):
        m=Model('bad','replay'); m.feed(b'garbageQC1,S,12'+fixture()); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest['accepted'],1)
        m.feed(b'x'*900); self.assertLess(len(m.buffer),769)
        m.feed(b'QC1,S,'); m.finish(); self.assertGreater(m.errors,1)
        bad=Model('bad','replay'); bad.feed(frame('P',[9]))
        self.assertTrue(bad.frozen)
        with self.assertRaises(ValueError): bad.finish()

    def test_retained_exports_and_file_limits(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'export.json'; m=model(); m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(large)

    def test_progress_and_boot_freeze(self):
        m=Model('synthetic','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10):
            m.feed(frame('P',[3,1,2,1]))
            s=dict(model().history[0]); m.feed(frame('S',[s[k] for k in FIELDS]))
            self.assertFalse(m.live())
        with patch('console.time.monotonic',return_value=10.01):
            s['now']+=10; s['observed']+=10
            m.feed(frame('S',[s[k] for k in FIELDS])); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(frame('S',[s[k] for k in FIELDS])); self.assertFalse(m.live())
            s['now']+=10; s['observed']+=10
            m.feed(frame('S',[s[k] for k in FIELDS])); self.assertTrue(m.frozen)
        m=model(); m.feed(frame('B',[99])); self.assertTrue(m.frozen)
        self.assertEqual(m.reset,42); self.assertEqual(m.later_reset,99)

    def test_cohort_transition(self):
        m=model(); a=next(s for s in m.history if s['owed']==3)
        b=dict(a); b['cohort']=1; b['owed']=1; b['released']+=1
        self.assertTrue(transition(a,b,10))
        b=dict(a); b['owed']=1; b['released']+=1
        self.assertFalse(transition(a,b,10))
        self.assertTrue(transition(b,a,10))

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Console(self.root)
    def tearDown(self):
        self.ui.close(); self.root.destroy()
    def pump(self):
        until=time.monotonic()+.05
        while time.monotonic()<until:
            self.root.update(); time.sleep(.005)
    def test_replay_screens_export_and_absent_device(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'synthetic.qc1'; p.write_bytes(fixture())
            self.ui.load(p); self.pump()
            self.assertIn('accepted: 1',self.ui.summary.get('1.0','end'))
            self.assertIn('replay',self.ui.integrity.get('1.0','end'))
            self.assertEqual(len(self.ui.tabs.tabs()),3)
            old=self.ui.model
            self.ui.path.set('/nonexistent/quiescent-cut'); self.ui.connect()
            self.assertIs(self.ui.model,old); self.assertTrue(self.ui.message)
            self.ui.model.export(Path(d)/'gui.json')
            self.ui.disconnect(); self.assertEqual(self.ui.model.origin,'replay')
    def test_live_pty_and_hangup(self):
        master,slave=pty.openpty(); path=os.ttyname(slave)
        original=termios.tcgetattr(slave)
        try:
            self.ui.path.set(path); self.ui.connect()
            port=self.ui.port; self.assertIsNotNone(port)
            self.ui.connect(); self.assertIs(self.ui.port,port)
            # Stream the actual C encoder at its nominal device cadence.
            last=0
            for line in fixture().splitlines(keepends=True):
                if line.startswith(b'QC1,S,'):
                    now=int(line.split(b',')[2]); time.sleep(max(0,(now-last)/1000)); last=now
                os.write(master,line); self.root.update(); self.ui.poll()
            self.assertFalse(self.ui.model.frozen); self.assertEqual(self.ui.model.errors,0)
            self.assertEqual(self.ui.model.latest['accepted'],1)
            self.assertTrue(self.ui.model.live())
            self.ui.disconnect(); self.assertEqual(termios.tcgetattr(slave),original)
            self.ui.connect(); os.close(master); master=None; self.pump()
            self.assertIsNone(self.ui.port); self.assertFalse(self.ui.model.connected)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_serial_setup_failure_restores_attributes(self):
        master,slave=pty.openpty()
        try:
            original=termios.tcgetattr(slave)
            with patch('console.termios.tcflush',side_effect=termios.error('injected')):
                with self.assertRaises(termios.error): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),original)
        finally: os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
