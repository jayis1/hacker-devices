# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import json
import os
from pathlib import Path
import pty
import stat
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
import console as app
from contract import FIELDS, validate

class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture=subprocess.check_output([ROOT/'firmware/build/fixture'])
    def model(self):
        m=app.Model('synthetic C fixture','replay'); m.feed(self.fixture); m.finish()
        self.assertFalse(m.frozen); self.assertEqual(m.errors,0)
        return m
    def test_fixture(self):
        m=self.model()
        self.assertEqual(m.latest['attempts'],2)
        self.assertEqual(m.latest['failures'],1)
        self.assertEqual(m.latest['successes'],1)
        self.assertEqual(m.latest['early'],1)
        self.assertFalse(m.live())
    def test_invalid(self):
        s=self.model().latest.copy()
        for key,value in [('attempts',1),('grade',8),('remaining',1),('age',1001),('faults',0),('early',3),('state',2)]:
            with self.subTest(key=key):
                bad=dict(s); bad[key]=value
                with self.assertRaises(ValueError): validate(bad)
                m=app.Model('bad','replay'); m.feed(app.frame('P',[3,10,7,1000]))
                m.feed(app.frame('S',[bad[k] for k in FIELDS])); self.assertEqual(m.errors,1)
    def test_framing(self):
        m=app.Model('fragment','replay'); m.feed(b'BR1,S,123'+self.fixture)
        self.assertTrue(m.latest); self.assertGreater(m.errors,0)
        m.feed(b'BR1,S'); m.finish(); self.assertGreater(m.errors,1)
        with self.assertRaises(ValueError): app.Model('empty','replay').finish()
    def test_boot_and_duplicate(self):
        m=self.model(); last=m.latest.copy()
        m.feed(app.frame('S',[last[k] for k in FIELDS])); self.assertFalse(m.frozen)
        last['tx_fail']+=1
        m.feed(app.frame('S',[last[k] for k in FIELDS])); self.assertIn('duplicate',m.frozen)
        m=self.model(); m.feed(app.frame('B',[456])); self.assertTrue(m.frozen)
        self.assertEqual(m.reset,123); self.assertEqual(m.later_reset,456)
    def test_live_pause_and_drift(self):
        base=self.model().latest
        for step in (2.0,0.11):
            m=app.Model('synthetic transport','live'); m.connected=True
            with patch.object(app.time,'monotonic',return_value=0):
                m.feed(app.frame('P',[3,10,7,1000])); m.feed(app.frame('S',[base[k] for k in FIELDS]))
            for i in range(1,150):
                s=dict(base); s['now']+=i*100
                with patch.object(app.time,'monotonic',return_value=i*step):
                    m.feed(app.frame('S',[s[k] for k in FIELDS]))
                if m.frozen: break
            self.assertTrue(m.frozen)
    def test_files(self):
        m=self.model()
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.json'; m.export(p)
            self.assertEqual(stat.S_IMODE(p.stat().st_mode),0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'pipe'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): app.read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(app.FILE_LIMIT+1))
            with self.assertRaises(ValueError): app.read_replay(large)
    def test_sparse_transition_and_late_attach(self):
        m=self.model()
        # Drop B entirely: repeated policy must suffice for late attachment.
        late=app.Model('late synthetic capture','replay')
        late.feed(b'partial suffix'+b'\n'.join(self.fixture.split(b'\n')[1:]))
        self.assertEqual(late.latest,m.latest)
        self.assertIsNone(late.reset)
        # A valid individual snapshot cannot mint a new escalation grade
        # without another failure, even when the CRC was recomputed.
        bad=dict(m.latest); bad['now']+=100; bad['grade']=1
        validate(bad)
        m.feed(app.frame('S',[bad[k] for k in FIELDS]))
        self.assertIn('outcome',m.frozen)
    def test_duplicate_does_not_refresh(self):
        m=self.model(); m.origin='live'; m.connected=True
        previous=m.received
        with patch.object(app.time,'monotonic',return_value=previous+2):
            m.feed(app.frame('S',[m.latest[k] for k in FIELDS]))
            self.assertFalse(m.live())
            self.assertEqual(m.received,previous)
    def test_real_ui_pty(self):
        root=tk.Tk(); ui=app.Console(root)
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            ui.path.set(os.ttyname(slave)); ui.connect(); ui.connect()
            self.assertIn('Disconnect',ui.message)
            os.write(master,self.fixture)
            for _ in range(8): root.update(); time.sleep(.025)
            self.assertEqual(ui.model.latest['early'],1)
            self.assertIn('READY',ui.summary.get('1.0','end'))
            self.assertEqual(len(ui.tabs.tabs()),3)
            self.assertIn('attempts',ui.history.get('1.0','end'))
            ui.disconnect(); self.assertEqual(termios.tcgetattr(slave),old)
            self.assertFalse(ui.model.live())
            with tempfile.TemporaryDirectory() as d:
                path=Path(d)/'synthetic.txt'; path.write_bytes(self.fixture)
                ui.load(path); self.assertEqual(ui.model.origin,'replay')
                ui.model.export(Path(d)/'evidence.json')
            ui.connect(); os.close(master); master=None
            for _ in range(5): root.update(); time.sleep(.025)
            self.assertIsNone(ui.port)
        finally:
            ui.close(); root.destroy()
            if master is not None: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
