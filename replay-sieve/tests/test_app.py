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
    m=Model('synthetic C encoder','replay'); m.feed(fixture()); m.finish(); return m

class ContractTests(unittest.TestCase):
    def test_fixture(self):
        m=model(); self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        self.assertEqual(m.latest['fresh'],3); self.assertEqual(m.latest['duplicate'],1)
        self.assertEqual(m.latest['sessions'],2); self.assertTrue(m.latest['halted'])
    def test_invalid_snapshots(self):
        s=model().latest
        for key in ('fresh','admissions','sessions','ended','invalid'):
            bad=dict(s); bad[key]+=1
            with self.subTest(key=key), self.assertRaises(ValueError): validate(bad)
        m=model(); bad=dict(s); bad['seen']=16
        m.feed(frame('S',[bad[k] for k in FIELDS])); self.assertEqual(m.errors,1)
    def test_membership_transitions(self):
        a=next(s for s in model().history if s['seen']==10)
        b=dict(a); b['seen']=2
        self.assertTrue(transition(a,b,10))
        b=dict(a); b['seen']=11
        self.assertTrue(transition(a,b,10))
    def test_new_session_membership_and_capacity(self):
        a=dict(model().history[0])
        a.update(now=100, observed=97, sessions=1, ended=1, fresh=1, admissions=1, seen=1)
        for additions,seen in [(0,2),(7,15)]:
            b=dict(a)
            b.update(now=200, observed=197, sessions=2, phase=2, levels=8,
                     fresh=1+additions, admissions=1+additions, seen=seen)
            validate(a); validate(b)
            m=Model('impossible synthetic session','replay')
            m.feed(frame('P',[3,1,5,4]))
            m.feed(frame('S',[a[k] for k in FIELDS]))
            m.feed(frame('S',[b[k] for k in FIELDS]))
            with self.subTest(additions=additions): self.assertTrue(m.frozen)
    def test_frames_limits_resync(self):
        m=Model('bad','replay'); m.feed(b'junkRS1,S,12'+fixture()); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest['fresh'],3)
        m.feed(b'x'*900); self.assertLess(len(m.buffer),769)
        m.feed(b'RS1,S,'); m.finish(); self.assertGreater(m.errors,1)
        bad=Model('bad','replay'); bad.feed(frame('P',[9])); self.assertTrue(bad.frozen)
        with self.assertRaises(ValueError): bad.finish()
    def test_exports_file_limits(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'export.json'; m=model(); m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(large)
    def test_progress_and_boot(self):
        m=Model('synthetic','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10):
            m.feed(frame('P',[3,1,5,4])); s=dict(model().history[0])
            m.feed(frame('S',[s[k] for k in FIELDS])); self.assertFalse(m.live())
        with patch('console.time.monotonic',return_value=10.01):
            s['now']+=10; s['observed']+=10
            m.feed(frame('S',[s[k] for k in FIELDS])); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(frame('S',[s[k] for k in FIELDS])); self.assertFalse(m.live())
            s['now']+=10; s['observed']+=10
            m.feed(frame('S',[s[k] for k in FIELDS])); self.assertTrue(m.frozen)
        m=model(); m.feed(frame('B',[99])); self.assertTrue(m.frozen)
        self.assertEqual(m.reset,42); self.assertEqual(m.later_reset,99)
    def test_halted_and_rollback(self):
        m=model(); s=dict(m.latest); s['now']+=10; s['seen']=0
        m.feed(frame('S',[s[k] for k in FIELDS])); self.assertTrue(m.frozen)
        m=model(); s=dict(m.latest); s['now']-=1
        m.feed(frame('S',[s[k] for k in FIELDS])); self.assertTrue(m.frozen)
    def test_gradual_backlog(self):
        m=Model('synthetic','live'); m.connected=True; s=dict(model().history[0])
        for i in range(100):
            with patch('console.time.monotonic',return_value=10+i*.09):
                m.feed(frame('P',[3,1,5,4])); s['now']+=10; s['observed']+=10
                m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertTrue(m.frozen)

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Console(self.root)
    def tearDown(self):
        self.ui.close(); self.root.destroy()
    def pump(self):
        until=time.monotonic()+.025
        while time.monotonic()<until:
            self.root.update(); time.sleep(.005)
    def test_replay_screens_export(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'synthetic.rs1'; p.write_bytes(fixture())
            self.ui.load(p); self.pump()
            self.assertIn('fresh: 3',self.ui.summary.get('1.0','end'))
            self.assertIn('replay',self.ui.integrity.get('1.0','end'))
            self.assertEqual(len(self.ui.tabs.tabs()),3)
            old=self.ui.model; self.ui.path.set('/nonexistent/replay-sieve'); self.ui.connect()
            self.assertIs(self.ui.model,old); self.assertTrue(self.ui.message)
            with patch('console.filedialog.asksaveasfilename',return_value=str(Path(d)/'gui.json')):
                self.ui.export()
            self.assertEqual(json.loads((Path(d)/'gui.json').read_text())['latest']['fresh'],3)
            self.ui.disconnect(); self.assertEqual(self.ui.model.origin,'replay')
    def test_live_pty_hangup(self):
        master,slave=pty.openpty(); original=termios.tcgetattr(slave)
        try:
            self.ui.path.set(os.ttyname(slave)); self.ui.connect()
            port=self.ui.port; self.assertIsNotNone(port)
            self.ui.connect(); self.assertIs(self.ui.port,port)
            last=0
            for line in fixture().splitlines(keepends=True):
                if line.startswith(b'RS1,S,'):
                    now=int(line.split(b',')[2]); time.sleep(max(0,(now-last)/1000)); last=now
                os.write(master,line); self.pump()
            self.assertFalse(self.ui.model.frozen); self.assertEqual(self.ui.model.errors,0)
            self.assertEqual(self.ui.model.latest['fresh'],3); self.assertTrue(self.ui.model.live())
            self.ui.disconnect(); self.assertEqual(termios.tcgetattr(slave),original)
            self.ui.connect(); self.ui.draw()
            self.assertIn('No compatible',self.ui.summary.get('1.0','end'))
            os.close(master); master=None; self.pump(); self.assertIsNone(self.ui.port)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_setup_failure_restores(self):
        master,slave=pty.openpty()
        try:
            original=termios.tcgetattr(slave)
            with patch('console.termios.tcflush',side_effect=termios.error('injected')):
                with self.assertRaises(termios.error): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),original)
        finally: os.close(master); os.close(slave)
if __name__=='__main__': unittest.main()
