# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from console import Console, Model, Serial, frame, read_replay
from contract import FIELDS, validate

class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture=subprocess.check_output([str(ROOT/'firmware/build/fixture')])

    def model(self):
        m=Model('synthetic C fixture','replay'); m.feed(self.fixture); m.finish()
        self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        return m

    def test_real_encoder(self):
        m=self.model(); s=m.latest
        self.assertEqual(len(m.history),13)
        self.assertEqual([s[k] for k in ('anchors','forward','repeat','skip','reverse','collisions')],[2,4,1,1,1,1])
        self.assertEqual(s['last'],3)
        self.assertFalse(m.live())

    def test_invalid_matrix_and_flags(self):
        s=self.model().latest
        for key,value in [('m0',2),('faults',0),('last',0),('ready',0),('idle',1),('collision_active',1),('anchors',0)]:
            bad=dict(s); bad[key]=value
            with self.assertRaises(ValueError,msg=key): validate(bad)
            m=Model('negative','replay'); m.feed(frame('P',[3,1,4,16]))
            m.feed(frame('S',[bad[k] for k in FIELDS])); self.assertEqual(m.errors,1)

    def test_stream_integrity(self):
        m=self.model(); s=dict(m.latest)
        m.feed(frame('S',[s[k] for k in FIELDS])); self.assertEqual(len(m.history),13)
        s['tx_fail']+=1
        m.feed(frame('S',[s[k] for k in FIELDS])); self.assertIn('duplicate',m.frozen)
        m=self.model(); s=dict(m.latest); s['now']-=1
        m.feed(frame('S',[s[k] for k in FIELDS])); self.assertIn('rollback',m.frozen)
        m=self.model(); s=dict(m.latest); s['now']+=1; s['anchors']+=2
        m.feed(frame('S',[s[k] for k in FIELDS])); self.assertIn('elapsed',m.frozen)

    def test_framing_and_eof(self):
        m=Model('partial','replay'); m.feed(b'BO1,S,123'+self.fixture)
        self.assertGreater(m.errors,0); self.assertIsNotNone(m.latest)
        m.feed(b'BO1,S,'); previous=m.errors; m.finish()
        self.assertEqual(m.errors,previous+1)
        with self.assertRaises(ValueError): Model('empty','replay').finish()
        m=Model('bad policy','replay'); m.feed(frame('P',[3,1,4,6])+self.fixture)
        self.assertTrue(m.frozen); self.assertIsNone(m.latest)

    def test_live_freshness_and_backlog(self):
        snapshots=list(self.model().history)
        with patch('console.time.monotonic') as clock:
            clock.return_value=1
            m=Model('mock transport','live'); m.connected=True
            m.feed(frame('P',[3,1,4,16]))
            m.feed(frame('S',[snapshots[0][k] for k in FIELDS])); self.assertFalse(m.live())
            clock.return_value=1.1
            m.feed(frame('S',[snapshots[1][k] for k in FIELDS])); self.assertTrue(m.live())
            clock.return_value=2.2
            m.feed(frame('S',[snapshots[2][k] for k in FIELDS])); self.assertFalse(m.live())
            self.assertIn('pause',m.frozen)
        with patch('console.time.monotonic') as clock:
            clock.return_value=0
            m=Model('slow backlog','live'); m.connected=True
            m.feed(frame('P',[3,1,4,16])); s=dict(snapshots[0])
            for i in range(20):
                clock.return_value=i*.1; s['now']=99+i
                m.feed(frame('S',[s[k] for k in FIELDS]))
            self.assertIn('diverged',m.frozen)

    def test_gradual_backlog_is_bounded(self):
        s=dict(self.model().history[0])
        with patch('console.time.monotonic') as clock:
            m=Model('four percent backlog','live'); m.connected=True
            m.feed(frame('P',[3,1,4,16]))
            for i in range(1001):
                clock.return_value=i*.1; s['now']=99+i*96
                m.feed(frame('S',[s[k] for k in FIELDS]))
            self.assertFalse(m.live())
            self.assertIn('diverged',m.frozen)

    def test_export_and_replay_bounds(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'report.json'; m=self.model(); m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',p.read_text())
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            big=Path(d)/'big'; big.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(big)

    def test_gui_pty_replay_export_and_hangup(self):
        master,slave=pty.openpty(); root=tk.Tk(); ui=Console(root)
        try:
            ui.path.set(os.ttyname(slave)); ui.connect()
            saved=ui.port; ui.connect(); self.assertIs(ui.port,saved)
            # Send paced packets through the real Tk polling loop. Fixture time
            # increments by 100 ms; tests are not a physical acquisition claim.
            lines=self.fixture.splitlines(keepends=True)
            for line in lines:
                os.write(master,line)
                until=time.monotonic()+(.1 if b',S,' in line else .025)
                while time.monotonic()<until:
                    root.update(); time.sleep(.005)
            self.assertEqual(ui.model.errors,0); self.assertFalse(ui.model.frozen)
            self.assertEqual(ui.model.latest['forward'],4)
            self.assertTrue(ui.model.live())
            self.assertEqual(len(ui.tabs.tabs()),3)
            self.assertIn('matrix',ui.summary.get('1.0','end').lower())
            os.close(master); master=None
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertIsNone(ui.port); self.assertFalse(ui.model.live())
            self.assertEqual(ui.model.origin,'live')
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'synthetic.txt'; p.write_bytes(self.fixture)
                ui.load(p); self.assertEqual(ui.model.origin,'replay')
                ui.model.export(Path(d)/'out.json')
                ui.path.set('/nonexistent/baton-order'); ui.connect()
                self.assertEqual(ui.model.origin,'replay')
                self.assertEqual(ui.model.latest['forward'],4)
        finally:
            ui.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
