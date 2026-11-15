# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import json
from itertools import permutations
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
    # Accounting builder deliberately does not enforce policy: supports forgeries.
    s=baseline()
    for tag,alloc in ops:
        s['now']+=24; s['observed']+=24; s['attempts']+=1
        if alloc:
            s['queued']+=bool(s['active']); s['active']|=1<<tag
            s['launched']+=1; s['use'+str(tag)]+=1; s['stamp'+str(tag)]=s['launched']
            s['depth']+=1; s['peak']=max(s['peak'],s['depth'])
        else:
            s['active']&=~(1<<tag); s['depth']-=1; s['completed']+=1
            s['ret'+str(tag)]=s['completed']
            s['rounds']+=not s['active']
    return s

def stream(*states):
    return frame('P',POLICY)+b''.join(frame('S',[s[k] for k in FIELDS]) for s in states)

class ContractTests(unittest.TestCase):
    def test_c_fixture(self):
        m=Model('synthetic C fixture','replay')
        m.feed(subprocess.check_output([ROOT/'firmware/build/fixture'])); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual((m.latest['faults'],m.latest['active'],m.latest['completed']),(1,0,5))
    def test_eligibility(self):
        for order in permutations(range(4)):
            for mask in range(16):
                returned=[i for i in order if not mask&(1<<i)]
                s=snapshot([(i,True) for i in range(4)]+[(i,False) for i in returned])
                validate(s)
                self.assertEqual(eligible(s),returned[:1])
        self.assertEqual(eligible(baseline()),[0])
    def test_only_free_renewal(self):
        m=Model('synthetic repeated only-free renewal','replay')
        m.feed(subprocess.check_output([ROOT/'firmware/build/fixture','repeat'])); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual((m.latest['use3'],m.latest['faults']),(4,0))
    def test_return_order_not_admission_order(self):
        ops=[(i,True) for i in range(4)]+[(3,False),(1,False)]
        a=snapshot(ops); b=snapshot(ops+[(3,True)])
        self.assertEqual(eligible(a),[3])
        self.assertEqual(transition(a,b,24),'')
    def test_sparse_barriers(self):
        cases=[([],[(1,True)]),
               ([],[(0,True),(0,False),(0,True)]),
               ([(i,True) for i in range(4)]+[(2,False)],[(0,False),(0,True)]),
               ([(i,True) for i in range(4)]+[(1,False),(3,False)],[(3,True)])]
        for ops,extra in cases:
            a=snapshot(ops); b=snapshot(ops+extra)
            validate(a); validate(b)
            m=Model('CRC-valid impossible sparse reuse','replay'); m.feed(stream(a,b))
            self.assertIn('queue predecessor',m.frozen)
    def test_barrier_consumed_before_renewal(self):
        ops=[(i,True) for i in range(4)]+[(2,False)]
        a=snapshot(ops); b=snapshot(ops+[(0,False),(2,True),(0,True)])
        self.assertEqual(transition(a,b,72),'')
    def test_all_legal_sparse_pairs(self):
        # Independent FIFO enumerates short words and every prefix snapshot pair.
        def walk(ops,free,remaining):
            b=snapshot(ops); validate(b)
            for k in range(len(ops)):
                a=snapshot(ops[:k])
                self.assertEqual(transition(a,b,b['now']-a['now']),'')
            if not remaining: return
            if free: walk(ops+[(free[0],True)],free[1:],remaining-1)
            for tag in range(4):
                if tag not in free: walk(ops+[(tag,False)],free+[tag],remaining-1)
        walk([],list(range(4)),7)
    def test_migration(self):
        a=snapshot([(0,True),(1,True),(0,False),(1,False)])
        for changes in [dict(ret0=2,ret1=1),dict(stamp0=2,stamp1=1)]:
            b=dict(a,now=a['now']+100,**changes); validate(b)
            self.assertTrue(transition(a,b,100))
    def test_invalid_counts(self):
        for k,v in [('active',1),('use0',1),('depth',1),('peak',5),('rounds',1),('faults',3),('now',True),('stamp0',1)]:
            s=baseline(); s[k]=v
            with self.assertRaises(ValueError): validate(s)
    def test_head_fault(self):
        s=baseline(); s.update(halted=1,valid=0,faults=1,rejected=1,attempts=1)
        m=Model('impossible head fault','replay'); m.feed(stream(s))
        self.assertEqual(m.errors,1)
    def test_halted_immutable(self):
        a=snapshot([(0,True),(0,False)])
        a.update(halted=1,valid=0,faults=1,rejected=1,attempts=3,levels=0)
        b=dict(a,now=a['now']+100,observed=a['observed']+100)
        m=Model('test','replay'); m.feed(stream(a,b)); self.assertIn('Halted evidence',m.frozen)
    def test_parser_recovery_tail_policy(self):
        m=Model('test','replay'); m.feed(b'x'*900+b'RQ1,S,1'+stream(baseline())); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest,baseline())
        n=m.errors; m.feed(b'RQ1,S,1*0000\n'); self.assertGreater(m.errors,n)
        m.feed(b'RQ1,S'); n=m.errors; m.finish(); self.assertEqual(m.errors,n+1)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,4,2,1])); self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(stream(baseline())+frame('B',[42])); self.assertTrue(m.frozen)
        m=Model('wrong protocol','replay'); m.feed(stream(baseline()).replace(b'RQ1,',b'RS1,'))
        self.assertFalse(m.policy)
    def test_live_stale_duplicate(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10): m.feed(stream(baseline()))
        b=dict(baseline(),now=120,observed=117)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(stream(b)); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(stream(b)); self.assertFalse(m.live())
            m.feed(stream(dict(b,now=220,observed=217))); self.assertTrue(m.frozen)
    def test_drift_and_rollback(self):
        m=Model('slow delivery','live'); m.connected=True
        for i in range(200):
            with patch('console.time.monotonic',return_value=10+i*.11):
                m.feed(stream(dict(baseline(),now=20+i*100,observed=17+i*100)))
            if m.frozen: break
        self.assertIn('progress diverged',m.frozen)
        a=snapshot([(0,True),(0,False)])
        b=dict(baseline(),now=a['now']+100,observed=a['observed']+100)
        m=Model('rollback','replay'); m.feed(stream(a,b)); self.assertIn('rollback',m.frozen)
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
    def test_screens_serial_and_export(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            os.write(master,stream(snapshot([(0,True)])))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertEqual(app.model.latest['use0'],1)
            self.assertIn('Eligible allocation tags: [1]',app.summary.get('1.0','end'))
            self.assertIn('Return-queue priority',app.summary.get('1.0','end'))
            self.assertEqual(len(app.tabs.tabs()),3)
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            app.connect(); self.assertIn('Disconnect',app.message)
            app.disconnect(); self.assertEqual(old,termios.tcgetattr(slave))
            app.connect(); os.close(master); master=None
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertFalse(app.model.connected)
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'fixture'
                p.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture','legal']))
                app.load(p); root.update()
                self.assertIn('Eligible allocation tags: [0]',app.summary.get('1.0','end'))
                self.assertEqual((app.model.errors,app.model.frozen),(0,''))
                self.assertEqual(app.history.get('1.0','end').count("'now':"),len(app.model.history))
                p.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture']))
                app.load(p); root.update()
                self.assertEqual(app.model.latest['faults'],1)
                self.assertIn('free-list head bypassed',app.summary.get('1.0','end'))
                retained=app.model
                with patch('console.os.open',side_effect=PermissionError('permission denied')): app.connect()
                self.assertIs(app.model,retained)
                app.path.set('/dev/null'); app.connect(); self.assertTrue(app.message)
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
