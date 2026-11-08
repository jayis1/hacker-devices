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
from contract import FIELDS, POLICY, validate, transition, eligible

def baseline():
    s=dict.fromkeys(FIELDS,0)
    s.update(now=20,valid=15,phase=1,stable=5,observed=17)
    return s

def snapshot(ops):
    s=baseline()
    for tag,alloc in ops:
        s['now']+=24; s['observed']+=24; s['attempts']+=1
        if alloc:
            s['queued']+=bool(s['active']); s['active']|=1<<tag
            s['launched']+=1; s['use'+str(tag)]+=1; s['stamp'+str(tag)]=s['launched']
            s['depth']+=1; s['peak']=max(s['peak'],s['depth'])
        else:
            s['active']&=~(1<<tag); s['depth']-=1; s['completed']+=1
            s['rounds']+=not s['active']
    return s

def stream(*states):
    return frame('P',POLICY)+b''.join(frame('S',[s[k] for k in FIELDS]) for s in states)

class ContractTests(unittest.TestCase):
    def test_c_fixture(self):
        m=Model('synthetic C fixture','replay')
        m.feed(subprocess.check_output([ROOT/'firmware/build/fixture'])); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual(m.latest['faults'],1)
        self.assertEqual((m.latest['active'],m.latest['completed']),(0,5))
    def test_allocation_eligibility(self):
        # Same occupancy has the same priority regardless of retained history.
        for last in range(4):
            order=list(range(4))
            order=order[last+1:]+order[:last+1]
            for mask in range(16):
                s=snapshot([(i,True) for i in order]+[(i,False) for i in range(4) if not mask&(1<<i)])
                validate(s)
                free=[i for i in range(4) if not mask&(1<<i)]
                self.assertEqual(eligible(s),free[:1])
        self.assertEqual(eligible(baseline()),[0])
    def test_immediate_reuse(self):
        real=Model('C-encoder repeated lowest renewal','replay')
        real.feed(subprocess.check_output([ROOT/'firmware/build/fixture','repeat'])); real.finish()
        self.assertEqual((real.errors,real.frozen),(0,''))
        self.assertEqual((real.latest['use0'],real.latest['faults']),(3,0))
        s=snapshot([(0,True),(0,False),(0,True),(0,False)])
        validate(s); self.assertEqual(s['stamp0'],2)
        self.assertEqual(transition(baseline(),s,s['now']-20),'')
        m=Model('repeated lowest renewal','replay'); m.feed(stream(baseline(),s))
        self.assertEqual((m.errors,m.frozen),(0,''))
    def test_lower_slot_can_fill_before_higher_hidden_reuse(self):
        a=baseline()
        b=snapshot([(0,True),(1,True),(1,False),(1,True)])
        validate(b)
        self.assertEqual(transition(a,b,b['now']-a['now']),'')
    def test_foreign_protocol_is_not_compatible(self):
        m=Model('other device','replay')
        m.feed(stream(baseline()).replace(b'LF1,',b'CF1,'))
        self.assertFalse(m.policy)
        self.assertIsNone(m.latest)
    def test_lower_free_barrier_blocks_hidden_renewal(self):
        ops=[(0,True),(1,True),(0,False),(1,False)]
        a=snapshot(ops); b=snapshot(ops+[(1,True),(1,False)])
        validate(a); validate(b)
        self.assertTrue(transition(a,b,48))
        m=Model('CRC-valid impossible return','replay'); m.feed(stream(a,b))
        self.assertIn('lower free',m.frozen)
    def test_rank_not_recency(self):
        ops=[(i,True) for i in range(4)]+[(1,False),(1,True),(3,False),(3,True),(1,False),(2,False)]
        a=snapshot(ops); b=snapshot(ops+[(1,True)])
        self.assertLess(a['stamp2'],a['stamp1'])
        self.assertEqual(eligible(a),[1])
        self.assertEqual(transition(a,b,24),'')
    def test_free_barrier_cannot_be_skipped_in_sparse_interval(self):
        a=baseline(); b=snapshot([(1,True),(1,False),(2,True)])
        validate(b)
        m=Model('forged sparse skip','replay'); m.feed(stream(a,b))
        self.assertIn('lower free',m.frozen)
    def test_busy_predecessor_can_release_before_admission(self):
        ops=[(i,True) for i in range(4)]+[(2,False)]
        a=snapshot(ops); b=snapshot(ops+[(0,False),(0,True)])
        validate(a); validate(b)
        self.assertEqual(eligible(a),[2])
        self.assertEqual(transition(a,b,48),'')
    def test_busy_older_may_be_excluded(self):
        ops=[(0,True),(1,True),(2,True),(3,True)]
        a=snapshot(ops); b=snapshot(ops+[(3,False),(3,True),(3,False)])
        validate(a); validate(b); self.assertEqual(transition(a,b,72),'')
    def test_invalid_counts(self):
        for k,v in [('active',1),('use0',1),('depth',1),('peak',5),('rounds',1),('faults',3),('now',True),('stamp0',1)]:
            with self.subTest(k=k):
                s=baseline(); s[k]=v
                with self.assertRaises(ValueError): validate(s)
    def test_impossible_lowest_fault(self):
        s=snapshot([(0,True)])
        s.update(halted=1,valid=0,faults=1,rejected=1,attempts=2)
        m=Model('forged lowest fault','replay'); m.feed(stream(s))
        self.assertEqual(m.errors,1); self.assertIsNone(m.latest)
    def test_stamp_migration(self):
        a=snapshot([(0,True),(1,True)])
        b=dict(a,now=a['now']+100,stamp0=2,stamp1=1)
        validate(b); self.assertTrue(transition(a,b,100))
    def test_halted_immutable(self):
        a=snapshot([(0,True),(0,False)])
        a.update(halted=1,valid=0,faults=1,rejected=1,attempts=3,levels=1)
        b=dict(a,now=a['now']+100,observed=a['observed']+100)
        m=Model('test','replay'); m.feed(stream(a,b)); self.assertIn('Halted evidence',m.frozen)
    def test_parser_recovery_and_tail(self):
        m=Model('test','replay'); m.feed(b'x'*900+b'LF1,S,1'+stream(baseline())); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest,baseline())
        n=m.errors; m.feed(b'LF1,S,1*0000\n'); self.assertGreater(m.errors,n)
        m.feed(b'LF1,S'); n=m.errors; m.finish(); self.assertEqual(m.errors,n+1)
    def test_policy_reset(self):
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,4,2,1])); self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(stream(baseline())+frame('B',[42])); self.assertTrue(m.frozen)
    def test_live_stale_duplicate(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10): m.feed(stream(baseline()))
        b=dict(baseline(),now=120,observed=117)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(stream(b)); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(stream(b)); self.assertFalse(m.live())
            m.feed(stream(dict(b,now=220,observed=217))); self.assertTrue(m.frozen)
    def test_cumulative_drift_and_counter_rollback(self):
        m=Model('synthetic slow delivery','live'); m.connected=True
        for i in range(200):
            with patch('console.time.monotonic',return_value=10+i*.11):
                m.feed(stream(dict(baseline(),now=20+i*100,observed=17+i*100)))
            if m.frozen:
                break
        self.assertIn('progress diverged',m.frozen)
        m=Model('synthetic counter rollback','replay')
        a=snapshot([(0,True),(0,False)])
        b=dict(baseline(),now=a['now']+100,observed=a['observed']+100)
        m.feed(stream(a,b))
        self.assertIn('rollback',m.frozen)
        self.assertEqual(m.latest,a)
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
    def test_permission_denial_preserves_evidence(self):
        root=tk.Tk(); app=Console(root)
        try:
            app.model=Model('retained fixture','replay')
            app.model.feed(stream(baseline()))
            retained=app.model
            with patch('console.os.open',side_effect=PermissionError('permission denied')):
                app.connect()
            root.update(); app.draw()
            self.assertIs(app.model,retained)
            self.assertIsNone(app.port)
            self.assertIn('permission denied',app.integrity.get('1.0','end'))
        finally: app.close(); root.destroy()
    def test_legal_screens(self):
        root=tk.Tk(); app=Console(root)
        try:
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'fixture'
                p.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture','legal']))
                app.load(p); root.update()
                self.assertIn('Eligible allocation tags: [0]',app.summary.get('1.0','end'))
                self.assertEqual((app.model.errors,app.model.frozen),(0,''))
                self.assertEqual(app.model.latest['halted'],0)
                self.assertEqual(app.history.get('1.0','end').count("'now':"),len(app.model.history))
        finally: app.close(); root.destroy()
    def test_live_replay_screens_and_export(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            os.write(master,stream(snapshot([(0,True)])))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertEqual(app.model.latest['use0'],1)
            self.assertIn('Eligible allocation tags: [1]',app.summary.get('1.0','end'))
            self.assertIn('Fixed numeric priority',app.summary.get('1.0','end'))
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
                self.assertIn('lowest free resource bypassed',app.summary.get('1.0','end'))
                app.path.set('/dev/null'); app.connect(); self.assertTrue(app.message)
                self.assertEqual(app.model.origin,'replay')
                out=Path(d)/'report.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                saved=json.loads(out.read_text()); self.assertEqual(saved['latest']['active'],0)
                self.assertEqual(saved['author'],'jayis1'); self.assertFalse(saved['live'])
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                self.assertNotIn('Saved private',app.message)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
