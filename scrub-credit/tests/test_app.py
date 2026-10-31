# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import json
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
from console import Console, Model, frame, read_replay, Serial
from contract import FIELDS, validate

def baseline():
    return dict(zip(FIELDS,[20,0,15,0,0,1,0,0,5,17,0,0,0,0,0,0,0,0,0,0]))

def allocated():
    s=baseline(); s.update(now=100,observed=97,active=1,attempts=2,launched=1,scrubbed=1,peak=1)
    return s

class AppTests(unittest.TestCase):
    def test_real_encoder_fixture(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m=Model('synthetic C fixture','replay'); m.feed(data); m.finish()
        self.assertEqual(m.errors,0); self.assertEqual(m.frozen,'')
        self.assertEqual(m.latest['faults'],1)
        self.assertEqual(m.latest['scrubbed'],3)
        self.assertEqual(m.latest['completed'],3)
        self.assertEqual(m.latest['active'],0)
        self.assertFalse(m.live())
    def test_impossible_credit_migration_valid_crc(self):
        a=baseline(); a.update(clean=1,scrubbed=1,attempts=1)
        b=dict(a,now=120,observed=117,clean=2)
        validate(a); validate(b)
        m=Model('synthetic','replay')
        m.feed(frame('P',[3,1,5,2])+frame('S',a.values())+frame('S',b.values()))
        self.assertTrue(m.frozen); self.assertEqual(m.latest,a)
    def test_invalid_scrub_snapshot_valid_crc(self):
        a=allocated(); a['clean']=1
        m=Model('synthetic','replay'); m.feed(frame('P',[3,1,5,2])+frame('S',a.values()))
        self.assertEqual(m.errors,1); self.assertIsNone(m.latest)
    def test_malformed_and_resync(self):
        m=Model('test','replay'); m.feed(b'x'*900+b'bad\nSC1,S,1')
        m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values()))
        m.finish(); self.assertGreater(m.errors,0); self.assertIsNotNone(m.latest)
        before=m.errors; m.feed(b'SC1,S,1*0000\n'); self.assertGreater(m.errors,before)
    def test_policy_and_reset(self):
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,4])); self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values())+frame('B',[1]))
        self.assertTrue(m.frozen)
    def test_partial_and_empty(self):
        m=Model('test','replay'); m.feed(b'SC1,')
        with self.assertRaises(ValueError): m.finish()
        self.assertEqual(m.errors,1)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values())+b'SC')
        m.finish(); self.assertEqual(m.errors,1)
    def test_export(self):
        m=Model('test','replay')
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'report.json'; m.export(p)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            q=Path(d)/'link'; q.symlink_to(p)
            with self.assertRaises(FileExistsError): m.export(q)
    def test_replay_fifo_and_oversize(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): read_replay(p)
            q=Path(d)/'large'; q.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(q)
    def test_freshness_and_duplicate(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10):
            m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values()))
        b=dict(baseline(),now=120,observed=117)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(frame('S',b.values())); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(frame('S',b.values())); self.assertFalse(m.live())
            c=dict(b,now=220,observed=217); m.feed(frame('S',c.values()))
            self.assertTrue(m.frozen)
    def test_slow_backlog(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=0):
            m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values()))
        for i in range(1,100):
            with patch('console.time.monotonic',return_value=i*.1):
                m.feed(frame('S',dict(baseline(),now=20+i,observed=17+i).values()))
        self.assertTrue(m.frozen)
    def test_open_error_and_restore(self):
        with self.assertRaises(OSError): Serial('/no/such/serial/device')
        import termios
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            with patch('console.termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),old)
        finally: os.close(master); os.close(slave)
    def test_halted_immutable(self):
        a=allocated(); a.update(halted=1,valid=0,faults=1,attempts=3,rejected=1)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,2])+frame('S',a.values()))
        b=dict(a,now=200,observed=98)
        m.feed(frame('S',b.values())); self.assertTrue(m.frozen)

class GuiTests(unittest.TestCase):
    def test_serial_screens_replay_export(self):
        root=tk.Tk(); app=Console(root)
        master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            self.assertTrue(app.model.connected)
            os.write(master,frame('P',[3,1,5,2])+frame('S',allocated().values()))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertEqual(app.model.latest['active'],1)
            self.assertIn('Active slots: [0]',app.summary.get('1.0','end'))
            self.assertIn('fresh per-slot scrub',app.summary.get('1.0','end'))
            self.assertEqual(len(app.tabs.tabs()),3)
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            app.connect(); self.assertIn('Disconnect',app.message)
            os.close(master); master=None
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertFalse(app.model.connected)
            self.assertIsNotNone(app.model.latest)
            with tempfile.TemporaryDirectory() as d:
                fixture=Path(d)/'fixture.txt'
                fixture.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture']))
                app.load(fixture); root.update()
                self.assertEqual(app.model.latest['faults'],1)
                self.assertEqual(app.model.origin,'replay')
                self.assertIn('Clean vacant slots: []',app.summary.get('1.0','end'))
                self.assertEqual(len(app.model.history),31)
                output=Path(d)/'evidence.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(output)):
                    app.export()
                saved=json.loads(output.read_text())
                self.assertEqual(saved['latest']['faults'],1); self.assertFalse(saved['live'])
                old=app.model; app.path.set('/dev/null'); app.connect()
                self.assertIs(app.model,old)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
