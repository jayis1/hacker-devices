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
from contract import FIELDS, validate, transition, queue

def baseline():
    return dict(zip(FIELDS,[20,0,15,0,0,1,0,0,5,17,0,0,0,0,0,0,0,0,0,0]))

def proposed():
    s=baseline(); s.update(now=100,observed=97,active=1,order=4,attempts=1,launched=1,peak=1)
    return s

class ContractTests(unittest.TestCase):
    def test_fixture(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m=Model('synthetic C fixture','replay'); m.feed(data); m.finish()
        self.assertEqual(m.errors,0); self.assertEqual(m.frozen,'')
        self.assertEqual(m.latest['faults'],1)
        self.assertEqual(m.latest['completed'],2)
        self.assertEqual(queue(m.latest),[(0,1)])
        self.assertFalse(m.live())
    def test_invalid(self):
        for key,value in [('active',4),('order',16),('order',1),('peak',3),('faults',3),('launched',1),('rounds',1),('now',-1),('now',True)]:
            with self.subTest(key=key,value=value):
                s=baseline(); s[key]=value
                with self.assertRaises(ValueError): validate(s)
    def test_same_proposer_parallel(self):
        s=proposed(); s.update(active=3,order=12,attempts=2,launched=2,queued=1,peak=2)
        validate(s)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,2])+frame('S',s.values()))
        self.assertEqual(m.errors,0); self.assertEqual(queue(m.latest),[(0,0),(1,0)])
        s['order']=15; validate(s)
    def test_proposer_replacement_needs_cycle(self):
        a=proposed(); b=dict(a,now=200,observed=197,order=5)
        validate(a); validate(b)
        self.assertTrue(transition(a,b,100))
        m=Model('test','replay')
        m.feed(frame('P',[3,1,5,2])+frame('S',a.values())+frame('S',b.values()))
        self.assertTrue(m.frozen)
        b.update(attempts=3,launched=2,completed=1,rounds=1)
        validate(b); self.assertFalse(transition(a,b,100))
    def test_reuse(self):
        a=proposed(); b=dict(a,now=200,observed=197,attempts=3,launched=2,completed=1,rounds=1)
        validate(a); validate(b); self.assertFalse(transition(a,b,100))
    def test_history_does_not_disappear(self):
        a=proposed(); a.update(active=0,completed=1,attempts=2,rounds=1)
        self.assertTrue(transition(a,dict(a,order=0),1))
    def test_malformed_and_resync(self):
        m=Model('test','replay'); m.feed(b'x'*900+b'bad\nSP1,S,1')
        m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values()))
        m.finish(); self.assertGreater(m.errors,0); self.assertIsNotNone(m.latest)
        before=m.errors; m.feed(b'SP1,S,1*0000\n'); self.assertGreater(m.errors,before)
    def test_policy_and_reset(self):
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,4]))
        self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values())+frame('B',[1]))
        self.assertTrue(m.frozen)
    def test_partial_and_empty(self):
        m=Model('test','replay'); m.feed(b'SP1,')
        with self.assertRaises(ValueError): m.finish()
        self.assertEqual(m.errors,1)
        m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values())+b'SP1,S,')
        m.finish(); self.assertEqual(m.errors,2)
    def test_export(self):
        m=Model('test','replay')
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'report.json'; m.export(p)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            q=Path(d)/'link'; q.symlink_to(p)
            with self.assertRaises(FileExistsError): m.export(q)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): read_replay(p)
            p=Path(d)/'large'; p.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(p)
    def test_freshness_and_duplicate(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10):
            m.feed(frame('P',[3,1,5,2])+frame('S',baseline().values()))
        b=dict(baseline(),now=120,observed=117)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(frame('S',b.values())); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(frame('S',b.values())); self.assertFalse(m.live())
            b.update(now=220,observed=217); m.feed(frame('S',b.values()))
            self.assertTrue(m.frozen)
    def test_open_error(self):
        with self.assertRaises(OSError): Serial('/no/such/serial/device')
    def test_halted_immutable(self):
        a=proposed(); a.update(halted=1,valid=0,faults=1,attempts=2,rejected=1)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,2])+frame('S',a.values()))
        m.feed(frame('S',dict(a,now=200,observed=98).values()))
        self.assertTrue(m.frozen)

class GuiTests(unittest.TestCase):
    def test_serial_screens_replay_export(self):
        root=tk.Tk(); app=Console(root)
        master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            self.assertTrue(app.model.connected)
            os.write(master,frame('P',[3,1,5,2])+frame('S',proposed().values()))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertEqual(app.model.latest['active'],1)
            self.assertIn('(0, 0)',app.summary.get('1.0','end'))
            self.assertIn('slot, proposer',app.summary.get('1.0','end'))
            self.assertEqual(len(app.tabs.tabs()),3)
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
                self.assertIn('other actor',app.summary.get('1.0','end'))
                output=Path(d)/'evidence.json'; app.model.export(output)
                saved=json.loads(output.read_text())
                self.assertEqual(saved['latest']['faults'],1)
                self.assertFalse(saved['live'])
                self.assertEqual(len(saved['history']),len(app.model.history))
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
