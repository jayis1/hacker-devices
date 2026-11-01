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
from contract import FIELDS, POLICY, validate, transition, eligible, distances

def baseline():
    s=dict.fromkeys(FIELDS,0)
    s.update(now=20,valid=15,phase=1,stable=5,observed=17)
    return s

def allocated():
    s=baseline()
    s.update(now=100,observed=97,active=1,depth=1,peak=1,launched=1,attempts=1,use0=1,stamp0=1)
    return s

def empty_history(tags):
    s=baseline()
    s.update(now=10000,observed=9997,launched=len(tags),completed=len(tags),
             attempts=2*len(tags),rounds=len(tags),peak=int(bool(tags)))
    for ordinal,tag in enumerate(tags,1):
        s['use'+str(tag)]+=1; s['stamp'+str(tag)]=ordinal
    return s

def stream(*states):
    return frame('P',POLICY)+b''.join(frame('S',[s[k] for k in FIELDS]) for s in states)

class ContractTests(unittest.TestCase):
    def test_c_fixture(self):
        m=Model('synthetic C fixture','replay')
        m.feed(subprocess.check_output([ROOT/'firmware/build/fixture'])); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual(m.latest['faults'],1)
        self.assertEqual(m.latest['completed'],6)
        self.assertEqual([m.latest['use'+str(i)] for i in range(4)],[2,2,2,0])
    def test_invalid_counts(self):
        for k,v in [('active',1),('use0',1),('depth',1),('peak',5),('rounds',1),('faults',3),('now',True),('stamp0',1)]:
            with self.subTest(k=k):
                s=baseline(); s[k]=v
                with self.assertRaises(ValueError): validate(s)
    def test_history_cannot_migrate(self):
        a=empty_history([0]); b=empty_history([1]); b['now']+=100
        validate(b)
        m=Model('test','replay'); m.feed(stream(a,b)); self.assertTrue(m.frozen)
    def test_distance_boundary(self):
        a=empty_history([0,1]); validate(a)
        self.assertEqual(eligible(a),[2,3])
        b=empty_history([0,1,2]); validate(b)
        self.assertEqual(eligible(b),[0,3])
        self.assertEqual(distances(b),[2,1,0,None])
        b=empty_history([0,1,0])
        with self.assertRaises(ValueError): validate(b)
    def test_stamp_change_needs_admission(self):
        a=empty_history([0,1,2,3]); b=dict(a,stamp0=2,stamp1=1)
        validate(b)
        self.assertTrue(transition(a,b,100))
    def test_sparse_reuse_needs_intervening_admissions(self):
        a=empty_history([1,2,3,0])
        b=empty_history([1,2,3,0,0]); b['now']+=100
        # Global counts and per-tag first-to-last spacing alone look plausible.
        # A sparse transition must still reject renewal of the recent identity.
        try: validate(b)
        except ValueError: return
        m=Model('impossible sparse reuse','replay'); m.feed(stream(a,b))
        self.assertTrue(m.frozen)
    def test_crc_valid_duplicate_stamps(self):
        s=empty_history([0,1,2]); s['stamp0']=2
        m=Model('forged endpoint','replay'); m.feed(stream(s))
        self.assertEqual(m.errors,1); self.assertIsNone(m.latest)
    def test_recent_suffix_must_have_all_ordinals(self):
        s=empty_history([0,1,2,3,0])
        s['stamp2']=1
        # Ordinal three is inside the last three admissions and cannot have
        # disappeared through a legal renewal yet.
        with self.assertRaises(ValueError): validate(s)
    def test_halted_immutable(self):
        a=empty_history([0]); a.update(halted=1,valid=0,faults=1,rejected=1,attempts=3)
        b=dict(a,now=10100,observed=10097)
        m=Model('test','replay'); m.feed(stream(a,b))
        self.assertIn('Halted evidence',m.frozen)
    def test_parser_recovery(self):
        m=Model('test','replay'); m.feed(b'x'*900+b'DN1,S,1'+stream(baseline())); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest,baseline())
        before=m.errors; m.feed(b'DN1,S,1*0000\n'); self.assertGreater(m.errors,before)
    def test_policy_reset_tail(self):
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,4,3,1])); self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(stream(baseline())+frame('B',[42])); self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(stream(baseline())+b'DN1,S'); m.finish(); self.assertEqual(m.errors,1)
    def test_live_stale_duplicate(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10): m.feed(stream(baseline()))
        b=dict(baseline(),now=120,observed=117)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(stream(b)); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(stream(b)); self.assertFalse(m.live())
            m.feed(stream(dict(b,now=220,observed=217))); self.assertTrue(m.frozen)
    def test_export_bounds_and_restore(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'export.json'; m=Model('test','replay'); m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): read_replay(p)
            p=Path(d)/'huge'; p.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(p)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            with patch('console.termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(old,termios.tcgetattr(slave))
        finally: os.close(master); os.close(slave)

class GuiTests(unittest.TestCase):
    def test_legal_renewal_screens(self):
        root=tk.Tk(); app=Console(root)
        try:
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'fixture'
                p.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture','legal']))
                app.load(p); root.update()
                text=app.summary.get('1.0','end')
                self.assertIn('Intervening admissions by tag (None = unused): [2, 1, 0, None]',text)
                self.assertIn('Eligible free tags: [0, 3]',text)
                self.assertEqual((app.model.errors,app.model.frozen),(0,''))
                self.assertEqual(app.model.latest['halted'],0)
                self.assertEqual(app.history.get('1.0','end').count("'now':"),len(app.model.history))
        finally: app.close(); root.destroy()
    def test_live_replay_screens_and_export(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect(); os.write(master,stream(allocated()))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertEqual(app.model.latest['use0'],1)
            self.assertIn('Eligible free tags: [1, 2, 3]',app.summary.get('1.0','end'))
            self.assertEqual(len(app.tabs.tabs()),3)
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            app.connect(); self.assertIn('Disconnect',app.message)
            app.disconnect(); self.assertEqual(old,termios.tcgetattr(slave))
            app.connect(); os.close(master); master=None
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertFalse(app.model.connected)
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'fixture'; p.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture']))
                app.load(p); root.update()
                self.assertEqual(app.model.latest['faults'],1)
                self.assertIn('admission distance violated',app.summary.get('1.0','end'))
                app.path.set('/dev/null'); app.connect(); self.assertTrue(app.message)
                self.assertEqual(app.model.origin,'replay')
                out=Path(d)/'report.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                saved=json.loads(out.read_text()); self.assertEqual(saved['latest']['stamp2'],6)
                self.assertEqual(saved['author'],'jayis1'); self.assertFalse(saved['live'])
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                self.assertNotIn('Saved private',app.message)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
