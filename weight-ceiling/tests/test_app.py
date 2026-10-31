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
from console import Console, Model, frame, read_replay, Serial
from contract import FIELDS, validate, transition, queue

def baseline():
    return dict(zip(FIELDS,[20,0,15,0,0,1,0,0,5,17,0,0,0,0,0,0,0,0,0,0]))

def allocated():
    s=baseline(); s.update(now=100,observed=97,active=8,load=4,attempts=1,launched=1,peak=4)
    return s

def stream(*states):
    return frame('P',[3,1,5,5])+b''.join(frame('S',s.values()) for s in states)

class ContractTests(unittest.TestCase):
    def test_fixture(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m=Model('synthetic C fixture','replay'); m.feed(data); m.finish()
        self.assertEqual(m.errors,0); self.assertEqual(m.frozen,'')
        self.assertEqual(m.latest['faults'],1)
        self.assertEqual(m.latest['completed'],2)
        self.assertEqual(queue(m.latest),[(2,3)])
        self.assertFalse(m.live())
    def test_invalid(self):
        for key,value in [('active',16),('load',1),('peak',6),('faults',3),('launched',1),('rounds',1),('now',-1),('now',True)]:
            with self.subTest(key=key,value=value):
                s=baseline(); s[key]=value
                with self.assertRaises(ValueError): validate(s)
    def test_exact_ceiling(self):
        s=allocated(); s.update(active=9,load=5,attempts=2,launched=2,queued=1,peak=5)
        validate(s)
        m=Model('test','replay'); m.feed(stream(s))
        self.assertEqual(m.errors,0); self.assertEqual(queue(m.latest),[(0,1),(3,4)])
        s.update(active=12,load=7)
        with self.assertRaises(ValueError): validate(s)
    def test_identity_replacement_needs_cycle(self):
        a=allocated(); b=dict(a,now=200,observed=197,active=4,load=3)
        validate(a); validate(b); self.assertTrue(transition(a,b,100))
        m=Model('test','replay'); m.feed(stream(a,b)); self.assertTrue(m.frozen)
        b.update(attempts=3,launched=2,completed=1,rounds=1)
        validate(b); self.assertFalse(transition(a,b,100))
    def test_peak_growth_requires_new_allocation(self):
        a=allocated(); a.update(active=0,load=0,completed=1,attempts=2,rounds=1)
        b=dict(a,now=200,observed=197,peak=5,queued=1,rounds=0)
        # The raw contract transition must not accept growth without admission.
        self.assertTrue(transition(a,b,100))
    def test_unchanged_membership_hidden_cycle(self):
        a=allocated(); b=dict(a,now=200,observed=197,attempts=3,launched=2,completed=1,rounds=1)
        validate(b); self.assertFalse(transition(a,b,100))
    def test_masks(self):
        for mask in range(16):
            load=sum(i+1 for i in range(4) if mask&(1<<i))
            s=baseline(); n=mask.bit_count()
            s.update(active=mask,load=load,peak=load,attempts=n,launched=n,queued=max(0,n-1))
            if load<=5: validate(s)
            else:
                with self.assertRaises(ValueError): validate(s)
    def test_malformed_and_resync(self):
        m=Model('test','replay'); m.feed(b'x'*900+b'bad\nWC1,S,1')
        m.feed(stream(baseline())); m.finish()
        self.assertGreater(m.errors,0); self.assertIsNotNone(m.latest)
        before=m.errors; m.feed(b'WC1,S,1*0000\n'); self.assertGreater(m.errors,before)
    def test_policy_and_reset(self):
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,4])); self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(stream(baseline())+frame('B',[1])); self.assertTrue(m.frozen)
    def test_partial_and_empty(self):
        m=Model('test','replay'); m.feed(b'WC1,')
        with self.assertRaises(ValueError): m.finish()
        self.assertEqual(m.errors,1)
        m.feed(stream(baseline())+b'WC1,S,'); m.finish(); self.assertEqual(m.errors,2)
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
        with patch('console.time.monotonic',return_value=10): m.feed(stream(baseline()))
        b=dict(baseline(),now=120,observed=117)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(frame('S',b.values())); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(frame('S',b.values())); self.assertFalse(m.live())
            b.update(now=220,observed=217); m.feed(frame('S',b.values())); self.assertTrue(m.frozen)
    def test_backlog(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10): m.feed(stream(baseline()))
        for i in range(1,100):
            b=dict(baseline(),now=20+i,observed=17+i)
            with patch('console.time.monotonic',return_value=10+i*.1): m.feed(frame('S',b.values()))
        self.assertTrue(m.frozen)
    def test_open_error_and_restore(self):
        with self.assertRaises(OSError): Serial('/no/such/serial/device')
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            with patch('console.termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(old,termios.tcgetattr(slave))
        finally: os.close(master); os.close(slave)
    def test_halted_immutable(self):
        a=allocated(); a.update(halted=1,valid=0,faults=1,attempts=2,rejected=1)
        m=Model('test','replay'); m.feed(stream(a))
        m.feed(frame('S',dict(a,now=200,observed=98).values())); self.assertTrue(m.frozen)

class GuiTests(unittest.TestCase):
    def test_serial_screens_replay_export(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty()
        old=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect(); self.assertTrue(app.model.connected)
            os.write(master,stream(allocated()))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertEqual(app.model.latest['active'],8)
            self.assertIn('(3, 4)',app.summary.get('1.0','end'))
            self.assertIn('tag, weight',app.summary.get('1.0','end'))
            self.assertEqual(len(app.tabs.tabs()),3)
            app.connect(); self.assertIn('Disconnect',app.message)
            app.disconnect(); self.assertEqual(old,termios.tcgetattr(slave))
            app.connect(); os.close(master); master=None
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertFalse(app.model.connected)
            with tempfile.TemporaryDirectory() as d:
                fixture=Path(d)/'fixture.txt'
                fixture.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture']))
                app.load(fixture); root.update()
                self.assertEqual(app.model.latest['faults'],1)
                self.assertEqual(app.model.origin,'replay')
                self.assertIn('aggregate ceiling 5',app.summary.get('1.0','end'))
                self.assertIn('(2, 3)',app.summary.get('1.0','end'))
                app.path.set('/dev/null'); app.connect()
                self.assertEqual(app.model.origin,'replay'); self.assertTrue(app.message)
                output=Path(d)/'evidence.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(output)): app.export()
                saved=json.loads(output.read_text())
                self.assertEqual(saved['latest']['faults'],1); self.assertFalse(saved['live'])
                self.assertEqual(len(saved['history']),len(app.model.history))
                with patch('console.filedialog.asksaveasfilename',return_value=str(output)): app.export()
                self.assertNotIn('Saved private',app.message)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
